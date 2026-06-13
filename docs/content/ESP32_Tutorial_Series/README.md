# ESP32 Arduino 教程系列

本系列教程面向完全没有 ESP32 或 Arduino 经验的初学者。每个示例只聚焦一个核心知识点，代码中包含大量中文注释，解释"为什么这样写"以及"每个专业名词是什么意思"。建议按顺序从第 01 课开始阅读。

> **ESP32** 是乐鑫（Espressif）推出的一款低成本、低功耗的 32 位微控制器，内置 Wi-Fi 和蓝牙，非常适合物联网（IoT）入门学习。

---

## 目录

- [ESP32 Arduino 教程系列](#esp32-arduino-教程系列)
  - [目录](#目录)
  - [基础教程（01 ~ 15）](#基础教程01--15)
    - [第一阶段：GPIO 与基础外设](#第一阶段gpio-与基础外设)
    - [第二阶段：网络与无线通信](#第二阶段网络与无线通信)
    - [第三阶段：中断与定时器](#第三阶段中断与定时器)
    - [第四阶段：进阶系统功能](#第四阶段进阶系统功能)
  - [无线与网络扩展](#无线与网络扩展)
  - [外设项目](#外设项目)
  - [快速开始](#快速开始)
  - [推荐的 ESP32 开发板设置](#推荐的-esp32-开发板设置)
  - [常用引脚说明](#常用引脚说明)
  - [外设项目所需库一览](#外设项目所需库一览)
  - [常见问题与排查](#常见问题与排查)
  - [下一步可以做什么](#下一步可以做什么)

---

## 基础教程（01 ~ 15）

这 15 个示例是整套教程的根基，建议按编号顺序学习。每课只讲一个核心概念，并把涉及的每个专业术语解释清楚。

### 第一阶段：GPIO 与基础外设

| 编号 | 项目 | 核心知识点 | 一句话说明 |
|---|---|---|---|
| 01 | [Digital Output Blink](./01_Digital_Output_Blink) | `pinMode()`、`digitalWrite()`、`delay()` | 学会让 ESP32 控制引脚输出高/低电平，让 LED 闪烁。 |
| 02 | [Digital Input Button](./02_Digital_Input_Button) | `digitalRead()`、`INPUT_PULLUP`、按键消抖 | 读取按键状态，用内部上拉电阻简化接线，处理机械抖动。 |
| 03 | [PWM LED Fade](./03_PWM_LED_Fade) | `ledcAttach()`、`ledcWrite()`、PWM 占空比 | 用 PWM（脉宽调制）让 LED 亮度平滑变化，做出呼吸灯效果。 |
| 04 | [ADC Read Analog](./04_ADC_Read_Analog) | `analogRead()`、`map()`、电位器调光 | 把模拟电压转换成数字值，用电位器实现 LED 无级调光。 |
| 05 | [Serial Communication](./05_Serial_Communication) | `Serial.print()`、`Serial.readString()`、命令解析 | 通过串口和电脑"对话"，学习调试和命令交互。 |

### 第二阶段：网络与无线通信

| 编号 | 项目 | 核心知识点 | 一句话说明 |
|---|---|---|---|
| 06 | [WiFi Connect](./06_WiFi_Connect) | `WiFi.begin()`、`WiFi.status()`、IP 地址 | 让 ESP32 连接家里的 2.4GHz Wi-Fi，获取并打印网络信息。 |
| 07 | [Web Server LED](./07_Web_Server_LED) | `WebServer.h`、HTTP 请求/响应、网页控制 | 搭建简易 Web 服务器，用手机浏览器远程控制 LED。 |
| 08 | [Bluetooth Serial](./08_Bluetooth_Serial) | `BluetoothSerial.h`、蓝牙 SPP、手机 APP 控制 | 使用经典蓝牙串口协议（SPP），通过手机 APP 无线控制 ESP32。 |

> **SSID**：Wi-Fi 热点的名称；**IP 地址**：设备在网络中的"门牌号"；**MAC 地址**：网卡的全球唯一硬件地址；**HTTP**：浏览器和服务器之间最常用的通信协议；**SPP（Serial Port Profile）**：把蓝牙模拟成无线串口的协议。

### 第三阶段：中断与定时器

| 编号 | 项目 | 核心知识点 | 一句话说明 |
|---|---|---|---|
| 09 | [External Interrupt](./09_External_Interrupt) | `attachInterrupt()`、`volatile`、ISR | 用外部中断让按键按下时立即响应，不被主循环中的 `delay()` 耽误。 |
| 10 | [Timer Interrupt](./10_Timer_Interrupt) | 硬件定时器、`timerBegin()`、`timerAlarm()` | 用硬件定时器周期性地触发任务，实现精确、不阻塞主循环的定时控制。 |

> **中断（Interrupt）**：当外部事件发生时，CPU 暂停当前任务去处理紧急事务的机制；**ISR（Interrupt Service Routine）**：中断服务函数；**volatile**：告诉编译器变量可能在中断中被修改，不要优化掉读取操作。

### 第四阶段：进阶系统功能

| 编号 | 项目 | 核心知识点 | 一句话说明 |
|---|---|---|---|
| 11 | [Touch Sensor](./11_Touch_Sensor) | `touchRead()`、电容触摸 | 使用 ESP32 内置的电容触摸引脚，无需机械按键也能控制 LED。 |
| 12 | [FreeRTOS Tasks](./12_FreeRTOS_Tasks) | 多任务、`xTaskCreatePinnedToCore()`、双核 | 在 ESP32 的两个 CPU 核心上同时运行多个任务，互不阻塞。 |
| 13 | [Preferences Storage](./13_Preferences_Storage) | `Preferences.h`、NVS、断电保存 | 把配置写入 Flash，重启后仍然保留。 |
| 14 | [HTTP Client](./14_HTTP_Client) | `HTTPClient.h`、GET 请求、访问 API | 让 ESP32 作为客户端访问网络 API，获取远程数据。 |
| 15 | [Deep Sleep](./15_Deep_Sleep) | 低功耗模式、定时唤醒、GPIO 唤醒、RTC 内存 | 让 ESP32 进入深度睡眠省电，并通过定时器或按键唤醒。 |

> **FreeRTOS**：ESP32 内置的实时操作系统，负责调度多个任务；**NVS（Non-Volatile Storage）**：非易失性存储，断电后数据不丢失；**API（Application Programming Interface）**：程序之间交互数据的接口；**JSON**：一种轻量级数据格式，常用于网络 API；**RTC 内存**：深度睡眠时仍然保留数据的一块特殊内存。

---

## 无线与网络扩展

> 本组项目位于 [`extensions/`](./extensions/) 目录下，是对第二阶段网络内容的进阶扩展。每个项目都会更深入地讲解无线网络相关概念，例如 AP 模式、STA 模式、DNS、mDNS、WebSocket 等。

| 项目 | 核心知识点 | 一句话说明 |
|---|---|---|
| WiFi_AP_Mode | `WIFI_AP`、热点、DHCP、网关 | 让 ESP32 自己创建一个 Wi-Fi 热点，其他设备可以直接连上来。 |
| WiFi_Manager_Config |  Captive Portal、Web 配置页、Flash 保存凭据 | 通过网页 portal 配置 Wi-Fi 名称和密码，无需重新烧录程序。 |
| mDNS_Web_Server | mDNS、`ESPmDNS.h`、域名解析 | 通过 `http://esp32.local` 这样的域名访问 ESP32，无需记忆 IP。 |
| WebSocket_LED | WebSocket、双向实时通信 | 使用 WebSocket 实现浏览器和 ESP32 之间的实时双向控制。 |

> **AP（Access Point）**：无线接入点，也就是"热点"；**STA（Station）**：连接热点的客户端；**DHCP**：自动分配 IP 地址的协议；**DNS**：把域名（如 `esp32.local`）解析成 IP 地址的系统；**mDNS（Multicast DNS）**：局域网内的零配置域名解析；**WebSocket**：在单个 TCP 连接上进行全双工通信的协议，适合实时控制。

---

## 外设项目

> 本组项目位于 [`peripherals/`](./peripherals/) 目录下，讲解常见传感器和执行器的接线与驱动。每个项目都会说明传感器工作原理、需要用到的第三方库、详细接线表，以及常见故障排查方法。

| 项目 | 外设 | 核心知识点 | 一句话说明 |
|---|---|---|---|
| DHT11_DHT22_Sensor | DHT11 / DHT22 温湿度传感器 | 单总线协议、温湿度读取、数据校验 | 读取环境温度和湿度，串口或屏幕显示。 |
| HC_SR04_Ultrasonic_Distance | HC-SR04 超声波测距模块 | 超声波、回声测距、GPIO 时序 | 测量前方障碍物距离。 |
| SG90_Servo_Motor | SG90 舵机 | PWM 控制舵机角度、占空比与角度关系 | 控制舵机旋转到指定角度。 |
| Relay_Module_Control | 继电器模块 | 高低电平触发、强电隔离 | 用 ESP32 控制继电器开关，进而控制大功率电器。 |
| I2C_OLED_SSD1306_Display | SSD1306 OLED 显示屏 | I2C 协议、SSD1306 驱动、屏幕寻址 | 在 0.96 寸 OLED 屏幕上显示文字和图形。 |
| WS2812B_NeoPixel_LED | WS2812B / NeoPixel 灯带 | 单线归零码、可寻址 RGB LED | 控制彩色灯带实现流水灯、彩虹等效果。 |
| PIR_Motion_Sensor | HC-SR501 人体红外传感器 | 热释电、数字输入、感应延时 | 检测是否有人经过，触发报警或开灯。 |
| LDR_Light_Sensor | 光敏电阻（LDR） | 分压电路、ADC 读取、光照强度映射 | 根据环境光线强度自动控制 LED 或记录数据。 |
| Active_Buzzer | 有源蜂鸣器 | 数字输出、频率发声 | 用高低电平控制蜂鸣器发出声音。 |
| Soil_Moisture_Sensor | 土壤湿度传感器 | ADC 读取、土壤电导率、水分映射 | 检测土壤湿度，用于自动浇花等场景。 |

> **I2C**：一种两线串行通信协议，只需要 SDA（数据线）和 SCL（时钟线）；**单总线（One-Wire）**：DHT 传感器使用的单线通信方式；**PWM 舵机**：舵机角度由固定 50Hz PWM 的高电平脉宽决定。

---

## 快速开始

1. 安装 **Arduino IDE**（2.x 或 1.8.x 均可），并在"开发板管理器"中添加 ESP32 支持包。
2. 用 Arduino IDE 打开任意教程目录下的 `.ino` 文件（文件名与目录名相同）。
3. 根据代码顶部注释连接硬件（电阻、按键、传感器等）。
4. 把代码中的 `YOUR_WIFI_SSID` 和 `YOUR_WIFI_PASSWORD` 改成自己的 Wi-Fi 信息（第 06、07、14 课以及所有 `extensions/` 项目需要）。
5. 选择开发板 **ESP32 Dev Module**，选择正确的 COM 口，点击上传。
6. 打开 Arduino IDE 的"串口监视器"，波特率设置为 **115200**，观察输出。

> 如果上传时提示 "Failed to connect to ESP32"，通常是开发板没进入下载模式。按住开发板上的 **BOOT** 键，点击上传，看到 "Connecting..." 后松开即可。

---

## 推荐的 ESP32 开发板设置

| 参数 | 推荐值 | 说明 |
|---|---|---|
| 开发板 | ESP32 Dev Module | 最常见的通用选项 |
| Upload Speed | 921600 | 加快上传速度 |
| CPU Frequency | 240MHz | ESP32 最高主频 |
| Flash Frequency | 80MHz | 与大多数开发板兼容 |
| Flash Mode | QIO | 四线 I/O 模式 |
| Flash Size | 4MB | 大多数开发板的默认闪存容量 |

---

## 常用引脚说明

| 功能 | 推荐引脚 | 说明 |
|---|---|---|
| 数字输出/输入 | GPIO0 ~ GPIO33 | 通用 GPIO，注意避开 GPIO6 ~ GPIO11 |
| ADC 模拟输入 | GPIO32 ~ GPIO39 | 最稳定，其中 GPIO34 ~ GPIO39 只能输入 |
| 电容触摸 | GPIO0、2、4、12、13、14、15、27、32、33 | ESP32 经典款支持 |
| DAC 模拟输出 | GPIO25、GPIO26 | 真正的模拟电压输出 |
| SPI | GPIO18（SCK）、19（MISO）、23（MOSI）、5（SS） | 硬件 SPI 默认引脚 |
| I2C | GPIO21（SDA）、GPIO22（SCL） | 软件 I2C 可改其他引脚 |

> ⚠️ **GPIO6 ~ GPIO11 不要接任何外部设备**，它们内部连接 Flash 芯片，接错会导致程序崩溃或无法启动。

---

## 外设项目所需库一览

> 以下库需要在 Arduino IDE 的"库管理器"（`项目 > 加载库 > 管理库`）中搜索并安装。安装前请确认库名称和作者，避免安装到同名但功能不同的库。

| 项目 | 需要安装的库 | 推荐作者/关键词 | 安装方式 |
|---|---|---|---|
| DHT11_DHT22_Sensor | DHT sensor library | Adafruit | 库管理器搜索 "DHT sensor library" |
| DHT11_DHT22_Sensor | Adafruit Unified Sensor | Adafruit | 与 DHT 库一起被依赖 |
| HC_SR04_Ultrasonic_Distance | NewPing | Tim Eckel | 库管理器搜索 "NewPing"（也可不用库，直接 GPIO 驱动） |
| SG90_Servo_Motor | ESP32Servo | Kevin Harrington / John K. Bennett | 库管理器搜索 "ESP32Servo" |
| Relay_Module_Control | 无需额外库 | — | 使用 `digitalWrite()` 控制 |
| I2C_OLED_SSD1306_Display | Adafruit SSD1306 | Adafruit | 库管理器搜索 "Adafruit SSD1306" |
| I2C_OLED_SSD1306_Display | Adafruit GFX Library | Adafruit | SSD1306 库的图形依赖 |
| WS2812B_NeoPixel_LED | Adafruit NeoPixel | Adafruit | 库管理器搜索 "Adafruit NeoPixel" |
| WS2812B_NeoPixel_LED | FastLED | FastLED | 可选，功能更强大 |
| PIR_Motion_Sensor | 无需额外库 | — | 使用 `digitalRead()` 读取 |
| LDR_Light_Sensor | 无需额外库 | — | 使用 `analogRead()` 读取 |
| Active_Buzzer | 无需额外库 | — | 使用 `digitalWrite()` 或 PWM 控制 |
| Soil_Moisture_Sensor | 无需额外库 | — | 使用 `analogRead()` 读取 |

> 安装库时，Arduino IDE 会自动处理大多数依赖。如果编译时提示缺少头文件，请回到库管理器搜索并安装相应库。

---

## 常见问题与排查

1. **上传失败："Failed to connect to ESP32"**
   - 按住 BOOT 键，点上传，看到 "Connecting..." 后松开。
   - 检查 USB 线是否支持数据传输（有些线只充电）。
   - 检查 COM 口是否选择正确。

2. **串口输出乱码**
   - 检查串口监视器波特率是否为 **115200**。
   - 检查开发板设置中的 CPU Frequency 是否与代码匹配。

3. **ADC 读数不稳定**
   - 在 ADC 引脚和 GND 之间加一个 **100nF 电容**。
   - 确保电位器/传感器供电稳定，GND 接线可靠。

4. **Wi-Fi 连不上**
   - 确认路由器是 **2.4GHz**，ESP32 不支持 5GHz。
   - 检查 SSID 和密码是否输入正确（区分大小写和空格）。
   - 确认路由器没有开启 MAC 地址过滤。

5. **蓝牙搜索不到 ESP32**
   - 经典蓝牙（BluetoothSerial）在 iOS 上受限，建议使用 BLE 项目。
   - 确认手机蓝牙已开启，并在 APP 中主动搜索设备。

6. **OLED 屏幕不亮**
   - 确认 I2C 地址是 `0x3C` 还是 `0x3D`（可用 I2C 扫描程序检测）。
   - 检查 SDA/SCL 接线，必要时加上拉电阻（部分模块已内置）。

---

## 下一步可以做什么

学完基础 15 课后，建议按兴趣选择以下方向继续深入：

- **无线与物联网**
  - 把 [07_Web_Server_LED](./07_Web_Server_LED) 和 [04_ADC_Read_Analog](./04_ADC_Read_Analog) 结合，在网页上实时显示电压曲线。
  - 用 [14_HTTP_Client](./14_HTTP_Client) 访问天气、时间等真实 API，并配合 [13_Preferences_Storage](./13_Preferences_Storage) 保存配置。
  - 学习 MQTT 协议，把 ESP32 接入 Home Assistant 或阿里云 IoT。

- **多任务与系统**
  - 用 [12_FreeRTOS_Tasks](./12_FreeRTOS_Tasks) 设计一个任务：一个负责读取传感器，一个负责发送网络请求，一个负责 LED 状态指示。
  - 用 [15_Deep_Sleep](./15_Deep_Sleep) 实现电池供电的低功耗项目。

- **外设与显示**
  - 接 OLED 屏幕，把温湿度、距离、光线等数据可视化。
  - 用 WS2812B 灯带制作音乐频谱灯或氛围灯。
  - 用继电器控制真实家电，配合传感器实现自动化。

- **综合项目**
  - 做一个无需重新烧录即可配置 Wi-Fi 的智能小夜灯。
  - 做一个带网页配置、实时数据上报、远程控制的完整物联网节点。

祝你学习愉快，做出自己满意的作品！
