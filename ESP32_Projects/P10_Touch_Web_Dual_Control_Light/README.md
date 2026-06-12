# P10 触摸 + Web 双控灯

## 1. 项目简介

本项目做一个可以用两种方式控制的灯：

- **本地触摸控制**：用手指碰一下连接到 ESP32 触摸引脚的导线或铜箔片，LED 就会在开和关之间切换。
- **远程 Web 控制**：在手机或电脑的浏览器里打开 `http://esp32.local`，点击按钮可以开关灯，拖动滑块可以调节亮度。

通过这个项目，你可以学会如何把**触摸输入、PWM 调光、Wi-Fi、HTTP Web 服务器、mDNS 域名**整合在一起，做出一个真正的"智能硬件"。

## 2. 涉及知识点

本项目会用到前面教程中的以下技能：

| 技能 | 来源教程 | 作用 |
|------|----------|------|
| 电容触摸读取 | `11_Touch_Sensor` | 用 `touchRead()` 检测触摸动作 |
| PWM 调光 | `03_PWM_LED_Fade` | 用 `ledcAttach()` / `ledcWrite()` 控制 LED 亮度 |
| Wi-Fi 连接 | `06_WiFi_Connect` | 让 ESP32 接入局域网 |
| Web 服务器 | `07_Web_Server_LED` | 用浏览器远程控制硬件 |
| mDNS | `extensions/mDNS_Web_Server` | 用 `esp32.local` 域名代替 IP 地址 |

## 3. 硬件清单

| 元器件 | 数量 | 说明 |
|--------|------|------|
| ESP32 开发板 | 1 块 | 推荐使用 ESP32 DevKitC 或 NodeMCU-32S |
| LED | 1 颗 | 普通 3mm/5mm LED，颜色随意 |
| 330Ω 电阻 | 1 只 | 限制 LED 电流 |
| 杜邦线 | 若干 | 连接电路 |
| 导线/铜箔/硬币 | 1 个 | 作为触摸电极 |

## 4. 电路连接

| ESP32 引脚 | 连接目标 | 说明 |
|------------|----------|------|
| GPIO2 | LED 正极（长脚） | 输出 PWM 信号 |
| GND | 330Ω 电阻 → LED 负极（短脚） | 限流后接地 |
| GPIO4 | 导线/铜箔/硬币 | 触摸电极，不接电源或地 |

**实物接线步骤：**

1. 把 LED 长脚（正极）插到 GPIO2。
2. 把 LED 短脚（负极）串一只 330Ω 电阻后接到 GND。
3. 拿一根杜邦线，一端接 GPIO4，另一端悬空或焊一小块铜箔片作为触摸电极。

> **注意**：ESP32 的触摸引脚支持的列表是 GPIO0、GPIO2、GPIO4、GPIO12、GPIO13、GPIO14、GPIO15、GPIO27、GPIO32、GPIO33。本项目使用 GPIO4。

## 5. 代码思路

程序启动后按以下顺序工作：

1. **初始化 PWM**：把 GPIO2 配置为 5kHz、8 bit 分辨率的 PWM 输出，这样可以用 0~255 的数值控制亮度。
2. **连接 Wi-Fi**：输入你的路由器名称和密码，ESP32 以 STA 模式连入局域网。
3. **启动 mDNS**：注册 `esp32.local` 域名，这样手机/电脑不需要记住 IP 地址。
4. **启动 Web 服务器**：注册 `/`、`/on`、`/off`、`/set` 四个路由。
5. **主循环**：
   - 处理浏览器请求；
   - 维持 mDNS；
   - 读取触摸值，判断是否被触摸；
   - 用"边沿检测 + 时间消抖"实现稳定的触摸开关。

## 6. 关键代码解析

### 6.1 PWM 初始化

```cpp
ledcAttach(LED_PIN, PWM_FREQ, PWM_RESOLUTION);
```

`ledcAttach()` 是 ESP32 Arduino 3.x 的写法，它把引脚绑定到 LEDC 硬件。`PWM_FREQ` 是 5000Hz，`PWM_RESOLUTION` 是 8 bit，所以占空比范围是 `0~255`。

### 6.2 触摸检测与消抖

```cpp
bool isTouched = (touchValue < TOUCH_THRESHOLD);
if (isTouched && !lastTouchState && (millis() - lastTouchTime > TOUCH_DEBOUNCE)) {
  ledState = !ledState;
  applyLED();
  lastTouchTime = millis();
}
lastTouchState = isTouched;
```

- `isTouched && !lastTouchState` 是**边沿检测**，只在"从未触摸变成触摸"的那一瞬间触发一次。
- `millis() - lastTouchTime > TOUCH_DEBOUNCE` 是**时间消抖**，防止手指抖动导致一次触摸被识别成多次。
- 如果没有这两个保护，手指按着不放时 LED 会高速闪烁。

### 6.3 双通道状态同步

```cpp
void applyLED() {
  int duty = ledState ? brightness : 0;
  ledcWrite(LED_PIN, duty);
}
```

不管是触摸触发还是网页命令，最终都通过 `applyLED()` 把 `ledState` 和 `brightness` 转换成 PWM 输出。这样两个控制源不会冲突。

### 6.4 网页亮度调节

```cpp
if (server.hasArg("duty")) {
  brightness = constrain(server.arg("duty").toInt(), 0, 255);
  ledState = true;
  applyLED();
}
```

网页上的滑块提交后会访问 `/set?duty=128`。`server.arg("duty")` 读取这个参数，`constrain()` 保证数值在安全范围内。

### 6.5 mDNS 域名

```cpp
MDNS.begin("esp32");
MDNS.addService("http", "tcp", 80);
```

这两行让 ESP32 在局域网里广播自己的名字。其他设备只要支持 mDNS，就可以用 `http://esp32.local` 访问，而不需要输入类似 `192.168.1.105` 的 IP。

## 7. 运行效果

上传程序后，打开串口监视器（波特率 115200），你应该看到：

```text
================================
综合项目 P10：触摸 + Web 双控灯
================================
正在连接 Wi-Fi......
Wi-Fi 已连接，IP 地址：192.168.x.x
mDNS 已启动，可通过 http://esp32.local 访问
HTTP 服务器已启动
```

然后用手机或电脑连接**同一个 Wi-Fi**，在浏览器地址栏输入 `http://esp32.local`，会看到控制页面。

测试方法：

1. **触摸控制**：用手指碰一下 GPIO4 上的导线，LED 状态应该切换一次。连续按住不会连续切换。
2. **网页开关**：点击"打开 LED"/"关闭 LED"按钮，观察 LED 变化。
3. **网页调光**：拖动亮度滑块到中间位置，LED 会变为中等亮度。

串口会实时打印触摸值、LED 状态和当前亮度，方便调试。

## 8. 常见问题

### 8.1 触摸没有反应

1. 先看串口打印的触摸值，未触摸时一般在 50~100，触摸时会明显下降到 30 以下。
2. 如果变化不明显，调整 `TOUCH_THRESHOLD` 阈值。
3. 检查触摸引脚是否接在 GPIO4，且导线不要太长（长导线容易受干扰）。

### 8.2 访问 `http://esp32.local` 打不开

1. 确认手机和 ESP32 连接的是**同一个 Wi-Fi**。
2. 部分 Android 手机或 Windows 电脑不支持 `.local` 域名，可直接使用串口打印的 IP 地址访问。
3. Windows 通常需要安装 Bonjour（iTunes 自带）才能解析 `.local`。

### 8.3 LED 自己乱闪

1. 阈值设得太低，环境干扰被误判为触摸，把 `TOUCH_THRESHOLD` 调小一点。
2. 触摸电极附近有大片金属或电源线，尝试换成小片铜箔并远离干扰源。

### 8.4 网页按钮点击后无反应

1. 检查串口是否有 "Web 命令：..." 输出，确认请求是否到达 ESP32。
2. 检查 GPIO2 上的 LED 是否接反，或电阻是否接好。

## 9. 扩展挑战

1. **网页自动刷新**：在 HTML 头部加入 `<meta http-equiv='refresh' content='5'>`，让页面每 5 秒自动刷新，这样多人同时操作时状态更及时。
2. **AJAX 无刷新控制**：用 JavaScript 的 `fetch()` 发送 `/on`、`/off`、`/set` 请求，不刷新整个页面就能更新状态。
3. **JSON API**：增加 `/api/status` 接口，返回 `{"led":true,"brightness":128}`，方便其他程序（如 Python、Home Assistant）调用。
4. **实体触控台灯**：把触摸电极换成弹簧或铜箔片，3D 打印一个底座，做一个可以摆在桌上的触控台灯。
