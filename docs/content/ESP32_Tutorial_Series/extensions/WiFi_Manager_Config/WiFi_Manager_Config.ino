/*
 * ESP32 扩展教程：WiFi_Manager_Config - 网页配网，无需重新烧录
 *
 * 学习目标：
 * 1. 理解为什么把 Wi-Fi 密码写死在代码里不方便
 * 2. 学会使用 WiFiManager 库创建网页配网门户
 * 3. 让 ESP32 记住 Wi-Fi 配置，断电后仍然有效
 *
 * 需要安装的库：
 * - 库名称：WiFiManager
 * - 作者：tzapu
 * - 安装方法：
 *   1. 在 Arduino IDE 中点击“项目”→“加载库”→“管理库...”。
 *   2. 在搜索框输入 "WiFiManager"。
 *   3. 找到作者为 tzapu 的 "WiFiManager"，点击“安装”。
 *   4. 如果提示安装依赖库，请选择“安装全部”。
 *
 * 前置知识：
 * - 建议先学习第 06 课（WiFi_Connect）和第 07 课（Web_Server_LED）。
 * - 了解 STA 模式、AP 模式、SSID、IP 地址的基本概念。
 *
 * 硬件连接：
 * - ESP32 GPIO2  →  LED 正极（长脚）
 * - LED 负极（短脚） →  330Ω 电阻  →  GND
 *
 * 为什么加 330Ω 电阻？
 * LED 导通后内阻很小，直接接 3.3V 会流过过大电流导致烧坏。
 * 330Ω 电阻把电流限制在约 10mA 以内，让 LED 安全工作。
 *
 * 使用步骤：
 * 1. 安装 WiFiManager 库，然后上传本代码到 ESP32。
 * 2. 第一次运行时，ESP32 找不到保存的 Wi-Fi 信息，会自动进入配网模式，
 *    发射一个名为 "ESP32_Config" 的热点。
 * 3. 用手机/电脑连接这个热点，浏览器会自动弹出配网页面
 *    （如果没有自动弹出，手动访问 http://192.168.4.1）。
 * 4. 在页面上选择你的家庭 Wi-Fi，输入密码，点击“保存”。
 * 5. ESP32 会自动重启并连接该 Wi-Fi，之后无需再次配网。
 */

#include <WiFi.h>
#include <WiFiManager.h>

// LED 引脚选择 GPIO2，这是大多数 ESP32 开发板板载 LED 所在引脚。
#define LED_PIN 2

// 创建 WiFiManager 对象。
// WiFiManager 是一个第三方库，它帮我们做了很多事：
// - 在无法连接已知 Wi-Fi 时自动开启 AP 热点；
// - 提供一个网页，让用户输入 Wi-Fi 名称和密码；
// - 把配置保存到 ESP32 的 Flash（NVS）中，断电不丢失。
WiFiManager wm;

// 标记 Wi-Fi 是否已成功连接。
bool wifiConnected = false;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("================================");
  Serial.println("ESP32 扩展教程：WiFiManager 配网");
  Serial.println("================================");

  // 初始化 LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // ===== WiFiManager 配置说明 =====
  // setConfigPortalTimeout(秒)：设置配网门户超时时间。
  // 如果 120 秒内用户没有完成配网，ESP32 会继续尝试连接或进入失败处理。
  // 这样可以避免设备永远停在配网模式而无法正常工作。
  wm.setConfigPortalTimeout(120);

  // setConnectTimeout(秒)：设置连接 Wi-Fi 的超时时间。
  // 超过这个时间还没连上，就认为该 Wi-Fi 不可用。
  wm.setConnectTimeout(10);

  // setDebugOutput(true)：打开 WiFiManager 的调试输出，
  // 方便我们在串口监视器里看它正在做什么。
  wm.setDebugOutput(true);

  // setAPCallback()：设置一个回调函数，当进入 AP 配网模式时调用。
  // 回调函数（Callback Function）就是“某个事件发生时自动执行的函数”。
  // 这里我们在进入配网模式时让 LED 慢闪，提示用户可以进行配网。
  wm.setAPCallback([](WiFiManager* myWm) {
    Serial.println("进入配网模式，请连接热点 ESP32_Config");
  });

  // setSaveConfigCallback()：设置保存配置后的回调函数。
  // 当用户在网页上点击保存后，这个函数会被调用。
  wm.setSaveConfigCallback([]() {
    Serial.println("用户已保存 Wi-Fi 配置，即将自动重启...");
  });

  // autoConnect() 是 WiFiManager 最核心的函数：
  // 1. 它先去 Flash 里找以前保存的 Wi-Fi 配置。
  // 2. 如果有配置，就尝试连接。
  // 3. 如果连不上（或没有配置），就开启 AP 热点 "ESP32_Config"，
  //    并启动网页配网门户。
  // 参数 "ESP32_Config" 是 AP 热点的名称，"12345678" 是密码。
  // 返回值 true 表示成功连接 Wi-Fi，false 表示连接失败。
  Serial.println("正在尝试自动连接 Wi-Fi...");
  wifiConnected = wm.autoConnect("ESP32_Config", "12345678");

  if (wifiConnected) {
    Serial.println("Wi-Fi 连接成功！");
    Serial.print("IP 地址：");
    Serial.println(WiFi.localIP());

    // 连接成功后点亮 LED，作为视觉提示。
    digitalWrite(LED_PIN, HIGH);
  } else {
    Serial.println("Wi-Fi 连接失败，未能完成配网");
    Serial.println("可能原因：");
    Serial.println("1. 配网超时，用户没有在规定时间内完成配置");
    Serial.println("2. 输入的 Wi-Fi 密码错误");
    Serial.println("3. 目标 Wi-Fi 信号太弱或不是 2.4GHz 频段");

    // 连接失败时让 LED 快闪，提示用户需要重新配网。
    // 通过反复写入高低电平实现闪烁。
    for (int i = 0; i < 10; i++) {
      digitalWrite(LED_PIN, HIGH);
      delay(100);
      digitalWrite(LED_PIN, LOW);
      delay(100);
    }
  }
}

void loop() {
  if (wifiConnected) {
    // 如果已经连上 Wi-Fi，可以在这里执行正常的网络任务，
    // 例如运行 Web 服务器、发送 HTTP 请求等。
    Serial.println("Wi-Fi 已连接，正在运行主程序...");
  } else {
    // 如果连接失败，可以在这里进入低功耗等待，或重新启动配网流程。
    Serial.println("Wi-Fi 未连接，请重置设备重新配网");
  }

  // 每 5 秒打印一次状态，方便观察。
  delay(5000);
}

/*
 * 扩展练习：
 * 1. 学习如何重置已保存的 Wi-Fi 配置：
 *    在 setup() 中加入一个按键检测，按下按键时调用 wm.resetSettings()，
 *    这样下次启动就会重新进入配网模式。
 * 2. 在配网成功后启动一个 Web 服务器，用手机浏览器控制 LED，
 *    把本节课和第 07 课的 Web 服务器结合起来。
 * 3. 给 WiFiManager 添加自定义参数，例如设备名称、MQTT 服务器地址等，
 *    实现更复杂的可配置物联网设备。
 */
