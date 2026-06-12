/*
 * ESP32 教程系列 04：ADC 模拟输入 - 读取电位器/光照强度
 *
 * 学习目标：
 * 1. 理解模拟信号和数字信号的区别
 * 2. 理解 ADC（模数转换器）的作用
 * 3. 学会使用 analogRead() 读取 ESP32 的 ADC 值
 * 4. 学会使用 map() 函数把数值从一个范围映射到另一个范围
 * 5. 学会把 ADC 读数映射到 PWM 占空比，实现电位器调光
 *
 * 前置知识：
 * - 已完成教程 01、03，理解 GPIO、PWM、ledcAttach()、ledcWrite() 等
 *
 * 关键概念解释：
 *
 * 【模拟信号 vs 数字信号】
 * 现实世界中的很多物理量（温度、亮度、声音、角度）是连续变化的，
 * 这种连续变化的信号叫"模拟信号"。
 * 例如电位器中间引脚的电压可以从 0V 平滑变化到 3.3V，中间有无数种可能值。
 *
 * 数字信号则只有有限个离散值，在 Arduino 中通常是 HIGH/LOW 或 0/4095。
 * 单片机内部只能处理数字信号，所以需要用 ADC 把模拟信号转换成数字值。
 *
 * 【ADC 是什么？】
 * ADC 是 Analog-to-Digital Converter（模数转换器）的缩写。
 * 它把引脚上的模拟电压转换成单片机能理解的数字值。
 * ESP32 的 ADC 默认分辨率是 12 位，读数范围是 0~4095。
 * - 0V 对应 0
 * - 3.3V 对应 4095
 * - 1.65V 对应约 2047
 *
 * 分辨率越高，能区分的电压差异越精细：
 * - 12 bit：4096 个等级，每个等级约 0.8mV（3.3V / 4096）
 * - 10 bit：1024 个等级
 * - 8 bit：256 个等级
 *
 * 【ESP32 的 ADC 引脚】
 * ESP32 有两组 ADC：
 * - ADC1：GPIO32~GPIO39，推荐用于一般模拟输入
 * - ADC2：GPIO0、GPIO2、GPIO4、GPIO12~GPIO15、GPIO25~GPIO27
 *   ADC2 在 Wi-Fi/BLE 使用时可能受影响，建议初学者优先使用 ADC1 的 GPIO32~GPIO39。
 * GPIO34、GPIO35、GPIO36、GPIO39 只能做输入，不能输出，最适合做 ADC。
 *
 * 【什么是电位器？】
 * 电位器（Potentiometer）是一种可调电阻，通常有三个引脚：
 * - 两边两个引脚：分别接电源和地
 * - 中间引脚：随旋钮位置输出 0V 到电源电压之间的任意电压
 * 本教程用电位器模拟各种模拟传感器，通过旋转旋钮改变输入电压。
 *
 * 【map() 函数】
 * map(value, fromLow, fromHigh, toLow, toHigh)
 * 作用：把数值从一个范围线性映射到另一个范围。
 * 例如 map(adcValue, 0, 4095, 0, 255) 表示：
 * - 当 adcValue 为 0 时，输出 0
 * - 当 adcValue 为 4095 时，输出 255
 * - 中间值按比例映射
 * 这里用来把 ADC 读数（0~4095）转换成 PWM 占空比（0~255）。
 *
 * 【衰减倍数 Attenuation】
 * ESP32 的 ADC 可以设置输入电压范围，叫"衰减"（Attenuation）。
 * 默认通常是 11dB，可测量约 0~3.3V。
 * 如果输入信号超过 3.3V，需要用电阻分压；如果信号很小，可以调整衰减提高灵敏度。
 * 初学者用电位器接 3.3V，用默认衰减即可。
 *
 * 硬件连接：
 * - 电位器中间引脚 →  ESP32 GPIO34
 * - 电位器两边引脚 →  3.3V 和 GND（顺序不影响，只是旋转方向相反）
 *
 * - LED 正极（长脚） →  ESP32 GPIO2
 * - LED 负极（短脚） →  330Ω 电阻  →  ESP32 GND
 *
 * 为什么电位器可以输出 0~3.3V？
 * 电位器内部是一个连续可调的电阻，相当于一个分压器。
 * 当旋钮转到一端，中间引脚接近 3.3V；转到另一端，接近 0V；
 * 中间位置则输出两者之间的电压。
 *
 * 常见问题与排查：
 * 1. 旋转电位器时 ADC 值没有变化：
 *    - 检查电位器中间引脚是否接在 GPIO34
 *    - 检查电位器两边是否分别接了 3.3V 和 GND
 *    - 用万用表测量中间引脚电压是否随旋钮变化
 * 2. ADC 值跳动很大：
 *    - 这是正常现象，模拟读数会有一定噪声
 *    - 可以多次读取取平均值，或用 0.1uF 电容并在中间引脚和 GND 之间滤波
 * 3. LED 亮度变化不明显：
 *    - 检查 PWM 是否正确初始化（ledcAttach()）
 *    - 检查 map() 的目标范围是否正确（0~255）
 * 4. ADC 读数最大不是 4095：
 *    - 检查 analogReadResolution(12) 是否设置成功
 *    - 检查电位器是否接触良好
 * 5. 接光敏电阻时读数不对：
 *    - 光敏电阻需要搭分压电路，和固定电阻串联后中间接 ADC
 *    - 光照越强，光敏电阻阻值越小，分压值变化
 */

// 定义引脚
// POT_PIN 是电位器/模拟传感器接的引脚，这里用 GPIO34
// GPIO34 属于 ADC1，只能输入，非常适合做模拟读取
#define POT_PIN   34

// LED_PIN 是 LED 接的引脚，这里用 GPIO2
#define LED_PIN   2

// PWM 参数
// PWM_FREQ 是 PWM 频率，5000Hz 足够平滑
#define PWM_FREQ        5000

// PWM_RESOLUTION 是 PWM 分辨率，8 bit 对应占空比 0~255
#define PWM_RESOLUTION  8

void setup() {
  // 初始化串口，波特率 115200
  Serial.begin(115200);

  // 等待串口连接稳定
  delay(1000);

  Serial.println("================================");
  Serial.println("ESP32 教程 04：ADC 模拟输入 - 电位器调光");
  Serial.println("================================");

  // 设置 LED 引脚为 PWM 输出
  // ledcAttach(pin, freq, resolution) 让 GPIO2 输出 5kHz、8bit 的 PWM
  // 这样我们才能通过改变占空比来控制 LED 亮度
  ledcAttach(LED_PIN, PWM_FREQ, PWM_RESOLUTION);

  // ADC 引脚默认就是输入模式，不需要 pinMode
  // 但为了确保分辨率正确，可以显式设置
  // analogReadResolution(bits) 设置 ADC 分辨率
  // 12 位分辨率，读数范围 0~4095
  // 如果不设置，ESP32 默认也是 12 位，但显式设置更清晰
  analogReadResolution(12);

  // 也可以设置衰减倍数，控制可测量的电压范围
  // ADC_11db 是默认衰减，可测量约 0~3.3V
  // 如果输入信号很小，可以尝试 ADC_6db 或 ADC_0db
  // 初学者接电位器时不需要修改，保持注释状态即可
  // analogSetAttenuation(ADC_11db);

  Serial.println("旋转电位器，观察 LED 亮度和串口数值变化");
}

void loop() {
  // analogRead(pin) 读取指定 ADC 引脚的模拟值
  // 返回值范围由 analogReadResolution() 决定
  // 这里设置为 12 位，所以返回 0~4095
  // 0 对应 0V，4095 对应 3.3V（在默认衰减下）
  int adcValue = analogRead(POT_PIN);

  // map(value, fromLow, fromHigh, toLow, toHigh)：
  // 把数值从一个范围线性映射到另一个范围
  // 这里把 ADC 读数 0~4095 映射到 PWM 占空比 0~255
  // 这样电位器旋到最小时 LED 熄灭，旋到最大时 LED 最亮
  int pwmValue = map(adcValue, 0, 4095, 0, 255);

  // 输出 PWM 信号到 LED 引脚
  // pwmValue 越大，LED 越亮
  ledcWrite(LED_PIN, pwmValue);

  // 把数值打印到串口监视器
  // 可以看到电位器旋转时 ADC 值和 PWM 值的变化
  // Serial.print() 不会换行，Serial.println() 会换行
  Serial.print("ADC 原始值：");
  Serial.print(adcValue);
  Serial.print("  |  PWM 占空比：");
  Serial.println(pwmValue);

  // 每 100 毫秒读取一次
  // 如果读取和打印太快，串口输出会非常多，难以阅读
  // 100 毫秒是一个既能及时反映变化、又不会刷屏的平衡值
  delay(100);
}

/*
 * 扩展练习：
 * 1. 不接电位器，改接光敏电阻（需要搭一个分压电路），实现随光线自动调光。
 *    分压电路：3.3V → 光敏电阻 → GPIO34 → 10kΩ 固定电阻 → GND
 * 2. 用 map() 把 ADC 值映射到 0~100，打印成百分比亮度。
 * 3. 设置不同的 analogReadResolution()（比如 10 或 8），观察读数范围变化。
 * 4. 连续读取 10 次 ADC 值取平均，减少读数波动。
 * 5. 把 ADC 值同时映射到 PWM 和串口打印的电压值（单位：V），公式：voltage = adcValue * 3.3 / 4095。
 */
