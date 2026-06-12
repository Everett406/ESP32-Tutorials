/*
 * 教程：有源蜂鸣器（Active Buzzer）
 *
 * 学习目标：
 * 1. 了解有源蜂鸣器和无源蜂鸣器的区别。
 * 2. 学会用 digitalWrite() 控制蜂鸣器发声和停止。
 * 3. 用简单的时序编写报警声、门铃声等提示音。
 * 4. 理解为什么有源蜂鸣器不能直接接到 PWM 引脚产生不同音调。
 *
 * 所需库：
 * 本例不需要额外库。
 *
 * 硬件连接：
 * - 有源蜂鸣器正极（+） →  ESP32 GPIO25
 * - 有源蜂鸣器负极（-） →  GND
 *
 * 注意：
 * 有些蜂鸣器模块内部已经串联了限流电阻，可以直接接 GPIO。
 * 如果你的蜂鸣器是裸蜂鸣器（没有模块板），建议在正极串联一个 100Ω 电阻，
 * 限制电流，保护 ESP32 GPIO。
 * 有源蜂鸣器只需要直流电就会响，不需要也不会响应不同频率的 PWM。
 */

// BUZZER_PIN：连接蜂鸣器正极的 GPIO。
// GPIO25 是通用 GPIO，也是 DAC 引脚之一，这里当普通数字输出用即可。
#define BUZZER_PIN 25

void setup() {
  Serial.begin(115200);
  delay(1000);

  // 把蜂鸣器引脚设为输出，因为 ESP32 要控制它的通断。
  pinMode(BUZZER_PIN, OUTPUT);

  // 初始化时先关闭蜂鸣器，避免一上电就响。
  digitalWrite(BUZZER_PIN, LOW);

  Serial.println("有源蜂鸣器初始化完成");
}

void loop() {
  // 效果 1：短促的"滴滴"声，模拟提示音。
  Serial.println("提示音");
  beep(100, 100);  // 响 100ms，停 100ms
  beep(100, 100);
  beep(100, 500);  // 最后停 500ms

  // 效果 2：报警声，长响三次。
  Serial.println("报警声");
  for (int i = 0; i < 3; i++) {
    beep(500, 200);
  }

  // 效果 3：模拟门铃，"叮-咚"。
  Serial.println("门铃声");
  digitalWrite(BUZZER_PIN, HIGH);
  delay(80);
  digitalWrite(BUZZER_PIN, LOW);
  delay(150);
  digitalWrite(BUZZER_PIN, HIGH);
  delay(250);
  digitalWrite(BUZZER_PIN, LOW);

  // 每个循环结束后安静 2 秒，再重复。
  delay(2000);
}

// beep：让蜂鸣器响一段时间，然后静默一段时间。
// onTimeMs 是发声时长（毫秒），offTimeMs 是静音时长（毫秒）。
void beep(unsigned int onTimeMs, unsigned int offTimeMs) {
  // digitalWrite(HIGH) 让 GPIO 输出高电平，蜂鸣器通电发声。
  digitalWrite(BUZZER_PIN, HIGH);
  delay(onTimeMs);

  // digitalWrite(LOW) 让 GPIO 输出低电平，蜂鸣器断电停止。
  digitalWrite(BUZZER_PIN, LOW);
  delay(offTimeMs);
}

/*
 * 名词解释：
 * 有源蜂鸣器（Active Buzzer）：
 * 内部自带振荡电路，只要接通直流电源（给正极高电平）就会以固定频率鸣叫。
 * 优点：控制简单，一个 GPIO 就能让它响或停。
 * 缺点：音调和频率固定，无法播放不同音高。
 *
 * 无源蜂鸣器（Passive Buzzer）：
 * 内部只有线圈和振膜，没有振荡电路。
 * 需要外部给它提供一定频率的方波（例如用 PWM）才能发声，
 * 改变方波频率就能改变音调，可以播放简单音乐。
 *
 * 限流电阻：
 * 蜂鸣器工作时电流较大，直接接到 GPIO 可能会超过引脚最大输出电流。
 * 串联电阻可以限制电流，保护主控和蜂鸣器。
 */

/*
 * 扩展练习：
 * 1. 把有源蜂鸣器换成无源蜂鸣器，用 ledcWriteTone() 播放一首简单乐曲。
 * 2. 结合 PIR 传感器，检测到运动时蜂鸣器发出报警声。
 * 3. 用光敏电阻做一个"光线暗就鸣叫"的提醒装置。
 */
