/*
 * ESP32 教程系列 03：PWM 输出 - LED 呼吸灯
 *
 * 学习目标：
 * 1. 理解 PWM（脉宽调制）的基本原理
 * 2. 理解 PWM 频率和分辨率的含义
 * 3. 学会使用 ESP32 Arduino 3.x 的 LEDC API 输出 PWM
 * 4. 学会用 for 循环实现 LED 亮度连续变化
 * 5. 理解为什么 PWM 可以"模拟"出不同的亮度
 *
 * 前置知识：
 * - 已完成教程 01、02，理解 GPIO、HIGH/LOW、delay() 等基础概念
 *
 * 关键概念解释：
 *
 * 【PWM 是什么？】
 * PWM 是 Pulse Width Modulation 的缩写，中文叫"脉宽调制"。
 * 它不是真正的模拟输出，而是让引脚快速地在高电平和低电平之间切换。
 * 切换速度非常快，人眼有"视觉暂留"效应，看不到快速闪烁，只能看到平均亮度。
 * 如果高电平时间占总周期的比例大，LED 看起来就更亮；比例小就更暗。
 * 这个高电平时间占总周期的比例就叫"占空比"（Duty Cycle）。
 *
 * 举个例子：
 * - 占空比 0%：引脚一直 LOW，LED 全灭
 * - 占空比 50%：引脚一半时间 HIGH，一半时间 LOW，LED 中等亮度
 * - 占空比 100%：引脚一直 HIGH，LED 最亮
 *
 * 【PWM 频率】
 * 频率表示每秒完成多少个 PWM 周期，单位是 Hz（赫兹）。
 * 本程序用 5000Hz，意思是每秒开关 5000 次。
 * 频率太低时人眼能看到闪烁，频率足够高时（通常几百 Hz 以上）就看不到闪烁了。
 * 对于 LED 调光，一般 1kHz~20kHz 都可以，本程序选 5kHz 是一个兼顾稳定和效果的安全值。
 *
 * 【PWM 分辨率】
 * 分辨率表示占空比的精细程度，单位是 bit（位）。
 * 8 bit 表示占空比可以用 0~255 之间的整数表示，共 256 个等级。
 * 分辨率越高，亮度变化越平滑，但会消耗更多硬件资源。
 * 对于普通 LED 调光，8 bit 已经完全够用。
 *
 * 计算公式：
 * - 8 bit：范围 0 ~ 2^8 - 1 = 0 ~ 255
 * - 10 bit：范围 0 ~ 2^10 - 1 = 0 ~ 1023
 * - 12 bit：范围 0 ~ 2^12 - 1 = 0 ~ 4095
 *
 * 【LEDC 是什么？】
 * LEDC 是 ESP32 内部的 LED PWM 控制器硬件模块。
 * Arduino 通过 ledcAttach()、ledcWrite() 等函数来控制这个硬件模块。
 * 注意：ESP32 Arduino 3.x 使用的 API 和 2.x 不同，
 * 3.x 使用 ledcAttach()，而 2.x 使用 ledcSetup() + ledcAttachPin()。
 * 本教程使用 ESP32 Arduino 3.x 的新写法。
 *
 * 硬件连接：
 * - LED 正极（长脚） →  ESP32 GPIO2
 * - LED 负极（短脚） →  330Ω 电阻  →  ESP32 GND
 *
 * 为什么用 GPIO2？
 * GPIO2 是大多数 ESP32 开发板板载 LED 连接的引脚，
 * 即使没有外接 LED，也可以直接观察板载 LED 的呼吸效果。
 *
 * 常见问题与排查：
 * 1. LED 亮度没有变化，只有全亮或全灭：
 *    - 检查是否使用了 ESP32 Arduino 3.x 的 ledcAttach()，而不是 2.x 的 ledcSetup()
 *    - 检查 ledcWrite() 的 duty 值是否在 0~255 范围内
 * 2. LED 闪烁而不是平滑变化：
 *    - PWM 频率可能太低，尝试把 PWM_FREQ 提高到 10000 或更高
 *    - 检查电源是否稳定，电压不足可能导致异常
 * 3. 串口看不到输出：
 *    - 确认波特率为 115200
 * 4. 某些引脚无法输出 PWM：
 *    - ESP32 大多数 GPIO 都支持 PWM，但 GPIO6~GPIO11 通常用于连接 Flash，不建议使用
 *    - 如果换引脚，建议用 GPIO2、GPIO4、GPIO5、GPIO12~GPIO19 等
 */

// 定义 PWM 参数
// LED_PIN 是连接 LED 的引脚，这里用 GPIO2
#define LED_PIN         2

// PWM_FREQ 是 PWM 频率，单位 Hz
// 5000 表示每秒开关 5000 次，足够快，肉眼看不到闪烁
#define PWM_FREQ        5000

// PWM_RESOLUTION 是 PWM 分辨率，单位 bit
// 8 表示占空比范围是 0~255，共 256 个亮度等级
#define PWM_RESOLUTION  8

void setup() {
  // 初始化串口，波特率 115200
  Serial.begin(115200);

  // 等待串口连接稳定
  delay(1000);

  Serial.println("================================");
  Serial.println("ESP32 教程 03：PWM 输出 - LED 呼吸灯");
  Serial.println("================================");

  // ESP32 Arduino 3.x 的 LEDC PWM API：
  // ledcAttach(pin, freq, resolution)
  // 参数说明：
  // - pin：要输出 PWM 的 GPIO 引脚
  // - freq：PWM 频率，单位 Hz
  // - resolution：分辨率，单位 bit，决定占空比范围
  //   8 bit 表示占空比范围 0~255
  //   10 bit 表示占空比范围 0~1023
  //   12 bit 表示占空比范围 0~4095
  //
  // 这行代码的作用是：让 LED_PIN（GPIO2）输出 5kHz、8bit 分辨率的 PWM 信号。
  // 注意：ESP32 不同 GPIO 支持的最高频率和分辨率不同，
  // 初学者用 8 bit + 5kHz 通常不会出问题。
  ledcAttach(LED_PIN, PWM_FREQ, PWM_RESOLUTION);

  Serial.println("PWM 已初始化");
}

void loop() {
  // 第一阶段：LED 由暗到亮
  // for 循环让 duty 从 0 增加到 255，每次增加 1
  // duty 就是占空比的数值，越大 LED 越亮
  //
  // for 循环的执行过程拆解：
  // 1. int duty = 0：先创建变量 duty，初始值为 0（只在第一次执行）
  // 2. duty <= 255：判断条件，如果成立就执行循环体；不成立就结束循环
  // 3. 执行 { ... } 里的代码（ledcWrite 和 delay）
  // 4. duty++：把 duty 加 1
  // 5. 回到第 2 步，再次判断 duty <= 255
  //
  // 所以当 duty = 255 时，还会执行一次循环体；
  // 然后 duty 变成 256，判断 256 <= 255 不成立，循环结束。
  // 你的理解是对的！
  for (int duty = 0; duty <= 255; duty++) {
    // ledcWrite(pin, duty) 输出指定占空比的 PWM 波形
    // 参数说明：
    // - pin：输出 PWM 的引脚
    // - duty：占空比数值，范围由分辨率决定（8 bit 是 0~255）
    // duty 越大，一个周期内高电平时间越长，LED 平均电流越大，看起来越亮
    ledcWrite(LED_PIN, duty);

    // delay(10) 让程序暂停 10 毫秒
    // 这样从 0 变到 255 总共需要约 2550 毫秒（2.55 秒）
    // 如果时间太短，变化太快，人眼来不及感受；如果时间太长，变化又太慢
    // 10 毫秒是一个比较舒服的呼吸速度
    delay(10);
  }

  // 第二阶段：LED 由亮到暗
  // for 循环让 duty 从 255 减小到 0，每次减少 1
  for (int duty = 255; duty >= 0; duty--) {
    ledcWrite(LED_PIN, duty);
    delay(10);
  }

  // 每次 loop() 完成一次"由暗到亮再到暗"的呼吸效果
  // 打印一条信息，方便观察循环是否正常执行
  Serial.println("完成一次呼吸");
}

/*
 * 扩展练习：
 * 1. 修改 delay(10) 的数值，比如改成 5 或 20，观察呼吸速度变化。
 * 2. 修改 PWM_RESOLUTION 为 10 或 12，同时把 for 循环范围改成 0~1023 或 0~4095，
 *    观察亮度变化是否更细腻。
 * 3. 用按键控制呼吸灯开关：按下按键开始/暂停呼吸效果。
 * 4. 让 LED 先快速变亮、再慢速变暗，体验非对称呼吸效果。
 * 5. 把 LED_PIN 改成支持 PWM 的其他引脚，验证不同引脚是否都能输出 PWM。
 */
