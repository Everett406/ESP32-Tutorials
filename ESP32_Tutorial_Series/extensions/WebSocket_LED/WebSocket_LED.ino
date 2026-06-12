/*
 * ESP32 扩展教程：WebSocket_LED - 实时双向控制 LED
 *
 * 学习目标：
 * 1. 理解 WebSocket 与传统 HTTP 请求的区别
 * 2. 学会使用 WebSocketsServer 库建立 WebSocket 服务器
 * 3. 实现网页端实时控制 LED，并实时反馈 LED 状态
 * 4. 理解“全双工通信”和“持久连接”的概念
 *
 * 需要安装的库：
 * - 库名称：WebSockets
 * - 作者：Markus Sattler
 * - 安装方法：
 *   1. 在 Arduino IDE 中点击“项目”→“加载库”→“管理库...”。
 *   2. 在搜索框输入 "WebSockets"。
 *   3. 找到作者为 Markus Sattler 的 "WebSockets"，点击“安装”。
 *   4. 如果提示安装依赖库，请选择“安装全部”。
 *
 * 前置知识：
 * - 建议先学习第 06 课（WiFi_Connect）和第 07 课（Web_Server_LED）。
 * - 了解 HTTP 请求/响应、IP 地址、STA 模式的基本概念。
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
 * 2. 安装 WebSockets 库后上传代码到 ESP32。
 * 3. 打开串口监视器，获取 ESP32 的 IP 地址。
 * 4. 确保手机/电脑和 ESP32 连接到同一个 Wi-Fi。
 * 5. 打开浏览器，访问 ESP32 的 IP 地址，即可看到控制页面。
 * 6. 点击页面按钮控制 LED，LED 状态会实时同步到所有连接的网页上。
 */

#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>

// ===== 修改这里：填入你的 Wi-Fi 信息 =====
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// LED 引脚选择 GPIO2，这是大多数 ESP32 开发板板载 LED 所在引脚。
#define LED_PIN 2

// 创建 WebServer 对象，监听 80 端口，用来提供网页。
WebServer server(80);

// 创建 WebSocketsServer 对象，监听 81 端口，用来处理实时通信。
// WebSocket 不使用默认的 80 端口，是为了和 HTTP 服务器分开，避免冲突。
// 81 端口也是常见的 WebSocket 测试端口。
WebSocketsServer webSocket = WebSocketsServer(81);

// 记录 LED 当前状态。
bool ledState = false;

/*
 * 生成网页内容
 * 这个网页内部包含 JavaScript，用来与 ESP32 建立 WebSocket 连接。
 * 为什么把 HTML 直接写在代码里？
 * 因为 ESP32 的 Flash 空间有限，这样不需要额外的 SD 卡或文件系统，
 * 一个程序就能独立完成服务器和网页的提供。
 */
String generateHTML() {
  String html = "<!DOCTYPE html><html lang='zh-CN'>";
  html += "<head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>ESP32 WebSocket LED</title>";
  html += "<style>";
  html += "body{font-family:'Microsoft YaHei',Arial,sans-serif;text-align:center;background:#f0f2f5;margin:0;padding:20px;}";
  html += ".container{max-width:400px;margin:40px auto;background:white;padding:30px;border-radius:20px;box-shadow:0 4px 20px rgba(0,0,0,0.1);}";
  html += "h1{color:#333;}";
  html += ".info{color:#666;font-size:14px;margin-bottom:20px;}";
  html += ".status{font-size:60px;margin:20px 0;}";
  html += ".btn{display:inline-block;padding:15px 50px;font-size:22px;color:white;border:none;border-radius:10px;cursor:pointer;text-decoration:none;transition:0.3s;margin:5px;}";
  html += ".btn:hover{opacity:0.85;transform:translateY(-2px);}";
  html += "#log{text-align:left;background:#f8f9fa;padding:10px;border-radius:8px;font-size:12px;height:100px;overflow-y:auto;color:#555;margin-top:15px;}";
  html += "</style>";
  html += "</head><body>";
  html += "<div class='container'>";
  html += "<h1>WebSocket 实时控制</h1>";
  html += "<p class='info'>WebSocket 连接建立后，所有操作都会实时同步</p>";
  html += "<div class='status' id='ledIcon' style='color:#ccc'>●</div>";
  html += "<p>LED 状态：<strong id='ledText'>熄灭</strong></p>";
  html += "<button class='btn' style='background:#4CAF50' onclick='sendCmd(\"on\")'>打开 LED</button>";
  html += "<button class='btn' style='background:#f44336' onclick='sendCmd(\"off\")'>关闭 LED</button>";
  html += "<div id='log'></div>";

  // ===== 网页端的 JavaScript =====
  html += "<script>";
  // 获取当前网页地址，把 http:// 替换成 ws://，把端口 80 替换成 81。
  // WebSocket 的 URL 协议是 ws://（未加密）或 wss://（加密）。
  html += "var gateway = 'ws://' + window.location.hostname + ':81/';";
  html += "var websocket;";

  // log 函数在页面上输出调试信息，方便观察连接状态。
  html += "function log(msg) { document.getElementById('log').innerHTML += msg + '<br>'; }";

  // initWebSocket() 负责创建 WebSocket 连接。
  html += "function initWebSocket() {";
  html += "  log('正在连接 WebSocket：' + gateway);";
  html += "  websocket = new WebSocket(gateway);";

  // onopen：连接成功时触发。
  html += "  websocket.onopen = function() { log('WebSocket 已连接'); sendCmd('status'); };";

  // onclose：连接关闭时触发，3 秒后自动重连。
  html += "  websocket.onclose = function() { log('WebSocket 已断开，3秒后重连...'); setTimeout(initWebSocket, 3000); };";

  // onerror：发生错误时触发。
  html += "  websocket.onerror = function(e) { log('WebSocket 错误'); };";

  // onmessage：收到服务器消息时触发，根据消息更新页面显示。
  html += "  websocket.onmessage = function(event) {";
  html += "    var data = event.data;";
  html += "    log('收到：' + data);";
  html += "    if (data === 'on') { document.getElementById('ledIcon').style.color = '#4CAF50'; document.getElementById('ledText').innerText = '点亮'; }";
  html += "    else if (data === 'off') { document.getElementById('ledIcon').style.color = '#ccc'; document.getElementById('ledText').innerText = '熄灭'; }";
  html += "  };";
  html += "}";

  // sendCmd() 通过 WebSocket 发送命令给 ESP32。
  html += "function sendCmd(cmd) { if (websocket && websocket.readyState === 1) { websocket.send(cmd); log('发送：' + cmd); } else { log('WebSocket 未连接，无法发送'); } }";

  // 页面加载完成后立即初始化 WebSocket。
  html += "window.onload = initWebSocket;";
  html += "</script>";

  html += "</div></body></html>";
  return html;
}

// 处理 HTTP 根路径，返回网页。
void handleRoot() {
  server.send(200, "text/html; charset=utf-8", generateHTML());
}

/*
 * 处理不存在的 HTTP 路径
 */
void handleNotFound() {
  server.send(404, "text/plain; charset=utf-8", "页面不存在");
}

/*
 * WebSocket 事件处理函数
 * 这是 WebSocketsServer 库规定的回调函数格式：
 * void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length)
 * - num：客户端编号，每个连接的设备有一个唯一编号。
 * - type：事件类型，例如连接、断开、收到文本消息、收到二进制消息等。
 * - payload：收到的数据内容。
 * - length：数据长度。
 */
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      // 有客户端断开连接。
      Serial.print("客户端 ");
      Serial.print(num);
      Serial.println(" 已断开");
      break;

    case WStype_CONNECTED:
      // 有新客户端连接。
      {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.print("客户端 ");
        Serial.print(num);
        Serial.print(" 已连接，IP：");
        Serial.println(ip);

        // 新客户端连接后，立即把当前 LED 状态发给它，
        // 这样页面上显示的状态就和实际状态一致。
        webSocket.sendTXT(num, ledState ? "on" : "off");
      }
      break;

    case WStype_TEXT:
      // 收到文本消息。
      // payload 是 uint8_t* 类型，我们把它当作字符串处理。
      Serial.print("收到客户端 ");
      Serial.print(num);
      Serial.print(" 消息：");
      Serial.println((char*)payload);

      // 根据收到的命令执行操作。
      if (strcmp((char*)payload, "on") == 0) {
        ledState = true;
        digitalWrite(LED_PIN, HIGH);
        Serial.println("执行：开灯");

        // broadcastTXT() 向所有已连接的客户端发送消息。
        // 这样如果多个浏览器同时打开页面，它们的显示都会同步更新。
        webSocket.broadcastTXT("on");
      }
      else if (strcmp((char*)payload, "off") == 0) {
        ledState = false;
        digitalWrite(LED_PIN, LOW);
        Serial.println("执行：关灯");
        webSocket.broadcastTXT("off");
      }
      else if (strcmp((char*)payload, "status") == 0) {
        // 客户端请求当前状态，单独发给这个客户端。
        webSocket.sendTXT(num, ledState ? "on" : "off");
      }
      else {
        // 未知命令，返回错误提示。
        webSocket.sendTXT(num, "unknown command");
      }
      break;

    case WStype_BIN:
      // 收到二进制消息，本示例不处理。
      Serial.println("收到二进制消息，忽略");
      break;

    default:
      // 其他类型事件，暂不处理。
      break;
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("================================");
  Serial.println("ESP32 扩展教程：WebSocket LED");
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

  // ===== 启动 WebSocket 服务器 =====
  // WebSocket 是一种在单个 TCP 连接上进行全双工通信的协议。
  //
  // 全双工（Full-Duplex）的意思是：通信双方可以同时发送和接收数据，
  // 就像打电话一样，双方可以同时说话和听。
  //
  // 这与 HTTP 不同：HTTP 是“请求-响应”模式，客户端发一个请求，服务器返回一个响应，
  // 然后连接就关闭了。如果服务器想主动给客户端发消息，HTTP 做不到。
  //
  // WebSocket 建立连接后，会保持一条“持久连接”，
  // 服务器可以随时主动推送消息给客户端，非常适合实时控制场景。
  webSocket.begin();

  // 注册 WebSocket 事件处理函数。
  // 当客户端连接、断开或发送消息时，库会自动调用这个函数。
  webSocket.onEvent(webSocketEvent);

  Serial.println("WebSocket 服务器已启动，端口：81");

  // 注册 HTTP 路由
  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);

  // 启动 HTTP 服务器
  server.begin();
  Serial.println("HTTP 服务器已启动，端口：80");
}

void loop() {
  // 处理 HTTP 请求
  server.handleClient();

  // 处理 WebSocket 事件
  // 必须在 loop() 中不断调用，否则无法及时响应客户端消息。
  webSocket.loop();
}

/*
 * 扩展练习：
 * 1. 在网页上添加一个滑块，用 WebSocket 实时发送 PWM 占空比值，
 *    实现 LED 亮度的无级调节。
 * 2. 把 ESP32 的某个传感器数据（例如触摸值、ADC 读数）通过 WebSocket
 *    定时推送到网页上显示，实现实时数据监控。
 * 3. 给 WebSocket 增加用户名/密码验证，或在连接时检查客户端 IP，
 *    做一个简单的访问控制。
 */
