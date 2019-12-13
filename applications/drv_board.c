/**
  * Copyright (c) 2019, Aprilhome
  * 板上外设配置
  * -功能模块开关
  * Change Logs:
  * Date           Author       Notes
  * 2019-04-20     Aprilhome    first version
**/

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>

#include "drv_board.h"

/**
  * @brief    开关初始化   
  * @param    void    
  * @retval   void   
  * @logs   
  * date        author        notes
  * 2019/9/3  Aprilhome
**/
void config_switch(void)
{
    rt_pin_mode(LED_R_PIN, PIN_MODE_OUTPUT);
    rt_pin_mode(LED_G_PIN, PIN_MODE_OUTPUT);
    rt_pin_mode(LED_B_PIN, PIN_MODE_OUTPUT);
    rt_pin_write(LED_R_PIN, PIN_HIGH);
    rt_pin_write(LED_G_PIN, PIN_HIGH);
    rt_pin_write(LED_B_PIN, PIN_HIGH);
    
    
    rt_pin_mode(EN_TEMP_PIN, PIN_MODE_OUTPUT); 
    rt_pin_mode(EN_HR_PIN, PIN_MODE_OUTPUT);
    rt_pin_mode(EN_SD_PIN, PIN_MODE_OUTPUT);
    rt_pin_mode(EN_4G_PIN, PIN_MODE_OUTPUT);
    rt_pin_mode(EN_GPS_PIN, PIN_MODE_OUTPUT);
    rt_pin_write(EN_TEMP_PIN, PIN_LOW);
    rt_pin_write(EN_HR_PIN, PIN_LOW);
    rt_pin_write(EN_SD_PIN, PIN_LOW);
    rt_pin_write(EN_4G_PIN, PIN_LOW);
    rt_pin_write(EN_GPS_PIN, PIN_LOW);
    
    rt_pin_mode(EN_PC_T_PIN, PIN_MODE_OUTPUT); 
    rt_pin_mode(EN_FINSH_T_PIN, PIN_MODE_OUTPUT);
    rt_pin_mode(EN_4G_T_PIN, PIN_MODE_OUTPUT);   
    rt_pin_write(EN_PC_T_PIN, PIN_LOW);
    rt_pin_write(EN_FINSH_T_PIN, PIN_LOW);
    rt_pin_write(EN_4G_T_PIN, PIN_LOW);    
}

