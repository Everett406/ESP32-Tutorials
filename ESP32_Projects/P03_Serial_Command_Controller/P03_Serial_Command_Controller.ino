/*
 * ESP32 综合项目 P03：串口命令控制器
 *
 * 项目目标：
 * 1. 把串口通信、GPIO 输出和 GPIO 输入结合起来
 * 2. 学会设计简单的串口命令协议
 * 3. 学会解析带参数的串口命令
 * 4. 理解如何用非阻塞方式实现 LED 呼吸/渐变效果
 * 5. 理解命令行交互的基本设计思路
 *
 * 前置知识：
 * - 已完成教程 01~05，理解 GPIO、PWM、ADC、串口通信等
 *
 * 关键概念解释：
 *
 * 【串口命令协议】
 * 协议就是通信双方约定的"暗号"。
 * 本项目约定：电脑通过串口发送一行文本命令，ESP32 收到后执行对应动作。
 * 例如：
 * - "on"：点亮 LED
 * - "off"：熄灭 LED
 * - "read"：读取按键状态
 * - "fade"：启动/停止呼吸灯效果
 * - "info"：显示系统信息
 *
 * 为什么用换行符作为命令结束？
 * 因为 Arduino 串口监视器发送时会自动在末尾加换行符（\n），
 * ESP32 可以用 readStringUntil('\n') 方便地读取完整一行。
 *
 * 【字符串解析】
 * 用户输入的命令可能有参数，例如：
 * - "duty 128"：设置 PWM 占空比为 128
 * 需要把一行文本拆分成"命令"和"参数"两部分。
 * 本示例用 String.indexOf(' ') 找到空格位置，再用 substring() 拆分。
 *
 * 【非阻塞式动画】
 * "fade" 命令让 LED 做呼吸效果。如果用 delay() 实现，
 * 呼吸期间程序无法响应新的串口命令。
 * 本项目用 millis() 实现非阻塞呼吸：
 * 每个 loop() 只更新一点点亮度，程序可以继续检查串口。
 *
 * 硬件连接：
 * - LED 正极（长脚） →  ESP32 GPIO2
 * - LED 负极（短脚） →  330Ω 电阻  →  ESP32 GND
 *
 * - 按键一端        →  ESP32 GPIO4
 * - 按键另一端      →  ESP32 GND
 *   （使用内部上拉）
 *
 * 为什么用 GPIO2 和 GPIO4？
 * GPIO2 是板载 LED 常用引脚，GPIO4 支持内部上拉且不会和启动特殊功能冲突，
 * 这两个引脚在前面教程中已经反复使用，容易理解。
 *
 * 常见问题与排查：
 * 1. 发送命令没有反应：
 *    - 检查串口监视器波特率是否为 115200
 *    - 检查串口监视器是否设置为"换行符"（Newline）或"回车换行符"（CRLF）模式
 *    - 检查发送时是否按了回车键
 * 2. "duty XXX" 命令报错：
 *    - 检查 XXX 是否在 0~255 范围内
 *    - 检查命令和数字之间是否有空格
 * 3. fade 命令启动后无法停止：
 *    - 再次发送 "fade" 命令可以停止
 *    - 如果还是停不下来，检查 fadeMode 的处理逻辑
 * 4. read 命令显示按键状态不对：
 *    - 检查按键是否接到了 GPIO4 和 GND
 *    - 内部上拉模式下，未按下为 HIGH，按下为 LOW
 * 5. 命令大小写问题：
 *    - 本程序用 toLowerCase() 把命令转小写，所以 ON、On、on 都可以识别
 */

// 定义引脚
#define LED_PIN     2    // LED 输出引脚
#define BUTTON_PIN  4    // 按键输入引脚

// PWM 参数
#define PWM_FREQ        5000    // PWM 频率 5kHz
#define PWM_RESOLUTION  8       // 8 bit 分辨率，占空比 0~255

// 命令计数器
// 每成功处理一条命令就加 1，用于 info 命令显示统计信息
int commandCount = 0;

// LED 当前状态
// true = 开，false = 关
bool ledState = false;

// 当前 PWM 亮度值，范围 0~255
int currentDuty = 0;

// 呼吸灯模式标志
// true = 正在呼吸，false = 停止呼吸
bool fadeMode = false;

// 呼吸灯相关变量
int fadeDirection = 1;              // 1 表示亮度增加，-1 表示亮度减小
unsigned long lastFadeStep = 0;     // 上次改变亮度的时间
#define FADE_INTERVAL_MS    20      // 呼吸灯每 20ms 改变一次亮度
#define FADE_STEP           1       // 每次改变 1 个单位

void setup() {
  // 初始化串口
  Serial.begin(115200);
  delay(1000);

  Serial.println("================================");
  Serial.println("ESP32 综合项目 P03：串口命令控制器");
  Serial.println("================================");
  Serial.println("可用命令：");
  Serial.println("  on              - 点亮 LED");
  Serial.println("  off             - 熄灭 LED");
  Serial.println("  read            - 读取按键状态");
  Serial.println("  fade            - 启动/停止呼吸灯");
  Serial.println("  info            - 显示系统信息");
  Serial.println("  duty <0~255>    - 设置 PWM 占空比");
  Serial.println("  示例：duty 128");

  // 设置 LED 引脚为输出
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // 初始化 PWM
  ledcAttach(LED_PIN, PWM_FREQ, PWM_RESOLUTION);

  // 设置按键引脚为输入，启用内部上拉
  pinMode(BUTTON_PIN, INPUT_PULLUP);
}

void loop() {
  // 检查是否有串口命令
  // Serial.available() 返回缓冲区中等待读取的字节数
  if (Serial.available() > 0) {
    // 读取一行命令，直到遇到换行符
    String command = Serial.readStringUntil('\n');

    // 去掉首尾空白字符（空格、回车、换行等）
    command.trim();

    // 把命令全部转成小写，实现大小写不敏感
    command.toLowerCase();

    // 如果命令为空（用户只按了回车），不处理
    if (command.length() == 0) {
      return;
    }

    // 显示收到的命令，方便调试
    Serial.print(">>> ");
    Serial.println(command);

    // 处理命令
    processCommand(command);

    // 命令计数加 1
    commandCount++;
  }

  // 如果处于呼吸灯模式，更新亮度
  if (fadeMode) {
    updateFade();
  }
}

// 自定义函数：解析并执行串口命令
// 参数 cmd：用户输入的命令字符串
void processCommand(String cmd) {
  // "on" 命令：点亮 LED
  if (cmd == "on") {
    // 退出呼吸模式，避免 on 和 fade 冲突
    fadeMode = false;
    ledState = true;
    currentDuty = 255;
    ledcWrite(LED_PIN, currentDuty);
    Serial.println("LED 已点亮");
  }

  // "off" 命令：熄灭 LED
  else if (cmd == "off") {
    fadeMode = false;
    ledState = false;
    currentDuty = 0;
    ledcWrite(LED_PIN, currentDuty);
    Serial.println("LED 已熄灭");
  }

  // "read" 命令：读取按键状态
  else if (cmd == "read") {
    // digitalRead() 读取 GPIO4 状态
    // 内部上拉：未按下为 HIGH，按下为 LOW
    int buttonState = digitalRead(BUTTON_PIN);

    Serial.print("按键状态：");
    if (buttonState == LOW) {
      Serial.println("按下");
    }
    else {
      Serial.println("松开");
    }
  }

  // "fade" 命令：切换呼吸灯模式
  else if (cmd == "fade") {
    fadeMode = !fadeMode;  // 取反，实现开关切换

    if (fadeMode) {
      Serial.println("呼吸灯已启动，再次发送 fade 可停止");
      // 让呼吸从当前亮度继续，不要突兀跳转
      lastFadeStep = millis();
    }
    else {
      Serial.println("呼吸灯已停止");
      // 停止后保持当前亮度
      ledState = (currentDuty > 0);
    }
  }

  // "info" 命令：显示系统信息
  else if (cmd == "info") {
    Serial.println("--- 系统信息 ---");
    Serial.print("已执行命令数：");
    Serial.println(commandCount);
    Serial.print("LED 状态：");
    Serial.println(ledState ? "开" : "关");
    Serial.print("当前 PWM 占空比：");
    Serial.println(currentDuty);
    Serial.print("呼吸模式：");
    Serial.println(fadeMode ? "运行中" : "已停止");
    Serial.print("芯片型号：");
    Serial.println(ESP.getChipModel());
    Serial.print("CPU 频率：");
    Serial.print(ESP.getCpuFreqMHz());
    Serial.println(" MHz");
    Serial.print("Flash 大小：");
    Serial.print(ESP.getFlashChipSize() / 1024 / 1024);
    Serial.println(" MB");
    Serial.print("已运行时间：");
    Serial.print(millis() / 1000);
    Serial.println(" 秒");
  }

  // "duty XXX" 命令：设置 PWM 占空比
  else if (cmd.startsWith("duty ")) {
    // 找到空格位置
    int spaceIndex = cmd.indexOf(' ');

    // 取出空格后面的数字部分
    String valueStr = cmd.substring(spaceIndex + 1);

    // 把字符串转换成整数
    int duty = valueStr.toInt();

    // 检查数值是否在合法范围内
    if (duty >= 0 && duty <= 255) {
      fadeMode = false;  // 退出呼吸模式
      currentDuty = duty;
      ledState = (duty > 0);
      ledcWrite(LED_PIN, currentDuty);
      Serial.print("PWM 占空比已设置为：");
      Serial.println(currentDuty);
    }
    else {
      Serial.println("错误：占空比必须在 0~255 之间");
    }
  }

  // 未知命令
  else {
    Serial.println("未知命令，可输入 info 查看帮助");
  }
}

// 自定义函数：非阻塞式更新呼吸灯
// 每个 loop() 只改变一点点亮度，不会阻塞串口命令处理
void updateFade() {
  // 检查是否到了该改变亮度的时间
  if (millis() - lastFadeStep >= FADE_INTERVAL_MS) {
    lastFadeStep = millis();

    // 改变亮度
    currentDuty += FADE_STEP * fadeDirection;

    // 到达边界时改变方向
    if (currentDuty >= 255) {
      currentDuty = 255;
      fadeDirection = -1;
    }
    else if (currentDuty <= 0) {
      currentDuty = 0;
      fadeDirection = 1;
    }

    // 输出 PWM
    ledcWrite(LED_PIN, currentDuty);

    // 打印当前亮度，方便观察
    // 注意：为了避免串口刷屏，这里只在边界时打印
    if (currentDuty == 0 || currentDuty == 255) {
      Serial.print("呼吸灯边界值：");
      Serial.println(currentDuty);
    }
  }
}

/*
 * 扩展挑战：
 * 1. 增加 "blink <次数> <间隔>" 命令，比如 "blink 5 300" 让 LED 闪烁 5 次，每次 300ms。
 * 2. 增加 "status" 命令，以 JSON 格式返回当前 LED 状态、亮度、按键状态、运行时间。
 * 3. 增加 "adc" 命令，读取某个 ADC 引脚（比如光敏电阻）并返回数值。
 * 4. 用状态机重构命令处理逻辑，让添加新命令更容易。
 * 5. 把串口命令改成蓝牙串口命令，实现无线控制（结合项目 P07 的思路）。
 */
