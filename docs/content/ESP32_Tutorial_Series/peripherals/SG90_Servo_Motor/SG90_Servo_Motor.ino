/*
 * 教程：SG90 舵机控制
 *
 * 学习目标：
 * 1. 了解舵机的工作原理和控制信号。
 * 2. 理解 PWM、周期、脉宽、占空比这几个概念。
 * 3. 学会安装和使用 ESP32Servo 库驱动舵机。
 * 4. 让舵机在 0° 到 180° 之间来回摆动。
 *
 * 需要安装的库：
 * - "ESP32Servo" by Kevin Harrington、John K. Bennett 等
 * 安装方法：
 * 1. Arduino IDE → 项目 → 加载库 → 管理库...。
 * 2. 搜索 "ESP32Servo"，找到对应库并安装。
 *
 * 硬件连接：
 * - SG90 棕色线  →  ESP32 GND
 * - SG90 红色线  →  ESP32 5V（舵机启动电流较大，建议外接 5V 电源）
 * - SG90 橙色线  →  ESP32 GPIO13（信号线）
 *
 * 注意：
 * SG90 的电源最好使用独立 5V 电源，并把电源负极和 ESP32 GND 接到一起（共地）。
 * 如果只用 ESP32 的 5V 引脚供电，舵机转动时电压跌落可能导致 ESP32 重启。
 */

// 引入 ESP32Servo 库。
// 这个库封装了 PWM 定时器的细节，让我们能像控制普通舵机一样使用 write() 函数。
#include <ESP32Servo.h>

// SERVO_PIN：连接舵机信号线的 GPIO。
// GPIO13 是通用 GPIO，支持 PWM 输出，3.3V 信号即可驱动大部分舵机。
#define SERVO_PIN 13

// 创建一个 Servo 对象，名字叫 myServo。
// 对象里保存了 PWM 通道、角度、引脚等信息。
Servo myServo;

void setup() {
  Serial.begin(115200);
  delay(1000);

  // attach(pin, min, max) 把舵机对象绑定到某个 GPIO。
  // 第二个参数是 0° 对应的脉宽（微秒），第三个参数是 180° 对应的脉宽（微秒）。
  // SG90 通常用 500μs 表示 0°，2400μs 表示 180°。
  // 如果范围设得太小，舵机就转不到头；设得太大，可能会让舵机抖动或卡住。
  myServo.attach(SERVO_PIN, 500, 2400);

  // 上电后先把舵机转到中间位置 90°，
  // 这样可以避免突然从某个极限位置启动，机械冲击更小。
  myServo.write(90);

  Serial.println("SG90 舵机初始化完成");
}

void loop() {
  // 让舵机从 0° 转到 180°。
  // write(angle) 告诉舵机目标角度，库会自动把角度换算成对应的 PWM 脉宽。
  for (int angle = 0; angle <= 180; angle++) {
    myServo.write(angle);

    // 每转 1° 等待 15ms，给舵机机械结构足够时间到达目标位置。
    // 如果时间太短，舵机还没到位就收到下一个角度，会转得很吃力、发热甚至抖动。
    delay(15);
  }

  Serial.println("已转到 180°");

  // 让舵机从 180° 转回 0°。
  for (int angle = 180; angle >= 0; angle--) {
    myServo.write(angle);
    delay(15);
  }

  Serial.println("已转回 0°");

  // 在 0° 停留 500ms，方便观察。
  delay(500);
}

/*
 * 名词解释：
 * PWM（Pulse Width Modulation，脉宽调制）：
 * 引脚不是一直输出高电平，而是快速地开关。
 * 一个完整的开关周期叫"周期"，高电平时间占周期的比例叫"占空比"。
 *
 * 舵机控制信号：
 * 舵机不关心占空比本身，而是关心每个 20ms 周期内高电平的持续时间（脉宽）。
 * - 0.5ms 脉宽 ≈ 0°
 * - 1.5ms 脉宽 ≈ 90°
 * - 2.4ms 脉宽 ≈ 180°
 * 周期 20ms 对应频率 50Hz，所以控制舵机的 PWM 频率通常是 50Hz。
 */

/*
 * 扩展练习：
 * 1. 修改 attach() 里的 min/max 脉宽，观察舵机实际转动范围的变化。
 * 2. 用串口输入一个角度（如 90），让舵机转到指定位置。
 * 3. 接一个电位器，把 ADC 读数映射到 0~180°，用手势控制舵机。
 */
