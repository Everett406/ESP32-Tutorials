# P03 串口命令控制器

## 1. 项目简介

本项目把 ESP32 变成一个可以通过电脑串口命令控制的设备。用户在 Arduino IDE 串口监视器中输入命令，ESP32 解析后执行对应操作：
- 控制 LED 的开关和亮度
- 读取按键状态
- 启动/停止呼吸灯效果
- 显示系统信息

通过本项目，你可以学习如何设计简单的串口命令协议、解析带参数的字符串，以及如何用非阻塞方式实现动画效果。

## 2. 涉及知识点

- [05_Serial_Communication](../ESP32_Tutorial_Series/05_Serial_Communication/)：串口收发字符串
- [02_Digital_Input_Button](../ESP32_Tutorial_Series/02_Digital_Input_Button/)：按键读取
- [03_PWM_LED_Fade](../ESP32_Tutorial_Series/03_PWM_LED_Fade/)：PWM 调光
- [04_ADC_Read_Analog](../ESP32_Tutorial_Series/04_ADC_Read_Analog/)：map() 和数值处理

## 3. 硬件清单

| 序号 | 器件 | 数量 | 说明 |
|------|------|------|------|
| 1 | ESP32 开发板 | 1 块 | 主控芯片 |
| 2 | LED | 1 个 | 被控制的灯 |
| 3 | 330Ω 电阻 | 1 个 | LED 限流电阻 |
| 4 | 按键 | 1 个 | 被读取的输入 |
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

## 5. 代码思路

程序的整体流程如下：

1. **初始化**：设置串口、LED、PWM、按键。
2. **循环检查串口**：如果 `Serial.available()` 大于 0，读取一行命令。
3. **预处理命令**：去掉首尾空白、转小写、忽略空命令。
4. **分发命令**：根据命令内容调用 `processCommand()` 执行对应操作。
5. **处理呼吸灯**：如果 `fadeMode` 为 true，每次 loop() 用 millis() 非阻塞更新亮度。
6. **输出反馈**：把执行结果打印回串口，方便用户确认。

## 6. 关键代码解析

### 6.1 串口命令读取

```cpp
String command = Serial.readStringUntil('\n');
command.trim();
command.toLowerCase();
```

- `readStringUntil('\n')`：读取到换行符为止，得到完整一行命令。
- `trim()`：去掉首尾空格和回车，避免用户多输入空格导致命令匹配失败。
- `toLowerCase()`：转小写，让 "ON"、"On"、"on" 都能被识别。

### 6.2 命令分发

```cpp
if (cmd == "on") { ... }
else if (cmd == "off") { ... }
else if (cmd == "read") { ... }
// ...
```

用 if-else if 结构按顺序判断命令。对于初学者来说，这种写法最直观。命令多了以后可以用状态机或函数指针表重构。

### 6.3 带参数命令解析

```cpp
else if (cmd.startsWith("duty ")) {
  int spaceIndex = cmd.indexOf(' ');
  String valueStr = cmd.substring(spaceIndex + 1);
  int duty = valueStr.toInt();
  // ...
}
```

- `startsWith("duty ")`：判断命令是否以 "duty " 开头。
- `indexOf(' ')`：找到第一个空格的位置。
- `substring(spaceIndex + 1)`：取出空格后面的数字字符串。
- `toInt()`：把字符串转换为整数。

### 6.4 非阻塞呼吸灯

```cpp
void updateFade() {
  if (millis() - lastFadeStep >= FADE_INTERVAL_MS) {
    lastFadeStep = millis();
    currentDuty += FADE_STEP * fadeDirection;
    // ...
    ledcWrite(LED_PIN, currentDuty);
  }
}
```

用 `millis()` 控制每次亮度改变的时间间隔，而不是 `delay()`。这样呼吸灯运行时，程序仍然可以读取串口命令，响应不会中断。

## 7. 运行效果

上传程序后，打开串口监视器（波特率 115200，发送模式设为"换行符"），应该看到启动信息：

```
================================
ESP32 综合项目 P03：串口命令控制器
================================
可用命令：
  on              - 点亮 LED
  off             - 熄灭 LED
  read            - 读取按键状态
  fade            - 启动/停止呼吸灯
  info            - 显示系统信息
  duty <0~255>    - 设置 PWM 占空比
  示例：duty 128
```

测试命令示例：

```
>>> on
LED 已点亮
>>> duty 64
PWM 占空比已设置为：64
>>> read
按键状态：松开
>>> fade
呼吸灯已启动，再次发送 fade 可停止
>>> fade
呼吸灯已停止
>>> info
--- 系统信息 ---
已执行命令数：6
LED 状态：开
当前 PWM 占空比：...
呼吸模式：已停止
芯片型号：ESP32-D0WD
CPU 频率：240 MHz
Flash 大小：4 MB
已运行时间：... 秒
```

## 8. 常见问题

| 问题 | 可能原因 | 排查方法 |
|------|----------|----------|
| 发送命令没反应 | 没有换行符 | 串口监视器发送模式设为 Newline 或 CRLF |
| 命令匹配失败 | 大小写/空格问题 | 本程序已做 trim 和 toLowerCase，检查是否复制完整 |
| "duty" 命令报错 | 数字超出范围 | 确保数字在 0~255 之间 |
| 呼吸灯运行时命令不响应 | 使用了 delay() | 检查是否使用了本程序的非阻塞实现 |
| 按键状态一直显示"松开" | 按键接错 | 检查按键是否接在 GPIO4 和 GND |
| info 命令某些字段为空 | 不同芯片型号差异 | ESP.getChipModel() 在某些板子上可能返回空字符串 |

## 9. 扩展挑战

1. **闪烁命令**：增加 "blink 5 300" 命令，让 LED 闪烁指定次数和间隔。
2. **JSON 输出**：增加 "status" 命令，以 JSON 格式返回当前所有状态。
3. **ADC 扩展**：增加 "adc" 命令，读取光敏电阻或其他模拟传感器的值。
4. **命令历史**：记录最近执行的几条命令，用 "history" 命令查看。
5. **无线升级**：把串口命令改成蓝牙串口命令，用手机或电脑无线控制 ESP32。
