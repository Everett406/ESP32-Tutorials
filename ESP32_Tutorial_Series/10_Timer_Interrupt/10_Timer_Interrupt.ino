/*
 * ESP32 教程系列 10：定时器中断 - 精确定时执行任务
 * 
 * 学习目标：
 * 1. 理解什么是硬件定时器、定时器中断和软件延时的区别
 * 2. 学会使用 ESP32 Arduino 3.x 的定时器 API
 * 3. 学会用定时器实现周期性任务，不阻塞主循环
 * 4. 理解定时器分频、计数周期、自动重载等概念
 * 
 * 前置知识：
 * - 已完成教程 09《外部中断》，理解中断、ISR、volatile 等基本概念
 * - 已完成教程 01《数字输出》，会用 digitalWrite() 控制 LED
 * 
 * 硬件连接：
 * - ESP32 GPIO2  →  LED 正极（长脚）
 * - LED 负极（短脚） →  330Ω 电阻  →  GND
 * - 本示例不依赖外部传感器，定时器是 ESP32 内部外设
 * 
 * 关键概念解释：
 * - 定时器（Timer）：单片机内部的一种硬件外设，它按照固定频率计数。
 *   当计数值达到设定值时，可以触发中断或执行某个动作。
 *   定时器比软件延时更精确，而且不会占用 CPU。
 * - 硬件定时器 vs 软件延时（delay()）：
 *   * delay() 会让 CPU 空转等待，期间不能执行其他代码，称为“阻塞”。
 *   * 硬件定时器由独立硬件模块驱动，到时间就触发中断，主循环完全自由。
 *   * 例如：用 delay(500) 实现 LED 闪烁，主循环里就不能同时做别的事；
 *     用定时器则可以让 LED 每 500ms 自动闪烁，主循环读取传感器、处理网络请求等。
 * - 分频（Prescaler / Divider）：把定时器的输入时钟频率降低。
 *   例如 ESP32 的定时器时钟源通常是 80MHz，分频成 1MHz 后，每 1 微秒计数一次。
 *   分频值越大，计数越慢，能定时的周期越长；分频值越小，精度越高。
 * - 计数周期：定时器从 0 开始计数，到达设定值后触发一次中断，称为一个周期。
 *   如果计数频率是 1MHz（每微秒 1 次），计数值设为 500000，
 *   那么周期就是 500000 微秒 = 500 毫秒。
 * - 自动重载（Auto-reload）：计数到达设定值后，是否自动从 0 重新开始计数。
 *   设为 true 可以产生周期性中断；设为 false 只触发一次，需要手动重启。
 * - 中断计数（Reload Count）：ESP32 定时器可以设置触发多少次后停止。
 *   设为 0 表示无限次循环触发。
 * - 微秒（μs，us）和毫秒（ms）：1 毫秒 = 1000 微秒。定时器常用微秒级精度。
 * - MHz（兆赫兹）：频率单位，1MHz = 1 000 000 次/秒。
 *   80MHz 表示每秒计数 8000 万次。
 * 
 * ESP32 Arduino 3.x 定时器 API：
 * - hw_timer_t *timer = timerBegin(频率)
 *   创建一个定时器，参数是目标计数频率（Hz）。
 *   例如 timerBegin(1000000) 表示计数频率 1MHz。
 * - timerAttachInterrupt(timer, ISR)
 *   把中断服务函数绑定到定时器。
 * - timerAlarm(timer, 计数值, 是否自动重载, 中断计数)
 *   设置定时器报警值，并启动定时器。
 * - timerWrite(timer, 值)：手动设置当前计数值。
 * - timerStop(timer)：停止定时器。
 * - timerRestart(timer)：重启定时器。
 * 
 * 注意：ESP32 Arduino 3.x 不再使用旧的 timerBegin(freq, divider, countUp)
 * 和 timerAlarmWrite() 写法，请使用本示例中的新 API。
 */

#define LED_PIN 2

// 定时器指针。hw_timer_t 是 ESP32 定时器的结构体类型，
// 具体细节由底层库实现，我们只需要保存它返回的指针。
hw_timer_t* timer = NULL;

// 定时器中断标志。中断服务函数把它设为 true，主循环据此执行动作。
// 和外部中断一样，共享变量要加 volatile。
volatile bool timerFlag = false;

// LED 闪烁计数。记录定时器触发了多少次。
volatile unsigned long blinkCount = 0;

// 定时器目标频率：1MHz，即每秒计数 100 万次，每微秒计数 1 次。
// 选择 1MHz 是因为 ESP32 定时器很容易分频到这个频率，且精度足够。
const uint32_t TIMER_FREQ = 1000000;

// 定时周期：500000 个计数。因为计数频率是 1MHz，所以就是 500000 微秒 = 500 毫秒。
// 如果想改成 1 秒，设为 1000000；想改成 100 毫秒，设为 100000。
const uint64_t ALARM_VALUE = 500000;

// 定时器中断服务函数。
// 这个函数会按照设定周期被硬件自动调用，这里每 500ms 调用一次。
void IRAM_ATTR onTimer() {
  // 中断里只做最简单的事：设置标志位和计数。
  // 不要在这里调用 delay、Serial.print 等函数。
  timerFlag = true;
  blinkCount++;
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("================================");
  Serial.println("ESP32 教程 10：定时器中断");
  Serial.println("================================");

  // 初始化 LED 引脚为输出模式，并默认熄灭。
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // timerBegin(频率) 创建一个硬件定时器，并返回它的指针。
  // 参数 1000000 表示让定时器以 1MHz 的频率计数。
  // 为什么用 1MHz？因为 1MHz 对应 1 微秒 1 次计数，方便我们把计数值直接理解成微秒。
  timer = timerBegin(TIMER_FREQ);

  // 绑定中断服务函数。
  // 第一个参数是定时器指针，第二个参数是 ISR 函数名（要带取地址符 &）。
  // 加上 IRAM_ATTR 的 ISR 在这里绑定后，定时器中断就能快速响应。
  timerAttachInterrupt(timer, &onTimer);

  // timerAlarm(timer, 计数值, 是否自动重载, 中断计数) 设置报警并启动定时器。
  // - 计数值 500000：计数到 500000 时触发中断，也就是 500ms。
  // - 自动重载 true：触发后自动从 0 重新计数，实现周期性中断。
  // - 中断计数 0：表示无限次触发，不会自动停止。
  //
  // 周期计算公式：
  // 周期（秒）= 计数值 / 计数频率
  // 0.5 秒 = 500000 / 1000000
  timerAlarm(timer, ALARM_VALUE, true, 0);

  Serial.println("定时器已启动，每 500ms 触发一次中断");
}

void loop() {
  // 检查定时器标志。主循环负责执行具体动作，中断只负责“通知”。
  if (timerFlag) {
    // 读取共享变量前关闭中断，防止读取过程中被 ISR 修改。
    noInterrupts();
    timerFlag = false;
    unsigned long count = blinkCount;
    interrupts();

    // 切换 LED 状态。
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));

    // 在主循环里打印信息，不要在中断里打印。
    Serial.print("定时器触发 #");
    Serial.print(count);
    Serial.print("，LED 状态：");
    Serial.println(digitalRead(LED_PIN) == HIGH ? "点亮" : "熄灭");
  }

  // 主循环可以做其他事情，不会被定时任务阻塞。
  // 比如这里可以读取传感器、处理网络请求、刷新显示屏等。
}

/*
 * 故障排查：
 * 1. LED 不闪烁：
 *    - 检查 timer 是否为 NULL，timerBegin 是否成功返回。
 *    - 检查 ALARM_VALUE 是否太大，导致周期很长，看起来像没反应。
 *    - 检查 LED 是否接在 GPIO2 且方向正确。
 * 2. 闪烁频率不对：
 *    - 确认 timerBegin 的参数是目标频率，不是分频系数。
 *    - 重新核对公式：周期 = 计数值 / 计数频率。
 * 3. 程序编译报错，提示 timerBegin 参数不匹配：
 *    - 你可能是用了旧版 ESP32 Arduino 2.x，或者安装了多个版本。
 *    - 本示例基于 ESP32 Arduino 3.x 的新 API，请确认安装的是 3.x 版本。
 * 4. 定时器不稳定或漏触发：
 *    - 检查 ISR 是否过于复杂，执行时间是否超过了定时周期。
 *    - 检查主循环里是否长时间关闭中断（noInterrupts）。
 * 
 * 扩展练习：
 * 1. 修改 ALARM_VALUE，把闪烁频率改成 1 秒、100 毫秒或 2 秒，
 *    观察实际效果并验证周期计算公式。
 * 2. 用定时器实现一个精确的秒表：每秒在串口打印一次累计时间。
 * 3. 使用多个定时器处理不同频率的任务，例如：
 *    - 定时器 0 每 100ms 读取一次传感器；
 *    - 定时器 1 每 5 秒通过 Wi-Fi 上报一次数据。
 * 4. 把自动重载设为 false，实现只触发一次的“一次性定时器”，
 *    触发后让 ESP32 进入深度睡眠（参考教程 15）。
 */
