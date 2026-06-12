# ESP32 综合项目合集

这一组项目是把前面 `ESP32_Tutorial_Series`（Arduino 教程）和 `ESP32_C_Tutorial`（C 语言教程）中学到的技能组合起来，从简单到复杂，逐步做出有实际用途的物联网设备。

## 学习路线

建议按编号顺序完成。每个项目都会用到前面已经学过的知识点，并在其基础上增加新的复杂度。

### 第一阶段：两个技能组合

| 项目 | 名称 | 组合技能 | 难度 |
|---|---|---|---|
| P01 | [智能小夜灯](./P01_Smart_Night_Light) | ADC + PWM | ⭐ |
| P02 | [按键调光台灯](./P02_Button_Dimming_Lamp) | 数字输入 + PWM | ⭐ |
| P03 | [串口命令控制器](./P03_Serial_Command_Controller) | 串口通信 + GPIO | ⭐ |

### 第二阶段：三个技能组合

| 项目 | 名称 | 组合技能 | 难度 |
|---|---|---|---|
| P04 | [温湿度报警器](./P04_Temperature_Humidity_Alarm) | DHT22 + 报警输出 + 串口 | ⭐⭐ |
| P05 | [超声波测距仪](./P05_Ultrasonic_Distance_Meter) | HC-SR04 + OLED + 蜂鸣器 | ⭐⭐ |
| P06 | [Wi-Fi 状态显示器](./P06_WiFi_Status_Display) | Wi-Fi + OLED | ⭐⭐ |
| P07 | [蓝牙遥控灯](./P07_Bluetooth_Remote_Light) | 蓝牙串口 + PWM | ⭐⭐ |

### 第三阶段：加入网络服务器

| 项目 | 名称 | 组合技能 | 难度 |
|---|---|---|---|
| P08 | [网页温湿度监控](./P08_Web_Temperature_Monitor) | DHT22 + Web Server + Wi-Fi | ⭐⭐⭐ |
| P09 | [智能浇花系统](./P09_Smart_Watering_System) | 土壤湿度 + 继电器 + 定时器 | ⭐⭐⭐ |
| P10 | [触摸 + Web 双控灯](./P10_Touch_Web_Dual_Control_Light) | 触摸 + PWM + Web + mDNS | ⭐⭐⭐ |

### 第四阶段：系统级项目

| 项目 | 名称 | 组合技能 | 难度 |
|---|---|---|---|
| P11 | [智能家居中控](./P11_Smart_Home_Center) | 多传感器 + OLED + Web + Preferences | ⭐⭐⭐⭐ |
| P12 | [MQTT 气象站](./P12_MQTT_Weather_Station) | 多传感器 + MQTT + OLED | ⭐⭐⭐⭐ |
| P13 | [数据记录器](./P13_Data_Logger) | 传感器 + LittleFS + Web | ⭐⭐⭐⭐ |
| P14 | [低功耗环境节点](./P14_Low_Power_Environment_Node) | 传感器 + Deep Sleep + HTTP | ⭐⭐⭐⭐ |
| P15 | [完整智能家居节点](./P15_Complete_Smart_Home_Node) | 全技能综合 | ⭐⭐⭐⭐⭐ |

## 如何使用

1. 先完成对应的基础教程（例如做 P01 前先学 03、04 课）
2. 按项目 README 准备硬件并接线
3. 打开 `.ino` 文件，修改 Wi-Fi 名称密码等配置
4. 上传代码，观察运行效果
5. 完成 README 中的扩展挑战

## 所需硬件汇总

不同项目需要的硬件不同，建议按阶段准备：

**基础套件**：ESP32 开发板、面包板、杜邦线、LED、330Ω 电阻、按键、光敏电阻、10kΩ 电阻

**进阶套件**：DHT22 温湿度传感器、HC-SR04 超声波模块、SSD1306 OLED、继电器模块、土壤湿度传感器、有源蜂鸣器

**高级套件**：水泵（配合继电器）、各种传感器扩展模块

## 提示

- 每个项目目录下都有独立的 `README.md` 详细说明
- 项目代码使用 ESP32 Arduino 3.x API
- 所有项目都包含大量中文注释，解释"为什么这样写"
