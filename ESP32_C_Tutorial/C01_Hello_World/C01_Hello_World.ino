/*
 * ESP32 学 C 语言 01：Hello World 与程序结构
 *
 * 学习目标：
 * 1. 理解 C 语言程序的基本结构
 * 2. 学会使用 printf() 和 Serial.println() 输出信息
 * 3. 理解 setup() 和 loop() 在 Arduino 环境下的作用
 * 4. 知道 Arduino 的 .ino 文件本质上可以写 C 语言
 *
 * 重要说明：
 * 我们使用 Arduino IDE 来学习 C 语言，因为配置简单、上传方便。
 * Arduino 的 .ino 文件在底层会被转换成 C/C++ 代码。
 * 在本教程中，我们会尽量使用纯 C 语法，暂时不涉及 C++ 的类和对象。
 */

// C 语言中，#include 用来引入头文件。
// 头文件里声明了很多别人已经写好的函数，我们可以直接调用。
// stdio.h 是 "Standard Input/Output Header" 的缩写，
// 里面包含了 printf()、scanf() 等输入输出函数。
#include <stdio.h>

void c01_variations_and_traps() {
  /*
   * ===== 写法变体 =====
   *
   * 同一个功能往往有多种正确的写法。
   * 熟悉这些变体，能帮助你读懂别人写的代码，也能让你写出更简洁的代码。
   */

  // 示例 1：输出一行文字有三种等价写法
  Serial.println("写法 A：Serial.println() 自动换行");
  Serial.print("写法 B：Serial.print() 需要手动加 \\n\n");
  printf("写法 C：printf() 也输出到串口，末尾加 \\n\n");

  // 示例 2：输出整数变量也有不同写法
  int count = 42;
  Serial.print("count = ");
  Serial.println(count);
  printf("count = %d\n", count);

  // 示例 3：多行文字可以多次 println，也可以一次 printf 输出
  Serial.println("第一行");
  Serial.println("第二行");
  printf("第一行\n第二行\n第三行\n");

  /*
   * ===== 常见陷阱 =====
   *
   * 下面这些代码看起来像是正确的，但要么编译不过，要么运行结果不对。
   * 每个错误示例旁边都给出了正确写法，注意对比。
   */

  // 陷阱 1：在 setup() 里忘记调用 Serial.begin()
  // 错误写法（已注释，不要直接运行）：
  // Serial.println("还没初始化串口就输出");  // 可能看不到输出或出现乱码
  // 正确写法：
  // 在 setup() 最开始调用 Serial.begin(115200)，本课示例已经在 setup() 中完成。

  // 陷阱 2：语句末尾漏写分号
  // 错误写法：
  // Serial.println("Hello")
  // 正确写法：
  Serial.println("Hello");  // 语句末尾必须有分号

  // 陷阱 3：把字符串常量写成字符常量
  // 错误写法：
  // Serial.println('Hello');  // 'Hello' 是多字符常量，不是字符串，结果不确定
  // 正确写法：
  Serial.println("Hello");  // 用双引号表示字符串
}

// setup() 是 Arduino 程序的入口函数之一。
// 开发板上电或复位后，会先执行 setup()，而且只执行一次。
// 它相当于 C 语言标准程序中的 main() 函数里开头的初始化部分。
void setup() {
  // Serial.begin(115200) 初始化串口通信，波特率设置为 115200。
  // 波特率是串口通信的速度，电脑和 ESP32 必须设成一样才能正常通信。
  Serial.begin(115200);

  // delay(1000) 让程序暂停 1000 毫秒（1 秒）。
  // 这是为了等待串口监视器准备好，避免错过最开始的输出。
  delay(1000);

  // Serial.println() 是 Arduino 提供的串口输出函数。
  // println 是 "print line" 的缩写，意思是"打印一行并换行"。
  Serial.println("Hello, ESP32!");
  Serial.println("你好，ESP32！");

  // printf() 是 C 语言标准库中的输出函数，功能比 Serial.println() 更强大。
  // 在 Arduino 环境下，printf() 默认输出到串口，但需要先调用 Serial.begin()。
  // 注意：ESP32 的 Arduino 核心支持 printf() 直接输出到串口。
  printf("这是用 C 语言的 printf 输出的内容\n");

  // \n 是换行符，和 Serial.println() 自动换行的效果一样。
  // 在 C 语言字符串中，\ 开头的叫"转义字符"，用来表示一些特殊字符。

  // 调用本课的"写法变体与常见陷阱"示例
  c01_variations_and_traps();
}

// loop() 是 Arduino 程序的另一个入口函数。
// setup() 执行完后，loop() 会无限循环执行。
// 在标准 C 语言程序中没有 loop()，但这里为了兼容 Arduino 环境需要保留。
void loop() {
  // 每隔 2 秒打印一次心跳信息
  Serial.println("程序正在运行中...");

  // printf 也可以使用格式控制符输出数字
  // %d 表示输出整数，%u 表示输出无符号整数
  printf("运行时间：%lu 毫秒\n", (unsigned long)millis());

  delay(2000);
}

/*
 * 练习：
 * 1. 修改 Serial.println() 里的文字，输出你自己的名字。
 * 2. 用 printf() 输出多行文字，每行用 \n 分隔。
 * 3. 尝试把 delay(2000) 改成 delay(500)，观察输出速度变化。
 */
