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

#include "drv_board.h"
#include "conf.h"
#include "api_pm.h"
#include "api_gps.h"
#include "api_temp.h"
#include "api_timer.h"

/* 系统状态信息及fal分区地址 */
sys_info_t g_sysinfo = {0};
const struct fal_partition *g_sysinfo_partition = RT_NULL;

rt_event_t  g_sample_event = RT_NULL;

/* 函数声明 */
static void led_thread_entry(void *parameter);
static void sample_thread_entry(void *parameter);
static void deinit_sysinfo(void);

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
    init_uart_temp("uart1", 115200);
    init_timer("timer15");
    
    /* 根据断电前工作模式，是否开启定时器 */
    switch (SYSINFO.mode)
    {
    case MODE_SLEEP:
        config_hwtimer(0,5);
        break;
    case MODE_NORMAL:
        config_hwtimer(0, SYSINFO.interval);
        break;
    case MODE_AUTO:
        config_hwtimer(1, SYSINFO.interval);
        break;
    default:
        rt_kprintf("$err %d\r\n", ERR_MODE);
        break;        
    }
    
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
    
    /* 初始化完成提示 */
    config_pm_t(1);
    LED_R(1);
    pm_printf("$IoT_humansensor %d\r\n", g_sysinfo.id);
    config_pm_t(0);
    
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
                                             2048,
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
                                                  1024,
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
    rt_thread_t temp_thread = rt_thread_create("temp_thread",
                                               temp_execute_thread_entry,
                                               RT_NULL,
                                               1024,
                                               23,
                                               10);
    if (pm_thread != RT_NULL)
    {
        rt_thread_startup(temp_thread);
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
    
    //线程：一次采集完成存储
    rt_thread_t sample_thread = rt_thread_create("sample_thread",
                                                 sample_thread_entry,
                                                 RT_NULL,
                                                 4096,
                                                 27,
                                                 10);
    if (sample_thread != RT_NULL)
    {
        rt_thread_startup(sample_thread);
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
 * @function    采樣一次處理綫程
 * @para        
 * @return      
 * @created     Aprilhome,2020/5/5 
**/
static void sample_thread_entry(void *parameter)
{
    while (1)
    {
        rt_uint32_t e;
        rt_event_recv(g_sample_event, (EVENT_TEMP_RECV | EVENT_GPS_RECV), 
                     (RT_EVENT_FLAG_AND | RT_EVENT_FLAG_CLEAR), RT_WAITING_FOREVER, &e);
        
        //写入sd卡,包括gps数据g_gps_data,包括temp数据g_temp_data
        char name[20] = {0};
        sprintf(name, "/%02d%02d%02d.dat", g_gps_data.year, g_gps_data.month, g_gps_data.day);
        int fd = open(name, O_WRONLY | O_CREAT | O_APPEND);
//        int fd = open("/data.dat", O_WRONLY | O_CREAT | O_APPEND);
        //打开成功
        if (fd >= 0)
        {
            char data[1024] = {0};
            rt_uint16_t data_len = 0;
            rt_int16_t ret = 0;
            data_len = sprintf(data, "$GNRMC %02d-%02d-%02d %02d:%02d:%02d %f %c %f %c %f %f %c\r\n",
                               g_gps_data.year, g_gps_data.month, g_gps_data.day, 
                               g_gps_data.hour, g_gps_data.minute, g_gps_data.second, 
                               g_gps_data.latitude, g_gps_data.ns, g_gps_data.longitude, g_gps_data.ew, 
                               g_gps_data.speed, g_gps_data.direction, g_gps_data.mode);

            memcpy(data + data_len, g_temp_data.data, g_temp_data.len);
            
            ret = write(fd, data, data_len + g_temp_data.len);
            if (ret > 0)
            {
                rt_kprintf("sd write successed\n");
            }
            else
            {
                rt_kprintf("sd write failed\n");
            }
            
            close(fd);
        }  
        else
        {
            rt_kprintf("sd open file failed\n");
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