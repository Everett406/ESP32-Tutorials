/*
 * ESP32 学 C 语言 14：存储类别与变量生命周期
 *
 * 学习目标：
 * 1. 理解局部变量、全局变量、静态变量的区别
 * 2. 理解变量的作用域和生命周期
 * 3. 掌握 static、extern、volatile 关键字的用法
 * 4. 理解 Arduino 中 setup()、loop() 里变量的行为
 *
 * 什么是变量的生命周期？
 * 生命周期指的是变量从创建到销毁的时间段。
 * 不同存储类别的变量，生命周期和作用域都不同。
 */

#include <stdio.h>

// 全局变量：在所有函数外部定义
// 作用域：从定义位置到文件结束
// 生命周期：从程序开始到程序结束
int globalCounter = 0;

// 静态全局变量：只能在本文件内使用
// 其他文件即使声明 extern 也访问不到
static int filePrivateCounter = 0;

// 函数声明
void demoLocalVariable(void);
void demoStaticVariable(void);
void demoVolatile(void);

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("ESP32 学 C 语言 14：存储类别与变量生命周期");

  // ========== 局部变量 ==========

  Serial.println("\n===== 局部变量 =====");

  // setupLocal 是 setup() 函数内部的局部变量
  // 作用域：只在 setup() 函数内部
  // 生命周期：setup() 执行期间存在，setup() 结束后销毁
  int setupLocal = 100;
  Serial.print("setupLocal = ");
  Serial.println(setupLocal);

  // 调用 demoLocalVariable 三次
  Serial.println("\n调用 demoLocalVariable() 三次：");
  for (int i = 0; i < 3; i++) {
    demoLocalVariable();
  }

  // ========== 静态局部变量 ==========

  Serial.println("\n===== 静态局部变量 =====");
  Serial.println("调用 demoStaticVariable() 三次：");
  for (int i = 0; i < 3; i++) {
    demoStaticVariable();
  }

  // ========== 全局变量 ==========

  Serial.println("\n===== 全局变量 =====");
  Serial.print("调用前 globalCounter = ");
  Serial.println(globalCounter);

  globalCounter++;
  Serial.print("调用后 globalCounter = ");
  Serial.println(globalCounter);

  // ========== 静态全局变量 ==========

  Serial.println("\n===== 静态全局变量 =====");
  filePrivateCounter++;
  Serial.print("filePrivateCounter = ");
  Serial.println(filePrivateCounter);
  Serial.println("这个变量只能在当前文件中使用");

  // ========== volatile 关键字 ==========

  Serial.println("\n===== volatile 关键字 =====");
  demoVolatile();

  /*
   * ===== 写法变体 =====
   *
   * 同一个功能往往有多种正确的写法。
   * 熟悉这些变体，能帮助你读懂别人写的代码，也能让你写出更简洁的代码。
   */

  // 变体 1：全局计数器可以用全局变量、静态局部变量或静态全局变量实现
  // 写法 A：全局变量（所有函数都能访问）
  globalCounter++;
  Serial.print("全局变量计数：");
  Serial.println(globalCounter);

  // 写法 B：静态局部变量（只能在定义它的函数内访问，但生命周期贯穿程序）
  static int staticLocalCounter = 0;
  staticLocalCounter++;
  Serial.print("静态局部变量计数：");
  Serial.println(staticLocalCounter);

  // 写法 C：静态全局变量（只能在本文件内访问）
  filePrivateCounter++;
  Serial.print("静态全局变量计数：");
  Serial.println(filePrivateCounter);

  // 变体 2：跨文件共享变量可以用 extern 声明
  // 假设另一个文件定义了 int sharedValue = 0;
  // 在本文件中可以这样声明后使用：
  // extern int sharedValue;
  // sharedValue++;
  // 注意：这里只是演示写法，实际没有另一个文件，不能真正运行

  // 变体 3：volatile 可以放在类型前或后，效果相同
  volatile int v1 = 0;  // 常见写法
  int volatile v2 = 0;  // 等价写法，volatile 修饰的是 int

  v1 = 100;
  v2 = 200;
  Serial.print("volatile int = ");
  Serial.print(v1);
  Serial.print(", int volatile = ");
  Serial.println(v2);

  // 变体 4：指针变量也可以加 volatile
  // 普通指针指向 volatile 数据
  volatile int* pVol = &v1;
  // volatile 指针指向普通数据（指针本身 volatile，指向的数据不 volatile）
  int* volatile pPtr = &v2;
  *pVol = 150;
  *pPtr = 250;
  Serial.print("*pVol = ");
  Serial.print(*pVol);
  Serial.print(", *pPtr = ");
  Serial.println(*pPtr);

  /*
   * ===== 常见陷阱 =====
   *
   * 下面这些代码看起来像是正确的，但要么编译不过，要么运行结果不对。
   * 每个错误示例旁边都给出了正确写法，注意对比。
   */

  // 陷阱 1：在函数内部定义与全局变量同名的局部变量，导致"隐藏"
  // 错误写法：误以为修改的是全局变量
  // int globalCounter = 999;  // 在函数内重新定义，会隐藏全局变量
  // 正确写法：如果确实要修改全局变量，不要重复定义
  globalCounter = 999;
  Serial.print("修改后的 globalCounter = ");
  Serial.println(globalCounter);

  // 陷阱 2：忘记给中断或硬件寄存器变量加 volatile
  // 错误写法：
  // int sensorValueBad = 0;
  // 在中断里修改 sensorValueBad，主循环读取时编译器可能优化掉读取操作
  // 正确写法：
  volatile int sensorValueGood = 0;
  sensorValueGood = analogRead(34);
  Serial.print("加 volatile 的 sensorValueGood = ");
  Serial.println(sensorValueGood);

  // 陷阱 3：static 局部变量初始化时机理解错误
  // 错误写法：以为每次进入函数都会重新初始化
  // void func() { static int x = 0; x = 0; ... }  // 把 x = 0 写成 x = 0 会每次清零
  // 正确写法：static 只初始化一次，后续保留值
  // void func() { static int x = 0; x++; ... }
  static int initDemo = 0;
  initDemo++;
  Serial.print("static initDemo 只初始化一次，当前值 = ");
  Serial.println(initDemo);

  // 陷阱 4：局部变量返回地址
  // 错误写法：
  // int* badFunc() { int local = 5; return &local; }
  // 函数结束后 local 被销毁，返回的指针成为"悬空指针"
  // 正确写法：使用全局变量、静态局部变量，或让调用者提供缓冲区
  static int safeStatic = 5;
  int* safePointer = &safeStatic;
  Serial.print("安全的静态变量地址值 = ");
  Serial.println(*safePointer);
}

void loop() {
  // loopLocal 是 loop() 内部的局部变量
  // 每次 loop() 执行都会重新创建和销毁
  int loopLocal = 0;
  loopLocal++;

  // staticLoopLocal 是静态局部变量
  // 只在第一次进入 loop() 时初始化，之后保持上次的值
  static int staticLoopLocal = 0;
  staticLoopLocal++;

  Serial.print("loopLocal = ");
  Serial.print(loopLocal);
  Serial.print(", staticLoopLocal = ");
  Serial.println(staticLoopLocal);

  // 演示全局变量在 loop 中持续累加
  globalCounter++;
  Serial.print("globalCounter = ");
  Serial.println(globalCounter);

  delay(2000);
}

// ========== 局部变量演示 ==========
void demoLocalVariable(void) {
  // localVar 每次进入函数时创建，退出函数时销毁
  int localVar = 0;
  localVar++;

  Serial.print("demoLocalVariable 中 localVar = ");
  Serial.println(localVar);
  // 每次输出都是 1，因为 localVar 每次都重新创建
}

// ========== 静态局部变量演示 ==========
void demoStaticVariable(void) {
  // staticVar 只在第一次进入函数时初始化
  // 之后保持上次的值，直到程序结束
  static int staticVar = 0;
  staticVar++;

  Serial.print("demoStaticVariable 中 staticVar = ");
  Serial.println(staticVar);
  // 第一次输出 1，第二次 2，第三次 3
}

// ========== volatile 演示 ==========
void demoVolatile(void) {
  // volatile 告诉编译器：这个变量可能会被程序外部因素改变
  // 不要对读取/写入这个变量的操作做优化

  // 典型场景 1：硬件寄存器
  // 例如：volatile uint32_t* reg = (volatile uint32_t*)0x3FF44000;

  // 典型场景 2：中断服务函数中修改的变量
  // 例如：volatile bool buttonPressed = false;

  // 典型场景 3：多任务中共享的变量

  volatile int sensorValue = 0;

  // 模拟 sensorValue 被外部修改
  sensorValue = 100;

  Serial.print("sensorValue = ");
  Serial.println(sensorValue);
  Serial.println("volatile 保证每次读取都从内存取最新值，而不是用缓存的旧值");
}

/*
 * 练习：
 * 1. 在 loop() 中定义一个局部变量和一个静态局部变量，观察它们的区别。
 * 2. 写一个函数，用静态局部变量记录自己被调用了多少次。
 * 3. 解释为什么中断中修改的变量要加 volatile。
 * 4. 尝试在不同函数中使用同名局部变量，观察是否互相影响。
 */
