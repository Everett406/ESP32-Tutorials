/*
 * ESP32 教程系列 13：Preferences 存储 —— 断电也不丢失的数据
 * 
 * 学习目标：
 * 1. 理解什么是"非易失性存储"（Non-Volatile Storage，NVS）
 * 2. 学会使用 Preferences.h 库读写 Flash 中的键值对
 * 3. 实现"重启后恢复上次状态"的效果
 * 4. 理解 Flash 的写入寿命限制，学会避免频繁写入
 * 
 * 前置知识：
 * - 已经会用 digitalWrite() 控制 LED
 * - 已经会用 digitalRead() 读取按键
 * - 理解 INPUT_PULLUP 上拉输入
 * 
 * 硬件连接：
 * - ESP32 GPIO2  →  LED 正极（长脚）
 * - LED 负极（短脚） →  330Ω 电阻  →  GND
 * - ESP32 GPIO4  →  按键一端
 * - 按键另一端    →  GND
 * 
 * 为什么用 INPUT_PULLUP？
 * 因为按键另一端接 GND，按下时 GPIO4 被拉到低电平。
 * 没有按下时，ESP32 内部上拉电阻把 GPIO4 维持在高电平。
 * 这样我们只需要一个按键，不需要额外接电阻。
 */

// 包含 Preferences 库。这个库是 ESP32 Arduino 核心自带的，不需要额外安装。
// Preferences 库封装了 ESP32 的 NVS（Non-Volatile Storage，非易失性存储）。
#include <Preferences.h>

#define LED_PIN 2       // LED 引脚
#define BUTTON_PIN 4    // 按键引脚

// 创建一个 Preferences 对象，用来和 Flash 交互。
// 你可以把它理解为一个"小数据库客户端"。
Preferences prefs;

// 命名空间（Namespace）：相当于在 Flash 里建一个文件夹，
// 把相关的配置项放在同一个文件夹下，避免和其他程序的键名冲突。
// 注意：命名空间名字长度不能超过 15 个字符。
const char* namespaceName = "myConfig";

// 键名（Key）：用来标识我们要保存的某一项数据。
// 这里我们要保存 LED 的状态，所以叫 "ledState"。
const char* keyLedState = "ledState";

// ledState 保存当前 LED 的开关状态。
// true 表示点亮，false 表示熄灭。
bool ledState = false;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("================================");
  Serial.println("ESP32 教程 13：Preferences 存储");
  Serial.println("================================");

  // 初始化 LED 引脚为输出模式
  pinMode(LED_PIN, OUTPUT);

  // 初始化按键引脚为上拉输入模式
  // INPUT_PULLUP 表示启用 ESP32 内部上拉电阻。
  // 为什么用上拉？因为按键另一端接 GND，
  // 没按的时候 GPIO4 是高电平，按下的时候是低电平，
  // 程序里判断 LOW 就知道按键被按下了。
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // prefs.begin() 打开一个命名空间，准备读写。
  // 第二个参数表示打开模式：
  // - false：只读模式（read-only）。如果命名空间不存在会打开失败。
  // - true：  读写模式（read-write）。如果命名空间不存在会自动创建。
  //
  // 为什么这里先用只读模式？
  // 因为启动时我们只是"读取"上次保存的状态，不需要写。
  // 用只读模式更安全，也符合"最小权限"原则。
  prefs.begin(namespaceName, false);

  // getBool(key, defaultValue) 从 Flash 中读取一个布尔值。
  // - 如果 key 存在，返回保存的值；
  // - 如果 key 不存在（例如第一次烧录），返回 defaultValue。
  //
  // 这里默认值设为 false，表示第一次运行时 LED 默认是熄灭的。
  ledState = prefs.getBool(keyLedState, false);

  // 把读到的状态应用到 LED 上
  digitalWrite(LED_PIN, ledState ? HIGH : LOW);

  Serial.print("从 Flash 读取的 LED 状态：");
  Serial.println(ledState ? "点亮" : "熄灭");
  Serial.println("按下按键可切换状态，切换后会自动保存");

  // prefs.end() 关闭命名空间，释放资源。
  // 用完就关是个好习惯，可以避免后面误操作或冲突。
  prefs.end();
}

void loop() {
  // 读取按键状态。因为用了 INPUT_PULLUP，按下时为 LOW。
  if (digitalRead(BUTTON_PIN) == LOW) {
    // 按键按下，切换 LED 状态
    ledState = !ledState;

    // 立即更新 LED 实际状态，给用户即时反馈
    digitalWrite(LED_PIN, ledState ? HIGH : LOW);

    // 需要保存新状态了，所以用读写模式重新打开命名空间
    prefs.begin(namespaceName, true);

    // putBool(key, value) 把布尔值写入 Flash。
    // 注意：写入 Flash 需要一定时间，也比读写内存慢得多。
    prefs.putBool(keyLedState, ledState);

    // 写入完成后关闭命名空间
    prefs.end();

    Serial.print("状态已保存到 Flash：");
    Serial.println(ledState ? "点亮" : "熄灭");

    // 等待按键释放，避免一次按下触发多次保存。
    // 这段代码叫"按键消抖 + 等待释放"。
    delay(300);  // 先延时消抖
    while (digitalRead(BUTTON_PIN) == LOW) {
      delay(10);  // 一直等到按键松开
    }
    delay(300);  // 松手后再延时消抖
  }
}

/*
 * 重要概念解释：
 * 
 * 1. 什么是非易失性存储（NVS）？
 *    "非易失性"（Non-Volatile）指的是断电后数据不会消失。
 *    ESP32 的 Flash 芯片就是非易失性存储。
 *    NVS 是 ESP-IDF 提供的一套键值对存储机制，
 *    Preferences 库就是 Arduino 对 NVS 的封装。
 *    键值对的意思是：每个数据有一个"键"（名字）和一个"值"，
 *    就像字典一样，通过名字就能找到对应的值。
 * 
 * 2. 为什么不用 Arduino 的 EEPROM 库？
 *    ESP32 其实没有真正的 EEPROM，它的 EEPROM 库是用 Flash 模拟出来的，
 *    使用起来比分区寻址更麻烦，而且不是 ESP32 官方推荐的方式。
 *    Preferences 库用键值对的方式，更符合现代嵌入式开发习惯。
 * 
 * 3. Flash 写入寿命：
 *    Flash 存储器的每个存储单元都有擦写次数限制，
 *    通常约为 10 万次。
 *    这意味着如果你每秒写一次，一年多就把寿命用完了。
 *    所以不要把快速变化的值（比如传感器实时读数）频繁写入 Flash。
 *    本示例只在按键按下时才写一次，写入频率很低，完全没问题。
 * 
 * 4. 命名空间和键名的作用：
 *    命名空间把不同用途的数据隔开，
 *    例如一个程序可以用 "wifi" 存 Wi-Fi 配置，用 "user" 存用户设置。
 *    键名是同一命名空间下不同数据项的名字，
 *    例如 "wifi" 命名空间里可以有 "ssid" 和 "password" 两个键。
 */

/*
 * 排查提示：
 * 1. 如果重启后状态没有保留：
 *    - 检查是否调用了 prefs.end()，如果没有关闭可能数据没提交。
 *    - 检查命名空间和键名是否前后一致，大小写也要一样。
 *    - 检查是不是每次都重新烧录了程序并清空了 Flash。
 *
 * 2. 如果串口显示 "[Preferences] 命名空间未找到" 之类的错误：
 *    - 这是正常的，第一次运行时确实没有数据，程序会用默认值。
 *
 * 3. 如果按键反应不灵敏：
 *    - 检查按键接线是否正确。
 *    - 确认用了 INPUT_PULLUP，而不是 INPUT。
 */

/*
 * 扩展练习：
 * 1. 把 Wi-Fi 名称（SSID）和密码保存到 Preferences，
 *    重启后自动读取并连接，做成无需重新烧录的智能设备。
 * 2. 保存 PWM 亮度值，断电重启后自动恢复上次的亮度。
 * 3. 用 prefs.putString() 保存字符串配置，
 *    例如设备昵称或服务器地址。
 */
