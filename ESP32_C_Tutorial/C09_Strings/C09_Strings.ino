/*
 * ESP32 学 C 语言 09：字符串
 *
 * 学习目标：
 * 1. 理解 C 语言字符串的本质：以 '\0' 结尾的字符数组
 * 2. 掌握字符串的声明和初始化方式
 * 3. 学会常用的字符串处理函数
 * 4. 理解字符串操作中的缓冲区溢出风险
 *
 * 什么是 C 语言字符串？
 * C 语言没有专门的字符串类型。
 * 字符串实际上是一个字符数组，最后一个字符必须是 '\0'（空字符）。
 * '\0' 是字符串的结束标志，告诉各种字符串函数"到这里就结束了"。
 */

#include <stdio.h>
#include <string.h>

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("ESP32 学 C 语言 09：字符串");

  // ========== 字符串的声明方式 ==========

  Serial.println("\n===== 字符串声明方式 =====");

  // 方式 1：用字符串常量初始化
  // 编译器会自动在末尾添加 '\0'
  char str1[] = "Hello";

  Serial.print("str1 = ");
  Serial.println(str1);
  Serial.print("str1 长度：");
  Serial.println(strlen(str1));  // 5，不包含 '\0'
  Serial.print("str1 占用空间：");
  Serial.println(sizeof(str1));  // 6，包含 '\0'

  // 方式 2：逐个字符初始化
  char str2[] = {'H', 'e', 'l', 'l', 'o', '\0'};
  Serial.print("str2 = ");
  Serial.println(str2);

  // 方式 3：指定数组大小
  // 必须留出 '\0' 的位置
  char str3[20] = "ESP32";
  Serial.print("str3 = ");
  Serial.println(str3);

  // ========== 字符串的长度 ==========

  Serial.println("\n===== 字符串长度 =====");
  char message[] = "Hello, ESP32!";

  // strlen() 计算字符串长度（不包含 '\0'）
  size_t len = strlen(message);
  Serial.print("message 长度：");
  Serial.println(len);

  // sizeof() 计算数组总大小（包含 '\0'）
  Serial.print("message 数组大小：");
  Serial.println(sizeof(message));

  // ========== 字符串拷贝 ==========

  Serial.println("\n===== 字符串拷贝 =====");
  char source[] = "Source";
  char destination[20];

  // strcpy(dst, src) 把 src 拷贝到 dst
  // 目标数组必须足够大，否则会发生缓冲区溢出！
  strcpy(destination, source);
  Serial.print("拷贝后：");
  Serial.println(destination);

  // 更安全的版本：strncpy(dst, src, n)
  // 最多拷贝 n 个字符，但要注意它不会自动添加 '\0'
  char safeDest[10];
  strncpy(safeDest, source, sizeof(safeDest) - 1);
  safeDest[sizeof(safeDest) - 1] = '\0';  // 手动确保结尾
  Serial.print("安全拷贝后：");
  Serial.println(safeDest);

  // ========== 字符串连接 ==========

  Serial.println("\n===== 字符串连接 =====");
  char buffer[50] = "Hello";

  // strcat(dst, src) 把 src 连接到 dst 末尾
  // 同样要注意目标数组大小
  strcat(buffer, ", ESP32!");
  Serial.print("连接后：");
  Serial.println(buffer);

  // ========== 字符串比较 ==========

  Serial.println("\n===== 字符串比较 =====");
  char s1[] = "apple";
  char s2[] = "banana";
  char s3[] = "apple";

  // strcmp() 按字典序比较字符串
  // 返回 0 表示相等
  // 返回负数表示 s1 < s2
  // 返回正数表示 s1 > s2
  int result1 = strcmp(s1, s2);
  int result2 = strcmp(s1, s3);

  Serial.print("strcmp(apple, banana) = ");
  Serial.println(result1);
  Serial.print("strcmp(apple, apple) = ");
  Serial.println(result2);

  // 注意：不能直接用 == 比较字符串！
  // s1 == s2 比较的是两个指针的地址，不是字符串内容

  // ========== 查找字符和子串 ==========

  Serial.println("\n===== 查找字符和子串 =====");
  char text[] = "Hello, ESP32 World!";

  // strchr() 查找字符第一次出现的位置
  char* pChar = strchr(text, 'E');
  if (pChar != NULL) {
    Serial.print("找到字符 'E'，位置：");
    Serial.println(pChar - text);  // 指针相减得到偏移量
  }

  // strstr() 查找子串第一次出现的位置
  char* pSub = strstr(text, "ESP32");
  if (pSub != NULL) {
    Serial.print("找到子串 \"ESP32\"，位置：");
    Serial.println(pSub - text);
  }

  // ========== 实际应用：串口命令解析 ==========

  Serial.println("\n===== 串口命令解析示例 =====");
  char command[] = "set duty 128";

  // strtok() 用于分割字符串
  // 第一个参数是字符串，第二个参数是分隔符
  // 后续调用传 NULL 表示继续分割同一个字符串
  char* token = strtok(command, " ");
  while (token != NULL) {
    Serial.print("命令片段：");
    Serial.println(token);
    token = strtok(NULL, " ");
  }

  /*
   * ===== 写法变体 =====
   * 
   * 同一个功能往往有多种正确的写法。
   * 熟悉这些变体，能帮助你读懂别人写的代码，也能让你写出更简洁的代码。
   */

  Serial.println("\n===== 字符串写法变体 =====");

  // 写法变体 1：字符数组初始化的几种等价形式
  char sA[] = "Hi";                 // 编译器自动加 '\0'
  char sB[] = {'H', 'i', '\0'};     // 手动列出每个字符
  char sC[10] = "Hi";               // 指定数组大小，后面自动补 0
  Serial.print("sA = ");
  Serial.println(sA);
  Serial.print("sB = ");
  Serial.println(sB);
  Serial.print("sC = ");
  Serial.println(sC);

  // 写法变体 2：用字符指针指向字符串常量
  // 注意：这种方式指向的是常量，通常不能修改
  const char* s4 = "Hi";
  Serial.print("s4 = ");
  Serial.println(s4);

  // 写法变体 3：拷贝字符串可以用 strcpy，也可以自己逐个字符复制
  char src[] = "ABC";
  char dst1[10];
  strcpy(dst1, src);                // 方式 A
  char dst2[10];
  int i = 0;
  while (src[i] != '\0') {          // 方式 B：手动拷贝
    dst2[i] = src[i];
    i++;
  }
  dst2[i] = '\0';                   // 别忘了结尾
  Serial.print("strcpy 拷贝：");
  Serial.println(dst1);
  Serial.print("手动拷贝：");
  Serial.println(dst2);

  /*
   * ===== 常见陷阱 =====
   * 
   * 下面这些代码看起来像是正确的，但要么编译不过，要么运行结果不对。
   * 每个错误示例旁边都给出了正确写法，注意对比。
   */

  Serial.println("\n===== 字符串常见陷阱 =====");

  // 陷阱 1：数组名不能重新赋值
  // 错误写法（已注释掉）：
  // char strA[10];
  // strA = "Hello";   // 数组名是常量地址，不能被赋值
  // 正确写法：用 strcpy 或在初始化时赋值
  char strA[10];
  strcpy(strA, "Hello");
  Serial.print("用 strcpy 赋值后：");
  Serial.println(strA);

  // 陷阱 2：用 == 比较字符串
  char a1[] = "cat";
  char a2[] = "cat";
  // 错误写法：if (a1 == a2) ...  // 比较的是地址，不是内容
  // 正确写法：
  if (strcmp(a1, a2) == 0) {
    Serial.println("strcmp 判断：两个字符串内容相等");
  } else {
    Serial.println("strcmp 判断：不相等");
  }

  // 陷阱 3：缓冲区溢出
  char small[4];
  // 错误写法（已注释掉）：
  // strcpy(small, "Hello");  // "Hello" 需要 6 字节（含 '\0'），small 只有 4 字节，会越界
  // 正确写法：确保目标数组足够大，或用 strncpy
  strncpy(small, "Hi", sizeof(small) - 1);
  small[sizeof(small) - 1] = '\0';
  Serial.print("安全拷贝到小数组：");
  Serial.println(small);

  // 陷阱 4：修改字符串常量
  // 错误写法（已注释掉）：
  // char* pStr = "Hello";
  // pStr[0] = 'h';  // 字符串常量通常放在只读区域，修改可能崩溃
  // 正确写法：用字符数组
  char mutableStr[] = "Hello";
  mutableStr[0] = 'h';
  Serial.print("修改字符数组中的字符串：");
  Serial.println(mutableStr);
}

void loop() {
  // 主循环保持为空
}

/*
 * 练习：
 * 1. 写一个函数 stringLength(char str[])，自己实现 strlen 的功能。
 * 2. 写一个函数 reverseString(char str[])，把字符串反转。
 * 3. 判断一个字符串是否是回文（正读反读都相同，如 "level"）。
 * 4. 用 strtok 解析 "on 50" 命令，第一个词控制 LED 开关，第二个词设置亮度。
 */
