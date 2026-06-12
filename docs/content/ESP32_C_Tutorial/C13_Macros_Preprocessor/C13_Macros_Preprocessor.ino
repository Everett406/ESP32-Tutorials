/*
 * ESP32 学 C 语言 13：宏定义与预处理器
 *
 * 学习目标：
 * 1. 理解预处理器的作用
 * 2. 掌握 #define 宏定义
 * 3. 掌握带参数的宏
 * 4. 理解条件编译 #if / #ifdef / #ifndef
 * 5. 学会用宏提高代码可读性和可维护性
 *
 * 什么是预处理器？
 * 预处理器是在编译之前处理源代码的程序。
 * 它以 # 开头的指令工作，例如 #include、#define、#if。
 * 预处理器的处理结果是一个新的源代码文件，然后才被编译器编译。
 */

#include <stdio.h>

// ========== 宏定义常量 ==========

// #define 用来定义宏
// 编译前，预处理器会把代码中所有的 LED_PIN 替换成 2
// 这样如果以后换引脚，只需要改这里一处
#define LED_PIN 2

// 多个相关的宏可以一起定义
#define BUTTON_PIN 4
#define BAUD_RATE 115200

// 用宏定义版本号
#define FIRMWARE_VERSION "1.0.0"
#define HARDWARE_VERSION 2

// ========== 带参数的宏 ==========

// 宏也可以带参数，看起来像函数，但本质上是文本替换
// 注意：参数和整个表达式都要加括号，避免优先级问题
#define SQUARE(x) ((x) * (x))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define ABS(x) (((x) >= 0) ? (x) : -(x))

// ========== 条件编译 ==========

// 定义一个宏，用来控制是否启用调试输出
// 0 表示关闭，1 表示开启
#define DEBUG_MODE 1

// #if 根据条件决定是否编译某段代码
#if DEBUG_MODE
  #define DEBUG_PRINT(x) Serial.println(x)
#else
  #define DEBUG_PRINT(x)  // 空定义，什么都不做
#endif

// #ifdef 判断是否定义了某个宏
#ifdef LED_PIN
  // 如果定义了 LED_PIN，就编译这里
  #define LED_PIN_DEFINED 1
#else
  #define LED_PIN_DEFINED 0
#endif

void setup() {
  Serial.begin(BAUD_RATE);
  delay(1000);

  // 字符串化操作符 #
  // #define STR(x) #x 可以把宏参数变成字符串
  #define STR(x) #x
  Serial.print("LED_PIN 的值是：");
  Serial.println(STR(LED_PIN));

  Serial.println("ESP32 学 C 语言 13：宏定义与预处理器");
  Serial.print("固件版本：");
  Serial.println(FIRMWARE_VERSION);
  Serial.print("硬件版本：");
  Serial.println(HARDWARE_VERSION);

  // ========== 宏定义的常量使用 ==========

  Serial.println("\n===== 使用宏定义常量 =====");
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  Serial.println("LED 点亮");
  delay(500);
  digitalWrite(LED_PIN, LOW);
  Serial.println("LED 熄灭");

  // ========== 带参数宏的使用 ==========

  Serial.println("\n===== 使用带参数宏 =====");
  int num = 5;
  Serial.print("SQUARE(");
  Serial.print(num);
  Serial.print(") = ");
  Serial.println(SQUARE(num));  // 25

  int x = 10, y = 20;
  Serial.print("MAX(");
  Serial.print(x);
  Serial.print(", ");
  Serial.print(y);
  Serial.print(") = ");
  Serial.println(MAX(x, y));  // 20

  Serial.print("ABS(-7) = ");
  Serial.println(ABS(-7));  // 7

  // ========== 调试宏的使用 ==========

  Serial.println("\n===== 调试宏 =====");
  DEBUG_PRINT("这是一条调试信息");
  DEBUG_PRINT("当前运行到 setup() 函数");

  // 如果把 DEBUG_MODE 改成 0，上面的两行就不会被编译，也不会输出
  // 这样可以方便地在发布版本中关闭调试信息，节省空间和运行时间

  // ========== 宏与常量的选择 ==========

  Serial.println("\n===== 宏 vs const 常量 =====");

  // 宏定义：
  #define PI_MACRO 3.14159f

  // const 常量：
  const float PI_CONST = 3.14159f;

  // 区别：
  // 1. 宏在预处理阶段做文本替换，不占用内存
  // 2. const 常量有类型，编译器会做类型检查
  // 3. 现代 C 语言中，更推荐使用 const 定义常量
  // 4. 宏适合定义条件编译标志、简化重复代码、访问寄存器等场景

  Serial.print("PI_MACRO = ");
  Serial.println(PI_MACRO);
  Serial.print("PI_CONST = ");
  Serial.println(PI_CONST);

  /*
   * ===== 写法变体 =====
   *
   * 同一个功能往往有多种正确的写法。
   * 熟悉这些变体，能帮助你读懂别人写的代码，也能让你写出更简洁的代码。
   */

  // 变体 1：调试输出宏的多种实现
  // 简单版：直接输出字符串
  #define DEBUG_1(x) Serial.println(x)

  // 带文件名和行号版
  #define DEBUG_2(x) do { \
    Serial.print(__FILE__); \
    Serial.print(":"); \
    Serial.print(__LINE__); \
    Serial.print(" "); \
    Serial.println(x); \
  } while(0)

  DEBUG_1("简单调试输出");
  DEBUG_2("带位置信息的调试输出");

  // 变体 2：带参数宏可以用多种方式实现同一功能
  // 求两数最大值，三元运算符写法
  #define MAX_V1(a, b) (((a) > (b)) ? (a) : (b))

  // 同样的功能，也可以利用 GCC 扩展语句避免多次求值副作用
  // 注意：这种写法依赖编译器扩展，可移植性稍差
  #define MAX_V2(a, b) ({ \
    __typeof__(a) _a = (a); \
    __typeof__(b) _b = (b); \
    _a > _b ? _a : _b; \
  })

  Serial.print("MAX_V1(3, 7) = ");
  Serial.println(MAX_V1(3, 7));
  Serial.print("MAX_V2(3, 7) = ");
  Serial.println(MAX_V2(3, 7));

  // 变体 3：条件编译可以用 #if、#ifdef、#ifndef 互相转换
  // 以下三段在语义上等价：
  #define FEATURE_A 1

  #if FEATURE_A
    #define FEATURE_A_ENABLED
  #endif

  #ifdef FEATURE_A_ENABLED
    Serial.println("FEATURE_A 已启用");
  #endif

  #ifndef FEATURE_A_DISABLED
    Serial.println("FEATURE_A 未禁用");
  #endif

  // 变体 4：宏常量也可以用 const 或 enum 实现
  // 用 #define 定义整数常量
  #define LED_PIN_MACRO 2

  // 用 const 定义有类型的常量（现代 C 更推荐）
  const int LED_PIN_CONST = 2;

  // 用 enum 定义整型常量（编译期常量，有类型检查）
  enum PinDef {
    LED_PIN_ENUM = 2
  };

  Serial.print("LED_PIN_MACRO = ");
  Serial.println(LED_PIN_MACRO);
  Serial.print("LED_PIN_CONST = ");
  Serial.println(LED_PIN_CONST);
  Serial.print("LED_PIN_ENUM = ");
  Serial.println(LED_PIN_ENUM);

  /*
   * ===== 常见陷阱 =====
   *
   * 下面这些代码看起来像是正确的，但要么编译不过，要么运行结果不对。
   * 每个错误示例旁边都给出了正确写法，注意对比。
   */

  // 陷阱 1：宏定义不加括号，导致优先级错误
  // 错误写法：
  // #define SQUARE_BAD(x) x * x
  // 当调用 SQUARE_BAD(2 + 3) 时，会被展开为 2 + 3 * 2 + 3 = 11，而不是 25
  // 正确写法：
  #define SQUARE_GOOD(x) ((x) * (x))

  int sBad = 2 + 3 * 2 + 3;        // 模拟 SQUARE_BAD(2 + 3) 的展开结果
  int sGood = SQUARE_GOOD(2 + 3);  // 25
  Serial.print("错误展开结果：");
  Serial.println(sBad);
  Serial.print("正确结果：");
  Serial.println(sGood);

  // 陷阱 2：带参数宏内部有多条语句时，没有用 do { } while(0) 包裹
  // 错误写法：
  // #define SET_PIN_BAD(pin, val) pinMode(pin, OUTPUT); digitalWrite(pin, val)
  // 在 if 条件中使用时会出错：
  // if (cond) SET_PIN_BAD(2, HIGH); else ...
  // 展开后 else 会匹配错误
  // 正确写法：
  #define SET_PIN_GOOD(pin, val) do { \
    pinMode(pin, OUTPUT); \
    digitalWrite(pin, val); \
  } while(0)

  SET_PIN_GOOD(LED_PIN, HIGH);
  Serial.println("正确写法演示完成");

  // 陷阱 3：#ifdef 和 #if 混淆
  // 错误写法：
  // #define DEBUG_MODE 0
  // #ifdef DEBUG_MODE
  //   Serial.println("调试开启");
  // #endif
  // 这样只要定义了 DEBUG_MODE 就会进入，即使它的值是 0
  // 正确写法：
  #if DEBUG_MODE
    Serial.println("调试真正开启");
  #endif

  // 陷阱 4：字符串化操作符 # 的误用
  // 错误写法：
  // #define SHOW_VAR_BAD(x) Serial.println(#x)
  // SHOW_VAR_BAD(LED_PIN);  // 输出的是 "LED_PIN"，而不是它的值 2
  // 如果需要输出值，应该用下面这种方式：
  #define SHOW_VALUE(x) Serial.println(x)
  Serial.print("变量名是 LED_PIN，值是：");
  SHOW_VALUE(LED_PIN);
}

void loop() {
  // 只在 DEBUG_MODE 为 1 时编译这段代码
  #if DEBUG_MODE
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint > 2000) {
      lastPrint = millis();
      DEBUG_PRINT("loop() 正在运行");
    }
  #endif
}

/*
 * 练习：
 * 1. 定义一个宏 CIRCLE_AREA(r)，计算圆的面积。
 * 2. 用条件编译实现：根据 BOARD_VERSION 的值选择不同的 LED 引脚。
 * 3. 写一个调试宏 DEBUG_PRINTF(fmt, ...)，支持 printf 风格的格式化输出。
 *    提示：需要用到可变参数宏 __VA_ARGS__。
 * 4. 比较 #define LED_PIN 2 和 const int LED_PIN = 2; 的区别。
 */
