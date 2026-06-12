/**
 * 综合项目 P14：低功耗环境节点（Low Power Environment Node）
 *
 * 学习目标：
 * 1. 把 Deep Sleep、RTC 内存、Preferences、Wi-Fi、HTTP 客户端、DHT22 整合成一个电池供电的物联网节点。
 * 2. 理解 Deep Sleep 的工作方式：进入睡眠后 CPU 停止，唤醒后程序从 setup() 重新开始。
 * 3. 理解 RTC 内存与普通 RAM 的区别：RTC 内存能在 Deep Sleep 期间保持数据。
 * 4. 学会用 Preferences 保存需要跨断电保留的配置和计数。
 * 5. 学会设计“采集 → 上报 → 睡眠”的省电循环，让电池续航从几天延长到数月。
 *
 * 前置知识：
 * - 已学习《DHT11_DHT22_Sensor》《15_Deep_Sleep》《13_Preferences_Storage》《14_HTTP_Client》《06_WiFi_Connect》。
 *
 * 硬件连接：
 * - DHT22 VCC  →  ESP32 3.3V
 * - DHT22 GND  →  ESP32 GND
 * - DHT22 DATA →  ESP32 GPIO4（建议 DATA 与 3.3V 之间接 4.7kΩ ~ 10kΩ 上拉电阻）
 *
 * 使用步骤：
 * 1. 安装 DHT sensor library（Adafruit）和 Adafruit Unified Sensor。
 * 2. 修改下方的 WIFI_SSID / WIFI_PASSWORD 为你自己的 2.4GHz Wi-Fi。
 * 3. 修改 SERVER_URL 为你的 HTTP 服务器地址，或者在串口通过 config 命令写入 Preferences。
 * 4. 上传程序，打开串口监视器（波特率 115200），观察唤醒、上报、睡眠的完整流程。
 *
 * 关键概念解释：
 *
 * 【Deep Sleep 深度睡眠】
 * ESP32 进入 Deep Sleep 后，CPU 和大部分外设会关闭，只保留 RTC 内存和 RTC 外设。
 * 功耗可以从正常运行时的 100mA 左右降到 10~150μA，非常适合电池供电。
 * 唤醒方式包括定时器唤醒、GPIO 外部唤醒、触摸唤醒、ULP 唤醒等。
 * 唤醒后 ESP32 会复位，程序从 setup() 重新开始执行。
 *
 * 【RTC 内存】
 * RTC（Real-Time Clock）内存是 ESP32 内部一块特殊的存储区域，
 * 在 Deep Sleep 期间不会被断电清除。
 * 用 `RTC_DATA_ATTR` 修饰的变量会存放在这里，可以用来记录“第几次启动”。
 * 注意：RTC 内存只在本轮供电周期内保持，如果拔掉电源或按下复位键，数据会丢失。
 *
 * 【Preferences / NVS】
 * Preferences 是 ESP32 Arduino 封装好的键值对存储库，底层是 NVS（Non-Volatile Storage）。
 * 与 RTC 内存不同，NVS 写在 Flash 中，断电后数据仍然保留。
 * 本项目用它保存服务器地址和累计上报次数。
 *
 * 【HTTP GET 上报】
 * 把传感器数据附加到 URL 参数中，例如：
 * http://192.168.1.100:8080/upload?temp=25.3&hum=58.1&boot=12
 * 服务器收到 GET 请求后可以从 URL 中解析出数据。
 */

#include <WiFi.h>        // Wi-Fi 连接
#include <HTTPClient.h>  // 发送 HTTP 请求
#include <Preferences.h> // Flash 键值对存储
#include "DHT.h"         // DHT 温湿度传感器

// ===== 修改这里：填入你的 Wi-Fi 信息 =====
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// 默认服务器地址。可以通过串口命令修改并保存到 Preferences。
// 示例格式：http://192.168.1.100:8080/upload
const char* DEFAULT_SERVER_URL = "http://192.168.1.100:8080/upload";

// DHT22 引脚与类型
#define DHT_PIN  4
#define DHT_TYPE DHT22

// 深度睡眠时间：10 分钟 = 10 * 60 秒 = 600 秒 = 600,000,000 微秒
// 为什么要用微秒？因为 esp_sleep_enable_timer_wakeup() 的参数单位就是微秒。
#define SLEEP_TIME_US 600000000ULL

// Wi-Fi 最大重试次数，约 15 秒
#define MAX_WIFI_RETRY 30

// 创建 DHT 对象
DHT dht(DHT_PIN, DHT_TYPE);

// 创建 Preferences 对象
Preferences prefs;

// 命名空间，相当于在 Flash 中建立一个独立文件夹
const char* NAMESPACE = "envNode";

// 键名
const char* KEY_SERVER_URL = "serverUrl";
const char* KEY_TOTAL_UPLOADS = "totalUploads";

// RTC 内存变量：记录本次供电周期内的启动次数。
// RTC_DATA_ATTR 是关键，没有这个修饰符，变量在 Deep Sleep 后会被清空。
RTC_DATA_ATTR int bootCount = 0;

// 保存从 Preferences 读取到的服务器地址
String serverUrl;

/**
 * 打印本次唤醒原因。
 * 在调试时非常有用，可以帮助我们判断是定时器唤醒、按键唤醒还是普通复位。
 */
void printWakeupReason() {
  esp_sleep_wakeup_cause_t reason = esp_sleep_get_wakeup_cause();
  Serial.print("唤醒原因：");
  switch (reason) {
    case ESP_SLEEP_WAKEUP_TIMER:
      Serial.println("定时器唤醒");
      break;
    case ESP_SLEEP_WAKEUP_EXT0:
      Serial.println("外部 GPIO 唤醒（EXT0）");
      break;
    case ESP_SLEEP_WAKEUP_EXT1:
      Serial.println("外部 GPIO 组合唤醒（EXT1）");
      break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD:
      Serial.println("触摸唤醒");
      break;
    case ESP_SLEEP_WAKEUP_ULP:
      Serial.println("ULP 协处理器唤醒");
      break;
    default:
      Serial.println("普通开机或外部复位");
      break;
  }
}

/**
 * 从 Preferences 读取服务器地址和累计上报次数。
 * 如果服务器地址不存在，就写入默认值。
 */
void loadConfig() {
  // 以只读模式打开命名空间
  prefs.begin(NAMESPACE, false);

  // getString(key, defaultValue) 读取字符串，不存在时返回默认值
  serverUrl = prefs.getString(KEY_SERVER_URL, DEFAULT_SERVER_URL);

  // getULong 读取无符号长整型，不存在时返回默认值 0
  unsigned long totalUploads = prefs.getULong(KEY_TOTAL_UPLOADS, 0);

  prefs.end();

  Serial.print("目标服务器：");
  Serial.println(serverUrl);
  Serial.print("历史累计上报次数：");
  Serial.println(totalUploads);
}

/**
 * 把服务器地址保存到 Preferences。
 * 通过串口输入 config http://... 可以修改目标服务器。
 */
void saveServerUrl(const String& url) {
  // 读写模式打开
  prefs.begin(NAMESPACE, true);
  prefs.putString(KEY_SERVER_URL, url);
  prefs.end();
  serverUrl = url;
  Serial.print("服务器地址已保存：");
  Serial.println(serverUrl);
}

/**
 * 把累计上报次数加 1 并保存到 Preferences。
 */
void incrementTotalUploads() {
  prefs.begin(NAMESPACE, false);
  unsigned long totalUploads = prefs.getULong(KEY_TOTAL_UPLOADS, 0);
  totalUploads++;
  prefs.end();

  prefs.begin(NAMESPACE, true);
  prefs.putULong(KEY_TOTAL_UPLOADS, totalUploads);
  prefs.end();

  Serial.print("累计上报次数已更新为：");
  Serial.println(totalUploads);
}

/**
 * 连接 Wi-Fi。
 * 返回 true 表示连接成功，false 表示失败。
 */
bool connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("正在连接 Wi-Fi");
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < MAX_WIFI_RETRY) {
    delay(500);
    Serial.print(".");
    retry++;
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Wi-Fi 已连接，IP 地址：");
    Serial.println(WiFi.localIP());
    return true;
  } else {
    Serial.println("Wi-Fi 连接失败");
    return false;
  }
}

/**
 * 通过 HTTP GET 把数据发送到服务器。
 * 参数：
 *   temp - 温度
 *   hum  - 湿度
 *   boot - 本次供电周期内的启动次数
 * 返回 true 表示服务器返回 200，false 表示失败。
 */
bool uploadData(float temp, float hum, int boot) {
  // 检查 Wi-Fi 是否连接
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi 未连接，跳过上报");
    return false;
  }

  HTTPClient http;

  // 构造带参数的 URL
  // 例如：http://192.168.1.100:8080/upload?temp=25.30&hum=58.10&boot=5
  String url = serverUrl;
  url += "?temp=" + String(temp, 2);
  url += "&hum=" + String(hum, 2);
  url += "&boot=" + String(boot);

  Serial.print("正在上报：");
  Serial.println(url);

  http.begin(url);
  http.addHeader("User-Agent", "ESP32-EnvNode");

  int httpCode = http.GET();
  Serial.print("HTTP 状态码：");
  Serial.println(httpCode);

  if (httpCode == 200) {
    String response = http.getString();
    Serial.print("服务器响应：");
    Serial.println(response);
    http.end();
    return true;
  } else {
    Serial.println("上报失败");
    http.end();
    return false;
  }
}

/**
 * 进入深度睡眠。
 * 在调用之前会先把串口缓冲区数据发送完，并配置定时器唤醒。
 */
void goToDeepSleep() {
  Serial.print("即将进入 Deep Sleep，");
  Serial.print(SLEEP_TIME_US / 1000000UL);
  Serial.println(" 秒后自动唤醒...");

  // Serial.flush() 等待串口数据发送完成，避免进入睡眠后输出丢失
  Serial.flush();

  // 配置定时器唤醒，单位微秒
  esp_sleep_enable_timer_wakeup(SLEEP_TIME_US);

  // 进入深度睡眠。执行到这里后程序停止，到时间后自动复位并重新执行 setup()
  esp_deep_sleep_start();
}

/**
 * 处理串口命令。
 * 由于程序大部分时间都在睡觉，串口命令只在唤醒后的短时间内有效。
 * 可用命令：
 *   config <url>  - 修改并保存目标服务器地址
 *   resetcount    - 把累计上报次数清零
 *   now           - 立即执行一次上报（不进入睡眠，调试用）
 */
void handleSerialCommands() {
  if (!Serial.available()) return;

  String cmd = Serial.readStringUntil('\n');
  cmd.trim();

  if (cmd.startsWith("config ")) {
    String url = cmd.substring(7);  // 去掉 "config " 前缀
    url.trim();
    if (url.length() > 0) {
      saveServerUrl(url);
    } else {
      Serial.println("用法：config http://your-server/upload");
    }
  } else if (cmd == "resetcount") {
    prefs.begin(NAMESPACE, true);
    prefs.putULong(KEY_TOTAL_UPLOADS, 0);
    prefs.end();
    Serial.println("累计上报次数已清零");
  } else if (cmd == "now") {
    Serial.println("立即执行一次采集和上报...");
    float hum = dht.readHumidity();
    float temp = dht.readTemperature();
    if (!isnan(hum) && !isnan(temp)) {
      if (connectWiFi()) {
        uploadData(temp, hum, bootCount);
      }
    } else {
      Serial.println("传感器读取失败");
    }
  } else {
    Serial.println("未知命令。可用：config <url>, resetcount, now");
  }

  // 留出一点时间让用户看到输出
  delay(500);
}

void setup() {
  Serial.begin(115200);

  // 从 Deep Sleep 唤醒后串口需要一点时间准备
  delay(1000);

  // bootCount 加 1。每次启动都会执行这里。
  bootCount++;

  Serial.println("================================");
  Serial.println("综合项目 P14：低功耗环境节点");
  Serial.println("================================");

  printWakeupReason();

  Serial.print("本次供电周期内第 ");
  Serial.print(bootCount);
  Serial.println(" 次启动");

  // 初始化 DHT 传感器
  dht.begin();

  // 加载配置（服务器地址、累计上报次数）
  loadConfig();

  // 读取温湿度
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("DHT22 读取失败，本次不上报");
    // 即使读取失败也进入睡眠，避免一直耗电
    goToDeepSleep();
    return;  // 实际不会执行到这里
  }

  Serial.print("温度：");
  Serial.print(temperature, 2);
  Serial.print(" ℃，湿度：");
  Serial.print(humidity, 2);
  Serial.println(" %");

  // 尝试连接 Wi-Fi 并上报
  if (connectWiFi()) {
    if (uploadData(temperature, humidity, bootCount)) {
      incrementTotalUploads();
    }
  }

  // 短暂等待串口命令（调试用），最多 5 秒
  Serial.println("等待串口命令（5 秒）...");
  unsigned long cmdStart = millis();
  while (millis() - cmdStart < 5000) {
    handleSerialCommands();
    delay(10);
  }

  // 进入深度睡眠
  goToDeepSleep();
}

void loop() {
  // 进入 Deep Sleep 后 CPU 会停止，loop() 不会执行。
  // 即使从睡眠唤醒，ESP32 也会复位重新进入 setup()。
}

/**
 * 扩展练习：
 * 1. 增加一个按键连接到 GPIO4 以外的引脚（如 GPIO33），
 *    用 esp_sleep_enable_ext0_wakeup() 实现按键手动唤醒，方便即时采集。
 * 2. 在服务器端用 Python/Node.js 写一个接收端，把 GET 参数保存到数据库或 CSV 文件。
 * 3. 把 SLEEP_TIME_US 改成 1 小时或更长，用 18650 电池测试实际续航时间。
 * 4. 在上报失败后把数据暂存到 RTC 内存或 Preferences，下次唤醒时批量上报，避免数据丢失。
 */
