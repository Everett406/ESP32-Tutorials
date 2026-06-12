/*
 * 综合项目 P12：MQTT 气象站
 *
 * 学习目标：
 * 1. 理解 MQTT 协议的基本概念：代理（Broker）、主题（Topic）、发布/订阅
 * 2. 学会用 PubSubClient 库把 ESP32 接入 MQTT 服务器
 * 3. 实现"本地 OLED 显示 + 远程 MQTT 上报"的双通道数据发布
 * 4. 学会通过 MQTT 主题向 ESP32 下发命令并执行
 * 5. 巩固多传感器、Wi-Fi、OLED 的综合应用
 *
 * 涉及知识点：
 * - DHT22 温湿度读取
 * - 光敏电阻 ADC 读取
 * - I2C OLED 显示
 * - Wi-Fi 连接
 * - MQTT 发布/订阅
 *
 * 需要安装的库：
 * 1. "DHT sensor library" by Adafruit
 * 2. "Adafruit Unified Sensor" by Adafruit（DHT 库依赖）
 * 3. "Adafruit SSD1306" by Adafruit
 * 4. "Adafruit GFX Library" by Adafruit
 * 5. "PubSubClient" by Nick O'Leary（MQTT 客户端库）
 * 安装方法：Arduino IDE → 项目 → 加载库 → 管理库，搜索并安装。
 *
 * 硬件连接：
 * - DHT22 VCC  →  ESP32 3.3V
 * - DHT22 GND  →  ESP32 GND
 * - DHT22 DATA →  ESP32 GPIO4（建议 DATA 与 3.3V 之间接 4.7kΩ~10kΩ 上拉电阻）
 *
 * - 光敏电阻一端 →  ESP32 3.3V
 * - 光敏电阻另一端 →  ESP32 GPIO34 → 10kΩ 电阻 →  GND
 *
 * - OLED VCC → ESP32 3.3V
 * - OLED GND → ESP32 GND
 * - OLED SDA → ESP32 GPIO21
 * - OLED SCL → ESP32 GPIO22
 *
 * - ESP32 GPIO2 → LED 正极（长脚）
 * - LED 负极（短脚） → 330Ω 电阻 → GND
 *   这颗 LED 用来演示 MQTT 命令控制（开灯/关灯/闪烁）。
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ===== 修改这里：填入你的 Wi-Fi 信息 =====
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// ===== 修改这里：填入你的 MQTT 服务器信息 =====
// MQTT 代理（Broker）地址，可以是 IP 地址或域名
// 例如 Home Assistant 的 Mosquitto 插件、EMQX、RabbitMQ 等
const char* mqtt_server = "192.168.1.100";
const int mqtt_port = 1883;        // MQTT 默认端口是 1883
const char* mqtt_user = "";        // 如果服务器启用了认证，填写用户名；否则留空
const char* mqtt_pass = "";        // 如果服务器启用了认证，填写密码；否则留空

// MQTT 主题（Topic）。主题用斜杠分层，类似文件路径。
// 发布主题：ESP32 把传感器数据发到这里
const char* topic_temp = "esp32/weather/temperature";
const char* topic_hum = "esp32/weather/humidity";
const char* topic_light = "esp32/weather/light";
const char* topic_status = "esp32/weather/status";  // 在线状态
// 订阅主题：其他设备/平台向 ESP32 发送命令
const char* topic_cmd = "esp32/weather/cmd";

// 引脚定义
#define DHT_PIN 4
#define DHT_TYPE DHT22
#define LDR_PIN 34
#define LED_PIN 2

// OLED 参数
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

// 创建对象
DHT dht(DHT_PIN, DHT_TYPE);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// OLED 是否初始化成功
bool oledReady = false;

// 当前数据
float temperature = 0.0;
float humidity = 0.0;
int lightPercent = 0;

// MQTT 连接状态
bool mqttConnected = false;

// 时间戳
unsigned long lastReconnectAttempt = 0;
unsigned long lastPublish = 0;
const unsigned long PUBLISH_INTERVAL = 10000;  // 每 10 秒上报一次
const unsigned long RECONNECT_INTERVAL = 5000; // 重连间隔

// 函数前置声明
void setup_wifi();
void callback(char* topic, byte* payload, unsigned int length);
boolean reconnect();
void readSensors();
void updateOLED();
void publishData();

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("================================");
  Serial.println("综合项目 P12：MQTT 气象站");
  Serial.println("================================");

  // 初始化引脚
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // 初始化 ADC
  analogReadResolution(12);

  // 初始化 DHT
  dht.begin();

  // 初始化 OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("OLED 初始化失败，请检查 I2C 地址和接线！");
  } else {
    oledReady = true;
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("MQTT Weather");
    display.println("Booting...");
    display.display();
  }

  // 连接 Wi-Fi
  setup_wifi();

  // 设置 MQTT 服务器地址和端口
  mqttClient.setServer(mqtt_server, mqtt_port);

  // 设置收到消息时的回调函数
  // 当有人向本设备订阅的主题发布消息时，这个函数会被调用
  mqttClient.setCallback(callback);

  Serial.println("系统初始化完成");
}

void loop() {
  // 如果 Wi-Fi 断开，先重连 Wi-Fi
  if (WiFi.status() != WL_CONNECTED) {
    setup_wifi();
  }

  // 如果 MQTT 未连接，周期性尝试重连
  if (!mqttClient.connected()) {
    mqttConnected = false;
    if (millis() - lastReconnectAttempt > RECONNECT_INTERVAL) {
      lastReconnectAttempt = millis();
      if (reconnect()) {
        lastReconnectAttempt = 0;
      }
    }
  } else {
    mqttConnected = true;
    // mqttClient.loop() 必须定期调用，它负责接收消息、维持心跳等
    mqttClient.loop();
  }

  // 周期性读取传感器
  readSensors();

  // 周期性刷新 OLED
  static unsigned long lastOledUpdate = 0;
  if (millis() - lastOledUpdate > 1000) {
    lastOledUpdate = millis();
    updateOLED();
  }

  // 周期性向 MQTT 发布数据
  if (mqttClient.connected() && millis() - lastPublish > PUBLISH_INTERVAL) {
    lastPublish = millis();
    publishData();
  }
}

// 连接 Wi-Fi
void setup_wifi() {
  Serial.print("正在连接 Wi-Fi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 30) {
    delay(500);
    Serial.print(".");
    retry++;
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Wi-Fi 已连接，IP 地址：");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Wi-Fi 连接失败，5 秒后重试");
  }
}

// MQTT 消息回调函数
// topic：消息来自哪个主题
// payload：消息内容，是字节数组
// length：消息长度（字节数）
void callback(char* topic, byte* payload, unsigned int length) {
  // 把 payload 转换成 String，方便比较
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  // 去掉首尾空格，避免因为空格导致命令不匹配
  message.trim();

  Serial.print("收到 MQTT 消息 [");
  Serial.print(topic);
  Serial.print("]：");
  Serial.println(message);

  // 根据命令控制 LED
  if (message == "on") {
    digitalWrite(LED_PIN, HIGH);
    Serial.println("命令执行：LED 点亮");
  } else if (message == "off") {
    digitalWrite(LED_PIN, LOW);
    Serial.println("命令执行：LED 熄灭");
  } else if (message == "blink") {
    // 闪烁三次
    for (int i = 0; i < 3; i++) {
      digitalWrite(LED_PIN, HIGH);
      delay(200);
      digitalWrite(LED_PIN, LOW);
      delay(200);
    }
    Serial.println("命令执行：LED 闪烁");
  } else {
    Serial.println("未知命令");
  }
}

// 连接 MQTT 服务器
boolean reconnect() {
  // 生成唯一客户端 ID，避免多台设备互相挤掉
  // ESP.getEfuseMac() 返回芯片的 MAC 地址，全球唯一
  String clientId = "ESP32_Weather_" + String((uint32_t)ESP.getEfuseMac(), HEX);

  Serial.print("正在连接 MQTT 服务器...");

  // connect(clientId, username, password, willTopic, willQoS, willRetain, willMessage)
  // 遗嘱消息（LWT）：当 ESP32 异常断开时，服务器会自动向其他订阅者发送 "offline"
  boolean connected = mqttClient.connect(
                        clientId.c_str(),
                        mqtt_user[0] ? mqtt_user : NULL,
                        mqtt_pass[0] ? mqtt_pass : NULL,
                        topic_status, 0, true, "offline"
                      );

  if (connected) {
    Serial.println("已连接");

    // 发布上线消息
    mqttClient.publish(topic_status, "online", true);

    // 订阅命令主题
    // 订阅后，其他设备向 esp32/weather/cmd 发送的消息会进入 callback
    mqttClient.subscribe(topic_cmd);
    Serial.print("已订阅主题：");
    Serial.println(topic_cmd);
  } else {
    Serial.print("连接失败，状态码：");
    Serial.println(mqttClient.state());
  }

  return connected;
}

// 读取传感器
void readSensors() {
  // DHT22 建议两次读取间隔不少于 2 秒
  // 这里用静态变量记录上次读取时间，避免在 loop 中硬延时
  static unsigned long lastDHTRead = 0;
  if (millis() - lastDHTRead < 2500) {
    return;
  }
  lastDHTRead = millis();

  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (!isnan(h) && !isnan(t)) {
    humidity = h;
    temperature = t;
  } else {
    Serial.println("DHT22 读取失败");
  }

  // 读取光敏电阻，取 10 次平均
  long sum = 0;
  for (int i = 0; i < 10; i++) {
    sum += analogRead(LDR_PIN);
    delay(5);
  }
  int lightRaw = sum / 10;
  lightPercent = map(lightRaw, 0, 4095, 100, 0);
  lightPercent = constrain(lightPercent, 0, 100);
}

// 更新 OLED 显示
void updateOLED() {
  // 如果 OLED 初始化失败，直接返回，避免调用未初始化的对象导致异常
  if (!oledReady) {
    return;
  }

  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("MQTT Weather");

  // 显示 Wi-Fi 和 MQTT 状态
  display.setCursor(0, 10);
  display.print("WiFi:");
  display.print(WiFi.status() == WL_CONNECTED ? "OK" : "--");
  display.print(" MQTT:");
  display.println(mqttConnected ? "OK" : "--");

  // 显示传感器数据
  display.setCursor(0, 24);
  display.print("T:");
  display.print(temperature, 1);
  display.print((char)247);
  display.println("C");

  display.print("H:");
  display.print(humidity, 1);
  display.println("%");

  display.print("L:");
  display.print(lightPercent);
  display.println("%");

  // 显示 IP 地址，方便排查
  display.setCursor(0, 56);
  display.print(WiFi.localIP());

  display.display();
}

// 发布传感器数据到 MQTT
void publishData() {
  // String(value, decimalPlaces) 可以把浮点数转成指定精度的字符串
  String tempStr = String(temperature, 1);
  String humStr = String(humidity, 1);
  String lightStr = String(lightPercent);

  // publish(topic, payload) 把消息发布到指定主题
  mqttClient.publish(topic_temp, tempStr.c_str());
  mqttClient.publish(topic_hum, humStr.c_str());
  mqttClient.publish(topic_light, lightStr.c_str());

  Serial.print("已发布：T=");
  Serial.print(tempStr);
  Serial.print(" H=");
  Serial.print(humStr);
  Serial.print(" L=");
  Serial.println(lightStr);
}

/*
 * 扩展挑战：
 * 1. 用 JSON 格式一次性发布所有传感器数据到 esp32/weather/data，
 *    方便 Home Assistant 等平台的 MQTT sensor 解析。
 * 2. 实现 Home Assistant MQTT Discovery，让 ESP32 自动在 Home Assistant 里生成传感器实体。
 * 3. 增加 OLED 页面切换：用按键或触摸切换"传感器页"和"网络状态页"。
 * 4. 增加离线缓存：当 MQTT 断开时，把数据暂存在数组里，恢复连接后批量补发。
 * 5. 用 TLS/SSL 连接 MQTT 服务器（端口 8883），提高数据传输安全性。
 */
