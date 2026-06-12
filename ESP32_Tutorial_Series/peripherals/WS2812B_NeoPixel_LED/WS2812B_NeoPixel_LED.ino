/*
 * 教程：WS2812B NeoPixel LED 灯带
 *
 * 学习目标：
 * 1. 了解 WS2812B 这种"可寻址"LED 的工作原理。
 * 2. 理解 RGB 颜色模型和 GRB 数据顺序。
 * 3. 学会安装 Adafruit NeoPixel 库。
 * 4. 实现彩虹流动、逐个点亮等常见灯效。
 *
 * 需要安装的库：
 * - "Adafruit NeoPixel" by Adafruit
 * 安装方法：
 * Arduino IDE → 项目 → 加载库 → 管理库...，搜索 "Adafruit NeoPixel" 并安装。
 *
 * 硬件连接：
 * - 灯带 5V（或 +） →  外部 5V 电源正极
 * - 灯带 GND（或 -） →  外部 5V 电源负极 并且 连接到 ESP32 GND（共地）
 * - 灯带 DIN（数据输入） →  ESP32 GPIO14
 *
 * 注意：
 * 整条灯带点亮时电流可能很大（每个 LED 全白约 60mA），
 * 不要用 ESP32 的 5V 引脚直接给长灯带供电，否则会导致重启或烧板。
 * 建议使用独立 5V 电源，容量根据 LED 数量计算，并务必共地。
 * DIN 和 ESP32 之间建议串联一个 300Ω ~ 470Ω 电阻，保护第一个 LED 的数据脚。
 */

#include <Adafruit_NeoPixel.h>

// LED_PIN：连接灯带 DIN 的 GPIO。
// GPIO14 是通用 GPIO，选择它是因为 ESP32 的 RMT 外设可以很好地驱动 WS2812B 时序。
#define LED_PIN 14

// LED_COUNT：灯带上 LED 的数量。
// 请根据你实际接的灯带修改这个数字。
#define LED_COUNT 8

// 创建一个 Adafruit_NeoPixel 对象。
// 参数依次是：LED 数量、数据引脚、像素颜色和协议格式。
// NEO_GRB + NEO_KHZ800 表示颜色顺序是绿-红-蓝，数据速率 800kHz，
// 这是 WS2812B 最常见的配置。
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(115200);
  delay(1000);

  // begin() 初始化灯带，设置引脚和内部缓冲区。
  strip.begin();

  // show() 把当前缓冲区里的颜色发送到灯带上。
  // 刚开始缓冲区全黑，所以这一步相当于熄灭所有 LED。
  strip.show();

  // setBrightness(0~255) 设置整体亮度。
  // 255 最亮，但会很刺眼且耗电；初学者建议先设低一点，比如 50。
  strip.setBrightness(50);

  Serial.println("WS2812B 灯带初始化完成");
}

void loop() {
  // 效果 1：用红色逐个点亮所有 LED。
  // colorWipe(color, wait) 是下面自定义的函数。
  colorWipe(strip.Color(255, 0, 0), 100);  // 红色，间隔 100ms

  // 效果 2：用绿色逐个点亮。
  colorWipe(strip.Color(0, 255, 0), 100);

  // 效果 3：用蓝色逐个点亮。
  colorWipe(strip.Color(0, 0, 255), 100);

  // 效果 4：彩虹流动效果，循环 10 次。
  for (int j = 0; j < 10; j++) {
    rainbow(20);
  }
}

// colorWipe：像擦玻璃一样，用一种颜色从第一个 LED 逐个填满整条灯带。
// color 是目标颜色，wait 是每点亮一个 LED 后等待的时间（毫秒）。
void colorWipe(uint32_t color, int wait) {
  for (int i = 0; i < strip.numPixels(); i++) {
    // setPixelColor(index, color) 设置第 index 个 LED 的颜色。
    strip.setPixelColor(i, color);

    // show() 发送数据到灯带，只有调用后 LED 才会真正变色。
    strip.show();

    delay(wait);
  }
}

// rainbow：彩虹流动效果。
// wait 是每次刷新之间的间隔，越小流动越快。
void rainbow(int wait) {
  // firstPixelHue 控制彩虹的起始色相，每次循环增加 256，实现流动。
  for (long firstPixelHue = 0; firstPixelHue < 3 * 65536; firstPixelHue += 256) {
    for (int i = 0; i < strip.numPixels(); i++) {
      // 给每个 LED 分配不同的色相，形成彩虹渐变。
      int pixelHue = firstPixelHue + (i * 65536L / strip.numPixels());
      strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue)));
    }
    strip.show();
    delay(wait);
  }
}

/*
 * 名词解释：
 * 可寻址 LED（Addressable LED）：
 * 每个 LED 内部都有一个控制芯片（WS2812B 里集成了控制芯片和 RGB 芯片）。
 * 数据像"接力赛"一样从第一个 LED 传到下一个，
 * 所以只需要一根数据线就能单独控制每个 LED 的颜色。
 *
 * RGB：
 * 红（Red）、绿（Green）、蓝（Blue）三种颜色的组合。
 * 每个颜色用 0~255 表示亮度，(255, 0, 0) 是纯红，(255, 255, 255) 是白光。
 *
 * GRB：
 * WS2812B 接收数据时颜色顺序是绿、红、蓝，不是常见的 RGB。
 * Adafruit_NeoPixel 库会自动处理这个顺序，我们只需要在初始化时指定 NEO_GRB。
 *
 * HSV（Hue, Saturation, Value）：
 * 色相、饱和度、亮度。Hue 是颜色种类（0~65535），Saturation 是鲜艳程度，
 * Value 是明暗。彩虹效果就是通过连续变化 Hue 实现的。
 */

/*
 * 扩展练习：
 * 1. 写一个呼吸灯效果，让 LED 亮度从 0 渐变到 255 再变回 0。
 * 2. 用串口输入 RGB 值，实时改变灯带颜色。
 * 3. 结合麦克风或声音传感器，做一个随声音跳动的频谱灯。
 */
