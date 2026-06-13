# ESP32 学习教程合集

[![GitHub Pages](https://img.shields.io/badge/GitHub%20Pages-在线阅读-blue)](https://everett406.github.io/ESP32-Tutorials/)

📖 **在线阅读地址：https://everett406.github.io/ESP32-Tutorials/**

这个仓库收录了三套 ESP32 学习资料：

1. **ESP32_Tutorial_Series**：用 Arduino IDE 快速上手的示例教程
2. **ESP32_C_Tutorial**：系统学习 C 语言基础，结合 ESP32 硬件实践
3. **ESP32_Projects**：综合项目，把前面学到的技能组合起来，从简单到复杂

## 目录

```
ESP32-Tutorials/
├── ESP32_Tutorial_Series/     # Arduino IDE 入门教程
│   ├── 01_Digital_Output_Blink/
│   ├── 02_Digital_Input_Button/
│   ├── ...
│   ├── extensions/             # 网络与无线扩展项目
│   ├── peripherals/            # 外设项目
│   └── README.md
│
├── ESP32_C_Tutorial/          # C 语言基础教程
│   ├── C01_Hello_World/
│   ├── C02_Variables_Types/
│   ├── ...
│   ├── C16_Code_Variations_and_Traps/
│   └── README.md
│
└── ESP32_Projects/            # 综合项目（由易到难）
    ├── P01_Smart_Night_Light/
    ├── P02_Button_Dimming_Lamp/
    ├── ...
    ├── P15_Complete_Smart_Home_Node/
    └── README.md
```

## 适用人群

- 零基础想学 ESP32 的初学者
- 想从 Arduino 进阶到 C 语言底层的开发者
- 需要一套完整、注释详细的 ESP32 参考资料的人

## 两套教程的区别

| 教程 | 侧重点 | 前置要求 |
|---|---|---|
| ESP32_Tutorial_Series | 用 Arduino 方式快速做出东西 | 安装 Arduino IDE + ESP32 开发板支持 |
| ESP32_C_Tutorial | 理解 C 语言本身和底层原理 | 同上，建议先过一遍 Arduino 教程 |

## 快速开始

1. 安装 [Arduino IDE](https://www.arduino.cc/en/software)
2. 在 Arduino IDE 中添加 ESP32 开发板支持包
3. 用 USB 线连接 ESP32 开发板
4. 打开任意教程目录下的 `.ino` 文件
5. 选择开发板 `ESP32 Dev Module`，选择正确的 COM 口，上传代码
6. 打开串口监视器，波特率设置为 **115200**

## 学习建议

- 如果你完全没接触过 ESP32，建议从 `ESP32_Tutorial_Series/01_Digital_Output_Blink` 开始
- 如果你想系统学习 C 语言，可以并行学习 `ESP32_C_Tutorial/C01_Hello_World`
- 每课都包含大量中文注释、写法变体、常见陷阱和课后练习，建议边读边动手做
- 学完基础教程后，进入 `ESP32_Projects/` 做综合项目，把零散技能串成完整作品

## 硬件准备

### 基础教程需要
- ESP32 开发板一块
- USB 数据线一根
- 面包板、杜邦线若干
- LED、330Ω 电阻、按键

### 外设项目需要（按需购买）
- DHT11/DHT22 温湿度传感器
- HC-SR04 超声波测距模块
- SG90 舵机
- 继电器模块
- SSD1306 OLED 显示屏
- WS2812B 彩色灯带
- PIR 人体红外传感器
- 光敏电阻
- 有源蜂鸣器
- 土壤湿度传感器

## 版权声明

本教程为个人学习资料，欢迎自由使用、修改和分享。
