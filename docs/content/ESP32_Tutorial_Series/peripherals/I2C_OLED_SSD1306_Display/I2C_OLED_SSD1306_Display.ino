/*
 * 教程：I2C OLED SSD1306 显示屏
 *
 * 学习目标：
 * 1. 了解 I2C 总线的概念，认识 SDA 和 SCL 两条线。
 * 2. 理解 I2C 设备地址的作用。
 * 3. 学会安装 Adafruit SSD1306 和 Adafruit GFX 库。
 * 4. 掌握在 OLED 上显示文字、数字和简单图形。
 *
 * 需要安装的库：
 * - "Adafruit SSD1306" by Adafruit
 * - "Adafruit GFX Library" by Adafruit
 * - "Adafruit BusIO" by Adafruit（通常是 SSD1306 的依赖，会自动安装）
 * 安装方法：
 * Arduino IDE → 项目 → 加载库 → 管理库...，搜索上述名称并安装。
 *
 * 硬件连接：
 * - OLED VCC → ESP32 3.3V
 * - OLED GND → ESP32 GND
 * - OLED SDA → ESP32 GPIO21（默认 I2C 数据线）
 * - OLED SCL → ESP32 GPIO22（默认 I2C 时钟线）
 *
 * 注意：
 * 大部分 128×64 的 SSD1306 OLED 模块工作电压是 3.3V，可以直接接 ESP32。
 * 如果你的 OLED 是 128×32 分辨率，请在代码里把 SCREEN_HEIGHT 改成 32。
 */

#include <Wire.h>               // Wire 库提供 I2C 通信功能，是 ESP32 自带的库。
#include <Adafruit_GFX.h>       // GFX 库提供画点、画线、显示文字等基础图形函数。
#include <Adafruit_SSD1306.h>   // SSD1306 库专门驱动 SSD1306 芯片的 OLED 屏幕。

// SCREEN_WIDTH 和 SCREEN_HEIGHT：屏幕分辨率，单位是像素。
// 这里以最常见的 128×64 像素 OLED 为例。
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// OLED_RESET：复位引脚。
// 很多模块没有单独的复位引脚，直接用 -1 表示使用 ESP32 内部软件复位。
#define OLED_RESET -1

// SCREEN_ADDRESS：OLED 的 I2C 地址。
// 常见地址有两种：0x3C 和 0x3D，具体取决于模块背面的电阻焊接方式。
// 如果不确定，可以先用 I2C 扫描程序查找地址。
#define SCREEN_ADDRESS 0x3C

// 创建一个 Adafruit_SSD1306 对象。
// 参数依次是：屏幕宽度、屏幕高度、使用的 I2C 总线、复位引脚。
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// 计数器变量，用来演示动态刷新。
int counter = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);

  // SSD1306_SWITCHCAPVCC 表示使用内部电荷泵升压供电，
 // 这样只需要接 3.3V 就能产生 OLED 需要的较高驱动电压。
  // 第二个参数是 I2C 地址。
  // begin() 返回 true 表示初始化成功，false 表示失败（常见原因是地址或接线错误）。
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("OLED 初始化失败，请检查 I2C 地址和接线！");
    // 初始化失败后进入死循环，避免继续执行后续绘制代码。
    while (true) {
      delay(1000);
    }
  }

  // clearDisplay() 清除屏幕上的所有内容，但只是把显存清零，
  // 真正的刷新需要调用 display.display() 才会发送到屏幕上。
  display.clearDisplay();

  // setTextSize(1) 设置文字大小为 1 倍，也就是每个字符占 6×8 像素。
  display.setTextSize(1);

  // setTextColor(SSD1306_WHITE) 设置文字颜色为白色。
  // OLED 只有黑白两色，白色表示点亮像素，黑色表示熄灭。
  display.setTextColor(SSD1306_WHITE);

  // setCursor(x, y) 设置文字起始坐标，左上角是 (0, 0)。
  display.setCursor(0, 0);

  // println() 把字符串写入显存。
  display.println("Hello ESP32!");
  display.println("OLED 显示成功");

  // display() 把显存里的内容一次性发送到 OLED 屏幕。
  // 所有绘制操作都要在 display() 之后才看得见。
  display.display();

  Serial.println("OLED 初始化完成");
}

void loop() {
  // 每次循环先清屏，避免旧内容残留。
  display.clearDisplay();

  // 显示第一行固定标题。
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.println("ESP32 OLED 示例");

  // 画一条水平线，把标题和下方内容分开。
  // drawLine(x0, y0, x1, y1, color)
  display.drawLine(0, 10, SCREEN_WIDTH - 1, 10, SSD1306_WHITE);

  // 显示计数器，字号放大到 2，更醒目。
  display.setCursor(0, 16);
  display.setTextSize(2);
  display.print("Count:");
  display.println(counter);

  // 显示当前时间估算（毫秒数）。
  display.setTextSize(1);
  display.setCursor(0, 40);
  display.print("Millis: ");
  display.println(millis());

  display.display();

  // 计数器递增。
  counter++;

  // 每 500ms 刷新一次，肉眼刚好能看清数字变化，又不会刷新太快导致闪烁。
  delay(500);
}

/*
 * 名词解释：
 * I2C（Inter-Integrated Circuit，集成电路互联总线）：
 * 一种只需要两根线（SDA 数据线、SCL 时钟线）的同步通信协议。
 * 多设备可以共用这两根线，每个设备有唯一的 I2C 地址，
 * 主控通过地址选择要和哪个设备通信。
 *
 * SDA（Serial Data，串行数据）：传输数据的双向线。
 * SCL（Serial Clock，串行时钟）：由主控产生的时钟线，用来同步数据传输。
 *
 * 显存（Framebuffer）：
 * 屏幕控制器内部的一块存储区，保存着屏幕上每个像素的状态。
 * 我们先在 ESP32 的显存里画好内容，再一次性发送给 OLED，这样效率更高。
 */

/*
 * 扩展练习：
 * 1. 用 drawCircle() 或 fillRect() 画一个简单的动画。
 * 2. 把 DHT 传感器的温湿度显示到 OLED 上。
 * 3. 实现一个简单的菜单，用按键切换显示不同页面。
 */
