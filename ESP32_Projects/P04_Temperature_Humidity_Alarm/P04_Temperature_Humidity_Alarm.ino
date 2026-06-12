/*
 * 综合项目 P04：温湿度报警器
 *
 * 学习目标：
 * 1. 理解 DHT22（AM2302）温湿度传感器的工作原理和接线方式
 * 2. 学会使用 Adafruit DHT 库读取温度和湿度
 * 3. 掌握根据阈值触发声光报警的逻辑
 * 4. 熟练使用串口输出调试信息，排查传感器问题
 *
 * 前置知识：
 * - 已完成教程 05《串口通信》，会使用 Serial.print() / Serial.println()
 * - 了解 digitalWrite() 控制 LED / 蜂鸣器（教程 01）
 *
 * 硬件连接：
 * - ESP32 GPIO4  →  DHT22 数据引脚（模块标 DATA / OUT）
 * - ESP32 3.3V   →  DHT22 VCC（+）
 * - ESP32 GND    →  DHT22 GND（-）
 * - ESP32 GPIO2  →  有源蜂鸣器正极 或 LED 正极（长脚）
 * - 蜂鸣器负极 / LED 负极（短脚） →  GND
 *   注意：如果接普通 LED，必须在负极串联 330Ω 限流电阻，否则 LED 容易烧毁。
 *
 * 关键概念解释：
 * - DHT22（也叫 AM2302）：一种单总线数字温湿度传感器，能同时输出温度
 *   （精度 ±0.5 °C）和湿度（精度 ±2% ~ ±5%）。它的数据引脚通过一根线
 *   与 ESP32 通信，所以叫“单总线”。两次读取之间至少要间隔 2 秒，
 *   否则会得到上一次的老数据或读取失败。
 * - 有源蜂鸣器（Active Buzzer）：内部自带振荡电路，只要给正极通上高电平
 *   （比如 3.3V），它就会持续发出声音。适合用来做简单的报警提示。
 *   而无源蜂鸣器需要外部不断切换高低电平才能发声，本项目不选它。
 * - 阈值（Threshold）：一个判断条件。当温度或湿度超过这个设定值时，
 *   就触发报警。阈值可以根据实际环境需要修改。
 */

// 引入 DHT 库。DHT 库封装了单总线通信的复杂时序，
// 让我们只需要调用 readTemperature()、readHumidity() 就能得到结果。
// 在 Arduino IDE 的“库管理器”中搜索并安装：
// 1. “DHT sensor library” by Adafruit
// 2. “Adafruit Unified Sensor”（DHT 库依赖它）
#include <DHT.h>

// DHT22 数据引脚。GPIO4 是一个普通 GPIO，不会受到 Wi-Fi / ADC 等特殊功能影响，
// 用来接单总线设备比较稳定。
#define DHT_PIN 4

// 报警引脚，接有源蜂鸣器或 LED。GPIO2 是大多数 ESP32 开发板板载 LED 所在引脚，
// 即使不接外部 LED，也能通过板载 LED 观察报警状态。
#define ALARM_PIN 2

// 告诉 DHT 库我们接的是 DHT22，而不是 DHT11 或其他型号。
#define DHT_TYPE DHT22

// 报警阈值。温度超过 30.0 °C 或湿度超过 70.0% 就报警。
// 这两个数字可以根据你的房间环境自由调整。
#define TEMP_THRESHOLD 30.0
#define HUMI_THRESHOLD 70.0

// 读取间隔。DHT22 每次测量需要约 2 秒，如果读得太快会失败，
// 所以这里设为 2000 毫秒（2 秒）。
#define READ_INTERVAL 2000

// 创建 DHT 对象。第一个参数是数据引脚，第二个参数是传感器型号。
// 对象会在 setup() 中被初始化，然后在 loop() 中反复读取。
DHT dht(DHT_PIN, DHT_TYPE);

// 记录上一次读取传感器的时间，配合 millis() 实现非阻塞定时。
// 这样做的好处是：即使传感器读取失败，程序也不会卡在 delay() 里，
// 主循环可以继续做其他事情。
unsigned long lastReadTime = 0;

void setup() {
  // 初始化串口，波特率 115200。上传代码后，在 Arduino IDE 串口监视器
  // 里也要选择 115200，否则看到的是乱码。
  Serial.begin(115200);

  // 等待串口稳定，避免最开始的一两条信息打印不出来。
  delay(1000);

  Serial.println("==============================");
  Serial.println("P04：温湿度报警器");
  Serial.println("==============================");

  // 把报警引脚设为输出模式。我们要让 ESP32 主动驱动蜂鸣器 / LED，
  // 所以必须是 OUTPUT。如果是 INPUT，引脚只能读取外部信号。
  pinMode(ALARM_PIN, OUTPUT);

  // 初始状态关闭报警，防止一上电就响。
  digitalWrite(ALARM_PIN, LOW);

  // 初始化 DHT 传感器。这会把数据引脚设置为正确的输入/输出状态，
  // 并发送启动信号让传感器进入就绪状态。
  dht.begin();

  Serial.println("DHT22 初始化完成");
  Serial.print("读取间隔：");
  Serial.print(READ_INTERVAL);
  Serial.println(" 毫秒");
  Serial.print("温度阈值：");
  Serial.print(TEMP_THRESHOLD);
  Serial.println(" °C");
  Serial.print("湿度阈值：");
  Serial.print(HUMI_THRESHOLD);
  Serial.println(" %");
}

void loop() {
  // millis() 返回程序启动以来经过的毫秒数。用它来做定时，比 delay() 更灵活，
  // 不会阻塞其他代码。
  unsigned long now = millis();

  // 每隔 READ_INTERVAL（2 秒）读取一次传感器。
  if (now - lastReadTime >= READ_INTERVAL) {
    lastReadTime = now;

    // readTemperature() 读取摄氏温度，readHumidity() 读取相对湿度。
    // 注意：这两个函数返回的是 float（浮点数），可能带有小数。
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();

    // isnan() 用来判断读取结果是不是“不是一个数字”。
    // 当传感器没有接好、断电、或者读取太快时，DHT 库会返回 NaN，
    // 这时要立即关闭报警并提示用户检查接线，避免用错误数据做判断。
    if (isnan(temperature) || isnan(humidity)) {
      Serial.println("DHT22 读取失败，请检查电源、接线和读取间隔");
      digitalWrite(ALARM_PIN, LOW);
      return;
    }

    // 把温湿度打印到串口，方便观察实时数据。
    Serial.print("温度：");
    Serial.print(temperature);
    Serial.print(" °C  |  湿度：");
    Serial.print(humidity);
    Serial.println(" %");

    // 如果温度或湿度超过阈值，就打开蜂鸣器 / LED 报警。
    // 逻辑上用的是“或”（||），只要有一个条件满足就报警。
    if (temperature > TEMP_THRESHOLD || humidity > HUMI_THRESHOLD) {
      Serial.println("⚠ 警告：温湿度超过阈值！");
      digitalWrite(ALARM_PIN, HIGH);
    } else {
      // 温湿度正常时关闭报警。
      digitalWrite(ALARM_PIN, LOW);
    }
  }
}

/*
 * 扩展挑战：
 * 1. 把报警改成“连续三秒都超过阈值才报警”，避免偶尔的尖峰误触发。
 *    提示：增加一个计数器，每次正常就清零，连续异常达到一定次数再 HIGH。
 * 2. 用 PWM 控制蜂鸣器或 LED 的“强弱”，距离阈值越近报警越轻微，
 *    远超阈值时报警最强烈。
 * 3. 把阈值改成可以通过串口命令动态修改，例如发送 "temp=35" 就修改温度阈值。
 */
