这里是我用于完成一个基于Arduino Mega 2560的BDS卫星模块的定位消息实施解析的内容。（并将其显示在SSD 1306显示屏上）
这个GitHub项目用于纪念我完成的第一个小型个人项目。
Here is my source code for receiving BDS signal and analyze it in real-time, based on Arduino Mega 2560. (And display my result in SSD 1306)
This GitHub Project aims to in memory of my first completeed personal project.

```
// 和GNSS操作无关的注释不会被翻译为英语 | I won't translate my comments for non-GNSS Operations.
// 以下所有代码均来自我的个人工程项目 | These code are ALL comes from my personal project.
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <nmeaparser.h>  // 引入解析器头文件 | Headerfile Here

#define TIMEZONE 8

// NMEA 解析器和信息结构体 | An NMEA Analyzer
NMEAInfo info;  // 定义一个 NMEA 信息结构体 | Define an NMEA Struct HERE

// SSD1306 “OLED显示屏”参数，并创建OLED显示屏对象  / (Nothing here, ignore it)
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 screen(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);  // 创建OLED对象 | Create an OLED Object

void setup() {
  Serial.begin(115200);

  // 设置你的GNSS接收机的波特率 | Set your Baud for your GNSS Receiver at YOUR OWN SERIAL
  Serial2.begin(115200);  // 卫星模块的串口数据（确保与 GPS 模块的波特率匹配） 

  // 初始化 OLED 显示屏
  if (!screen.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);  // 一个用于阻止程序进一步执行的无限循环
  }

  // 显示屏初始化设置
  screen.display();  // 显示初始化的内容
  delay(2000);        // 延迟2秒
  screen.clearDisplay(); // 清除显示屏
  screen.setTextSize(1);         // 文字大小（1到5）
  screen.setTextColor(SSD1306_WHITE); // 文字颜色（SSD1306_WHITE 或 SSD1306_BLACK）
  screen.setCursor(0, 0);         // 设置光标起始位置
  screen.println(F("GPS Data Parsing"));  // 显示初始文本
  screen.display();

  // 初始化NMEA读取头 | Initiate NMEA Reader
  NMEAParser_Init(&info);  // 初始化NMEA解析器 | Initialize NMEA Parser
}

void convert_time(const char* input, char* output, int hour_offset) {
  int hours = (input[0] - '0') * 10 + (input[1] - '0'); // 解析小时
  int minutes = (input[2] - '0') * 10 + (input[3] - '0'); // 解析分钟
  int seconds = (input[4] - '0') * 10 + (input[5] - '0'); // 解析秒
  
  // 对小时进行偏置处理
  hours = (hours + hour_offset) % 24;
  if (hours < 0) {
    hours += 24;
  }

  // 格式化为"hh:mm:ss"并写入output
  sprintf(output, "%02d:%02d:%02d", hours, minutes, seconds);
}


void loop() {
  // 检查Serial2中是否有数据
  if (Serial2.available()) {
    // 从Serial2中读取并解析数据
    // 如果你将你的GNSS模块接入了Serial2，则读取这个地方。 | Read the serial that same to your serial that connected to your GNSS module.
    NMEAParser_ReadFromSerial(&info, Serial2);

    // 清屏
    screen.clearDisplay();
    screen.setCursor(0, 0);

    // 显示经纬度和高程数据 | Show Latitude, Longitude and Altitude
    screen.print("Lat:");
    screen.print(info.latitude, 6);  // 显示纬度，保留6位小数
    screen.print(" ");
    screen.println(info.lat_dir);    // 显示纬度方向 | Show direction of Latitude

    screen.print("Lon:");
    screen.print(info.longitude, 6); // 显示经度，保留6位小数
    screen.print(" ");
    screen.println(info.lon_dir);    // 显示经度方向 | Show direction of Longitude

    screen.print("Alt:");
    screen.print(info.altitude);     // 显示高度 | Show Altitude
    screen.println(" m");            // 显示单位

    screen.print("Sat. nums: ");
    screen.println(info.num_satellites);

    screen.print("Pos. mode: "); // 显示定位模式 | Positioning Mode
    if (info.pos_mode == 'A') {
      screen.println("Auto");  // 'A' 表示自动模式，GPS可以自主定位
    } else if (info.pos_mode == 'E') {
      screen.println("Estimate");  // 'E' 表示估算模式，GPS可能未得到精确定位
    } else if (info.pos_mode == 'N') {
      screen.println("N/A");  // 'N' 表示数据无效，未获得有效定位
    } else if (info.pos_mode == 'D') {
      screen.println("Difference");  // 'D' 表示差分定位模式，使用额外参考信号进行修正
    } else if (info.pos_mode == 'M') {
      screen.println("Input");  // 'M' 表示手动输入模式，GPS可能依赖外部数据
    }
    
    screen.print("Fix quality: "); // 显示固定模式 | Fixing Mode
    // 根据NMEA-0183中语句的GPS fix质量值显示定位质量信息
    if (info.fix_quality == 0) {
      screen.println("N/A"); // Fix质量为0，表示无有效定位
    } else if (info.fix_quality == 1) {
      screen.println("SPS"); // Fix质量为1，表示标准位置服务（SPS），通常指基本的GPS定位
    } else if (info.fix_quality == 2) {
      screen.println("D-SPS"); // Fix质量为2，表示差分GPS定位（D-SPS），通过地面站修正
    } else if (info.fix_quality == 3) {
      screen.println("PPS"); // Fix质量为3，表示精密位置服务（PPS），通常由军方使用
    } else if (info.fix_quality == 4) {
      screen.println("RTK-Fix"); // Fix质量为4，表示实时动态定位（RTK），是一种高精度的定位技术
    } else if (info.fix_quality == 5) {
      screen.println("RTK-Float"); // Fix质量为5，表示浮动RTK，精度较RTK固定解稍低
    } else if (info.fix_quality == 6) {
      screen.println("Estim"); // Fix质量为6，表示估算位置，没有得到精确信号
    
    screen.print("Time: ");
    // screen.println(info.utc_time);
    char formatted_time[9];  // 8个位置+1个休止符
    convert_time(info.utc_time, formatted_time, TIMEZONE);  // 这个函数在上方有定义，用于将时间转为HH:MM:SS格式，并加入当地时区偏置值。 | This func had been defined above to transfer time format string to "HH:MM:SS" and add a bias for your timezone.
    screen.println(formatted_time);
    
    // 更新显示内容 | Refresh frame in my screen
    screen.display();
      
  }
}

```
