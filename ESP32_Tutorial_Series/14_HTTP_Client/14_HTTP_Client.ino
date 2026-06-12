/*
 * ESP32 教程系列 14：HTTP 客户端 —— 访问网络 API 获取数据
 * 
 * 学习目标：
 * 1. 理解 HTTP 协议中 GET 请求和响应的基本概念
 * 2. 学会使用 HTTPClient.h 发送 GET 请求
 * 3. 理解 HTTP 状态码的含义
 * 4. 学会解析简单的 JSON 响应
 * 5. 理解什么是 API，以及为什么设备需要调用 API
 * 
 * 前置知识：
 * - 已经会用 WiFi.begin() 连接 Wi-Fi
 * - 已经会用 Serial.println() 输出调试信息
 * 
 * 硬件连接：
 * - 本示例不需要额外硬件，ESP32 本身通过 Wi-Fi 联网
 * - 可选：ESP32 GPIO2  →  LED 正极，负极串 330Ω 电阻到 GND，
 *        用来直观显示请求成功/失败
 * 
 * 准备工作：
 * - 需要一个可以访问互联网的 2.4GHz Wi-Fi
 * - 本示例访问 httpbin.org，这是一个专门用来测试 HTTP 请求的公开网站
 */

// 包含 WiFi 库，用于连接无线网络
#include <WiFi.h>

// 包含 HTTPClient 库，用于发送 HTTP 请求
#include <HTTPClient.h>

// ===== 修改这里：填入你的 Wi-Fi 信息 =====
const char* ssid = "YOUR_WIFI_SSID";         // Wi-Fi 名称，也叫 SSID
const char* password = "YOUR_WIFI_PASSWORD"; // Wi-Fi 密码

// 测试 API 地址。
// httpbin.org 是一个免费的 HTTP 测试服务，
// 它会把你发送的请求内容原样返回，方便我们观察。
// /get 表示发送一个 GET 请求。
const char* testURL = "http://httpbin.org/get";

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("================================");
  Serial.println("ESP32 教程 14：HTTP 客户端");
  Serial.println("================================");

  // WiFi.mode(WIFI_STA) 把 ESP32 设置为 STA 模式。
  // STA 是 Station 的缩写，意思是"站点"，也就是作为客户端去连接路由器。
  // 另一种常见模式是 AP 模式（Access Point，热点模式），ESP32 自己当路由器。
  WiFi.mode(WIFI_STA);

  // WiFi.begin() 开始连接 Wi-Fi。
  // 第一个参数是 Wi-Fi 名称（SSID），第二个参数是密码。
  WiFi.begin(ssid, password);

  Serial.print("正在连接 Wi-Fi");

  // 等待 Wi-Fi 连接成功，最多等待 30 次，每次 500ms，也就是最多 15 秒。
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 30) {
    delay(500);
    Serial.print(".");
    retry++;
  }
  Serial.println();

  // 如果 15 秒后还没连上，打印错误并结束 setup() 中后续操作
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi 连接失败，请检查名称和密码");
    return;
  }

  // WiFi.localIP() 返回 ESP32 在局域网中分配的 IP 地址。
  // IP 地址是设备在网络中的"门牌号"，其他设备可以通过这个地址找到它。
  Serial.print("Wi-Fi 已连接，ESP32 的 IP 地址：");
  Serial.println(WiFi.localIP());

  // 创建一个 HTTPClient 对象。
  // HTTP 是一种应用层协议，规定了客户端和服务器之间如何交换数据。
  // 在这个示例中，ESP32 是客户端，httpbin.org 是服务器。
  HTTPClient http;

  Serial.print("正在访问：");
  Serial.println(testURL);

  // http.begin(url) 告诉 HTTPClient 要向哪个地址发送请求。
  // URL（Uniform Resource Locator，统一资源定位符）就是网页地址，
  // 包含协议（http://）、域名（httpbin.org）和路径（/get）。
  http.begin(testURL);

  // http.addHeader() 添加请求头。
  // HTTP 请求由三部分组成：请求行、请求头、请求体。
  // 请求头用来携带一些附加信息，例如浏览器类型、接收格式等。
  // User-Agent 用来告诉服务器"我是谁"。
  // 有些服务器会拒绝没有 User-Agent 的请求，所以我们加上它。
  http.addHeader("User-Agent", "ESP32-Tutorial");

  // http.GET() 发送一个 HTTP GET 请求，并返回服务器响应的状态码。
  // 
  // GET 是什么意思？
  // GET 是 HTTP 协议中的一种请求方法，表示"从服务器获取数据"。
  // 与之对应的常见方法还有 POST（提交数据）、PUT、DELETE 等。
  // 
  // 状态码是什么？
  // 状态码是服务器返回的一个三位数字，表示请求的处理结果：
  // - 200：OK，请求成功
  // - 404：Not Found，请求的资源不存在
  // - 500：Internal Server Error，服务器内部出错
  // - 401：Unauthorized，需要身份验证
  // - 503：Service Unavailable，服务不可用
  int httpCode = http.GET();

  Serial.print("HTTP 状态码：");
  Serial.println(httpCode);

  if (httpCode == 200) {
    // http.getString() 读取服务器返回的完整响应内容。
    // httpbin.org/get 会把我们发送的请求信息以 JSON 格式返回。
    String response = http.getString();
    Serial.println("服务器响应内容：");
    Serial.println(response);
  } else {
    Serial.println("请求失败，请检查网络或 URL 是否正确");
  }

  // http.end() 结束这次 HTTP 请求，释放占用的资源。
  // 每次 HTTP 请求结束后都应该调用，否则可能造成内存泄漏。
  http.end();
}

void loop() {
  // setup() 里已经完成了一次请求。
  // loop() 里可以定时发送请求，例如每 30 秒获取一次天气数据。

  static unsigned long lastRequest = 0;

  // millis() 返回程序启动后经过的毫秒数。
  // 用 millis() 做定时比 delay() 更好，因为 delay() 会阻塞其他代码，
  // 而 millis() 检查是非阻塞的，loop() 里的其他代码仍然可以继续执行。
  if (millis() - lastRequest > 30000) {
    lastRequest = millis();

    // 每次请求前都要重新创建 HTTPClient 对象并 begin()
    HTTPClient http;
    http.begin(testURL);

    int httpCode = http.GET();
    Serial.print("定时请求状态码：");
    Serial.println(httpCode);

    http.end();
  }

  // 短暂延时，避免 loop() 空转占用太多 CPU
  delay(100);
}

/*
 * 重要概念解释：
 * 
 * 1. 什么是 API？
 *    API（Application Programming Interface，应用程序编程接口）
 *    是一组规定了"如何与某个服务交互"的规则。
 *    例如天气 API 允许你发送一个请求，它返回当前天气数据；
 *    本示例用的 httpbin.org/get 就是一个测试 API，
 *    它把请求信息原样返回，帮助我们学习 HTTP。
 * 
 * 2. 什么是 JSON？
 *    JSON（JavaScript Object Notation）是一种轻量级数据交换格式，
 *    人和机器都容易读写。
 *    例如：
 *    {
 *      "name": "ESP32",
 *      "temperature": 25.6
 *    }
 *    ESP32 可以用 ArduinoJson 库来解析 JSON。
 * 
 * 3. GET 和 POST 的区别：
 *    - GET：从服务器获取数据，参数通常放在 URL 后面，适合查询。
 *    - POST：向服务器提交数据，参数放在请求体里，适合上传、登录等。
 * 
 * 4. 为什么用 HTTP 而不是 HTTPS？
 *    HTTPS 在 HTTP 基础上加了 SSL/TLS 加密，更安全但更复杂。
 *    ESP32 访问 HTTPS 需要配置服务器根证书。
 *    本示例为了简化，使用 HTTP。
 */

/*
 * 排查提示：
 * 1. 如果 Wi-Fi 连不上：
 *    - 确认 ssid 和 password 是否正确，注意大小写和空格。
 *    - 确认路由器是 2.4GHz，ESP32 不支持 5GHz Wi-Fi。
 *    - 如果路由器有 MAC 地址过滤，需要把 ESP32 加入白名单。
 *
 * 2. 如果 HTTP 状态码是 -1 或连接失败：
 *    - 确认 ESP32 是否已经成功连接 Wi-Fi。
 *    - 确认目标服务器是否可用，可以在电脑浏览器里打开同一个 URL 测试。
 *    - 有些网络环境（例如需要登录的公共 Wi-Fi）会阻止设备访问外网。
 *
 * 3. 如果状态码是 404：
 *    - 说明 URL 路径写错了，检查 /get 是否拼写正确。
 *
 * 4. 如果状态码是 301 或 302：
 *    - 说明服务器要求重定向到 HTTPS 或其他地址。
 *    - 可以尝试把 URL 改成重定向后的地址。
 */

/*
 * 扩展练习：
 * 1. 用 POST 请求把传感器数据（例如温度）上传到服务器。
 * 2. 访问真实的天气 API（例如 OpenWeatherMap），并用 ArduinoJson 解析 JSON。
 * 3. 学习访问 HTTPS API，配置服务器根证书。
 */
