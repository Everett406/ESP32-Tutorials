/*
 * ESP32 综合项目 P02：按键调光灯
 *
 * 项目目标：
 * 1. 把数字输入（按键）和 PWM 输出结合起来，完成一个完整的交互式灯具
 * 2. 理解"短按"和"长按"的检测原理
 * 3. 学会用 millis() 实现非阻塞式按键计时
 * 4. 理解状态机思想：用变量记录当前状态，根据输入改变状态
 * 5. 学会消除按键抖动，避免一次按键被识别成多次
 *
 * 前置知识：
 * - 已完成教程 02（按键）、03（PWM），理解 INPUT_PULLUP、ledcAttach()、millis() 等
 *
 * 关键概念解释：
 *
 * 【短按 vs 长按】
 * 同一个按键可以通过按下的时间长短实现不同功能：
 * - 短按：按下后很快松开，用于"切换开关状态"
 * - 长按：按下超过一定时间还不松开，用于"连续调节亮度"
 *
 * 区分方法：
 * 1. 检测到按键按下时，记录当前时间 millis() 到 buttonDownTime
 * 2. 检测到按键松开时，计算松开时间 - 按下时间 = 按住时长
 * 3. 如果按住时长 < 长按阈值，视为短按；否则视为长按
 * 4. 在长按期间，每过一段时间就改变一次亮度，实现连续调光
 *
 * 【状态机 State Machine】
 * 状态机是一种编程思想：程序有有限个"状态"，在不同状态之间根据条件切换。
 * 本项目用 ledState 记录"开/关"，用 brightness 记录"亮度等级"，
 * 它们共同决定了当前灯具的状态。
 *
 * 为什么用状态机？
 * 因为它能清楚地描述"现在是什么状态"、"什么条件下会变成什么状态"，
 * 比一堆 if-else 嵌套更容易理解和维护。
 *
 * 【millis() 非阻塞计时】
 * millis() 返回程序开始运行以来的毫秒数，不会阻塞程序。
 * 和 delay() 不同，millis() 不会让程序停下来等待，
 * 所以 loop() 可以继续做其他事情（比如检查串口、更新显示等）。
 * 本项目用 millis() 来测量按键按下的时长，以及控制长按调光的速度。
 *
 * 【按键抖动 Debounce】
 * 机械按键按下和松开时，金属触点会快速颤动几毫秒到十几毫秒，
 * 导致 digitalRead() 在极短时间内多次变化。
 * 本项目用"状态变化 + 时间间隔"的方法消抖：
 * 只有上次状态变化后超过 DEBOUNCE_MS 毫秒，才认为新的状态变化是真实的。
 *
 * 硬件连接：
 * - LED 正极（长脚） →  ESP32 GPIO2
 * - LED 负极（短脚） →  330Ω 电阻  →  ESP32 GND
 *
 * - 按键一端        →  ESP32 GPIO4
 * - 按键另一端      →  ESP32 GND
 *   （使用内部上拉，不需要外部电阻）
 *
 * 为什么用 GPIO4 接按键？
 * GPIO4 支持内部上拉，且不是启动时的特殊引脚，适合初学者使用。
 * 内部上拉会把引脚默认拉到 HIGH，按下按键时引脚被拉到 LOW。
 *
 * 常见问题与排查：
 * 1. 短按没反应或反应迟钝：
 *    - 检查按键是否接到了 GPIO4 和 GND
 *    - 检查 LONG_PRESS_MS 阈值是否设得太小，导致短按被误判为长按
 * 2. 按一下按键 LED 切换多次：
 *    - 这是按键抖动，检查 DEBOUNCE_MS 是否足够大（建议 30~50ms）
 *    - 也可以给按键并联一个 0.1uF 电容硬件消抖
 * 3. 长按调光速度太快或太慢：
 *    - 调整 DIM_STEP_INTERVAL_MS，数值越小调光越快
 * 4. LED 调到最亮后不能再按：
 *    - 本项目长按是循环调光：0%→100%→0%，如果到 100% 不继续，
 *      检查 map() 和 brightness 的更新方向
 * 5. 程序似乎卡住：
 *    - 检查 loop() 里是否用了 delay() 阻塞，本项目尽量使用 millis()
 */

// 定义引脚
#define LED_PIN      2    // LED 输出引脚
#define BUTTON_PIN   4    // 按键输入引脚

// PWM 参数
#define PWM_FREQ        5000    // PWM 频率 5kHz
#define PWM_RESOLUTION  8       // 8 bit 分辨率，占空比 0~255

// 按键消抖时间，单位毫秒
// 机械按键抖动通常在 5~20ms，留一点余量用 40ms
#define DEBOUNCE_MS     40

// 长按阈值，单位毫秒
// 按住超过 600ms 认为是长按
#define LONG_PRESS_MS   600

// 长按调光时，每多少毫秒改变一级亮度
// 数值越小，长按调光越快
#define DIM_STEP_INTERVAL_MS  80

// 亮度变化步进值
// 每次调光增加或减少多少 PWM 值
#define BRIGHTNESS_STEP       5

// 当前 LED 开关状态
// true = 开，false = 关
bool ledState = false;

// 当前亮度值，范围 0~255
// 0 表示最暗（熄灭），255 表示最亮
int brightness = 128;

// 亮度变化方向，用于长按循环调光
// 1 表示增加亮度，-1 表示减小亮度
int dimDirection = 1;

// 按键相关变量
int lastButtonReading = HIGH;       // 上一次读取到的按键状态（带消抖前）
int stableButtonState = HIGH;       // 稳定后的按键状态
unsigned long lastDebounceTime = 0; // 上次状态变化的时间
unsigned long buttonDownTime = 0;   // 按键按下的时间
bool longPressHandled = false;      // 标记本次长按是否已经处理过，避免松开后又触发短按
unsigned long lastDimStepTime = 0;  // 上次调光的时间

void setup() {
  // 初始化串口
  Serial.begin(115200);
  delay(1000);

  Serial.println("================================");
  Serial.println("ESP32 综合项目 P02：按键调光灯");
  Serial.println("================================");

  // 设置 LED 引脚为输出
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // 初始化 PWM
  ledcAttach(LED_PIN, PWM_FREQ, PWM_RESOLUTION);

  // 设置按键引脚为输入，启用内部上拉
  // 按键未按下时为 HIGH，按下时为 LOW
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // 初始化时 LED 是关闭的
  updateLedOutput();

  Serial.println("短按按键：开关 LED");
  Serial.println("长按按键：循环调节亮度 0%→100%→0%");
}

void loop() {
  // 读取按键当前状态
  int reading = digitalRead(BUTTON_PIN);

  // 如果读取值和上一次不同，说明按键状态可能发生变化
  // 更新去抖计时器
  if (reading != lastButtonReading) {
    lastDebounceTime = millis();
  }

  // 只有当状态变化持续超过 DEBOUNCE_MS 毫秒，才认为是真实变化
  if ((millis() - lastDebounceTime) > DEBOUNCE_MS) {
    // 如果稳定状态和当前读取值不同，说明按键状态真的变了
    if (reading != stableButtonState) {
      stableButtonState = reading;

      // 按键被按下（稳定状态变成 LOW）
      if (stableButtonState == LOW) {
        buttonDownTime = millis();
        longPressHandled = false;
        Serial.println("按键按下");
      }
      // 按键被松开（稳定状态变成 HIGH）
      else {
        unsigned long pressDuration = millis() - buttonDownTime;
        Serial.print("按键松开，按住时长：");
        Serial.print(pressDuration);
        Serial.println(" ms");

        // 如果按住时间小于长按阈值，且本次没有处理过长按，视为短按
        if (pressDuration < LONG_PRESS_MS && !longPressHandled) {
          toggleLed();
        }
      }
    }
  }

  // 长按处理：按键一直按着时，循环调节亮度
  // 条件：
  // 1. 按键当前稳定状态是 LOW（按着）
  // 2. 已经按下的时间超过长按阈值
  // 3. 距离上次调光已经过去了 DIM_STEP_INTERVAL_MS 毫秒
  if (stableButtonState == LOW &&
      (millis() - buttonDownTime) >= LONG_PRESS_MS &&
      (millis() - lastDimStepTime) >= DIM_STEP_INTERVAL_MS) {

    // 标记本次长按已经处理过，这样松开后不会触发短按
    longPressHandled = true;

    // 更新上次调光时间
    lastDimStepTime = millis();

    // 改变亮度
    brightness += BRIGHTNESS_STEP * dimDirection;

    // 如果亮度达到上限或下限，改变方向
    if (brightness >= 255) {
      brightness = 255;
      dimDirection = -1;
      Serial.println("亮度达到最大，开始减小");
    }
    else if (brightness <= 0) {
      brightness = 0;
      dimDirection = 1;
      Serial.println("亮度达到最小，开始增加");
    }

    // 长按调光时自动把灯打开
    if (!ledState) {
      ledState = true;
    }

    // 更新 LED 输出
    updateLedOutput();

    Serial.print("长按调光，当前亮度：");
    Serial.println(brightness);
  }

  // 保存本次读取值，供下次循环比较
  lastButtonReading = reading;
}

// 自定义函数：切换 LED 开关状态
// 短按时调用此函数
void toggleLed() {
  // 切换状态
  ledState = !ledState;

  // 输出当前状态
  updateLedOutput();

  Serial.print("短按切换，LED 状态：");
  Serial.println(ledState ? "开" : "关");
}

// 自定义函数：根据 ledState 和 brightness 更新 LED 输出
// 如果灯是关的，输出 0；如果是开的，输出当前亮度
void updateLedOutput() {
  if (ledState) {
    // 灯开着，输出当前亮度
    ledcWrite(LED_PIN, brightness);
  }
  else {
    // 灯关着，输出 0
    ledcWrite(LED_PIN, 0);
  }
}

/*
 * 扩展挑战：
 * 1. 增加双击检测：快速按两下实现某个功能（比如直接跳到最亮）。
 * 2. 把亮度保存到 Preferences/Flash 中，断电后再开机恢复上次的亮度。
 * 3. 增加一个独立按键，一个负责开关，一个负责调光，降低学习成本。
 * 4. 加入 OLED 显示，实时显示当前亮度和开关状态。
 * 5. 把 LED 换成 RGB LED，长按不仅调亮度还切换颜色。
 */
