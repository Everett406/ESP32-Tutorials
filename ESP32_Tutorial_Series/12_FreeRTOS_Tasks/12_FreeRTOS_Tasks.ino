/*
 * ESP32 教程系列 12：FreeRTOS 多任务 —— 同时做好几件事
 * 
 * 学习目标：
 * 1. 理解什么是"实时操作系统"和"多任务"
 * 2. 理解 ESP32 的双核 CPU 意味着什么
 * 3. 学会用 xTaskCreatePinnedToCore() 创建新任务
 * 4. 理解任务函数、堆栈、优先级、核心编号这些概念
 * 5. 学会用 vTaskDelay() 代替 delay()，让出 CPU 给其他任务
 * 
 * 前置知识：
 * - 已经会用 digitalWrite() 控制 LED
 * - 已经会用 Serial.println() 输出调试信息
 * 
 * 硬件连接：
 * - ESP32 GPIO2  →  LED1 正极（长脚）
 * - LED1 负极（短脚） →  330Ω 电阻  →  GND
 * - ESP32 GPIO4  →  LED2 正极（长脚）
 * - LED2 负极（短脚） →  330Ω 电阻  →  GND
 * 
 * 为什么需要多任务？
 * 在普通的 Arduino 程序里，所有代码都写在 loop() 里，按顺序一行一行执行。
 * 如果 loop() 中间写了一个 delay(5000)，那么在这 5 秒内，
 * 程序什么也干不了：不能读取传感器，不能响应按键，不能刷新屏幕。
 * 这叫做"阻塞"。
 * 
 * FreeRTOS 是一个"实时操作系统"（Real-Time Operating System，RTOS），
 * 它允许我们把程序拆分成多个"任务"（Task），每个任务看起来都在同时运行。
 * 操作系统会快速在各个任务之间切换，让我们感觉它们是并行的。
 * ESP32 还有两颗 CPU 核心，真正可以同时执行两个任务。
 */

#define LED1_PIN 2    // 第一个 LED，接 GPIO2
#define LED2_PIN 4    // 第二个 LED，接 GPIO4

// 任务 1：让 LED1 以 1 秒为周期慢速闪烁
// 
// 什么是任务函数？
// FreeRTOS 中的任务就是一个"永远不会返回"的函数。
// 它的返回值必须是 void，参数必须是 void* 类型（一个通用指针）。
// 函数内部通常是一个 while(true) 死循环，不断做自己的事情。
void taskBlinkLED1(void* parameter) {
  // parameter 是创建任务时传进来的参数，本示例没有用到。
  // (void)parameter 这行代码的作用是告诉编译器"我故意不用这个参数"，
  // 避免编译器报"未使用参数"的警告。
  (void)parameter;

  // 每个任务都要自己初始化它用到的引脚。
  // 为什么？因为 FreeRTOS 的任务是独立的执行流，
  // 不能假设 setup() 已经帮它做好了一切。
  pinMode(LED1_PIN, OUTPUT);

  // 任务的主体循环。和普通 loop() 不同，这个循环只属于本任务。
  while (true) {
    digitalWrite(LED1_PIN, HIGH);

    // vTaskDelay() 是 FreeRTOS 的任务延时函数。
    // 它和 Arduino 的 delay() 最大的区别是：
    // delay() 会让整个 CPU 空转等待，期间其他代码也跑不了；
    // vTaskDelay() 会让当前任务"挂起"，把 CPU 让给其他任务去执行。
    // 参数单位是"系统节拍"（tick），不是毫秒。
    // portTICK_PERIOD_MS 表示每个 tick 多少毫秒，
    // 所以 500 / portTICK_PERIOD_MS 就是把 500ms 换算成 tick 数。
    vTaskDelay(500 / portTICK_PERIOD_MS);

    digitalWrite(LED1_PIN, LOW);
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

// 任务 2：让 LED2 以 200ms 为周期快速闪烁
void taskBlinkLED2(void* parameter) {
  (void)parameter;

  pinMode(LED2_PIN, OUTPUT);

  while (true) {
    digitalWrite(LED2_PIN, HIGH);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    digitalWrite(LED2_PIN, LOW);
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

// 任务 3：后台打印系统运行状态
void taskPrintStatus(void* parameter) {
  (void)parameter;

  while (true) {
    Serial.print("当前任务运行在核：");

    // xPortGetCoreID() 返回当前任务正在哪个 CPU 核心上运行。
    // ESP32 有两个核心：Core 0 和 Core 1。
    // 返回值 0 表示 Core 0，1 表示 Core 1。
    Serial.print(xPortGetCoreID());

    Serial.print(" | 剩余空闲内存：");

    // ESP.getFreeHeap() 返回当前可用的动态内存（堆）大小，单位字节。
    // 创建任务、使用 Wi-Fi/蓝牙都会消耗内存，观察这个值可以帮助我们判断
    // 程序是否因为内存不足而崩溃。
    Serial.print(ESP.getFreeHeap());
    Serial.println(" bytes");

    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("================================");
  Serial.println("ESP32 教程 12：FreeRTOS 多任务");
  Serial.println("================================");

  // xTaskCreatePinnedToCore() 用来创建一个任务，并指定它运行在哪个核心。
  // 
  // 参数依次是：
  // 1. taskFunction    ：任务函数名（就是上面写的那些函数）
  // 2. "TaskName"      ：任务的名字，只在调试时显示，方便辨认
  // 3. stackSize       ：给这个任务分配的堆栈大小，单位是字节
  // 4. parameter       ：传给任务函数的参数，没有就写 NULL
  // 5. priority        ：任务优先级，数字越大优先级越高，0 是最低
  // 6. taskHandle      ：任务句柄，如果需要以后删除/挂起这个任务就传变量地址，
  //                     不需要就写 NULL
  // 7. coreID          ：指定运行在哪个核心，0 或 1
  //
  // 什么是堆栈（Stack）？
  // 堆栈是任务运行时的"临时工作空间"，用来保存局部变量、函数返回地址等。
  // 堆栈太小，任务可能会崩溃；堆栈太大，又浪费内存。
  // 简单的闪烁任务 2048 字节通常够用，复杂任务可以设 4096 或更大。
  //
  // 什么是优先级？
  // FreeRTOS 是抢占式调度器，优先级高的任务会优先执行。
  // 如果两个任务都ready（准备好运行），优先级高的先得到 CPU。
  // 相同优先级的任务会轮流执行（时间片轮转）。
  // 注意：优先级太高且一直不释放 CPU，会导致低优先级任务"饿死"。
  //
  // 核心怎么选？
  // - Core 0：通常被 Arduino 框架用来运行系统任务、Wi-Fi/蓝牙协议栈。
  //           我们自己的任务也可以放在这里，但要避免和系统任务抢资源。
  // - Core 1：通常运行 Arduino 的 setup() 和 loop()。
  //           把用户任务放这里一般比较安全。

  // 创建 LED1 闪烁任务，指定运行在 Core 0
  xTaskCreatePinnedToCore(
    taskBlinkLED1,      // 任务函数
    "Blink LED1",       // 任务名
    2048,               // 堆栈大小 2048 字节
    NULL,               // 没有传参数
    1,                  // 优先级 1
    NULL,               // 不需要句柄
    0                   // 运行在 Core 0
  );

  // 创建 LED2 闪烁任务，指定运行在 Core 1
  xTaskCreatePinnedToCore(
    taskBlinkLED2,
    "Blink LED2",
    2048,
    NULL,
    1,
    NULL,
    1                   // 运行在 Core 1
  );

  // 创建状态打印任务，运行在 Core 1
  // 这个任务用到串口，堆栈设大一点（4096 字节），避免打印长字符串时出错
  xTaskCreatePinnedToCore(
    taskPrintStatus,
    "Print Status",
    4096,
    NULL,
    1,
    NULL,
    1
  );

  Serial.println("3 个 FreeRTOS 任务已创建");
  Serial.println("观察两个 LED 是否以不同频率同时闪烁");
}

void loop() {
  // loop() 本身也是一个任务，默认运行在 Core 1。
  // 这里我们简单打印 loop() 运行在哪个核心，然后让出 CPU。
  //
  // 注意：loop() 里不要再写 Arduino 的 delay()，
  // 否则整个 Core 1 都会被阻塞，包括运行在上面的 LED2 和状态打印任务。
  Serial.print("loop() 运行在核：");
  Serial.println(xPortGetCoreID());

  // 用 vTaskDelay 让出 CPU，让其他任务有机会运行
  vTaskDelay(3000 / portTICK_PERIOD_MS);
}

/*
 * 排查提示：
 * 1. 如果上传后板子不断重启，打印 "Guru Meditation Error"：
 *    - 很可能是某个任务的堆栈设得太小，尝试把 2048 改成 4096 或 8192。
 *    - 也可能是任务函数里访问了串口但串口还没初始化好。
 *
 * 2. 如果只有一个 LED 闪：
 *    - 检查 LED 接线，确认 GPIO4 上的 LED 能单独点亮。
 *    - 检查是否误把两个任务都指定到了同一个核心且其中一个阻塞了 CPU。
 *
 * 3. 如果串口输出乱码或中间断了：
 *    - 确保波特率设置为 115200。
 *    - 多个任务同时调用 Serial.println() 虽然一般不会冲突，
 *      但打印很长的字符串时最好加互斥锁保护。
 *
 * 4. 关于 Core 0 和 Core 1：
 *    - ESP32 的 Wi-Fi、蓝牙、TCP/IP 协议栈默认在 Core 0 运行。
 *    - 如果你的任务大量使用网络功能，放在 Core 1 通常更稳定。
 */

/*
 * 扩展练习：
 * 1. 用队列（Queue）在不同任务之间安全传递数据，
 *    例如一个任务读温度，另一个任务收到后打印。
 * 2. 用信号量（Semaphore）保护共享资源，
 *    例如两个任务都要控制同一个 LED 时，避免冲突。
 * 3. 写一个任务读取传感器，另一个任务通过 HTTP 把数据上传到服务器。
 */
