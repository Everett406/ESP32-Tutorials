# P09_Smart_Watering_System

## 项目简介

本项目实现一个**自动浇花系统**。ESP32 通过土壤湿度传感器定时检测花盆土壤的干湿程度，当发现土壤偏干时，自动通过继电器打开水泵浇水 3 秒钟，浇完水后关闭水泵，等待下一次检测。这样即使主人出门几天，植物也能得到基本的水分补给。

如果没有水泵，也可以用水泵接口控制一个 LED，LED 亮起表示"正在浇水"，方便学习和测试。

## 涉及知识点

- ADC 模拟输入读取土壤湿度
- 继电器的工作原理与接线
- 硬件定时器中断（Timer Interrupt）
- ISR（中断服务函数）的设计原则
- ESP32 Arduino 3.x 定时器新 API：`timerBegin(freq)`、`timerAttachInterrupt(timer, &func)`、`timerAlarm()`

## 硬件清单

| 名称 | 数量 | 说明 |
|---|---|---|
| ESP32 开发板 | 1 | 推荐 ESP32 Dev Module |
| 土壤湿度传感器 | 1 | 输出模拟电压 AO |
| 继电器模块 | 1 | 低电平触发（5V/3.3V 兼容） |
| 水泵（或 LED） | 1 | 用于演示浇水动作 |
| 电源（USB 5V 或电池） | 1 | 为水泵供电 |
| 杜邦线 | 若干 | 连接电路 |

## 电路连接

### 土壤湿度传感器

| ESP32 | 连接 | 土壤湿度传感器 |
|---|---|---|
| 3.3V | → | VCC |
| GND | → | GND |
| GPIO34 | → | AO |

> **为什么选 GPIO34？**
> GPIO34 ~ GPIO39 是 ESP32 的"仅输入"引脚，内部没有数字输出电路的干扰，做 ADC 读取时噪声更小，读数更稳定。

### 继电器模块

| ESP32 | 连接 | 继电器模块 |
|---|---|---|
| 3.3V | → | VCC |
| GND | → | GND |
| GPIO18 | → | IN |

> **继电器触发方式说明：**
> 本代码按**低电平触发**继电器编写：给 IN 脚输出 LOW 时继电器吸合（接通），输出 HIGH 时释放（断开）。如果你的继电器是高电平触发，请把 `startWatering()` 和 `stopWatering()` 里的 `digitalWrite()` 高低电平反一下。

### 水泵（以 USB 5V 供电为例）

| 电源 / 设备 | 连接 | 继电器 / 设备 |
|---|---|---|
| USB 5V 正极 | → | 继电器 COM |
| 继电器 NO | → | 水泵正极 |
| 水泵负极 | → | USB 5V 负极 / GND |

> **为什么继电器要独立供电？**
> ESP32 的 GPIO 只能提供很小电流（约 12mA），而水泵启动时电流较大。继电器模块本身只需要几毫安驱动电流，由 ESP32 控制；水泵的电源则由外部 USB 或电池提供，两者在电气上是隔离的，这样既安全又不会拉低 ESP32 的电压导致复位。

## 代码思路

1. **初始化**：配置 GPIO、启动串口、初始化硬件定时器。
2. **定时器中断**：每 10 秒触发一次中断，中断服务函数只设置一个标志位 `check_flag`。
3. **主循环检测标志**：`loop()` 发现 `check_flag` 为真后，清除标志并读取土壤湿度 ADC 值。
4. **判断是否需要浇水**：如果 ADC 值高于 `DRY_THRESHOLD`，说明土壤偏干，调用 `startWatering()`。
5. **控制浇水时长**：`startWatering()` 记录开始时间，`loop()` 中不断用 `millis()` 检查是否已到 3 秒，到时间就 `stopWatering()`。

## 关键代码解析

### 1. ESP32 Arduino 3.x 定时器初始化

```cpp
check_timer = timerBegin(1000000);
timerAttachInterrupt(check_timer, &onTimer);
timerAlarm(check_timer, CHECK_INTERVAL_US, true, 0);
timerStart(check_timer);
```

- `timerBegin(1000000)`：指定定时器计数频率为 1MHz，即每 1 微秒计数一次。
- `timerAttachInterrupt(timer, &callback)`：绑定中断服务函数。注意 3.x 版本只有 2 个参数，不再像 2.x 那样有第三个 `edge` 参数。
- `timerAlarm(timer, count, autoreload, repeat_count)`：设置闹钟值。`CHECK_INTERVAL_US` 是 10,000,000，也就是 10 秒。`true` 表示自动重载，`0` 表示无限循环。
- `timerStart(timer)`：显式启动定时器。

> 旧版 ESP32 Arduino 2.x 的写法是 `timerBegin(timer, divider, countUp)` + `timerAttachInterrupt(timer, func, edge)` + `timerAlarmWrite(timer, count, autoreload)` + `timerAlarmEnable(timer)`，在 3.x 中已废弃。

### 2. 中断里只设置标志位

```cpp
void IRAM_ATTR onTimer() {
  portENTER_CRITICAL_ISR(&timer_mux);
  check_flag = true;
  portEXIT_CRITICAL_ISR(&timer_mux);
}
```

`IRAM_ATTR` 告诉编译器把这个函数放在 IRAM（内部高速内存）中，这样即使 Flash 正在做其他事，中断也能立刻响应。

ISR 里不做读取传感器、控制继电器等耗时操作，只快速修改一个标志位。真正的业务逻辑放在 `loop()` 里执行，避免中断执行时间过长导致系统不稳定。

### 3. 临界区保护共享变量

```cpp
portENTER_CRITICAL(&timer_mux);
check_flag = false;
portEXIT_CRITICAL(&timer_mux);
```

`check_flag` 同时被中断服务函数和主循环访问。进入临界区可以临时关闭中断，防止在修改标志的过程中中断又把它改回 `true`，造成标志丢失。

### 4. 非阻塞式浇水计时

```cpp
if (now - watering_start_time >= WATERING_DURATION_MS) {
  stopWatering();
}
```

用 `millis()` 记录浇水开始时间，主循环中不断检查是否已过 3 秒。这样做的好处是不会阻塞程序，土壤湿度的定时检查仍然可以继续进行。

## 运行效果

1. 上传程序，打开串口监视器，波特率 **115200**。
2. 看到提示：
   ```
   智能浇花系统已启动
   每 10 秒检查一次土壤湿度
   干燥阈值：2500
   如果土壤干燥，将自动浇水 3 秒
   ```
3. 系统每 10 秒打印一次土壤湿度 ADC 值，例如：
   ```
   土壤湿度 ADC 值：3200
   土壤偏干，开始浇水
   水泵已开启
   浇水结束，水泵已关闭
   ```
   或者：
   ```
   土壤湿度 ADC 值：1800
   土壤湿度正常，无需浇水
   ```
4. 如果土壤 ADC 值高于 2500，继电器吸合，水泵运行 3 秒后停止。

## 常见问题

### 1. 土壤湿度读数不稳定

- 在传感器 AO 引脚和 GND 之间加一个 **100nF 陶瓷电容**，滤除高频噪声。
- 确保传感器电源稳定，GND 接线牢固。
- 多取几次读数再平均，而不是只读一次就判断。

### 2. 继电器不吸合或水泵不转

- 确认继电器是低电平触发还是高电平触发，必要时修改代码中的高低电平。
- 确认继电器 VCC 接到了 3.3V 或 5V（看模块要求），GND 接好。
- 确认水泵有独立电源，不要指望 ESP32 的 GPIO 直接驱动。
- 用万用表测量继电器 COM 和 NO 之间是否导通，判断继电器是否动作。

### 3. 编译报错：timerBegin 参数不匹配

- 确认你安装的是 **ESP32 Arduino 3.x 核心**。
- 本代码使用 3.x 新 API：`timerBegin(freq)`、`timerAttachInterrupt(timer, &func)`。
- 如果你用的是 2.x 核心，需要改用旧 API，或者升级到 3.x。

### 4. 浇水过于频繁或从不浇水

- 传感器的"干燥"和"湿润"读数因品牌和土壤而异。先把传感器插进土里，分别记录"很干"和"很湿"时的 ADC 值。
- 根据实际读数调整 `DRY_THRESHOLD`，一般取中间偏干一点的值。

### 5. 土壤湿度传感器探针生锈

- 土壤湿度传感器的探针长期通电会电解腐蚀。进阶做法是在测量前给传感器 VCC 供电，测完立即断电，只在需要时通电。

## 扩展挑战

1. **手动浇水按钮**：增加一个物理按键，按下后立即浇水一次，不受 10 秒定时器限制。
2. **OLED 状态显示**：接一块 0.96 寸 OLED，实时显示当前湿度、阈值、浇水次数、系统运行时间等信息。
3. **联网远程监控**：加入 Wi-Fi 和 Web 服务器，手机浏览器可以查看当前土壤湿度、修改干燥阈值、手动远程浇水。
