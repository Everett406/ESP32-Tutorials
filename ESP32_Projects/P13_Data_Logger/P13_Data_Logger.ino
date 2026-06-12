/**
 * 综合项目 P13：数据记录仪（Data Logger）
 *
 * 学习目标：
 * 1. 把多个技能整合到一个完整项目中：DHT22 传感器、硬件定时器、文件系统、Web 服务器、串口命令。
 * 2. 理解 LittleFS/SPIFFS 文件系统在 ESP32 上的作用，能把传感器数据持久化到 Flash。
 * 3. 理解 CSV（逗号分隔值）文件格式，方便后续用 Excel、Python 等分析。
 * 4. 学会用硬件定时器实现“每小时记录一次”的周期性任务，同时不阻塞主循环。
 * 5. 通过 Web 页面和串口命令两种方式导出 Flash 里的历史数据。
 *
 * 前置知识：
 * - 已学习《DHT11_DHT22_Sensor》《10_Timer_Interrupt》《14_HTTP_Client》《07_Web_Server_LED》。
 *
 * 硬件连接：
 * - DHT22 VCC  →  ESP32 3.3V
 * - DHT22 GND  →  ESP32 GND
 * - DHT22 DATA →  ESP32 GPIO4（建议 DATA 与 3.3V 之间接 4.7kΩ ~ 10kΩ 上拉电阻）
 * - LED 正极    →  ESP32 GPIO2（板载 LED 通常已接好）
 * - LED 负极    →  330Ω 电阻  →  GND
 *
 * 使用步骤：
 * 1. 在 Arduino IDE 中安装库：DHT sensor library（Adafruit）、Adafruit Unified Sensor。
 * 2. 选择“工具”→“Partition Scheme”，建议选带有 SPIFFS 或默认的分区表（Default 4MB with spiffs）。
 *    ESP32 Arduino 3.x 默认已包含 LittleFS，本例使用 LittleFS。
 * 3. 修改下方的 WIFI_SSID 和 WIFI_PASSWORD 为你自己的 2.4GHz Wi-Fi。
 * 4. 上传程序，打开串口监视器（波特率 115200）。
 * 5. 等待记录后，在浏览器输入串口打印的 IP 地址，点击“下载 CSV”即可拿到历史数据。
 *
 * 关键概念解释：
 *
 * 【LittleFS 是什么？】
 * LittleFS（Little File System）是一种轻量级文件系统，适合 Flash 存储。
 * 在 ESP32 Arduino 3.x 中，官方推荐使用 LittleFS 替代老旧的 SPIFFS。
 * 它可以把数据以“文件”的形式写入 ESP32 的 Flash，断电后仍然保留。
 *
 * 【Flash 写入寿命】
 * Flash 的每个存储单元擦写次数有限（通常约 10 万次）。
 * 本项目每小时才写一次，写入频率很低，不会很快耗尽寿命。
 * 如果改成每秒写一次，一年多就会写坏，所以记录间隔要根据实际需求选择。
 *
 * 【CSV 格式】
 * CSV（Comma-Separated Values）是常见的表格数据格式，
 * 每一行是一条记录，字段之间用逗号分隔。
 * 用记事本、Excel、Python pandas 都能直接打开分析。
 *
 * 【硬件定时器】
 * ESP32 的硬件定时器可以在设定时间到达时触发中断，
 * 不占用 CPU 主循环时间，适合周期性任务。
 * 这里用来每小时提醒主循环“该记一次数据了”。
 */

#include <WiFi.h>        // Wi-Fi 连接库
#include <WebServer.h>   // 简易 Web 服务器库
#include <LittleFS.h>    // ESP32 推荐使用的 Flash 文件系统
#include "DHT.h"         // DHT 温湿度传感器库

// ===== 修改这里：填入你的 Wi-Fi 信息 =====
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// DHT22 引脚与类型
#define DHT_PIN  4
#define DHT_TYPE DHT22

// 状态 LED 引脚
#define LED_PIN 2

// 记录间隔。
// 3600000 毫秒 = 1 小时，这是项目要求。
// 初次测试时可以改成 10000（10 秒），方便快速观察效果。
#define LOG_INTERVAL_MS 3600000UL

// CSV 文件路径，存放在 Flash 根目录
#define LOG_FILE "/log.csv"

// 创建 DHT 对象
DHT dht(DHT_PIN, DHT_TYPE);

// 创建 Web 服务器，监听 80 端口
WebServer server(80);

// 定时器相关变量
hw_timer_t* logTimer = NULL;          // 定时器指针
volatile bool logFlag = false;        // 定时器中断设置的标志位

// 记录序号，方便在串口看到这是第几条记录
volatile unsigned long recordCount = 0;

/**
 * 定时器中断服务函数。
 * 名字前面的 IRAM_ATTR 告诉编译器把这个函数放到 RAM 中，
 * 这样中断响应更快，不会因为从 Flash 读取代码而延迟。
 *
 * 中断里只做最简单的事：把 logFlag 置为 true。
 * 真正读取传感器、写文件等耗时操作放在主循环中执行，
 * 因为中断函数不能执行耗时或阻塞操作（例如写文件、串口大量输出）。
 */
void IRAM_ATTR onLogTimer() {
  logFlag = true;
}

/**
 * 初始化硬件定时器。
 * 定时器计数频率设为 1MHz，每微秒计数 1 次。
 * 报警值设为 LOG_INTERVAL_MS * 1000，也就是对应的微秒数。
 * 自动重载设为 true，这样到时间后会自动从零开始，实现周期性每小时触发。
 */
void initLogTimer() {
  // timerBegin(频率) 创建定时器，1MHz = 每秒计数 100 万次
  logTimer = timerBegin(1000000);

  // 绑定中断服务函数，&onLogTimer 表示取函数地址
  timerAttachInterrupt(logTimer, &onLogTimer);

  // timerAlarm(定时器, 计数值, 自动重载, 重复次数)
  // 计数值 = 微秒数；自动重载 true；0 表示无限次触发
  // 例如 1 小时 = 3600 * 1000 * 1000 = 3600000000 微秒
  timerAlarm(logTimer, (uint64_t)LOG_INTERVAL_MS * 1000ULL, true, 0);

  Serial.println("定时器已启动，将按设定间隔自动记录数据");
}

/**
 * 向 CSV 文件追加一条记录。
 * 参数：
 *   timestampMs - 程序运行以来的毫秒数（因为没有 RTC，用它作为相对时间戳）
 *   temperature - 温度值
 *   humidity    - 湿度值
 */
void appendLog(unsigned long timestampMs, float temperature, float humidity) {
  // LittleFS.open(路径, 模式) 打开文件。
  // "a" 是 append 模式：如果文件不存在则创建，存在则在末尾追加。
  File f = LittleFS.open(LOG_FILE, "a");
  if (!f) {
    Serial.println("打开日志文件失败，无法写入");
    return;
  }

  // 按 CSV 格式写入一行：时间戳,温度,湿度
  // 逗号后面不加空格，保持标准 CSV 格式
  f.print(timestampMs);
  f.print(",");
  f.print(temperature, 2);  // 保留 2 位小数
  f.print(",");
  f.println(humidity, 2);

  // 关闭文件，确保数据真正写入 Flash
  f.close();

  recordCount++;

  Serial.print("已记录第 ");
  Serial.print(recordCount);
  Serial.print(" 条数据：时间=");
  Serial.print(timestampMs);
  Serial.print("ms, 温度=");
  Serial.print(temperature, 2);
  Serial.print("℃, 湿度=");
  Serial.println(humidity, 2);
}

/**
 * 读取 DHT22 并记录到文件。
 * 如果读数失败，会打印错误但不会写入无效数据。
 */
void readAndLog() {
  // 读取温湿度
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  // 判断读数是否有效。isnan() 用于检查“Not a Number”。
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("DHT22 读取失败，本次跳过记录");
    return;
  }

  // 点亮 LED 100 毫秒，作为“正在记录”的视觉提示
  digitalWrite(LED_PIN, HIGH);
  appendLog(millis(), temperature, humidity);
  digitalWrite(LED_PIN, LOW);
}

/**
 * 生成 Web 首页 HTML。
 * 页面显示当前状态、已记录条数，并提供下载和清空按钮。
 */
String generateIndexHTML() {
  String html = "<!DOCTYPE html><html lang='zh-CN'>";
  html += "<head><meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>ESP32 数据记录仪</title>";
  html += "<style>";
  html += "body{font-family:'Microsoft YaHei',Arial,sans-serif;text-align:center;background:#f5f5f5;margin:0;padding:20px;}";
  html += ".container{max-width:500px;margin:30px auto;background:white;padding:30px;border-radius:20px;box-shadow:0 4px 15px rgba(0,0,0,0.1);}";
  html += "h1{color:#333;}";
  html += ".info{color:#666;font-size:14px;margin:10px 0;}";
  html += ".btn{display:inline-block;margin:10px;padding:12px 30px;font-size:18px;color:white;border:none;border-radius:8px;cursor:pointer;text-decoration:none;transition:0.3s;}";
  html += ".btn:hover{opacity:0.85;}";
  html += ".download{background:#4CAF50;}";
  html += ".clear{background:#f44336;}";
  html += "</style></head><body>";
  html += "<div class='container'>";
  html += "<h1>ESP32 数据记录仪</h1>";
  html += "<p class='info'>本设备每小时自动记录一次温湿度</p>";
  html += "<p class='info'>已记录条数：<strong>" + String((unsigned long)recordCount) + "</strong></p>";
  html += "<p class='info'>当前运行时间：<strong>" + String(millis() / 1000) + "</strong> 秒</p>";
  html += "<a href='/download' class='btn download'>下载 CSV</a>";
  html += "<a href='/clear' class='btn clear'>清空日志</a>";
  html += "</div></body></html>";
  return html;
}

/**
 * 处理根路径 "/" 的请求，返回首页。
 */
void handleRoot() {
  server.send(200, "text/html; charset=utf-8", generateIndexHTML());
}

/**
 * 处理 "/download" 请求，把 CSV 文件发给浏览器。
 * 通过设置 Content-Disposition 头，让浏览器自动弹出下载对话框。
 */
void handleDownload() {
  // 先检查文件是否存在
  if (!LittleFS.exists(LOG_FILE)) {
    server.send(404, "text/plain; charset=utf-8", "日志文件不存在");
    return;
  }

  File f = LittleFS.open(LOG_FILE, "r");
  if (!f) {
    server.send(500, "text/plain; charset=utf-8", "无法打开日志文件");
    return;
  }

  // 一次性读取文件内容。数据量不大时这样做最简单。
  String content = f.readString();
  f.close();

  // 设置响应头，告诉浏览器这是一个附件，文件名为 log.csv
  server.sendHeader("Content-Disposition", "attachment; filename=\"log.csv\"");
  server.send(200, "text/csv; charset=utf-8", content);
}

/**
 * 处理 "/clear" 请求，清空日志文件后重定向回首页。
 */
void handleClear() {
  // remove() 删除文件
  if (LittleFS.remove(LOG_FILE)) {
    // 重新创建带表头的空文件
    File f = LittleFS.open(LOG_FILE, "w");
    if (f) {
      f.println("elapsed_ms,temperature_c,humidity_pct");
      f.close();
    }
    recordCount = 0;
    Serial.println("日志已清空");
  } else {
    Serial.println("清空日志失败（文件可能不存在）");
  }

  // 重定向回首页
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "");
}

/**
 * 处理未注册路径，返回 404。
 */
void handleNotFound() {
  server.send(404, "text/plain; charset=utf-8", "页面不存在");
}

/**
 * 串口打印整个 CSV 文件内容。
 * 方便在没有浏览器时也能查看历史数据。
 */
void printLogToSerial() {
  if (!LittleFS.exists(LOG_FILE)) {
    Serial.println("日志文件不存在");
    return;
  }

  File f = LittleFS.open(LOG_FILE, "r");
  if (!f) {
    Serial.println("无法打开日志文件");
    return;
  }

  Serial.println("========== CSV 日志内容 ==========");
  while (f.available()) {
    Serial.write(f.read());  // 逐字节读取并输出
  }
  Serial.println("========== 日志结束 ==========");
  f.close();
}

/**
 * 处理串口命令。
 * 输入：
 *   download 或 d → 在串口打印 CSV 内容
 *   clear 或 c    → 清空日志
 *   info 或 i     → 打印运行状态
 */
void handleSerialCommands() {
  if (!Serial.available()) return;

  String cmd = Serial.readStringUntil('\n');  // 读取一行命令
  cmd.trim();  // 去掉首尾空格和换行符
  cmd.toLowerCase();  // 统一转小写，方便比较

  if (cmd == "download" || cmd == "d") {
    printLogToSerial();
  } else if (cmd == "clear" || cmd == "c") {
    if (LittleFS.remove(LOG_FILE)) {
      File f = LittleFS.open(LOG_FILE, "w");
      if (f) {
        f.println("elapsed_ms,temperature_c,humidity_pct");
        f.close();
      }
      recordCount = 0;
      Serial.println("日志已清空");
    }
  } else if (cmd == "info" || cmd == "i") {
    Serial.print("运行时间：");
    Serial.print(millis() / 1000);
    Serial.println(" 秒");
    Serial.print("已记录条数：");
    Serial.println((unsigned long)recordCount);
    Serial.print("文件系统总空间：");
    Serial.print(LittleFS.totalBytes());
    Serial.println(" 字节");
    Serial.print("文件系统已用空间：");
    Serial.print(LittleFS.usedBytes());
    Serial.println(" 字节");
  } else {
    Serial.println("未知命令。可用：download/d, clear/c, info/i");
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("================================");
  Serial.println("综合项目 P13：数据记录仪");
  Serial.println("================================");

  // 初始化 LED 引脚
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // 初始化 DHT 传感器
  dht.begin();
  Serial.println("DHT22 已初始化");

  // 初始化 LittleFS 文件系统
  // begin() 成功返回 true，失败返回 false
  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS 初始化失败，程序停止");
    while (true) {
      delay(1000);
    }
  }
  Serial.println("LittleFS 文件系统已挂载");

  // 如果日志文件不存在，创建并写入 CSV 表头
  if (!LittleFS.exists(LOG_FILE)) {
    File f = LittleFS.open(LOG_FILE, "w");
    if (f) {
      f.println("elapsed_ms,temperature_c,humidity_pct");
      f.close();
      Serial.println("已创建新的日志文件");
    }
  } else {
    // 如果文件已存在，统计已有记录条数（不包括表头）
    File f = LittleFS.open(LOG_FILE, "r");
    if (f) {
      String line;
      bool firstLine = true;
      while (f.available()) {
        line = f.readStringUntil('\n');
        if (line.length() > 0 && !firstLine) {
          recordCount++;
        }
        firstLine = false;
      }
      f.close();
      Serial.print("检测到已有日志，共 ");
      Serial.print((unsigned long)recordCount);
      Serial.println(" 条记录");
    }
  }

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

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Wi-Fi 已连接，IP 地址：");
    Serial.println(WiFi.localIP());
    Serial.print("请用浏览器访问：http://");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("Wi-Fi 连接失败，Web 功能不可用，但串口命令仍可工作");
  }

  // 注册 Web 路由
  server.on("/", handleRoot);
  server.on("/download", handleDownload);
  server.on("/clear", handleClear);
  server.onNotFound(handleNotFound);

  // 启动 Web 服务器
  server.begin();
  Serial.println("Web 服务器已启动");

  // 启动硬件定时器
  initLogTimer();

  // 立即记录第一条数据，方便验证硬件是否正常
  readAndLog();

  Serial.println("输入 download/d 可在串口查看日志，clear/c 清空日志");
}

void loop() {
  // 处理 Web 客户端请求。非阻塞，没有请求时立即返回。
  server.handleClient();

  // 检查定时器标志。如果到时间了，就读取传感器并记录。
  // 先关闭中断再读取 logFlag，防止读取过程中被中断修改。
  bool shouldLog = false;
  noInterrupts();
  if (logFlag) {
    shouldLog = true;
    logFlag = false;
  }
  interrupts();

  if (shouldLog) {
    readAndLog();
  }

  // 处理串口命令
  handleSerialCommands();

  // 短暂延时，避免主循环空转占用过多 CPU
  delay(10);
}

/**
 * 扩展练习：
 * 1. 添加实时时钟模块（DS1302/DS3231），把绝对时间写入 CSV，而不是 millis() 相对时间。
 * 2. 增加光敏电阻或土壤湿度传感器，扩展 CSV 字段，例如 elapsed_ms,temp,hum,light,soil。
 * 3. 用定时器中断触发记录后，把最近 24 条记录缓存到 RTC 内存，再用批量 HTTP POST 上传到服务器。
 * 4. 在 Web 页面增加 /chart 路径，用 JavaScript 把 CSV 数据绘制成折线图。
 */
