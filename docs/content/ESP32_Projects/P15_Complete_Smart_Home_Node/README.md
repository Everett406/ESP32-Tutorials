# 综合项目 P15：完整智能家居节点（Complete Smart Home Node）

## 1. 项目简介

本项目是 ESP32 综合教程的“集大成者”。它把一个真正的智能家居节点需要的功能整合到一个程序中：

- **多传感器采集**：DHT22 读取温湿度，光敏电阻读取环境光照。
- **本地显示**：OLED 屏幕实时显示所有状态和系统运行情况。
- **Web 控制**：手机或电脑通过浏览器查看数据、开关 LED、控制继电器、调节 LED 亮度。
- **MQTT 上报与远程控制**：把传感器数据发布到 MQTT 服务器，并订阅命令主题实现远程开关控制。
- **OTA 无线升级**：无需 USB 线，通过 Wi-Fi 直接更新程序。
- **WiFiManager 配网**：第一次使用时自动生成热点，手机配置 Wi-Fi 信息。
- **FreeRTOS 多任务**：传感器采集、屏幕刷新、网络通信分别运行在不同任务中，互不阻塞。

学完本项目，你已经具备设计一个可落地的小型物联网终端的完整能力。

## 2. 涉及知识点

| 知识点 | 对应前置教程 | 本项目中的作用 |
|--------|--------------|----------------|
| DHT22 温湿度读取 | `peripherals/DHT11_DHT22_Sensor` | 采集环境温湿度 |
| 光敏电阻 / ADC | `peripherals/LDR_Light_Sensor`、教程 04 | 采集环境光照强度 |
| OLED 显示 | `peripherals/I2C_OLED_SSD1306_Display` | 本地状态显示 |
| PWM LED | `03_PWM_LED_Fade` | LED 亮度调节 |
| Web 服务器 | `07_Web_Server_LED` | 网页控制面板 |
| Wi-Fi 与 WiFiManager | `06_WiFi_Connect`、`extensions/WiFi_Manager_Config` | 网络连接与配网 |
| MQTT | MQTT 基础教程 / 扩展阅读 | 数据上报与远程命令 |
| OTA | ArduinoOTA 扩展阅读 | 无线固件升级 |
| Preferences | `13_Preferences_Storage` | 保存 MQTT 服务器地址 |
| FreeRTOS 多任务 | `12_FreeRTOS_Tasks` | 任务并行执行 |

## 3. 硬件清单

| 器件 | 数量 | 说明 |
|------|------|------|
| ESP32 开发板 | 1 块 | 主控芯片 |
| DHT22 温湿度传感器 | 1 个 | 采集温湿度 |
| SSD1306 OLED（128×64，I2C） | 1 块 | 本地显示 |
| 光敏电阻 | 1 个 | 测量环境光照 |
| 10kΩ 电阻 | 1 个 | 与光敏电阻组成分压电路 |
| LED | 1 个 | PWM 调光输出 |
| 330Ω 电阻 | 1 个 | LED 限流 |
| 继电器模块 | 1 个 | 控制高功率设备（如水泵、风扇、灯） |
| 杜邦线 | 若干 | 连接电路 |
| 面包板 | 1 块 | 搭建电路 |

## 4. 电路连接

| ESP32 引脚 | 连接器件 | 备注 |
|------------|----------|------|
| 3.3V | DHT22 VCC | DHT22 可直接用 3.3V 供电 |
| GND | DHT22 GND | 共地 |
| GPIO4 | DHT22 DATA | 读取温湿度，建议加 4.7kΩ ~ 10kΩ 上拉 |
| 3.3V | 光敏电阻一端 | 组成分压电路 |
| GPIO34 | 光敏电阻另一端 + 10kΩ 电阻 → GND | ADC1 引脚，适合模拟输入 |
| 3.3V | OLED VCC | OLED 工作电压 |
| GND | OLED GND | 共地 |
| GPIO21 | OLED SDA | 默认 I2C 数据线 |
| GPIO22 | OLED SCL | 默认 I2C 时钟线 |
| GPIO5 | 继电器 IN | 控制继电器 |
| GND | 继电器 GND | 共地 |
| GPIO2 | LED 正极（长脚） | 板载 LED 通常已接好 |
| GND | LED 负极（短脚）→ 330Ω 电阻 → GND | 限流 |

> **继电器供电说明**：小型继电器模块通常可以用 3.3V 或 5V 供电。如果继电器无法吸合，请改用 5V 供电，但 IN 引脚仍需接到 ESP32 的 3.3V GPIO 上。注意：继电器控制高电压设备时务必注意安全。

## 5. 代码思路

整个程序由 **setup()** 完成初始化，然后创建三个 FreeRTOS 任务并行运行，**loop()** 只负责处理串口命令。

### 5.1 初始化流程（setup）

1. 创建互斥锁 `dataMutex`，保护共享的 `SensorData` 结构体。
2. 从 Preferences 读取 MQTT 服务器地址。
3. 初始化硬件：LED PWM、继电器、DHT22、OLED。
4. 使用 WiFiManager 连接或配置 Wi-Fi。
5. 如果 Wi-Fi 连接成功，配置 MQTT、启动 Web 服务器、启用 OTA。
6. 创建三个 FreeRTOS 任务。

### 5.2 任务分工

| 任务名 | 运行核心 | 职责 |
|--------|----------|------|
| SensorRead | Core 0 | 每 5 秒读取 DHT22 和光敏电阻，更新共享数据 |
| DisplayUpdate | Core 1 | 每 1 秒刷新 OLED 显示 |
| NetworkLoop | Core 1 | 处理 Web 请求、MQTT 连接与发布、OTA |

### 5.3 数据共享

三个任务都需要访问传感器数值和设备状态。为了避免冲突，使用 FreeRTOS 互斥锁：

```cpp
xSemaphoreTake(dataMutex, portMAX_DELAY);
sensorData.temperature = t;
xSemaphoreGive(dataMutex);
```

在读取时也使用 `getSensorDataCopy()` 先复制一份本地副本，再释放锁。这样不会长时间阻塞其他任务。

### 5.4 控制方式

- **Web 页面**：访问 ESP32 IP 地址，可查看数据、开关 LED 和继电器、拖动滑块调光。页面每 2 秒自动刷新。
- **MQTT**：向 `esp32/cmd` 发送命令，例如 `led on`、`relay off`、`led duty 128`。
- **串口命令**：输入 `mqtt <IP>` 修改服务器，`info` 查看状态，`reboot` 重启。

## 6. 关键代码解析

### 6.1 互斥锁保护共享数据

```cpp
dataMutex = xSemaphoreCreateMutex();
```

FreeRTOS 中多个任务同时读写同一个变量时，必须使用互斥锁。否则可能出现“LED 状态已经改变，但显示任务读到旧值”的显示错误。

### 6.2 WiFiManager 自动配网

```cpp
wm.setConfigPortalTimeout(120);
bool connected = wm.autoConnect("ESP32_SmartHome", "12345678");
```

- 如果有已保存的 Wi-Fi，直接连接。
- 如果没有或连接失败，开启热点 `ESP32_SmartHome`，密码 `12345678`。
- 手机连接后访问 `192.168.4.1` 配置 Wi-Fi。

### 6.3 MQTT 命令解析

```cpp
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  char msg[64];
  memcpy(msg, payload, length);
  msg[length] = '\0';
  String cmd = String(msg);
  // ... 解析并执行 led on / relay off 等命令
}
```

MQTT 消息是字节数组，末尾不会自动带 `\0`。必须手动复制到字符数组并加结束符，才能用 `String` 安全处理。

### 6.4 Web 服务器 API

```cpp
server.on("/api/led", handleApiLed);
server.on("/api/relay", handleApiRelay);
server.on("/api/brightness", handleApiBrightness);
server.on("/api/data", handleApiData);
```

- `/api/data` 返回 JSON 格式的实时数据，方便前端或第三方程序调用。
- `/api/led?state=on` 和 `/api/relay?state=on` 用于开关设备。
- `/api/brightness?duty=128` 用于设置 LED PWM 亮度。

### 6.5 OTA 无线升级

```cpp
ArduinoOTA.setHostname("esp32-smarthome");
ArduinoOTA.begin();
```

启动 OTA 后，在 Arduino IDE “工具”→“端口”中会看到一个网络端口 `esp32-smarthome`。选择它并点击上传，即可像用 USB 一样更新程序。

## 7. 运行效果

上传程序后，打开串口监视器（波特率 115200）：

```
================================
综合项目 P15：完整智能家居节点
================================
MQTT 服务器：192.168.1.100
正在启动 WiFiManager...
正在连接 Wi-Fi.......................
Wi-Fi 已连接，IP：192.168.1.107
Web 服务器已启动
OTA 已启用，主机名：esp32-smarthome
所有任务已创建，系统运行中...
```

OLED 上会显示温度、湿度、光照、LED 状态、继电器状态、Wi-Fi 和 MQTT 连接状态。

如果这是第一次运行且没有保存 Wi-Fi，ESP32 会开启热点 `ESP32_SmartHome`。用手机连接后，按提示输入家庭 Wi-Fi 密码即可。

之后在手机/电脑浏览器访问串口打印的 IP 地址，可以看到控制面板；或者通过 MQTT 客户端向 `esp32/cmd` 发送命令控制设备。

## 8. 常见问题

| 问题现象 | 可能原因 | 排查方法 |
|----------|----------|----------|
| OLED 不显示 | I2C 地址错误或接线松动 | 用 I2C 扫描程序确认地址是 0x3C 还是 0x3D；检查 SDA/SCL |
| DHT22 读取失败 | 接线错误或缺少上拉 | 检查 GPIO4；DATA 与 3.3V 之间加 4.7kΩ 上拉 |
| 无法进入 WiFiManager 配网 | 已经保存了错误的 Wi-Fi 信息 | 在代码中临时调用 `wm.resetSettings()` 一次，或重新烧录清除 Flash |
| MQTT 连接失败 | 服务器地址错误、Broker 未运行、防火墙 | 用电脑 MQTT 客户端测试 Broker；检查串口打印的 MQTT 状态码 |
| Web 页面打不开 | Wi-Fi 未连接、设备不在同一局域网 | 检查串口 IP；确认手机和 ESP32 连同一 Wi-Fi |
| OTA 找不到网络端口 | 电脑与 ESP32 不在同一网络、防火墙 | 确认电脑和 ESP32 在同一局域网；检查 Arduino IDE 端口列表 |
| 程序不断重启 | 某个任务堆栈太小 | 增大任务堆栈，例如 NetworkLoop 用 8192 字节 |
| 继电器不受控制 | 继电器是低电平触发 | 有些继电器模块是低电平触发（IN 为 LOW 时吸合），需要把代码中的 HIGH/LOW 反过来 |

## 9. 扩展挑战

1. **自动亮度**：根据光敏电阻数值自动调节 LED 和 OLED 亮度，夜晚不刺眼。
2. **Home Assistant 自动发现**：在 MQTT 中加入 Home Assistant 的 discovery 报文，让设备被自动识别为温湿度传感器、光照传感器、灯光和开关。
3. **历史数据曲线**：把传感器数据定期写入 LittleFS，并在 Web 页面用 JavaScript 绘制成折线图。
4. **多继电器/多路传感器**：扩展代码支持多个继电器和更多传感器，做成真正的智能家居控制中心。
5. **物理按键交互**：增加一个按键，短按切换 LED，长按清除 Wi-Fi 配置并重新进入配网模式。
