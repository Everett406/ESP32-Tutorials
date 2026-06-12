/*
 * P08_Web_Temperature_Monitor
 * 网页温湿度监测器
 *
 * 学习目标：
 * 1. 让 ESP32 连接 2.4GHz Wi-Fi
 * 2. 用 DHT22 传感器读取环境温度和湿度
 * 3. 搭建简易 Web 服务器，把温湿度显示在网页上
 * 4. 实现网页自动刷新，不用手动刷新就能看到最新数据
 *
 * 前置知识：
 * - DHT22：一种数字温湿度传感器，用单总线协议通信，精度比 DHT11 更高。
 * - HTTP（HyperText Transfer Protocol，超文本传输协议）：浏览器和服务器之间
 *   交换网页内容的"对话规则"。ESP32 在这里作为服务器，手机/电脑是客户端。
 * - Web 服务器：监听特定端口（默认 80），根据浏览器发来的请求返回网页。
 * - HTML（HyperText Markup Language）：网页的结构语言，浏览器把它渲染成页面。
 * - meta refresh：HTML 的一种自动刷新机制，让浏览器每隔几秒重新请求页面。
 *
 * 需要安装的库：
 * - DHT sensor library（作者：Adafruit）
 * - Adafruit Unified Sensor（DHT 库的依赖，通常会自动安装）
 *
 * 硬件连接：
 * - ESP32 3.3V  →  DHT22 VCC（电源正极）
 * - ESP32 GND   →  DHT22 GND（电源负极）
 * - ESP32 GPIO4 →  DHT22 DATA（数据线）
 *
 * 注意：DHT22 数据引脚建议外接一个 4.7kΩ ~ 10kΩ 的上拉电阻到 3.3V，
 *       很多模块已经内置了这个电阻。
 */

// 引入 Wi-Fi 库
// WiFi.h 提供连接无线网络、获取 IP 地址等功能
#include <WiFi.h>

// 引入 Web 服务器库
// WebServer.h 让 ESP32 能够响应浏览器的 HTTP 请求
#include <WebServer.h>

// 引入 DHT 温湿度传感器库
#include <DHT.h>

// ================== Wi-Fi 配置 ==================
// 把这里改成你家里的 Wi-Fi 名称和密码
// 注意：ESP32 只支持 2.4GHz Wi-Fi，不支持 5GHz
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// ================== DHT22 配置 ==================
// DHT22 数据线连接的 GPIO 引脚
#define DHT_PIN 4

// 指定传感器类型为 DHT22
// 如果你用的是 DHT11，把 DHT22 改成 DHT11 即可
#define DHT_TYPE DHT22

// 创建 DHT 对象
// 它会负责和 DHT22 通信，并把读取到的温湿度转换为我们能用的数值
DHT dht(DHT_PIN, DHT_TYPE);

// ================== Web 服务器配置 ==================
// 创建 WebServer 对象，监听 80 端口
// 80 端口是 HTTP 协议的默认端口，浏览器访问时可以不写端口号
WebServer server(80);

// 存储最近一次读取到的温度和湿度
// 用 float（浮点数）是因为温湿度通常带有小数
float temperature = 0.0;
float humidity = 0.0;

// 记录上次读取传感器的时间（毫秒）
// DHT22 的采样间隔建议不少于 2 秒，读太快会返回错误
unsigned long last_sensor_read = 0;

// 传感器读取间隔：2000 毫秒 = 2 秒
#define SENSOR_READ_INTERVAL 2000

// 记录上次读取是否失败
bool sensor_read_error = false;

// setup() 只执行一次，完成初始化
void setup() {
  // 启动串口监视器，波特率 115200
  Serial.begin(115200);

  // 初始化 DHT22 传感器
  // begin() 会配置引脚，准备通信
  dht.begin();

  // 先做一次读取，让温湿度变量有初始值
  readSensor();

  // 连接 Wi-Fi
  Serial.print("正在连接 Wi-Fi: ");
  Serial.println(WIFI_SSID);

  // WiFi.begin(ssid, password) 启动连接过程
  // 这是一个异步过程，连接需要时间，所以下面用 while 循环等待
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  // 每隔 500 毫秒检查一次连接状态，直到连接成功
  // WL_CONNECTED 表示连接成功并获取到 IP 地址
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // 连接成功后，打印 IP 地址
  // 手机或电脑浏览器输入这个 IP 地址就能访问网页
  Serial.println();
  Serial.println("Wi-Fi 连接成功！");
  Serial.print("IP 地址：");
  Serial.println(WiFi.localIP());

  // 注册网页请求处理函数
  // 当浏览器访问根路径 "/" 时，调用 handleRoot() 函数返回网页
  server.on("/", handleRoot);

  // 启动 Web 服务器
  // 之后 ESP32 就会监听 80 端口的 HTTP 请求
  server.begin();

  Serial.println("Web 服务器已启动");
  Serial.println("请在浏览器中输入上面的 IP 地址访问网页");
}

// loop() 不断循环，处理客户端请求和传感器读取
void loop() {
  // 处理 Web 服务器事务
  // 每次循环都要调用，否则浏览器发来的请求不会被响应
  server.handleClient();

  // 每隔一段时间读取一次传感器
  unsigned long now = millis();
  if (now - last_sensor_read >= SENSOR_READ_INTERVAL) {
    last_sensor_read = now;
    readSensor();
  }
}

// readSensor() 负责读取 DHT22 的温湿度数据
void readSensor() {
  // 读取温度，单位为摄氏度（℃）
  // readTemperature() 返回 float 类型，如果读取失败会返回 NaN（非数字）
  float new_temp = dht.readTemperature();

  // 读取相对湿度，单位为百分比（%）
  float new_humi = dht.readHumidity();

  // 检查读取结果是否有效
  // isnan() 用来判断一个 float 是不是 NaN
  if (isnan(new_temp) || isnan(new_humi)) {
    Serial.println("DHT22 读取失败，可能是接线松动或读取间隔太短");
    sensor_read_error = true;
    return;
  }

  // 读取成功，更新全局变量
  temperature = new_temp;
  humidity = new_humi;
  sensor_read_error = false;

  // 在串口监视器打印结果，方便调试
  Serial.print("温度：");
  Serial.print(temperature);
  Serial.print(" ℃，湿度：");
  Serial.print(humidity);
  Serial.println(" %");
}

// handleRoot() 用于生成并返回网页内容
// 当浏览器访问 ESP32 的 IP 地址时，这个函数会被调用
void handleRoot() {
  // 构造 HTML 网页字符串
  // String 类可以方便地拼接长文本
  String html = "<!DOCTYPE html><html>";
  html += "<head>";
  html += "<meta charset='UTF-8'>";  // 让中文正常显示

  // meta refresh 实现自动刷新：每隔 2 秒重新加载页面
  // content='2' 表示 2 秒，url='/' 表示刷新后回到根路径
  html += "<meta http-equiv='refresh' content='2;url=/'>";

  html += "<title>ESP32 温湿度监测</title>";

  // 简单的美化样式：让页面在手机上也能好看
  html += "<style>";
  html += "body{font-family:Arial,sans-serif;text-align:center;margin-top:50px;background:#f0f8ff;}";
  html += "h1{color:#333;}";
  html += ".card{display:inline-block;background:#fff;padding:30px 50px;border-radius:15px;box-shadow:0 4px 8px rgba(0,0,0,0.1);}";
  html += ".value{font-size:48px;font-weight:bold;color:#007acc;margin:10px 0;}";
  html += ".label{font-size:20px;color:#666;}";
  html += ".error{color:#cc0000;font-size:24px;}";
  html += "</style>";

  html += "</head><body>";
  html += "<div class='card'>";
  html += "<h1>ESP32 温湿度监测</h1>";

  if (sensor_read_error) {
    // 如果传感器读取失败，在网页上显示错误提示
    html += "<p class='error'>传感器读取失败，请检查接线</p>";
  } else {
    // 显示温度
    html += "<p class='label'>温度 Temperature</p>";
    html += "<p class='value'>" + String(temperature) + " &deg;C</p>";

    // 显示湿度
    html += "<p class='label'>湿度 Humidity</p>";
    html += "<p class='value'>" + String(humidity) + " %</p>";
  }

  html += "<p style='color:#999;font-size:14px;'>页面每 2 秒自动刷新</p>";
  html += "</div>";
  html += "</body></html>";

  // 返回 HTTP 200 状态码和 HTML 内容
  // 200 表示请求成功，"text/html" 告诉浏览器这是网页内容
  server.send(200, "text/html", html);
}

/*
 * 扩展挑战：
 * 1. 在网页上增加温度/湿度曲线图，用 JavaScript 定时请求 /api 接口获取 JSON 数据并绘制。
 * 2. 增加高温报警功能：当温度超过设定阈值时，蜂鸣器响并在网页上变红提示。
 * 3. 把温湿度数据上传到第三方物联网平台（如 ThingSpeak、巴法云），实现远程长期监测。
 */
