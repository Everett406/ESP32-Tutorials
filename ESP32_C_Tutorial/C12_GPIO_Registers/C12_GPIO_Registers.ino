/*
 * ESP32 学 C 语言 12：直接操作 GPIO 寄存器
 *
 * 学习目标：
 * 1. 理解什么是寄存器
 * 2. 学会用 C 语言直接读写 ESP32 的 GPIO 寄存器
 * 3. 理解库函数（如 digitalWrite）底层做了什么
 * 4. 了解直接操作寄存器的风险和适用场景
 *
 * 什么是寄存器？
 * 寄存器是芯片内部的一种特殊存储单元，每个寄存器都有固定的地址。
 * 通过读写这些地址，可以控制芯片的各种硬件功能，比如 GPIO、定时器、串口等。
 *
 * 为什么要直接操作寄存器？
 * 1. 速度更快：不需要经过库函数的多层封装
 * 2. 更灵活：可以控制库函数没有暴露的功能
 * 3. 学习底层：理解单片机的工作原理
 *
 * 什么时候不要直接操作寄存器？
 * 1. 普通项目用 Arduino 库函数就够了，代码更易读
 * 2. 寄存器写错可能导致硬件异常
 * 3. 不同芯片的寄存器地址不同，可移植性差
 */

#include <stdio.h>
#include <soc/gpio_struct.h>  // ESP32 GPIO 寄存器定义

#define LED_PIN 2

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("ESP32 学 C 语言 12：直接操作 GPIO 寄存器");

  // ========== 方法 1：使用 Arduino 库函数 ==========

  Serial.println("\n===== 方法 1：Arduino 库函数 =====");

  // 这是我们最熟悉的方式
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  Serial.println("用 digitalWrite 点亮 LED");
  delay(500);
  digitalWrite(LED_PIN, LOW);
  Serial.println("用 digitalWrite 熄灭 LED");
  delay(500);

  // ========== 方法 2：直接操作寄存器 ==========

  Serial.println("\n===== 方法 2：直接操作 GPIO 寄存器 =====");

  // GPIO 的输出使能寄存器（GPIO_ENABLE_REG）
  // 把第 LED_PIN 位设置为 1，表示这个引脚是输出模式
  // 相当于 pinMode(LED_PIN, OUTPUT)
  GPIO.enable = (1ULL << LED_PIN);

  // GPIO 输出置位寄存器（GPIO_OUT_W1TS_REG）
  // 写 1 表示把对应引脚输出高电平
  GPIO.out_w1ts = (1ULL << LED_PIN);
  Serial.println("用寄存器点亮 LED");
  delay(500);

  // GPIO 输出清零寄存器（GPIO_OUT_W1TC_REG）
  // 写 1 表示把对应引脚输出低电平
  GPIO.out_w1tc = (1ULL << LED_PIN);
  Serial.println("用寄存器熄灭 LED");
  delay(500);

  // ========== 解释寄存器操作的含义 ==========

  Serial.println("\n===== 寄存器操作解释 =====");

  // 1ULL 表示 unsigned long long 类型的 1，占 64 位
  // 左移 LED_PIN 位后，得到只在 LED_PIN 位为 1 的数
  // 例如 LED_PIN = 2 时：
  // 1ULL << 2 = 0b0000...00000100 = 4
  uint64_t bitMask = (1ULL << LED_PIN);
  Serial.print("LED_PIN 对应的位掩码：");
  Serial.println((uint32_t)bitMask);

  // GPIO.enable 是 GPIO 使能寄存器
  // 每一位对应一个 GPIO，写 1 使能输出
  // 这里用赋值的方式只设置 LED_PIN 位，其他位清零
  // 实际项目中如果要保留其他引脚状态，应该使用 |=

  // GPIO.out_w1ts 是"写 1 置位"寄存器
  // 特点是：写 1 会把对应位置 1，写 0 不影响其他位
  // 所以用 GPIO.out_w1ts = bitMask; 可以只把 LED_PIN 置高

  // GPIO.out_w1tc 是"写 1 清零"寄存器
  // 特点是：写 1 会把对应位清 0，写 0 不影响其他位
  // 所以用 GPIO.out_w1tc = bitMask; 可以只把 LED_PIN 置低

  // ========== 读取 GPIO 输入状态 ==========

  Serial.println("\n===== 读取 GPIO 输入寄存器 =====");

  // 先把 LED_PIN 设为输入模式
  // 相当于 pinMode(LED_PIN, INPUT)
  GPIO.enable &= ~(1ULL << LED_PIN);

  // GPIO.in 是输入状态寄存器
  // 读取对应位可以知道引脚当前是高电平还是低电平
  uint32_t inputState = GPIO.in;
  Serial.print("GPIO 输入寄存器值：");
  Serial.println(inputState);

  Serial.print("LED_PIN 当前状态：");
  if (inputState & (1ULL << LED_PIN)) {
    Serial.println("高电平");
  }
  else {
    Serial.println("低电平");
  }

  // 恢复输出模式
  GPIO.enable = (1ULL << LED_PIN);

  // 写法变体与常见陷阱演示
  demoRegisterVariations();
  demoRegisterTraps();
}

void loop() {
  // 用寄存器方式让 LED 闪烁
  // 这种写法比 digitalWrite 快很多，但可读性较差
  GPIO.out_w1ts = (1ULL << LED_PIN);
  delay(200);
  GPIO.out_w1tc = (1ULL << LED_PIN);
  delay(200);
}

/*
 * ===== 写法变体 =====
 *
 * 同一个功能往往有多种正确的写法。
 * 熟悉这些变体，能帮助你读懂别人写的代码，也能让你写出更简洁的代码。
 */

void demoRegisterVariations() {
  uint8_t pin = LED_PIN;

  // 例子 1：设置 GPIO 为输出模式的多种写法
  // 写法 A：直接赋值（只保留当前引脚状态，其他位清零）
  GPIO.enable = (1ULL << pin);

  // 写法 B：用 |= 保留其他引脚状态，只设置当前位
  GPIO.enable |= (1ULL << pin);

  // 写法 C：等价于 pinMode(pin, OUTPUT)
  pinMode(pin, OUTPUT);

  // 例子 2：输出高电平的多种写法
  // 写法 A：直接写 GPIO 输出寄存器
  GPIO.out = (1ULL << pin);          // 影响整个寄存器的值

  // 写法 B：用写 1 置位寄存器（推荐，不影响其他位）
  GPIO.out_w1ts = (1ULL << pin);

  // 写法 C：等价于 digitalWrite(pin, HIGH)
  digitalWrite(pin, HIGH);

  // 例子 3：输出低电平的多种写法
  // 写法 A：直接写 GPIO 输出寄存器
  GPIO.out = 0;

  // 写法 B：用写 1 清零寄存器（推荐，不影响其他位）
  GPIO.out_w1tc = (1ULL << pin);

  // 写法 C：等价于 digitalWrite(pin, LOW)
  digitalWrite(pin, LOW);

  // 例子 4：读取输入状态的多种写法
  // 写法 A：直接读 GPIO.in 寄存器
  uint32_t inputVal = GPIO.in;
  bool stateA = (inputVal >> pin) & 1;

  // 写法 B：用掩码提取某一位
  bool stateB = (GPIO.in & (1ULL << pin)) != 0;

  // 写法 C：等价于 digitalRead(pin) == HIGH
  bool stateC = digitalRead(pin) == HIGH;

  Serial.print("输入状态写法 B 结果：");
  Serial.println(stateB);
}

/*
 * ===== 常见陷阱 =====
 *
 * 下面这些代码看起来像是正确的，但要么编译不过，要么运行结果不对。
 * 每个错误示例旁边都给出了正确写法，注意对比。
 */

void demoRegisterTraps() {
  uint8_t pin = LED_PIN;

  // 陷阱 1：直接给 GPIO.enable 赋值，清除了其他引脚的输出配置
  // 错误写法：
  // GPIO.enable = (1ULL << pin);
  // 如果 GPIO4 之前被设为输出，这一行会把它改回输入！
  // 正确写法：
  GPIO.enable |= (1ULL << pin);  // 只设置当前位，保留其他位
  Serial.println("正确设置输出使能：只改当前位");

  // 陷阱 2：想输出高电平却写错了寄存器地址
  // 错误写法：
  // GPIO.out_w1tc = (1ULL << pin);  // 这是清零寄存器，会把 pin 输出低电平
  // 正确写法：
  GPIO.out_w1ts = (1ULL << pin);  // 置位寄存器，才能把 pin 输出高电平
  Serial.println("正确点亮 LED：使用 out_w1ts");

  // 陷阱 3：操作寄存器前没有设置输出使能
  // 错误写法：
  // GPIO.enable &= ~(1ULL << pin);  // 先禁用了输出
  // GPIO.out_w1ts = (1ULL << pin);  // 此时引脚不是输出，LED 不会亮
  // 正确写法：
  GPIO.enable |= (1ULL << pin);     // 先使能输出
  GPIO.out_w1ts = (1ULL << pin);    // 再输出高电平
  Serial.println("正确顺序：先使能输出，再写输出寄存器");

  // 陷阱 4：移位时用了普通 int 而不是 1ULL
  // 错误写法：
  // GPIO.out_w1ts = (1 << pin);
  // 当 pin >= 31 时，1 是 int（通常 32 位），左移 31 位可能进入符号位，导致结果错误
  // 正确写法：
  GPIO.out_w1ts = (1ULL << pin);  // 用 unsigned long long，保证 64 位宽
  Serial.println("正确掩码：使用 1ULL");
}

/*
 * 练习：
 * 1. 用寄存器方式同时控制 GPIO2 和 GPIO4 两个 LED。
 * 2. 写一个函数 regDigitalWrite(uint8_t pin, uint8_t value)，用寄存器实现 digitalWrite。
 * 3. 用寄存器读取 GPIO4 按键状态（注意先设置为输入模式）。
 * 4. 比较 digitalWrite 和寄存器操作的执行速度差异（可用逻辑分析仪或示波器观察）。
 */
