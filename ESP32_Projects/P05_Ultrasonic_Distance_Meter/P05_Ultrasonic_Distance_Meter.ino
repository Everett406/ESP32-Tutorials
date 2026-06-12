/*
 * 综合项目 P05：超声波测距仪
 *
 * 学习目标：
 * 1. 理解 HC-SR04 超声波测距模块的工作原理
 * 2. 学会用 pulseIn() 测量超声波往返时间并换算成距离
 * 3. 学会使用 SSD1306 OLED 显示屏显示文字和数字
 * 4. 掌握根据距离变化控制蜂鸣器频率的方法
 * 5. 学会用 millis() 实现“非阻塞式”蜂鸣器和屏幕刷新
 *
 * 前置知识：
 * - 已完成教程 01、03，了解 GPIO、PWM、延时等基本概念
 * - 了解 I2C 基础（本项目在注释中会解释）
 *
 * 硬件连接：
 * - ESP32 GPIO5  →  HC-SR04 Trig（触发）
 * - ESP32 GPIO18 →  HC-SR04 Echo（回响）
 * - ESP32 5V     →  HC-SR04 VCC（供电）
 * - ESP32 GND    →  HC-SR04 GND
 * - ESP32 GPIO21 →  OLED SDA（I2C 数据线）
 * - ESP32 GPIO22 →  OLED SCL（I2C 时钟线）
 * - ESP32 3.3V   →  OLED VCC
 * - ESP32 GND    →  OLED GND
 * - ESP32 GPIO19 →  有源蜂鸣器正极
 * - ESP32 GND    →  有源蜂鸣器负极
 *
 * 如果你的 ESP32 开发板没有 5V 引脚，可以把 HC-SR04 的 VCC 接到 3.3V，
 * 但测距稳定性可能会下降，建议优先使用 5V。
 *
 * 关键概念解释：
 * - HC-SR04：一种超声波测距模块。它先发出 40kHz 的超声波脉冲，
 *   然后等待回声。通过测量从发射到接收的时间，再利用声速计算出距离。
 *   公式：距离（cm） = 时间（μs） × 0.034 / 2。
 *   除以 2 是因为声波走了“去”和“回”两段路。
 * - I2C（Inter-Integrated Circuit）：一种常用的两线通信协议，只需要
 *   两根线 SDA（数据线）和 SCL（时钟线），就可以让主设备（ESP32）和
 *   多个从设备（如 OLED）通信。每个从设备有一个唯一的 7 位地址，
 *   常见 OLED 地址是 0x3C 或 0x3D。
 * - SSD1306：一种常见的 OLED 驱动芯片，分辨率通常是 128×64 像素。
 *   它通过 I2C 与 ESP32 通信，可以显示文字、数字和简单图形。
 * - pulseIn(pin, value, timeout)：等待指定引脚出现指定电平，并返回
 *   该电平持续的时间，单位是微秒（μs）。timeout 是最大等待时间，
 *   超过这个时间就返回 0，避免程序一直卡在这里。
 * - 有源蜂鸣器：内部自带振荡电路，只要给正极通上高电平（3.3V），
 *   它就会持续发出声音。本项目中通过控制“响”与“不响”的时间间隔，
 *   来表现距离远近。
 */

// Wire.h 是 Arduino 的 I2C 库，负责 SDA/SCL 通信。
// SSD1306 OLED 通过 I2C 连接，所以必须先引入它。
#include <Wire.h>

// Adafruit_GFX 是基础图形库，Adafruit_SSD1306 是专门驱动 SSD1306 的库。
// 需要在 Arduino IDE 库管理器中安装：
// 1. "Adafruit SSD1306"
// 2. "Adafruit GFX Library"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// HC-SR04 触发引脚。我们向这个引脚发送一个 10μs 的高电平脉冲，模块就开始测距。
#define TRIG_PIN 5

// HC-SR04 回响引脚。模块收到回声后，会在这个引脚上输出一个高电平脉冲，
// 脉冲宽度与距离成正比。
#define ECHO_PIN 18

// 有源蜂鸣器引脚。距离越近，蜂鸣器响得越频繁。
#define BUZZER_PIN 19

// OLED 的 I2C 引脚。ESP32 默认 I2C 引脚通常是 GPIO21（SDA）和 GPIO22（SCL）。
// 如果你的开发板不同，请修改这两个宏。
#define SDA_PIN 21
#define SCL_PIN 22

// OLED 分辨率。SSD1306 常见尺寸为 128×64。
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// OLED 复位引脚。很多 I2C 模块没有复位引脚，所以设为 -1，表示不用。
#define OLED_RESET -1

// OLED 的 I2C 地址。大多数 128×64 的 SSD1306 模块是 0x3C，
// 如果屏幕不亮，可以尝试改成 0x3D。
#define SCREEN_ADDRESS 0x3C

// 测距间隔。200 毫秒刷新一次，既不会太慢，也不会让串口刷屏。
#define MEASURE_INTERVAL 200

// 蜂鸣器每次“嘀”的持续时长，单位毫秒。
#define BEEP_DURATION 50

// 创建 SSD1306 显示对象。
// 参数：宽度、高度、I2C 总线指针、复位引脚。
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// 记录上一次测距的时间，用于非阻塞定时。
unsigned long lastMeasureTime = 0;

// 蜂鸣器相关的时间变量。
unsigned long lastBeepTime = 0;

// 蜂鸣器当前是否正在“嘀”的状态。
bool beeping = false;

// 最近一次测量到的距离，单位 cm。无效时记为 -1。
// 把它定义为全局变量，measure 和 buzzer 都能访问。
float gCurrentDistance = -1.0;

// 测量距离的函数。
// 返回值是 float 类型的距离，单位厘米（cm）。
// 如果超时没有收到回声，返回 -1 表示“超出范围或测量失败”。
float measureDistance() {
  // 先把 Trig 拉低 2 微秒，确保模块处于稳定状态。
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  // 向 Trig 发送一个 10 微秒的高电平脉冲，命令模块开始发射超声波。
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // 等待 Echo 引脚变高，并测量高电平持续的时间，单位微秒。
  // 第三个参数 30000 表示最多等 30000 微秒（30 毫秒）。
  // 声速 340m/s 时，30 毫秒对应的单程距离约 5 米，足够覆盖 HC-SR04 的量程。
  unsigned long duration = pulseIn(ECHO_PIN, HIGH, 30000);

  // 如果 duration 为 0，说明在 30 毫秒内没有收到回声，可能是距离太远或没有反射面。
  if (duration == 0) {
    return -1.0;
  }

  // 声速在空气中约为 0.034 cm/μs。
  // 因为声波走了往返两段路，所以结果要除以 2。
  float distance = duration * 0.034 / 2.0;

  // HC-SR04 的可靠测量范围一般是 2cm ~ 400cm，
  // 超出这个范围的结果可信度低，统一返回 -1。
  if (distance < 2.0 || distance > 400.0) {
    return -1.0;
  }

  return distance;
}

// 根据距离更新蜂鸣器状态。
// 距离越近，蜂鸣器“嘀”的间隔越短，听起来就越急促。
// 本函数使用 millis() 非阻塞实现，不会卡住主循环。
void updateBuzzer(float distance) {
  // 如果距离无效或大于 100cm，就不响。
  if (distance < 0 || distance > 100.0) {
    digitalWrite(BUZZER_PIN, LOW);
    beeping = false;
    return;
  }

  // 把距离 0~100cm 映射到蜂鸣器间隔 100~2000 毫秒。
  // 距离为 0cm 时间隔 100ms（很急促），距离为 100cm 时间隔 2000ms（很稀疏）。
  int beepInterval = map((int)distance, 0, 100, 100, 2000);

  unsigned long now = millis();

  if (!beeping) {
    // 当前没在响，且距离上一次“嘀”已经过去了 beepInterval，就再次发出声音。
    if (now - lastBeepTime >= (unsigned long)beepInterval) {
      digitalWrite(BUZZER_PIN, HIGH);
      beeping = true;
      lastBeepTime = now;
    }
  } else {
    // 当前正在响，持续时间达到 BEEP_DURATION 后就关闭，等待下一次。
    if (now - lastBeepTime >= BEEP_DURATION) {
      digitalWrite(BUZZER_PIN, LOW);
      beeping = false;
      lastBeepTime = now;
    }
  }
}

// 在 OLED 上显示距离和提示信息。
void updateDisplay(float distance) {
  // clearDisplay() 清空显存。注意这只是清除内存中的画面，
  // 必须再调用 display.display() 才会真正显示到屏幕上。
  display.clearDisplay();

  // 设置文字大小。1 表示最小字号，适合显示多行内容。
  display.setTextSize(1);

  // 设置文字颜色。SSD1306_WHITE 表示白色像素点亮，黑色像素熄灭。
  display.setTextColor(SSD1306_WHITE);

  // 设置光标位置，单位是像素。坐标原点是屏幕左上角。
  display.setCursor(0, 0);
  display.println(F("超声波测距仪"));
  display.println(F("================"));

  if (distance < 0) {
    display.setCursor(0, 24);
    display.setTextSize(2);
    display.println(F("超出范围"));
  } else {
    display.setCursor(0, 20);
    display.setTextSize(2);
    display.print(distance, 1);  // 保留 1 位小数
    display.println(F(" cm"));

    display.setTextSize(1);
    display.setCursor(0, 44);
    if (distance < 10.0) {
      display.println(F("状态：过近！"));
    } else if (distance < 30.0) {
      display.println(F("状态：较近"));
    } else if (distance < 60.0) {
      display.println(F("状态：适中"));
    } else {
      display.println(F("状态：较远"));
    }
  }

  // 把显存内容一次性刷新到 OLED 屏幕上。
  display.display();
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("==============================");
  Serial.println("P05：超声波测距仪");
  Serial.println("==============================");

  // 设置 Trig 为输出，Echo 为输入，蜂鸣器为输出。
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  // 初始化 I2C 总线，并指定 SDA、SCL 引脚。
  // 必须在 display.begin() 之前调用，否则 OLED 可能无法识别引脚。
  Wire.begin(SDA_PIN, SCL_PIN);

  // 初始化 OLED。SSD1306_SWITCHCAPVCC 表示使用内部升压电路从 3.3V 驱动屏幕。
  // 如果初始化失败，就打印错误信息并停在这里。
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("OLED 初始化失败，请检查接线和 I2C 地址"));
    while (true) {
      delay(100);  // 卡住，等待用户检查硬件
    }
  }

  // 开机显示一次欢迎信息。
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 28);
  display.println(F("  超声波测距仪\n    启动中..."));
  display.display();
  delay(1000);

  Serial.println("系统初始化完成");
}

void loop() {
  unsigned long now = millis();

  // 每隔 MEASURE_INTERVAL 测量一次距离。
  if (now - lastMeasureTime >= MEASURE_INTERVAL) {
    lastMeasureTime = now;

    gCurrentDistance = measureDistance();

    // 把结果打印到串口。
    if (gCurrentDistance < 0) {
      Serial.println("距离：超出范围 / 无回波");
    } else {
      Serial.print("距离：");
      Serial.print(gCurrentDistance, 1);
      Serial.println(" cm");
    }

    // 更新 OLED 显示。
    updateDisplay(gCurrentDistance);
  }

  // 蜂鸣器需要更频繁地检查，所以每次 loop 都执行。
  updateBuzzer(gCurrentDistance);
}

/*
 * 扩展挑战：
 * 1. 把蜂鸣器改成 PWM 驱动，距离越近音调越高，
 *    或把蜂鸣器换成无源蜂鸣器，用 ledc 输出不同频率的“滴滴”声。
 * 2. 在 OLED 上画一条随距离变化的进度条，直观显示远近。
 * 3. 加入距离记录功能：保存最近 10 次测量值，计算并显示平均距离，
 *    减少 HC-SR04 的偶然跳变。
 * 4. 当物体进入 10cm 以内时，让蜂鸣器长鸣而不是间断响，用作倒车雷达效果。
 */
