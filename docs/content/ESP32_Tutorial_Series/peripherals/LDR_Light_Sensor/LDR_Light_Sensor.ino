/*
 * 教程：光敏电阻（LDR）光照强度检测
 *
 * 学习目标：
 * 1. 了解光敏电阻的工作原理。
 * 2. 理解什么是分压电路，以及为什么需要它。
 * 3. 学会使用 ESP32 的 ADC 读取模拟电压。
 * 4. 把 ADC 原始值映射成百分比，并显示在串口上。
 *
 * 所需库：
 * 本例不需要额外库。
 *
 * 硬件连接（分压电路）：
 * - ESP32 3.3V  →  LDR 一端
 * - LDR 另一端  →  ESP32 GPIO34  并且  →  10kΩ 电阻  →  GND
 *
 * 说明：
 * LDR 和 10kΩ 电阻组成一个"分压电路"。
 * 当光照强时，LDR 阻值变小，GPIO34 上的电压变高；
 * 当光照弱时，LDR 阻值变大，GPIO34 上的电压变低。
 * ESP32 通过 ADC 读取这个电压，就能知道环境亮度。
 *
 * 注意：
 * ESP32 的 ADC 在 3.x 版本中默认分辨率是 12 位，返回值范围 0~4095。
 * GPIO34 属于 ADC1，推荐用于模拟输入。
 */

// LDR_PIN：连接分压电路中点的 GPIO。
// GPIO34 是 ADC1 通道 6，专门用于模拟输入，不会受到内部 Flash 等信号干扰。
#define LDR_PIN 34

// ADC_MAX_VALUE：12 位 ADC 的最大值。
// 2 的 12 次方等于 4096，所以有效范围是 0~4095。
#define ADC_MAX_VALUE 4095

void setup() {
  Serial.begin(115200);
  delay(1000);

  // analogReadResolution(12) 设置 ADC 分辨率为 12 位。
  // ESP32 Arduino 3.x 默认就是 12 位，这里显式写出来是为了让初学者清楚知道当前分辨率。
  analogReadResolution(12);

  // analogSetAttenuation(ADC_11db) 设置 ADC 输入衰减，让量程接近 0~3.3V。
  // ESP32 的 ADC 默认衰减通常也是 11dB，这里显式设置确保一致性。
  analogSetAttenuation(ADC_11db);

  Serial.println("LDR 光照传感器初始化完成");
  Serial.println("光线越强，原始值越大，百分比也越大。");
}

void loop() {
  // analogRead(pin) 读取指定引脚的模拟电压，返回 0~4095 的整数。
  // 这个值不是真正的电压，而是电压经过 ADC 转换后的数字量。
  int rawValue = analogRead(LDR_PIN);

  // 把原始值映射到 0~100 的百分比范围。
  // map(value, fromLow, fromHigh, toLow, toHigh) 是线性映射函数。
  // 这里假设 rawValue 最小 0 表示全黑，最大 4095 表示最亮。
  // 实际项目中，建议先遮住 LDR 和用手电筒照射，记录真实的最小值和最大值，
  // 再替换下面 map 里的 0 和 4095，这样百分比更准确。
  int brightnessPercent = map(rawValue, 0, ADC_MAX_VALUE, 0, 100);

  // 限制百分比在 0~100 之间，防止 map 因异常值产生负数或超过 100。
  // constrain(value, min, max) 会把越界值裁剪到范围内。
  brightnessPercent = constrain(brightnessPercent, 0, 100);

  // 把电压换算出来，方便理解。
  // 3.3V 对应 4095，所以每 1 个单位约等于 3.3 / 4095 V。
  float voltage = rawValue * 3.3 / ADC_MAX_VALUE;

  // 输出结果。
  Serial.print("原始值: ");
  Serial.print(rawValue);
  Serial.print("\t电压: ");
  Serial.print(voltage);
  Serial.print(" V\t亮度: ");
  Serial.print(brightnessPercent);
  Serial.println(" %");

  // 每 500ms 读取一次，光照变化不需要太快响应。
  delay(500);
}

/*
 * 名词解释：
 * LDR（Light Dependent Resistor，光敏电阻）：
 * 一种阻值随光照强度变化的电阻。
 * 光照越强，阻值越小；光照越弱，阻值越大。
 * 常见的 LDR 在暗处可能有几十 kΩ 到几 MΩ，在强光下只有几百 Ω。
 *
 * 分压电路（Voltage Divider）：
 * 用两个电阻串联，利用它们阻值比例把高电压按比例降低。
 * 因为 ESP32 ADC 只能测量电压，而 LDR 是电阻，
 * 所以必须把它和一个固定电阻组成分压电路，才能把"电阻变化"变成"电压变化"。
 *
 * ADC（Analog-to-Digital Converter，模数转换器）：
 * 把连续变化的模拟电压转换成离散数字值的电路。
 * ESP32 的 ADC 是 12 位，意味着把 0~3.3V 分成 4096 个等级。
 */

/*
 * 扩展练习：
 * 1. 根据亮度自动开关灯：亮度低于 30% 时打开 LED，高于 60% 时关闭。
 * 2. 把 LDR 和 OLED 结合，做一个实时亮度计。
 * 3. 把分压电阻换成 4.7kΩ 或 100kΩ，观察原始值范围的变化。
 */
