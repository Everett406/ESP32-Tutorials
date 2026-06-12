/*
 * ESP32 学 C 语言 07：函数
 *
 * 学习目标：
 * 1. 理解什么是函数，以及为什么要用函数
 * 2. 掌握函数的定义、声明和调用
 * 3. 理解函数参数和返回值
 * 4. 理解值传递和地址传递的区别
 * 5. 学会把代码模块化
 *
 * 什么是函数？
 * 函数是一段完成特定功能的代码块，可以被重复调用。
 * 使用函数的好处：
 * 1. 代码复用：写一次，多次调用
 * 2. 代码模块化：把复杂问题拆分成小功能
 * 3. 代码易读：函数名能说明这段代码的作用
 * 4. 易于调试：定位问题更容易
 */

#include <stdio.h>

#define LED_PIN 2

// 函数声明（也叫函数原型）
// 在函数定义之前告诉编译器：后面会有一个这样的函数
// 这样函数就可以写在调用位置的后面
void sayHello(void);
int add(int a, int b);
int multiply(int a, int b);
float celsiusToFahrenheit(float c);
void swapByValue(int a, int b);
void swapByAddress(int* a, int* b);
int findMax(int arr[], int len);

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("ESP32 学 C 语言 07：函数");

  pinMode(LED_PIN, OUTPUT);

  // ========== 无参数、无返回值的函数 ==========

  Serial.println("\n===== 调用 sayHello() =====");
  sayHello();
  sayHello();  // 可以重复调用

  // ========== 有参数、有返回值的函数 ==========

  Serial.println("\n===== 调用 add() 和 multiply() =====");
  int result1 = add(3, 5);
  Serial.print("3 + 5 = ");
  Serial.println(result1);

  int result2 = multiply(4, 7);
  Serial.print("4 * 7 = ");
  Serial.println(result2);

  // ========== 实际应用：温度转换 ==========

  Serial.println("\n===== 温度转换 =====");
  float celsius = 25.0f;
  float fahrenheit = celsiusToFahrenheit(celsius);

  Serial.print(celsius);
  Serial.print("℃ = ");
  Serial.print(fahrenheit);
  Serial.println("℉");

  // ========== 值传递 vs 地址传递 ==========

  Serial.println("\n===== 值传递 vs 地址传递 =====");

  int x = 10;
  int y = 20;
  Serial.print("交换前：x = ");
  Serial.print(x);
  Serial.print(", y = ");
  Serial.println(y);

  // 值传递：函数内部交换的是副本，外部不变
  swapByValue(x, y);
  Serial.print("值传递 swapByValue 后：x = ");
  Serial.print(x);
  Serial.print(", y = ");
  Serial.println(y);  // 还是 10 和 20

  // 地址传递：函数内部通过指针修改外部变量
  swapByAddress(&x, &y);
  Serial.print("地址传递 swapByAddress 后：x = ");
  Serial.print(x);
  Serial.print(", y = ");
  Serial.println(y);  // 变成了 20 和 10

  // ========== 函数与数组 ==========

  Serial.println("\n===== 函数处理数组 =====");
  int scores[] = {78, 92, 85, 66, 90, 88};
  int len = sizeof(scores) / sizeof(scores[0]);

  int maxScore = findMax(scores, len);
  Serial.print("最高分：");
  Serial.println(maxScore);

  // ========== 实际应用：LED 闪烁函数 ==========

  Serial.println("\n===== LED 闪烁 =====");
  blinkLED(3, 500);  // 闪烁 3 次，每次 500ms

  /*
   * ===== 写法变体 =====
   * 
   * 同一个功能往往有多种正确的写法。
   * 熟悉这些变体，能帮助你读懂别人写的代码，也能让你写出更简洁的代码。
   */

  Serial.println("\n===== 函数写法变体 =====");

  // 写法变体 1：函数可以先声明再定义，也可以直接写在调用前面
  // 本课前面用的是"先声明后定义"；如果函数定义写在 setup() 之前，就不需要声明
  Serial.println("函数写在调用前则不需要额外声明");

  // 写法变体 2：有返回值的函数可以把返回值直接用在表达式里，也可以先存到变量
  Serial.print("直接打印 add(2, 3)：");
  Serial.println(add(2, 3));          // 返回值直接当参数
  int temp = add(2, 3);
  Serial.print("先存变量再打印：");   // 结果一样，只是多了一步
  Serial.println(temp);

  // 写法变体 3：单行返回可以省略局部变量
  // int add(int a, int b) { int result = a + b; return result; }
  // 等价于 int add(int a, int b) { return a + b; }
  Serial.println("add() 也可以写成 return a + b;");

  // 写法变体 4：无参数函数的参数列表可以写 void，也可以空着
  // void sayHello(void) 和 void sayHello() 在 C/C++ 中都可以，但写 void 更规范
  sayHello();   // 调用时两种写法没区别

  /*
   * ===== 常见陷阱 =====
   * 
   * 下面这些代码看起来像是正确的，但要么编译不过，要么运行结果不对。
   * 每个错误示例旁边都给出了正确写法，注意对比。
   */

  Serial.println("\n===== 函数常见陷阱 =====");

  // 陷阱 1：想调用函数，却忘了写括号
  // 错误写法：sayHello;   // 这只是函数地址，不会执行
  // 正确写法：
  sayHello();

  // 陷阱 2：想通过函数交换两个变量，却用了值传递
  int m = 1, n = 2;
  Serial.print("swapByValue 前：m = ");
  Serial.print(m);
  Serial.print(", n = ");
  Serial.println(n);
  swapByValue(m, n);  // 内部交换的是副本，外部不变
  Serial.print("swapByValue 后（没变，说明值传递不起作用）：m = ");
  Serial.print(m);
  Serial.print(", n = ");
  Serial.println(n);

  // 正确写法：用地址传递
  swapByAddress(&m, &n);
  Serial.print("swapByAddress 后（正确交换）：m = ");
  Serial.print(m);
  Serial.print(", n = ");
  Serial.println(n);

  // 陷阱 3：非 void 函数忘了写 return
  // 错误写法（已注释掉，不要取消注释）：
  // int noReturn(int a) { int b = a + 1; }  // 没有 return，结果不确定
  // 正确写法：
  int r = add(1, 2);  // add() 内部有 return，结果确定
  Serial.print("add(1, 2) = ");
  Serial.println(r);

  // 陷阱 4：返回指向局部变量的指针
  // 错误写法（已注释掉）：
  // int* badFunc() { int local = 10; return &local; }  // local 在函数结束后消失，地址无效
  // 正确写法：返回普通数值，或者把数据存在调用者提供的缓冲区/全局变量里
}

void loop() {
  // 主循环保持为空
}

// ========== 函数定义 ==========

// 无参数、无返回值的函数
// void 表示"没有返回值"
// 参数列表里的 void 表示"没有参数"（也可以省略不写）
void sayHello(void) {
  Serial.println("你好，这是 sayHello() 函数！");
}

// 有两个 int 参数，返回 int 类型结果
int add(int a, int b) {
  int result = a + b;
  return result;  // return 把结果返回给调用者
}

int multiply(int a, int b) {
  return a * b;
}

// 温度转换函数
float celsiusToFahrenheit(float c) {
  return c * 9.0f / 5.0f + 32.0f;
}

// 值传递：交换的是参数的副本
void swapByValue(int a, int b) {
  int temp = a;
  a = b;
  b = temp;
  // 这里的 a 和 b 是局部变量，函数结束后就消失了
  // 外部的 x 和 y 不会被修改
}

// 地址传递：通过指针访问外部变量
void swapByAddress(int* a, int* b) {
  int temp = *a;  // *a 表示"指针 a 指向的变量的值"
  *a = *b;
  *b = temp;
  // 通过地址直接修改了外部变量
}

// 查找数组中的最大值
// 数组作为参数时，实际传递的是数组首地址
// 所以必须同时传入数组长度，否则函数不知道数组有多大
int findMax(int arr[], int len) {
  int maxVal = arr[0];
  for (int i = 1; i < len; i++) {
    if (arr[i] > maxVal) {
      maxVal = arr[i];
    }
  }
  return maxVal;
}

// LED 闪烁函数
void blinkLED(int times, int intervalMs) {
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(intervalMs);
    digitalWrite(LED_PIN, LOW);
    delay(intervalMs);
    Serial.print("闪烁 ");
    Serial.print(i + 1);
    Serial.println(" 次");
  }
}

/*
 * 练习：
 * 1. 写一个函数 isEven(int n)，判断一个整数是否为偶数，返回 true 或 false。
 * 2. 写一个函数 factorial(int n)，用循环计算 n 的阶乘。
 * 3. 写一个函数 reverseArray(int arr[], int len)，把数组元素反转。
 * 4. 写一个函数 average(int arr[], int len)，返回数组平均值。
 */
