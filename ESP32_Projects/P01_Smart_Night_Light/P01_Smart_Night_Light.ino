/*
 * ESP32 综合项目 P01：智能小夜灯
 *
 * 项目目标：
 * 1. 把 ADC 模拟输入和 PWM 模拟输出结合起来，完成一个完整的应用
 * 2. 理解光敏电阻（Photoresistor / LDR）的工作原理
 * 3. 理解电阻分压电路如何把一个随光线变化的电阻转换成电压
 * 4. 学会用 map() 把 ADC 读数反向映射到 PWM 占空比
 * 5. 理解为什么实际项目中需要加入简单的软件滤波
 *
 * 前置知识：
 * - 已完成教程 03（PWM）、04（ADC），理解 ledcAttach()、analogRead()、map() 等
 *
 * 关键概念解释：
 *
 * 【光敏电阻 LDR】
 * LDR 是 Light Dependent Resistor（光敏电阻）的缩写。
 * 它的阻值会随光线强度变化：
 * - 光线越强，阻值越小（可能只有几百欧姆）
 * - 光线越弱，阻值越大（可能达到几兆欧姆）
 * 我们不能直接用 digitalRead() 读取它，因为阻值是连续变化的，
 * 需要把它接到 ADC 引脚，通过电压变化来判断亮度。
 *
 * 【电阻分压电路】
 * 光敏电阻只有两根引脚，本身不会输出电压，需要和一个固定电阻组成"分压电路"。
 * 分压电路可以把"电阻变化"转换成"电压变化"，这样 ADC 才能读取。
 *
 * 本项目的接法：
 * 3.3V → 光敏电阻 → ADC 引脚(GPIO34) → 10kΩ 固定电阻 → GND
 * 光线变强 → 光敏电阻变小 → ADC 引脚电压变低 → ADC 读数变小
 * 光线变弱 → 光敏电阻变大 → ADC 引脚电压变高 → ADC 读数变大
 *
 * 为什么要用 10kΩ 固定电阻？
 * 它决定分压的"中间点"。如果固定电阻太大或太小，
 * 亮度变化时 ADC 读数可能只在一个很小的范围内变化，不够灵敏。
 * 10kΩ 是一个常用的经验值，在室内光线变化范围内效果较好。
 *
 * 【反向映射】
 * 本项目要求"越暗越亮"，而 ADC 读数是"越暗越大"，
 * 所以需要把 ADC 读数反向映射到 PWM 占空比：
 * - ADC 大（暗）→ PWM 大（亮）
 * - ADC 小（亮）→ PWM 小（暗/灭）
 * 这正好和 map(adc, 0, 4095, 255, 0) 的方向相反。
 *
 * 【软件滤波 / 平均值】
 * ADC 读数会有噪声，单个值可能忽高忽低。
 * 本项目用"多次采样取平均"的简单方法让亮度变化更平滑，
 * 避免 LED 因为单次读数波动而忽明忽暗。
 *
 * 硬件连接：
 * - ESP32 GPIO2     →  LED 正极（长脚）
 * - LED 负极（短脚） →  330Ω 电阻      →  ESP32 GND
 *
 * - ESP32 3.3V      →  光敏电阻一端
 * - 光敏电阻另一端   →  ESP32 GPIO34
 * - ESP32 GPIO34    →  10kΩ 电阻      →  ESP32 GND
 *   （光敏电阻和 10kΩ 电阻串联，GPIO34 接在两者之间）
 *
 * 为什么要用 GPIO34？
 * GPIO34 属于 ADC1，只能输入，不能做输出，因此特别适合做模拟输入。
 * 另外它不受 Wi-Fi 影响，初学者优先使用 GPIO32~GPIO39 这一组 ADC 引脚。
 *
 * 常见问题与排查：
 * 1. LED 一直亮或一直灭，不随光线变化：
 *    - 检查光敏电阻是否和 10kΩ 电阻组成了正确的分压电路
 *    - 用串口监视器查看 ADC 原始值，确认它真的会随光线变化
 * 2. 光线很暗时 LED 反而熄灭：
 *    - 说明映射方向反了，检查 map() 的参数是不是 (adc, 0, 4095, 255, 0)
 * 3. LED 亮度忽明忽暗、不稳定：
 *    - 这是 ADC 噪声导致的正常现象
 *    - 可以增加 SAMPLE_COUNT（采样次数），让平均值更稳定
 *    - 也可以在 GPIO34 和 GND 之间并联一个 0.1uF 电容进行硬件滤波
 * 4. ADC 读数范围很小（比如只从 500 变到 1500）：
 *    - 可能是固定电阻不合适，可以尝试换成 4.7kΩ 或 22kΩ
 *    - 也可以调整 map() 的 fromLow/fromHigh 到实际最小/最大值
 * 5. 上电瞬间 LED 很亮然后变正常：
 *    - 这是因为 PWM 初始化前引脚处于不确定状态，通常不影响使用
 *    - 可以在 setup() 里先把 LED 熄灭
 */

// 定义 LED 引脚
// GPIO2 是大多数 ESP32 开发板板载 LED 所在的引脚
#define LED_PIN         2

// 定义光敏电阻连接的 ADC 引脚
// GPIO34 属于 ADC1，只能输入，适合做模拟读取
#define LDR_PIN         34

// PWM 参数
// PWM_FREQ 是 PWM 频率，5000Hz 足够平滑且不会闪烁
#define PWM_FREQ        5000

// PWM_RESOLUTION 是 PWM 分辨率，8 bit 表示占空比范围 0~255
#define PWM_RESOLUTION  8

// 采样次数，用于平均值滤波
// 每次读取光线时连续取 10 次再平均，可以减少 ADC 噪声
// 次数越多越稳定，但响应会稍微变慢；10 次是响应速度和稳定性的折中
#define SAMPLE_COUNT    10

// 用于存储当前 LED 的目标亮度
// 范围 0~255，0 全灭，255 最亮
int targetBrightness = 0;

void setup() {
  // 初始化串口，波特率 115200
  // 串口用来输出 ADC 原始值和 PWM 值，方便调试
  Serial.begin(115200);

  // 等待串口连接稳定
  delay(1000);

  Serial.println("================================");
  Serial.println("ESP32 综合项目 P01：智能小夜灯");
  Serial.println("================================");

  // 先把 LED 引脚设为输出并熄灭，避免上电瞬间出现不确定状态
  // OUTPUT 表示 ESP32 主动控制这个引脚的高低电平
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // 初始化 PWM
  // ledcAttach(pin, freq, resolution) 让 GPIO2 输出 5kHz、8bit 的 PWM
  // 这样我们才能通过改变占空比来连续调节 LED 亮度
  ledcAttach(LED_PIN, PWM_FREQ, PWM_RESOLUTION);

  // 显式设置 ADC 分辨率为 12 位，读数范围 0~4095
  // ESP32 Arduino 3.x 默认通常也是 12 位，但显式设置可以让意图更清晰
  analogReadResolution(12);

  // 设置 ADC 衰减为 11dB，可测量约 0~3.3V
  // 光敏电阻分压后的电压在 0~3.3V 之间，所以用 11dB 衰减
  analogSetAttenuation(ADC_11db);

  Serial.println("用手遮挡或照射光敏电阻，观察 LED 亮度变化");
}

void loop() {
  // 读取光线强度（带平均值滤波）
  // 调用自定义函数 getAmbientLight()，返回 0~4095 的平均 ADC 值
  int lightValue = getAmbientLight();

  // 把 ADC 值反向映射到 PWM 占空比
  // 参数说明：
  // - lightValue：当前 ADC 读数
  // - 0, 4095：ADC 的实际范围
  // - 255, 0：注意这里顺序是反的，表示 ADC 越大，PWM 越小？
  //   不对！仔细看：map(lightValue, 0, 4095, 255, 0)
  //   当 lightValue = 0（很亮）时，输出 255（最亮）
  //   当 lightValue = 4095（很暗）时，输出 0（熄灭）
  //   这样写会让白天很亮、晚上熄灭，和"夜灯"需求相反！
  //   所以我们应该写成 map(lightValue, 0, 4095, 0, 255)：
  //   白天 ADC 小 → PWM 小（暗或灭）
  //   晚上 ADC 大 → PWM 大（亮）
  //
  // 不过实际光敏电阻的值可能到不了 0 或 4095 的极限，
  // 我们可以用 map() 把实际观察到的最小/最大值映射到 0~255。
  // 这里为了演示清晰，先用 0~4095 的全范围。
  targetBrightness = map(lightValue, 0, 4095, 0, 255);

  // 限制 PWM 值在合法范围内
  // map() 的结果可能略微超出 0~255（比如 ADC 读数超过 4095 时），
  // 用 constrain() 把它限制在 0~255 之间，避免写入非法占空比
  targetBrightness = constrain(targetBrightness, 0, 255);

  // 输出 PWM 信号到 LED
  // targetBrightness 越大，LED 越亮
  ledcWrite(LED_PIN, targetBrightness);

  // 把当前状态打印到串口，方便观察调试
  // 格式：光线 ADC 值  |  LED 亮度 PWM 值
  Serial.print("环境光 ADC：");
  Serial.print(lightValue);
  Serial.print("  |  LED 亮度 PWM：");
  Serial.println(targetBrightness);

  // 每 100 毫秒更新一次
  // 如果更新太快，LED 会因为噪声而抖动；
  // 如果更新太慢，对光线变化的响应会不跟手。
  // 100 毫秒既能让眼睛感觉平滑，又能及时响应环境变化。
  delay(100);
}

// 自定义函数：读取环境光并做平均值滤波
// 作用：连续采样多次，取平均后返回更稳定的光线值
// 返回值：0~4095 的整数，数值越大表示环境越暗
int getAmbientLight() {
  // sum 用来累加多次采样的结果
  // 用 long 类型是为了防止多次累加后超过 int 的最大值
  long sum = 0;

  // 循环 SAMPLE_COUNT 次，每次读取一次 ADC 值
  for (int i = 0; i < SAMPLE_COUNT; i++) {
    // analogRead(LDR_PIN) 读取 GPIO34 的模拟电压
    // 返回值范围由 analogReadResolution() 决定，这里是 0~4095
    sum += analogRead(LDR_PIN);

    // 每次采样之间等待 1 毫秒
    // 让 ADC 电路有时间稳定，也让采样在时间上更分散
    delay(1);
  }

  // 返回平均值
  // 整数除法会舍去小数部分，对亮度控制来说已经足够
  return sum / SAMPLE_COUNT;
}

/*
 * 扩展挑战：
 * 1. 增加一个"亮度阈值"，只有当环境光低于某个值时才开启小夜灯，
 *    高于阈值时完全熄灭，避免白天也微微发光。
 *    提示：可以用 if(lightValue > threshold) 判断。
 * 2. 把 LED 换成 WS2812B 全彩灯，根据亮度变化颜色（比如暗时暖黄光，亮时白光）。
 * 3. 用 OLED 显示屏实时显示当前环境光 ADC 值和 LED 亮度百分比。
 * 4. 加入 EEPROM/Preferences，让用户可以校准自己家里的"最亮"和"最暗"ADC 值。
 */
