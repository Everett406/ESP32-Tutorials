/*
 * 综合项目 P10：触摸 + Web 双控灯
 *
 * 学习目标：
 * 1. 把电容触摸、PWM 调光、Web 服务器、mDNS 四个技能整合到一个项目里
 * 2. 理解"本地控制 + 远程控制"双通道的设计思路
 * 3. 学会处理两个控制源可能冲突的状态同步问题
 * 4. 掌握用 HTML 表单实现网页亮度调节
 * 5. 加深对 ESP32 Arduino 3.x LEDC API 的理解
 *
 * 涉及知识点：
 * - 触摸传感器（touchRead、阈值判断、边沿检测）
 * - PWM 调光（ledcAttach、ledcWrite）
 * - Wi-Fi 联网（WiFi.h）
 * - HTTP Web 服务器（WebServer.h）
 * - mDNS 域名解析（ESPmDNS.h）
 *
 * 硬件连接：
 * - ESP32 GPIO2  →  LED 正极（长脚）
 * - LED 负极（短脚） →  330Ω 电阻  →  ESP32 GND
 * - ESP32 GPIO4  →  导线/铜箔/硬币（触摸电极）
 *
 * 为什么 LED 要串 330Ω 电阻？
 * LED 导通后内阻很小，直接接 3.3V 会过流烧毁。
 * 330Ω 把电流限制在约 10mA，让 LED 安全工作。
 *
 * 为什么触摸电极只用一根导线？
 * 电容触摸检测的是引脚对地的等效电容变化，不需要形成闭合回路。
 * 人手靠近时相当于给电极增加了一个并联电容，touchRead() 读数会下降。
 */

// 引入 Wi-Fi、Web 服务器和 mDNS 库，这三个都是 ESP32 Arduino 核心自带的库
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>

// ===== 修改这里：填入你的 Wi-Fi 信息 =====
// const char* 表示"指向常量字符的指针"，用来保存不会修改的字符串
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// LED 引脚。GPIO2 是大多数 ESP32 开发板板载 LED 所在引脚，方便观察效果
#define LED_PIN 2

// 触摸引脚。ESP32 经典款支持触摸的引脚有：
// GPIO0、GPIO2、GPIO4、GPIO12、GPIO13、GPIO14、GPIO15、GPIO27、GPIO32、GPIO33
#define TOUCH_PIN 4

// PWM 参数
// PWM_FREQ：PWM 频率，单位 Hz。5000Hz 足够快，肉眼看不到闪烁
#define PWM_FREQ 5000
// PWM_RESOLUTION：PWM 分辨率，8 bit 表示占空比范围 0~255
#define PWM_RESOLUTION 8

// 触摸阈值。当 touchRead() 返回值低于这个值时，认为"被触摸了"
// 这个值需要根据实际情况微调，建议先上传程序观察串口打印的触摸值
#define TOUCH_THRESHOLD 30

// 创建 WebServer 对象，监听 80 端口
// 80 端口是 HTTP 默认端口，浏览器访问时不需要额外写 ":80"
WebServer server(80);

// mDNS 主机名。"esp32" 对应域名 "esp32.local"
// 局域网内支持 mDNS 的设备可以用这个域名访问 ESP32
const char* MDNS_HOSTNAME = "esp32";

// 标记 Web 服务器和 mDNS 是否成功启动
bool webServerStarted = false;
bool mdnsStarted = false;

// LED 当前开关状态。true 表示点亮，false 表示熄灭
bool ledState = false;

// LED 当前亮度，范围 0~255。亮度为 0 时即使"开灯"也看不到光
int brightness = 255;

// 触摸状态相关变量，用于边沿检测和消抖
bool lastTouchState = false;              // 上一次循环是否检测到触摸
unsigned long lastTouchTime = 0;          // 上次触摸触发的时间戳
const unsigned long TOUCH_DEBOUNCE = 300; // 消抖间隔，单位毫秒

// 函数前置声明。C/C++ 中函数必须先声明后使用，或者把定义写在调用前面
void applyLED();
String generateHTML();
void handleRoot();
void handleOn();
void handleOff();
void handleSet();

void setup() {
  // 初始化串口，波特率 115200
  Serial.begin(115200);

  // 等待 1 秒，让串口监视器有时间打开，避免错过开头信息
  delay(1000);

  Serial.println("================================");
  Serial.println("综合项目 P10：触摸 + Web 双控灯");
  Serial.println("================================");

  // ESP32 Arduino 3.x 的 PWM 初始化方式：
  // ledcAttach(pin, freq, resolution) 把指定引脚绑定到 LEDC 硬件
  // 8 bit 分辨率下，ledcWrite() 的 duty 范围是 0~255
  ledcAttach(LED_PIN, PWM_FREQ, PWM_RESOLUTION);

  // 初始化时先把 LED 关掉
  ledcWrite(LED_PIN, 0);

  // 连接 Wi-Fi
  // WiFi.mode(WIFI_STA) 设置为 STA 模式，ESP32 作为客户端连接路由器
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("正在连接 Wi-Fi");
  int retry = 0;
  // 最多等待 30 次，每次 500ms，约 15 秒
  while (WiFi.status() != WL_CONNECTED && retry < 30) {
    delay(500);
    Serial.print(".");
    retry++;
  }
  Serial.println();

  // 如果连接失败，打印提示并停止初始化后续功能
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi 连接失败，请检查 ssid 和密码");
    Serial.println("触摸控制仍可工作，Web 控制需要连接 Wi-Fi 后才能使用");
    return;
  }

  Serial.print("Wi-Fi 已连接，IP 地址：");
  Serial.println(WiFi.localIP());

  // 启动 mDNS 服务
  // mDNS（Multicast DNS，多播 DNS）允许设备在局域网内广播自己的名字，
  // 不需要专门的 DNS 服务器，其他设备就能通过 "esp32.local" 找到它
  if (!MDNS.begin(MDNS_HOSTNAME)) {
    Serial.println("mDNS 启动失败，将使用 IP 地址访问");
  } else {
    mdnsStarted = true;
    Serial.print("mDNS 已启动，可通过 http://");
    Serial.print(MDNS_HOSTNAME);
    Serial.println(".local 访问");

    // 向局域网宣告本设备提供 HTTP 服务，方便网络工具发现
    MDNS.addService("http", "tcp", 80);
  }

  // 注册 Web 服务器路由
  // server.on(path, handler)：当浏览器访问指定路径时，调用对应函数处理
  server.on("/", handleRoot);    // 首页
  server.on("/on", handleOn);    // 开灯
  server.on("/off", handleOff);  // 关灯
  server.on("/set", handleSet);  // 设置亮度

  // 处理未注册的路径
  server.onNotFound(handleRoot);

  // 启动 Web 服务器，开始监听 80 端口
  server.begin();
  webServerStarted = true;
  Serial.println("HTTP 服务器已启动");
}

void loop() {
  // 只有在 setup() 中成功启动后才处理网络相关任务
  if (webServerStarted) {
    // server.handleClient() 必须在 loop() 中反复调用，否则浏览器请求不会被响应
    // 它的执行时间很短，没有请求时立即返回，不会阻塞其他任务
    server.handleClient();
  }

  if (mdnsStarted) {
    // MDNS.update() 也要定期调用，维持 mDNS 服务，响应局域网内的域名查询
    MDNS.update();
  }

  // ===== 触摸检测 =====
  // touchRead(pin) 读取引脚电容触摸值。
  // 返回值越小，代表电容越大，越接近"被触摸"状态
  int touchValue = touchRead(TOUCH_PIN);

  // 根据阈值判断是否被触摸
  bool isTouched = (touchValue < TOUCH_THRESHOLD);

  // 边沿检测 + 时间消抖：
  // 1. 现在被触摸了，上一次没被触摸（边沿检测）
  // 2. 距离上次触发已经超过消抖时间（防止一次触摸被多次触发）
  if (isTouched && !lastTouchState && (millis() - lastTouchTime > TOUCH_DEBOUNCE)) {
    // 切换 LED 开关状态
    ledState = !ledState;
    applyLED();

    lastTouchTime = millis();

    Serial.print("触摸触发，LED 状态：");
    Serial.println(ledState ? "点亮" : "熄灭");
  }

  // 保存本次状态，供下次循环比较
  lastTouchState = isTouched;

  // 每 500ms 打印一次触摸值，方便观察和校准阈值
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 500) {
    lastPrint = millis();
    Serial.print("触摸值：");
    Serial.print(touchValue);
    Serial.print("  |  LED 状态：");
    Serial.print(ledState ? "开" : "关");
    Serial.print("  |  亮度：");
    Serial.println(brightness);
  }
}

// 根据 ledState 和 brightness 更新 LED 实际亮度
void applyLED() {
  // 如果 LED 处于关闭状态，无论亮度是多少都输出 0
  // 如果处于开启状态，输出当前设定的 brightness
  int duty = ledState ? brightness : 0;

  // ledcWrite(pin, duty) 输出指定占空比的 PWM 波形
  // duty 越大，LED 越亮
  ledcWrite(LED_PIN, duty);
}

// 生成网页内容
String generateHTML() {
  // String 是 Arduino 的字符串类，支持用 += 拼接
  String html = "<!DOCTYPE html><html lang='zh-CN'>";
  html += "<head>";
  html += "<meta charset='UTF-8'>";
  // viewport 让手机浏览器以正确比例显示页面
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>ESP32 双控灯</title>";
  html += "<style>";
  html += "body{font-family:'Microsoft YaHei',Arial,sans-serif;text-align:center;background:#f5f5f5;margin:0;padding:20px;}";
  html += ".container{max-width:400px;margin:40px auto;background:white;padding:30px;border-radius:20px;box-shadow:0 4px 15px rgba(0,0,0,0.1);}";
  html += "h1{color:#333;}";
  html += ".status{font-size:60px;margin:15px 0;}";
  html += ".info{color:#666;font-size:14px;margin-bottom:20px;}";
  html += ".btn{display:inline-block;padding:12px 40px;font-size:20px;color:white;border:none;border-radius:10px;cursor:pointer;text-decoration:none;margin:5px;transition:0.3s;}";
  html += ".btn:hover{opacity:0.85;transform:translateY(-2px);}";
  html += ".slider{width:80%;margin:15px 0;}";
  html += "</style>";
  html += "</head><body>";
  html += "<div class='container'>";
  html += "<h1>ESP32 双控灯</h1>";
  html += "<p class='info'>访问地址：http://esp32.local</p>";

  // 根据 LED 状态动态显示
  if (ledState) {
    html += "<div class='status' style='color:#4CAF50'>●</div>";
    html += "<p>LED 状态：<strong>点亮</strong></p>";
    html += "<a href='/off' class='btn' style='background:#f44336'>关闭 LED</a>";
  } else {
    html += "<div class='status' style='color:#ccc'>●</div>";
    html += "<p>LED 状态：<strong>熄灭</strong></p>";
    html += "<a href='/on' class='btn' style='background:#4CAF50'>打开 LED</a>";
  }

  // 亮度滑块表单
  // type='range' 是 HTML5 的滑块输入控件，min 和 max 决定范围
  html += "<p>当前亮度：" + String(brightness) + "/255</p>";
  html += "<form action='/set' method='GET'>";
  html += "<input type='range' name='duty' class='slider' min='0' max='255' value='" + String(brightness) + "'>";
  html += "<br><input type='submit' class='btn' style='background:#2196F3' value='设置亮度'>";
  html += "</form>";

  html += "</div></body></html>";
  return html;
}

// 处理首页请求
void handleRoot() {
  // server.send(状态码, 内容类型, 正文)
  // "text/html; charset=utf-8" 确保中文正常显示
  server.send(200, "text/html; charset=utf-8", generateHTML());
}

// 处理 /on 请求：开灯
void handleOn() {
  ledState = true;
  applyLED();
  Serial.println("Web 命令：开灯");

  // 用 302 重定向回首页，让用户看到更新后的状态
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "");
}

// 处理 /off 请求：关灯
void handleOff() {
  ledState = false;
  applyLED();
  Serial.println("Web 命令：关灯");

  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "");
}

// 处理 /set 请求：设置亮度
void handleSet() {
  // server.arg("duty") 读取 URL 查询参数 duty 的值
  // 例如 /set?duty=128，读取到的字符串就是 "128"
  if (server.hasArg("duty")) {
    int newDuty = server.arg("duty").toInt();

    // constrain(value, min, max) 把值限制在指定范围内，防止非法输入
    brightness = constrain(newDuty, 0, 255);

    // 设置亮度后自动把灯打开，方便观察效果
    ledState = true;
    applyLED();

    Serial.print("Web 命令：设置亮度为 ");
    Serial.println(brightness);
  }

  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "");
}

/*
 * 扩展挑战：
 * 1. 增加网页自动刷新（<meta http-equiv='refresh' content='5'>），
 *    这样用手机控制时，另一端也能看到最新状态。
 * 2. 用 AJAX 代替页面跳转，实现无刷新控制。
 * 3. 增加一个 /api/status 接口，以 JSON 格式返回 ledState 和 brightness，
 *    供其他程序调用。
 * 4. 把触摸电极换成弹簧或铜箔片，做一个精致的桌面触控台灯。
 */
