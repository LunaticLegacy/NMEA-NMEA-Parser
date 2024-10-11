#ifndef NMEA_PARSER_H
#define NMEA_PARSER_H

#include <Arduino.h>

// 定义常量
#define NMEA_MAX_LENGTH 512    // 每个NMEA报文的最大长度
#define NMEA_MAX_FIELDS 20     // 每个NMEA报文的最大字段数量
#define MAX_TOKENS 20          // 分割后的最大片段数
#define MAX_TOKEN_LENGTH 50    // 每个片段的最大长度

// 定义GSV中最大可存储卫星数目
#define MAX_SATELLITES 32


// 定义报文类型
typedef enum {
    NMEA_UNKNOWN = 0,
    NMEA_GGA,  // 1
    NMEA_RMC,  // 2
    NMEA_GLL,  // 3
    NMEA_GSA,  // 4
    NMEA_GSV,  // 5
    NMEA_VTG,  // 6
    NMEA_ZDA,  // 7
    NMEA_TXT,  // 8
} NMEAType;

// 定义卫星信息结构体，来自GSV报文
typedef struct{
    int prn;  // 卫星编号
    int elevation;  // 仰角
    int azimuth;  // 方位角
    int snr;  // 信噪比
} SatelliteInfo;

// 定义NMEA报文解析结果结构体
typedef struct {
    NMEAType type;             // 当前报文类型
    char utc_time[11];         // UTC时间
    float latitude;            // 纬度 (已转换为十进制度)
    char lat_dir;              // 纬度方向 (N/S)
    float longitude;           // 经度 (已转换为十进制度)
    char lon_dir;              // 经度方向 (E/W)
    int fix_quality;           // GGA: 定位质量
    int num_satellites;        // GGA: 卫星数量
    float altitude;            // GGA: 海拔高度
    char alt_dir;              // GGA: 海拔高度方向 (M/F)
    float speed;               // RMC/VTG: 地面速度
    float course;              // RMC/VTG: 航向
    char pos_mode;             // RMC：定位模式
    float pdop;                // GSA: 精度因子
    float hdop;                // GSA: 水平精度因子
    float vdop;                // GSA: 垂直精度因子
    int active_satellites[12]; // GSA: 活跃卫星 PRN 列表
    int GSV_num_satellites;        // GSV：卫星数目
    SatelliteInfo satellite_info[MAX_SATELLITES];  // GSV：卫星列表，卫星数量可能不定
    int now_page;                  // GSV：当前页码
    int max_page;              // GSV：总页码
    char txt_message[80];      // GPTXT: 文本消息
    char date[6];      // ZDA: 日期
} NMEAInfo;

// 辅助函数：将一个char*中的特定符号去除，随后按照区分进行分块。
char** NMEAParser_Split(char *buffer, char remove_sign, char split_sign);

// 初始化解析器
void NMEAParser_Init(NMEAInfo *info);

// 处理一个新的字符，返回是否完成了一个完整的报文
bool NMEAParser_ProcessChar(NMEAInfo *info, char c, HardwareSerial &serial_port);

// 从指定串口读取数据，并进行实时解析
void NMEAParser_ReadFromSerial(NMEAInfo *info, HardwareSerial &serial_port);

// 检查是否已解析到一个完整报文
bool NMEAParser_IsComplete(NMEAInfo *info);

// 解析 NMEA 报文的函数，解析传入的缓冲区并更新结构体
void NMEAParser_Parse(NMEAInfo *info, char *buffer);

#endif  // NMEA_PARSER_H
