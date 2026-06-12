/*
 * ESP32 学 C 语言 05：循环
 *
 * 学习目标：
 * 1. 掌握 for 循环的用法和语法结构
 * 2. 掌握 while 和 do...while 循环的区别
 * 3. 理解 break 和 continue 的作用
 * 4. 学会用循环处理重复性任务
 *
 * 什么是循环？
 * 循环就是让一段代码重复执行多次的结构。
 * 例如：LED 闪烁 10 次、读取 100 个传感器数据、遍历数组中的每个元素。
 */

#include <stdio.h>

#define LED_PIN 2

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("ESP32 学 C 语言 05：循环");

  pinMode(LED_PIN, OUTPUT);

  // ========== for 循环 ==========

  Serial.println("\n===== for 循环 =====");
  // for (初始化; 条件; 更新) { 循环体 }
  // 执行顺序：初始化 → 判断条件 → 执行循环体 → 更新 → 判断条件 → ...

  for (int i = 0; i < 5; i++) {
    Serial.print("i = ");
    Serial.println(i);
  }

  // for 循环的三个部分都可以省略，但分号不能省
  // 例如：无限循环 for (;;) { ... }

  // ========== for 循环倒序 ==========

  Serial.println("\n===== for 循环倒序 =====");
  for (int i = 5; i > 0; i--) {
    Serial.print("倒计时：");
    Serial.println(i);
  }
  Serial.println("发射！");

  // ========== while 循环 ==========

  Serial.println("\n===== while 循环 =====");
  // while (条件) { 循环体 }
  // 先判断条件，条件为 true 才执行循环体

  int count = 0;
  while (count < 5) {
    Serial.print("count = ");
    Serial.println(count);
    count++;  // 不要忘了更新计数器，否则会死循环！
  }

  // ========== do...while 循环 ==========

  Serial.println("\n===== do...while 循环 =====");
  // do { 循环体 } while (条件);
  // 先执行一次循环体，再判断条件
  // 所以 do...while 至少会执行一次

  int num = 0;
  do {
    Serial.print("num = ");
    Serial.println(num);
    num++;
  } while (num < 3);

  // ========== break 和 continue ==========

  Serial.println("\n===== break 和 continue =====");

  // break：立即跳出整个循环
  Serial.println("演示 break：");
  for (int i = 0; i < 10; i++) {
    if (i == 5) {
      Serial.println("遇到 5，跳出循环");
      break;
    }
    Serial.println(i);
  }

  // continue：跳过当前循环的剩余部分，进入下一次循环
  Serial.println("\n演示 continue：");
  for (int i = 0; i < 6; i++) {
    if (i == 2) {
      Serial.println("跳过 2");
      continue;
    }
    Serial.println(i);
  }

  // ========== 嵌套循环 ==========

  Serial.println("\n===== 嵌套循环 =====");
  // 一个循环里面再写一个循环
  for (int row = 1; row <= 3; row++) {
    for (int col = 1; col <= 3; col++) {
      Serial.print("(");
      Serial.print(row);
      Serial.print(",");
      Serial.print(col);
      Serial.print(") ");
    }
    Serial.println();
  }

  // ========== 用循环让 LED 闪烁 10 次 ==========

  Serial.println("\n===== LED 闪烁 10 次 =====");
  for (int i = 0; i < 10; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(200);
    digitalWrite(LED_PIN, LOW);
    delay(200);
    Serial.print("闪烁次数：");
    Serial.println(i + 1);
  }


  /*
   * ===== 写法变体 =====
   *
   * 同一个功能往往有多种正确的写法。
   * 熟悉这些变体，能帮助你读懂别人写的代码，也能让你写出更简洁的代码。
   */

  Serial.println("\n===== 写法变体 =====");

  // 1. for 循环的循环变量也可以定义在循环外面（旧版 C 标准必须这样写）
  int j;
  for (j = 0; j < 3; j++) {
    Serial.print("j = ");
    Serial.println(j);
  }

  // 2. 任何 for 循环都可以改写成 while 循环
  // for (初始化; 条件; 更新) { 循环体 } 等价于：初始化; while (条件) { 循环体; 更新; }
  int i2 = 0;
  while (i2 < 3) {
    Serial.print("i2 = ");
    Serial.println(i2);
    i2++;  // 更新不要忘了，否则会死循环
  }

  // 3. 如果循环体至少执行一次，可以把 while 改成 do...while
  int k = 0;
  do {
    Serial.print("k = ");
    Serial.println(k);
    k++;
  } while (k < 3);

  // 4. 遍历数组时，C++11 支持范围 for（for-each），写起来更简洁
  int arr[] = {10, 20, 30};
  for (int n : arr) {
    Serial.print("数组元素 = ");
    Serial.println(n);
  }

  /*
   * ===== 常见陷阱 =====
   *
   * 下面这些代码看起来像是正确的，但要么编译不过，要么运行结果不对。
   * 每个错误示例旁边都给出了正确写法，注意对比。
   */

  Serial.println("\n===== 常见陷阱 =====");

  // 陷阱 1：while 里忘记更新循环变量，造成死循环
  // 错误写法（已注释掉，不要运行）：
  // int m = 0;
  // while (m < 3) { Serial.println(m); }  // 没有 m++，永远不会退出
  // 正确写法：
  int m = 0;
  while (m < 3) {
    Serial.print("m = ");
    Serial.println(m);
    m++;  // 必须有更新
  }

  // 陷阱 2：条件写成 <= 导致多执行一次（off-by-one）
  Serial.println("正确：打印 0~4（执行 5 次）");
  for (int idx = 0; idx < 5; idx++) {
    Serial.println(idx);
  }
  // 错误写法是 for (int idx = 0; idx <= 5; idx++)，会执行 6 次

  // 陷阱 3：死循环没有退出条件，程序会永远卡住
  // 错误写法（已注释掉，不要运行）：
  // for (;;) { Serial.println("永远运行"); }
  // 正确写法：提供 break 或其他退出条件
  int cnt = 0;
  for (;;) {
    Serial.println("带 break 的死循环跑一次");
    cnt++;
    if (cnt >= 2) {
      break;
    }
  }

  // 陷阱 4：do...while 末尾漏写分号
  int n = 0;
  do {
    Serial.print("n = ");
    Serial.println(n);
    n++;
  } while (n < 3);  // 这里的分号不能漏，漏了会编译错误
}

void loop() {
  // 本课重点在 setup() 中演示循环
}

/*
 * 练习：
 * 1. 用 for 循环计算 1 + 2 + 3 + ... + 100 的和。
 * 2. 用 while 循环实现：当按键按下时，LED 以越来越快的速度闪烁，直到松开。
 * 3. 用嵌套循环在串口打印一个 5x5 的星号方阵。
 * 4. 用 continue 打印 1~20 中所有奇数。
 */
