/*
 * ESP32 教程系列 08：蓝牙串口 - 用手机 APP 控制 LED
 * 
 * 学习目标：
 * 1. 理解什么是蓝牙、经典蓝牙、BLE（低功耗蓝牙）和 SPP
 * 2. 学会使用 BluetoothSerial.h 建立经典蓝牙串口
 * 3. 学会通过手机蓝牙串口 APP 向 ESP32 发送命令
 * 4. 学会在 ESP32 里解析字符串命令并执行对应动作
 * 
 * 前置知识：
 * - 已完成教程 06《Wi-Fi 连接》，了解 ESP32 的无线外设
 * - 已完成教程 05《串口通信》，熟悉 Serial.readStringUntil() 和字符串基本操作
 * 
 * 硬件连接：
 * - ESP32 GPIO2  →  LED 正极（长脚）
 * - LED 负极（短脚） →  330Ω 电阻  →  GND
 * - 本示例主要依靠蓝牙通信，不需要连接手机数据线
 * 
 * 使用步骤：
 * 1. 把程序上传到 ESP32，打开串口监视器（波特率 115200）。
 * 2. 在手机上安装蓝牙串口 APP：
 *    - Android：Serial Bluetooth Terminal、蓝牙串口助手、BlueTerm 等
 *    - iOS：由于苹果限制，经典蓝牙 SPP 不被支持，建议使用 BLE 方案
 * 3. 打开手机蓝牙，搜索并连接名为“ESP32_LED”的设备。
 * 4. 在 APP 中发送 on、off 或 blink 命令。
 * 5. ESP32 会通过蓝牙返回执行结果，同时串口监视器也会打印调试信息。
 * 
 * 关键概念解释：
 * - 蓝牙（Bluetooth）：一种短距离无线通信技术，工作频段主要是 2.4GHz。
 *   常用于手机、耳机、键盘、音箱等设备之间的数据传输。
 * - 经典蓝牙（Classic Bluetooth，又称 BR/EDR）：传输速率较快，适合持续传输
 *   音频、文件等较大量数据，功耗相对较高。ESP32 支持经典蓝牙。
 * - BLE（Bluetooth Low Energy，低功耗蓝牙）：专为低功耗设计，传输速率较慢，
 *   但省电，适合传感器、手环等电池供电设备。ESP32 也支持 BLE。
 * - SPP（Serial Port Profile，串口配置文件）：经典蓝牙里的一种协议，
 *   作用是把蓝牙虚拟成一个“无线串口”。手机和 ESP32 之间可以像用 USB 串口一样
 *   收发文本数据，对初学者最友好。
 * - 蓝牙设备名称（Device Name）：手机搜索蓝牙时看到的名字，比如本示例的
 *   "ESP32_LED"。这个名字可以自定义，方便识别。
 * - 配对（Pairing）：手机和蓝牙设备之间建立信任关系的过程，通常需要确认 PIN 码。
 *   配对一次后，下次会自动连接。
 * - RSSI（Received Signal Strength Indicator）：蓝牙信号强度，和 Wi-Fi 的 RSSI
 *   含义类似，越接近 0 表示信号越好。
 * - 射频（Radio Frequency，RF）：无线信号的发射和接收部分。ESP32 的 Wi-Fi 和
 *   蓝牙共用同一个 2.4GHz 射频硬件，因此可以同时开启，但可能会互相影响性能。
 * 
 * 为什么用经典蓝牙而不是 BLE？
 * 经典蓝牙 SPP 对新手最友好：手机 APP 直接把蓝牙当串口用，发送字符串、
 * 接收字符串，和 Serial 的用法几乎一样。BLE 更省电，但涉及服务（Service）、
 * 特征值（Characteristic）、GATT 等概念，学习曲线更陡，后续扩展教程会涉及。
 */

// 引入经典蓝牙串口库。这个库是 ESP32 Arduino 3.x 自带的，
// 封装了蓝牙协议栈的复杂细节，让我们可以像操作 Serial 一样收发数据。
#include "BluetoothSerial.h"

// 创建蓝牙串口对象，名字可以自取，这里叫 SerialBT。
// 之后我们用 SerialBT.begin() 初始化它，用 SerialBT.available()、
// SerialBT.readStringUntil() 等方法收发数据。
BluetoothSerial SerialBT;

// 蓝牙设备名称。手机搜索蓝牙时会看到这个名称。
// 建议取一个有意义且唯一的名字，避免和周围其他 ESP32 混淆。
#define BT_DEVICE_NAME "ESP32_LED"

// LED 引脚。GPIO2 是大多数 ESP32 开发板板载 LED 所在的引脚。
#define LED_PIN 2

void setup() {
  // 普通串口用于调试信息输出到电脑，波特率 115200。
  // 注意：蓝牙串口和 USB 串口是两个独立的东西，不要混淆。
  Serial.begin(115200);
  delay(1000);
  Serial.println("================================");
  Serial.println("ESP32 教程 08：蓝牙串口控制 LED");
  Serial.println("================================");

  // 初始化 LED 引脚为输出模式，并默认熄灭。
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // 启动蓝牙串口，并设置设备名称。
  // SerialBT.begin(name) 返回 bool：true 表示启动成功，false 表示失败。
  // 失败可能的原因：ESP32 蓝牙初始化异常、名称太长、内存不足等。
  if (!SerialBT.begin(BT_DEVICE_NAME)) {
    Serial.println("蓝牙启动失败！");
  } else {
    Serial.print("蓝牙已启动，设备名称：");
    Serial.println(BT_DEVICE_NAME);
    Serial.println("请用手机蓝牙串口 APP 连接此设备");
  }

  // 在串口和蓝牙同时打印可用命令提示，方便用户知道怎么操作。
  Serial.println("发送 'on' 开灯，'off' 关灯，'blink' 闪烁");
  SerialBT.println("发送 'on' 开灯，'off' 关灯，'blink' 闪烁");
}

void loop() {
  // SerialBT.available() 检查蓝牙串口接收缓冲区里有没有数据。
  // 返回大于 0 表示收到了数据，可以读取。
  // 为什么要先检查？因为如果没有数据就读取，会读到空字符串或乱码。
  if (SerialBT.available()) {
    // readStringUntil('\n') 会一直读取字符，直到遇到换行符 \n 为止。
    // 手机 APP 通常会在发送内容末尾自动加上换行，这样一条命令就是一行。
    String command = SerialBT.readStringUntil('\n');

    // trim() 去掉字符串开头和结尾的空白字符，包括空格、回车、换行。
    // 不同 APP 发送的换行符可能不同，trim 可以防止这些隐藏字符影响命令判断。
    command.trim();

    // toLowerCase() 把命令全部转成小写，这样用户输入 ON、On、on 都会被识别。
    // 对新手来说，大小写不敏感更友好。
    command.toLowerCase();

    // 把收到的命令同时打印到 USB 串口，方便在电脑上调试。
    // 这是嵌入式开发常用的技巧：保留一个“调试串口”观察程序运行状态。
    Serial.print("蓝牙收到：");
    Serial.println(command);

    // 根据命令执行对应动作。
    if (command == "on") {
      digitalWrite(LED_PIN, HIGH);
      Serial.println("LED 已点亮");
      SerialBT.println("LED 已点亮");
    }
    else if (command == "off") {
      digitalWrite(LED_PIN, LOW);
      Serial.println("LED 已熄灭");
      SerialBT.println("LED 已熄灭");
    }
    else if (command == "blink") {
      Serial.println("开始闪烁 5 次");
      SerialBT.println("开始闪烁 5 次");
      // for 循环让 LED 闪烁 5 次。注意闪烁期间 loop() 被 delay 阻塞，
      // 所以在闪烁过程中不会响应新的蓝牙命令。如果希望实时响应，
      // 需要用非阻塞方式实现（参考教程 10 的定时器思想）。
      for (int i = 0; i < 5; i++) {
        digitalWrite(LED_PIN, HIGH);
        delay(200);
        digitalWrite(LED_PIN, LOW);
        delay(200);
      }
      Serial.println("闪烁完成");
      SerialBT.println("闪烁完成");
    }
    else if (command.length() > 0) {
      // command.length() > 0 过滤空命令。有些 APP 会发送空行，
      // 如果不判断就直接回复“未知命令”，体验不好。
      Serial.print("未知命令：");
      Serial.println(command);
      SerialBT.print("未知命令：");
      SerialBT.println(command);
    }
  }

  // 也可以把 USB 串口的数据转发到蓝牙串口。
  // 这样电脑和手机可以双向通信：电脑 Serial 里输入的内容会出现在手机上，
  // 手机发来的内容也会显示在电脑 Serial 里。
  if (Serial.available()) {
    String msg = Serial.readStringUntil('\n');
    SerialBT.print("来自电脑：");
    SerialBT.println(msg);
  }
}

/*
 * 故障排查：
 * 1. 手机搜索不到“ESP32_LED”：
 *    - 确认 ESP32 已经上电，串口监视器有“蓝牙已启动”输出。
 *    - 确认手机蓝牙已经打开，并且允许发现新设备。
 *    - 某些 Android 版本需要授予 APP 定位权限才能扫描蓝牙设备。
 * 2. 连接上但发送命令没有反应：
 *    - 检查 APP 是否连接的是 SPP 串口服务，而不是 BLE 服务。
 *    - 观察串口监视器是否有“蓝牙收到：xxx”输出，确认数据是否到达。
 *    - 确认命令末尾是否发送了换行符。如果没有换行，readStringUntil('\n')
 *      会一直等待，导致命令不执行。
 * 3. iPhone 无法连接：
 *    - 经典蓝牙 SPP 不被 iOS 支持，需要使用 BLE 方案。
 * 4. 蓝牙和 Wi-Fi 同时使用时不稳定：
 *    - ESP32 的蓝牙和 Wi-Fi 共用射频，同时高强度使用可能互相干扰。
 *    - 如果只需要其中一种，可以只开启一种以节省资源。
 * 
 * 扩展练习：
 * 1. 发送 "duty 128" 命令，用空格分割命令和参数，控制 PWM 亮度。
 *    提示：用 command.indexOf(' ') 找到空格位置，再分别取出命令和数字。
 * 2. 添加蓝牙配对密码：SerialBT.setPin("1234")，提高安全性。
 * 3. 改用 BLE（低功耗蓝牙）实现，让 iOS 设备也能连接。
 * 4. 增加一条 "status" 命令，让 ESP32 返回当前 LED 状态。
 */
