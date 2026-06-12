/*
 * 教程：HC-SR04 超声波测距传感器
 *
 * 学习目标：
 * 1. 了解超声波测距的基本原理。
 * 2. 学会使用 pulseIn() 测量高电平脉冲宽度。
 * 3. 理解距离、时间、声速之间的换算关系。
 * 4. 掌握在代码中对异常值进行简单过滤。
 *
 * 所需库：
 * 本例不需要额外安装第三方库，全部使用 Arduino 核心函数完成。
 *
 * 硬件连接：
 * - HC-SR04 VCC  →  ESP32 5V
 * - HC-SR04 GND  →  ESP32 GND
 * - HC-SR04 Trig →  ESP32 GPIO5
 * - HC-SR04 Echo →  ESP32 GPIO18
 *
 * 注意：
 * HC-SR04 的 VCC 通常需要 5V 才能稳定工作，
 * 但 Echo 引脚输出的是 5V 高电平，ESP32 GPIO 只能承受 3.3V。
 * 所以建议在 Echo 和 ESP32 之间加一个电平转换模块，
 * 或者串联一个 1kΩ 和 2kΩ 电阻组成的分压器把 5V 降到约 3.3V。
 * 有些新版 HC-SR04（如 RCW-0001）本身兼容 3.3V，可直接连接，请查看模块说明。
 */

// TRIG_PIN：触发测距的引脚。
// 我们向这个引脚发送一个短暂的高电平脉冲，传感器就开始发射超声波。
#define TRIG_PIN 5

// ECHO_PIN：接收回波的引脚。
// 传感器在发送超声波的同时会把这个引脚置高，
// 当接收到反射回来的超声波时再拉低。
// 高电平持续的时间就是超声波往返的时间。
#define ECHO_PIN 18

// SPEED_OF_SOUND_CM_PER_US：声音在空气中的传播速度。
// 常温下约为 343 m/s，也就是 0.0343 cm/μs。
// 计算距离时用这个值把时间（微秒）换算成距离（厘米）。
#define SPEED_OF_SOUND_CM_PER_US 0.0343

// MAX_DISTANCE_CM：我们关心的最大测量距离，这里设为 400 cm。
// HC-SR04 标称最大量程约 400cm，超过这个值认为超出范围。
// 用来计算最大等待时间，避免 pulseIn() 无限等待。
#define MAX_DISTANCE_CM 400

void setup() {
  Serial.begin(115200);
  delay(1000);

  // pinMode() 设置引脚模式。
  // TRIG 引脚由 ESP32 控制，输出触发脉冲，所以设为 OUTPUT。
  pinMode(TRIG_PIN, OUTPUT);

  // ECHO 引脚由传感器控制，ESP32 读取它的高电平宽度，所以设为 INPUT。
  pinMode(ECHO_PIN, INPUT);

  // 初始化时把 TRIG 置为 LOW，保证传感器处于空闲状态。
  // 如果 TRIG 一开始就是高电平，可能会导致传感器误判为收到触发信号。
  digitalWrite(TRIG_PIN, LOW);

  Serial.println("HC-SR04 超声波测距初始化完成");
}

void loop() {
  // 步骤 1：发送触发脉冲。
  // 先把 TRIG 拉低至少 2μs，让传感器准备接收新的触发。
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  // 把 TRIG 拉高 10μs，这是 HC-SR04 规定的触发条件。
  // 时间不能太长也不能太短，10μs 是数据手册推荐值。
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);

  // 触发结束后立刻把 TRIG 拉低。
  digitalWrite(TRIG_PIN, LOW);

  // 步骤 2：测量 Echo 高电平持续时间。
  // pulseIn(pin, HIGH, timeout) 会等待引脚变成高电平，
  // 然后计时直到它变回低电平，返回这段时间的微秒数。
  // timeout 是最大等待时间，单位也是微秒。
  // 这里根据最大距离计算：声音往返 400cm 需要 400*2/0.0343 ≈ 23324μs。
  unsigned long timeoutUs = (unsigned long)(MAX_DISTANCE_CM * 2 / SPEED_OF_SOUND_CM_PER_US);
  unsigned long durationUs = pulseIn(ECHO_PIN, HIGH, timeoutUs);

  // 步骤 3：计算距离。
  // durationUs 是超声波从发出到返回的总时间，走的是"去程+回程"两段路。
  // 所以实际距离要除以 2。
  // 公式：距离 = 时间 × 声速 / 2。
  float distanceCm = durationUs * SPEED_OF_SOUND_CM_PER_US / 2.0;

  // 步骤 4：输出结果或错误提示。
  // 如果 durationUs 为 0，说明在 timeout 时间内没有检测到回波，
  // 可能是前方没有障碍物、距离太远，或者接线有误。
  if (durationUs == 0) {
    Serial.println("超出测量范围或未检测到障碍物");
  } else {
    Serial.print("距离: ");
    Serial.print(distanceCm);
    Serial.println(" cm");
  }

  // 每隔 200ms 测量一次。
  // 测得太频繁会导致回声干扰，测得太慢则不够实时，200ms 是一个比较平衡的值。
  delay(200);
}

/*
 * 扩展练习：
 * 1. 修改 MAX_DISTANCE_CM，观察不同最大量程下的响应速度。
 * 2. 把距离分段：小于 10cm 亮红灯，10~30cm 亮黄灯，大于 30cm 亮绿灯。
 * 3. 连续读取 5 次取平均值，减少偶然误差。
 */
