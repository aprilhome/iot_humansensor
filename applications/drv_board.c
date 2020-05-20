/**
  * Copyright (c) 2019, Aprilhome
  * 板上外设配置
    -功能模块开关GPIO初始化
    -创建块设备sd，并初始化
    -重定向各个串口printf
  * Change Logs:
  * Date           Author       Notes
  * 2019-04-20     Aprilhome    first version
  * 2020-04-15     Aprilhome    buoy_clt_v1.0
**/

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>

#include <stdio.h>
//#include <string.h>
//#include <stdlib.h>      
//#include <stdarg.h>
//#include <math.h>

#include "drv_spi.h"
#include "spi_msd.h"

#include "drv_board.h"


/**
  * @brief    各开关引脚初始化
              RGB灯，温度/finsh控制台的232发送使能，sd电源
              4G模块电源，GPS模块电源，4G/GPS/上位机的232发送使能
  * @param    void    
  * @retval   void   
  * @logs   
  * date        author        notes
  * 2019/9/3  Aprilhome
**/
void init_switch(void)
{
    rt_pin_mode(LED_R_PIN, PIN_MODE_OUTPUT);
    rt_pin_mode(LED_G_PIN, PIN_MODE_OUTPUT);
    rt_pin_mode(LED_B_PIN, PIN_MODE_OUTPUT);
    rt_pin_mode(EN_232_UART24_T_PIN, PIN_MODE_OUTPUT);
    rt_pin_mode(EN_485_LPUART1_T_PIN, PIN_MODE_OUTPUT);
    rt_pin_mode(EN_SD_PIN, PIN_MODE_OUTPUT);
    rt_pin_write(LED_R_PIN, PIN_HIGH);
    rt_pin_write(LED_G_PIN, PIN_HIGH);
    rt_pin_write(LED_B_PIN, PIN_HIGH);
    rt_pin_write(EN_232_UART24_T_PIN, PIN_HIGH);
    rt_pin_write(EN_485_LPUART1_T_PIN, PIN_LOW);
    rt_pin_write(EN_SD_PIN, PIN_HIGH);    
    
    rt_pin_mode(EN_TEMP_PIN, PIN_MODE_OUTPUT); 
    rt_pin_mode(EN_GPS_PIN, PIN_MODE_OUTPUT);
    rt_pin_mode(EN_HR_PIN, PIN_MODE_OUTPUT);
    rt_pin_write(EN_TEMP_PIN, PIN_LOW);
    rt_pin_write(EN_GPS_PIN, PIN_LOW);
    rt_pin_write(EN_HR_PIN, PIN_LOW);    
}

void config_pm_t(uint8_t a)
{
    if (a == 0)
    {
        rt_thread_mdelay(2);
        rt_pin_write(EN_485_LPUART1_T_PIN, PIN_LOW);
    }
    else
    {
        rt_pin_write(EN_485_LPUART1_T_PIN, PIN_HIGH);
        rt_thread_mdelay(2);
    }   
}


/**
  * @brief    基于spi1总线上的第一个spi设备spi10，创建块设备sd0，直接在开始初始化    
  * @param        
  * @retval      
  * @logs   
  * date        author        notes
  * 2020/4/17  Aprilhome
**/
static int rt_hw_spi1_tfcard(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    rt_hw_spi_device_attach("spi1", "spi10", GPIOA, GPIO_PIN_4);
    return msd_init("sd0", "spi10");
}
INIT_COMPONENT_EXPORT(rt_hw_spi1_tfcard);

/****************
 *printf重定向函数*
 ****************/
/**
 * @function    重定向fput函数，在UART5中使用printf函数
 * @para1       ch:输出字符
 * @para2       f:文件指针
 * @return      
 * @created     Aprilhome,2019/8/25 
**/
int fputc (int ch, FILE *f)
{
    while((UART5->ISR & 0x40) == 0); //循环发送，直到发送完毕
    UART5->TDR = (unsigned char) ch;
    return ch;
}

void pm_printf (char *fmt, ...)
{
    char bufffer[MAX_SEND_LEN_PM + 1] = {0};  // CMD_BUFFER_LEN长度自己定义吧
    unsigned int i = 0;
    unsigned int len = 0;
    
    va_list arg_ptr;
    va_start(arg_ptr, fmt);  
    len = vsnprintf(bufffer, MAX_SEND_LEN_PM + 1, fmt, arg_ptr);
    while ((i < MAX_SEND_LEN_PM) && (i < len) && (len > 0))
    {
        while((PC_UART->ISR & 0x40) == 0); //循环发送，直到发送完毕
        PC_UART->TDR = bufffer[i++];
    }
    va_end(arg_ptr);
}

void u1_printf (char *fmt, ...)
{
    char bufffer[MAX_SEND_LEN_U1 + 1] = {0};  // CMD_BUFFER_LEN长度自己定义吧
    unsigned int i = 0;
    unsigned int len = 0;
    
    va_list arg_ptr;
    va_start(arg_ptr, fmt);  
    len = vsnprintf(bufffer, MAX_SEND_LEN_U1 + 1, fmt, arg_ptr);
    while ((i < MAX_SEND_LEN_U1) && (i < len) && (len > 0))
    {
        while((USART1->ISR & 0x40) == 0); //循环发送，直到发送完毕
        USART1->TDR = bufffer[i++];
    }
    va_end(arg_ptr);
}

void u2_printf (char *fmt, ...)
{
    char bufffer[MAX_SEND_LEN_U2 + 1] = {0};  // CMD_BUFFER_LEN长度自己定义吧
    unsigned int i = 0;
    unsigned int len = 0;
    
    va_list arg_ptr;
    va_start(arg_ptr, fmt);  
    len = vsnprintf(bufffer, MAX_SEND_LEN_U2 + 1, fmt, arg_ptr);
    while ((i < MAX_SEND_LEN_U2) && (i < len) && (len > 0))
    {
        while((USART2->ISR & 0x40) == 0); //循环发送，直到发送完毕
        USART2->TDR = bufffer[i++];
    }
    va_end(arg_ptr);
}

void u3_printf (char *fmt, ...)
{
    char bufffer[MAX_SEND_LEN_U3 + 1] = {0};  // CMD_BUFFER_LEN长度自己定义吧
    unsigned int i = 0;
    unsigned int len = 0;
    
    va_list arg_ptr;
    va_start(arg_ptr, fmt);  
    len = vsnprintf(bufffer, MAX_SEND_LEN_U3 + 1, fmt, arg_ptr);
    while ((i < MAX_SEND_LEN_U3) && (i < len) && (len > 0))
    {
        while((USART3->ISR & 0x40) == 0); //循环发送，直到发送完毕
        USART3->TDR = bufffer[i++];
    }
    va_end(arg_ptr);
}

void u4_printf (char *fmt, ...)
{
    char bufffer[MAX_SEND_LEN_U4 + 1] = {0};  // CMD_BUFFER_LEN长度自己定义吧
    unsigned int i = 0;
    unsigned int len = 0;
    
    va_list arg_ptr;
    va_start(arg_ptr, fmt);  
    len = vsnprintf(bufffer, MAX_SEND_LEN_U4 + 1, fmt, arg_ptr);
    while ((i < MAX_SEND_LEN_U4) && (i < len) && (len > 0))
    {
        while((UART4->ISR & 0x40) == 0); //循环发送，直到发送完毕
        UART4->TDR = bufffer[i++];
    }
    va_end(arg_ptr);
}

void u5_printf (char *fmt, ...)
{
    char bufffer[MAX_SEND_LEN_U5 + 1] = {0};  // CMD_BUFFER_LEN长度自己定义吧
    unsigned int i = 0;
    unsigned int len = 0;
    
    va_list arg_ptr;
    va_start(arg_ptr, fmt);  
    len = vsnprintf(bufffer, MAX_SEND_LEN_U5 + 1, fmt, arg_ptr);
    while ((i < MAX_SEND_LEN_U5) && (i < len) && (len > 0))
    {
        while((UART5->ISR & 0x40) == 0); //循环发送，直到发送完毕
        UART5->TDR = bufffer[i++];
    }
    va_end(arg_ptr);
}

void lpu1_printf (char *fmt, ...)
{
    char bufffer[MAX_SEND_LEN_LPU1 + 1] = {0};  // CMD_BUFFER_LEN长度自己定义吧
    unsigned int i = 0;
    unsigned int len = 0;
    
    va_list arg_ptr;
    va_start(arg_ptr, fmt);  
    len = vsnprintf(bufffer, MAX_SEND_LEN_LPU1 + 1, fmt, arg_ptr);
    while ((i < MAX_SEND_LEN_LPU1) && (i < len) && (len > 0))
    {
        while((LPUART1->ISR & 0x40) == 0); //循环发送，直到发送完毕
        LPUART1->TDR = bufffer[i++];
    }
    va_end(arg_ptr);
}