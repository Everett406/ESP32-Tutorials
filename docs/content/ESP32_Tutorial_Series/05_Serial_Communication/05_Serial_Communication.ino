/*
 * ESP32 教程系列 05：串口通信 - 与电脑对话
 *
 * 学习目标：
 * 1. 理解串口（UART）通信的基本概念
 * 2. 学会使用 Serial.print() / Serial.println() 发送数据到电脑
 * 3. 学会使用 Serial.available() 和 Serial.readStringUntil() 接收电脑发来的数据
 * 4. 理解串口缓冲区、波特率、字符串处理等概念
 * 5. 学会用 millis() 实现非阻塞式定时任务
 *
 * 前置知识：
 * - 已完成教程 01~04，理解 GPIO、PWM、ADC 等基础概念
 *
 * 关键概念解释：
 *
 * 【串口 UART 是什么？】
 * UART 是 Universal Asynchronous Receiver/Transmitter（通用异步收发传输器）的缩写，
 * 简称"串口"。
 * 它是一种异步串行通信协议：
 * - "异步"：通信双方没有共享时钟线，通过约定的波特率来同步
 * - "串行"：数据一位一位地按顺序传输
 * ESP32 通过 USB 转串口芯片和电脑连接，Arduino IDE 的"串口监视器"就是查看串口数据的工具。
 *
 * 【波特率 Baud Rate】
 * 波特率是串口通信的速度，表示每秒传输多少位（bit）。
 * 通信双方的波特率必须一致，否则看到乱码。
 * 常用波特率：9600、115200 等。本程序使用 115200，速度较快。
 * 注意：上传代码时的串口波特率由上传协议决定，和这里设置的串口通信波特率无关。
 *
 * 【串口缓冲区】
 * ESP32 内部有一块小内存，叫"串口接收缓冲区"（RX Buffer）。
 * 当电脑发送数据时，数据先存在缓冲区里，等程序调用 Serial.readStringUntil() 等函数读取。
 * Serial.available() 返回缓冲区中还未读取的字节数。
 * 如果缓冲区满了，新数据会覆盖旧数据，所以要及时读取。
 *
 * 【字符串处理函数】
 * - trim()：去掉字符串开头和结尾的空白字符（空格、换行、回车等）
 * - toLowerCase()：把字符串中所有字母转成小写
 * - length()：返回字符串长度（字符个数）
 * 这些函数让我们可以灵活处理用户输入的命令。
 *
 * 【static 关键字】
 * static 用来声明静态变量。
 * 普通局部变量在函数每次执行时都会重新创建，而 static 局部变量只会创建一次，
 * 并且会记住上一次的值。
 * 这里用 static 声明 lastHeartbeat，让定时器状态在多次 loop() 调用之间保持。
 *
 * 【millis() 函数】
 * millis() 返回程序开始运行以来的毫秒数。
 * 它不会阻塞程序，所以适合用来做定时任务。
 * 我们用 millis() - lastHeartbeat > 2000 判断是否已经过了 2 秒，
 * 这样就不会像 delay() 那样让程序停下来等待。
 *
 * 硬件连接：
 * - 用 USB 线把 ESP32 连接到电脑即可
 * - LED 正极（长脚） →  ESP32 GPIO2
 * - LED 负极（短脚） →  330Ω 电阻  →  ESP32 GND
 *
 * 本示例功能：
 * - ESP32 每隔 2 秒发送一次心跳信息
 * - 用户在串口监视器输入命令，ESP32 执行对应操作
 * - 支持命令：
 *   * on（开灯）
 *   * off（关灯）
 *   * blink（闪烁 5 次）
 *   * info（显示系统信息）
 *
 * 常见问题与排查：
 * 1. 串口监视器看到乱码：
 *    - 检查串口监视器右下角波特率是否为 115200
 *    - 检查 USB 线是否支持数据传输
 *    - 尝试按一下开发板复位键
 * 2. 发送命令没有反应：
 *    - 检查发送时是否按了回车键
 *    - 检查串口监视器是否设置为"换行符"（Newline）或"回车换行符"（CRLF）模式
 *    - 本程序用 readStringUntil('\n')，需要收到换行符才结束读取
 * 3. 命令大小写问题：
 *    - 本程序用 toLowerCase() 把命令转小写，所以 ON、On、on 都可以识别
 * 4. 心跳信息不显示：
 *    - 检查 loop() 是否被某个长时间 delay() 阻塞
 *    - 本程序使用 millis() 实现心跳，不会阻塞
 * 5. info 命令显示异常：
 *    - 某些 ESP32 开发板型号可能不支持 ESP.getChipModel()，可以尝试注释掉相关代码
 */

// 定义 LED 引脚
#define LED_PIN 2

// 记录已执行命令的次数
// int 是整数类型，初始值为 0
// 每次执行完一条命令后累加，用于 info 命令显示统计信息
int commandCount = 0;

void setup() {
  // 初始化串口，波特率 115200
  // Serial.begin() 设置 ESP32 和电脑通信的速度
  // 串口监视器也要设置成 115200，否则看到乱码
  Serial.begin(115200);

  // 等待串口准备好
  // 有些开发板上电后 USB 转串口芯片需要一点时间建立连接
  delay(1000);

  // 打印欢迎信息和命令说明
  // 这样用户一打开串口监视器就知道可以输入哪些命令
  Serial.println("================================");
  Serial.println("ESP32 教程 05：串口通信");
  Serial.println("================================");
  Serial.println("可用命令：");
  Serial.println("  on    - 点亮 LED");
  Serial.println("  off   - 熄灭 LED");
  Serial.println("  blink - LED 闪烁 5 次");
  Serial.println("  info  - 显示系统信息");

  // 设置 LED 引脚为输出模式
  pinMode(LED_PIN, OUTPUT);

  // 初始化时把 LED 熄灭，让状态确定
  digitalWrite(LED_PIN, LOW);
}

void loop() {
  // Serial.available() 返回串口接收缓冲区中等待读取的字节数
  // 如果返回值大于 0，表示电脑发来了数据
  // 等于 0 表示暂时没有新数据
  if (Serial.available() > 0) {
    // Serial.readStringUntil(terminator) 从缓冲区读取字符串，
    // 直到遇到指定的终止字符（这里是换行符 '\n'）为止。
    // 使用换行符作为结束标志，是因为串口监视器发送时会自动在末尾加换行。
    String command = Serial.readStringUntil('\n');

    // trim() 去掉字符串开头和结尾的空白字符
    // 比如用户输入 "on "（后面带空格），trim 后变成 "on"
    // 又比如 readStringUntil 可能读到 "\r\n"，trim 后去掉了回车换行
    command.trim();

    // toLowerCase() 把命令中所有大写字母转成小写
    // 这样用户输入 ON、On、on 都会被识别为同一个命令
    // 注意：toLowerCase() 会修改原字符串
    command.toLowerCase();

    // 打印收到的命令，方便用户确认输入已被识别
    Serial.print("收到命令：");
    Serial.println(command);

    // 根据命令执行不同操作
    // if-else if-else 结构让程序按顺序判断命令
    if (command == "on") {
      // "on" 命令：点亮 LED
      // HIGH 表示输出高电平（约 3.3V），LED 点亮
      digitalWrite(LED_PIN, HIGH);
      Serial.println("LED 已点亮");
    }
    else if (command == "off") {
      // "off" 命令：熄灭 LED
      // LOW 表示输出低电平（0V），LED 熄灭
      digitalWrite(LED_PIN, LOW);
      Serial.println("LED 已熄灭");
    }
    else if (command == "blink") {
      // "blink" 命令：让 LED 闪烁 5 次
      Serial.println("LED 开始闪烁 5 次...");

      // for 循环重复执行 5 次
      // 变量 i 从 0 开始，每次加 1，直到 i < 5 不再成立
      for (int i = 0; i < 5; i++) {
        // 点亮 LED
        digitalWrite(LED_PIN, HIGH);

        // 延时 200 毫秒
        delay(200);

        // 熄灭 LED
        digitalWrite(LED_PIN, LOW);

        // 再延时 200 毫秒
        delay(200);
      }

      Serial.println("闪烁完成");
    }
    else if (command == "info") {
      // "info" 命令：显示系统信息
      Serial.println("--- 系统信息 ---");

      // 打印已执行命令数
      Serial.print("已执行命令数：");
      Serial.println(commandCount);

      // ESP.getChipModel() 返回芯片型号字符串
      Serial.print("芯片型号：");
      Serial.println(ESP.getChipModel());

      // ESP.getCpuFreqMHz() 返回 CPU 工作频率，单位 MHz
      Serial.print("CPU 频率：");
      Serial.print(ESP.getCpuFreqMHz());
      Serial.println(" MHz");

      // ESP.getFlashChipSize() 返回 Flash 大小，单位字节
      // 除以 1024 * 1024 转换成 MB
      Serial.print("闪存大小：");
      Serial.print(ESP.getFlashChipSize() / 1024 / 1024);
      Serial.println(" MB");
    }
    else if (command.length() > 0) {
      // 如果命令不是空的，但又不是以上任何一种，说明是未知命令
      // length() 返回字符串长度，大于 0 表示用户确实输入了内容
      // 如果是空字符串（比如只按了回车），就不提示错误
      Serial.println("未知命令，请检查输入");
    }

    // 每处理完一条命令，命令计数加 1
    commandCount++;
  }

  // 每隔 2 秒发送一次心跳信息
  // 这里使用 millis() 实现非阻塞定时，不会耽误串口命令的响应
  // static 让 lastHeartbeat 变量在多次 loop() 之间保持上次的值
  static unsigned long lastHeartbeat = 0;

  // millis() 返回从上电开始经过的毫秒数
  // 如果当前时间减去上次心跳时间超过 2000 毫秒（2 秒），就发送一次心跳
  if (millis() - lastHeartbeat > 2000) {
    // 更新上次心跳时间为当前时间
    lastHeartbeat = millis();

    // 发送心跳信息，显示程序已运行时间
    Serial.print("心跳：运行中，已运行 ");
    Serial.print(millis() / 1000);
    Serial.println(" 秒");
  }
}

/*
 * 扩展练习：
 * 1. 增加 "fade" 命令，让 LED 做呼吸灯效果（结合教程 03 的 PWM 知识）。
 * 2. 增加 "duty 128" 命令，用空格分割命令和参数，设置 PWM 占空比。
 *    提示：可以用 command.indexOf(' ') 找到空格位置，再分别取出命令和数字。
 * 3. 用 Serial.parseInt() 读取数字命令，比如用户输入数字 0~255 直接设置 LED 亮度。
 * 4. 增加 "status" 命令，返回当前 LED 状态和已运行时间。
 * 5. 用非阻塞方式重写 blink 命令，让闪烁期间仍然能响应新命令。
 */
