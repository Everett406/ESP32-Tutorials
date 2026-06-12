/*
 * ESP32 学 C 语言 06：数组
 *
 * 学习目标：
 * 1. 理解什么是数组，以及为什么要用数组
 * 2. 掌握一维数组的声明、初始化和访问
 * 3. 掌握二维数组的基本用法
 * 4. 学会用循环遍历数组
 * 5. 理解数组越界的危害
 *
 * 什么是数组？
 * 数组是一组相同类型数据的集合，用一个变量名统一管理。
 * 例如：一个班级 30 个学生的成绩，如果不用数组，需要定义 30 个变量；
 * 用数组就只需要定义一个 `int scores[30]`。
 */

#include <stdio.h>
#include <string.h>

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("ESP32 学 C 语言 06：数组");

  // ========== 一维数组 ==========

  Serial.println("\n===== 一维数组 =====");

  // 声明一个包含 5 个整数的数组
  // 语法：类型 数组名[元素个数];
  int temperatures[5];

  // 给数组元素赋值
  // 数组下标从 0 开始，不是从 1 开始！
  temperatures[0] = 20;
  temperatures[1] = 22;
  temperatures[2] = 25;
  temperatures[3] = 23;
  temperatures[4] = 21;

  // 打印数组元素
  Serial.print("temperatures[0] = ");
  Serial.println(temperatures[0]);
  Serial.print("temperatures[2] = ");
  Serial.println(temperatures[2]);

  // 声明时直接初始化
  int scores[5] = {85, 92, 78, 90, 88};

  // 用循环遍历数组
  Serial.println("\n遍历 scores 数组：");
  for (int i = 0; i < 5; i++) {
    Serial.print("scores[");
    Serial.print(i);
    Serial.print("] = ");
    Serial.println(scores[i]);
  }

  // 计算数组平均值
  int sum = 0;
  for (int i = 0; i < 5; i++) {
    sum += scores[i];  // 累加每个元素
  }
  float average = (float)sum / 5;
  Serial.print("平均分：");
  Serial.println(average);

  // ========== 求数组最大值和最小值 ==========

  Serial.println("\n===== 最大值和最小值 =====");
  int maxVal = scores[0];
  int minVal = scores[0];

  for (int i = 1; i < 5; i++) {
    if (scores[i] > maxVal) {
      maxVal = scores[i];
    }
    if (scores[i] < minVal) {
      minVal = scores[i];
    }
  }

  Serial.print("最高分：");
  Serial.println(maxVal);
  Serial.print("最低分：");
  Serial.println(minVal);

  // ========== 数组大小自动计算 ==========

  Serial.println("\n===== 自动计算数组长度 =====");
  // 如果初始化时提供了所有元素，可以省略数组长度
  int numbers[] = {10, 20, 30, 40, 50, 60};

  // sizeof(数组名) 返回整个数组占用的字节数
  // sizeof(数组名[0]) 返回一个元素占用的字节数
  // 两者相除就得到数组元素个数
  int length = sizeof(numbers) / sizeof(numbers[0]);

  Serial.print("numbers 数组长度：");
  Serial.println(length);

  Serial.println("numbers 数组内容：");
  for (int i = 0; i < length; i++) {
    Serial.println(numbers[i]);
  }

  // ========== 二维数组 ==========

  Serial.println("\n===== 二维数组 =====");
  // 二维数组可以看成表格或矩阵
  // 语法：类型 数组名[行数][列数];
  int matrix[3][3] = {
    {1, 2, 3},
    {4, 5, 6},
    {7, 8, 9}
  };

  Serial.println("3x3 矩阵：");
  for (int row = 0; row < 3; row++) {
    for (int col = 0; col < 3; col++) {
      Serial.print(matrix[row][col]);
      Serial.print(" ");
    }
    Serial.println();
  }

  // ========== 数组越界警告 ==========

  Serial.println("\n===== 数组越界警告 =====");
  // 数组越界是指访问了不存在的下标。
  // 例如 scores[5] 是不存在的，因为合法下标只有 0~4。
  // 越界访问不会报错，但会读写不属于数组的内存，可能导致程序崩溃或行为异常！
  // 这是 C 语言非常常见的 bug，一定要小心。

  Serial.println("scores 数组合法下标：0 ~ 4");
  Serial.println("访问 scores[5] 或 scores[-1] 都是越界，不要这样做！");

  /*
   * ===== 写法变体 =====
   *
   * 同一个功能往往有多种正确的写法。
   * 熟悉这些变体，能帮助你读懂别人写的代码，也能让你写出更简洁的代码。
   */

  Serial.println("\n===== 写法变体 =====");

  // 1. 可以先声明数组，再逐个赋值
  int a1[3];
  a1[0] = 10;
  a1[1] = 20;
  a1[2] = 30;

  // 2. 也可以在声明时一次性初始化，两者完全等价
  int a2[3] = {10, 20, 30};

  // 3. 初始化时如果给出所有元素，数组长度可以省略
  int a3[] = {10, 20, 30};

  // 4. 只给部分元素初始化时，剩余元素自动为 0
  int a4[5] = {1, 2, 3};
  Serial.print("a4[4] 自动为 0：");
  Serial.println(a4[4]);

  // 5. 用 sizeof 自动计算长度，再遍历数组
  int a5[] = {100, 200, 300, 400};
  int lenA5 = sizeof(a5) / sizeof(a5[0]);
  Serial.print("a5 长度：");
  Serial.println(lenA5);
  for (int i = 0; i < lenA5; i++) {
    Serial.println(a5[i]);
  }

  /*
   * ===== 常见陷阱 =====
   *
   * 下面这些代码看起来像是正确的，但要么编译不过，要么运行结果不对。
   * 每个错误示例旁边都给出了正确写法，注意对比。
   */

  Serial.println("\n===== 常见陷阱 =====");

  // 陷阱 1：访问超出合法下标的元素，造成数组越界
  int b1[3] = {1, 2, 3};
  // 错误写法（已注释掉，不要运行）：
  // b1[3] = 4;  // 合法下标只有 0、1、2
  // 正确写法：访问最后一个元素用 b1[2]
  Serial.print("最后一个元素 b1[2] = ");
  Serial.println(b1[2]);

  // 陷阱 2：把数组传给函数或指针后，再用 sizeof 求长度会出错
  int b2[4] = {10, 20, 30, 40};
  int* p = b2;
  // 错误思路（已注释掉，不要运行）：
  // int wrongLen = sizeof(p) / sizeof(p[0]);  // p 是指针，结果不是 4
  // 正确写法：在数组还保持"数组类型"时求长度
  int rightLen = sizeof(b2) / sizeof(b2[0]);
  Serial.print("b2 正确长度：");
  Serial.println(rightLen);

  // 陷阱 3：数组名是地址常量，不能被整体赋值
  char str[10];
  // 错误写法（已注释掉，不要运行）：
  // str = "hello";  // 编译错误
  // 正确写法 1：声明时直接初始化
  char str2[10] = "hello";
  // 正确写法 2：用字符串函数拷贝
  strcpy(str, "hello");
  Serial.print("正确字符串：");
  Serial.println(str);
  Serial.print("声明时初始化的字符串：");
  Serial.println(str2);

  // 陷阱 4：二维数组初始化时混淆行和列
  // 错误写法（已注释掉，不要运行）：
  // int mat[2][3] = { {1,2}, {3,4}, {5,6} };  // 行数不对，会报错或结果混乱
  // 正确写法：
  int mat[2][3] = {
    {1, 2, 3},
    {4, 5, 6}
  };
  Serial.print("mat[1][2] = ");
  Serial.println(mat[1][2]);
}

void loop() {
  // 本课重点在 setup() 中演示数组
}

/*
 * 练习：
 * 1. 定义一个数组存储 7 天的温度，计算一周的平均温度和最高温度。
 * 2. 把数组元素倒序打印出来。
 * 3. 写一个函数，接收一个数组和长度，返回数组元素之和。
 *    （提示：学完函数后再做这题）
 * 4. 用二维数组表示一个 2x4 的 LED 点阵状态，并在串口打印出来。
 */
