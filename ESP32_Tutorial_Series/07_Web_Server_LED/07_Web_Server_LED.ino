/*
 * ESP32 教程系列 07：Web 服务器 - 用手机/电脑浏览器控制 LED
 * 
 * 学习目标：
 * 1. 理解什么是 Web 服务器、HTTP 请求和 HTTP 响应
 * 2. 学会用 WebServer.h 创建一个简单的 HTTP 服务器
 * 3. 理解 URL、路径、端口、HTML、CSS 的基本概念
 * 4. 实现通过浏览器远程开关 ESP32 上的 LED
 * 
 * 前置知识：
 * - 已完成教程 06《Wi-Fi 连接》，知道如何把 ESP32 连入 Wi-Fi
 * - 了解 digitalWrite() 控制 LED 亮灭（教程 01）
 * 
 * 硬件连接：
 * - ESP32 GPIO2  →  LED 正极（长脚）
 * - LED 负极（短脚） →  330Ω 电阻  →  GND
 * 
 * 为什么要加电阻？
 * LED 导通后内阻很小，如果直接接 3.3V 会流过过大电流，导致 LED 烧坏。
 * 330Ω 电阻把电流限制在约 10mA 以内，让 LED 安全工作。
 * 
 * 使用步骤：
 * 1. 修改下面的 ssid 和 password 为你自己的 Wi-Fi 信息。
 * 2. 用 USB 线把程序上传到 ESP32。
 * 3. 打开 Arduino IDE 串口监视器（波特率 115200），记录打印出的 IP 地址。
 * 4. 确保你的手机/电脑和 ESP32 连接同一个 Wi-Fi。
 * 5. 在手机/电脑的浏览器地址栏输入 ESP32 的 IP 地址，按回车访问。
 * 6. 点击网页上的按钮即可控制 LED。
 * 
 * 关键概念解释：
 * - Web 服务器：一台一直等待别人来“访问”的计算机程序。这里 ESP32 就是服务器，
 *   你的手机/电脑浏览器就是“客户端”。服务器收到请求后返回网页或数据。
 * - HTTP（HyperText Transfer Protocol，超文本传输协议）：浏览器和 Web 服务器
 *   之间对话使用的规则。它规定了请求怎么发、响应怎么回，是无状态的协议，
 *   即服务器默认不会记住你上一步做了什么（所以这里用 URL 路径来区分命令）。
 * - HTTP 请求（Request）：浏览器发给服务器的一条消息，比如“请给我首页内容”。
 *   请求里包含方法（GET、POST 等）、路径（/、/on、/off）、协议版本等信息。
 * - HTTP 响应（Response）：服务器收到请求后返回的消息，包括状态码、内容类型、
 *   正文内容等。比如 200 表示成功，404 表示找不到页面。
 * - URL（Uniform Resource Locator，统一资源定位符）：就是浏览器地址栏里那串地址，
 *   比如 http://192.168.1.105/on。它告诉浏览器要访问哪台服务器上的哪个资源。
 * - 路径（Path）：URL 中域名/IP 后面的部分，如 /、/on、/off。Web 服务器根据
 *   路径来决定返回什么内容或执行什么操作。
 * - 端口（Port）：服务器上不同服务的“门牌号”。HTTP 默认端口是 80，HTTPS 默认是 443。
 *   本示例用 80 端口，所以访问时不需要在 IP 后面加“:80”。
 * - HTML（HyperText Markup Language，超文本标记语言）：网页的骨架，用来描述
 *   网页里有哪些标题、按钮、段落等内容。
 * - CSS（Cascading Style Sheets，层叠样式表）：网页的“化妆品”，用来设置颜色、
 *   字体大小、按钮样式、布局等，让网页更美观。
 * - 状态码：HTTP 响应中用来表示结果的三位数字。常见：
 *   * 200 OK：请求成功。
 *   * 302 Found：重定向，告诉浏览器去访问另一个地址。
 *   * 404 Not Found：服务器找不到请求的资源。
 */

#include <WiFi.h>
#include <WebServer.h>

// ===== 修改这里：填入你的 Wi-Fi 信息 =====
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// LED 引脚。GPIO2 是大多数 ESP32 开发板板载 LED 所在的引脚，
// 直接控制这个引脚就能看到效果，不需要外接元件。
#define LED_PIN 2

// 创建 WebServer 对象，监听 80 端口。
// 80 端口是 HTTP 协议的默认端口，浏览器访问时如果不写端口就默认是 80。
// 如果想用其他端口，比如 8080，访问时就要写 http://192.168.x.x:8080。
WebServer server(80);

// LED 当前状态。用 bool 类型（true/false）记录，方便网页显示当前状态。
// 这里 false 表示熄灭，true 表示点亮。
bool ledState = false;

// 生成网页内容的函数。
// 为什么要单独写成一个函数？因为首页 / 和每次操作后返回的页面结构几乎一样，
// 写成函数可以避免重复代码，也更容易修改网页样式。
String generateHTML() {
  // String 是 Arduino 的字符串类，支持用 += 拼接内容。
  // 这里拼接出一个完整的 HTML 页面字符串，最后一次性发给浏览器。
  String html = "<!DOCTYPE html><html lang='zh-CN'>";
  html += "<head>";
  html += "<meta charset='UTF-8'>";
  // viewport 元标签让手机浏览器以正确比例显示页面，不会把网页缩得很小。
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>ESP32 LED 控制</title>";
  html += "<style>";
  html += "body{font-family:'Microsoft YaHei',Arial,sans-serif;text-align:center;background:#f5f5f5;margin:0;padding:20px;}";
  html += ".container{max-width:400px;margin:50px auto;background:white;padding:30px;border-radius:20px;box-shadow:0 4px 15px rgba(0,0,0,0.1);}";
  html += "h1{color:#333;}";
  html += ".status{font-size:60px;margin:20px 0;}";
  html += ".btn{display:inline-block;padding:15px 50px;font-size:24px;color:white;border:none;border-radius:10px;cursor:pointer;text-decoration:none;transition:0.3s;}";
  html += ".btn:hover{opacity:0.85;transform:translateY(-2px);}";
  html += "</style>";
  html += "</head><body>";
  html += "<div class='container'>";
  html += "<h1>ESP32 LED 控制</h1>";

  // 根据 LED 状态动态生成不同的显示内容和按钮。
  // 点亮时显示绿色圆点，并提供“关闭 LED”的红色按钮；
  // 熄灭时显示灰色圆点，并提供“打开 LED”的绿色按钮。
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

// 处理根路径 "/" 的函数。
// 当浏览器只输入 IP 地址（例如 http://192.168.1.105）时，默认访问的就是根路径。
// 这里返回生成的 HTML 首页。
void handleRoot() {
  // server.send(状态码, 内容类型, 正文内容) 把响应发回浏览器。
  // "text/html; charset=utf-8" 告诉浏览器：这是一段 HTML 文本，字符编码是 UTF-8，
  // 这样中文才能正常显示，不会乱码。
  server.send(200, "text/html; charset=utf-8", generateHTML());
}

// 处理 "/on" 请求的函数。
// 当用户点击“打开 LED”按钮时，浏览器会访问 http://IP地址/on，
// 服务器收到这个请求后把 LED 点亮，然后重定向回首页。
void handleOn() {
  ledState = true;
  digitalWrite(LED_PIN, HIGH);
  Serial.println("Web 命令：开灯");

  // 重定向：告诉浏览器“请重新访问 /”。
  // 使用 302 状态码和 Location 头实现。
  // 这样做的好处是：用户操作完 LED 后，浏览器会自动回到首页，
  // 看到更新后的 LED 状态和按钮，而不是停留在一个空白页面。
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "");
}

// 处理 "/off" 请求的函数。逻辑与 handleOn 对应。
void handleOff() {
  ledState = false;
  digitalWrite(LED_PIN, LOW);
  Serial.println("Web 命令：关灯");

  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "");
}

// 处理不存在路径的函数，即 404 页面。
// 当用户输入了服务器没有注册的路径时，返回友好的中文提示。
void handleNotFound() {
  server.send(404, "text/plain; charset=utf-8", "页面不存在");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("================================");
  Serial.println("ESP32 教程 07：Web 服务器控制 LED");
  Serial.println("================================");

  // 初始化 LED 引脚为输出模式，并默认熄灭。
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // 连接 Wi-Fi。流程和教程 06 相同，这里不再赘述。
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("连接 Wi-Fi");
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 30) {
    delay(500);
    Serial.print(".");
    retry++;
  }
  Serial.println();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi 连接失败，请检查 ssid 和密码");
    return;  // 连接失败直接返回，不再启动 Web 服务器
  }

  Serial.print("Wi-Fi 已连接，IP 地址：");
  Serial.println(WiFi.localIP());
  // 把访问地址也打印出来，方便复制到浏览器。
  Serial.print("请用浏览器访问：http://");
  Serial.println(WiFi.localIP());

  // 注册路由处理函数。
  // server.on(path, handler) 的意思是：当浏览器访问指定路径时，
  // 调用对应的 handler 函数来处理。
  // 这里注册了 3 个路径和一个默认的 404 处理函数。
  server.on("/", handleRoot);
  server.on("/on", handleOn);
  server.on("/off", handleOff);
  server.onNotFound(handleNotFound);

  // 启动 Web 服务器，开始监听 80 端口。
  // 从这行开始，ESP32 就在后台等待浏览器连接了。
  server.begin();
  Serial.println("HTTP 服务器已启动，等待浏览器连接...");
}

void loop() {
  // server.handleClient() 负责处理客户端（浏览器）发来的请求。
  // 必须在 loop() 中反复调用，否则服务器不会响应。
  // 它不会阻塞程序太久，如果没有请求就会立即返回，所以主循环还能做别的事。
  server.handleClient();
}

/*
 * 故障排查：
 * 1. 浏览器提示“无法访问此网站”：
 *    - 确认手机和 ESP32 连接的是同一个 Wi-Fi。
 *    - 确认串口监视器打印的 IP 地址没有抄错。
 *    - 某些公共 Wi-Fi/校园网会禁止设备之间互相访问，可换家用路由器测试。
 * 2. 网页显示乱码：
 *    - 检查 server.send() 的内容类型里是否包含 "charset=utf-8"。
 *    - 检查 HTML 头部是否有 <meta charset='UTF-8'>。
 * 3. 点击按钮没有反应：
 *    - 观察串口监视器是否有“Web 命令：开灯/关灯”输出，确认请求是否到达。
 *    - 检查 GPIO2 上是否接了 LED，且 LED 方向没有接反。
 * 
 * 扩展练习：
 * 1. 增加 /api/status 接口，用 JSON 格式返回 LED 当前状态，
 *    例如 {"led":true}。这样其他程序可以通过 API 读取状态。
 * 2. 增加 PWM 调光滑块，用 /set?duty=128 的查询参数方式控制 LED 亮度，
 *    并在 loop() 里用 server.arg("duty") 读取参数。
 * 3. 用 mDNS 让 ESP32 可以通过 http://esp32.local 访问，就不用记 IP 地址了。
 *    （extensions/ 目录有专门教程。）
 * 4. 给网页增加自动刷新，或改用 AJAX 请求，使页面不用整体刷新就能更新状态。
 */
