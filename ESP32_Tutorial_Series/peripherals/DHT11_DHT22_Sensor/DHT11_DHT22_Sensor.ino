/*
 * 教程：DHT11 / DHT22 温湿度传感器
 *
 * 学习目标：
 * 1. 了解什么是温湿度传感器，以及 DHT11 和 DHT22 的区别。
 * 2. 学会给 Arduino IDE 安装第三方库。
 * 3. 理解单总线（单线）通信的基本概念。
 * 4. 掌握 DHT 库的用法，读取温度和湿度并打印到串口。
 *
 * 需要安装的库：
 * - "DHT sensor library" by Adafruit（DHT 传感器库）
 * - "Adafruit Unified Sensor" by Adafruit（DHT 库的依赖，必须一起安装）
 * 安装方法：
 * 1. 打开 Arduino IDE，点击菜单栏 项目 → 加载库 → 管理库...。
 * 2. 在搜索框输入 "DHT sensor library"，找到作者为 Adafruit 的那一项，点击安装。
 * 3. 安装过程中会提示安装依赖 "Adafruit Unified Sensor"，请一并安装。
 *
 * 硬件连接：
 * - DHT 传感器 VCC  →  ESP32 3.3V
 * - DHT 传感器 GND  →  ESP32 GND
 * - DHT 传感器 DATA →  ESP32 GPIO4
 *
 * 注意：
 * DHT22 可以直接用 3.3V 供电；
 * 部分 DHT11 模块也支持 3.3V，如果读数异常，可以尝试改用 5V 供电，
 * 但 DATA 引脚仍然要接到 ESP32 的 3.3V 容忍 GPIO 上。
 * 建议在 DATA 和 3.3V 之间接一个 4.7kΩ ~ 10kΩ 的上拉电阻，
 * 让空闲时数据线保持高电平，通信更稳定。
 */

// 引入 DHT 库，它封装了复杂的单总线通信时序，我们只需要调用简单函数。
#include "DHT.h"

// DHT_PIN：连接传感器 DATA 引脚的 ESP32 GPIO 编号。
// 这里选 GPIO4，因为 GPIO4 是通用 GPIO，3.3V 容忍，且位置方便。
// 你也可以换成其他 GPIO，但要在下面两处一起改。
#define DHT_PIN 4

// DHT_TYPE：告诉程序我们用的是哪款传感器。
// DHT22 精度更高（温度 ±0.5℃，湿度 ±2%），量程更大，推荐使用。
// 如果你手头只有 DHT11，请改成 #define DHT_TYPE DHT11。
#define DHT_TYPE DHT22

// 用 DHT_PIN 和 DHT_TYPE 创建一个 DHT 对象。
// 这个对象会帮我们完成初始化、读取、校验等工作。
DHT dht(DHT_PIN, DHT_TYPE);

void setup() {
  // Serial.begin(baudrate) 初始化串口通信，波特率设为 115200。
  // 波特率是数据传输速度，串口监视器也要选同样的 115200，否则看到的是乱码。
  Serial.begin(115200);

  // 等待一小段时间，让串口监视器来得及打开，避免错过最开始的输出。
  // 在一些主控上不是必须的，但对初学者调试更友好。
  delay(1000);

  // dht.begin() 初始化 DHT 传感器。
  // 它会设置 DATA 引脚为输入/输出状态，并准备开始通信。
  dht.begin();

  Serial.println("DHT 传感器初始化完成");
  Serial.println("每 2 秒读取一次温湿度...");
}

void loop() {
  // DHT 传感器转换一次数据大约需要 250ms，官方建议两次读取间隔不少于 2 秒。
  // 如果读得太快，会得到错误值或 "nan"。
  delay(2000);

  // readHumidity() 读取相对湿度，单位是 %RH（Relative Humidity）。
  // 返回值是 float 类型，表示空气中水蒸气含量占饱和水蒸气的百分比。
  float humidity = dht.readHumidity();

  // readTemperature() 读取温度，默认返回摄氏度（℃）。
  // 如果要华氏度，可以用 dht.readTemperature(true)。
  float temperature = dht.readTemperature();

  // isNaN() 判断读数是不是 "Not a Number"（不是一个有效数字）。
  // 当传感器未接好、通信失败、读数间隔太短时，函数会返回 nan。
  // 这里加判断，避免把无效值显示成 0.00 误导初学者。
  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("读取失败！请检查接线、上拉电阻和传感器供电。");
    return;  // 结束本次 loop，2 秒后再次尝试。
  }

  // 计算体感温度（Heat Index），它综合考虑了温度和湿度对人体的影响。
  // computeHeatIndex 需要摄氏温度，最后一个参数传 false。
  float heatIndex = dht.computeHeatIndex(temperature, humidity, false);

  // 输出结果到串口监视器，\t 是制表符，让数据对齐。
  Serial.print("温度: ");
  Serial.print(temperature);
  Serial.print(" ℃\t");

  Serial.print("湿度: ");
  Serial.print(humidity);
  Serial.print(" %\t");

  Serial.print("体感温度: ");
  Serial.print(heatIndex);
  Serial.println(" ℃");
}

/*
 * 扩展练习：
 * 1. 把 DHT22 换成 DHT11，修改 DHT_TYPE，观察精度差异。
 * 2. 当温度超过 30℃ 或湿度超过 70% 时，点亮一颗 LED 报警。
 * 3. 把读取到的数据通过 HTTP 请求发送给服务器，做远程温湿度监控。
 */
