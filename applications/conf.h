#ifndef __CONF_H__
#define __CONF_H__

#include "stm32l4xx_hal.h"

//
#define VERSION                "1.0"
#define HARDWARE_VERSION       "1.0.0"
#define SOFTWARE_VERSION       "1.0.18"

typedef unsigned char uchar;
typedef unsigned int  uint;
typedef unsigned long ulong;
//۪֨ӥjjӇǷ٤طģʽ
#define MODE_SLEEP  0
#define MODE_FAST   1
#define MODE_AUTO   2
#define MODE_SM     3
//共计120字节
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
  //配置 16字节
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
  uint16_t interval;
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
#define SYSINFO_LEN 256
#define SYSINFO  (*(sys_info_t *)(640* 1024))


#define ERR_CMD         (-1)
#define ERR_PARA        (-2)
#define ERR_MODE        (-3)
#define ERR_FLASH       (-4)
#define ERR_GC_DATA     (-5)
#define ERR_PROCESS     (-6)


#define SENSOR_CTD  1
#define SENSOR_TD   2
#define SENSOR_CT   3

//typedef enum
//{
//    PRESSER_BASIC,
//    PRESSER_HIGH_PRECISION,
//    PRESSER_KELLER,            
//}pressure_e;

#define PRESSURE_BASIC           1
#define PRESSURE_HIGH_PRECISION  2
#define PRESSURE_KELLER          3


/*串口*/
typedef enum
{
    STATE_IDLE    = 0, 
    STATE_RECEIVE = 1, 
    STATE_SUSPEND = 2,
}uart_receive_flag_e;

#define RECEIVE_LEN             200
#define RECEIVE_ARGC            20
typedef struct 
{
    char buff[RECEIVE_LEN];
    rt_uint16_t len;
    char *argv[RECEIVE_ARGC];
    rt_uint8_t argc;
    uart_receive_flag_e flag;
}uart_receive_t;

struct serial_data
{  
    char *serial_data_name; // 指令名字符串  
    void (*serial_data_function)(char *argv1, char *argv2, char *argv3, 
                                 char *argv4, char *argv5, char *argv6); // 指向该指令处理函数的指针
};

struct serial_data_long
{
    char *serial_data_name; // 指令名字符串  
    void (*serial_data_function)(void);
};







int config_sysinfo(sys_info_t *info);

#endif