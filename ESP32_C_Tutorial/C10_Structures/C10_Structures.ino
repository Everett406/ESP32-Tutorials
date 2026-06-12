/*
 * ESP32 学 C 语言 10：结构体
 *
 * 学习目标：
 * 1. 理解什么是结构体，以及为什么要用结构体
 * 2. 掌握结构体的定义、声明和访问方式
 * 3. 理解结构体数组
 * 4. 理解指向结构体的指针
 * 5. 学会用 typedef 简化结构体类型名
 *
 * 什么是结构体？
 * 结构体是一种用户自定义的数据类型，可以把多个不同类型的数据组合在一起。
 * 例如：一个学生有学号（int）、姓名（char[]）、成绩（float），
 * 用结构体可以把它们打包成一个"学生"类型。
 *
 * 为什么需要结构体？
 * 1. 把相关的数据组织在一起，逻辑更清晰
 * 2. 函数可以一次返回/传递一组数据
 * 3. 在嵌入式中常用于表示传感器数据、配置参数、设备状态等
 */

#include <stdio.h>
#include <string.h>

// ========== 定义结构体 ==========

// struct 是 C 语言的关键字，用来定义结构体
// Student 是结构体的名字（也叫"结构体标签"）
struct Student {
  int id;           // 学号
  char name[20];    // 姓名
  float score;      // 成绩
};

// 使用 typedef 给结构体起一个简短的别名
// 这样声明变量时就不用写 struct 了
typedef struct {
  float temperature;
  float humidity;
  uint32_t timestamp;
} SensorData;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("ESP32 学 C 语言 10：结构体");

  // ========== 声明和使用结构体变量 ==========

  Serial.println("\n===== 结构体变量 =====");

  // 声明一个 Student 类型的变量
  struct Student stu1;

  // 用点号 . 访问结构体成员
  stu1.id = 1001;
  strcpy(stu1.name, "张三");
  stu1.score = 89.5f;

  Serial.print("学号：");
  Serial.println(stu1.id);
  Serial.print("姓名：");
  Serial.println(stu1.name);
  Serial.print("成绩：");
  Serial.println(stu1.score);

  // 声明时直接初始化
  struct Student stu2 = {1002, "李四", 92.0f};
  Serial.print("\n学号：");
  Serial.println(stu2.id);
  Serial.print("姓名：");
  Serial.println(stu2.name);

  // ========== 使用 typedef 的简化写法 ==========

  Serial.println("\n===== typedef 简化写法 =====");

  SensorData data;
  data.temperature = 25.5f;
  data.humidity = 60.0f;
  data.timestamp = millis();

  Serial.print("温度：");
  Serial.println(data.temperature);
  Serial.print("湿度：");
  Serial.println(data.humidity);
  Serial.print("时间戳：");
  Serial.println(data.timestamp);

  // ========== 结构体数组 ==========

  Serial.println("\n===== 结构体数组 =====");

  struct Student classStudents[3] = {
    {1001, "张三", 89.5f},
    {1002, "李四", 92.0f},
    {1003, "王五", 78.0f}
  };

  // 计算数组长度
  int count = sizeof(classStudents) / sizeof(classStudents[0]);

  Serial.println("班级学生成绩：");
  for (int i = 0; i < count; i++) {
    Serial.print(classStudents[i].id);
    Serial.print(" ");
    Serial.print(classStudents[i].name);
    Serial.print(" ");
    Serial.println(classStudents[i].score);
  }

  // 计算平均分
  float total = 0.0f;
  for (int i = 0; i < count; i++) {
    total += classStudents[i].score;
  }
  Serial.print("平均分：");
  Serial.println(total / count);

  // ========== 指向结构体的指针 ==========

  Serial.println("\n===== 结构体指针 =====");

  SensorData sensor;
  sensor.temperature = 30.0f;
  sensor.humidity = 55.0f;

  // 定义指向结构体的指针
  SensorData* pSensor = &sensor;

  // 用 -> 运算符通过指针访问结构体成员
  // pSensor->temperature 等价于 (*pSensor).temperature
  Serial.print("通过指针读取温度：");
  Serial.println(pSensor->temperature);

  // 通过指针修改结构体成员
  pSensor->humidity = 70.0f;
  Serial.print("修改后的湿度：");
  Serial.println(sensor.humidity);

  // ========== 函数传递结构体 ==========

  Serial.println("\n===== 函数中使用结构体 =====");

  SensorData reading = {27.3f, 45.0f, 123456};
  printSensorData(reading);

  // 修改结构体成员
  reading.temperature = 28.0f;
  printSensorData(reading);

  // 写法变体与常见陷阱演示
  demoStructVariations();
  demoStructTraps();
}

void loop() {
  // 主循环保持为空
}

// 接收结构体作为参数的函数
void printSensorData(SensorData data) {
  Serial.print("SensorData { temp=");
  Serial.print(data.temperature);
  Serial.print(", humi=");
  Serial.print(data.humidity);
  Serial.print(", time=");
  Serial.print(data.timestamp);
  Serial.println(" }");
}

/*
 * ===== 写法变体 =====
 *
 * 同一个功能往往有多种正确的写法。
 * 熟悉这些变体，能帮助你读懂别人写的代码，也能让你写出更简洁的代码。
 */

void demoStructVariations() {
  // 例子 1：结构体初始化的多种等价写法
  // 写法 A：按成员顺序初始化（最常用）
  struct Student s1 = {1001, "张三", 89.5f};

  // 写法 B：C99 指定初始化器（可读性更好，不用记顺序）
  struct Student s2 = {
    .id = 1002,
    .name = "李四",
    .score = 92.0f
  };

  // 写法 C：先定义再逐个赋值
  struct Student s3;
  s3.id = 1003;
  strcpy(s3.name, "王五");
  s3.score = 78.0f;

  // 例子 2：typedef 的两种常见形式
  // 形式 A：先定义 struct 标签，再用 typedef 起别名
  struct Point {
    int x;
    int y;
  };
  typedef struct Point Point;  // 以后可以直接写 Point

  // 形式 B：定义和 typedef 一次性完成
  typedef struct {
    int x;
    int y;
  } Point2;

  Point p1 = {10, 20};
  Point2 p2 = {30, 40};

  // 例子 3：点号 . 和箭头 -> 的转换
  // 当持有结构体变量时用 .
  SensorData localData = {25.0f, 50.0f, 0};
  Serial.print("用 . 访问：");
  Serial.println(localData.temperature);

  // 当持有结构体指针时用 ->
  SensorData* ptr = &localData;
  Serial.print("用 -> 访问：");
  Serial.println(ptr->temperature);
  // ptr->temperature 完全等价于 (*ptr).temperature

  // 例子 4：把结构体传入函数的两种方式
  printSensorData(localData);   // 传值：函数内修改不影响原变量
  modifySensorData(ptr);        // 传指针：函数内修改会影响原变量
}

void modifySensorData(SensorData* pData) {
  if (pData != NULL) {
    pData->temperature = 99.0f;
  }
}

/*
 * ===== 常见陷阱 =====
 *
 * 下面这些代码看起来像是正确的，但要么编译不过，要么运行结果不对。
 * 每个错误示例旁边都给出了正确写法，注意对比。
 */

void demoStructTraps() {
  // 陷阱 1：typedef 后还写 struct
  // 错误写法：
  // typedef struct { int x; int y; } Point;
  // struct Point p;  // 编译错误！typedef 已经把 Point 当成完整类型名了
  // 正确写法：
  typedef struct {
    int x;
    int y;
  } Point;
  Point p = {1, 2};  // 直接用别名，不要再加 struct
  Serial.print("正确 typedef 用法：");
  Serial.println(p.x);

  // 陷阱 2：用 . 访问指针指向的结构体成员
  // 错误写法：
  // Point* pPtr = &p;
  // pPtr.x = 10;  // 编译错误！pPtr 是指针，不能直接用 .
  // 正确写法：
  Point* pPtr = &p;
  pPtr->x = 10;        // 正确
  (*pPtr).y = 20;      // 也正确，与上一行等价
  Serial.print("指针访问正确：");
  Serial.println(pPtr->x);

  // 陷阱 3：结构体数组初始化漏写大括号
  // 错误写法：
  // struct Student arr[2] = {1001, "A", 80, 1002, "B", 90};
  // 这样写虽然可能编译通过，但可读性差，容易把数据填错位
  // 正确写法：
  struct Student arr[2] = {
    {1001, "A", 80.0f},
    {1002, "B", 90.0f}
  };
  Serial.print("结构体数组初始化正确：");
  Serial.println(arr[0].id);

  // 陷阱 4：试图直接给结构体里的 char 数组赋值字符串
  // 错误写法：
  // struct Student s;
  // s.name = "张三";  // 编译错误！数组名是地址常量，不能直接赋值
  // 正确写法：
  struct Student s;
  strcpy(s.name, "张三");  // 用字符串拷贝函数
  Serial.print("字符串成员正确赋值：");
  Serial.println(s.name);
}

/*
 * 练习：
 * 1. 定义一个结构体表示一本书，包含书名、作者、价格，并打印出来。
 * 2. 创建一个结构体数组存储 5 个学生的信息，找出成绩最高的学生。
 * 3. 写一个函数，接收结构体指针，修改结构体中的温度值。
 * 4. 用结构体封装 LED 的配置信息（引脚号、亮度、闪烁间隔），并写一个函数根据配置控制 LED。
 */
