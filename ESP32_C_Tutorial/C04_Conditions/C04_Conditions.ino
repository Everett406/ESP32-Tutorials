/*
 * ESP32 学 C 语言 04：条件判断
 *
 * 学习目标：
 * 1. 掌握 if / else if / else 的用法
 * 2. 掌握 switch / case 的用法
 * 3. 理解条件表达式 ? : 的用法
 * 4. 学会用条件判断处理按钮、传感器等输入
 *
 * 什么是条件判断？
 * 程序不会永远按顺序执行，有时需要根据不同的条件执行不同的代码。
 * 例如：如果温度超过 30 度，打开风扇；否则，关闭风扇。
 */

#include <stdio.h>

// 定义 LED 和按键引脚
#define LED_PIN     2
#define BUTTON_PIN  4

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("ESP32 学 C 语言 04：条件判断");

  // 为后面示例准备的变量
  int a = 10;
  int b = 5;

  // 初始化引脚
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // ========== if / else if / else ==========

  int temperature = 32;

  Serial.println("\n===== if / else if / else =====");
  Serial.print("当前温度：");
  Serial.println(temperature);

  // if 后面括号里是条件表达式
  // 如果结果为 true（或非 0），就执行 {} 里的代码
  if (temperature >= 35) {
    Serial.println("高温警告！请降温。");
  }
  else if (temperature >= 30) {
    // 上面的条件不满足时，才会判断这里的条件
    Serial.println("温度偏高，注意散热。");
  }
  else if (temperature >= 20) {
    Serial.println("温度舒适。");
  }
  else {
    // 前面所有条件都不满足时，执行这里
    Serial.println("温度较低。");
  }

  // ========== if 嵌套 ==========

  Serial.println("\n===== if 嵌套 =====");
  int humidity = 80;

  if (temperature > 30) {
    if (humidity > 70) {
      Serial.println("又热又潮湿，体感闷热。");
    }
    else {
      Serial.println("热但比较干燥。");
    }
  }

  // ========== switch / case ==========

  Serial.println("\n===== switch / case =====");
  // switch 适合对一个变量的多个离散值进行判断
  // 例如：根据按键按下的次数执行不同操作

  int mode = 2;

  switch (mode) {
    case 1:
      Serial.println("模式 1：低速运行");
      break;  // break 很重要，否则会"穿透"到下一个 case

    case 2:
      Serial.println("模式 2：中速运行");
      break;

    case 3:
      Serial.println("模式 3：高速运行");
      break;

    default:
      // 所有 case 都不匹配时执行
      Serial.println("未知模式");
      break;
  }

  // ========== 条件表达式（三目运算符）==========

  Serial.println("\n===== 三目运算符 =====");
  // 条件 ? 表达式1 : 表达式2
  // 如果条件为 true，返回表达式1 的值；否则返回表达式2 的值

  int score = 75;
  const char* result = (score >= 60) ? "及格" : "不及格";

  Serial.print("成绩：");
  Serial.print(score);
  Serial.print("，结果：");
  Serial.println(result);

  // 也可以用三目运算符做简单的赋值
  int maxValue = (a > b) ? a : b;
  Serial.print("a 和 b 中的较大值：");
  Serial.println(maxValue);

  /*
   * ===== 写法变体 =====
   *
   * 同一个功能往往有多种正确的写法。
   * 熟悉这些变体，能帮助你读懂别人写的代码，也能让你写出更简洁的代码。
   */

  Serial.println("\n===== 写法变体 =====");

  // 1. 只有一条语句时，if 可以省略大括号，但不推荐初学者使用
  int flag = 1;
  if (flag == 1)
    Serial.println("flag 等于 1（省略大括号版本）");

  // 2. 简单的二选一可以用三元运算符代替 if/else
  int s = 75;
  const char* level = (s >= 60) ? "及格" : "不及格";
  Serial.print("三元运算符结果：");
  Serial.println(level);

  // 3. 判断变量是否为 0 时，可以写成 if (!value)，等价于 if (value == 0)
  int value = 0;
  if (!value) {
    Serial.println("value 为 0");
  }

  // 4. 多个等值判断既可以用 if/else if，也可以用 switch
  int cmd = 2;
  if (cmd == 1) {
    Serial.println("if/else if: cmd = 1");
  } else if (cmd == 2) {
    Serial.println("if/else if: cmd = 2");
  } else {
    Serial.println("if/else if: 其他");
  }

  switch (cmd) {
    case 1: Serial.println("switch: cmd = 1"); break;
    case 2: Serial.println("switch: cmd = 2"); break;
    default: Serial.println("switch: 其他"); break;
  }

  /*
   * ===== 常见陷阱 =====
   *
   * 下面这些代码看起来像是正确的，但要么编译不过，要么运行结果不对。
   * 每个错误示例旁边都给出了正确写法，注意对比。
   */

  Serial.println("\n===== 常见陷阱 =====");

  // 陷阱 1：if 条件里把 == 写成 =，结果变成赋值并且条件永远为真
  int num = 5;
  // 错误写法（已注释掉，不要运行）：
  // if (num = 10) { Serial.println("相等"); }
  // 正确写法：
  if (num == 5) {
    Serial.println("正确：num 等于 5");
  }

  // 陷阱 2：悬空 else，else 总是匹配最近的未配对 if
  int x = 1, y = 2;
  if (x == 1)
    if (y == 2)
      Serial.println("x=1 且 y=2");
    else
      Serial.println("这个 else 属于第二个 if，不是第一个！");

  // 正确写法：加花括号消除歧义
  if (x == 1) {
    if (y == 2) {
      Serial.println("x=1 且 y=2");
    } else {
      Serial.println("现在 else 的归属清楚了");
    }
  }

  // 陷阱 3：switch 里漏写 break，导致 case 穿透
  int mode2 = 1;
  // 错误写法（已注释掉）：
  // switch (mode2) {
  //   case 1: Serial.println("模式 1");
  //   case 2: Serial.println("模式 2");  // mode2=1 时也会执行到这里
  // }
  // 正确写法：
  switch (mode2) {
    case 1: Serial.println("switch 正确：模式 1"); break;
    case 2: Serial.println("switch 正确：模式 2"); break;
    default: Serial.println("switch 正确：未知模式"); break;
  }

  // 陷阱 4：把按位与 & 当成逻辑与 &&，导致判断结果错误
  int p = 1, q = 2;
  // 错误写法（已注释掉）：
  // if (p & q) { Serial.println("都非 0"); }  // 1 & 2 = 0，条件为假
  // 正确写法：
  if (p && q) {
    Serial.println("正确：p 和 q 都非 0");
  }
}

void loop() {
  // 读取按键状态
  // 因为用了 INPUT_PULLUP，按键未按下为 HIGH，按下为 LOW
  int buttonState = digitalRead(BUTTON_PIN);

  if (buttonState == LOW) {
    // 按键被按下
    Serial.println("按键被按下");

    // 切换 LED 状态
    if (digitalRead(LED_PIN) == HIGH) {
      digitalWrite(LED_PIN, LOW);
      Serial.println("LED 熄灭");
    }
    else {
      digitalWrite(LED_PIN, HIGH);
      Serial.println("LED 点亮");
    }

    // 简单消抖
    delay(300);
  }
}

/*
 * 练习：
 * 1. 用 if / else 判断一个年份是否是闰年。
 *    闰年规则：能被 4 整除但不能被 100 整除，或者能被 400 整除。
 * 2. 用 switch / case 实现一个简单的计算器：输入 1 做加法，2 做减法，等等。
 * 3. 用三目运算符把 LED 的状态在"点亮"和"熄灭"之间切换。
 */
