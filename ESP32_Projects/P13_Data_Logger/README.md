# 综合项目 P13：数据记录仪（Data Logger）

## 1. 项目简介

数据记录仪是物联网中最常见的应用之一：让 ESP32 定时读取环境数据，把结果保存到本地 Flash，并通过网络或串口把数据导出到电脑进行分析。

本项目使用 **DHT22 温湿度传感器** 采集环境数据，利用 **ESP32 硬件定时器** 每隔固定时间（默认 1 小时）记录一次，数据以 **CSV** 格式写入 **LittleFS 文件系统**。用户既可以通过 **浏览器** 下载 CSV 文件，也可以通过 **串口命令** 查看和清空日志。

学完本项目，你将理解如何把“传感器 + 定时器 + 文件系统 + Web 服务器 + 串口交互”整合成一个完整的可运行系统。

## 2. 涉及知识点

| 知识点 | 对应前置教程 | 本项目中的作用 |
|--------|--------------|----------------|
| DHT22 温湿度读取 | `peripherals/DHT11_DHT22_Sensor` | 获取需要记录的环境数据 |
| 硬件定时器中断 | `10_Timer_Interrupt` | 周期性触发记录任务，不阻塞主循环 |
| LittleFS 文件系统 | 本项目新引入 | 把数据持久化保存到 Flash |
| Web 服务器 | `07_Web_Server_LED` | 提供下载日志的网页接口 |
| 串口通信 | `05_Serial_Communication` | 提供无需网络的命令行交互 |
| Wi-Fi 连接 | `06_WiFi_Connect` | 让 Web 服务器可以通过局域网访问 |

## 3. 硬件清单

| 器件 | 数量 | 说明 |
|------|------|------|
| ESP32 开发板 | 1 块 | 主控芯片 |
| DHT22 温湿度传感器 | 1 个 | 采集温度和湿度 |
| LED | 1 个 | 记录数据时闪烁提示 |
| 330Ω 电阻 | 1 个 | LED 限流 |
| 4.7kΩ ~ 10kΩ 电阻 | 1 个 | DHT22 DATA 引脚上拉（可选但推荐） |
| 杜邦线 | 若干 | 连接电路 |
| 面包板 | 1 块 | 搭建电路 |

> **为什么 DATA 需要上拉电阻？**  
> DHT22 使用单总线协议，空闲时数据线需要保持高电平。虽然 ESP32 可以配置内部上拉，但外部上拉电阻能让通信在长杜邦线或嘈杂环境下更稳定。

## 4. 电路连接

| ESP32 引脚 | 连接器件 | 备注 |
|------------|----------|------|
| 3.3V | DHT22 VCC | DHT22 可直接用 3.3V 供电 |
| GND | DHT22 GND | 共地 |
| GPIO4 | DHT22 DATA | 读取温湿度 |
| GPIO2 | LED 正极（长脚） | 板载 LED 通常已接好 |
| GND | LED 负极（短脚）→ 330Ω 电阻 → GND | 限流 |

> 如果使用的 DHT22 模块已经内置上拉电阻，可以省略外部上拉电阻。模块是否内置上拉通常可以在商品说明中查看。

## 5. 代码思路

整个程序的运行流程如下：

1. **初始化阶段（setup）**
   - 初始化串口、LED、DHT22。
   - 挂载 LittleFS 文件系统。
   - 如果日志文件不存在，创建文件并写入 CSV 表头；如果已存在，统计已有记录条数。
   - 连接 Wi-Fi。
   - 注册 Web 路由并启动 Web 服务器。
   - 启动硬件定时器，设置每小时触发一次。
   - 立即记录第一条数据，方便快速验证硬件。

2. **主循环（loop）**
   - 调用 `server.handleClient()` 处理浏览器请求。
   - 检查定时器标志 `logFlag`：如果为 true，则读取 DHT22 并把结果追加到 CSV。
   - 处理串口命令（`download`、`clear`、`info`）。
   - 延时 10 毫秒，避免空转。

3. **数据导出方式**
   - 浏览器访问 `http://ESP32_IP/` 可看到下载按钮。
   - 浏览器访问 `/download` 可直接下载 `log.csv`。
   - 串口输入 `download` 或 `d` 可在串口监视器查看完整 CSV 内容。
   - 串口输入 `clear` 或 `c` 可清空日志。

## 6. 关键代码解析

### 6.1 初始化日志文件

```cpp
if (!LittleFS.exists(LOG_FILE)) {
  File f = LittleFS.open(LOG_FILE, "w");
  if (f) {
    f.println("elapsed_ms,temperature_c,humidity_pct");
    f.close();
  }
}
```

**为什么要先判断文件是否存在？**  
因为 Flash 中的文件断电后仍然保留，程序重启后不应该重复创建表头，否则会出现多条表头行。先判断再创建可以避免这个问题。

### 6.2 硬件定时器配置

```cpp
logTimer = timerBegin(1000000);
timerAttachInterrupt(logTimer, &onLogTimer);
timerAlarm(logTimer, (uint64_t)LOG_INTERVAL_MS * 1000ULL, true, 0);
```

- `timerBegin(1000000)`：让定时器以 1MHz 计数，也就是每微秒加 1。
- `timerAttachInterrupt(...)`：绑定中断服务函数。
- `timerAlarm(...)`：设置报警值为 `LOG_INTERVAL_MS * 1000` 微秒，自动重载，无限次触发。

**为什么用硬件定时器而不是 `delay()`？**  
`delay()` 会让 CPU 空转，期间无法处理 Web 请求和串口命令。硬件定时器由独立硬件驱动，到时间才通知主循环，主循环其余时间可以做其他事情。

### 6.3 追加 CSV 记录

```cpp
void appendLog(unsigned long timestampMs, float temperature, float humidity) {
  File f = LittleFS.open(LOG_FILE, "a");
  f.print(timestampMs);
  f.print(",");
  f.print(temperature, 2);
  f.print(",");
  f.println(humidity, 2);
  f.close();
}
```

- 模式 `"a"` 表示 append，会在文件末尾追加，不会覆盖已有数据。
- `f.print(value, 2)` 让浮点数保留两位小数，符合日常阅读习惯。
- **写完后一定要 `f.close()`**：否则数据可能只停留在缓冲区，没有真正写入 Flash。

### 6.4 Web 下载 CSV

```cpp
server.sendHeader("Content-Disposition", "attachment; filename=\"log.csv\"");
server.send(200, "text/csv; charset=utf-8", content);
```

- `Content-Disposition: attachment` 告诉浏览器这是一个需要下载的附件，而不是直接显示在网页中。
- `text/csv` 表示内容类型是 CSV 文件，电脑收到后可能会用 Excel 打开。

## 7. 运行效果

上传程序后，打开 Arduino IDE 串口监视器（波特率 115200），你应该能看到类似输出：

```
================================
综合项目 P13：数据记录仪
================================
DHT22 已初始化
LittleFS 文件系统已挂载
已创建新的日志文件
正在连接 Wi-Fi.......................
Wi-Fi 已连接，IP 地址：192.168.1.105
请用浏览器访问：http://192.168.1.105
Web 服务器已启动
定时器已启动，将按设定间隔自动记录数据
已记录第 1 条数据：时间=5236ms, 温度=25.60, 湿度=58.00
输入 download/d 可在串口查看日志，clear/c 清空日志
```

过一段时间后（默认 1 小时），串口会再次打印新的记录。在浏览器中打开打印出的 IP 地址，可以看到下载按钮。点击“下载 CSV”后，文件内容类似：

```csv
elapsed_ms,temperature_c,humidity_pct
5236,25.60,58.00
3605236,25.50,58.20
7205236,25.40,58.10
```

## 8. 常见问题

| 问题现象 | 可能原因 | 排查方法 |
|----------|----------|----------|
| 串口显示 “DHT22 读取失败” | 接线松动、缺少上拉、供电不足 | 检查 GPIO4 接线；DATA 与 3.3V 之间加 4.7kΩ 上拉；确认 VCC 为 3.3V |
| LittleFS 初始化失败 | 分区表没有选择带文件系统的 | 在 Arduino IDE “工具”→“Partition Scheme” 中选择 Default 4MB with spiffs 或类似选项 |
| 日志文件为空 | 尚未到达记录间隔或读取失败 | 程序启动时会记录一条；之后每 1 小时一条；可把 `LOG_INTERVAL_MS` 改小测试 |
| 浏览器无法访问 | Wi-Fi 连接失败、手机和 ESP32 不在同一局域网 | 检查串口是否打印 IP；确认访问设备与 ESP32 连接同一 Wi-Fi；检查路由器是否禁止内网互访 |
| 点击下载后内容乱码 | 浏览器编码问题 | 本程序已设置 `charset=utf-8`；可尝试换浏览器或用记事本打开下载文件 |
| Flash 写入次数是否够用？ | 每小时写一次，寿命约 10 万次 | 大约可用 11 年；如果每秒写一次则一年多耗尽 |

## 9. 扩展挑战

1. **加入实时时钟**：购买 DS3231 或 NTP 网络对时，把 CSV 中的 `elapsed_ms` 替换成真实年月日时分秒，方便长期数据分析。
2. **多传感器扩展**：在 CSV 中增加光敏电阻、土壤湿度、气压等字段，做成家庭环境综合记录仪。
3. **云端上传**：增加一个 HTTP POST 任务，每天把当天 CSV 上传到服务器或写入 OneNet、ThingsBoard 等物联网平台。
