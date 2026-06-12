# P02 按键调光灯

## 1. 项目简介

本项目制作一个**单按键调光灯**。一个按键实现两种操作：
- **短按**：切换 LED 的开/关状态
- **长按**：循环调节 LED 亮度，从 0% 逐渐增加到 100%，再逐渐减少到 0%

通过本项目，你可以学习如何在同一个按键上区分"短按"和"长按"，理解状态机和非阻塞计时的编程思想。

## 2. 涉及知识点

- [02_Digital_Input_Button](../ESP32_Tutorial_Series/02_Digital_Input_Button/)：按键读取和消抖
- [03_PWM_LED_Fade](../ESP32_Tutorial_Series/03_PWM_LED_Fade/)：PWM 调光
- [05_Serial_Communication](../ESP32_Tutorial_Series/05_Serial_Communication/)：millis() 非阻塞计时

## 3. 硬件清单

| 序号 | 器件 | 数量 | 说明 |
|------|------|------|------|
| 1 | ESP32 开发板 | 1 块 | 主控芯片 |
| 2 | LED | 1 个 | 被控制的灯 |
| 3 | 330Ω 电阻 | 1 个 | LED 限流电阻 |
| 4 | 按键 | 1 个 | 用户输入 |
| 5 | 杜邦线 | 若干 | 连接电路 |
| 6 | 面包板 | 1 块 | 方便插接（可选） |

## 4. 电路连接

| 起点 | 终点 | 备注 |
|------|------|------|
| ESP32 GPIO2 | LED 正极（长脚） | PWM 输出 |
| LED 负极（短脚） | 330Ω 电阻一端 | 限流 |
| 330Ω 电阻另一端 | ESP32 GND | 接地 |
| ESP32 GPIO4 | 按键一端 | 数字输入 |
| 按键另一端 | ESP32 GND | 使用内部上拉 |

### 4.1 为什么按键不需要外部上拉电阻？

本项目使用 `pinMode(BUTTON_PIN, INPUT_PULLUP)` 启用 ESP32 内部上拉电阻。内部上拉电阻约 30kΩ~50kΩ，会把 GPIO4 默认拉到 HIGH。按键按下时，GPIO4 直接连到 GND，读到 LOW。

如果以后遇到按键不稳定、容易误触发的情况，可以改为外部 10kΩ 上拉电阻，通常比内部上拉更稳定。

## 5. 代码思路

程序的整体流程如下：

1. **初始化**：设置串口、LED、PWM、按键（内部上拉）。
2. **读取按键**：每次循环读取 GPIO4 状态。
3. **消抖处理**：只有当状态变化持续超过 40ms，才认为是真实按键动作。
4. **检测按下**：记录按下时间 `buttonDownTime`。
5. **检测松开**：
   - 如果按住时间 < 600ms，视为短按，调用 `toggleLed()` 切换开关。
   - 如果按住时间 ≥ 600ms，视为长按（在按下过程中已经处理）。
6. **长按调光**：按键持续按着时，每隔 80ms 改变一次亮度，并自动循环。
7. **更新输出**：根据 `ledState` 和 `brightness` 输出 PWM。

## 6. 关键代码解析

### 6.1 消抖逻辑

```cpp
if (reading != lastButtonReading) {
  lastDebounceTime = millis();
}

if ((millis() - lastDebounceTime) > DEBOUNCE_MS) {
  if (reading != stableButtonState) {
    stableButtonState = reading;
    // ...
  }
}
```

当读取值变化时，不立即相信，而是等待 40ms。如果 40ms 后状态仍然稳定，才认为是真实按键动作。这能有效消除机械按键的抖动。

### 6.2 短按和长按的判断

```cpp
if (stableButtonState == LOW) {
  buttonDownTime = millis();
  longPressHandled = false;
}
else {
  unsigned long pressDuration = millis() - buttonDownTime;
  if (pressDuration < LONG_PRESS_MS && !longPressHandled) {
    toggleLed();  // 短按
  }
}
```

- 按下时记录时间。
- 松开时计算按住时长。
- 如果时长小于阈值且没有处理过长按，就执行短按。
- `longPressHandled` 防止长按松开后又触发一次短按。

### 6.3 长按循环调光

```cpp
brightness += BRIGHTNESS_STEP * dimDirection;

if (brightness >= 255) {
  brightness = 255;
  dimDirection = -1;
}
else if (brightness <= 0) {
  brightness = 0;
  dimDirection = 1;
}
```

用 `dimDirection` 控制亮度变化方向。达到上限时改为减小，达到下限时改为增加，实现 0%→100%→0% 的循环效果。

## 7. 运行效果

上传程序后，打开串口监视器（波特率 115200），应该看到类似输出：

```
================================
ESP32 综合项目 P02：按键调光灯
================================
短按按键：开关 LED
长按按键：循环调节亮度 0%→100%→0%
按键按下
按键松开，按住时长：150 ms
短按切换，LED 状态：开
按键按下
按键松开，按住时长：2500 ms
亮度达到最大，开始减小
亮度达到最小，开始增加
```

实际现象：
- 快速按一下按键：LED 在开和关之间切换。
- 按住按键不松：LED 亮度从 0 慢慢变到 255，再从 255 慢慢变回 0，循环往复。
- 松开按键时：如果当前是亮的，保持当前亮度；如果是灭的，保持熄灭。

## 8. 常见问题

| 问题 | 可能原因 | 排查方法 |
|------|----------|----------|
| 短按没反应 | 按键接错引脚 | 检查是否接在 GPIO4 和 GND |
| 短按变成长按 | 长按阈值太小 | 增大 `LONG_PRESS_MS` |
| 按一下切换多次 | 消抖时间不足 | 增大 `DEBOUNCE_MS` |
| 长按调光太快 | 调光间隔太小 | 增大 `DIM_STEP_INTERVAL_MS` |
| LED 亮度变化不平滑 | 步进值太大 | 减小 `BRIGHTNESS_STEP` |
| 长按松开后又开关一次 | `longPressHandled` 逻辑异常 | 检查代码是否完整复制 |

## 9. 扩展挑战

1. **双击检测**：快速按两下实现某个快捷功能，比如直接跳到最亮或最暗。
2. **断电记忆**：用 Preferences 保存当前的开关状态和亮度，下次上电自动恢复。
3. **双按键版本**：一个按键负责开关，另一个负责调光，更符合日常台灯的使用习惯。
4. **OLED 显示**：实时显示当前亮度百分比和开关状态。
5. **RGB 调色**：把单色 LED 换成 RGB LED，长按调亮度的同时短按切换颜色。
