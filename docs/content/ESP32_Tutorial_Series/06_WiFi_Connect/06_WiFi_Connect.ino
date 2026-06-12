/*
 * ESP32 教程系列 06：Wi-Fi 连接 - 让 ESP32 接入无线网络
 * 
 * 学习目标：
 * 1. 理解什么是 Wi-Fi、SSID、IP、MAC 地址和 RSSI
 * 2. 学会使用 WiFi.h 把 ESP32 连接到家里的路由器
 * 3. 学会判断 Wi-Fi 连接状态并做简单的断线重连
 * 4. 掌握串口打印调试信息的基本方法
 * 
 * 前置知识：
 * - 已完成教程 05《串口通信》，了解 Serial.begin() 和 Serial.println()
 * - 知道家里路由器的 Wi-Fi 名称和密码
 * 
 * 硬件连接：
 * - 本示例只需要一块 ESP32 开发板，通过 USB 线连接电脑
 * - 不需要外接 LED、按钮等元件
 * - 如果希望更直观，可以在 GPIO2 上接一个 LED（教程 01 有详细接法）
 * 
 * 关键概念解释：
 * - Wi-Fi：一种短距离无线通信技术，让设备不用插网线就能接入局域网/互联网。
 * - SSID（Service Set Identifier，服务集标识符）：通俗说就是 Wi-Fi 的名称，
 *   比如“TP-LINK_1234”。手机搜索 Wi-Fi 时看到的名字就是 SSID。
 * - 路由器（Router）：把家里的宽带信号转换成 Wi-Fi 信号的设备，也是局域网的“大门”。
 * - 2.4GHz / 5GHz：Wi-Fi 常用的两个频段。ESP32 只支持 2.4GHz，所以必须连接
 *   2.4GHz 的 Wi-Fi。很多双频路由器会把 5GHz 命名为另一个 SSID，要注意区分。
 * - STA（Station，站点）模式：ESP32 像手机、电脑一样，主动去连接路由器。
 *   这是最常见的上网方式。
 * - AP（Access Point，接入点）模式：ESP32 自己发出一个 Wi-Fi 热点，其他设备
 *   可以连到它。本示例暂时不用，后续扩展会讲到。
 * - IP 地址（Internet Protocol Address）：设备在局域网中的“门牌号”。
 *   只有拿到 IP 地址，ESP32 才能和其他设备（比如你的电脑、手机）互相通信。
 * - MAC 地址（Media Access Control Address）：每块网卡出厂时自带的全球唯一
 *   硬件地址，相当于设备的“身份证号”，不会随网络变化而改变。
 * - RSSI（Received Signal Strength Indicator，接收信号强度指示）：
 *   单位是 dBm（分贝毫瓦）。数值越接近 0 信号越好，比如 -30 dBm 很强，
 *   -80 dBm 就很弱，可能会断线。
 * - dBm：功率的一种对数单位，用来表示无线信号强度。0 dBm 表示 1 毫瓦，
 *   负数越大表示信号越弱。
 */

// 引入 WiFi 库。这是 ESP32 Arduino 3.x 内置的核心库，
// 封装了连接 Wi-Fi、扫描网络、获取 IP 等复杂操作。
#include <WiFi.h>

// ===== 修改这里：填入你的 Wi-Fi 信息 =====
// const char* 表示“指向常量字符的指针”，这里用来保存不会修改的字符串。
// 用双引号包住的就是字符串常量。改成你自己家里/公司的 Wi-Fi 名称和密码。
// 注意：SSID 和密码区分大小写，有一个字母错了就连不上。
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// 最大重试次数。连接路由器需要一定时间，如果 30 次（约 15 秒）还连不上，
// 就判定失败，并提示用户检查配置。这个数字不能太小，否则路由器还没响应
// 就提前放弃了；也不能太大，否则程序会卡很久。
const int MAX_RETRY = 30;

void setup() {
  // 初始化串口，波特率 115200。波特率必须和 Arduino IDE 串口监视器选的一致，
  // 否则看到的是乱码。delay(1000) 给串口一点准备时间，避免最先打印的字符丢失。
  Serial.begin(115200);
  delay(1000);

  // 打印标题，方便在串口监视器中确认运行的是哪个程序。
  Serial.println("================================");
  Serial.println("ESP32 教程 06：Wi-Fi 连接");
  Serial.println("================================");

  // WiFi.mode(mode) 设置 ESP32 的 Wi-Fi 工作模式：
  // - WIFI_STA：STA 模式，ESP32 作为客户端去连接路由器（最常用）。
  // - WIFI_AP：AP 模式，ESP32 自己当热点，等待别人来连接。
  // - WIFI_AP_STA：同时开启 STA 和 AP，既能连路由器又能被别设备连接。
  // 本示例只需要让 ESP32 上网，所以用最简单的 WIFI_STA。
  // 如果不设置模式直接 begin()，库也会默认使用 STA 模式，
  // 但显式写出来可以让代码更清晰，也便于后续修改。
  WiFi.mode(WIFI_STA);

  // WiFi.begin(ssid, password) 开始连接指定的 Wi-Fi。
  // 这个函数不会等待连接完成，它会立即返回，然后在后台自动完成认证、
  // 获取 IP 等步骤。所以我们接下来要用循环查询状态。
  Serial.print("正在连接 Wi-Fi：");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  // 等待连接成功。WiFi.status() 返回当前连接状态：
  // - WL_CONNECTED：连接成功，已经拿到 IP 地址。
  // - WL_IDLE_STATUS：正在尝试连接，还未完成。
  // - WL_NO_SSID_AVAIL：找不到指定的 Wi-Fi（可能名称错了或信号太弱）。
  // - WL_CONNECT_FAILED：连接失败，常见原因是密码错误。
  // 循环里每次等 500ms 打印一个“.”，这样用户能看到程序还在努力连接。
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < MAX_RETRY) {
    delay(500);
    Serial.print(".");
    retry++;
  }

  // 先换行，让后面的输出从新的一行开始。
  Serial.println();

  // 根据最终状态给出不同的提示。
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Wi-Fi 连接成功！");

    // WiFi.localIP() 返回 ESP32 在当前局域网中被分配到的 IP 地址。
    // 例如 192.168.1.105。这个地址在每次重启后可能不同，取决于路由器的 DHCP。
    Serial.print("IP 地址：");
    Serial.println(WiFi.localIP());

    // WiFi.subnetMask() 返回子网掩码，用来判断哪些 IP 和自己在同一个子网。
    Serial.print("子网掩码：");
    Serial.println(WiFi.subnetMask());

    // WiFi.gatewayIP() 返回网关地址，通常就是路由器的 IP 地址。
    // 设备想访问互联网时，数据会先发给网关。
    Serial.print("网关地址：");
    Serial.println(WiFi.gatewayIP());

    // WiFi.RSSI() 返回当前 Wi-Fi 信号强度，单位 dBm。
    // -30 到 -50 dBm：信号很好；
    // -50 到 -70 dBm：信号一般但通常可用；
    // -70 dBm 以下：信号较弱，可能会出现卡顿或掉线。
    Serial.print("信号强度：");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");

    // WiFi.macAddress() 返回 ESP32 的 MAC 地址，格式如 AA:BB:CC:DD:EE:FF。
    // 有些路由器开启了“MAC 地址过滤”，只允许白名单里的设备上网，
    // 此时需要把这个地址添加到路由器白名单中。
    Serial.print("MAC 地址：");
    Serial.println(WiFi.macAddress());
  } else {
    // 连接失败时给出清晰的排查清单，这是新手最容易困惑的地方。
    Serial.println("Wi-Fi 连接失败，请按顺序检查：");
    Serial.println("1. ssid 和 password 是否填写正确，注意大小写和空格");
    Serial.println("2. 路由器是否为 2.4GHz 频段（ESP32 不支持 5GHz）");
    Serial.println("3. 路由器是否开启了 MAC 地址过滤，可尝试关闭或把 MAC 加入白名单");
    Serial.println("4. Wi-Fi 信号是否太弱，ESP32 是否离路由器太远");
    Serial.println("5. 密码是否使用了特殊字符，某些字符可能导致解析异常");
  }
}

void loop() {
  // 每 5 秒检查一次连接状态，实现简单的断线重连。
  // 实际项目中，如果连接断开，可以调用 WiFi.reconnect() 重新连接。
  // reconnect() 会保留之前 begin() 时保存的 ssid 和 password。
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Wi-Fi 连接正常");
  } else {
    Serial.println("Wi-Fi 已断开，正在重连...");
    WiFi.reconnect();
  }
  delay(5000);
}

/*
 * 扩展练习：
 * 1. 用 WiFi.scanNetworks() 扫描附近有哪些 Wi-Fi，把它们的 SSID 和 RSSI 打印出来。
 *    这样可以确认你要连接的 Wi-Fi 是否真的能被 ESP32 搜到。
 * 2. 连接失败时自动进入 AP 模式，创建一个名为“ESP32_Config”的热点，
 *    让用户用手机连接后再配置正确的 Wi-Fi 信息（类似教程 extensions 中的思路）。
 * 3. 在连接成功后调用 WiFi.setAutoReconnect(true)，让 ESP32 在掉线后
 *    自动尝试重连，不用程序手动调用 reconnect()。
 * 4. 把 WiFi.localIP() 存到一个变量里，后续发送给服务器或显示在 OLED 上。
 */
