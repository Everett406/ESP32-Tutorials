# P08_Web_Temperature_Monitor

## 项目简介

本项目让 ESP32 变身一个**网页版温湿度监测站**。它先通过 Wi-Fi 连入家庭网络，然后不断读取 DHT22 传感器的温度和湿度，最后在手机或电脑的浏览器上以网页形式展示出来。页面每 2 秒自动刷新一次，无需手动点击刷新按钮。

这个项目把"传感器读取"和"网络服务"结合在一起，是学习物联网（IoT）的经典入门案例。

## 涉及知识点

- ESP32 连接 2.4GHz Wi-Fi（`WiFi.h`）
- DHT22 温湿度传感器读取（`DHT.h`）
- 搭建 HTTP Web 服务器（`WebServer.h`）
- HTML 基础与 `meta refresh` 自动刷新
- ESP32 Arduino 3.x 环境下的库安装

## 硬件清单

| 名称 | 数量 | 说明 |
|---|---|---|
| ESP32 开发板 | 1 | 推荐 ESP32 Dev Module |
| DHT22 温湿度传感器 | 1 | 也可用 DHT11，但需要修改代码中的传感器类型 |
| 杜邦线 | 3 | 连接 VCC、GND、DATA |

> 很多 DHT22 模块已经内置了 4.7kΩ ~ 10kΩ 上拉电阻，如果没有内置，需要在 DATA 和 3.3V 之间接一个。

## 电路连接

| ESP32 | 连接 | DHT22 |
|---|---|---|
| 3.3V | → | VCC |
| GND | → | GND |
| GPIO4 | → | DATA |

> **为什么选 GPIO4？**
> GPIO4 是通用 GPIO，既可以输入也可以输出，适合驱动 DHT22 的单总线协议。你也可以改成其他 GPIO，只要在代码里同步修改 `DHT_PIN` 即可。
>
> **能不能接 5V？**
> ESP32 的 GPIO 只能承受 3.3V 电平。如果你的 DHT22 模块把 DATA 上拉到 5V，请改成 3.3V，否则可能损坏 ESP32。

## 代码思路

1. **引入库**：`WiFi.h` 负责联网，`WebServer.h` 负责响应浏览器请求，`DHT.h` 负责读取传感器。
2. **连接 Wi-Fi**：用 `WiFi.begin()` 输入 SSID 和密码，等待 `WiFi.status() == WL_CONNECTED`。
3. **启动 Web 服务器**：注册根路径 `/` 的处理函数 `handleRoot()`，然后调用 `server.begin()`。
4. **定时读取传感器**：主循环中每隔 2 秒调用 `readSensor()`，更新全局变量 `temperature` 和 `humidity`。
5. **生成网页**：`handleRoot()` 构造一段 HTML 文本，包含当前的温度和湿度，并通过 `server.send()` 返回给浏览器。
6. **自动刷新**：HTML 中加入 `<meta http-equiv='refresh' content='2;url=/'>`，让浏览器每 2 秒自动重新请求页面。

## 关键代码解析

### 1. 连接 Wi-Fi

```cpp
WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
while (WiFi.status() != WL_CONNECTED) {
  delay(500);
  Serial.print(".");
}
Serial.println(WiFi.localIP());
```

`WiFi.begin()` 开始连接，但连接需要时间。`while` 循环不断检查状态，直到 `WL_CONNECTED` 表示成功并获取到 IP 地址。`WiFi.localIP()` 就是 ESP32 在局域网中的"门牌号"，浏览器输入这个地址就能访问网页。

### 2. 注册网页路由

```cpp
server.on("/", handleRoot);
server.begin();
```

`server.on(path, function)` 告诉 Web 服务器：当浏览器访问某个路径时，应该调用哪个函数。这里把根路径 `/` 映射到 `handleRoot()`。

### 3. 处理客户端请求

```cpp
void loop() {
  server.handleClient();
  // ...
}
```

`server.handleClient()` 必须在 `loop()` 中反复调用。它会检查是否有浏览器发来请求，并调用对应的处理函数。如果长时间不调用，网页就会"卡死"。

### 4. 自动刷新网页

```cpp
html += "<meta http-equiv='refresh' content='2;url=/'>";
```

这是一个纯 HTML 的自动刷新方案。`content='2'` 表示 2 秒后刷新，`url='/'` 表示刷新后请求的地址。它的优点是简单，不需要写 JavaScript；缺点是整页会重新加载。

### 5. 传感器读取与错误处理

```cpp
float new_temp = dht.readTemperature();
float new_humi = dht.readHumidity();
if (isnan(new_temp) || isnan(new_humi)) {
  Serial.println("DHT22 读取失败");
  return;
}
```

DHT22 偶尔会因为时序问题读取失败，`readTemperature()` 和 `readHumidity()` 失败时会返回 `NaN`（Not a Number）。用 `isnan()` 判断后丢弃异常值，避免把错误数据传到网页上。

## 运行效果

1. 在代码中把 `WIFI_SSID` 和 `WIFI_PASSWORD` 改成你的 Wi-Fi 信息。
2. 确认 Arduino IDE 已安装 **DHT sensor library**（Adafruit）和 **Adafruit Unified Sensor**。
3. 上传程序，打开串口监视器，波特率 **115200**。
4. 看到如下输出：
   ```
   正在连接 Wi-Fi: YOUR_WIFI_SSID
   .....Wi-Fi 连接成功！
   IP 地址：192.168.x.x
   Web 服务器已启动
   请在浏览器中输入上面的 IP 地址访问网页
   温度：25.60 ℃，湿度：60.00 %
   ```
5. 在电脑或手机浏览器中输入串口打印的 IP 地址，例如 `http://192.168.1.100`。
6. 页面会显示当前温度和湿度，并每 2 秒自动刷新。

## 常见问题

### 1. 编译报错：找不到 DHT.h

- 在 Arduino IDE 中点击 **项目 → 加载库 → 管理库**。
- 搜索 "DHT sensor library"，安装作者为 Adafruit 的版本。
- 安装时会自动安装依赖库 "Adafruit Unified Sensor"。

### 2. Wi-Fi 连不上

- 确认你的 Wi-Fi 是 **2.4GHz**，ESP32 不支持 5GHz。
- 检查 SSID 和密码是否输入正确（区分大小写和空格）。
- 确认路由器没有开启 MAC 地址过滤或访客网络隔离。

### 3. 网页打不开

- 确认访问的是串口监视器打印的 IP 地址。
- 确认手机和 ESP32 连接的是同一个路由器（同一个局域网）。
- 检查防火墙是否拦截了局域网访问。

### 4. 温湿度显示为 "nan" 或固定不变

- 检查 DHT22 的 VCC、GND、DATA 是否接对。
- 检查 DATA 是否接了上拉电阻（有些模块已内置）。
- DHT22 读取间隔不要小于 2 秒，否则容易失败。
- 尝试把 `DHT_PIN` 改成其他 GPIO，排除引脚损坏问题。

### 5. 中文显示乱码

- 网页中已加入 `<meta charset='UTF-8'>`，确保浏览器使用 UTF-8 编码。
- 如果仍然乱码，检查浏览器编码设置是否为 UTF-8。

## 扩展挑战

1. **数据曲线图**：把温湿度数据通过 `/api` 接口以 JSON 格式返回，再用网页 JavaScript 每 2 秒请求一次，并用 Canvas 或 Chart.js 绘制实时曲线。
2. **高温报警**：当温度超过设定阈值时，蜂鸣器响起，网页背景变红并显示警告信息。
3. **云平台上传**：把数据定时上传到 ThingSpeak、巴法云或自建服务器，实现远程长期监测和历史记录。
