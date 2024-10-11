#include "NMEAParser.h"
#include <string.h>
#include <stdlib.h>

#include <Arduino.h>
#include <HardwareSerial.h>

// 私有变量
static char nmea_buffer[NMEA_MAX_LENGTH];  // 增加缓冲区大小到 512 字节
static int buffer_index = 0;   // 当前缓冲区索引
static bool message_complete = false;  // 报文是否完整

// 初始化解析器
void NMEAParser_Init(NMEAInfo *info) {
    memset(info, 0, sizeof(NMEAInfo));     // 清空所有信息
    memset(nmea_buffer, 0, sizeof(nmea_buffer));  // 清空缓冲区
    buffer_index = 0;
    message_complete = false;
    //Serial.println("Initiation completed.");
}

// 处理一个新的字符，并返回是否完成了一个完整的报文
// 检测到完整报文后，对校验码进行验证
bool NMEAParser_ProcessChar(NMEAInfo *info, char c, HardwareSerial &serial_port) {
    // 检测缓冲区是否溢出
    if (buffer_index >= sizeof(nmea_buffer) - 1) {
        // Serial.println("Buffer overflow, discarding current message.");
        buffer_index = 0;  // 防止溢出，丢弃当前消息并重置索引
        memset(info, 0, sizeof(NMEAInfo));  // 重置当前消息内的一切
        return false;
    }

    // 只处理 ASCII 可打印字符，排除控制字符
    if (c >= 32 && c <= 126) {
        nmea_buffer[buffer_index] = c;
        buffer_index++;  // 自增索引
    }

    // 检测星号，提取校验码
    if (c == '*') {
        nmea_buffer[buffer_index] = '\0';  // 添加字符串终止符，保证缓冲区内容是有效字符串

        // 确保后面有足够的字符来存储校验码
        char next_char_1 = serial_port.read();
        char next_char_2 = serial_port.read();
        
        if (isxdigit(next_char_1) && isxdigit(next_char_2)) {
            // 读取并解析校验码
            char received_checksum_str[3] = { next_char_1, next_char_2, '\0' };
            unsigned int received_checksum = 0;
            sscanf(received_checksum_str, "%2X", &received_checksum);  // 提取校验码（两位十六进制）

            // 计算报文内容的校验码（从 $ 开始到 * 之前）
            unsigned int calculated_checksum = 0;
            for (char *p = nmea_buffer + 1; *p != '*'; ++p) {
                calculated_checksum ^= *p;
            }

            // 校验比较
            if (calculated_checksum == received_checksum) {
                //Serial.println("Checksum valid, complete NMEA sentence received.");
                message_complete = true;  // 报文完成标志
                buffer_index = 0;  // 重置索引
                //Serial.println(nmea_buffer);  // 已确认这里的nmea_buffer储存了完整的消息

                return true;  // 返回 true 表示报文已完成且校验通过
            } else {
                //Serial.println("Invalid checksum, discarding current message.");
                buffer_index = 0;  // 重置索引
                memset(nmea_buffer, 0, sizeof(nmea_buffer));  // 清空缓冲区
            }
        } else {
            //Serial.println("Invalid checksum format, discarding current message.");
            buffer_index = 0;  // 重置索引
            memset(nmea_buffer, 0, sizeof(nmea_buffer));  // 清空缓冲区
        }
    }

    return false;  // 尚未接收到完整的报文
}

// 从指定串口读取数据，并进行实时解析
void NMEAParser_ReadFromSerial(NMEAInfo *info, HardwareSerial &serial_port) {
    while (serial_port.available()) {
        char c = serial_port.read();
        //Serial.print(c);  // 正常
        
        NMEAParser_ProcessChar(info, c, serial_port);

        if (NMEAParser_IsComplete(info)) {
            NMEAParser_Parse(info, nmea_buffer);
        }
    }
}

// 检查是否已解析到一个完整报文
bool NMEAParser_IsComplete(NMEAInfo *info) {
    return message_complete;
}

char** NMEAParser_Split(char* buffer, char remove_sign, char split_sign) {
    static char tokens[MAX_TOKENS][MAX_TOKEN_LENGTH]; // 静态二维数组存储每个分割片段
    static char* token_pointers[MAX_TOKENS + 1]; // 指针数组，用于返回结果

    // 清空所有分割片段
    for (int i = 0; i < MAX_TOKENS; i++) {
        tokens[i][0] = '\0'; // 初始化为结束符，表示为空
        token_pointers[i] = tokens[i]; // 指向每个片段的指针
    }
    token_pointers[MAX_TOKENS] = NULL; // 最后一个为NULL，表示结束

    int token_index = 0;  // 当前处理片段的索引
    int char_index = 0;   // 当前处理字符在片段中的索引

    // 遍历输入的缓冲区字符串
    for (int i = 0; buffer[i] != '\0'; i++) {
        char c = buffer[i];

        // 如果当前字符是要移除的符号（如 '*')，则跳过
        if (c == remove_sign) {
            continue;
        }

        // 如果当前字符是分割符号（如逗号），则开始新的片段
        if (c == split_sign) {
            tokens[token_index][char_index] = '\0';  // 添加字符串结束符
            token_index++;  // 移动到下一个片段
            char_index = 0; // 重置字符索引

            // 如果片段数量超过了预设的最大数量，直接退出
            if (token_index >= MAX_TOKENS) {
                break;
            }
        } else {
            // 添加字符到当前片段中
            if (char_index < MAX_TOKEN_LENGTH - 1) {  // 防止溢出
                tokens[token_index][char_index++] = c;
            }
        }
    }

    // 处理最后一个片段
    tokens[token_index][char_index] = '\0';

    // 设置返回的指针数组
    for (int i = 0; i <= token_index; i++) {
        token_pointers[i] = tokens[i];
    }
    token_pointers[token_index + 1] = NULL;  // 最后一个设置为NULL表示结束

    return token_pointers;
}

// 解析NMEA报文并更新结构体
void NMEAParser_Parse(NMEAInfo *info, char *buffer) {
    // 所以，现在唯一的问题就是这个地方。
    char msg_type[4] = {buffer[3], buffer[4], buffer[5], '\0'};  // 强行读取头部
    
    // 将buffer分开，成为char**。
    char** token = NMEAParser_Split(buffer, '*', ',');

    // 检查是否分割成功
        // if (token == NULL) {
        //     Serial.println("Tokenization failed.");
        //     return;
        // }

    // 调试输出，逐个打印分割后的内容
        // int i = 0;
        // while (token[i] != NULL) {
        //     Serial.print(token[i]); Serial.print(";");
        //     i++;
        // }
        // Serial.println();

    // 根据类型，选择不同的解析逻辑
    if (strcmp(msg_type, "GGA") == 0) {
        info->type = NMEA_GGA;  // 设置标识符
        // 然后开始分析GGA内容
        for (int i = 0; i < 11; i++){  // 提取时间
            if (token[1][i] != NULL){
                info->utc_time[i] = token[1][i];
            } else {
                break;
            }
        }

        // 经纬度和高程，还有单位
        info->latitude = (atof(token[2]) / 100.0f) + fmod(atof(token[2]), 100.00f) / 60 + fmod(atof(token[2]), 1.0f) / 600000;
        info->lat_dir = token[3][0];
        info->longitude = (atof(token[4]) / 100.0f) + fmod(atof(token[4]), 100.00f) / 60 + fmod(atof(token[4]), 1.0f) / 600000;
        info->lon_dir = token[5][0];
        info->altitude = atof(token[9]);
        info->alt_dir = token[10][0];

        // 简单的卫星参数内容
        info->fix_quality = atoi(token[6]);  // 固定质量
        info->num_satellites = atoi(token[7]);  // 卫星数目

    } else if (strcmp(msg_type, "RMC") == 0) {
        info->type = NMEA_RMC;

        for (int i = 0; i < 11; i++){  // 提取时间
            if (token[1][i] != NULL){
                info->utc_time[i] = token[1][i];
            } else {
                break;
            }
        }
        // 经纬度和单位
        info->latitude = atof(token[3]) / 100.00f + fmod(atof(token[3]), 100.00f) / 60 + fmod(atof(token[3]), 1.0f) / 600000;
        info->lat_dir = token[4][0];
        info->longitude = atof(token[5]) / 100.0f + fmod(atof(token[5]), 100.00f) / 60 + fmod(atof(token[5]), 1.0f) / 600000;
        info->lon_dir = token[6][0];
        
        // 速度和方向
        info->speed = atof(token[7]);
        info->course = atof(token[8]);

        // 日期
        for (int j = 0; j < 7; j++){
            if (token[9][j] != NULL) {
                info->date[j] = token[9][j];
            } else{
                break;
            }
        };
        
        // 定位模式
        info->pos_mode = token[12][0];

    } else if (strcmp(msg_type, "GLL") == 0) {
        info->type = NMEA_GLL;
        info->latitude = atof(token[1]);
        info->lat_dir = token[2][0];
        info->longitude = atof(token[3]);
        info->lon_dir = token[4][0];
        strncpy(info->utc_time, token[5], sizeof(info->utc_time) - 1);
        info->utc_time[sizeof(info->utc_time) - 1] = '\0';


    } else if (strcmp(msg_type, "GSA") == 0) {
        info->type = NMEA_GSA;
        info->pdop = atof(token[15]);
        info->hdop = atof(token[16]);
        info->vdop = atof(token[17]);

    } else if (strcmp(msg_type, "GSV") == 0) {
        info->type = NMEA_GSV;

        // 解析报文头部信息
        info->max_page = atoi(token[1]);      // 总页数
        info->now_page = atoi(token[2]);      // 当前页码
        info->GSV_num_satellites = atoi(token[3]);  // 可见卫星总数

        // 如果是第一页，清空卫星信息
        if (info->now_page == 1) {
            memset(info->satellite_info, 0, sizeof(info->satellite_info));
        }

        // 每页最多包含4颗卫星，计算当前页的卫星数量
        int satellites_in_this_page = (info->GSV_num_satellites > 4 * info->now_page) ? 4 : info->num_satellites % 4;
        if (satellites_in_this_page == 0 && info->GSV_num_satellites >= 4) {
            satellites_in_this_page = 4;  // 如果整除，意味着这一页有4颗卫星
        }

        // 遍历每个卫星的信息
        for (int i = 0; i < satellites_in_this_page; i++) {
            int base_index = 4 + i * 4;  // 每个卫星的基准索引（从token[4]开始，每4个为一组）
            int satellite_index = (info->now_page - 1) * 4 + i;  // 在satellite_info中的索引

            if (satellite_index < MAX_SATELLITES) {  // 防止越界
                info->satellite_info[satellite_index].prn = atoi(token[base_index]);            // 卫星编号 PRN
                info->satellite_info[satellite_index].elevation = atoi(token[base_index + 1]);  // 仰角
                info->satellite_info[satellite_index].azimuth = atoi(token[base_index + 2]);    // 方位角
                info->satellite_info[satellite_index].snr = atoi(token[base_index + 3]);        // 信噪比
            }
        }
        
    } else if (strcmp(msg_type, "VTG") == 0) {
        info->type = NMEA_VTG;
        info->course = atof(token[1]);
        info->speed = atof(token[7]);

    } else if (strcmp(msg_type, "ZDA") == 0) {
        info->type = NMEA_ZDA;
        strncpy(info->utc_time, token[1], sizeof(info->utc_time) - 1);
        info->utc_time[sizeof(info->utc_time) - 1] = '\0';

    } else if (strcmp(msg_type, "TXT") == 0) {
        info->type = NMEA_TXT;
        strncpy(info->txt_message, token[4], sizeof(info->txt_message) - 1);
        info->txt_message[sizeof(info->txt_message) - 1] = '\0';

    } else {
        info->type = NMEA_UNKNOWN;
    }

    message_complete = false;  // 重置报文完成标志
}
