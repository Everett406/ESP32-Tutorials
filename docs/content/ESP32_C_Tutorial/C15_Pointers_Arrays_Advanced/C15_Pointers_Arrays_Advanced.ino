/*
 * ESP32 学 C 语言 15：指针与数组进阶
 *
 * 学习目标：
 * 1. 深入理解数组名、指针、地址的关系
 * 2. 掌握指针数组和数组指针
 * 3. 理解函数指针的基本概念
 * 4. 理解 const 与指针的组合
 * 5. 学会用指针高效操作内存
 *
 * 本课是 C 语言指针和数组的进阶内容，建议先掌握 C06、C07、C08 再学习。
 */

#include <stdio.h>

// 函数声明
void printArray(int* arr, int len);
int compareAscending(const void* a, const void* b);

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("ESP32 学 C 语言 15：指针与数组进阶");

  // ========== 数组名与指针的关系 ==========

  Serial.println("\n===== 数组名与指针 =====");

  int arr[5] = {10, 20, 30, 40, 50};

  // 数组名 arr 大多数情况下会被隐式转换成指向首元素的指针
  // 所以 arr 和 &arr[0] 是等价的
  Serial.print("arr 的值（地址）：");
  Serial.println((unsigned long)arr, HEX);
  Serial.print("&arr[0] 的值：");
  Serial.println((unsigned long)&arr[0], HEX);

  // 访问数组元素的多种等价写法
  Serial.print("arr[2] = ");
  Serial.println(arr[2]);
  Serial.print("*(arr + 2) = ");
  Serial.println(*(arr + 2));
  Serial.print("2[arr] = ");  // 这种写法合法但很少用
  Serial.println(2[arr]);

  // ========== 指针数组 ==========

  Serial.println("\n===== 指针数组 =====");
  // 指针数组：数组中的每个元素都是指针
  // 常用于存储多个字符串

  const char* names[] = {
    "Alice",
    "Bob",
    "Charlie",
    "David"
  };

  int nameCount = sizeof(names) / sizeof(names[0]);
  Serial.println("学生名单：");
  for (int i = 0; i < nameCount; i++) {
    Serial.print(i);
    Serial.print(": ");
    Serial.println(names[i]);
  }

  // ========== 数组指针 ==========

  Serial.println("\n===== 数组指针 =====");
  // 数组指针：指向整个数组的指针

  int matrix[3][4] = {
    {1, 2, 3, 4},
    {5, 6, 7, 8},
    {9, 10, 11, 12}
  };

  // matrix 是一个指向包含 4 个 int 的数组的指针
  // 即 int (*)[4]
  int (*pMatrix)[4] = matrix;

  Serial.print("matrix[1][2] = ");
  Serial.println(matrix[1][2]);
  Serial.print("pMatrix[1][2] = ");
  Serial.println(pMatrix[1][2]);
  Serial.print("*(*(pMatrix + 1) + 2) = ");
  Serial.println(*(*(pMatrix + 1) + 2));

  // ========== const 与指针 ==========

  Serial.println("\n===== const 与指针 =====");

  int value = 100;

  // 指向常量的指针：指针指向的内容不能通过指针修改
  const int* p1 = &value;
  // *p1 = 200;  // 错误！不能通过 p1 修改 value
  p1 = NULL;     // 正确，指针本身可以指向别处

  // 常量指针：指针本身不能修改，但指向的内容可以修改
  int* const p2 = &value;
  *p2 = 200;     // 正确
  // p2 = NULL;  // 错误！p2 不能指向别处

  // 指向常量的常量指针：都不能修改
  const int* const p3 = &value;
  // *p3 = 300;  // 错误
  // p3 = NULL;  // 错误

  Serial.print("value = ");
  Serial.println(value);

  // ========== 函数指针 ==========

  Serial.println("\n===== 函数指针 =====");

  // 函数指针：指向函数的指针
  // 可以用来实现回调函数、状态机等
  int (*operation)(int, int);

  // 让函数指针指向 add 函数
  operation = add;
  Serial.print("add(3, 5) = ");
  Serial.println(operation(3, 5));

  // 让函数指针指向 multiply 函数
  operation = multiply;
  Serial.print("multiply(3, 5) = ");
  Serial.println(operation(3, 5));

  // ========== 用函数指针实现简单计算器 ==========

  Serial.println("\n===== 函数指针计算器 =====");
  int x = 12, y = 4;
  char op = '/';

  int (*calc)(int, int) = NULL;

  switch (op) {
    case '+': calc = add; break;
    case '-': calc = subtract; break;
    case '*': calc = multiply; break;
    case '/': calc = divide; break;
  }

  if (calc != NULL) {
    Serial.print("12 / 4 = ");
    Serial.println(calc(x, y));
  }

  // ========== 指针作为函数参数传递数组 ==========

  Serial.println("\n===== 指针传递数组 =====");
  int numbers[] = {64, 34, 25, 12, 22, 11, 90};
  int len = sizeof(numbers) / sizeof(numbers[0]);

  Serial.println("排序前：");
  printArray(numbers, len);

  // qsort 是 C 标准库的快速排序函数
  // 参数：数组地址、元素个数、每个元素大小、比较函数指针
  qsort(numbers, len, sizeof(int), compareAscending);

  Serial.println("排序后：");
  printArray(numbers, len);

  /*
   * ===== 写法变体 =====
   *
   * 同一个功能往往有多种正确的写法。
   * 熟悉这些变体，能帮助你读懂别人写的代码，也能让你写出更简洁的代码。
   */

  // 变体 1：访问数组元素的多种等价写法
  int sample[] = {10, 20, 30, 40, 50};

  // 以下三种写法访问的是同一个元素
  Serial.print("sample[2] = ");
  Serial.println(sample[2]);
  Serial.print("*(sample + 2) = ");
  Serial.println(*(sample + 2));
  Serial.print("*(2 + sample) = ");
  Serial.println(*(2 + sample));

  // 变体 2：指针声明和数组遍历的多种写法
  int* p1 = sample;      // 指针指向数组首元素
  int* p2 = &sample[0];  // 等价写法

  Serial.print("*p1 = ");
  Serial.println(*p1);
  Serial.print("*p2 = ");
  Serial.println(*p2);

  // 用指针遍历数组
  Serial.print("指针遍历：");
  for (int* p = sample; p < sample + 5; p++) {
    Serial.print(*p);
    Serial.print(" ");
  }
  Serial.println();

  // 用下标遍历数组
  Serial.print("下标遍历：");
  for (int i = 0; i < 5; i++) {
    Serial.print(sample[i]);
    Serial.print(" ");
  }
  Serial.println();

  // 变体 3：const 与指针的多种组合
  int val = 100;

  // 指向常量的指针：内容不能改，指针可以改
  const int* cp1 = &val;
  cp1 = NULL;  // 合法
  // *cp1 = 200;  // 非法

  // 常量指针：指针不能改，内容可以改
  int* const cp2 = &val;
  *cp2 = 200;  // 合法
  // cp2 = NULL;  // 非法

  // 指向常量的常量指针：都不能改
  const int* const cp3 = &val;
  // *cp3 = 300;  // 非法
  // cp3 = NULL;   // 非法

  Serial.print("const 演示后 val = ");
  Serial.println(val);

  // 变体 4：函数指针的两种赋值和调用写法
  int (*funcPtr)(int, int);

  // 赋值时可以加取地址符，也可以不加
  funcPtr = add;       // 等价
  funcPtr = &add;      // 也等价

  // 调用时可以用 *，也可以不用
  Serial.print("funcPtr(4, 6) = ");
  Serial.println(funcPtr(4, 6));
  Serial.print("(*funcPtr)(4, 6) = ");
  Serial.println((*funcPtr)(4, 6));

  /*
   * ===== 常见陷阱 =====
   *
   * 下面这些代码看起来像是正确的，但要么编译不过，要么运行结果不对。
   * 每个错误示例旁边都给出了正确写法，注意对比。
   */

  // 陷阱 1：数组名不是指针变量，不能赋值
  // 错误写法：
  // int arr[5];
  // arr = someOtherArray;  // 数组名是常量地址，不能作为左值
  // 正确写法：用元素逐个拷贝，或用 memcpy
  int src[] = {1, 2, 3, 4, 5};
  int dst[5];
  for (int i = 0; i < 5; i++) {
    dst[i] = src[i];
  }
  Serial.print("拷贝后 dst[0] = ");
  Serial.println(dst[0]);

  // 陷阱 2：const 指针类型混淆
  // 错误写法：以为 const int* p 和 int* const p 是一回事
  // const int* p 表示"指向的内容不能改"
  // int* const p 表示"指针本身不能改"
  const int* pConstContent = &val;
  // *pConstContent = 500;  // 错误！内容不能改
  pConstContent = &src[0];  // 正确，指针可以指向别处

  int* const pConstPointer = &val;
  *pConstPointer = 500;  // 正确，内容可以改
  // pConstPointer = &src[0];  // 错误！指针不能改

  Serial.print("const 指针演示后 val = ");
  Serial.println(val);

  // 陷阱 3：函数指针语法写错
  // 错误写法：
  // int *func(int, int);   // 这是声明一个返回 int* 的函数，不是函数指针
  // int (*func)(int, int); // 这才是函数指针
  // 正确写法：
  int (*correctFuncPtr)(int, int) = add;
  Serial.print("正确函数指针调用结果：");
  Serial.println(correctFuncPtr(2, 3));

  // 陷阱 4：sizeof 数组和 sizeof 指针结果不同
  // 错误写法：在函数内部用 sizeof(arr) 计算数组长度
  // void badLen(int arr[]) { int n = sizeof(arr) / sizeof(arr[0]); }
  // 这里的 arr 已经退化成指针，sizeof(arr) 等于指针大小，不是数组总大小
  // 正确写法：把数组长度作为参数传入
  int correctLen = sizeof(sample) / sizeof(sample[0]);
  Serial.print("正确的数组长度：");
  Serial.println(correctLen);
}

void loop() {
  // 主循环保持为空
}

// ========== 辅助函数 ==========

int add(int a, int b) {
  return a + b;
}

int subtract(int a, int b) {
  return a - b;
}

int multiply(int a, int b) {
  return a * b;
}

int divide(int a, int b) {
  if (b == 0) {
    Serial.println("错误：除数不能为 0");
    return 0;
  }
  return a / b;
}

void printArray(int* arr, int len) {
  for (int i = 0; i < len; i++) {
    Serial.print(arr[i]);
    Serial.print(" ");
  }
  Serial.println();
}

// qsort 需要的比较函数
// 返回负数表示 a < b，0 表示相等，正数表示 a > b
int compareAscending(const void* a, const void* b) {
  int numA = *(const int*)a;
  int numB = *(const int*)b;
  return numA - numB;
}

/*
 * 练习：
 * 1. 解释 arr[i] 和 *(arr + i) 为什么等价。
 * 2. 写一个函数，用指针方式把字符串中的小写字母转为大写。
 * 3. 用函数指针数组实现一个更完整的计算器。
 * 4. 写一个冒泡排序函数，参数为 int* arr 和 int len。
 */
