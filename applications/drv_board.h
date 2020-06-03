#ifndef __DRV_BOARD_H__
#define __DRV_BOARD_H__

/************ 引脚宏 ************/
//控制核心模块开关引脚
#define LED_R_PIN               GET_PIN(C, 6)
#define LED_G_PIN               GET_PIN(B, 0)
#define LED_B_PIN               GET_PIN(C, 7)
#define EN_232_UART24_T_PIN     GET_PIN(C, 3)
#define EN_485_UART3_T_PIN      GET_PIN(B, 1)
#define EN_485_LPUART1_T_PIN    GET_PIN(B, 2)
#define EN_SD_PIN               GET_PIN(C, 13)
//扩展开关引脚
#define EN_TEMP_PIN             GET_PIN(B, 4)
#define EN_HR_PIN               GET_PIN(B, 9)
#define EN_4G_PIN               GET_PIN(A, 12)
#define EN_GPS_PIN              GET_PIN(B, 8)
#define EN_232_UART15_T_PIN     GET_PIN(A, 11)

/************ IO开关宏  1开；0关 ************/
#define LED_R(n)        (n?rt_pin_write(LED_R_PIN, PIN_LOW):rt_pin_write(LED_R_PIN, PIN_HIGH))
#define LED_G(n)        (n?rt_pin_write(LED_G_PIN, PIN_LOW):rt_pin_write(LED_G_PIN, PIN_HIGH))
#define LED_B(n)        (n?rt_pin_write(LED_B_PIN, PIN_LOW):rt_pin_write(LED_B_PIN, PIN_HIGH))
#define EN_FINSH_T(n)   (n?rt_pin_write(EN_232_UART24_T_PIN, PIN_HIGH):rt_pin_write(EN_232_UART24_T_PIN, PIN_LOW))
#define EN_SD(n)        (n?rt_pin_write(EN_SD_PIN, PIN_HIGH):rt_pin_write(EN_SD_PIN, PIN_LOW))

#define EN_HR(n)        (n?rt_pin_write(EN_HR_PIN, PIN_HIGH):rt_pin_write(EN_HR_PIN, PIN_LOW))
#define EN_TEMP(n)      (n?rt_pin_write(EN_TEMP_PIN, PIN_HIGH):rt_pin_write(EN_TEMP_PIN, PIN_LOW))
#define EN_4G(n)        (n?rt_pin_write(EN_4G_PIN, PIN_HIGH):rt_pin_write(EN_4G_PIN, PIN_LOW))
#define EN_GPS(n)       (n?rt_pin_write(EN_GPS_PIN, PIN_HIGH):rt_pin_write(EN_GPS_PIN, PIN_LOW))
#define EN_PC_4G_GPS_T(n)   (n?rt_pin_write(EN_232_UART15_T_PIN, PIN_HIGH):rt_pin_write(EN_232_UART15_T_PIN, PIN_LOW))
#define EN_PC_T(n)      (n?rt_pin_write(EN_232_UART15_T_PIN, PIN_HIGH):rt_pin_write(EN_232_UART15_T_PIN, PIN_LOW))//以下3个与上同，便于调用
#define EN_4G_T(n)      (n?rt_pin_write(EN_232_UART15_T_PIN, PIN_HIGH):rt_pin_write(EN_232_UART15_T_PIN, PIN_LOW))
#define EN_GPS_T(n)     (n?rt_pin_write(EN_232_UART15_T_PIN, PIN_HIGH):rt_pin_write(EN_232_UART15_T_PIN, PIN_LOW))

/************ 外设宏 ************/
#define PC_UART                 LPUART1
#define HR_UART                 USART1
#define HWTIMER_DEV_NAME        "timer15" /* 定时器名称 */

/************ 串口 ************/
typedef enum
{
    STATE_IDLE    = 0, 
    STATE_RECEIVE = 1, 
    STATE_SUSPEND = 2,
}uart_receive_flag_e;

/* 接收 */
#define MAX_RECEIVE_LEN_PM      200
#define MAX_RECEIVE_ARGC_PM     20

#define MAX_RECEIVE_LEN_GPS     128
#define MAX_RECEIVE_ARGC_GPS    20

#define MAX_RECEIVE_LEN_HR    512
#define MAX_RECEIVE_ARGC_HR   128

#define RECEIVE_LEN             512
#define RECEIVE_ARGC            20
typedef struct 
{
    char buff[RECEIVE_LEN];
    rt_uint16_t len;
    char *argv[RECEIVE_ARGC];
    rt_uint8_t argc;
    uart_receive_flag_e flag;
}uart_receive_t;

/* 发送 */
#define MAX_SEND_LEN_PM           200


#define MAX_SEND_LEN_U1           200
#define MAX_SEND_LEN_U2           200
#define MAX_SEND_LEN_U3           200
#define MAX_SEND_LEN_U4           200
#define MAX_SEND_LEN_U5           200
#define MAX_SEND_LEN_LPU1         200
void gc_printf (char *fmt, ...);
void pm_printf (char *fmt, ...);
void hr_printf (char *fmt, ...);
void u1_printf (char *fmt, ...);
void u2_printf (char *fmt, ...);
void u3_printf (char *fmt, ...);
void u4_printf (char *fmt, ...);
void u5_printf (char *fmt, ...);
void lpu1_printf (char *fmt, ...);


/* 函数声明 */
void init_switch(void);
void config_pm_t(uint8_t a);

#endif