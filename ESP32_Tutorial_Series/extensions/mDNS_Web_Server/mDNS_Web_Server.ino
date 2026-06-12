/*
 * ESP32 扩展教程：mDNS_Web_Server - 用域名代替 IP 地址访问 ESP32
 *
 * 学习目标：
 * 1. 理解 IP 地址访问设备的不便之处
 * 2. 理解 DNS 和 mDNS 的基本作用
 * 3. 学会用 ESPmDNS 库让 ESP32 响应 "esp32.local" 域名
 * 4. 在手机/电脑浏览器中通过 http://esp32.local 控制 LED
 *
 * 前置知识：
 * - 建议先学习第 06 课（WiFi_Connect）和第 07 课（Web_Server_LED）。
 * - 需要知道 STA 模式、IP 地址、HTTP 请求/响应的基本概念。
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
 * 1. 修改下方的 ssid 和 password 为你自己的 Wi-Fi 信息。
 * 2. 上传代码到 ESP32，打开串口监视器获取 IP 地址。
 * 3. 确保手机/电脑和 ESP32 连接到同一个 Wi-Fi。
 * 4. 打开浏览器，访问 http://esp32.local，即可看到控制页面。
 *
 * 注意事项：
 * - mDNS 在部分 Android 手机上可能不支持 .local 域名，
 *   如果访问不了，可以使用串口打印出的 IP 地址访问。
 * - Windows 电脑通常需要安装 Bonjour（iTunes 会自带）或 Apple Print Service 才能解析 .local。
 */

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>

// ===== 修改这里：填入你的 Wi-Fi 信息 =====
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// LED 引脚选择 GPIO2，这是大多数 ESP32 开发板板载 LED 所在引脚。
#define LED_PIN 2

// 创建 WebServer 对象，监听 80 端口（HTTP 默认端口）。
WebServer server(80);

// 记录 LED 当前状态。
bool ledState = false;

// mDNS 主机名。
// 主机名是域名前面的部分，例如 "esp32" 对应 "esp32.local"。
// 只能使用英文字母、数字和连字符，不能用中文或空格。
const char* MDNS_HOSTNAME = "esp32";

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
  // viewport 让手机浏览器以正确比例显示页面。
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>ESP32 mDNS 控制</title>";
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
  html += "<h1>ESP32 mDNS 控制</h1>";
  html += "<p class='info'>访问地址：http://esp32.local</p>";

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
void handleRoot() {
  server.send(200, "text/html; charset=utf-8", generateHTML());
}

// 处理 "/on" 请求
void handleOn() {
  ledState = true;
  digitalWrite(LED_PIN, HIGH);
  Serial.println("收到命令：开灯");

  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "");
}

// 处理 "/off" 请求
void handleOff() {
  ledState = false;
  digitalWrite(LED_PIN, LOW);
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
  Serial.println("ESP32 扩展教程：mDNS Web 服务器");
  Serial.println("================================");

  // 初始化 LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

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
    Serial.println("Wi-Fi 连接失败，请检查配置");
    return;
  }

  Serial.print("Wi-Fi 已连接，IP 地址：");
  Serial.println(WiFi.localIP());

  // ===== mDNS 启动 =====
  // DNS（Domain Name System，域名系统）的作用是把人类好记的域名
  // （例如 www.example.com）转换成机器能识别的 IP 地址。
  //
  // mDNS（Multicast DNS，多播 DNS）是 DNS 的一种简化版本，
  // 它不需要专门的 DNS 服务器，设备直接在局域网内广播自己的名字。
  // 其他设备收到广播后，就知道 "esp32.local" 对应的 IP 地址是多少。
  //
  // MDNS.begin(hostname) 启动 mDNS 服务，向局域网宣告自己的主机名。
  // 这里的主机名是 "esp32"，所以其他设备可以用 "esp32.local" 找到它。
  if (!MDNS.begin(MDNS_HOSTNAME)) {
    Serial.println("mDNS 启动失败");
    return;
  }

  Serial.print("mDNS 已启动，可通过 http://");
  Serial.print(MDNS_HOSTNAME);
  Serial.println(".local 访问");

  // MDNS.addService() 向局域网广播本设备提供的服务。
  // 参数 "http" 表示提供 HTTP 服务，"tcp" 表示使用 TCP 协议，80 是端口号。
  // 这样某些网络工具（例如 Bonjour 浏览器）可以发现这是一台 Web 服务器。
  MDNS.addService("http", "tcp", 80);

  // 注册路由处理函数
  server.on("/", handleRoot);
  server.on("/on", handleOn);
  server.on("/off", handleOff);
  server.onNotFound(handleNotFound);

  // 启动 Web 服务器
  server.begin();
  Serial.println("HTTP 服务器已启动");
}

void loop() {
  // 处理客户端 HTTP 请求
  server.handleClient();

  // MDNS.update() 必须在 loop() 中定期调用，
  // 它的作用是维持 mDNS 服务，及时响应局域网内其他设备的查询请求。
  // 如果长时间不调用，其他设备可能无法解析 esp32.local。
  MDNS.update();
}

/*
 * 扩展练习：
 * 1. 修改 MDNS_HOSTNAME 为你喜欢的名字（例如 "myesp"），
 *    然后通过 http://myesp.local 访问。
 * 2. 在网页上同时显示 ESP32 的 IP 地址、MAC 地址和 Wi-Fi 信号强度。
 * 3. 把本节课和第 06 课结合：当 Wi-Fi 连接失败时，自动进入 AP 模式
 *    并用 mDNS 提供配网页面。
 */
