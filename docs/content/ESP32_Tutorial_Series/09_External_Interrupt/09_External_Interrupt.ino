/*
 * ESP32 教程系列 09：外部中断 - 用按键触发事件
 * 
 * 学习目标：
 * 1. 理解什么是中断、中断服务函数（ISR）和“异步”处理
 * 2. 学会使用 attachInterrupt() 配置 ESP32 的外部中断
 * 3. 理解 volatile 关键字的作用，以及为什么中断里要慎用 delay/Serial
 * 4. 掌握机械按键的硬件消抖和软件消抖基本思路
 * 
 * 前置知识：
 * - 已完成教程 02《数字输入与按键》，了解 pinMode(INPUT_PULLUP) 和 digitalRead()
 * - 已完成教程 04《读取模拟量》，了解程序是顺序执行的
 * 
 * 硬件连接：
 * - LED 正极 → ESP32 GPIO2，LED 负极 → 330Ω 电阻 → GND
 * - 按键一端 → ESP32 GPIO4，按键另一端 → GND
 * 
 * 为什么要用 INPUT_PULLUP（内部上拉）？
 * 按键没有按下时，GPIO4 通过 ESP32 内部的一个大电阻被拉到 3.3V，读取为高电平 HIGH。
 * 按键按下时，GPIO4 直接接到 GND，读取为低电平 LOW。
 * 这样只需要两根线连接按键（GPIO4 和 GND），不用外接上拉电阻，接线更简单。
 * 如果不启用上拉，按键未按下时引脚悬空，电平会飘忽不定，造成误判。
 * 
 * 关键概念解释：
 * - 中断（Interrupt）：CPU 正在执行主程序时，如果某个事件（比如按键按下）
 *   发生，CPU 会“暂停”当前任务，转去执行一段专门的处理代码，处理完再回来继续
 *   执行原来的任务。这个机制让紧急事件得到及时响应。
 * - 中断服务函数（ISR，Interrupt Service Routine）：中断发生时自动调用的函数。
 *   也叫“中断处理程序”。它要尽量简短、快速执行，不能阻塞。
 * - 异步：主程序不需要一直轮询按键状态，按键按下的时刻会“通知”CPU。
 *   这和“同步轮询”正好相反，可以节省 CPU 资源。
 * - 轮询（Polling）：主程序反复用 digitalRead() 读取按键状态。简单但浪费 CPU，
 *   如果主循环里有长时间 delay，可能错过按键动作。
 * - 触发模式（Interrupt Mode）：决定中断在什么时刻发生：
 *   * RISING：上升沿触发，从 LOW 变为 HIGH 的瞬间触发一次。
 *   * FALLING：下降沿触发，从 HIGH 变为 LOW 的瞬间触发一次。
 *   * CHANGE：电平变化触发，上升沿和下降沿都会触发。
 *   * HIGH：高电平持续触发（ESP32 部分引脚支持，通常少用）。
 *   * LOW：低电平持续触发（ESP32 部分引脚支持，通常少用）。
 * - 消抖（Debouncing）：机械按键按下/松开时，金属触点会快速抖动几次，
 *   导致电平在短时间内反复变化。如果不处理，一次按键可能被识别成多次。
 *   解决方法有硬件消抖（并联电容）和软件消抖（延时或记录时间戳）。
 * - volatile 关键字：告诉编译器“这个变量可能会在意想不到的地方被修改”，
 *   不要对它做优化。中断服务函数和主循环会共享变量，必须加 volatile，
 *   否则主循环可能永远读不到中断里更新的值。
 * - IRAM_ATTR：一个编译器属性，告诉编译器把这个函数放到 ESP32 的 IRAM
 *   （指令 RAM）中执行。Flash 读取可能较慢，中断要求快速响应，放在 IRAM
 *   可以减少延迟。ESP32 的中断服务函数通常都要加 IRAM_ATTR。
 * - IRAM：ESP32 内部的高速 RAM，用来存放需要快速执行的代码。普通代码放在
 *   Flash 中，读取速度比 IRAM 慢。
 * - noInterrupts() / interrupts()：分别用于关闭和打开全局中断。在修改被中断
 *   共享的变量时，先关闭中断可以防止 ISR 突然插入导致数据不一致。
 */

#define LED_PIN     2
#define BUTTON_PIN  4

// 按键按下标志位。中断服务函数把它设为 true，主循环检测到后处理。
// 必须加 volatile，因为中断里会修改它。
volatile bool buttonPressed = false;

// 中断计数器。记录按键中断触发的总次数，用来观察消抖效果。
// unsigned long 是无符号长整型，范围很大，适合计数。
volatile unsigned long interruptCount = 0;

// 上一次触发的时间戳，用于软件消抖。
// 单位是毫秒，记录上一次真正处理按键动作的时刻。
volatile unsigned long lastDebounceTime = 0;

// 消抖间隔。机械按键的抖动通常在 20~50 毫秒内结束，
// 这里设为 200 毫秒，既过滤掉抖动，又保证用户连续按键能被识别。
const unsigned long DEBOUNCE_DELAY = 200;

// 中断服务函数（ISR）。
// 当 GPIO4 的电平出现 FALLING（下降沿）时，这个函数会被硬件自动调用。
// 函数名可以自取，但参数和返回值都必须是 void。
void IRAM_ATTR onButtonPress() {
  // IRAM_ATTR 表示把这个函数放在 IRAM 中执行，减少中断响应延迟。

  // 获取当前时间。millis() 返回程序启动以来的毫秒数，不会受 delay 影响。
  unsigned long now = millis();

  // 软件消抖：如果距离上次处理的时间小于 DEBOUNCE_DELAY，就忽略这次触发。
  // 这是最简单的消抖策略，适合大多数情况。
  if (now - lastDebounceTime < DEBOUNCE_DELAY) {
    return;
  }
  lastDebounceTime = now;

  // 中断里只做最少的工作：设置标志位和计数。
  // 不要在这里做 digitalWrite、Serial.print、delay 等耗时或可能阻塞的操作。
  buttonPressed = true;
  interruptCount++;
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("================================");
  Serial.println("ESP32 教程 09：外部中断");
  Serial.println("================================");

  // 初始化 LED 引脚为输出模式。
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // 初始化按键引脚为输入上拉模式。
  // 按键未按下时为 HIGH，按下时为 LOW。
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // attachInterrupt(pin, ISR, mode) 把外部中断绑定到指定引脚：
  // - pin：引脚编号。用 digitalPinToInterrupt(BUTTON_PIN) 转换成中断号。
  // - ISR：中断服务函数名，这里是我们自己写的 onButtonPress。
  // - mode：触发模式，这里用 FALLING，即 HIGH → LOW 的瞬间触发。
  //
  // 为什么用 FALLING？
  // 因为按键使用了内部上拉，未按下时是 HIGH，按下时变成 LOW。
  // 按下瞬间是下降沿，所以用 FALLING 最合适。
  // 如果用 RISING，则会在松开时触发；如果用 CHANGE，按下和松开都会触发。
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), onButtonPress, FALLING);

  Serial.println("按下按键会触发中断，LED 会切换状态");
}

void loop() {
  // 检查中断标志。如果中断服务函数把 buttonPressed 设为 true，
  // 主循环就在这里处理具体逻辑。
  if (buttonPressed) {
    // 先关闭全局中断，防止在读取/修改变量时 ISR 又插入，造成数据不一致。
    // 这在多字节变量或结构体共享时尤其重要。
    noInterrupts();
    buttonPressed = false;
    unsigned long count = interruptCount;
    interrupts();

    // 中断里不做打印，主循环里做。Serial.print 可能阻塞，不适合放在 ISR 中。
    Serial.print("按键中断触发！累计次数：");
    Serial.println(count);

    // 切换 LED 状态。
    // digitalRead(LED_PIN) 读取当前电平，取反后再写回去，实现亮灭切换。
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
  }

  // 主循环可以做其他事情。这里用 delay(100) 模拟一个周期性任务，
  // 同时也证明：即使主循环在 delay，外部中断仍然能及时响应。
  delay(100);
}

/*
 * 故障排查：
 * 1. 按一次按键 LED 切换多次：
 *    - 这是典型的按键抖动现象。检查 DEBOUNCE_DELAY 是否设置得太小。
 *    - 也可以尝试在按键两端并联一个 0.1uF 电容做硬件消抖。
 * 2. 按键没有反应：
 *    - 检查按键是否接在 GPIO4 和 GND 之间，有没有接反或虚焊。
 *    - 检查 attachInterrupt 的引脚是否支持外部中断（ESP32 几乎所有 GPIO 都支持）。
 *    - 检查触发模式是否选对：FALLING 要求按下时是下降沿。
 * 3. 串口打印次数比实际按键多：
 *    - 可能是中断触发模式设为 CHANGE 了，按下和松开都会计数。
 *    - 也可能是按键接线不稳，导致电平反复跳动。
 * 4. 程序崩溃或重启：
 *    - 检查 ISR 里是否调用了 delay、Serial.print、malloc 等不允许的函数。
 *    - 检查是否对共享变量加了 volatile 保护。
 * 
 * 扩展练习：
 * 1. 把触发模式改成 CHANGE，分别统计按下和松开的次数。
 * 2. 用 micros() 实现更精确的软件消抖。
 * 3. 用外部中断读取旋转编码器，理解 A/B 相脉冲的编码原理。
 * 4. 尝试把 LED 状态也做成 volatile 变量，在中断里直接切换 LED，
 *    观察这样做和主循环处理有什么区别。
 */
