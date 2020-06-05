/**
  * Copyright (c) 2019, Aprilhome
  * 定时器功能
  * Change Logs:
  * Date           Author       Notes
  * 2019-04-20     Aprilhome    first version
**/

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>

#include "drv_board.h"
#include "api_pm.h"

static rt_device_t g_hw_dev = RT_NULL;       /* 定时器设备句柄 */
static rt_sem_t g_hwtimer_sem = RT_NULL;

rt_err_t timeout_callback(rt_device_t dev, rt_size_t size)
{
//    rt_kprintf("this is hwtimer timeout callback function\n");
//    rt_kprintf("tick is :%d\n", rt_tick_get());
    rt_sem_release(g_hwtimer_sem);
    return 0;    
}

/**
 * @function    hwtimer中断执行线程
 * @param       void *parameter
 * @return      void
 * @created     Aprilhome,2019/4/19 
**/
void hwtimer_thread_entry(void *parameter)
{
    while (1)
    {
        rt_sem_take(g_hwtimer_sem, RT_WAITING_FOREVER);
        
        config_pm_t(1);
        pm_ts(0, 0, 0, 0, 0, 0);
        config_pm_t(0);        
        
        rt_thread_mdelay(20);
    }
}

/**
  * @brief    定时器初始化为定时器模式    
  * @param    const char *name：定时器名
  * @retval      
  * @logs   
  * date        author        notes
  * 2020/4/16  Aprilhome
**/
int init_timer(const char *name)
{
    rt_err_t ret = RT_EOK;
    
    /* 查找定时器设备 */
    g_hw_dev = rt_device_find(name);
    if (g_hw_dev == RT_NULL)
    {
        rt_kprintf("find %s device failed!\n", name);
        return RT_ERROR;
    }
    
    /* 以读写方式打开设备 */
    ret = rt_device_open(g_hw_dev, RT_DEVICE_OFLAG_RDWR);
    if (ret != RT_EOK)
    {
        rt_kprintf("open %s device failed!\n", name);
        return ret;
    }
    
    /* 设置超时回调函数 */
    rt_device_set_rx_indicate(g_hw_dev, timeout_callback);
    
    /* 设置定时器 */
    /* 模式为周期性定时器 */
    rt_hwtimer_mode_t mode = HWTIMER_MODE_PERIOD;
    ret = rt_device_control(g_hw_dev, HWTIMER_CTRL_MODE_SET, &mode);
    if (ret != RT_EOK)
    {
        rt_kprintf("set mode failed, ret is :%d\n", ret);
        return ret;
    }
    
    //初始化信号量
    g_hwtimer_sem = rt_sem_create("hwtimer_sem", 0, RT_IPC_FLAG_FIFO);
        
    return ret;  
}

/**
  * @brief   设置定时器开关状态，和定时周期（单位秒）     
  * @param   rt_uint8_t a：0关，其他开
             float timeout：超时时间，单位秒
  * @retval      
  * @logs   
  * date        author        notes
  * 2020/5/6  Aprilhome
**/
int config_hwtimer(rt_uint8_t a, float timeout)
{
    rt_err_t ret = RT_EOK;    
   
    if (a == 0)
    {
        ret = rt_device_control(g_hw_dev, HWTIMER_CTRL_STOP, 0);
        if (ret != RT_EOK)
        {
            rt_kprintf("set mode failed, ret is :%d\n", ret);
            return ret;
        }
    }
    else
    {
        /* 设置超时值，并启动定时器 */
        if (timeout <= 0)
        {
            timeout = 5;
        }
        rt_hwtimerval_t timeout_s;          /* 定时器超时值 */
        timeout_s.sec = (rt_int32_t)timeout;
        timeout_s.usec = (rt_int32_t)((timeout - (rt_int32_t)timeout) * 1000000);
        if (rt_device_write(g_hw_dev, 0, &timeout_s, sizeof(timeout_s)) != sizeof(timeout_s))
        {
            rt_kprintf("set timeout failed\n");
            return RT_ERROR;
        }      
    } 
    return ret;    
}