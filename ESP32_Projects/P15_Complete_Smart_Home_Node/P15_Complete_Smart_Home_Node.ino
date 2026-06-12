/**
 * 综合项目 P15：完整智能家居节点（Complete Smart Home Node）
 *
 * 学习目标：
 * 1. 把多传感器、OLED 显示屏、Web 服务器、MQTT、OTA、WiFiManager、FreeRTOS 整合到一个项目中。
 * 2. 理解“多任务”在智能家居节点中的意义：传感器采集、屏幕刷新、网络通信互不阻塞。
 * 3. 学会用 FreeRTOS 的互斥锁（Mutex）保护共享数据，避免多任务同时读写导致错误。
 * 4. 理解 MQTT 发布/订阅模型，实现远程控制和数据上报。
 * 5. 理解 OTA（Over-The-Air）无线升级的作用和基本用法。
 * 6. 学会用 WiFiManager 实现无需重新烧录的 Wi-Fi 配置。
 *
 * 前置知识：
 * - 已学习《DHT11_DHT22_Sensor》《I2C_OLED_SSD1306_Display》《07_Web_Server_LED》
 *   《12_FreeRTOS_Tasks》《13_Preferences_Storage》《extensions/WiFi_Manager_Config》
 *   《extensions/mDNS_Web_Server》以及 MQTT、OTA 相关基础知识。
 *
 * 硬件连接：
 * - DHT22 VCC  →  ESP32 3.3V
 * - DHT22 GND  →  ESP32 GND
 * - DHT22 DATA →  ESP32 GPIO4
 * - 光敏电阻一端 →  ESP32 3.3V
 * - 光敏电阻另一端 →  ESP32 GPIO34 → 10kΩ 电阻 → GND（分压电路）
 * - OLED VCC   →  ESP32 3.3V
 * - OLED GND   →  ESP32 GND
 * - OLED SDA   →  ESP32 GPIO21（默认 I2C 数据线）
 * - OLED SCL   →  ESP32 GPIO22（默认 I2C 时钟线）
 * - 继电器 VCC →  ESP32 3.3V 或 5V（视模块而定）
 * - 继电器 GND →  ESP32 GND
 * - 继电器 IN  →  ESP32 GPIO5
 * - LED 正极   →  ESP32 GPIO2
 * - LED 负极   →  330Ω 电阻 → GND
 *
 * 需要安装的第三方库：
 * - DHT sensor library（Adafruit）
 * - Adafruit Unified Sensor
 * - Adafruit SSD1306
 * - Adafruit GFX Library
 * - WiFiManager（tzapu）
 * - PubSubClient（knolleary）
 * - ArduinoOTA（ESP32 核心自带）
 *
 * 关键概念解释：
 *
 * 【MQTT】
 * MQTT（Message Queuing Telemetry Transport）是一种轻量级的发布/订阅消息协议，
 * 非常适合物联网设备。设备把数据“发布”到一个主题（Topic），
 * 服务器（Broker）负责把消息转发给所有“订阅”了该主题的客户端。
 * 本项目发布主题：esp32/sensor
 * 本项目订阅主题：esp32/cmd（用于接收远程命令）
 *
 * 【OTA】
 * OTA（Over-The-Air）无线升级，指不需要 USB 线，直接通过 Wi-Fi 把新程序上传到 ESP32。
 * 在 Arduino IDE 中，上传端口会出现一个网络端口，选择它即可像普通串口一样上传。
 *
 * 【WiFiManager】
 * 第一次运行时如果 Flash 中没有保存的 Wi-Fi 信息，ESP32 会自动开启一个名为
 * "ESP32_SmartHome" 的热点。手机连接后访问 192.168.4.1 即可配置 Wi-Fi。
 * 配置成功后会自动保存到 Flash，以后无需再次配网。
 *
 * 【FreeRTOS 互斥锁 Mutex】
 * 多个任务同时读写同一个变量时，可能读到“一半新、一半旧”的数据，造成显示或网络异常。
 * Mutex（互斥锁）可以保证同一时间只有一个任务访问共享数据。
 */

#include <WiFi.h>             // Wi-Fi 核心库
#include <WiFiManager.h>      // 网页配网库
#include <WebServer.h>        // Web 服务器库
#include <PubSubClient.h>     // MQTT 客户端库
#include <ArduinoOTA.h>       // OTA 无线升级库
#include <Preferences.h>      // Flash 键值对存储
#include <Wire.h>             // I2C 通信
#include <Adafruit_GFX.h>     // OLED 图形库
#include <Adafruit_SSD1306.h> // OLED 驱动库
#include "DHT.h"              // DHT 温湿度库

// ========== 引脚与硬件参数 ==========
#define DHT_PIN      4     // DHT22 数据引脚
#define DHT_TYPE     DHT22 // 传感器类型
#define LDR_PIN      34    // 光敏电阻分压输出，接 ADC1 的 GPIO34
#define LED_PIN      2     // LED 引脚，支持 PWM
#define RELAY_PIN    5     // 继电器控制引脚

// OLED 参数
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1   // 无独立复位引脚
#define SCREEN_ADDRESS 0x3C

// PWM 参数
#define PWM_FREQ       5000
#define PWM_RESOLUTION 8

// MQTT 默认参数
const char* DEFAULT_MQTT_SERVER = "192.168.1.100";
const int   MQTT_PORT = 1883;
const char* MQTT_CLIENT_ID = "ESP32_SmartHome";
const char* TOPIC_SENSOR = "esp32/sensor";
const char* TOPIC_CMD    = "esp32/cmd";

// ========== 全局对象 ==========
DHT dht(DHT_PIN, DHT_TYPE);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
WiFiManager wm;
WebServer server(80);
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);
Preferences prefs;

// 共享数据结构，所有任务都会读写它
struct SensorData {
  float temperature;
  float humidity;
  int   light;        // 光敏电阻 ADC 原始值 0~4095
  bool  ledOn;        // LED 开关状态
  int   ledDuty;      // LED PWM 占空比 0~255
  bool  relayOn;      // 继电器开关状态
  bool  wifiConnected;
  bool  mqttConnected;
  unsigned long lastSensorUpdate; // 上次传感器更新时间
};

SensorData sensorData = {0, 0, 0, false, 128, false, false, false, 0};

// 互斥锁，保护 sensorData
SemaphoreHandle_t dataMutex = NULL;

// OLED 是否初始化成功
bool oledReady = false;

// MQTT 服务器地址，从 Preferences 读取
String mqttServer;

// 网络任务中的时间记录
unsigned long lastMqttPublish = 0;
const unsigned long MQTT_PUBLISH_INTERVAL = 10000; // 每 10 秒发布一次

// ========== 工具函数 ==========

/**
 * 获取互斥锁并复制一份共享数据。
 * 为什么需要复制？因为生成网页或发送 MQTT 时可能耗时较长，
 * 如果一直占用锁，其他任务会被阻塞。先复制到本地变量再释放锁更合理。
 */
SensorData getSensorDataCopy() {
  SensorData copy;
  xSemaphoreTake(dataMutex, portMAX_DELAY);
  copy = sensorData;
  xSemaphoreGive(dataMutex);
  return copy;
}

/**
 * 初始化硬件：LED PWM、继电器、DHT、OLED。
 */
void initHardware() {
  // LED 使用 PWM 输出
  ledcAttach(LED_PIN, PWM_FREQ, PWM_RESOLUTION);
  ledcWrite(LED_PIN, 0);

  // 继电器初始关闭
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  // DHT 初始化
  dht.begin();

  // OLED 初始化
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("OLED 初始化失败，请检查 I2C 地址和接线");
    // OLED 失败后不影响其他功能，只是没有显示
    oledReady = false;
  } else {
    oledReady = true;
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("Smart Home Node");
    display.display();
  }
}

/**
 * 从 Preferences 加载配置。
 * 这里保存 MQTT 服务器地址，避免每次都要重新烧录程序。
 */
void loadConfig() {
  prefs.begin("smartHome", false);
  mqttServer = prefs.getString("mqttServer", DEFAULT_MQTT_SERVER);
  prefs.end();

  Serial.print("MQTT 服务器：");
  Serial.println(mqttServer);
}

/**
 * 保存 MQTT 服务器地址到 Preferences。
 */
void saveMqttServer(const String& server) {
  prefs.begin("smartHome", true);
  prefs.putString("mqttServer", server);
  prefs.end();
  mqttServer = server;

  // 更新 MQTT 客户端连接地址
  mqttClient.setServer(mqttServer.c_str(), MQTT_PORT);

  Serial.print("MQTT 服务器已保存：");
  Serial.println(mqttServer);
}

// ========== LED 与继电器控制 ==========

void setLed(bool on) {
  xSemaphoreTake(dataMutex, portMAX_DELAY);
  sensorData.ledOn = on;
  ledcWrite(LED_PIN, on ? sensorData.ledDuty : 0);
  xSemaphoreGive(dataMutex);

  Serial.print("LED 状态：");
  Serial.println(on ? "开" : "关");
}

void setLedDuty(int duty) {
  if (duty < 0) duty = 0;
  if (duty > 255) duty = 255;

  xSemaphoreTake(dataMutex, portMAX_DELAY);
  sensorData.ledDuty = duty;
  if (sensorData.ledOn) {
    ledcWrite(LED_PIN, duty);
  }
  xSemaphoreGive(dataMutex);

  Serial.print("LED 亮度：");
  Serial.println(duty);
}

void setRelay(bool on) {
  xSemaphoreTake(dataMutex, portMAX_DELAY);
  sensorData.relayOn = on;
  digitalWrite(RELAY_PIN, on ? HIGH : LOW);
  xSemaphoreGive(dataMutex);

  Serial.print("继电器状态：");
  Serial.println(on ? "开" : "关");
}

// ========== 网络与配网 ==========

bool setupWiFi() {
  Serial.println("正在启动 WiFiManager...");

  // 配网超时 120 秒
  wm.setConfigPortalTimeout(120);
  wm.setConnectTimeout(10);
  wm.setDebugOutput(true);

  // autoConnect 会优先连接已保存的 Wi-Fi；失败则开启热点
  bool connected = wm.autoConnect("ESP32_SmartHome", "12345678");

  if (connected) {
    Serial.print("Wi-Fi 已连接，IP：");
    Serial.println(WiFi.localIP());

    xSemaphoreTake(dataMutex, portMAX_DELAY);
    sensorData.wifiConnected = true;
    xSemaphoreGive(dataMutex);
    return true;
  } else {
    Serial.println("Wi-Fi 连接/配网失败");
    return false;
  }
}

// ========== MQTT ==========

/**
 * MQTT 收到消息的回调函数。
 * payload 是字节数组，需要先转换成字符串再解析命令。
 * 支持的命令格式：
 *   led on
 *   led off
 *   led duty 128
 *   relay on
 *   relay off
 */
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  // 把 payload 复制到字符串并加结束符
  char msg[64];
  if (length >= sizeof(msg)) length = sizeof(msg) - 1;
  memcpy(msg, payload, length);
  msg[length] = '\0';

  Serial.print("MQTT 收到消息 [");
  Serial.print(topic);
  Serial.print("]：");
  Serial.println(msg);

  String cmd = String(msg);
  cmd.trim();
  cmd.toLowerCase();

  if (cmd == "led on") {
    setLed(true);
  } else if (cmd == "led off") {
    setLed(false);
  } else if (cmd == "relay on") {
    setRelay(true);
  } else if (cmd == "relay off") {
    setRelay(false);
  } else if (cmd.startsWith("led duty ")) {
    int duty = cmd.substring(9).toInt();
    setLedDuty(duty);
  } else {
    Serial.println("未知 MQTT 命令");
  }
}

bool reconnectMQTT() {
  if (mqttClient.connected()) return true;
  if (WiFi.status() != WL_CONNECTED) return false;

  Serial.print("正在连接 MQTT...");
  // 连接时可以使用客户端 ID，也可以加上用户名密码（视服务器配置而定）
  if (mqttClient.connect(MQTT_CLIENT_ID)) {
    Serial.println("已连接");
    mqttClient.subscribe(TOPIC_CMD);
    Serial.println("已订阅命令主题：" + String(TOPIC_CMD));

    xSemaphoreTake(dataMutex, portMAX_DELAY);
    sensorData.mqttConnected = true;
    xSemaphoreGive(dataMutex);
    return true;
  } else {
    Serial.print("失败，状态码=");
    Serial.println(mqttClient.state());
    return false;
  }
}

/**
 * 发布传感器数据到 MQTT。
 * 数据以 JSON 格式发布，方便 Home Assistant、Node-RED 等工具解析。
 */
void publishSensorData() {
  if (!mqttClient.connected()) return;

  SensorData data = getSensorDataCopy();

  String json = "{";
  json += "\"temperature\":" + String(data.temperature, 2) + ",";
  json += "\"humidity\":" + String(data.humidity, 2) + ",";
  json += "\"light\":" + String(data.light) + ",";
  json += "\"led\":" + String(data.ledOn ? "true" : "false") + ",";
  json += "\"relay\":" + String(data.relayOn ? "true" : "false");
  json += "}";

  mqttClient.publish(TOPIC_SENSOR, json.c_str());
  Serial.print("MQTT 发布：");
  Serial.println(json);
}

// ========== Web 服务器 ==========

String generateIndexHTML() {
  SensorData data = getSensorDataCopy();

  String html = "<!DOCTYPE html><html lang='zh-CN'>";
  html += "<head><meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>ESP32 智能家居节点</title>";
  html += "<style>";
  html += "body{font-family:'Microsoft YaHei',Arial,sans-serif;text-align:center;background:#f5f5f5;margin:0;padding:20px;}";
  html += ".container{max-width:420px;margin:20px auto;background:white;padding:25px;border-radius:20px;box-shadow:0 4px 15px rgba(0,0,0,0.1);}";
  html += "h1{color:#333;font-size:24px;}";
  html += ".card{display:flex;justify-content:space-between;padding:12px 0;border-bottom:1px solid #eee;}";
  html += ".label{color:#666;}";
  html += ".value{font-weight:bold;color:#333;}";
  html += ".btn{display:inline-block;margin:8px 5px;padding:10px 24px;font-size:16px;color:white;border:none;border-radius:8px;cursor:pointer;text-decoration:none;}";
  html += ".green{background:#4CAF50;}";
  html += ".red{background:#f44336;}";
  html += ".slider{width:80%;margin:10px 0;}";
  html += "</style></head><body>";
  html += "<div class='container'>";
  html += "<h1>ESP32 智能家居节点</h1>";
  html += "<div class='card'><span class='label'>温度</span><span class='value'>" + String(data.temperature, 1) + " ℃</span></div>";
  html += "<div class='card'><span class='label'>湿度</span><span class='value'>" + String(data.humidity, 1) + " %</span></div>";
  html += "<div class='card'><span class='label'>光照</span><span class='value'>" + String(data.light) + "</span></div>";
  html += "<div class='card'><span class='label'>Wi-Fi</span><span class='value'>" + String(data.wifiConnected ? "已连接" : "未连接") + "</span></div>";
  html += "<div class='card'><span class='label'>MQTT</span><span class='value'>" + String(data.mqttConnected ? "已连接" : "未连接") + "</span></div>";
  html += "<div class='card'><span class='label'>LED</span><span class='value'>" + String(data.ledOn ? "开" : "关") + "</span></div>";
  html += "<div class='card'><span class='label'>继电器</span><span class='value'>" + String(data.relayOn ? "开" : "关") + "</span></div>";
  html += "<div>";
  html += "<a href='/api/led?state=on' class='btn green'>开 LED</a>";
  html += "<a href='/api/led?state=off' class='btn red'>关 LED</a>";
  html += "</div>";
  html += "<div>";
  html += "<a href='/api/relay?state=on' class='btn green'>开继电器</a>";
  html += "<a href='/api/relay?state=off' class='btn red'>关继电器</a>";
  html += "</div>";
  html += "<div>";
  html += "<p>LED 亮度：<span id='dutyVal'>" + String(data.ledDuty) + "</span></p>";
  html += "<input type='range' min='0' max='255' value='" + String(data.ledDuty) + "' class='slider' onchange=\"fetch('/api/brightness?duty='+this.value);document.getElementById('dutyVal').innerText=this.value;\">";
  html += "</div>";
  html += "<p style='font-size:12px;color:#999;'>页面每 2 秒自动刷新</p>";
  html += "<script>setInterval(()=>location.reload(),2000);</script>";
  html += "</div></body></html>";
  return html;
}

void handleRoot() {
  server.send(200, "text/html; charset=utf-8", generateIndexHTML());
}

void handleApiData() {
  SensorData data = getSensorDataCopy();
  String json = "{";
  json += "\"temperature\":" + String(data.temperature, 2) + ",";
  json += "\"humidity\":" + String(data.humidity, 2) + ",";
  json += "\"light\":" + String(data.light) + ",";
  json += "\"ledOn\":" + String(data.ledOn ? "true" : "false") + ",";
  json += "\"ledDuty\":" + String(data.ledDuty) + ",";
  json += "\"relayOn\":" + String(data.relayOn ? "true" : "false") + ",";
  json += "\"wifiConnected\":" + String(data.wifiConnected ? "true" : "false") + ",";
  json += "\"mqttConnected\":" + String(data.mqttConnected ? "true" : "false");
  json += "}";
  server.send(200, "application/json; charset=utf-8", json);
}

void handleApiLed() {
  String state = server.arg("state");
  state.toLowerCase();
  if (state == "on") {
    setLed(true);
  } else if (state == "off") {
    setLed(false);
  }
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "");
}

void handleApiRelay() {
  String state = server.arg("state");
  state.toLowerCase();
  if (state == "on") {
    setRelay(true);
  } else if (state == "off") {
    setRelay(false);
  }
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "");
}

void handleApiBrightness() {
  int duty = server.arg("duty").toInt();
  setLedDuty(duty);
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "");
}

void handleNotFound() {
  server.send(404, "text/plain; charset=utf-8", "页面不存在");
}

void setupWebServer() {
  server.on("/", handleRoot);
  server.on("/api/data", handleApiData);
  server.on("/api/led", handleApiLed);
  server.on("/api/relay", handleApiRelay);
  server.on("/api/brightness", handleApiBrightness);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("Web 服务器已启动");
}

// ========== OTA ==========

void setupOTA() {
  ArduinoOTA.setHostname("esp32-smarthome");

  ArduinoOTA.onStart([]() {
    Serial.println("OTA 开始，准备写入新固件...");
  });

  ArduinoOTA.onEnd([]() {
    Serial.println("\nOTA 升级完成，即将重启");
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("OTA 进度：%u%%\r", (progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA 错误 [%u]\n", error);
  });

  ArduinoOTA.begin();
  Serial.println("OTA 已启用，主机名：esp32-smarthome");
}

// ========== FreeRTOS 任务 ==========

/**
 * 任务 1：传感器采集。
 * 每 5 秒读取一次 DHT22 和光敏电阻，更新共享数据结构。
 * 运行在 Core 0。
 */
void taskSensorRead(void* parameter) {
  (void)parameter;

  while (true) {
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    int light = analogRead(LDR_PIN);

    if (!isnan(h) && !isnan(t)) {
      xSemaphoreTake(dataMutex, portMAX_DELAY);
      sensorData.temperature = t;
      sensorData.humidity = h;
      sensorData.light = light;
      sensorData.lastSensorUpdate = millis();
      xSemaphoreGive(dataMutex);
    } else {
      Serial.println("DHT22 读取失败");
    }

    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }
}

/**
 * 任务 2：OLED 显示刷新。
 * 每 1 秒从共享数据复制一份，显示到 OLED 上。
 * 运行在 Core 1。
 */
void taskDisplayUpdate(void* parameter) {
  (void)parameter;

  while (true) {
    // 如果 OLED 初始化失败，本任务不再操作显示屏，避免异常
    if (!oledReady) {
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      continue;
    }

    SensorData data = getSensorDataCopy();

    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("Smart Home Node");
    display.drawLine(0, 10, SCREEN_WIDTH - 1, 10, SSD1306_WHITE);

    display.setCursor(0, 14);
    display.print("T:");
    display.print(data.temperature, 1);
    display.print("C H:");
    display.print(data.humidity, 1);
    display.println("%");

    display.setCursor(0, 24);
    display.print("Light:");
    display.println(data.light);

    display.setCursor(0, 34);
    display.print("LED:");
    display.print(data.ledOn ? "ON " : "OFF");
    display.print(" D:");
    display.println(data.ledDuty);

    display.setCursor(0, 44);
    display.print("Relay:");
    display.println(data.relayOn ? "ON" : "OFF");

    display.setCursor(0, 54);
    display.print("WiFi:");
    display.print(data.wifiConnected ? "OK " : "NO ");
    display.print("MQTT:");
    display.println(data.mqttConnected ? "OK" : "NO");

    display.display();

    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

/**
 * 任务 3：网络循环。
 * 负责处理 Web 请求、MQTT 连接与发布、OTA。
 * 运行在 Core 1。
 */
void taskNetworkLoop(void* parameter) {
  (void)parameter;

  while (true) {
    // 处理 Web 请求
    server.handleClient();

    // 处理 OTA
    ArduinoOTA.handle();

    // MQTT 连接维护
    if (!mqttClient.connected()) {
      reconnectMQTT();
    }
    mqttClient.loop();

    // 更新 MQTT 连接状态
    xSemaphoreTake(dataMutex, portMAX_DELAY);
    sensorData.mqttConnected = mqttClient.connected();
    xSemaphoreGive(dataMutex);

    // 定时发布传感器数据
    unsigned long now = millis();
    if (mqttClient.connected() && (now - lastMqttPublish >= MQTT_PUBLISH_INTERVAL)) {
      lastMqttPublish = now;
      publishSensorData();
    }

    // 每 10 毫秒让出 CPU，避免该任务占用过多时间
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

// ========== 串口命令 ==========

void handleSerialCommands() {
  if (!Serial.available()) return;

  String cmd = Serial.readStringUntil('\n');
  cmd.trim();

  if (cmd.startsWith("mqtt ")) {
    String server = cmd.substring(5);
    server.trim();
    if (server.length() > 0) {
      saveMqttServer(server);
    }
  } else if (cmd == "reboot") {
    ESP.restart();
  } else if (cmd == "info") {
    SensorData data = getSensorDataCopy();
    Serial.print("温度：");
    Serial.println(data.temperature, 2);
    Serial.print("湿度：");
    Serial.println(data.humidity, 2);
    Serial.print("光照：");
    Serial.println(data.light);
    Serial.print("LED：");
    Serial.println(data.ledOn ? "开" : "关");
    Serial.print("继电器：");
    Serial.println(data.relayOn ? "开" : "关");
    Serial.print("Wi-Fi：");
    Serial.println(data.wifiConnected ? "已连接" : "未连接");
    Serial.print("MQTT：");
    Serial.println(data.mqttConnected ? "已连接" : "未连接");
  } else {
    Serial.println("未知命令。可用：mqtt <ip>, reboot, info");
  }
}

// ========== setup & loop ==========

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("================================");
  Serial.println("综合项目 P15：完整智能家居节点");
  Serial.println("================================");

  // 创建互斥锁
  dataMutex = xSemaphoreCreateMutex();
  if (dataMutex == NULL) {
    Serial.println("互斥锁创建失败，程序停止");
    while (true) delay(1000);
  }

  // 加载配置
  loadConfig();

  // 初始化硬件
  initHardware();

  // Wi-Fi 配网/连接
  if (setupWiFi()) {
    // 配置 MQTT 服务器
    mqttClient.setServer(mqttServer.c_str(), MQTT_PORT);
    mqttClient.setCallback(mqttCallback);

    // 启动 Web 服务器
    setupWebServer();

    // 启动 OTA
    setupOTA();
  }

  // 创建 FreeRTOS 任务
  xTaskCreatePinnedToCore(taskSensorRead,    "SensorRead",   4096, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(taskDisplayUpdate, "DisplayUpdate", 4096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(taskNetworkLoop,   "NetworkLoop",   8192, NULL, 1, NULL, 1);

  Serial.println("所有任务已创建，系统运行中...");
}

void loop() {
  // loop() 里只处理串口命令。
  // 传感器、显示、网络都已经放到独立的 FreeRTOS 任务中。
  handleSerialCommands();
  delay(100);
}

/**
 * 扩展练习：
 * 1. 把光敏电阻数值映射到 OLED 背光或 LED 自动亮度：环境越暗，LED 越亮。
 * 2. 在 MQTT 之外增加 Home Assistant 自动发现报文，让设备能被 Home Assistant 自动识别为传感器和开关。
 * 3. 增加 DS18B20 或土壤湿度传感器，扩展更多环境数据通道。
 * 4. 给 Web 页面增加历史曲线，把数据缓存到 LittleFS 再绘制折线图。
 * 5. 增加一个物理按键，短按切换 LED，长按重置 WiFiManager 配置。
 */
