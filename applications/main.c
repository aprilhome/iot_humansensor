/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 * 
 * Change Logs:
 * Date           Author       Notes
 * 2018-11-06     SummerGift   first version
 * 2019-12-18     Aprilhome    iot_humansensorv1.0
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>

#include <dfs_fs.h>   
#include <dfs_posix.h>
#include "fal.h"
#include "syswatch.h"
     
#include "drv_board.h"
#include "conf.h"
#include "api_pm.h"
#include "api_gps.h"
#include "api_hr.h"
#include "api_timer.h"
#include "drv_max30205.h"
#include "api_sd.h"
     
/* 系统状态信息及fal分区地址 */
sys_info_t g_sysinfo = {0};
const struct fal_partition *g_sysinfo_partition = RT_NULL;

rt_event_t  g_sample_event = RT_NULL;

/* 函数声明 */
static void led_thread_entry(void *parameter);
static void save_data_thread_entry(void *parameter);
static void deinit_sysinfo(void);

/* 全局变量 */
rt_event_t  g_save_data_event = RT_NULL;
struct rt_ringbuffer *g_sd_buff;
rt_mutex_t g_ringbuffer_mutex = RT_NULL;

#define APP_VERSION "2.0.0"

int main(void)
{   
    /* flash：fal初始化;读出系统信息 */
    fal_init();
    g_sysinfo_partition = fal_partition_find("sysinfo"); 
    g_sysinfo = SYSINFO;
    deinit_sysinfo();
    
    rt_kprintf("The current version of APP firmware is %s\n", APP_VERSION);
    
    /* 外设初始化 */
    init_switch();
    EN_GPS(1);
    EN_TEMP(1);
    EN_HR(1);
    EN_SD(1);
    EN_FINSH_T(1);

    init_uart_pm("lpuart1", SYSINFO.bound);
    init_uart_gps("uart4", 9600);
    init_uart_hr("uart1", 115200);
    init_timer("timer15");
    
    max30205_init();
        
    /* 挂载文件系统，将块设备sd0挂载在路径"/"(根路径)上 */
    if (dfs_mount("sd0", "/", "elm", 0, 0) == 0)
    {
        rt_kprintf("Filesystem initialized!");
    }
    else
    {
        rt_kprintf("Failed to initialize filesystem!");
    }
    
    /* 创建事件集 */
    g_sample_event = rt_event_create("temp_event", RT_IPC_FLAG_FIFO);
    /* 创建事件集 */
//    g_get_data_event = rt_event_create("get_data_event", RT_IPC_FLAG_FIFO);
    g_save_data_event = rt_event_create("save_data_event", RT_IPC_FLAG_FIFO);
    
    /* 创建环形缓冲区 */
    g_sd_buff = rt_ringbuffer_create(RINGBUFFERSIZE);    
    /* 创建一个动态互斥量 */
    g_ringbuffer_mutex = rt_mutex_create("dmutex", RT_IPC_FLAG_FIFO);
    
    /* 初始化完成提示 */
    config_pm_t(1);
    LED_R(1);
    pm_printf("$humansensor %d\r\n", g_sysinfo.id);
    /* 根据断电前工作模式，是否开启定时器 */
    switch (SYSINFO.mode)
    {
    case MODE_SLEEP:
        config_hwtimer(0,5);
        pm_printf("$mode %d\r\n", SYSINFO.mode);
        break;
    case MODE_NORMAL:
        config_hwtimer(0,5);
        pm_printf("$mode %d\r\n", SYSINFO.mode);
        break;
    case MODE_AUTO:
        config_hwtimer(1, SYSINFO.interval);
        pm_printf("$mode %d\r\n", SYSINFO.mode);
        break;
    default:
        pm_printf("$err %d\r\n", ERR_MODE);
        break;        
    }
    config_pm_t(0);
    
    syswatch_init();
    
    /* 线程 */
    //线程：LED闪烁
    rt_thread_t led_thread = rt_thread_create("led_thread",
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
    rt_thread_t pm_thread = rt_thread_create("pm_thread",
                                             pm_uart_execute_thread_entry,
                                             RT_NULL,
                                             4096,
                                             19,
                                             10);
    if (pm_thread != RT_NULL)
    {
        rt_thread_startup(pm_thread);
    }
    else
    {
        return RT_ERROR;
    }
    
    //线程：定时器中断处理
    rt_thread_t hwtimer_thread = rt_thread_create("hwtimer_thread",
                                                  hwtimer_thread_entry,
                                                  RT_NULL,
                                                  2048,
                                                  22,
                                                  10);
    if (pm_thread != RT_NULL)
    {
        rt_thread_startup(hwtimer_thread);
    }
    else
    {
        return RT_ERROR;
    }
    
    //线程：温度数据及GPS数据存储
    rt_thread_t hr_thread = rt_thread_create("hr_thread",
                                               hr_execute_thread_entry,
                                               RT_NULL,
                                               1024,
                                               18,
                                               10);
    if (pm_thread != RT_NULL)
    {
        rt_thread_startup(hr_thread);
    }
    else
    {
        return RT_ERROR;
    }
    
    //线程：GPS数据接收处理
    rt_thread_t gps_data_process_thread = rt_thread_create("gps_thread",
                                                           gps_execute_thread_entry,
                                                           RT_NULL,
                                                           1024,
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
    
    //线程：存储
    rt_thread_t save_data_thread = rt_thread_create("save_data_thread",
                                                 save_data_thread_entry,
                                                 RT_NULL,
                                                 4096,
                                                 27,
                                                 10);
    if (save_data_thread != RT_NULL)
    {
        rt_thread_startup(save_data_thread);
    }
    else
    {
        return RT_ERROR;
    }    
    
    return RT_EOK;
}



/**
  * @brief   led线程               
  * @param   void *parameter     
  * @retval  void    
  * @logs   
  * date        author        notes
  * 2020/4/20  Aprilhome
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
 * @function    存储数据线程
                在ringbuff写够阈值后，写入sd卡
 * @para        
 * @return      
 * @created     Aprilhome,2020/5/5 
**/
static void save_data_thread_entry(void *parameter)
{
    while (1)
    {
        rt_uint32_t e;
//        rt_event_recv(g_save_data_event, (EVENT_TEMP_RECV | EVENT_GPS_RECV), 
//                     (RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR), RT_WAITING_FOREVER, &e);
        
        if (SYSINFO.save == 1)
        {
            if (rt_ringbuffer_data_len(g_sd_buff) > THRESHOLD)
            {
                if (write_data_sd(g_sd_buff) != 0)
                {
                    pm_printf("$err %d\r\n", ERR_SD);
                }
            }
        }        
        rt_thread_mdelay(20);    
    }
}

static void deinit_sysinfo(void)
{
    if ((g_sysinfo.bound == 0xffffffff) || (g_sysinfo.id == 0xffff) || 
        (g_sysinfo.interval == 0xffffffff))
    {
        
        g_sysinfo.id = 20001;
        g_sysinfo.bound = 115200;
        g_sysinfo.interval = 5;
        if (config_sysinfo(&g_sysinfo) == -1)
        {
            pm_printf("$err %d\r\n",ERR_FLASH);
        }
    }    
}

/**
  * @brief    配置系统信息，最大占256字节    
  * @param    sys_info_t *info 系统信息变量（已经通过全局变量更新）    
  * @retval   0:成功；-1：失败 
  * @logs   
  * date        author        notes
  * 2019/8/30  Aprilhome
**/
int config_sysinfo(sys_info_t *info)
{
    uchar ret = 0;
    uchar buff[SYSINFO_LEN] = {0};
    for (uint i = 0; i < SYSINFO_LEN; i++)
    {
        buff[i] = *(((uchar *)info) + i);
    }
    ret = fal_partition_erase_all(g_sysinfo_partition);
    if (ret != RT_EOK)
        return ret;
    ret = fal_partition_write(g_sysinfo_partition, 0, buff, sizeof(sys_info_t));
    return ret;
}

/**
 * Function    ota_app_vtor_reconfig
 * Description Set Vector Table base location to the start addr of app(RT_APP_PART_ADDR).
*/
static int ota_app_vtor_reconfig(void)
{
    #define NVIC_VTOR_MASK   0x3FFFFF80
    /* Set the Vector Table base location by user application firmware definition */
    SCB->VTOR = RT_APP_PART_ADDR & NVIC_VTOR_MASK;

    return 0;
}
INIT_BOARD_EXPORT(ota_app_vtor_reconfig);