/*
 * 综合项目 P06：Wi-Fi 状态显示器
 *
 * 学习目标：
 * 1. 理解 ESP32 作为 STA（站点）连接 Wi-Fi 的完整流程
 * 2. 学会获取并显示 SSID、IP 地址、RSSI 等网络信息
 * 3. 学会使用 SSD1306 OLED 显示多行中文/英文信息
 * 4. 掌握简单的断线自动重连机制
 * 5. 学会用 millis() 定期刷新屏幕，避免闪烁和阻塞
 *
 * 前置知识：
 * - 已完成教程 06《Wi-Fi 连接》，理解 WiFi.begin()、WiFi.status() 等基本用法
 * - 已完成 P05 或相关 OLED 教程，了解 I2C 和 Adafruit SSD1306 库
 *
 * 硬件连接：
 * - ESP32 GPIO21 → OLED SDA（I2C 数据线）
 * - ESP32 GPIO22 → OLED SCL（I2C 时钟线）
 * - ESP32 3.3V   → OLED VCC
 * - ESP32 GND    → OLED GND
 *
 * 关键概念解释：
 * - SSID（Service Set Identifier）：Wi-Fi 的名称，比如“TP-LINK_1234”。
 * - IP 地址（Internet Protocol Address）：设备在局域网中的“门牌号”，
 *   由路由器通过 DHCP 分配。只有拿到 IP，ESP32 才能和局域网内其他设备通信。
 * - RSSI（Received Signal Strength Indicator，接收信号强度指示）：
 *   单位是 dBm，数值越接近 0 信号越好。例如 -30 dBm 很强，-80 dBm 较弱。
 * - STA 模式（Station）：ESP32 像手机、电脑一样，作为客户端去连接路由器。
 * - 断线重连：当路由器信号不稳定或重启时，ESP32 能自动检测到断开，
 *   并重新尝试连接，提高项目的可靠性。
 */

// Wire.h 提供 I2C 通信能力。
#include <Wire.h>

// Adafruit SSD1306 和 GFX 库用于驱动 OLED。
// 需要在 Arduino IDE 库管理器中安装：
// 1. "Adafruit SSD1306"
// 2. "Adafruit GFX Library"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// WiFi.h 是 ESP32 的核心网络库，封装了连接 Wi-Fi、获取 IP、RSSI 等功能。
#include <WiFi.h>

// ===== 修改这里：填入你的 Wi-Fi 信息 =====
// const char* 表示“指向常量字符的指针”，用来保存不会修改的字符串。
// 注意：ESP32 只支持 2.4GHz Wi-Fi，不能连接 5GHz。
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

// OLED 的 I2C 引脚。ESP32 默认 I2C 引脚通常是 GPIO21（SDA）和 GPIO22（SCL）。
#define SDA_PIN 21
#define SCL_PIN 22

// OLED 分辨率。
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// OLED 复位引脚。很多 I2C 模块没有复位引脚，设为 -1 表示不用。
#define OLED_RESET -1

// OLED 的 I2C 地址。大多数 SSD1306 是 0x3C，如果不亮可尝试 0x3D。
#define SCREEN_ADDRESS 0x3C

// 屏幕刷新间隔。2 秒刷新一次，既能及时反映信号变化，又不会让屏幕闪烁太频繁。
#define UPDATE_INTERVAL 2000

// Wi-Fi 连接超时。最多等待 30 次 × 500ms = 15 秒。
#define WIFI_MAX_RETRY 30

// 创建 SSD1306 显示对象。
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// 记录上一次刷新屏幕的时间。
unsigned long lastUpdateTime = 0;

// 在 OLED 上显示当前 Wi-Fi 状态的函数。
void displayWiFiStatus() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // 标题
  display.setCursor(0, 0);
  display.println(F("Wi-Fi 状态"));
  display.println(F("================"));

  // 如果还没连上 Wi-Fi，显示提示并返回。
  if (WiFi.status() != WL_CONNECTED) {
    display.setCursor(0, 24);
    display.println(F("正在连接..."));
    display.print(F("SSID: "));
    display.println(WIFI_SSID);
    display.display();
    return;
  }

  // 第一行：SSID
  display.setCursor(0, 12);
  display.print(F("SSID:"));
  display.println(WiFi.SSID());

  // 第二行：IP 地址
  display.print(F("IP: "));
  display.println(WiFi.localIP());

  // 第三行：RSSI 信号强度
  display.print(F("RSSI: "));
  display.print(WiFi.RSSI());
  display.println(F(" dBm"));

  // 第四行：连接状态
  display.print(F("状态: 已连接"));

  // 把内存中的画面刷新到 OLED。
  display.display();
}

void setup() {
  // 初始化串口，波特率 115200。
  Serial.begin(115200);
  delay(1000);

  Serial.println("==============================");
  Serial.println("P06：Wi-Fi 状态显示器");
  Serial.println("==============================");

  // 初始化 I2C，指定 SDA/SCL 引脚。
  Wire.begin(SDA_PIN, SCL_PIN);

  // 初始化 OLED。
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("OLED 初始化失败，请检查接线和 I2C 地址"));
    while (true) {
      delay(100);
    }
  }

  // 开机先显示一条提示，让用户知道系统正在启动。
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 24);
  display.println(F("  Wi-Fi 状态显示器\n     启动中..."));
  display.display();

  // 设置 Wi-Fi 为 STA 模式：ESP32 作为客户端去连接路由器。
  WiFi.mode(WIFI_STA);

  // 开始连接 Wi-Fi。
  Serial.print(F("正在连接 Wi-Fi: "));
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  // 等待连接成功，最多等待 WIFI_MAX_RETRY 次。
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < WIFI_MAX_RETRY) {
    delay(500);
    Serial.print(".");
    retry++;
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(F("Wi-Fi 连接成功！"));
    Serial.print(F("IP 地址: "));
    Serial.println(WiFi.localIP());
    Serial.print(F("RSSI: "));
    Serial.print(WiFi.RSSI());
    Serial.println(F(" dBm"));
  } else {
    Serial.println(F("Wi-Fi 连接失败，请在代码中检查 SSID 和密码"));
    Serial.println(F("程序会继续尝试在 loop() 中自动重连"));
  }

  // 先把当前状态显示到 OLED 上。
  displayWiFiStatus();
  lastUpdateTime = millis();
}

void loop() {
  unsigned long now = millis();

  // 每隔 UPDATE_INTERVAL 刷新一次屏幕。
  if (now - lastUpdateTime >= UPDATE_INTERVAL) {
    lastUpdateTime = now;

    // 如果检测到断开连接，就尝试自动重连。
    // reconnect() 会保留之前 begin() 时的 SSID 和密码，重新发起连接。
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println(F("Wi-Fi 已断开，正在自动重连..."));
      WiFi.reconnect();
    } else {
      // 连接正常时，把 SSID、IP、RSSI 打印到串口。
      Serial.print(F("SSID: "));
      Serial.print(WiFi.SSID());
      Serial.print(F("  IP: "));
      Serial.print(WiFi.localIP());
      Serial.print(F("  RSSI: "));
      Serial.print(WiFi.RSSI());
      Serial.println(F(" dBm"));
    }

    // 无论是否连接成功，都刷新 OLED 显示。
    displayWiFiStatus();
  }

  // 这里可以加入其他任务，比如读取传感器、处理 Web 请求等，
  // 因为上面的逻辑是非阻塞的。
}

/*
 * 扩展挑战：
 * 1. 在屏幕上显示 MAC 地址和子网掩码，做一个更完整的网络信息面板。
 * 2. 根据 RSSI 强弱在 OLED 上画一个信号强度图标，像手机顶部那样。
 * 3. 用 mDNS 把设备注册为 http://esp32.local，让手机不用记 IP。
 * 4. 把 Wi-Fi 信息保存到 Preferences（闪存）中，上电自动连接上次成功的网络。
 * 5. 连接失败后自动创建一个 AP 热点，让用户通过手机配网（WiFiManager 思路）。
 */
