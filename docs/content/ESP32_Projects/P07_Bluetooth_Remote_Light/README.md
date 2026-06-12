# P07_Bluetooth_Remote_Light

## 项目简介

本项目教你用**手机蓝牙**无线控制 ESP32 上的 LED 灯。手机通过蓝牙串口协议（SPP）向 ESP32 发送文字命令，例如 `on`、`off`、`duty 100`、`blink`，ESP32 收到后控制 LED 的亮灭、亮度或闪烁模式。

这是一个非常典型的"手机 ↔ 蓝牙 ↔ 单片机"物联网入门场景。学会之后，你可以把手机当成一个无线遥控器，控制家里的灯、风扇或其他小电器。

## 涉及知识点

- 经典蓝牙串口（BluetoothSerial / SPP）
- PWM（脉宽调制）控制 LED 亮度
- 字符串命令解析
- 非阻塞式闪烁（用 `millis()` 代替 `delay()`）
- ESP32 Arduino 3.x 的 `ledcAttach()` / `ledcWrite()` 新 API

## 硬件清单

| 名称 | 数量 | 说明 |
|---|---|---|
| ESP32 开发板 | 1 | 推荐使用 ESP32 Dev Module |
| LED | 1 | 任意颜色，5mm 或 3mm 均可 |
| 330Ω 电阻 | 1 | 限制 LED 电流，防止烧坏 |
| 安卓手机 | 1 | 安装蓝牙串口 APP（iOS 对经典蓝牙支持有限） |
| 杜邦线 | 若干 | 连接电路 |

## 电路连接

ESP32 通过 PWM 控制 LED 亮度，接线如下：

| ESP32 | 连接 | 元器件 |
|---|---|---|
| GPIO2 | → | LED 正极（长脚） |
| GND | → | 330Ω 电阻 → LED 负极（短脚） |

> **为什么选 GPIO2？**
> 大多数 ESP32 开发板已经把 GPIO2 接到板载 LED 上，即使没有外接 LED，也能看到效果。
>
> **为什么要加 330Ω 电阻？**
> LED 导通后内阻很小，如果直接接 3.3V，电流会过大导致 LED 烧毁。330Ω 电阻把电流限制在约 10mA，既安全又足够亮。

## 代码思路

1. **初始化**：启动串口、绑定 LED 引脚到 PWM、启动蓝牙并设置设备名称。
2. **等待连接**：手机打开蓝牙串口 APP，搜索名为 `ESP32_Remote_Light` 的设备并连接。
3. **接收命令**：ESP32 通过 `SerialBT.read()` 逐个字符读取，遇到换行符就把整条命令交给 `processCommand()` 处理。
4. **执行命令**：
   - `on`：点亮 LED，亮度恢复为上一次设置的值（默认 255）。
   - `off`：熄灭 LED。
   - `duty XXX`：设置 PWM 占空比，范围 0 ~ 255，数值越大越亮。
   - `blink`：让 LED 以 500ms 间隔闪烁。
5. **闪烁模式**：用 `millis()` 做非阻塞定时，即使 LED 在闪烁，也不会影响蓝牙命令的实时接收。

## 关键代码解析

### 1. 启动蓝牙串口

```cpp
BluetoothSerial SerialBT;
SerialBT.begin("ESP32_Remote_Light");
```

`BluetoothSerial` 把 ESP32 变成一个蓝牙串口设备。手机连接后，双方就像用 USB 串口一样互相发送文字。

### 2. ESP32 Arduino 3.x 的 PWM 写法

```cpp
ledcAttach(LED_PIN, PWM_FREQUENCY, PWM_RESOLUTION);
ledcWrite(LED_PIN, current_duty);
```

- `ledcAttach(pin, freq, resolution)`：自动分配 PWM 通道并绑定引脚。
- `ledcWrite(pin, duty)`：设置占空比。

> 旧版的 `ledcSetup()` + `ledcAttachPin()` 在 ESP32 Arduino 3.x 中已被废弃，新写法更简洁。

### 3. 命令解析

```cpp
if (cmd.startsWith("duty ")) {
  String value_str = cmd.substring(5);
  int value = value_str.toInt();
  // ...
}
```

先用 `startsWith()` 判断命令类型，再用 `substring()` 提取参数，最后用 `toInt()` 把字符串转成数字。这种写法简单直观，适合命令种类不多的项目。

### 4. 非阻塞闪烁

```cpp
if (now - last_blink_time >= BLINK_INTERVAL) {
  last_blink_time = now;
  // 切换 LED 状态
}
```

如果闪烁使用 `delay(500)`，在 delay 期间 `loop()` 会暂停，手机发来的命令就会被忽略。用 `millis()` 做非阻塞定时，可以在闪烁的同时持续处理蓝牙数据。

## 运行效果

1. 上传程序后，打开 Arduino IDE 串口监视器，波特率 **115200**。
2. 看到提示：
   ```
   蓝牙遥控灯已启动
   设备名称：ESP32_Remote_Light
   等待手机连接...
   可用命令：on / off / duty 0~255 / blink
   ```
3. 手机打开蓝牙串口 APP，搜索并连接 `ESP32_Remote_Light`。
4. 在 APP 中输入命令并发送：
   - 发送 `on`：LED 最亮。
   - 发送 `duty 50`：LED 变暗。
   - 发送 `duty 0`：LED 熄灭。
   - 发送 `blink`：LED 开始闪烁。
   - 发送 `off`：停止闪烁并熄灭 LED。
5. 每发送一条命令，ESP32 都会把命令回显到手机 APP，方便确认。

## 常见问题

### 1. 手机搜索不到 ESP32

- 确认你的手机是**安卓系统**。iOS 对经典蓝牙 SPP 支持很差，建议使用 BLE 项目。
- 确认手机蓝牙已开启，并在 APP 中主动搜索设备。
- 确认 ESP32 已经上电并运行程序（看串口监视器是否有输出）。

### 2. 能连接但发送命令没反应

- 检查 APP 是否发送了换行符。本程序以 `\n` 或 `\r` 作为命令结束标志。
- 检查命令是否有额外空格或大小写问题。程序会自动转小写并去除首尾空格，但有些 APP 可能发送不可见字符。
- 打开串口监视器，看是否收到"收到命令：xxx"的调试信息。

### 3. LED 不亮

- 检查 LED 正负极是否接反，长脚为正极。
- 检查电阻是否串联在 LED 负极和 GND 之间。
- 尝试不插 LED，直接观察开发板上的板载 LED（通常也是 GPIO2）。

### 4. 编译报错：找不到 BluetoothSerial.h

- 确认你选择的开发板是 **ESP32 Dev Module** 或类似 ESP32 开发板。
- `BluetoothSerial.h` 是 ESP32 核心自带库，不需要额外安装。

## 扩展挑战

1. **呼吸灯命令**：增加一个 `fade` 命令，让 LED 在 0% ~ 100% 之间平滑呼吸，收到 `on`/`off` 后停止。
2. **多路灯控制**：用 3 个 GPIO 分别控制红、绿、蓝三个 LED，实现命令 `red on`、`green duty 100`、`blue blink` 等。
3. **加入密码保护**：首次连接时要求发送密码（如 `pass 1234`），验证通过后才执行控制命令，防止陌生人随意控制。
