/*
 * ESP32 教程系列 02：数字输入 - 按键控制 LED
 *
 * 学习目标：
 * 1. 理解数字输入和数字输出的区别
 * 2. 理解什么是上拉电阻，以及 INPUT_PULLUP 的内部上拉用法
 * 3. 学会用 digitalRead() 读取引脚状态
 * 4. 理解机械按键的"抖动"现象及简单消抖方法
 * 5. 学会用变量记录状态，实现按键切换 LED
 *
 * 前置知识：
 * - 已完成教程 01，理解 GPIO、HIGH/LOW、pinMode()、digitalWrite()、delay()
 *
 * 关键概念解释：
 *
 * 【数字输入 vs 数字输出】
 * - 数字输出：ESP32 主动控制引脚电平（HIGH/LOW），用来驱动 LED、蜂鸣器等
 * - 数字输入：ESP32 读取外部设备的电平状态，用来检测按键、传感器等
 * 输入模式下，引脚不会主动输出电压，而是"观察"外部电路把引脚拉到 HIGH 还是 LOW。
 *
 * 【什么是上拉电阻？】
 * 按键有两个状态：按下和松开。如果没有上拉电阻，当按键松开时，
 * 引脚悬空，容易受到周围电磁干扰，读到的值可能是 HIGH 也可能是 LOW，不稳定。
 * 上拉电阻的作用是把引脚"默认拉高高"：
 * - 按键未按下时：引脚通过上拉电阻连接到 3.3V，读到稳定的 HIGH
 * - 按键按下时：引脚直接连接到 GND，被拉低到 LOW
 * 这样我们就能明确区分按键是否被按下。
 *
 * 上拉电阻可以有两种实现方式：
 * 1. 外部上拉：在按键和 3.3V 之间接一个 10kΩ 左右的电阻。
 *    具体接法：
 *    - 电阻一端接 ESP32 的 3.3V 引脚
 *    - 电阻另一端接按键的一端，同时也接 ESP32 的 GPIO4
 *    - 按键的另一端接 GND
 *    这样按键未按下时，GPIO4 通过 10kΩ 电阻连到 3.3V，读到 HIGH；
 *    按键按下时，GPIO4 直接连到 GND，读到 LOW。
 * 2. 内部上拉：ESP32 芯片内部已经集成了上拉电阻，用 INPUT_PULLUP 即可启用。
 * 本教程使用内部上拉，可以省去外部电阻，接线更简单。
 * （如果你以后遇到按键不稳定、容易误触发的情况，可以试试改成外部上拉，
 *  外部上拉电阻通常比内部上拉更稳定。）
 *
 * 【按键抖动】
 * 机械按键内部是金属触点，按下和松开时触点会快速颤动几次，
 * 导致电平在 HIGH 和 LOW 之间快速跳变，持续几毫秒到十几毫秒。
 * 如果不处理，程序可能把一次按键识别成多次，出现 LED 状态乱跳的现象。
 * 对于"计数器"这种场景，影响尤其大：按一下可能被计成 3~5 次。
 * 处理方法叫"消抖"（Debounce），常用方法有两种：
 * 1. 延时消抖：检测到按键状态变化后，等待一小段时间（比如 50ms），等抖动过去再读取。
 *    你的理解基本是对的：状态一变，先等 50ms 不处理，然后再看稳定后的状态。
 *    不过更准确地说，不是"50ms 不读取"，而是"50ms 后再读取一次"，
 *    如果还是按下的状态，就认为这次按键是真的。
 * 2. 时间戳消抖：用 millis() 记录上次触发时间，间隔超过一定值才认为有效（更精确）。
 *    millis() 和 delay() 的区别：
 *    - delay(200)：程序在这里停 200ms，期间什么都不做
 *    - millis()：返回从上电到现在经过的毫秒数，程序不停，可以继续做别的事
 *    所以 millis() 消抖更适合复杂程序，delay() 消抖简单但会阻塞其他任务。
 * 本示例先用延时消抖，便于理解概念。
 *
 * 【bool 类型】
 * bool 是布尔类型，只有两种取值：true（真）或 false（假）。
 * 在这里我们用 ledState 表示 LED 的当前状态：
 * - true 表示 LED 亮
 * - false 表示 LED 灭
 * 用 bool 比用 0 和 1 更直观。
 *
 * 【?: 三元运算符】
 * 表达式：条件 ? 值A : 值B
 * 含义：如果条件为真，返回值A；否则返回值B。
 * 例如 ledState ? HIGH : LOW 表示：
 * - 如果 ledState 是 true，返回 HIGH
 * - 如果 ledState 是 false，返回 LOW
 * 这是一种简洁写法，和下面的 if-else 效果完全相同：
 *   if (ledState) { return HIGH; } else { return LOW; }
 * 初学者如果觉得三元运算符绕，完全可以先用 if-else，熟练后再用 ?:。
 *
 * 硬件连接：
 * - LED 正极（长脚） →  ESP32 GPIO2
 * - LED 负极（短脚） →  330Ω 电阻  →  ESP32 GND
 *
 * - 按键一端        →  ESP32 GPIO4
 * - 按键另一端      →  ESP32 GND
 *   （使用内部上拉，不需要外部电阻）
 *
 * 为什么 LED 要串联电阻？
 * LED 导通后内阻很小，直接接 3.3V 会产生过大电流，可能烧坏 LED 或 ESP32 引脚。
 * 330Ω 电阻把电流限制在安全范围内。
 *
 * 常见问题与排查：
 * 1. 按键没反应：
 *    - 检查按键引脚是否接在 GPIO4
 *    - 检查按键另一端是否接到了 GND，而不是 3.3V
 *    - 如果接到 3.3V，按键按下时读到的是 HIGH，而程序判断的是 LOW，逻辑就反了
 * 2. 按一下按键 LED 切换多次（抖动）：
 *    - 增加 delay(200) 的时间，比如改成 300 或 500
 *    - 进阶做法：改用 millis() 时间戳消抖
 * 3. LED 一直亮或一直灭：
 *    - 检查 LED 正负极是否接反
 *    - 检查 ledState 初始值和 digitalWrite 是否对应
 * 4. 串口监视器看不到信息：
 *    - 确认波特率设置为 115200
 */

// 定义引脚
// LED_PIN 是 LED 输出引脚，这里用 GPIO2
#define LED_PIN     2

// BUTTON_PIN 是按键输入引脚，这里用 GPIO4
// 选择 GPIO4 的原因是它支持内部上拉，且不会和启动时的特殊功能冲突
#define BUTTON_PIN  4

// 记录 LED 当前状态
// bool 类型只有 true/false 两个值
// 初始设为 false，表示程序开始时 LED 是熄灭状态
// 用变量记录状态的原因是：我们要用按键"切换"LED，而不是只按住才亮。
// 如果不记录状态，就无法知道当前是亮还是灭，也就无法在"亮"和"灭"之间切换。
bool ledState = false;

void setup() {
  // 初始化串口，波特率 115200
  // 串口用于把调试信息发送到电脑，方便观察按键和 LED 状态变化
  Serial.begin(115200);

  // 等待串口连接稳定，避免错过开机信息
  delay(1000);

  Serial.println("================================");
  Serial.println("ESP32 教程 02：数字输入 - 按键控制 LED");
  Serial.println("================================");

  // 设置 LED 引脚为输出模式
  // OUTPUT 表示 ESP32 要主动控制这个引脚的高低电平
  pinMode(LED_PIN, OUTPUT);

  // 设置按键引脚为输入模式，并启用内部上拉电阻
  // INPUT_PULLUP 的含义：
  // - 引脚内部通过一个约 30kΩ~50kΩ 的电阻连接到 3.3V
  // - 当按键未按下时，引脚被内部电阻拉到 3.3V，读到 HIGH
  // - 当按键按下时，引脚直接连到 GND，被外部电路拉低，读到 LOW
  // 使用 INPUT_PULLUP 可以省去外部上拉电阻，简化电路
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // 根据 ledState 的初始值设置 LED 状态
  // ?: 是三元运算符：条件 ? 真值 : 假值
  // ledState 为 false，所以 digitalWrite 收到 LOW，LED 初始熄灭
  digitalWrite(LED_PIN, ledState ? HIGH : LOW);

  Serial.println("初始化完成，按下按键切换 LED 状态");
}

void loop() {
  // digitalRead(pin) 读取指定数字引脚的状态，返回值是 HIGH 或 LOW
  // 这里读取按键引脚的状态
  // 因为用了内部上拉，按键未按下时是 HIGH，按下时是 LOW
  // digitalRead() 的返回值类型是 int，所以用 int 接收最自然。
  // 实际上它只返回 HIGH 或 LOW 两个值，理论上也可以用 bool 或 uint8_t 接收：
  //   bool buttonState = digitalRead(BUTTON_PIN);
  //   uint8_t buttonState = digitalRead(BUTTON_PIN);
  // 用 int 是 Arduino 官方示例中最常见的写法，对初学者最友好。
  int buttonState = digitalRead(BUTTON_PIN);

  // 判断按键是否被按下
  // 由于内部上拉，按键按下时引脚被拉低到 GND，所以 buttonState == LOW 表示按下
  if (buttonState == LOW) {
    // 按键被按下了，在串口打印提示
    Serial.println("按键被按下");

    // 切换 LED 状态
    // ! 是逻辑非运算符，ledState = !ledState 表示把 true 变 false，false 变 true
    // 这样每按一次按键，LED 就在"亮"和"灭"之间切换一次
    ledState = !ledState;

    // 根据新的 ledState 控制 LED
    // 如果 ledState 是 true，输出 HIGH 点亮 LED；否则输出 LOW 熄灭 LED
    digitalWrite(LED_PIN, ledState ? HIGH : LOW);

    // 打印当前 LED 状态，方便调试
    Serial.print("LED 状态：");
    Serial.println(ledState ? "点亮" : "熄灭");

    // 延时消抖
    // 机械按键在按下和松开时会有几毫秒到十几毫秒的抖动
    // 这段时间内触点快速通断，digitalRead 可能读到多次 LOW
    // 简单方法：检测到按键后延时 200ms，等待抖动过去
    // 更精确的方法可以用 millis() 记录时间，但初学者先用 delay 理解概念
    delay(200);

    // 等待按键释放，避免一直触发
    // 当按键还按着时，digitalRead 一直为 LOW，程序停在这里循环等待
    // 直到用户松开按键，buttonState 变成 HIGH，才退出 while 循环
    while (digitalRead(BUTTON_PIN) == LOW) {
      delay(10);
    }

    // 再次延时消抖（松开时的抖动）
    delay(200);
  }
}

/*
 * 扩展练习：
 * 1. 把按键改成"按住才亮，松开就灭"：不再切换 ledState，直接根据 buttonState 控制 LED。
 * 2. 增加一个按键分别控制"开"和"关"，实现两个按键控制一盏灯。
 * 3. 用 millis() 实现不阻塞的按键消抖，让程序在等待按键时也能做其他事情。
 * 4. 在每次按键时让 LED 切换亮灭的同时，让串口打印按下的次数。
 */
