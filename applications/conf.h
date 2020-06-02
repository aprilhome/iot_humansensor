#ifndef __CONF_H__
#define __CONF_H__

#include "stm32l4xx_hal.h"

typedef unsigned char uchar;
typedef unsigned int  uint;
typedef unsigned long ulong;

#define VERSION                "1.0"
#define HARDWARE_VERSION       "1.0.0"
#define SOFTWARE_VERSION       "1.0.0"

/* 系统运行状态 */
#define MODE_SLEEP      0   // 休眠，低功耗
#define MODE_NORMAL     1   // 正常，各个模块均打开
#define MODE_AUTO       2   // 自动，可按设定间隔自动向上位机发送采集数据
#define MODE_SM         3   // 自容，可按设定间隔自动采集数据并存储

/* 错误码 */
#define ERR_OK          (0)
#define ERR_CMD         (-1)    //命令输入错误
#define ERR_PARA        (-2)    //命令参数输入错误
#define ERR_MODE        (-3)    //仅在待机模式下可执行设置类命令
#define ERR_FLASH       (-4)    //FLASH读写错误
#define ERR_SD          (-8)    //SD卡读写错误
#define ERR_RTC         (-9)    //RTC读写错误
#define ERR_HR     (-5)
#define ERR_PROCESS     (-6)    //程序执行错误
#define ERR_GPS         (-7)


/* 传感器类型 */
#define SENSOR_CTD  1
#define SENSOR_TD   2
#define SENSOR_CT   3
#define PRESSURE_BASIC           1
#define PRESSURE_HIGH_PRECISION  2
#define PRESSURE_KELLER          3

/* 事件集 */
//温度接收事件
#define EVENT_TEMP_RECV      (1<<0)
#define EVENT_GPS_RECV       (1<<1)

#define SD_FILE_NAME_LEN      256

/* 系统状态信息，共计124字节 */
#define SYSINFO_LEN 256
#define SYSINFO  (*(sys_info_t *)(640* 1024)) //!!!注意!!!若修改了fal_cfg.h，需注意该宏
typedef struct
{
  //更新状态 4字节
  uint32_t upgrade;     
  //id 12个模块，共24字节
  uint16_t id;
  uint16_t id_core;
  uint16_t id_acq;
  uint16_t id_pre;
  uint16_t id_con;
  uint16_t id_motherboard;
  uint16_t id_thermistor;
  uint16_t id_pressure;
  uint16_t id_motor;
  uint16_t id_module;
  uint16_t id_conductivity;
  uint16_t id_unused;
  //波特率 12字节
  uint32_t bound;
  uint32_t bound_acq;
  uint32_t bound_pre;
  //配置 20字节
  uint8_t mode;
  uint8_t raw;
  uint8_t save;
  uint8_t pressure;
  int8_t  motor;
  int8_t motor_cur;
  uint8_t sensor;
  uint8_t output;                 
  uint8_t format; 
  uint8_t unit;
  uint16_t interval_1;
  float interval;
  float fre;
  //帧计数 8字节
  uint64_t gathertimes;
  //年月日 4字节
  uint8_t year;
  uint8_t month;
  uint8_t day;
  uint8_t unused;
  //系数 52字节
  float t_coef0;
  float t_coef1;
  float t_coef2;
  float t_coef3;
  float t_coef4;
  float p_coef0;
  float p_coef1;
  float p_coef2;
  float c_coef0;
  float c_coef1;
  float c_coef2;
  float c_coef3;
  float c_coef4;
}sys_info_t;

/* 执行 */
struct uart_execute
{  
    char *uart_execute_cmd; // 指令名字符串  
    void (*uart_execute_function)(char *argv1, char *argv2, char *argv3, 
                                  char *argv4, char *argv5, char *argv6); // 指向该指令处理函数的指针
};

struct uart_execute_long
{
    char *uart_execute_cmd; // 指令名字符串  
    void (*uart_execute_function)(void);
};

///* GPS数据 */
//typedef struct 
//{
//    char buff[128];
//    rt_uint8_t len;
//    rt_uint8_t  hour;
//    rt_uint8_t  minute;
//    rt_uint8_t  second;  
//    rt_uint8_t  status;
//    rt_uint8_t  latitude_dir;
//    float       latitude;
//    rt_uint8_t  lonitude_dir;
//    float       lonitude;
//    rt_uint16_t lonitude_minute;
//    rt_uint8_t  year;
//    rt_uint8_t  month;
//    rt_uint8_t  date;    
//}gps_data_t;

extern sys_info_t g_sysinfo;
extern rt_event_t  g_sample_event;

int config_sysinfo(sys_info_t *info);

#endif