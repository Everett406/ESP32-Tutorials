/*
 * ESP32 扩展教程：WiFi_AP_Mode - 把 ESP32 变成 Wi-Fi 热点
 *
 * 学习目标：
 * 1. 理解 Wi-Fi 的两种基本工作模式：STA 模式 和 AP 模式
 * 2. 学会用 WiFi.softAP() 让 ESP32 自己发射 Wi-Fi 信号
 * 3. 在手机/电脑连接 ESP32 热点后，通过网页控制 LED
 *
 * 前置知识：
 * - 建议先学习第 06 课（WiFi_Connect）和第 07 课（Web_Server_LED）
 * - 理解什么是 SSID、IP 地址、HTTP 请求/响应
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
 * 1. 上传代码到 ESP32。
 * 2. 打开手机或电脑的 Wi-Fi 列表，找到名为 "ESP32_AP" 的热点。
 * 3. 输入密码 "12345678" 连接。
 * 4. 打开浏览器，访问 http://192.168.4.1。
 * 5. 点击网页按钮控制 LED。
 */

#include <WiFi.h>
#include <WebServer.h>

// ===== 热点配置 =====
// AP_SSID 是 ESP32 发射出的 Wi-Fi 名称（也叫 SSID）。
// SSID（Service Set Identifier，服务集标识符）就是我们在手机 Wi-Fi 列表里看到的名字。
const char* AP_SSID = "ESP32_AP";

// AP_PASSWORD 是连接这个热点需要的密码。
// 为了保证安全，密码长度至少 8 位。如果写 "" 空字符串，热点将不加密（任何人都能连）。
const char* AP_PASSWORD = "12345678";

// LED 引脚选择 GPIO2，这是大多数 ESP32 开发板板载 LED 所在引脚。
// 即使你的开发板没有板载 LED，GPIO2 也是一个安全可用的 GPIO。
#define LED_PIN 2

// 创建 WebServer 对象，监听 80 端口。
// 80 端口是 HTTP 协议的默认端口，所以浏览器访问时不需要写 "域名:80"。
WebServer server(80);

// 记录 LED 当前状态，用来在网页上显示正确的按钮文字。
bool ledState = false;

/*
 * 生成网页内容
 * 为什么用函数生成 HTML？
 * 因为 LED 状态会变化，网页上的按钮和提示也要跟着变化。
 * 每次有浏览器访问，我们就根据 ledState 重新拼一份 HTML 返回。
 */
String generateHTML() {
  String html = "<!DOCTYPE html><html lang='zh-CN'>";
  html += "<head>";
  html += "<meta charset='UTF-8'>";
  // viewport 让手机浏览器以正确比例显示页面，不会缩放得很小。
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>ESP32 AP 模式控制</title>";
  html += "<style>";
  html += "body{font-family:'Microsoft YaHei',Arial,sans-serif;text-align:center;background:#f0f2f5;margin:0;padding:20px;}";
  html += ".container{max-width:400px;margin:40px auto;background:white;padding:30px;border-radius:20px;box-shadow:0 4px 20px rgba(0,0,0,0.1);}";
  html += "h1{color:#333;}";
  html += ".info{color:#666;font-size:14px;margin-bottom:20px;}";
  html += ".status{font-size:60px;margin:20px 0;}";
  html += ".btn{display:inline-block;padding:15px 50px;font-size:22px;color:white;border:none;border-radius:10px;cursor:pointer;text-decoration:none;transition:0.3s;}";
  html += ".btn:hover{opacity:0.85;transform:translateY(-2px);}";
  html += "</style>";
  html += "</head><body>";
  html += "<div class='container'>";
  html += "<h1>ESP32 热点模式</h1>";
  html += "<p class='info'>你已连接到 ESP32 自己的 Wi-Fi 热点</p>";

  // 根据 LED 状态生成不同的图标和按钮。
  if (ledState) {
    html += "<div class='status' style='color:#4CAF50'>●</div>";
    html += "<p>LED 状态：<strong>点亮</strong></p>";
    html += "<a href='/off' class='btn' style='background:#f44336'>关闭 LED</a>";
  } else {
    html += "<div class='status' style='color:#ccc'>●</div>";
    html += "<p>LED 状态：<strong>熄灭</strong></p>";
    html += "<a href='/on' class='btn' style='background:#4CAF50'>打开 LED</a>";
  }

  html += "</div></body></html>";
  return html;
}

// 处理根路径 "/"
// 当浏览器只输入 IP 地址时，默认访问的就是根路径。
void handleRoot() {
  // server.send(状态码, 内容类型, 内容)
  // 200 表示请求成功；text/html 表示返回的是网页。
  server.send(200, "text/html; charset=utf-8", generateHTML());
}

// 处理 "/on" 请求
void handleOn() {
  ledState = true;
  digitalWrite(LED_PIN, HIGH);  // GPIO2 输出高电平，LED 点亮
  Serial.println("收到命令：开灯");

  // 操作完成后重定向回首页，这样用户看到的页面会自动刷新。
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "");
}

// 处理 "/off" 请求
void handleOff() {
  ledState = false;
  digitalWrite(LED_PIN, LOW);   // GPIO2 输出低电平，LED 熄灭
  Serial.println("收到命令：关灯");

  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "");
}

// 处理不存在的路径
void handleNotFound() {
  server.send(404, "text/plain; charset=utf-8", "页面不存在");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("================================");
  Serial.println("ESP32 扩展教程：Wi-Fi AP 模式");
  Serial.println("================================");

  // 初始化 LED 引脚
  // pinMode(pin, mode) 告诉 ESP32 这个引脚要做什么用。
  // OUTPUT 表示输出模式，因为我们要主动控制 LED 亮灭。
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);   // 初始状态关闭 LED

  // WiFi.mode() 设置 Wi-Fi 工作模式：
  // - WIFI_STA：Station 模式，ESP32 作为“客户端”连接路由器。
  // - WIFI_AP：AP 模式（Access Point，无线接入点），ESP32 自己当“路由器”发热点。
  // - WIFI_AP_STA：同时支持两种模式。
  // 这里只用 AP 模式，所以设为 WIFI_AP。
  WiFi.mode(WIFI_AP);

  // WiFi.softAP(ssid, password) 启动热点。
  // 返回值是 bool，true 表示启动成功。
  // channel 参数可以用默认，这里不传就是默认信道。
  bool result = WiFi.softAP(AP_SSID, AP_PASSWORD);

  if (!result) {
    Serial.println("热点启动失败，请检查配置后重启");
    return;  // 失败后直接返回，不再启动服务器
  }

  Serial.println("热点已启动！");
  Serial.print("热点名称（SSID）：");
  Serial.println(AP_SSID);
  Serial.print("热点密码：");
  Serial.println(AP_PASSWORD);

  // WiFi.softAPIP() 返回 ESP32 在 AP 模式下的 IP 地址。
  // 默认情况下这个地址是 192.168.4.1，是连接设备的“网关”。
  // IP 地址（Internet Protocol Address）是设备在网络中的“门牌号”，
  // 其他设备通过这个地址找到 ESP32。
  Serial.print("ESP32 热点 IP 地址：");
  Serial.println(WiFi.softAPIP());

  // WiFi.softAPmacAddress() 返回 AP 模式下的 MAC 地址。
  // MAC 地址（Media Access Control Address）是网卡的硬件地址，全球唯一。
  Serial.print("AP 模式 MAC 地址：");
  Serial.println(WiFi.softAPmacAddress());

  // WiFi.softAPgetStationNum() 返回当前连接到热点的设备数量。
  Serial.print("当前连接设备数：");
  Serial.println(WiFi.softAPgetStationNum());

  // 注册路由处理函数
  server.on("/", handleRoot);
  server.on("/on", handleOn);
  server.on("/off", handleOff);
  server.onNotFound(handleNotFound);

  // 启动 Web 服务器
  server.begin();
  Serial.println("HTTP 服务器已启动");
  Serial.println("请连接热点后访问 http://192.168.4.1");
}

void loop() {
  // server.handleClient() 必须在 loop() 中不断调用，
  // 它的作用是检查有没有浏览器发来请求，并调用对应的处理函数。
  // 如果长时间不调用，浏览器会显示“无法访问此网站”。
  server.handleClient();

  // 每隔 5 秒在串口打印一次连接设备数量，方便调试。
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 5000) {
    lastPrint = millis();
    Serial.print("当前连接设备数：");
    Serial.println(WiFi.softAPgetStationNum());
  }
}

/*
 * 扩展练习：
 * 1. 给热点加上隐藏功能：使用 WiFi.softAP(AP_SSID, AP_PASSWORD, channel, hidden=1)。
 *    隐藏后手机 Wi-Fi 列表里看不到，需要手动输入 SSID 连接。
 * 2. 在网页上显示已连接设备的 MAC 地址和 IP 地址（用 WiFi.softAPgetStationMac() 等函数）。
 * 3. 把 AP 模式和 STA 模式结合（WIFI_AP_STA）：
 *    ESP32 既连接家里的路由器上网，又发射自己的热点供手机连接。
 */
