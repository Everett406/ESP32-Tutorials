/*
 * P09_Smart_Watering_System
 * 智能浇花系统
 *
 * 学习目标：
 * 1. 学会读取土壤湿度传感器的模拟信号
 * 2. 学会用继电器控制水泵（或电机、LED 等大电流设备）
 * 3. 理解硬件定时器中断的工作原理
 * 4. 掌握 ESP32 Arduino 3.x 定时器中断的新写法
 *
 * 前置知识：
 * - ADC（Analog-to-Digital Converter，模数转换器）：
 *   把传感器输出的 0~3.3V 模拟电压转换成数字值（ESP32 默认 12 位，范围 0~4095）。
 * - 继电器（Relay）：
 *   一种电控开关，用小电流控制大电流设备。ESP32 GPIO 不能直接驱动水泵，
 *   需要通过继电器来控制水泵的电源通断。
 * - 定时器中断（Timer Interrupt）：
 *   由 ESP32 内部硬件定时器产生的周期性中断。到达设定时间后，CPU 会暂停
 *   当前任务，先执行中断服务函数（ISR），然后返回继续原来的任务。
 * - ISR（Interrupt Service Routine，中断服务函数）：
 *   处理中断事件的函数。ISR 里不能做耗时操作，通常只设置标志位，
 *   主循环里再处理具体逻辑。
 *
 * 硬件连接：
 * - 土壤湿度传感器：
 *   ESP32 3.3V  →  传感器 VCC
 *   ESP32 GND   →  传感器 GND
 *   ESP32 GPIO34 → 传感器 AO（模拟输出）
 *
 * - 继电器模块（低电平触发）：
 *   ESP32 3.3V  →  继电器 VCC
 *   ESP32 GND   →  继电器 GND
 *   ESP32 GPIO18 → 继电器 IN
 *
 * - 水泵（通过继电器控制）：
 *   继电器 COM  →  电源正极（如 USB 5V 或电池正极）
 *   继电器 NO   →  水泵正极
 *   水泵负极    →  电源负极 / GND
 *
 * 注意：
 * - 本项目用水泵做示例，实际测试中如果没有水泵，可用 LED 代替，
 *   看到 LED 亮起就代表"正在浇水"。
 * - 土壤湿度传感器不要长期插在有水的土壤中，容易腐蚀探针。
 *   建议只在测量时通电，测完后断电延长寿命（进阶功能）。
 */

// 土壤湿度传感器模拟输出引脚
// GPIO34 ~ GPIO39 是输入专用引脚，做 ADC 读取时干扰最小，推荐用于模拟输入
#define SOIL_SENSOR_PIN 34

// 继电器控制引脚
// GPIO18 是通用 GPIO，输出高低电平控制继电器
#define RELAY_PIN 18

// 土壤湿度阈值
// ADC 读数范围 0 ~ 4095（12 位分辨率）
// 大多数土壤湿度传感器：土壤越干燥，读数越高；土壤越湿润，读数越低
// 这里设定 2500，低于 2500 认为土壤够湿，高于 2500 认为偏干需要浇水
// 实际使用时请根据自己的传感器校准这个值
#define DRY_THRESHOLD 2500

// 浇水持续时间：3000 毫秒 = 3 秒
#define WATERING_DURATION_MS 3000

// 定时器中断周期：10 秒 = 10,000,000 微秒
// 每 10 秒检查一次土壤湿度，避免频繁浇水和频繁读取
#define CHECK_INTERVAL_US 10000000

// 硬件定时器对象指针
// ESP32 Arduino 3.x 中 timerBegin() 返回一个 hw_timer_t* 指针
hw_timer_t* check_timer = NULL;

// FreeRTOS 临界区互斥锁
// 用于保护被中断和主循环同时访问的共享变量
portMUX_TYPE timer_mux = portMUX_INITIALIZER_UNLOCKED;

// 中断标志位
// volatile 关键字非常重要：它告诉编译器这个变量可能在中断中被修改，
// 编译器不要优化掉对它的读取操作，否则主循环可能永远看不到变化
volatile bool check_flag = false;

// 当前是否正在浇水的标志
bool is_watering = false;

// 浇水开始时间（毫秒）
unsigned long watering_start_time = 0;

// 最近一次检查时的湿度读数
int last_soil_value = 0;

// setup() 只执行一次，完成初始化
void setup() {
  // 启动串口监视器，波特率 115200
  Serial.begin(115200);

  // 设置继电器引脚为输出模式
  // 继电器需要 ESP32 主动输出高低电平来控制开关
  pinMode(RELAY_PIN, OUTPUT);

  // 初始状态关闭继电器
  // 这里假设继电器是"低电平触发"：LOW 吸合（开），HIGH 释放（关）
  // 如果你的继电器是"高电平触发"，请把下面改成 HIGH 关闭、LOW 开启
  digitalWrite(RELAY_PIN, HIGH);

  // 设置土壤湿度传感器引脚为输入模式
  // 虽然 ADC 引脚默认就是输入，但显式声明更清晰
  pinMode(SOIL_SENSOR_PIN, INPUT);

  // 配置并启动硬件定时器
  // ESP32 Arduino 3.x 的新写法：
  // timerBegin(frequency) 直接指定定时器计数频率，单位 Hz
  // 这里 1,000,000 Hz = 1MHz，每个计数周期 1 微秒
  check_timer = timerBegin(1000000);

  // 检查定时器是否初始化成功
  if (check_timer == NULL) {
    Serial.println("定时器初始化失败，程序停止");
    while (1) {
      delay(1000);
    }
  }

  // 把中断服务函数绑定到定时器
  // timerAttachInterrupt(timer, &function) 是 ESP32 Arduino 3.x 的新写法
  // 注意：3.x 版本只有 2 个参数，不再有第三个 edge 参数
  timerAttachInterrupt(check_timer, &onTimer);

  // 设置定时器计数目标（闹钟值）
  // 1MHz 时钟下，计数 10,000,000 次就是 10 秒
  // 参数：定时器、闹钟值、是否自动重载、重复次数（0 表示无限次）
  timerAlarm(check_timer, CHECK_INTERVAL_US, true, 0);

  // 启动定时器
  // 有些示例中 timerAlarm 后会自动启动，但显式调用 timerStart 更安全
  timerStart(check_timer);

  Serial.println("智能浇花系统已启动");
  Serial.println("每 10 秒检查一次土壤湿度");
  Serial.print("干燥阈值：");
  Serial.println(DRY_THRESHOLD);
  Serial.println("如果土壤干燥，将自动浇水 3 秒");
}

// loop() 主循环，处理浇水控制和定时检查
void loop() {
  // 1. 检查定时器中断标志
  // 如果标志为 true，说明 10 秒时间到了，需要读取土壤湿度
  if (check_flag) {
    // 先清除标志，避免重复处理
    // 进入临界区，防止清除过程中定时器又触发并修改标志
    portENTER_CRITICAL(&timer_mux);
    check_flag = false;
    portEXIT_CRITICAL(&timer_mux);

    // 读取土壤湿度
    readSoilMoisture();
  }

  // 2. 如果正在浇水，检查是否已经浇了 3 秒
  if (is_watering) {
    unsigned long now = millis();
    if (now - watering_start_time >= WATERING_DURATION_MS) {
      stopWatering();
    }
  }
}

// 中断服务函数（ISR）
// 它由硬件定时器触发，每 10 秒执行一次
// ISR 里只做一件事：设置标志位
// 真正的读取和浇水逻辑交给 loop() 处理，避免在中断里做耗时操作
void IRAM_ATTR onTimer() {
  // 进入 ISR 临界区，安全地修改共享标志
  portENTER_CRITICAL_ISR(&timer_mux);
  check_flag = true;
  portEXIT_CRITICAL_ISR(&timer_mux);
}

// 读取土壤湿度并决定是否浇水
void readSoilMoisture() {
  // 读取 ADC 值
  // analogRead(pin) 返回 0 ~ 4095 之间的整数
  last_soil_value = analogRead(SOIL_SENSOR_PIN);

  Serial.print("土壤湿度 ADC 值：");
  Serial.println(last_soil_value);

  // 判断是否干燥
  // 如果读数大于阈值，说明土壤偏干
  if (last_soil_value > DRY_THRESHOLD) {
    Serial.println("土壤偏干，开始浇水");
    startWatering();
  } else {
    Serial.println("土壤湿度正常，无需浇水");
  }
}

// 开始浇水：吸合继电器
void startWatering() {
  // 如果已经在浇水，就不重复启动
  if (is_watering) return;

  is_watering = true;
  watering_start_time = millis();

  // 低电平触发继电器：LOW 吸合，接通水泵电源
  digitalWrite(RELAY_PIN, LOW);

  Serial.println("水泵已开启");
}

// 停止浇水：释放继电器
void stopWatering() {
  is_watering = false;

  // 低电平触发继电器：HIGH 释放，断开水泵电源
  digitalWrite(RELAY_PIN, HIGH);

  Serial.println("浇水结束，水泵已关闭");
}

/*
 * 扩展挑战：
 * 1. 增加一个按键，支持手动立即浇水一次，不受定时器限制。
 * 2. 用 OLED 或串口显示当前湿度、浇水次数、累计浇水时间等统计信息。
 * 3. 加入 Wi-Fi 和 Web 服务器，实现手机远程查看土壤湿度并手动控制浇水。
 */
