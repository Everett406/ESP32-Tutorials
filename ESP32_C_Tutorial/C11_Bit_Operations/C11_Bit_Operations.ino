/*
 * ESP32 学 C 语言 11：位运算
 *
 * 学习目标：
 * 1. 理解二进制、位、字节的含义
 * 2. 掌握按位与、按位或、按位异或、按位取反
 * 3. 掌握左移和右移运算
 * 4. 学会用位运算控制硬件寄存器中的标志位
 * 5. 理解位运算在嵌入式开发中的重要性
 *
 * 什么是位运算？
 * 位运算就是对整数在二进制表示下的每一位进行操作的运算。
 * 在嵌入式开发中，位运算非常重要，因为硬件寄存器通常用每一位控制一个功能。
 */

#include <stdio.h>

#define LED_PIN 2

// 打印一个数的二进制表示
// 这个函数会在后面用到
void printBinary(uint8_t value);

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("ESP32 学 C 语言 11：位运算");

  pinMode(LED_PIN, OUTPUT);

  // ========== 二进制基础 ==========

  Serial.println("\n===== 二进制基础 =====");

  // 十进制 13 的二进制是 0000 1101
  uint8_t num = 13;
  Serial.print("num = ");
  Serial.print(num);
  Serial.print("，二进制：");
  printBinary(num);

  // 二进制每一位的权值：
  // 第 0 位（最右边）：2^0 = 1
  // 第 1 位：2^1 = 2
  // 第 2 位：2^2 = 4
  // 第 3 位：2^3 = 8
  // ...
  // 所以 0000 1101 = 8 + 4 + 0 + 1 = 13

  // ========== 按位与 & ==========

  Serial.println("\n===== 按位与 & =====");
  // 规则：两个位都为 1，结果才为 1；否则为 0
  // 常用于"清零某些位"或"提取某些位"

  uint8_t a = 0b1100;  // 12
  uint8_t b = 0b1010;  // 10

  Serial.print("a = ");
  printBinary(a);
  Serial.print("b = ");
  printBinary(b);
  Serial.print("a & b = ");
  printBinary(a & b);  // 1000 = 8

  // 实际应用：判断一个数的第 2 位是否为 1
  uint8_t flags = 0b0000_0101;
  if (flags & 0b0000_0100) {
    Serial.println("第 2 位是 1");
  }
  else {
    Serial.println("第 2 位是 0");
  }

  // ========== 按位或 | ==========

  Serial.println("\n===== 按位或 | =====");
  // 规则：两个位只要有一个为 1，结果就为 1
  // 常用于"设置某些位为 1"

  Serial.print("a | b = ");
  printBinary(a | b);  // 1110 = 14

  // 实际应用：把第 3 位设置为 1
  uint8_t config = 0b0000_0001;
  config = config | 0b0000_1000;
  Serial.print("设置第 3 位后：");
  printBinary(config);

  // ========== 按位异或 ^ ==========

  Serial.println("\n===== 按位异或 ^ =====");
  // 规则：两个位不同，结果为 1；相同为 0
  // 常用于"翻转某些位"

  Serial.print("a ^ b = ");
  printBinary(a ^ b);  // 0110 = 6

  // 实际应用：翻转第 0 位
  uint8_t state = 0b0000_0001;
  state = state ^ 0b0000_0001;  // 第 0 位由 1 变 0
  Serial.print("翻转第 0 位后：");
  printBinary(state);

  // ========== 按位取反 ~ ==========

  Serial.println("\n===== 按位取反 ~ =====");
  // 规则：0 变 1，1 变 0
  // 注意：结果与数据类型宽度有关

  uint8_t c = 0b0000_1111;
  Serial.print("c = ");
  printBinary(c);
  Serial.print("~c = ");
  printBinary((uint8_t)~c);  // 1111 0000

  // ========== 左移 << 和右移 >> ==========

  Serial.println("\n===== 移位运算 =====");

  // 左移：所有位向左移动，右边补 0
  // 左移 n 位相当于乘以 2^n
  uint8_t d = 0b0000_0001;  // 1
  Serial.print("d << 1 = ");
  printBinary(d << 1);  // 0000 0010 = 2
  Serial.print("d << 3 = ");
  printBinary(d << 3);  // 0000 1000 = 8

  // 右移：所有位向右移动，左边补 0（无符号数）
  // 右移 n 位相当于除以 2^n
  uint8_t e = 0b0000_1000;  // 8
  Serial.print("e >> 1 = ");
  printBinary(e >> 1);  // 0000 0100 = 4
  Serial.print("e >> 2 = ");
  printBinary(e >> 2);  // 0000 0010 = 2

  // ========== 实际应用：用位运算控制 LED 模式 ==========

  Serial.println("\n===== 用位运算控制 LED =====");

  // 定义标志位
  #define FLAG_LED_ON     0b0000_0001  // 第 0 位：LED 开关
  #define FLAG_BLINK_MODE 0b0000_0010  // 第 1 位：闪烁模式
  #define FLAG_FAST_BLINK 0b0000_0100  // 第 2 位：快速闪烁

  uint8_t mode = 0;

  // 打开 LED
  mode |= FLAG_LED_ON;
  Serial.print("打开 LED：");
  printBinary(mode);

  // 开启闪烁模式
  mode |= FLAG_BLINK_MODE;
  Serial.print("开启闪烁：");
  printBinary(mode);

  // 检查是否处于闪烁模式
  if (mode & FLAG_BLINK_MODE) {
    Serial.println("当前是闪烁模式");
  }

  // 关闭 LED
  mode &= ~FLAG_LED_ON;
  Serial.print("关闭 LED：");
  printBinary(mode);

  // 写法变体与常见陷阱演示
  demoBitVariations();
  demoBitTraps();
}

void loop() {
  // 主循环保持为空
}

// 打印 8 位二进制
void printBinary(uint8_t value) {
  Serial.print(value);
  Serial.print(" (");
  for (int i = 7; i >= 0; i--) {
    // 通过右移和按位与提取每一位
    Serial.print((value >> i) & 1);
    if (i == 4) {
      Serial.print(" ");  // 每 4 位加一个空格，方便阅读
    }
  }
  Serial.println(")");
}

/*
 * ===== 写法变体 =====
 *
 * 同一个功能往往有多种正确的写法。
 * 熟悉这些变体，能帮助你读懂别人写的代码，也能让你写出更简洁的代码。
 */

void demoBitVariations() {
  uint8_t flags = 0;
  uint8_t bitPos = 3;

  // 例子 1：设置某一位为 1 的多种写法
  // 写法 A：用按位或和左移（最常用）
  flags = flags | (1 << bitPos);

  // 写法 B：用复合赋值运算符简化
  flags |= (1 << bitPos);

  // 写法 C：直接用字面量（当位置固定时）
  flags |= 0b0000_1000;  // 第 3 位

  // 例子 2：清零某一位的多种写法
  // 写法 A：先取反再按位与
  flags = flags & ~(1 << bitPos);

  // 写法 B：用复合赋值运算符简化
  flags &= ~(1 << bitPos);

  // 例子 3：提取某一位的多种写法
  uint8_t value = 0b0000_1010;
  // 写法 A：先右移再与 1
  uint8_t bit2A = (value >> 2) & 1;
  // 写法 B：先与掩码再右移（结果仍是 0 或 1，但中间值不同）
  uint8_t bit2B = (value & 0b0000_0100) >> 2;
  Serial.print("提取第 2 位写法 A：");
  Serial.println(bit2A);
  Serial.print("提取第 2 位写法 B：");
  Serial.println(bit2B);

  // 例子 4：翻转某一位的多种写法
  // 写法 A：用异或
  flags ^= (1 << bitPos);
  // 写法 B：用 if 判断（更直观但效率略低）
  if (flags & (1 << bitPos)) {
    flags &= ~(1 << bitPos);
  }
  else {
    flags |= (1 << bitPos);
  }
}

/*
 * ===== 常见陷阱 =====
 *
 * 下面这些代码看起来像是正确的，但要么编译不过，要么运行结果不对。
 * 每个错误示例旁边都给出了正确写法，注意对比。
 */

void demoBitTraps() {
  // 陷阱 1：混淆逻辑与 && 和按位与 &
  // 错误写法：
  uint8_t a = 0b0000_0010;  // 2
  uint8_t b = 0b0000_0100;  // 4
  // if (a && b) { ... }  // 这是逻辑判断：a 和 b 都非 0 就成立
  // if (a & b) { ... }   // 这是按位与：结果是 0b0000_0000，条件为假
  // 如果你本意是"判断两个位标志同时存在"，应该用按位与
  if (a & b) {
    Serial.println("按位与结果非 0");
  }
  else {
    Serial.println("按位与结果为 0（a 和 b 没有同时为 1 的位）");
  }

  // 陷阱 2：运算符优先级错误
  // 错误写法：
  // uint8_t mask = 1 << bitPos + 1;
  // 由于 + 的优先级高于 <<，实际会变成 1 << (bitPos + 1)，多移了一位
  // 正确写法：
  uint8_t bitPos = 2;
  uint8_t mask = (1 << bitPos) + 1;  // 先移位再加 1
  Serial.print("加了括号的正确掩码：");
  Serial.println(mask);

  // 陷阱 3：移位超过数据宽度
  // 错误写法：
  // uint8_t x = 1 << 8;  // 对 8 位变量左移 8 位，结果是未定义行为或 0
  // 正确写法：
  uint16_t y = 1 << 8;  // 用更宽的类型接收
  Serial.print("宽类型移位正确：");
  Serial.println(y);

  // 陷阱 4：用按位取反 ~ 时忽略数据宽度
  // 错误写法：
  uint8_t flags = 0b0000_1111;
  // uint32_t wrong = ~flags;  // ~ 会把 flags 提升为 int（32 位），结果高位全是 1
  // 正确写法：
  uint8_t right = (uint8_t)~flags;  // 强制截断为 8 位
  Serial.print("正确取反结果：");
  printBinary(right);
}

/*
 * 练习：
 * 1. 写出十进制 27、55、128 的二进制表示。
 * 2. 用位运算设置一个 uint8_t 变量的第 5 位为 1，然后清零第 5 位。
 * 3. 用异或运算交换两个 int 变量的值（不使用临时变量）。
 * 4. 写一个函数，用移位和按位与提取一个 uint32_t 变量的高 16 位和低 16 位。
 */
