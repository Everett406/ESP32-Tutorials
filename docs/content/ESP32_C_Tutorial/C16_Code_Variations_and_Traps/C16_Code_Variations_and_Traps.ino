/*
 * ESP32 学 C 语言 16：同一段代码的多种写法 & 看起来对其实错的陷阱
 *
 * 学习目标：
 * 1. 理解 C 语言中很多功能都有多种等价写法
 * 2. 学会在不同写法之间转换
 * 3. 识别那些"看起来正确，实际有问题"的代码
 * 4. 培养"读代码时不只看表面"的能力
 *
 * 为什么需要这一课？
 * C 语言非常灵活，同一个意思可以写成完全不同的样子。
 * 初学者容易困惑："为什么他写的和教材不一样？"
 * 也容易踩坑："这段代码看起来没问题，怎么运行结果不对？"
 * 这一课专门解决这两个问题。
 */

#include <stdio.h>

#define LED_PIN 2

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("ESP32 学 C 语言 16：多种写法 & 常见陷阱");

  pinMode(LED_PIN, OUTPUT);

  // ============================================================
  // 第一部分：for 循环的多种写法
  // ============================================================

  Serial.println("\n===== for 循环的多种写法 =====");

  // 写法 A：最标准的三段式
  Serial.println("写法 A：标准 for 循环");
  for (int i = 0; i < 3; i++) {
    Serial.println(i);
  }

  // 写法 B：把变量定义放到循环外面
  // 这种写法在旧版 C 标准（C89）中必须使用，因为 C89 不允许在 for 里定义变量
  Serial.println("\n写法 B：循环变量定义在外面");
  int i;
  for (i = 0; i < 3; i++) {
    Serial.println(i);
  }

  // 写法 C：循环变量在循环外定义，但更新写在循环体里
  // 这种写法容易忘记更新，导致死循环，不推荐
  Serial.println("\n写法 C：更新写在循环体里（不推荐）");
  int j = 0;
  for (; j < 3; ) {
    Serial.println(j);
    j++;  // 如果忘了这一行，就会死循环
  }

  // 写法 D：用 while 循环改写 for 循环
  // for (初始化; 条件; 更新) { 循环体 }
  // 等价于：
  // 初始化;
  // while (条件) { 循环体; 更新; }
  Serial.println("\n写法 D：for 改写成 while");
  int k = 0;
  while (k < 3) {
    Serial.println(k);
    k++;
  }

  // 写法 E：用 do...while 改写
  // 注意：do...while 至少执行一次，所以和 for 不完全等价
  Serial.println("\n写法 E：for 改写成 do...while");
  int m = 0;
  do {
    Serial.println(m);
    m++;
  } while (m < 3);

  // 写法 F：死循环的几种写法
  // 这些写法在语法上都是正确的，但语义是"永远循环"
  Serial.println("\n写法 F：死循环的写法（演示，不要运行太久）");
  int count = 0;
  for (;;) {
    Serial.println("死循环中...");
    count++;
    if (count >= 2) {
      break;  // 用 break 跳出来
    }
  }

  // ============================================================
  // 第二部分：自增自减的多种写法
  // ============================================================

  Serial.println("\n===== 自增自减的写法差异 =====");

  int a = 5;

  // 以下三行在"单独使用"时效果完全一样
  a = a + 1;   // 写法 1
  a += 1;      // 写法 2，更简洁
  a++;         // 写法 3，最简洁

  Serial.print("a 最后等于：");
  Serial.println(a);  // 8

  // 但在表达式内部，++a 和 a++ 有区别！
  int b = 5;
  int x = ++b;  // b 先变成 6，然后 x = 6
  Serial.print("++b 后，b = ");
  Serial.print(b);
  Serial.print(", x = ");
  Serial.println(x);  // b=6, x=6

  int c = 5;
  int y = c++;  // y 先等于 5，然后 c 变成 6
  Serial.print("c++ 后，c = ");
  Serial.print(c);
  Serial.print(", y = ");
  Serial.println(y);  // c=6, y=5

  // ============================================================
  // 第三部分：条件判断的多种写法
  // ============================================================

  Serial.println("\n===== 条件判断的多种写法 =====");

  int score = 75;

  // 写法 1：标准 if/else
  if (score >= 60) {
    Serial.println("及格");
  }
  else {
    Serial.println("不及格");
  }

  // 写法 2：三目运算符
  // 条件 ? 值1 : 值2
  // 只适合简单的二选一
  const char* result = (score >= 60) ? "及格" : "不及格";
  Serial.println(result);

  // 写法 3：如果只有一条语句，可以省略大括号
  // 但不推荐初学者这样做，很容易出错
  if (score >= 60)
    Serial.println("及格");
  else
    Serial.println("不及格");

  // ============================================================
  // 第四部分：看起来像对的，实际是错的
  // ============================================================

  Serial.println("\n===== 看起来像对，实际是错的 =====");

  // 陷阱 1：赋值运算符 = 和相等判断 == 混淆
  // 错误写法：
  // if (a = 10) { ... }
  // 这行代码会把 10 赋给 a，然后判断 a 是否为 0
  // 结果永远为真（非 0 即真），而且 a 的值被改掉了！
  int num = 5;
  if (num == 5) {
    Serial.println("正确：num 等于 5");
  }

  // 陷阱 2：& 和 && 混淆
  // & 是按位与，&& 是逻辑与
  // 错误写法：
  // if (a & b) { ... }
  // 如果本意是逻辑判断，应该用 &&
  int p = 1, q = 2;
  if (p && q) {
    Serial.println("正确：p 和 q 都非 0");
  }

  // 陷阱 3：| 和 || 混淆
  // | 是按位或，|| 是逻辑或
  if (p || q) {
    Serial.println("正确：p 或 q 至少一个非 0");
  }

  // 陷阱 4：dangling else（悬空 else）
  // 下面这段代码的 else 到底属于哪个 if？
  int x1 = 1, y1 = 2;
  if (x1 == 1)
    if (y1 == 2)
      Serial.println("x1=1 且 y1=2");
  else
    Serial.println("这一行属于第二个 if，不是第一个 if！");

  // 上面的代码等价于：
  // if (x1 == 1) {
  //   if (y1 == 2) {
  //     Serial.println("x1=1 且 y1=2");
  //   }
  //   else {
  //     Serial.println("这一行属于第二个 if，不是第一个 if！");
  //   }
  // }
  // 所以 else 总是匹配最近的 if。
  // 推荐永远使用大括号，避免歧义。

  // 陷阱 5：整数除法
  // 错误写法：
  // float average = (a + b) / 2;
  // 如果 a 和 b 是 int，那么 (a + b) / 2 也是 int，小数会被丢掉！
  int sum = 7;
  int count2 = 2;
  float wrongAverage = sum / count2;        // 结果是 3.0，不是 3.5！
  float rightAverage = (float)sum / count2; // 结果是 3.5

  Serial.print("错误平均数：");
  Serial.println(wrongAverage);
  Serial.print("正确平均数：");
  Serial.println(rightAverage);

  // 陷阱 6：数组越界
  // 错误写法：
  // int arr[3] = {1, 2, 3};
  // arr[3] = 4;  // 越界！合法下标只有 0、1、2
  // 编译器不会报错，但会写到别的内存，可能破坏其他变量

  // 陷阱 7：指针声明的迷惑写法
  // int* p1, p2;  // p1 是指针，p2 是 int！不是两个指针！
  // 正确写法：
  int* p1;
  int* p2;
  // 或者一行一个：
  int* p3;

  // 陷阱 8：字符串赋值错误
  // 错误写法：
  // char str[10];
  // str = "hello";  // 数组名是地址常量，不能赋值！
  // 正确写法：
  char str[10];
  strcpy(str, "hello");
  Serial.print("正确字符串赋值：");
  Serial.println(str);

  // 陷阱 9：switch 忘记 break
  // 错误写法：
  // switch (mode) {
  //   case 1: doSomething();
  //   case 2: doOtherThing();
  // }
  // 如果 mode=1，会执行 case 1 然后"穿透"到 case 2！
  // 必须加 break。

  // 陷阱 10：宏定义不加括号
  // 错误写法：
  // #define SQUARE(x) x * x
  // SQUARE(2 + 3) 会被替换成 2 + 3 * 2 + 3 = 11，不是 25！
  // 正确写法：
  // #define SQUARE(x) ((x) * (x))

  // ============================================================
  // 第五部分：编译错误 vs 运行错误
  // ============================================================

  Serial.println("\n===== 编译错误 vs 运行错误 =====");

  // 编译错误：代码语法不对，Arduino IDE 会直接标红，无法上传
  // 例如：
  // int a = 10
  // 缺少分号，编译器会报错

  // 运行错误：代码能编译通过，但运行结果不对或程序崩溃
  // 例如：数组越界、除数为 0、空指针解引用
  // 这些错误更隐蔽，需要仔细检查逻辑

  Serial.println("编译错误通常容易找，运行错误更需要逻辑分析。");

  // ============================================================
  // 第六部分：风格选择
  // ============================================================

  Serial.println("\n===== 风格选择 =====");

  // 下面这些写法都是正确的，选择你喜欢的风格并保持一致：

  // 大括号风格 1：K&R 风格
  // if (condition) {
  //   ...
  // }

  // 大括号风格 2：Allman 风格
  // if (condition)
  // {
  //   ...
  // }

  // 本教程统一使用 K&R 风格，因为 Arduino 官方示例也用这个风格。
}

void loop() {
  // 主循环保持为空
}

/*
 * 练习：
 * 1. 把下面这段代码用 while 和 do...while 各改写一次：
 *    for (int i = 0; i < 10; i++) { Serial.println(i); }
 * 2. 找出下面代码的问题：
 *    int a = 5, b = 10;
 *    if (a = b) { Serial.println("相等"); }
 * 3. 为什么 int* p1, p2; 中 p2 不是指针？怎么改才对？
 * 4. 写一段代码，展示 else 匹配哪个 if 的问题，并用大括号消除歧义。
 * 5. 解释为什么 (7 / 2) 等于 3 而不是 3.5。
 */
