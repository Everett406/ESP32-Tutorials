/*
 * 教程：土壤湿度传感器
 *
 * 学习目标：
 * 1. 了解土壤湿度传感器的工作原理。
 * 2. 理解电阻式和电容式土壤湿度传感器的区别。
 * 3. 学会用 ADC 读取土壤湿度并校准。
 * 4. 根据湿度自动控制水泵或 LED 提示浇水。
 *
 * 所需库：
 * 本例不需要额外库。
 *
 * 硬件连接（以最常见的电阻式传感器模块为例）：
 * - 传感器 VCC  →  ESP32 3.3V
 * - 传感器 GND  →  ESP32 GND
 * - 传感器 AO   →  ESP32 GPIO35
 *
 * 注意：
 * 土壤湿度传感器有水气和盐分腐蚀问题，长期使用建议选"电容式"传感器，
 * 因为它没有直接暴露在土壤中的金属电极，寿命更长、测量更稳定。
 * 电阻式传感器虽然便宜，但探针容易被腐蚀，适合学习和短期项目。
 */

// SOIL_PIN：连接土壤湿度传感器 AO（模拟输出）的 GPIO。
// GPIO35 是 ADC1 通道 7，适合模拟输入。
#define SOIL_PIN 35

// PUMP_PIN：控制小水泵或继电器的 GPIO。
// 这里用 GPIO26 连接一个 LED 或继电器，湿度太低时打开提示/浇水。
#define PUMP_PIN 26

// DRY_VALUE：传感器完全暴露在空气中时读取到的 ADC 值，代表"最干"。
// WET_VALUE：传感器完全浸入水中时读取到的 ADC 值，代表"最湿"。
// 这两个值需要你自己先做一次校准，因为不同传感器、不同土壤读数会不一样。
// 上传代码前，先用串口观察并记录下你的实际数值，再替换这里的默认值。
#define DRY_VALUE 3500
#define WET_VALUE 1500

// 湿度阈值百分比，低于这个值认为需要浇水。
#define WATER_THRESHOLD 30

void setup() {
  Serial.begin(115200);
  delay(1000);

  // 设置 ADC 分辨率为 12 位，返回 0~4095。
  analogReadResolution(12);

  // 设置衰减，使量程接近 0~3.3V。
  analogSetAttenuation(ADC_11db);

  // 水泵/继电器控制引脚设为输出。
  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, LOW);

  Serial.println("土壤湿度传感器初始化完成");
  Serial.println("提示：请先用串口观察干/湿两个极端情况下的原始值，再修改 DRY_VALUE 和 WET_VALUE。");
}

void loop() {
  // 读取土壤湿度传感器的模拟输出。
  int rawValue = analogRead(SOIL_PIN);

  // 把原始值映射到 0~100% 的湿度百分比。
  // 注意：原始值越小，表示湿度越高（水分让探针间电阻变小，电压变低）。
  // 所以我们把 WET_VALUE（小）映射到 100，DRY_VALUE（大）映射到 0。
  int moisturePercent = map(rawValue, DRY_VALUE, WET_VALUE, 0, 100);

  // 限制百分比在合理范围。
  moisturePercent = constrain(moisturePercent, 0, 100);

  // 输出结果。
  Serial.print("原始值: ");
  Serial.print(rawValue);
  Serial.print("\t湿度: ");
  Serial.print(moisturePercent);
  Serial.println(" %");

  // 根据湿度决定是否浇水。
  if (moisturePercent < WATER_THRESHOLD) {
    Serial.println("土壤太干，开始浇水！");
    digitalWrite(PUMP_PIN, HIGH);
  } else {
    Serial.println("土壤湿度足够，停止浇水。");
    digitalWrite(PUMP_PIN, LOW);
  }

  // 土壤湿度变化很慢，每 2 秒读取一次即可。
  delay(2000);
}

/*
 * 名词解释：
 * 电阻式土壤湿度传感器：
 * 通过测量两个金属电极之间土壤的导电能力来判断湿度。
 * 土壤越湿，电阻越小，输出电压越低；土壤越干，电阻越大，输出电压越高。
 * 缺点是探针长期通电会被电解腐蚀，导致读数漂移。
 *
 * 电容式土壤湿度传感器：
 * 利用土壤作为电介质，测量电容变化来判断湿度。
 * 没有裸露金属电极，耐腐蚀，寿命更长，测量也更稳定，但价格稍贵。
 *
 * 校准（Calibration）：
 * 由于每个传感器、每块土壤的导电性不同，
 * 必须先记录"最干"和"最湿"两个基准值，
 * 程序才能根据这两个基准把原始 ADC 值换算成有意义的百分比。
 */

/*
 * 扩展练习：
 * 1. 用串口命令修改 WATER_THRESHOLD，不用重新烧录就能调整浇水阈值。
 * 2. 结合 OLED 显示屏，实时显示土壤湿度百分比和浇水状态。
 * 3. 改用电容式土壤湿度传感器，对比两种传感器的稳定性和寿命差异。
 */
