/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 * 
 * Change Logs:
 * Date           Author       Notes
 * 2018-11-06     SummerGift   first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>

#include "fal.h"

#include "drv_board.h"
#include "conf.h"
#include "comm_pc.h"
#include "drv_gps.h"

/* 系统运行相关变量 */
sys_info_t g_sysinfo = {0};
const struct fal_partition *g_sysinfo_partition = RT_NULL;

/* 函数 */
static void led_thread_entry(void *parameter);

int main(void)
{   
    /*flash*/
    fal_init();
    g_sysinfo_partition = fal_partition_find("sysinfo"); 
    g_sysinfo = SYSINFO;
    
    /*外设*/
    config_switch();
    config_uart_pc("uart2", 115200);
    config_uart_gps("uart1", 38400);
    /*初始化完成提示*/
    LED_R(0);
    pc_printf("$IoT human sensor\r\n");
    
    /*线程*/
    //线程：LED闪烁
    rt_thread_t led_thread = rt_thread_create("led",
                                              led_thread_entry,
                                              RT_NULL,
                                              256,
                                              30,
                                              10);
    if (led_thread != RT_NULL)
    {
        rt_thread_startup(led_thread);
    }
    else
    {
        return RT_ERROR;
    }
    
    //线程：上位机指令处理
    rt_thread_t pc_cmd_process_thread = rt_thread_create("pc_cmd_process",
                                                         pc_execute_thread_entry,
                                                         RT_NULL,
                                                         1024,
                                                         25,
                                                         10);
    if (pc_cmd_process_thread != RT_NULL)
    {
        rt_thread_startup(pc_cmd_process_thread);
    }
    else
    {
        return RT_ERROR;
    }
    
    //线程：GPS数据接收处理
    rt_thread_t gps_data_process_thread = rt_thread_create("gps_data_process",
                                                         gps_execute_thread_entry,
                                                         RT_NULL,
                                                         512,
                                                         26,
                                                         10);
    if (gps_data_process_thread != RT_NULL)
    {
        rt_thread_startup(gps_data_process_thread);
    }
    else
    {
        return RT_ERROR;
    }
    
    
    return RT_EOK;
}


/**
 * @function    led线程
 * @param       void *parameter
 * @return      void
 * @created     Aprilhome,2019/4/19 
**/
static void led_thread_entry(void *parameter)
{
    while (1)
    {
        LED_B(1);
        rt_thread_mdelay(500);
        LED_B(0);
        rt_thread_mdelay(500);
    }
}

/**
  * @brief    配置系统信息    
  * @param    sys_info_t *info 系统信息变量（已经通过全局变量更新）    
  * @retval   0:成功；-1：失败 
  * @logs   
  * date        author        notes
  * 2019/8/30  Aprilhome
**/
int config_sysinfo(sys_info_t *info)
{
    uchar ret = 0;
    uchar buf[SYSINFO_LEN] = {0};
    for (uint i = 0; i < SYSINFO_LEN; i++)
    {
        buf[i] = *(((uchar *)info) + i);
    }
    ret = fal_partition_erase_all(g_sysinfo_partition);
    if (ret != RT_EOK)
        return ret;
    ret = fal_partition_write(g_sysinfo_partition, 0, buf, sizeof(sys_info_t));
    return ret;
}
