# P11 智能家居中心

## 1. 项目简介

本项目打造一个mini的"智能家居中心"：

- 用 **DHT22** 读取环境温度和湿度。
- 用 **光敏电阻分压电路** 检测环境光照强弱。
- 用 **OLED 屏幕** 实时显示当前数据。
- 用 **Web 服务器** 在浏览器里查看数据并修改报警阈值。
- 用 **Preferences** 把报警阈值保存到 Flash，断电后仍然保留。

它涵盖了"采集 → 处理 → 显示 → 存储"的完整数据链路，是后面 MQTT、数据记录等更复杂项目的基础。

## 2. 涉及知识点

| 技能 | 来源教程 | 作用 |
|------|----------|------|
| DHT22 读取 | `peripherals/DHT11_DHT22_Sensor` | 获取温湿度 |
| ADC 模拟输入 | `04_ADC_Read_Analog` | 读取光敏电阻电压 |
| I2C OLED 显示 | `peripherals/I2C_OLED_SSD1306_Display` | 本地显示数据 |
| Wi-Fi / Web 服务器 | `06_WiFi_Connect` / `07_Web_Server_LED` | 远程查看和控制 |
| Preferences 存储 | `13_Preferences_Storage` | 保存报警阈值 |

## 3. 硬件清单

| 元器件 | 数量 | 说明 |
|--------|------|------|
| ESP32 开发板 | 1 块 | 推荐 ESP32 DevKitC |
| DHT22 传感器 | 1 个 | 也可使用 DHT11，代码里改类型 |
| 光敏电阻 | 1 个 | 用于检测环境光 |
| 10kΩ 电阻 | 1 只 | 与光敏电阻组成分压电路 |
| 4.7kΩ~10kΩ 电阻 | 1 只 | DHT22 DATA 上拉电阻（部分模块已集成） |
| SSD1306 OLED（128×64，I2C） | 1 块 | 本地显示 |
| LED | 1 颗 | 报警指示灯 |
| 330Ω 电阻 | 1 只 | LED 限流 |
| 杜邦线 | 若干 | 连接电路 |

## 4. 电路连接

### 4.1 DHT22 接线

| DHT22 | ESP32 | 说明 |
|-------|-------|------|
| VCC | 3.3V | 供电 |
| GND | GND | 接地 |
| DATA | GPIO4 | 数据引脚 |

> 如果你的 DHT22 模块没有板载上拉电阻，请在 DATA 和 3.3V 之间接一只 4.7kΩ~10kΩ 电阻。

### 4.2 光敏电阻分压电路

| ESP32 引脚 | 连接 |
|------------|------|
| 3.3V | → 光敏电阻一端 |
| GPIO34 | → 光敏电阻另一端 + 10kΩ 电阻一端 |
| GND | → 10kΩ 电阻另一端 |

**原理说明**：

光敏电阻的阻值会随光线变化。光线越强，阻值越小，GPIO34 分到的电压就越低，ADC 读数越小。程序里通过 `map()` 把它反向映射成"亮度百分比"，百分比越大代表越亮。

### 4.3 OLED 接线

| OLED | ESP32 | 说明 |
|------|-------|------|
| VCC | 3.3V | 供电 |
| GND | GND | 接地 |
| SDA | GPIO21 | I2C 数据线 |
| SCL | GPIO22 | I2C 时钟线 |

### 4.4 报警 LED

| ESP32 引脚 | 连接 |
|------------|------|
| GPIO2 | → LED 正极（长脚） |
| GND | → 330Ω 电阻 → LED 负极（短脚） |

## 5. 代码思路

程序启动后的工作流程：

1. **初始化**：串口、ADC、DHT22、OLED、报警 LED。
2. **加载阈值**：从 Preferences 读取上次保存的温度/湿度/光照阈值，没有则使用默认值。
3. **连接 Wi-Fi**：成功后启动 Web 服务器。
4. **主循环**：
   - 处理浏览器请求；
   - 每隔 2.5 秒读取一次 DHT22 和光敏电阻；
   - 判断报警条件，控制报警 LED；
   - 每隔 500ms 刷新一次 OLED。

## 6. 关键代码解析

### 6.1 光敏电阻读数转换

```cpp
long sum = 0;
for (int i = 0; i < 10; i++) {
  sum += analogRead(LDR_PIN);
  delay(5);
}
lightRaw = sum / 10;
lightPercent = map(lightRaw, 0, 4095, 100, 0);
lightPercent = constrain(lightPercent, 0, 100);
```

- 连续读取 10 次取平均，可以降低 ADC 噪声。
- `map(raw, 0, 4095, 100, 0)` 把 ADC 值反向映射成百分比：越亮百分比越大。
- `constrain()` 保证结果一定在 0~100 之间，防止越界。

### 6.2 Preferences 保存阈值

```cpp
prefs.begin(PREF_NS, true);
prefs.putFloat(KEY_TEMP_TH, tempThreshold);
prefs.putFloat(KEY_HUM_TH, humThreshold);
prefs.putInt(KEY_LIGHT_TH, lightThreshold);
prefs.end();
```

- `begin(namespace, true)` 以读写模式打开命名空间。
- `putFloat()` / `putInt()` 分别写入浮点数和整数。
- `end()` 关闭命名空间，提交写入。 Flash 写入需要一定时间，只在阈值变化时写入。

### 6.3 Web 阈值设置

```cpp
if (server.hasArg("temp")) {
  tempThreshold = server.arg("temp").toFloat();
  changed = true;
}
```

网页表单提交后会访问 `/set?temp=...&hum=...&light=...`。程序读取参数并更新阈值，再保存到 Flash。

### 6.4 JSON 数据接口

```cpp
void handleData() {
  String json = "{\"temperature\":" + String(temperature, 1) + ... + "}";
  server.send(200, "application/json", json);
}
```

`/data` 接口返回 JSON，方便后续用 Python、Node-RED、Home Assistant 等读取数据。

## 7. 运行效果

上传程序后，串口监视器会输出：

```text
================================
综合项目 P11：智能家居中心
================================
已从 Flash 加载报警阈值：
  温度 > 30.00 ℃
  湿度 > 80.00 %
  光照 < 30 %
正在连接 Wi-Fi......
Wi-Fi 已连接，IP 地址：192.168.x.x
HTTP 服务器已启动
```

OLED 上会显示：

```text
Smart Home Center
T: 26.5°C
H: 55.0%
L: 68%
Status: OK
```

如果温度超过 30℃、湿度超过 80% 或光照低于 30%，OLED 和网页都会显示报警，同时 GPIO2 上的 LED 点亮。

用手机/电脑连接同一个 Wi-Fi，访问 ESP32 的 IP 地址，可以看到一个设置页面，上面显示实时数据，并且可以修改阈值。

## 8. 常见问题

### 8.1 DHT22 读数一直失败

1. 检查 DATA 是否接在 GPIO4，VCC 和 GND 是否接反。
2. 检查是否安装了 "DHT sensor library" 和 "Adafruit Unified Sensor"。
3. 如果模块没有上拉电阻，DATA 与 3.3V 之间接 4.7kΩ~10kΩ 电阻。
4. 读取间隔不能小于 2 秒，程序里已经设为 2.5 秒。

### 8.2 OLED 不显示

1. 检查 I2C 地址是否正确。常见地址是 0x3C，部分模块是 0x3D。
2. 检查 SDA/SCL 是否接反，VCC 是否是 3.3V。
3. 可以用 I2C 扫描程序确认 OLED 地址。

### 8.3 光照读数异常

1. 检查分压电路是否接错：3.3V → 光敏电阻 → GPIO34 → 10kΩ → GND。
2. 不同光敏电阻阻值范围差异很大，如果读数始终饱和，可以换一只不同阻值的光敏电阻或固定电阻。

### 8.4 阈值保存后重启丢失

1. 检查 Preferences 的命名空间和键名是否一致。
2. 检查是否调用了 `prefs.end()`。
3. 如果重新烧录程序时选择了"擦除所有 Flash"，会清空保存的阈值。

### 8.5 网页打不开

1. 确认手机和 ESP32 在同一个 Wi-Fi。
2. 检查串口输出的 IP 地址是否正确。
3. 某些公共/校园 Wi-Fi 会禁止设备间互相访问，换家用路由器测试。

## 9. 扩展挑战

1. **手机推送报警**：当温度/湿度/光照异常时，通过 HTTP 请求调用 Bark、PushPlus 或微信企业微信机器人，把报警信息推送到手机。
2. **联动控制**：增加一个继电器或蜂鸣器，报警时自动打开风扇或发出蜂鸣声。
3. **历史曲线**：用网页上的 ECharts 绘制最近一小时的温湿度折线图。需要把数据暂存在数组里，或者写入 Flash/SD 卡。
4. **夜间模式**：增加一个 RTC 时间判断或光敏阈值，晚上自动降低 OLED 亮度或关闭显示，避免影响休息。
5. **接入 Home Assistant**：把 `/data` 接口的数据通过 MQTT 或 REST  sensor 接入 Home Assistant，实现全屋智能联动。
