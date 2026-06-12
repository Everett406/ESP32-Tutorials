/*
 * 综合项目 P11：智能家居中心
 *
 * 学习目标：
 * 1. 把多个传感器（DHT22 温湿度、光敏电阻）整合到一个项目里
 * 2. 学会用 OLED 显示多行传感器数据
 * 3. 学会用 Web 服务器展示实时数据并接收用户设置
 * 4. 学会用 Preferences 把配置保存到 Flash，断电不丢失
 * 5. 理解"采集 → 处理 → 显示 → 存储"的完整数据流
 *
 * 涉及知识点：
 * - DHT22 温湿度传感器读取
 * - ADC 模拟输入与光敏电阻分压电路
 * - I2C OLED 显示（SSD1306）
 * - Wi-Fi 与 Web 服务器
 * - Preferences 非易失性存储
 *
 * 需要安装的库：
 * 1. "DHT sensor library" by Adafruit
 * 2. "Adafruit Unified Sensor" by Adafruit（DHT 库依赖）
 * 3. "Adafruit SSD1306" by Adafruit
 * 4. "Adafruit GFX Library" by Adafruit（SSD1306 依赖）
 * 安装方法：Arduino IDE → 项目 → 加载库 → 管理库，搜索并安装。
 *
 * 硬件连接：
 * - DHT22 VCC  →  ESP32 3.3V
 * - DHT22 GND  →  ESP32 GND
 * - DHT22 DATA →  ESP32 GPIO4（建议 DATA 与 3.3V 之间接 4.7kΩ~10kΩ 上拉电阻）
 *
 * - 光敏电阻一端 →  ESP32 3.3V
 * - 光敏电阻另一端 →  ESP32 GPIO34 → 10kΩ 电阻 →  GND
 *   这样 GPIO34 的电压会随光线强弱变化，光线越强电压越低。
 *
 * - OLED VCC → ESP32 3.3V
 * - OLED GND → ESP32 GND
 * - OLED SDA → ESP32 GPIO21（默认 I2C 数据线）
 * - OLED SCL → ESP32 GPIO22（默认 I2C 时钟线）
 *
 * - ESP32 GPIO2 → LED 正极（长脚）
 * - LED 负极（短脚） → 330Ω 电阻 → GND
 *   这颗 LED 作为报警指示灯，当温度/湿度/光照超过阈值时点亮。
 */

// 引入必要的库
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ===== 修改这里：填入你的 Wi-Fi 信息 =====
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// 引脚定义
#define DHT_PIN 4       // DHT22 数据引脚
#define DHT_TYPE DHT22  // 传感器类型，如果手头是 DHT11 请改成 DHT11
#define LDR_PIN 34      // 光敏电阻分压后接入的 ADC 引脚，ADC1 通道，不受 Wi-Fi 影响
#define LED_PIN 2       // 报警指示灯

// OLED 参数
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1           // 大多数模块没有独立复位引脚，用 -1 表示软件复位
#define SCREEN_ADDRESS 0x3C     // OLED 常见 I2C 地址，也可能是 0x3D

// 创建对象
DHT dht(DHT_PIN, DHT_TYPE);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
WebServer server(80);
Preferences prefs;

// OLED 是否初始化成功
bool oledReady = false;

// Preferences 命名空间和键名
// 命名空间相当于 Flash 里的一个"文件夹"，把相关配置放在一起
const char* PREF_NS = "shc";
const char* KEY_TEMP_TH = "tempTh";
const char* KEY_HUM_TH = "humTh";
const char* KEY_LIGHT_TH = "lightTh";

// 报警阈值
float tempThreshold = 30.0;   // 温度高于此值报警
float humThreshold = 80.0;    // 湿度高于此值报警
int lightThreshold = 30;      // 光照百分比低于此值报警（越暗数值越小）

// 当前传感器数据
float temperature = 0.0;
float humidity = 0.0;
int lightRaw = 0;             // ADC 原始值 0~4095
int lightPercent = 0;         // 转换后的亮度百分比 0~100

// 报警状态
bool tempAlarm = false;
bool humAlarm = false;
bool lightAlarm = false;

// 时间戳，用于非阻塞式定时任务
unsigned long lastSensorRead = 0;
unsigned long lastOledUpdate = 0;
const unsigned long SENSOR_INTERVAL = 2500;  // DHT22 建议两次读取间隔不少于 2 秒
const unsigned long OLED_INTERVAL = 500;     // OLED 刷新周期

// 函数前置声明
void loadThresholds();
void saveThresholds();
void readSensors();
void updateOLED();
void updateAlarmLED();
String generateHTML();
void handleRoot();
void handleSet();
void handleData();

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("================================");
  Serial.println("综合项目 P11：智能家居中心");
  Serial.println("================================");

  // 初始化引脚
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // 设置 ADC 分辨率。ESP32 默认 12 bit，这里显式写出更清晰
  analogReadResolution(12);

  // 初始化 DHT 传感器
  dht.begin();

  // 初始化 OLED
  // SSD1306_SWITCHCAPVCC 表示使用内部电荷泵升压，只需要外部供 3.3V
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("OLED 初始化失败，请检查 I2C 地址和接线！");
    // OLED 失败不影响 Web 和传感器功能，所以不进入死循环
  } else {
    oledReady = true;
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("Smart Home Center");
    display.println("Booting...");
    display.display();
  }

  // 从 Flash 读取保存的阈值。如果第一次运行没有数据，就使用上面的默认值
  loadThresholds();

  // 连接 Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("正在连接 Wi-Fi");
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 30) {
    delay(500);
    Serial.print(".");
    retry++;
  }
  Serial.println();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi 连接失败，请检查 ssid 和密码");
  } else {
    Serial.print("Wi-Fi 已连接，IP 地址：");
    Serial.println(WiFi.localIP());

    // 注册 Web 路由
    server.on("/", handleRoot);
    server.on("/set", handleSet);
    server.on("/data", handleData);
    server.begin();
    Serial.println("HTTP 服务器已启动");
  }
}

void loop() {
  // 处理浏览器请求
  server.handleClient();

  // 周期性读取传感器
  if (millis() - lastSensorRead > SENSOR_INTERVAL) {
    lastSensorRead = millis();
    readSensors();
    updateAlarmLED();
  }

  // 周期性刷新 OLED
  if (millis() - lastOledUpdate > OLED_INTERVAL) {
    lastOledUpdate = millis();
    updateOLED();
  }
}

// 从 Preferences 加载阈值
void loadThresholds() {
  // 以只读模式打开命名空间
  prefs.begin(PREF_NS, false);

  // getFloat(key, default)：如果键不存在就返回默认值
  tempThreshold = prefs.getFloat(KEY_TEMP_TH, 30.0);
  humThreshold = prefs.getFloat(KEY_HUM_TH, 80.0);
  lightThreshold = prefs.getInt(KEY_LIGHT_TH, 30);

  prefs.end();

  Serial.println("已从 Flash 加载报警阈值：");
  Serial.print("  温度 > ");
  Serial.print(tempThreshold);
  Serial.println(" ℃");
  Serial.print("  湿度 > ");
  Serial.print(humThreshold);
  Serial.println(" %");
  Serial.print("  光照 < ");
  Serial.print(lightThreshold);
  Serial.println(" %");
}

// 把阈值保存到 Preferences
void saveThresholds() {
  // 以读写模式打开命名空间
  prefs.begin(PREF_NS, true);

  prefs.putFloat(KEY_TEMP_TH, tempThreshold);
  prefs.putFloat(KEY_HUM_TH, humThreshold);
  prefs.putInt(KEY_LIGHT_TH, lightThreshold);

  prefs.end();

  Serial.println("报警阈值已保存到 Flash");
}

// 读取 DHT22 和光敏电阻
void readSensors() {
  // 读取温湿度
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  // 如果读数有效，更新全局变量
  // isnan() 用于判断是否为"非数字"，传感器未接好或读取间隔太短时会出现
  if (!isnan(h) && !isnan(t)) {
    humidity = h;
    temperature = t;
  } else {
    Serial.println("DHT22 读取失败，将保持上次数值");
  }

  // 读取光敏电阻 ADC 值
  // 这里连续读 10 次取平均，减少噪声
  long sum = 0;
  for (int i = 0; i < 10; i++) {
    sum += analogRead(LDR_PIN);
    delay(5);
  }
  lightRaw = sum / 10;

  // 把 ADC 原始值映射为亮度百分比
  // 接线方式：光线越强，光敏电阻阻值越小，GPIO34 电压越低，ADC 值越小
  // 所以映射时把 0~4095 反过来映射到 100~0，这样"百分比"越大代表越亮
  lightPercent = map(lightRaw, 0, 4095, 100, 0);
  lightPercent = constrain(lightPercent, 0, 100);

  // 判断报警
  tempAlarm = (temperature > tempThreshold);
  humAlarm = (humidity > humThreshold);
  lightAlarm = (lightPercent < lightThreshold);

  // 打印到串口，方便调试
  Serial.print("温度：");
  Serial.print(temperature);
  Serial.print(" ℃  湿度：");
  Serial.print(humidity);
  Serial.print(" %  光照：");
  Serial.print(lightPercent);
  Serial.print(" % (ADC:");
  Serial.print(lightRaw);
  Serial.println(")");
}

// 根据报警状态点亮或熄灭 LED
void updateAlarmLED() {
  // 任一条件报警就点亮 LED
  if (tempAlarm || humAlarm || lightAlarm) {
    digitalWrite(LED_PIN, HIGH);
  } else {
    digitalWrite(LED_PIN, LOW);
  }
}

// 刷新 OLED 显示
void updateOLED() {
  // 如果 OLED 初始化失败，直接返回，避免调用未初始化的对象导致异常
  if (!oledReady) {
    return;
  }

  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Smart Home Center");

  // 显示温湿度，保留 1 位小数
  display.setCursor(0, 12);
  display.print("T: ");
  display.print(temperature, 1);
  display.print((char)247);  // 度符号 °
  display.println("C");

  display.print("H: ");
  display.print(humidity, 1);
  display.println(" %");

  display.print("L: ");
  display.print(lightPercent);
  display.println(" %");

  // 显示报警状态
  display.setCursor(0, 44);
  if (tempAlarm || humAlarm || lightAlarm) {
    display.println("ALARM!");
    if (tempAlarm) display.println(" Temp High");
    if (humAlarm)  display.println(" Hum High");
    if (lightAlarm) display.println(" Dark");
  } else {
    display.println("Status: OK");
  }

  display.display();
}

// 生成 Web 页面
String generateHTML() {
  String html = "<!DOCTYPE html><html lang='zh-CN'>";
  html += "<head><meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>智能家居中心</title>";
  html += "<style>";
  html += "body{font-family:'Microsoft YaHei',Arial,sans-serif;background:#f5f5f5;padding:20px;}";
  html += ".container{max-width:500px;margin:0 auto;background:white;padding:25px;border-radius:15px;box-shadow:0 4px 15px rgba(0,0,0,0.1);}";
  html += "h1{text-align:center;color:#333;}";
  html += ".data{font-size:18px;line-height:1.8;}";
  html += ".alarm{color:#f44336;font-weight:bold;}";
  html += ".ok{color:#4CAF50;font-weight:bold;}";
  html += "form{margin-top:20px;padding-top:15px;border-top:1px solid #eee;}";
  html += "label{display:inline-block;width:120px;}";
  html += "input{width:120px;padding:5px;margin:5px 0;}";
  html += "button{padding:8px 20px;background:#2196F3;color:white;border:none;border-radius:5px;cursor:pointer;}";
  html += "</style></head><body>";
  html += "<div class='container'>";
  html += "<h1>智能家居中心</h1>";

  // 实时数据
  html += "<div class='data'>";
  html += "<p>温度：" + String(temperature, 1) + " ℃</p>";
  html += "<p>湿度：" + String(humidity, 1) + " %</p>";
  html += "<p>光照：" + String(lightPercent) + " %</p>";

  if (tempAlarm || humAlarm || lightAlarm) {
    html += "<p class='alarm'>状态：报警中</p>";
  } else {
    html += "<p class='ok'>状态：正常</p>";
  }
  html += "</div>";

  // 阈值设置表单
  html += "<form action='/set' method='GET'>";
  html += "<h3>报警阈值设置</h3>";
  html += "<label>温度上限 (℃)</label>";
  html += "<input type='number' name='temp' step='0.1' value='" + String(tempThreshold, 1) + "'><br>";
  html += "<label>湿度上限 (%)</label>";
  html += "<input type='number' name='hum' step='0.1' value='" + String(humThreshold, 1) + "'><br>";
  html += "<label>光照下限 (%)</label>";
  html += "<input type='number' name='light' value='" + String(lightThreshold) + "'><br>";
  html += "<button type='submit'>保存阈值</button>";
  html += "</form>";

  html += "<p style='font-size:12px;color:#999;margin-top:20px;'>阈值保存后会写入 Flash，断电不丢失。</p>";
  html += "</div></body></html>";
  return html;
}

// 首页
void handleRoot() {
  server.send(200, "text/html; charset=utf-8", generateHTML());
}

// 设置阈值
void handleSet() {
  bool changed = false;

  if (server.hasArg("temp")) {
    tempThreshold = server.arg("temp").toFloat();
    changed = true;
  }
  if (server.hasArg("hum")) {
    humThreshold = server.arg("hum").toFloat();
    changed = true;
  }
  if (server.hasArg("light")) {
    lightThreshold = server.arg("light").toInt();
    changed = true;
  }

  if (changed) {
    saveThresholds();
    // 重新判断报警状态，让 LED 立即响应新阈值
    readSensors();
    updateAlarmLED();
  }

  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "");
}

// JSON 数据接口，供其他程序读取
void handleData() {
  String json = "{";
  json += "\"temperature\":" + String(temperature, 1) + ",";
  json += "\"humidity\":" + String(humidity, 1) + ",";
  json += "\"light\":" + String(lightPercent) + ",";
  json += "\"tempAlarm\":" + String(tempAlarm ? "true" : "false") + ",";
  json += "\"humAlarm\":" + String(humAlarm ? "true" : "false") + ",";
  json += "\"lightAlarm\":" + String(lightAlarm ? "true" : "false");
  json += "}";

  server.send(200, "application/json; charset=utf-8", json);
}

/*
 * 扩展挑战：
 * 1. 把报警信息通过 HTTP 推送到手机通知，或接入 Bark、PushPlus 等推送服务。
 * 2. 增加一个继电器/蜂鸣器，报警时自动打开风扇或发出声音。
 * 3. 用 ECharts 在网页上绘制温湿度折线图，需要把历史数据保存到数组或 Flash。
 * 4. 增加定时任务：每天晚上 22:00 自动把 OLED 亮度调低或关闭背光，避免影响休息。
 */
