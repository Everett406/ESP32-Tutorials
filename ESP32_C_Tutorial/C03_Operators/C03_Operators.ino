/*
 * ESP32 学 C 语言 03：运算符
 *
 * 学习目标：
 * 1. 掌握 C 语言中的算术运算符
 * 2. 理解赋值运算符和复合赋值运算符
 * 3. 掌握关系运算符和逻辑运算符
 * 4. 理解自增（++）和自减（--）运算符的区别
 * 5. 学会使用括号控制运算优先级
 *
 * 运算符就是对数据进行操作的符号。
 * C 语言有丰富的运算符，熟练掌握它们是编程的基础。
 */

#include <stdio.h>

void c03_variations_and_traps() {
  /*
   * ===== 写法变体 =====
   *
   * 同一个功能往往有多种正确的写法。
   * 熟悉这些变体，能帮助你读懂别人写的代码，也能让你写出更简洁的代码。
   */

  // 示例 1：自增运算在单独使用时，三种写法等价
  int a = 5;
  a = a + 1;  // 写法 1
  a += 1;     // 写法 2，更简洁
  a++;        // 写法 3，最简洁
  Serial.print("a 最后 = ");
  Serial.println(a);  // 8

  // 示例 2：复合赋值与普通写法可以互换
  int b = 10;
  b = b * 2 + 1;  // 普通写法
  b *= 2;         // 等价于 b = b * 2
  b += 1;         // 等价于 b = b + 1
  Serial.print("b 最后 = ");
  Serial.println(b);

  // 示例 3：逻辑条件可以用不同方式表达（德摩根定律）
  int age = 25;
  bool ok1 = (age >= 18) && (age < 60);           // 直接写法
  bool ok2 = !(age < 18 || age >= 60);            // 等价写法
  Serial.print("ok1 = ");
  Serial.println(ok1);
  Serial.print("ok2 = ");
  Serial.println(ok2);

  // 示例 4：括号改变优先级，使意图更清晰
  int r1 = 2 + 3 * 4;    // 先乘后加，结果是 14
  int r2 = (2 + 3) * 4;  // 括号优先，结果是 20
  Serial.print("r1 = ");
  Serial.println(r1);
  Serial.print("r2 = ");
  Serial.println(r2);

  /*
   * ===== 常见陷阱 =====
   *
   * 下面这些代码看起来像是正确的，但要么编译不过，要么运行结果不对。
   * 每个错误示例旁边都给出了正确写法，注意对比。
   */

  // 陷阱 1：把比较运算符 == 写成赋值运算符 =
  int x = 5;
  // if (x = 10) {        // 错误：把 10 赋给 x，然后判断 x 是否为 0，结果永远为真
  //   Serial.println("x 等于 10");
  // }
  if (x == 10) {          // 正确：判断 x 是否等于 10
    Serial.println("x 确实等于 10（这里不会执行）");
  } else {
    Serial.println("正确写法：x 不等于 10");
  }

  // 陷阱 2：把按位与 & 当成逻辑与 &&
  int m = 1, n = 2;
  // if (m & n) {         // 错误：按位与结果是 0，本意如果是逻辑判断应该用 &&
  //   Serial.println("两者都非 0");
  // }
  if (m && n) {           // 正确：逻辑与，判断两者是否都非 0
    Serial.println("正确写法：m 和 n 都非 0");
  }

  // 陷阱 3：把按位或 | 当成逻辑或 ||
  // if (m | n) {         // 错误：按位或，虽然这里结果非 0，但语义不对
  //   Serial.println("至少一个非 0");
  // }
  if (m || n) {           // 正确：逻辑或
    Serial.println("正确写法：m 或 n 至少一个非 0");
  }

  // 陷阱 4：在表达式中混用 ++i 和 i++，导致未定义行为或难以理解的代码
  int i = 5;
  // int y = i++ + ++i;   // 错误：同一变量在一条语句中被多次修改，结果不确定
  int y1 = i++;           // 正确：先使用 i，再加 1
  int y2 = ++i;           // 正确：先加 1，再使用 i
  Serial.print("y1 = ");
  Serial.println(y1);
  Serial.print("y2 = ");
  Serial.println(y2);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("ESP32 学 C 语言 03：运算符");

  // ========== 算术运算符 ==========

  int a = 17;
  int b = 5;

  Serial.println("\n===== 算术运算符 =====");
  Serial.print("a = ");
  Serial.println(a);
  Serial.print("b = ");
  Serial.println(b);

  // + 加法
  Serial.print("a + b = ");
  Serial.println(a + b);  // 22

  // - 减法
  Serial.print("a - b = ");
  Serial.println(a - b);  // 12

  // * 乘法
  Serial.print("a * b = ");
  Serial.println(a * b);  // 85

  // / 除法
  // 注意：整数相除会丢掉小数部分！
  // 17 / 5 = 3.4，但因为是两个 int 相除，结果是 3。
  Serial.print("a / b = ");
  Serial.println(a / b);  // 3

  // % 取余（求模）
  // 17 除以 5 余 2
  Serial.print("a % b = ");
  Serial.println(a % b);  // 2

  // 如果想要得到浮点数结果，需要把其中一个数转成浮点类型
  Serial.print("(float)a / b = ");
  Serial.println((float)a / b);  // 3.4

  // ========== 赋值运算符 ==========

  Serial.println("\n===== 赋值与复合赋值 =====");
  int x = 10;
  Serial.print("x 初始值 = ");
  Serial.println(x);

  // = 是赋值运算符，把右边的值赋给左边
  x = 20;
  Serial.print("x = 20 后，x = ");
  Serial.println(x);

  // 复合赋值运算符：先做运算，再赋值
  x += 5;  // 等价于 x = x + 5;
  Serial.print("x += 5 后，x = ");
  Serial.println(x);  // 25

  x -= 3;  // 等价于 x = x - 3;
  Serial.print("x -= 3 后，x = ");
  Serial.println(x);  // 22

  x *= 2;  // 等价于 x = x * 2;
  Serial.print("x *= 2 后，x = ");
  Serial.println(x);  // 44

  x /= 4;  // 等价于 x = x / 4;
  Serial.print("x /= 4 后，x = ");
  Serial.println(x);  // 11

  x %= 3;  // 等价于 x = x % 3;
  Serial.print("x %= 3 后，x = ");
  Serial.println(x);  // 2

  // ========== 关系运算符 ==========

  Serial.println("\n===== 关系运算符 =====");
  // 关系运算符的结果是 bool 类型：true 或 false
  // true 在 C 语言中用 1 表示，false 用 0 表示

  Serial.print("a > b：");
  Serial.println(a > b);   // true，输出 1

  Serial.print("a < b：");
  Serial.println(a < b);   // false，输出 0

  Serial.print("a == b：");
  Serial.println(a == b);  // false，输出 0
  // 注意：== 是判断相等，= 是赋值，千万不要搞混！

  Serial.print("a != b：");
  Serial.println(a != b);  // true，输出 1
  // != 表示"不等于"

  Serial.print("a >= b：");
  Serial.println(a >= b);  // true

  Serial.print("a <= b：");
  Serial.println(a <= b);  // false

  // ========== 逻辑运算符 ==========

  Serial.println("\n===== 逻辑运算符 =====");
  bool p = true;
  bool q = false;

  // && 逻辑与：两边都为 true，结果才为 true
  Serial.print("p && q：");
  Serial.println(p && q);  // false

  // || 逻辑或：只要有一边为 true，结果就是 true
  Serial.print("p || q：");
  Serial.println(p || q);  // true

  // ! 逻辑非：取反
  Serial.print("!p：");
  Serial.println(!p);  // false
  Serial.print("!q：");
  Serial.println(!q);  // true

  // ========== 自增自减运算符 ==========

  Serial.println("\n===== 自增/自减运算符 =====");
  int n = 5;

  // n++：后置自增，先使用 n 的值，再把 n 加 1
  Serial.print("n++ 时输出：");
  Serial.println(n++);  // 输出 5，然后 n 变成 6
  Serial.print("n 的值：");
  Serial.println(n);    // 6

  n = 5;
  // ++n：前置自增，先把 n 加 1，再使用 n 的值
  Serial.print("++n 时输出：");
  Serial.println(++n);  // n 先变成 6，输出 6
  Serial.print("n 的值：");
  Serial.println(n);    // 6

  // -- 同理

  // ========== 运算符优先级 ==========

  Serial.println("\n===== 运算符优先级 =====");
  int result = 2 + 3 * 4;  // 先算乘法，再算加法，结果是 14
  Serial.print("2 + 3 * 4 = ");
  Serial.println(result);

  result = (2 + 3) * 4;  // 括号改变优先级，结果是 20
  Serial.print("(2 + 3) * 4 = ");
  Serial.println(result);

  // 建议：不确定优先级时，尽量加括号，代码更易读。

  // 调用本课的"写法变体与常见陷阱"示例
  c03_variations_and_traps();
}

void loop() {
  // 本课重点在 setup() 中演示运算符
}

/*
 * 练习：
 * 1. 写一个表达式，把温度从摄氏度转成华氏度：F = C * 9 / 5 + 32
 *    注意整数除法的问题，怎么得到准确的小数结果？
 * 2. 定义两个变量，用关系运算符和逻辑运算符判断：
 *    "年龄大于 18 岁 并且 年龄小于 60 岁"
 * 3. 尝试 n-- 和 --n，观察前置和后置的区别。
 */
