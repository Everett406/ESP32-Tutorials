# P06 Wi-Fi 状态显示器（WiFi Status Display）

## 1. 项目简介

本项目让 ESP32 连接家里的 Wi-Fi，然后把 **Wi-Fi 名称（SSID）、IP 地址、信号强度（RSSI）** 实时显示在 **OLED 屏幕** 上。当网络断开时，程序会自动尝试重连，适合作为后续所有网络项目的“状态面板”。

通过这个项目，你可以学会：
- ESP32 连接 Wi-Fi 的完整流程；
- 如何获取 SSID、IP、RSSI 等网络参数；
- 如何在 OLED 上显示多行信息；
- 如何实现简单的断线自动重连。

## 2. 涉及知识点

- Wi-Fi STA 模式
- `WiFi.begin()` / `WiFi.status()` / `WiFi.reconnect()`
- `WiFi.SSID()` / `WiFi.localIP()` / `WiFi.RSSI()`
- I2C 与 SSD1306 OLED 显示
- `millis()` 非阻塞刷新

## 3. 硬件清单

| 元器件 | 数量 | 备注 |
|--------|------|------|
| ESP32 开发板 | 1 | 支持 2.4GHz Wi-Fi |
| SSD1306 OLED（128×64，I2C） | 1 | 常见地址 0x3C |
| 杜邦线 | 若干 | |

## 4. 电路连接

### OLED（I2C）接线

| ESP32 | OLED | 说明 |
|-------|------|------|
| 3.3V | VCC | OLED 供电 |
| GND | GND | 共地 |
| GPIO21 | SDA | I2C 数据线 |
| GPIO22 | SCL | I2C 时钟线 |

**本示例不需要 LED、按钮等其他外设**，只需要 ESP32 和 OLED 即可。

## 5. 代码思路

1. **引入库并配置 Wi-Fi**：引入 `WiFi.h`、`Wire.h`、`Adafruit_SSD1306.h`，填写 `WIFI_SSID` 和 `WIFI_PASSWORD`。
2. **初始化 OLED**：在 `setup()` 中启动 I2C 并初始化 SSD1306。
3. **连接 Wi-Fi**：设置 STA 模式，调用 `WiFi.begin()`，用循环等待连接，最多等待 15 秒。
4. **显示状态**：把 SSID、IP、RSSI 显示到 OLED；同时打印到串口。
5. **自动重连**：在 `loop()` 中每 2 秒检查一次连接状态；如果断开，调用 `WiFi.reconnect()` 自动重连。

## 6. 关键代码解析

### 获取 Wi-Fi 信息

```cpp
String ssid = WiFi.SSID();      // Wi-Fi 名称
IPAddress ip = WiFi.localIP();  // 本地 IP 地址
int rssi = WiFi.RSSI();         // 信号强度，单位 dBm
```

- `WiFi.SSID()` 返回当前连接的 Wi-Fi 名称。
- `WiFi.localIP()` 返回路由器分配给 ESP32 的 IP 地址。
- `WiFi.RSSI()` 返回接收信号强度，数值越接近 0 越好。

### 断线重连

```cpp
if (WiFi.status() != WL_CONNECTED) {
  Serial.println(F("Wi-Fi 已断开，正在自动重连..."));
  WiFi.reconnect();
}
```

`WiFi.reconnect()` 会使用之前 `WiFi.begin()` 保存的 SSID 和密码重新发起连接。这比再次调用 `WiFi.begin(...)` 更简洁。

### 屏幕刷新

```cpp
if (millis() - lastUpdateTime >= UPDATE_INTERVAL) {
  lastUpdateTime = millis();
  displayWiFiStatus();
}
```

用 `millis()` 控制刷新频率，避免 OLED 每秒都被重写导致闪烁，也避免程序被 `delay()` 阻塞。

## 7. 运行效果

上传代码前，先把代码里的 `YOUR_WIFI_SSID` 和 `YOUR_WIFI_PASSWORD` 改成你自己的 Wi-Fi 名称和密码。注意 ESP32 只能连接 **2.4GHz** 网络。

上传后，OLED 会先显示“Wi-Fi 状态显示器 启动中...”，然后显示：

```text
Wi-Fi 状态
================
SSID: your_wifi
IP: 192.168.1.105
RSSI: -52 dBm
状态: 已连接
```

串口监视器（波特率 115200）会输出：

```text
==============================
P06：Wi-Fi 状态显示器
==============================
正在连接 Wi-Fi: your_wifi
......
Wi-Fi 连接成功！
IP 地址: 192.168.1.105
RSSI: -52 dBm
```

如果此时关闭路由器或让 ESP32 走出信号范围，OLED 会显示“正在连接...”，串口会打印“Wi-Fi 已断开，正在自动重连...”。信号恢复后，ESP32 会自动重新连上网络。

## 8. 常见问题

### 无法连接 Wi-Fi

1. **检查 SSID 和密码**：注意大小写和空格，中文 SSID 可能不兼容。
2. **确认是 2.4GHz**：ESP32 不支持 5GHz。很多双频路由器会把 2.4G 和 5G 分成两个名称，请确认连接的是 2.4G 那个。
3. **路由器 MAC 过滤**：某些路由器只允许白名单设备上网。可以在串口打印 ESP32 的 MAC 地址（`WiFi.macAddress()`），加入路由器白名单。
4. **信号太弱**：RSSI 低于 -75 dBm 时可能连接困难，尝试把 ESP32 靠近路由器。

### OLED 不亮

1. **检查 I2C 地址**：把 `SCREEN_ADDRESS` 改成 `0x3D` 再试。
2. **检查接线**：确认 SDA 接 GPIO21、SCL 接 GPIO22，VCC 接 3.3V。
3. **检查库是否安装**：Arduino IDE 库管理器中安装 `Adafruit SSD1306` 和 `Adafruit GFX Library`。

### 屏幕内容闪烁

- 如果刷新太频繁，可能看起来闪烁。本代码已设为 2 秒刷新一次。如果还闪，可能是电源不稳定，尝试给 OLED 单独加电容滤波或使用更好的 USB 线。

### IP 地址每次重启都不一样

- 这是 DHCP 的正常行为。如果希望固定 IP，可以在 `WiFi.config()` 中设置静态 IP。

## 9. 扩展挑战

1. **信号强度图标**：根据 RSSI 数值在 OLED 上画 1~4 格信号条。
2. **显示 MAC 地址和网关**：把 `WiFi.macAddress()`、`WiFi.gatewayIP()` 也显示出来，做成完整网络面板。
3. **断线记录**：统计断线次数和累计断线时长，显示在 OLED 上，用于评估网络稳定性。
4. **mDNS 访问**：引入 `ESPmDNS.h`，让 ESP32 可以通过 `http://esp32.local` 访问。
5. **多 Wi-Fi 记忆**：在 `setup()` 里尝试连接多个预设的 Wi-Fi，哪个能连就连哪个。
