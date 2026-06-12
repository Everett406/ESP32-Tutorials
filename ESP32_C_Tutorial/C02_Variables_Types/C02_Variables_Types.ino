/*
 * ESP32 学 C 语言 02：变量与数据类型
 *
 * 学习目标：
 * 1. 理解什么是变量，以及变量为什么需要"类型"
 * 2. 掌握 C 语言常用的基本数据类型
 * 3. 学会使用 sizeof() 查看不同类型占用的内存大小
 * 4. 理解有符号和无符号、整数和浮点数的区别
 *
 * 什么是变量？
 * 变量就是程序运行时用来存放数据的一个"盒子"。
 * 每个盒子都有名字（变量名）和大小（类型决定）。
 * 变量名方便我们记住数据存在哪里，类型决定这个盒子里能放什么数据。
 *
 * 为什么需要类型？
 * 因为不同类型的数据在内存中占用的空间不同，运算方式也不同。
 * 例如：整数和带小数的数在内存里的存储方式完全不同。
 */

#include <stdio.h>

void c02_variations_and_traps() {
  /*
   * ===== 写法变体 =====
   *
   * 同一个功能往往有多种正确的写法。
   * 熟悉这些变体，能帮助你读懂别人写的代码，也能让你写出更简洁的代码。
   */

  // 示例 1：变量声明可以一行一个，也可以一行多个
  int a = 1;
  int b = 2, c = 3;  // 一行声明多个变量，用逗号分隔

  // 示例 2：常量后缀让类型更清晰
  unsigned int x = 100U;   // U 表示 unsigned
  long y = 100L;           // L 表示 long
  unsigned long z = 100UL; // UL 表示 unsigned long

  // 示例 3：固定宽度类型与基本类型可以互相转换
  uint8_t byteVal = 255;        // 等价于 unsigned char byteVal = 255;
  int16_t shortVal = -1000;     // 等价于 short shortVal = -1000;
  uint32_t wordVal = 4294967295U; // 等价于 unsigned int wordVal = 4294967295U;

  // 示例 4：字符可以用字面量，也可以用 ASCII 码
  char ch1 = 'A';
  char ch2 = 65;  // 65 就是 'A' 的 ASCII 码，两者等价
  Serial.print("ch1 = ");
  Serial.println(ch1);
  Serial.print("ch2 = ");
  Serial.println(ch2);

  /*
   * ===== 常见陷阱 =====
   *
   * 下面这些代码看起来像是正确的，但要么编译不过，要么运行结果不对。
   * 每个错误示例旁边都给出了正确写法，注意对比。
   */

  // 陷阱 1：整数除法丢掉小数部分
  int sum = 7;
  int count = 2;
  // float wrongAvg = sum / count;        // 错误：结果是 3.0，不是 3.5
  float rightAvg = (float)sum / count;  // 正确：先把一个操作数转成 float
  Serial.print("正确平均数 = ");
  Serial.println(rightAvg);

  // 陷阱 2：无符号类型溢出
  uint8_t small = 255;
  // small = small + 1;  // 错误：结果变成 0，而不是 256
  uint16_t bigger = small;  // 正确：先放到足够大的类型里再运算
  bigger = bigger + 1;
  Serial.print("255 + 1 用 uint16_t 存储 = ");
  Serial.println(bigger);

  // 陷阱 3：有符号数和无符号数比较
  int signedVal = -1;
  unsigned int unsignedVal = 1;
  // if (signedVal < unsignedVal) {       // 错误：-1 会被转成大无符号数，结果可能相反
  //   Serial.println("signedVal 更小");  // 实际上不会执行
  // }
  if ((long)signedVal < (long)unsignedVal) {  // 正确：先统一转成有符号类型再比较
    Serial.println("正确比较：-1 确实小于 1");
  }

  // 陷阱 4：把大数赋给小类型，默默截断
  int big = 300;
  // uint8_t tiny = big;        // 错误：300 超过 uint8_t 范围，截断成 44
  uint16_t safe = big;          // 正确：选择能容纳数值的类型
  Serial.print("300 存到 uint16_t = ");
  Serial.println(safe);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("ESP32 学 C 语言 02：变量与数据类型");

  // ========== 整数类型 ==========

  // int 是最常用的整数类型。
  // 在 ESP32 上，int 通常是 32 位，范围约 -21 亿 ~ +21 亿。
  int temperature = 25;
  Serial.print("温度：");
  Serial.println(temperature);

  // short 是短整数，通常占 2 字节（16 位），范围 -32768 ~ 32767。
  // 适合存储不太大的整数，可以节省内存。
  short smallNumber = 1000;
  Serial.print("short 类型数值：");
  Serial.println(smallNumber);

  // long 是长整数，在 ESP32 上通常也是 32 位。
  //  historical 原因，long 在早期 16 位系统中是 32 位，int 是 16 位。
  long bigNumber = 123456789L;
  Serial.print("long 类型数值：");
  Serial.println(bigNumber);

  // long long 是更长的整数，占 8 字节（64 位），范围非常大。
  long long veryBigNumber = 9876543210LL;
  Serial.print("long long 类型数值：");
  Serial.println((long long)veryBigNumber);

  // ========== 无符号整数 ==========

  // unsigned 关键字表示"无符号"，即不能表示负数，但正数范围会翻倍。
  // 例如 unsigned char 范围是 0~255，而 char 范围是 -128~127。
  unsigned int positiveOnly = 4000000000U;
  Serial.print("无符号整数：");
  Serial.println(positiveOnly);

  // uint8_t、uint16_t、uint32_t、uint64_t 是固定宽度的无符号整数类型。
  // 这些类型在嵌入式开发中非常常用，因为它们的大小是确定的。
  // 定义在 <stdint.h> 头文件中，Arduino 环境已经自动引入。
  uint8_t oneByte = 255;       // 8 位无符号，范围 0~255
  uint16_t twoBytes = 65535;   // 16 位无符号，范围 0~65535
  uint32_t fourBytes = 4294967295U; // 32 位无符号
  Serial.print("uint8_t：");
  Serial.println(oneByte);
  Serial.print("uint16_t：");
  Serial.println(twoBytes);
  Serial.print("uint32_t：");
  Serial.println(fourBytes);

  // ========== 浮点类型 ==========

  // float 单精度浮点数，占 4 字节，适合表示带小数的数。
  // 精度大约 6~7 位有效数字。
  float pi = 3.14159f;
  Serial.print("pi = ");
  Serial.println(pi, 5);  // 5 表示保留 5 位小数

  // double 双精度浮点数，占 8 字节，精度更高。
  // 在 ESP32 上，double 是真正的 64 位双精度。
  double precisePi = 3.141592653589793;
  Serial.print("精确 pi = ");
  Serial.println(precisePi, 10);

  // ========== 字符类型 ==========

  // char 字符类型，占 1 字节。
  // 它既可以表示一个字符，也可以表示一个 -128~127 的整数。
  char letter = 'A';
  Serial.print("字符：");
  Serial.println(letter);
  Serial.print("字符 A 的 ASCII 码：");
  Serial.println((int)letter);  // 强制类型转换，把 char 转成 int 输出

  // ========== 布尔类型 ==========

  // bool 布尔类型，只有两个值：true（真）和 false（假）。
  // 在 C99 标准中引入，需要包含 <stdbool.h>。
  // Arduino 环境已经支持 bool，不需要额外包含。
  bool isRunning = true;
  Serial.print("isRunning = ");
  Serial.println(isRunning);

  // ========== sizeof 运算符 ==========

  // sizeof() 用来获取某个类型或变量占用的字节数。
  // 在 ESP32 这种 32 位处理器上，了解变量大小对内存优化很重要。
  Serial.println("\n各类型在 ESP32 上占用的字节数：");
  Serial.print("sizeof(char) = ");
  Serial.println(sizeof(char));
  Serial.print("sizeof(int) = ");
  Serial.println(sizeof(int));
  Serial.print("sizeof(long) = ");
  Serial.println(sizeof(long));
  Serial.print("sizeof(float) = ");
  Serial.println(sizeof(float));
  Serial.print("sizeof(double) = ");
  Serial.println(sizeof(double));
  Serial.print("sizeof(bool) = ");
  Serial.println(sizeof(bool));

  // 调用本课的"写法变体与常见陷阱"示例
  c02_variations_and_traps();
}

void loop() {
  // 本课重点在 setup() 中演示变量类型
  // loop() 留空
}

/*
 * 练习：
 * 1. 定义一个变量存储你的年龄，分别用 int、unsigned int、uint8_t 存储，观察输出。
 * 2. 用 printf() 输出以下信息："温度：25℃，湿度：60%"
 *    提示：printf("温度：%d℃，湿度：%d%%\n", 25, 60);
 *    注意：%% 用来输出百分号本身。
 * 3. 尝试给 uint8_t 赋值 256，观察会发生什么（超出范围会"溢出"）。
 */
