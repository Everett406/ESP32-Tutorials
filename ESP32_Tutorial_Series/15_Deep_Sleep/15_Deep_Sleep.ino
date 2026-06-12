/*
 * ESP32 教程系列 15：Deep Sleep 深度睡眠 —— 让设备省电运行
 * 
 * 学习目标：
 * 1. 理解 ESP32 有哪些功耗模式，以及它们之间的区别
 * 2. 理解 RTC 内存的作用：深度睡眠时保存数据
 * 3. 学会用定时器唤醒深度睡眠
 * 4. 学会用 GPIO 外部唤醒深度睡眠
 * 5. 学会读取唤醒原因，判断是怎么醒来的
 * 
 * 前置知识：
 * - 已经会用 pinMode()、digitalWrite() 控制 LED
 * - 已经会用串口打印调试信息
 * - 理解按键 INPUT_PULLUP 接法
 * 
 * 硬件连接：
 * - ESP32 GPIO2  →  LED 正极（长脚）
 * - LED 负极（短脚） →  330Ω 电阻  →  GND
 * - ESP32 GPIO4  →  按键一端
 * - 按键另一端    →  GND
 * 
 * 为什么需要深度睡眠？
 * 很多物联网设备用电池供电，例如温湿度传感器每 10 分钟上报一次数据。
 * 如果 ESP32 一直全速运行，电流可能高达 100~200mA，电池很快耗尽。
 * 进入 Deep Sleep 后，电流可以降到 10μA 左右，极大延长续航。
 */

#define LED_PIN 2              // LED 引脚
#define WAKEUP_BUTTON_PIN 4    // 用于外部唤醒的按键引脚

// RTC_DATA_ATTR 是一个特殊的修饰符，
// 它告诉编译器：这个变量要放在 RTC 内存里，而不是普通内存。
// 
// 什么是 RTC 内存？
// RTC（Real-Time Clock，实时时钟）内存是 ESP32 内部一块特殊的存储区域，
// 在 Deep Sleep 期间不会被断电清除。
// 普通内存（RAM）在深度睡眠后会全部丢失，程序从 setup() 重新开始。
// 如果我们想记录"这是第几次启动"，就必须用 RTC_DATA_ATTR 把变量存到 RTC 内存。
RTC_DATA_ATTR int bootCount = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);

  // 每次从 Deep Sleep 唤醒或普通开机，ESP32 都会复位并重新执行 setup()，
  // 所以 bootCount 会每次加 1。
  bootCount++;

  Serial.println("================================");
  Serial.println("ESP32 教程 15：Deep Sleep 深度睡眠");
  Serial.println("================================");

  Serial.print("这是第 ");
  Serial.print(bootCount);
  Serial.println(" 次启动");

  // 打印唤醒原因，帮助我们判断这次启动是因为定时器、按键还是普通复位
  printWakeupReason();

  // 初始化 LED 引脚
  pinMode(LED_PIN, OUTPUT);

  // 点亮 LED 2 秒，表示"我已经醒来了"
  digitalWrite(LED_PIN, HIGH);
  delay(2000);
  digitalWrite(LED_PIN, LOW);

  Serial.println("准备进入深度睡眠 10 秒...");
  Serial.println("你也可以按下 GPIO4 连接的按键提前唤醒");

  // Serial.flush() 强制把串口缓冲区里的数据全部发送出去。
  // 为什么需要它？
  // 因为进入 Deep Sleep 后 CPU 会停止，如果串口数据还没发完就睡了，
  // 电脑端可能只看到半截信息。flush() 会等待发送完成再进入睡眠。
  Serial.flush();

  // esp_sleep_enable_timer_wakeup(time_us) 配置定时器唤醒。
  // 参数单位是微秒（us），1 秒 = 1,000,000 微秒。
  // 这里配置 10 秒后自动唤醒。
  esp_sleep_enable_timer_wakeup(10 * 1000 * 1000);

  // esp_sleep_enable_ext0_wakeup(pin, level) 配置 GPIO 外部唤醒。
  // 
  // EXT0 是什么？
  // EXT0 是 ESP32 深度睡眠外部唤醒方式之一，只能配置一个 GPIO。
  // 当这个 GPIO 出现指定电平时，ESP32 会被唤醒。
  // 
  // 参数说明：
  // - 第一个参数：GPIO 编号，需要强制转换成 gpio_num_t 类型
  // - 第二个参数：触发电平，0 表示低电平触发，1 表示高电平触发
  // 
  // 为什么选低电平触发？
  // 因为按键另一端接 GND，按下时 GPIO4 变成低电平。
  // 所以选择低电平触发，按下按键就能唤醒。
  esp_sleep_enable_ext0_wakeup((gpio_num_t)WAKEUP_BUTTON_PIN, 0);

  // 进入深度睡眠。
  // 执行到这里后，当前程序停止运行，CPU 和大部分外设关闭。
  // 当定时器到时间或 GPIO 触发时，ESP32 会复位并从头开始执行 setup()。
  // 注意：进入 Deep Sleep 后 loop() 不会被执行。
  esp_deep_sleep_start();
}

void loop() {
  // 进入 Deep Sleep 后程序会停止，不会执行到这里。
  // 即使被定时器唤醒，ESP32 也会复位重新进入 setup()。
  // 所以 loop() 在这里实际上永远不会运行。
}

// printWakeupReason() 用来读取并打印唤醒原因
void printWakeupReason() {
  // esp_sleep_get_wakeup_cause() 返回本次唤醒的原因。
  // 它的返回值是一个枚举类型 esp_sleep_wakeup_cause_t。
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

  Serial.print("唤醒原因：");

  // 根据唤醒原因打印不同的中文说明
  switch (wakeup_reason) {
    case ESP_SLEEP_WAKEUP_EXT0:
      Serial.println("外部 GPIO 唤醒（EXT0）");
      break;
    case ESP_SLEEP_WAKEUP_EXT1:
      // EXT1 和 EXT0 类似，但可以同时监控多个 GPIO 的组合
      Serial.println("外部 GPIO 组合唤醒（EXT1）");
      break;
    case ESP_SLEEP_WAKEUP_TIMER:
      Serial.println("定时器唤醒");
      break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD:
      Serial.println("触摸唤醒");
      break;
    case ESP_SLEEP_WAKEUP_ULP:
      // ULP（Ultra-Low-Power）是 ESP32 的超低功耗协处理器，
      // 可以在 Deep Sleep 期间继续运行一些简单程序
      Serial.println("ULP 协处理器唤醒");
      break;
    default:
      // 如果不是以上任何一种，说明是普通的开机或复位按钮复位
      Serial.println("普通开机或外部复位");
      break;
  }
}

/*
 * 重要概念解释：
 * 
 * 1. ESP32 的功耗模式（从高到低）：
 *    - Active（全速运行）：所有外设都工作，功耗最高，约 100~240mA。
 *    - Modem Sleep：关闭 Wi-Fi/蓝牙射频，CPU 仍运行，功耗降低。
 *    - Light Sleep：CPU 暂停，部分外设关闭，可被快速唤醒，功耗约 0.8mA。
 *    - Deep Sleep：大部分外设关闭，只保留 RTC 内存和 RTC 外设，
 *                   功耗约 10~150μA（具体取决于板载稳压器和外围电路）。
 *    - Hibernation：比 Deep Sleep 更省电，但 RTC 外设也关闭，
 *                   只能用 RTC GPIO 中有限的几个引脚唤醒。
 * 
 * 2. 深度睡眠和普通睡眠的区别：
 *    进入 Deep Sleep 后，ESP32 会"复位"，程序从 setup() 重新开始执行，
 *    普通变量会全部丢失。
 *    这和我们用 delay() 暂停完全不同，delay() 只是暂停当前代码，
 *    变量和状态都保留。
 * 
 * 3. RTC 内存能存多少数据？
 *    ESP32 的 RTC 慢速内存大小约为 8KB，
 *    足够保存几个计数器、配置参数或少量传感器数据。
 *    不要用它保存大数组或长字符串。
 */

/*
 * 排查提示：
 * 1. 如果进入 Deep Sleep 后无法唤醒：
 *    - 检查定时器时间是否设得太长。
 *    - 检查唤醒引脚是否真的是 GPIO4，并且按键另一端接 GND。
 *    - 注意：ESP32 不同型号的 RTC GPIO 编号可能不同，
 *      经典款 ESP32 的 GPIO4 支持 EXT0 唤醒。
 *
 * 2. 如果唤醒原因总是显示"普通开机"：
 *    - 说明 ESP32 没有真正进入 Deep Sleep，可能在 Serial.flush() 之前就崩溃了。
 *    - 检查串口输出，看有没有错误信息。
 *
 * 3. 如果实测电流没有明显下降：
 *    - 开发板上的 USB 转串口芯片、电源指示灯会额外耗电。
 *    - 要测真实 Deep Sleep 电流，最好用裸模块或专门设计的低功耗板。
 *
 * 4. 如果串口输出乱码或中断：
 *    - 从 Deep Sleep 唤醒后重新初始化串口需要时间，
 *      开头加 delay(1000) 可以让串口监视器准备好。
 */

/*
 * 扩展练习：
 * 1. 用触摸唤醒代替按键唤醒，做一个触摸唤醒的灯。
 * 2. 在 RTC 内存中保存传感器读数，定时唤醒后批量上传到服务器。
 * 3. 用万用表或电流计测试 Deep Sleep 的实际电流消耗。
 */
