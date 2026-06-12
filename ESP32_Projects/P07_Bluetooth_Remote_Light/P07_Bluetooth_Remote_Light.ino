/*
 * P07_Bluetooth_Remote_Light
 * 蓝牙遥控灯
 *
 * 学习目标：
 * 1. 理解经典蓝牙串口（BluetoothSerial / SPP）的工作方式
 * 2. 学会用手机 APP 通过蓝牙无线控制 ESP32
 * 3. 掌握用 PWM 调节 LED 亮度的方法
 * 4. 学会解析字符串命令并执行不同动作
 *
 * 前置知识：
 * - PWM（Pulse Width Modulation，脉宽调制）：让引脚快速在高/低电平之间切换，
 *   通过改变高电平所占时间比例（占空比）来模拟不同的亮度。
 * - SPP（Serial Port Profile，串口端口配置文件）：把蓝牙模拟成"无线串口"，
 *   手机和 ESP32 之间可以像串口一样收发文本。
 *
 * 硬件连接：
 * - ESP32 GPIO2  →  LED 正极（长脚）
 * - LED 负极（短脚） →  330Ω 电阻  →  GND
 *
 * 手机端：
 * - 安卓手机可安装 "Serial Bluetooth Terminal" 或 "蓝牙串口" APP。
 * - iOS 对经典蓝牙支持有限，建议使用 BLE 项目。
 */

// 引入经典蓝牙串口库
// BluetoothSerial 把 ESP32 变成一个蓝牙串口设备，手机可以连上来发文字
#include "BluetoothSerial.h"

// LED 引脚定义
// GPIO2 是大多数 ESP32 开发板自带的板载 LED 引脚，方便测试
#define LED_PIN 2

// PWM（脉宽调制）通道和参数
// ESP32 Arduino 3.x 中，ledcAttach() 会自动分配通道，不需要手动指定通道号
#define PWM_FREQUENCY 5000  // PWM 频率 5kHz，LED 调光用这个频率比较平滑
#define PWM_RESOLUTION 8    // 分辨率 8 位，占空比范围 0 ~ 255

// 蓝牙设备名称，手机搜索时会看到这个名字
#define BT_DEVICE_NAME "ESP32_Remote_Light"

// 创建蓝牙串口对象
// BluetoothSerial 继承自 Stream，因此可以像 Serial 一样使用 print/read 等函数
BluetoothSerial SerialBT;

// 当前 LED 亮度，范围 0 ~ 255
// 0 表示完全熄灭，255 表示最亮
uint8_t current_duty = 0;

// 闪烁模式标志
// true：进入闪烁模式；false：正常控制模式
bool blink_mode = false;

// 记录上一次闪烁状态翻转的时间（毫秒）
// 用于非阻塞式闪烁，避免使用 delay() 阻塞蓝牙命令处理
unsigned long last_blink_time = 0;

// 闪烁间隔（毫秒）
#define BLINK_INTERVAL 500

// 从蓝牙接收到的完整命令字符串
// String 类方便拼接字符，适合这种短命令场景
String bt_command = "";

// setup() 只在上电或复位后执行一次，完成初始化工作
void setup() {
  // 初始化串口监视器，波特率 115200
  // 用于在电脑上观察调试输出
  Serial.begin(115200);

  // 将 LED 引脚绑定到 PWM 功能
  // ledcAttach(pin, freq, resolution) 是 ESP32 Arduino 3.x 的新写法
  // 它会自动分配 PWM 通道，并把指定引脚和该通道关联起来
  // 旧版的 ledcSetup() + ledcAttachPin() 已经被废弃，不建议使用
  ledcAttach(LED_PIN, PWM_FREQUENCY, PWM_RESOLUTION);

  // 初始状态熄灭 LED
  // ledcWrite(pin, duty) 设置指定引脚的 PWM 占空比
  // 占空比 = duty / (2^分辨率 - 1)，这里是 duty / 255
  ledcWrite(LED_PIN, 0);

  // 启动蓝牙串口，并设置设备名称
  // 手机搜索蓝牙时会看到 "ESP32_Remote_Light"，点击连接即可
  SerialBT.begin(BT_DEVICE_NAME);

  Serial.println("蓝牙遥控灯已启动");
  Serial.print("设备名称：");
  Serial.println(BT_DEVICE_NAME);
  Serial.println("等待手机连接...");
  Serial.println("可用命令：on / off / duty 0~255 / blink");
}

// loop() 会不断循环执行，负责读取蓝牙命令并更新 LED 状态
void loop() {
  // 1. 读取蓝牙串口收到的数据
  // available() 返回当前缓冲区中可读取的字节数
  while (SerialBT.available()) {
    // 每次读取一个字符
    char c = SerialBT.read();

    // 有些 APP 发送命令会带换行符 \n 或回车符 \r
    // 遇到换行或回车，说明一条命令接收完成
    if (c == '\n' || c == '\r') {
      // 只有命令非空才处理，避免连续空行
      if (bt_command.length() > 0) {
        // 处理这条命令
        processCommand(bt_command);
        // 清空命令缓冲区，准备接收下一条
        bt_command = "";
      }
    } else {
      // 把字符追加到命令缓冲区
      bt_command += c;
    }
  }

  // 2. 如果处于闪烁模式，按时间间隔切换 LED 亮灭
  // 使用 millis() 做非阻塞闪烁，这样即使闪烁也不会漏掉蓝牙命令
  if (blink_mode) {
    unsigned long now = millis();
    if (now - last_blink_time >= BLINK_INTERVAL) {
      last_blink_time = now;

      // 如果当前是亮的就熄灭，如果是灭的就按 current_duty 亮度点亮
      // 这里用 current_duty 控制闪烁时的亮度，默认熄灭时为 0
      static bool blink_state = false;
      blink_state = !blink_state;

      if (blink_state) {
        // 闪烁时用一个适中的亮度，如果 current_duty 为 0 则用 128
        uint8_t blink_duty = (current_duty > 0) ? current_duty : 128;
        ledcWrite(LED_PIN, blink_duty);
      } else {
        ledcWrite(LED_PIN, 0);
      }
    }
  }
}

// processCommand 用于解析并执行蓝牙发送来的命令
// 参数 cmd：从手机接收到的字符串命令
void processCommand(String cmd) {
  // 先把命令统一转成小写，方便判断，避免 "ON" 和 "on" 被当成不同命令
  cmd.toLowerCase();

  // 去掉首尾空格，防止 APP 自动添加空格导致命令不匹配
  cmd.trim();

  Serial.print("收到命令：");
  Serial.println(cmd);

  // 把命令也回传给手机，方便用户在 APP 里看到确认信息
  SerialBT.print("收到：");
  SerialBT.println(cmd);

  // 命令：on —— 点亮 LED，使用上一次的亮度（默认 255）
  if (cmd == "on") {
    blink_mode = false;                 // 退出闪烁模式
    current_duty = (current_duty == 0) ? 255 : current_duty;
    ledcWrite(LED_PIN, current_duty);
    SerialBT.println("LED 已点亮");
    return;
  }

  // 命令：off —— 熄灭 LED
  if (cmd == "off") {
    blink_mode = false;                 // 退出闪烁模式
    ledcWrite(LED_PIN, 0);
    SerialBT.println("LED 已熄灭");
    return;
  }

  // 命令：blink —— 让 LED 以当前亮度闪烁
  if (cmd == "blink") {
    blink_mode = true;
    SerialBT.println("LED 开始闪烁，发送 on/off 停止");
    return;
  }

  // 命令：duty XXX —— 设置 LED 亮度占空比，XXX 为 0 ~ 255 的数字
  // 例如：duty 50 表示比较暗，duty 255 表示最亮
  if (cmd.startsWith("duty ")) {
    // 提取 "duty " 后面的数字字符串
    String value_str = cmd.substring(5);

    // toInt() 把字符串转成整数
    int value = value_str.toInt();

    // 限制范围在 0 ~ 255 之间，防止用户输入过大或负数
    if (value < 0) value = 0;
    if (value > 255) value = 255;

    current_duty = (uint8_t)value;
    blink_mode = false;                 // 退出闪烁模式，让亮度设置立即生效
    ledcWrite(LED_PIN, current_duty);

    SerialBT.print("LED 占空比已设为：");
    SerialBT.println(current_duty);
    return;
  }

  // 如果命令不认识，给出提示
  SerialBT.println("未知命令。可用命令：on / off / duty 0~255 / blink");
}

/*
 * 扩展挑战：
 * 1. 增加 "fade" 命令，让 LED 在 0% 和 100% 之间做呼吸灯效果。
 * 2. 增加多个 LED 控制，例如 "red on" / "blue duty 100"，实现 RGB 调色。
 * 3. 增加密码验证，只有发送正确密码后才执行控制命令，提高安全性。
 */
