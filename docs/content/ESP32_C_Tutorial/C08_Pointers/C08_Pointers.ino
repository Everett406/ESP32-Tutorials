/*
 * ESP32 学 C 语言 08：指针
 *
 * 学习目标：
 * 1. 理解什么是指针，以及指针和变量的关系
 * 2. 掌握取地址运算符 & 和解引用运算符 *
 * 3. 理解指针作为函数参数的作用
 * 4. 理解指针和数组的关系
 * 5. 了解空指针和野指针的危害
 *
 * 什么是指针？
 * 指针是一种特殊的变量，它存储的不是普通的数据，而是另一个变量的内存地址。
 * 可以把指针理解为"地址本"，上面记着某个数据住在哪里。
 *
 * 为什么需要指针？
 * 1. 函数之间共享和修改数据（地址传递）
 * 2. 动态分配内存
 * 3. 高效操作数组和字符串
 * 4. 访问硬件寄存器（嵌入式开发非常重要）
 */

#include <stdio.h>

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("ESP32 学 C 语言 08：指针");

  // ========== 指针基础 ==========

  Serial.println("\n===== 指针基础 =====");

  int age = 25;

  // &age 表示取变量 age 的地址
  // %p 用来打印指针地址（十六进制）
  Serial.print("age 的值：");
  Serial.println(age);
  Serial.print("age 的地址：");
  Serial.println((unsigned long)&age, HEX);

  // 声明一个指向 int 的指针
  // int* 表示"指向 int 的指针"
  int* pAge = &age;

  Serial.print("pAge 存储的地址：");
  Serial.println((unsigned long)pAge, HEX);

  // *pAge 表示解引用，即访问指针指向的变量的值
  Serial.print("*pAge 的值：");
  Serial.println(*pAge);

  // 通过指针修改原变量的值
  *pAge = 30;
  Serial.print("通过指针修改后，age = ");
  Serial.println(age);

  // ========== 指针的大小 ==========

  Serial.println("\n===== 指针的大小 =====");
  // 在 ESP32 上，指针变量本身占用 4 字节（32 位系统）
  // 不管指向什么类型，指针变量的大小都是一样的
  Serial.print("sizeof(int*) = ");
  Serial.println(sizeof(int*));
  Serial.print("sizeof(char*) = ");
  Serial.println(sizeof(char*));
  Serial.print("sizeof(double*) = ");
  Serial.println(sizeof(double*));

  // ========== 指针与函数 ==========

  Serial.println("\n===== 指针修改函数外部变量 =====");
  int a = 10;
  int b = 20;
  Serial.print("调用前：a = ");
  Serial.print(a);
  Serial.print(", b = ");
  Serial.println(b);

  modifyValue(&a, 100);
  modifyValue(&b, 200);

  Serial.print("调用后：a = ");
  Serial.print(a);
  Serial.print(", b = ");
  Serial.println(b);

  // ========== 指针与数组 ==========

  Serial.println("\n===== 指针与数组 =====");
  int numbers[5] = {10, 20, 30, 40, 50};

  // 数组名本质上就是数组首元素的地址
  int* pNumbers = numbers;

  Serial.print("numbers[0] = ");
  Serial.println(numbers[0]);
  Serial.print("*pNumbers = ");
  Serial.println(*pNumbers);

  // 指针算术：pNumbers + 1 表示下一个 int 元素的地址
  Serial.print("*(pNumbers + 1) = ");
  Serial.println(*(pNumbers + 1));  // 20
  Serial.print("*(pNumbers + 2) = ");
  Serial.println(*(pNumbers + 2));  // 30

  // 用指针遍历数组
  Serial.println("\n用指针遍历数组：");
  for (int i = 0; i < 5; i++) {
    Serial.print("元素 ");
    Serial.print(i);
    Serial.print(" = ");
    Serial.println(*(pNumbers + i));
  }

  // ========== 空指针和野指针 ==========

  Serial.println("\n===== 空指针和野指针 =====");

  // NULL 表示指针不指向任何有效地址
  int* pNull = NULL;
  Serial.print("pNull = ");
  Serial.println((unsigned long)pNull, HEX);

  // 对空指针解引用会导致程序崩溃，所以使用前必须检查
  if (pNull != NULL) {
    Serial.println(*pNull);
  }
  else {
    Serial.println("pNull 是空指针，不能解引用！");
  }

  // 野指针：指向不确定地址的指针
  // int* pWild;  // 只声明未初始化，里面是随机值，非常危险！
  // 使用野指针可能导致不可预测的行为

  /*
   * ===== 写法变体 =====
   * 
   * 同一个功能往往有多种正确的写法。
   * 熟悉这些变体，能帮助你读懂别人写的代码，也能让你写出更简洁的代码。
   */

  Serial.println("\n===== 指针写法变体 =====");

  // 写法变体 1：声明时赋值和先声明后赋值是等价的
  int xv = 100;
  int* p1 = &xv;        // 声明的同时初始化
  int* p2;
  p2 = &xv;             // 先声明，再赋值
  Serial.print("p1 和 p2 指向同一变量，*p1 = ");
  Serial.print(*p1);
  Serial.print(", *p2 = ");
  Serial.println(*p2);

  // 写法变体 2：int* p 和 int *p 只是风格不同，意思一样
  // int* p 强调"p 是指针"，int *p 强调"*p 是 int"
  int* p3;   // 星号靠近类型
  int *p4;   // 星号靠近变量名
  p3 = &xv;
  p4 = &xv;

  // 写法变体 3：用数组下标和指针算术访问数组元素是等价的
  int arr[4] = {10, 20, 30, 40};
  int* pArr = arr;
  Serial.print("arr[2] = ");
  Serial.println(arr[2]);          // 下标写法
  Serial.print("*(pArr + 2) = ");
  Serial.println(*(pArr + 2));     // 指针算术写法
  Serial.print("pArr[2] = ");
  Serial.println(pArr[2]);         // 指针也可以当下标用

  // 写法变体 4：给指针加整数时，实际加的是"元素个数 * sizeof(类型)"
  // pArr + 1 不是地址加 1，而是加一个 int 的大小
  Serial.print("pArr 的地址：");
  Serial.println((unsigned long)pArr, HEX);
  Serial.print("pArr + 1 的地址：");
  Serial.println((unsigned long)(pArr + 1), HEX);

  /*
   * ===== 常见陷阱 =====
   * 
   * 下面这些代码看起来像是正确的，但要么编译不过，要么运行结果不对。
   * 每个错误示例旁边都给出了正确写法，注意对比。
   */

  Serial.println("\n===== 指针常见陷阱 =====");

  // 陷阱 1：int* p1, p2; 只有 p1 是指针，p2 是 int
  // 错误写法（已注释掉）：
  // int* pA, pB;   // pA 是 int*，pB 只是 int
  // 正确写法：一行一个，或者每个变量都加星号
  int* pA;
  int* pB;
  int varA = 1, varB = 2;
  pA = &varA;
  pB = &varB;
  Serial.print("pA 和 pB 都是指针：");
  Serial.print(*pA);
  Serial.print(", ");
  Serial.println(*pB);

  // 陷阱 2：未初始化的指针直接解引用
  // 错误写法（已注释掉）：
  // int* pWild;
  // *pWild = 10;   // pWild 指向随机地址，可能崩溃或破坏内存
  // 正确写法：初始化后再用
  int safeVar = 10;
  int* pSafe = &safeVar;
  *pSafe = 20;
  Serial.print("安全指针修改后：");
  Serial.println(safeVar);

  // 陷阱 3：空指针没有检查就解引用
  int* pNull2 = NULL;
  // 错误写法：Serial.println(*pNull2);  // 会崩溃
  // 正确写法：
  if (pNull2 != NULL) {
    Serial.println(*pNull2);
  } else {
    Serial.println("空指针要先检查，不能直接解引用！");
  }

  // 陷阱 4：把普通整数直接当成地址赋给指针
  // 错误写法（已注释掉）：
  // int* pAddr = 0x1234;  // 0x1234 不一定可访问，运行会出错
  // 正确写法：让指针指向真实存在的变量
  int realVar = 5;
  int* pReal = &realVar;
  Serial.print("指向真实变量的指针：");
  Serial.println(*pReal);
}

void loop() {
  // 主循环保持为空
}

// 通过指针修改外部变量的值
void modifyValue(int* p, int newValue) {
  *p = newValue;
}

/*
 * 练习：
 * 1. 定义一个 int 变量，定义一个指针指向它，分别通过变量名和指针修改变量值，观察结果。
 * 2. 写一个函数 swap(int* a, int* b)，交换两个变量的值。
 * 3. 写一个函数 sumArray(int* arr, int len)，计算数组元素之和。
 * 4. 解释为什么 int* p = numbers; 和 int* p = &numbers[0]; 是等价的。
 */
