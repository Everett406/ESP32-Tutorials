# P12 MQTT 气象站

## 1. 项目简介

本项目做一个可以通过 MQTT 协议接入智能家居系统的气象站：

- **本地**：用 OLED 实时显示温度、湿度、光照和网络状态。
- **远程**：通过 MQTT 把传感器数据发布到代理服务器（Broker），例如 Home Assistant 的 Mosquitto 插件。
- **控制**：订阅一个命令主题，支持通过 MQTT 发送 `on`、`off`、`blink` 控制 ESP32 上的 LED。

通过这个项目，你可以理解物联网中最核心的"发布/订阅"模型，为后续搭建全屋智能系统打下基础。

## 2. 涉及知识点

| 技能 | 来源教程 | 作用 |
|------|----------|------|
| DHT22 读取 | `peripherals/DHT11_DHT22_Sensor` | 获取温湿度 |
| ADC 光敏读取 | `04_ADC_Read_Analog` / `peripherals/LDR_Light_Sensor` | 获取光照 |
| OLED 显示 | `peripherals/I2C_OLED_SSD1306_Display` | 本地显示 |
| Wi-Fi 连接 | `06_WiFi_Connect` | 接入网络 |
| MQTT 通信 | 本项目新引入 | 数据上报与远程命令 |

## 3. 硬件清单

| 元器件 | 数量 | 说明 |
|--------|------|------|
| ESP32 开发板 | 1 块 | 推荐 ESP32 DevKitC |
| DHT22 传感器 | 1 个 | 也可用 DHT11 |
| 光敏电阻 | 1 个 | 检测环境光 |
| 10kΩ 电阻 | 1 只 | 与光敏电阻组成分压电路 |
| 4.7kΩ~10kΩ 电阻 | 1 只 | DHT22 DATA 上拉（如模块未集成） |
| SSD1306 OLED（128×64，I2C） | 1 块 | 本地显示 |
| LED | 1 颗 | 命令演示灯 |
| 330Ω 电阻 | 1 只 | LED 限流 |
| 杜邦线 | 若干 | 连接电路 |

## 4. 电路连接

### 4.1 DHT22 接线

| DHT22 | ESP32 |
|-------|-------|
| VCC | 3.3V |
| GND | GND |
| DATA | GPIO4 |

### 4.2 光敏电阻分压电路

| ESP32 引脚 | 连接 |
|------------|------|
| 3.3V | → 光敏电阻一端 |
| GPIO34 | → 光敏电阻另一端 + 10kΩ 电阻一端 |
| GND | → 10kΩ 电阻另一端 |

**原理**：光线越强，光敏电阻阻值越小，GPIO34 电压越低，ADC 读数越小。程序里反向映射成"亮度百分比"。

### 4.3 OLED 接线

| OLED | ESP32 |
|------|-------|
| VCC | 3.3V |
| GND | GND |
| SDA | GPIO21 |
| SCL | GPIO22 |

### 4.4 命令演示 LED

| ESP32 引脚 | 连接 |
|------------|------|
| GPIO2 | → LED 正极（长脚） |
| GND | → 330Ω 电阻 → LED 负极（短脚） |

## 5. 代码思路

程序启动后的工作流程：

1. **初始化**：串口、LED、ADC、DHT22、OLED。
2. **连接 Wi-Fi**：失败后会在 loop 中定时重试。
3. **设置 MQTT**：配置服务器地址、端口、回调函数。
4. **主循环**：
   - 检查并维持 MQTT 连接；
   - 周期性读取 DHT22 和光敏电阻；
   - 刷新 OLED；
   - 每隔 10 秒向 MQTT 发布一次数据；
   - 处理收到的 MQTT 命令。

## 6. 关键代码解析

### 6.1 什么是 MQTT？

MQTT（Message Queuing Telemetry Transport）是一种轻量级的物联网通信协议，基于"发布/订阅"模型：

- **Broker（代理）**：消息中转站，所有设备都连到它。
- **Topic（主题）**：消息的分类路径，例如 `esp32/weather/temperature`。
- **Publish（发布）**：设备把数据发送到某个主题。
- **Subscribe（订阅）**：设备告诉 Broker"我对某个主题感兴趣"，有新消息时 Broker 会推送过来。

这种方式的好处是发送方和接收方不需要直接知道对方的存在，扩展性非常好。

### 6.2 连接 MQTT 服务器

```cpp
mqttClient.setServer(mqtt_server, mqtt_port);
mqttClient.setCallback(callback);
```

`setServer()` 设置代理地址和端口，`setCallback()` 设置收到消息时调用的函数。

### 6.3 遗嘱消息（LWT）

```cpp
mqttClient.connect(clientId.c_str(), mqtt_user, mqtt_pass, topic_status, 0, true, "offline");
mqttClient.publish(topic_status, "online", true);
```

- LWT（Last Will and Testament）是 MQTT 的一项重要功能。
- 如果 ESP32 异常断开，Broker 会自动向 `topic_status` 发送 `"offline"`。
- 正常连接时发布 `"online"`，让订阅者知道设备在线状态。
- 第三个参数 `true` 表示"保留消息"（Retain），新订阅者上线时能立即收到最后一条状态。

### 6.4 发布传感器数据

```cpp
mqttClient.publish(topic_temp, tempStr.c_str());
mqttClient.publish(topic_hum, humStr.c_str());
mqttClient.publish(topic_light, lightStr.c_str());
```

每条 `publish()` 都会把一条消息发送到对应的主题。其他订阅了这些主题的设备或平台就能收到数据。

### 6.5 处理远程命令

```cpp
void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  message.trim();

  if (message == "on") { digitalWrite(LED_PIN, HIGH); }
  else if (message == "off") { digitalWrite(LED_PIN, LOW); }
  else if (message == "blink") { /* 闪烁三次 */ }
}
```

`callback()` 是收到订阅主题消息时自动调用的函数。这里把字节数组转成字符串，再根据不同的命令控制 LED。

## 7. 运行效果

### 7.1 串口输出

上传程序后，串口监视器会输出类似：

```text
================================
综合项目 P12：MQTT 气象站
================================
正在连接 Wi-Fi......
Wi-Fi 已连接，IP 地址：192.168.x.x
系统初始化完成
正在连接 MQTT 服务器...已连接
已订阅主题：esp32/weather/cmd
已发布：T=25.3 H=58.2 L=72
```

### 7.2 OLED 显示

```text
MQTT Weather
WiFi:OK MQTT:OK
T:25.3°C
H:58.2%
L:72%
192.168.x.x
```

### 7.3 MQTT 数据

如果代理服务器运行正常，你可以用 MQTT 客户端（如 MQTT.fx、MQTTX、mosquitto_sub）订阅：

```bash
mosquitto_sub -h 192.168.1.100 -t "esp32/weather/temperature"
mosquitto_sub -h 192.168.1.100 -t "esp32/weather/humidity"
mosquitto_sub -h 192.168.1.100 -t "esp32/weather/light"
```

每隔 10 秒会收到一条新的传感器数据。

### 7.4 远程控制 LED

向命令主题发送消息即可控制 LED：

```bash
mosquitto_pub -h 192.168.1.100 -t "esp32/weather/cmd" -m "on"
mosquitto_pub -h 192.168.1.100 -t "esp32/weather/cmd" -m "off"
mosquitto_pub -h 192.168.1.100 -t "esp32/weather/cmd" -m "blink"
```

## 8. 常见问题

### 8.1 MQTT 连接失败

1. 检查 `mqtt_server` 是否填对了 Broker 的 IP 或域名。
2. 检查 Broker 端口，默认是 1883。如果启用了 TLS，要用 8883 并配置证书。
3. 检查用户名密码是否正确，如果 Broker 没有启用认证，保持为空字符串 `""`。
4. 确认 ESP32 和 Broker 在同一个局域网，或者 Broker 允许外网访问。
5. 查看串口打印的状态码：
   - `-4`：连接超时，检查网络或服务器地址。
   - `-2`：服务器拒绝连接，常见原因是用户名/密码错误。

### 8.2 收不到传感器数据

1. 用 `mosquitto_sub` 手动订阅对应主题，确认 Broker 收到了数据。
2. 检查主题名称是否拼写正确，MQTT 主题区分大小写。
3. 检查 DHT22 和光敏电阻接线。

### 8.3 命令下发后 LED 没反应

1. 检查串口是否打印了"收到 MQTT 消息"。
2. 检查命令主题是否一致，发送端和接收端必须订阅/发布同一个主题。
3. 注意消息前后是否有空格，程序里用 `trim()` 去掉了空格，但某些调试工具可能发送了不可见字符。

### 8.4 OLED 不显示

1. 检查 I2C 地址，常见 0x3C 或 0x3D。
2. 检查 SDA/SCL 接线，VCC 是否为 3.3V。

### 8.5 多台 ESP32 互相挤掉

MQTT 的 `clientId` 必须是唯一的。程序里用 `ESP.getEfuseMac()` 生成基于芯片 MAC 地址的 ID，保证每台设备不同。

## 9. 扩展挑战

1. **JSON 格式上报**：把所有传感器数据打包成一条 JSON 消息发布到 `esp32/weather/data`，例如 `{"temperature":25.3,"humidity":58.2,"light":72}`，方便 Home Assistant 解析。
2. **Home Assistant 自动发现**：实现 MQTT Discovery，让 ESP32 自动在 Home Assistant 中注册为温度和湿度传感器。
3. **OLED 多页面**：增加按键或触摸切换显示页面，例如"传感器页"和"网络/MQTT 状态页"。
4. **离线缓存**：当 MQTT 断开时，把最近的几条数据保存在数组中，恢复连接后批量补发，避免数据丢失。
5. **TLS 加密连接**：使用端口 8883 和 SSL/TLS 证书连接 MQTT，提高安全性。
6. **低功耗改造**：结合 Deep Sleep，每 10 分钟唤醒一次上报数据，用电池供电做成室外气象站。
