/*
 * 教程：HC-SR501 / PIR 人体红外运动传感器
 *
 * 学习目标：
 * 1. 了解 PIR 传感器的工作原理。
 * 2. 理解被动红外、热释电效应这两个概念。
 * 3. 学会读取 PIR 模块的数字输出。
 * 4. 用 PIR 传感器实现"有人靠近就亮灯"的效果。
 *
 * 所需库：
 * 本例不需要额外库。
 *
 * 硬件连接：
 * - PIR 模块 VCC  →  ESP32 5V（多数 PIR 模块需要 5V 才能稳定工作）
 * - PIR 模块 GND  →  ESP32 GND
 * - PIR 模块 OUT  →  ESP32 GPIO12
 * - LED 正极       →  ESP32 GPIO2（板载 LED 引脚，大多数 ESP32 开发板都有）
 * - LED 负极       →  330Ω 电阻 → GND
 *
 * 注意：
 * PIR 模块上通常有两个电位器：
 * - Sensitivity（灵敏度）：调节检测距离，顺时针增大。
 * - Time Delay（延时）：检测到运动后输出高电平持续的时间，顺时针延长。
 * 模块第一次上电需要 10~60 秒预热稳定，期间输出可能不稳定。
 */

// PIR_PIN：连接 PIR 模块 OUT 引脚的 GPIO。
// GPIO12 是通用 GPIO，3.3V 容忍。
#define PIR_PIN 12

// LED_PIN：用来指示是否检测到运动的 LED 引脚。
// GPIO2 通常连接开发板上的板载 LED，方便调试。
#define LED_PIN 2

// 记录上一次检测到的状态，用来判断"状态变化"（从无人到有人，或从有人到无人）。
// 这样只有在变化时才打印一次，避免串口被刷屏。
bool lastMotionState = false;

void setup() {
  Serial.begin(115200);
  delay(1000);

  // PIR 模块 OUT 引脚输出高低电平给 ESP32 读取，所以设为 INPUT。
  pinMode(PIR_PIN, INPUT);

  // LED 由 ESP32 控制，设为 OUTPUT。
  pinMode(LED_PIN, OUTPUT);

  // 一开始把 LED 熄灭。
  digitalWrite(LED_PIN, LOW);

  Serial.println("PIR 传感器初始化完成");
  Serial.println("请等待模块预热 10~60 秒...");

  // 延时 30 秒，让 PIR 模块完成预热，减少误触发。
  // 如果你只是做快速测试，可以把时间改短，但正式使用建议留足时间。
  delay(30000);
  Serial.println("预热完成，开始检测运动");
}

void loop() {
  // digitalRead(PIR_PIN) 读取 PIR 模块的输出。
  // 当检测到人体运动时，模块输出 HIGH；没有检测到时输出 LOW。
  int motionDetected = digitalRead(PIR_PIN);

  // 如果检测到运动，点亮 LED。
  if (motionDetected == HIGH) {
    digitalWrite(LED_PIN, HIGH);
  } else {
    digitalWrite(LED_PIN, LOW);
  }

  // 只有状态发生变化时才通过串口打印，避免每秒都刷屏。
  if (motionDetected != lastMotionState) {
    if (motionDetected == HIGH) {
      Serial.println("检测到运动！");
    } else {
      Serial.println("运动停止。");
    }
    lastMotionState = motionDetected;
  }

  // PIR 模块不需要很高读取频率，100ms 读取一次足够。
  delay(100);
}

/*
 * 名词解释：
 * PIR（Passive Infrared，被动红外）：
 * "被动"表示传感器本身不发射红外线，而是接收物体发出的红外辐射。
 * 人体温度约 37℃，会持续向外辐射红外线，传感器通过检测这种红外变化来发现运动。
 *
 * 热释电效应（Pyroelectric Effect）：
 * PIR 传感器内部有一种特殊材料，当红外线照射强度变化时会产生微弱电压。
 * 传感器通过菲涅尔透镜把大范围的红外信号聚焦到这种材料上，从而提高灵敏度。
 *
 * 菲涅尔透镜：
 * 传感器前面那个半球形或网格状的东西，它把不同方向的红外线折射到传感器上，
 * 让人即使在较远的位置移动，也能被检测到。
 */

/*
 * 扩展练习：
 * 1. 检测到运动后让蜂鸣器响 1 秒，做简易报警器。
 * 2. 结合 Wi-Fi，把"有人"事件发送到手机或服务器。
 * 3. 记录 1 分钟内检测到的运动次数，显示在串口或 OLED 上。
 */
