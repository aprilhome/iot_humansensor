/**
  * Copyright (c) 2019, Aprilhome
  * 板级功能测试，通过FINSH指令导出
  * Change Logs:
  * Date           Author       Notes
  * 2019-04-20     Aprilhome    first version
**/
#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>

#include "drv_board.h"
#include "api_timer.h"

static int led_test(int argc, char *argv[])
{
    rt_err_t ret = RT_EOK;
    
    LED_R(1);
    LED_G(0);
    LED_B(0);
    rt_thread_mdelay(1000);
    LED_R(0);
    LED_G(1);
    LED_B(0);
    rt_thread_mdelay(1000);
    LED_R(0);
    LED_G(0);
    LED_B(1);
    rt_thread_mdelay(1000);

    return ret;
}
/* 导 出 到 msh 命 令 列 表 中 */
MSH_CMD_EXPORT(led_test, leds test);

static int switch_test(int argc, char *argv[])
{
    rt_err_t ret = RT_EOK;
    rt_uint8_t onoff = atoi(argv[1]);
    if (onoff == 0)
    {
        EN_PC_4G_GPS_T(1);
        EN_4G(1);
        EN_GPS(1);
    }
    else
    {
        EN_PC_4G_GPS_T(0);
        EN_4G(0);
        EN_GPS(0); 
    }
    return ret;
}
/* 导 出 到 msh 命 令 列 表 中 */
MSH_CMD_EXPORT(switch_test, switchs test);

static int rtc_test(int argc, char *argv[])
{
    rt_err_t ret = RT_EOK;
    time_t now;
    /* 设置日期 */
    ret = set_date(2018, 12, 3);
    if (ret != RT_EOK)
    {
        rt_kprintf("set RTC date failed\n");
        return ret;
    }
    /* 设置时间 */
    ret = set_time(11, 15, 50);
    if (ret != RT_EOK)
    {
        rt_kprintf("set RTC time failed\n");
        return ret;
    }
    
    /* 延时3秒 */
    rt_thread_mdelay(3000);
    /* 获取时间 */
    now = time(RT_NULL);
    rt_kprintf("%s\n", ctime(&now));
    return ret;
}
/* 导 出 到 msh 命 令 列 表 中 */
MSH_CMD_EXPORT(rtc_test, rtc test);

static int hwtimer_test(int argc, char *argv[])
{
    rt_err_t ret = RT_EOK;
    rt_hwtimerval_t timeout_s;          /* 定时器超时值 */
    rt_device_t hw_dev = RT_NULL;       /* 定时器设备句柄 */
    rt_hwtimer_mode_t mode;             /* 定时器模式 */
    rt_uint32_t freq = 10000;           /* 计数频率 */
    
    /* 查找定时器设备 */
    hw_dev = rt_device_find(HWTIMER_DEV_NAME);
    if (hw_dev == RT_NULL)
    {
        rt_kprintf("find %s device failed!\n", HWTIMER_DEV_NAME);
        return RT_ERROR;
    }
    
    /* 以读写方式打开设备 */
    ret = rt_device_open(hw_dev, RT_DEVICE_OFLAG_RDWR);
    if (ret != RT_EOK)
    {
        rt_kprintf("open %s device failed!\n", HWTIMER_DEV_NAME);
        return ret;
    }
    
    /* 设置超时回调函数 */
    rt_device_set_rx_indicate(hw_dev, timeout_callback);
    
//    /* 设置计数频率（默认1MHz或支持的最小计数频率）*/
//    ret = rt_device_control(hw_dev, HWTIMER_CTRL_FREQ_SET, &freq);
//    if (ret != RT_EOK)
//    {
//        rt_kprintf("set frequency failed, ret is :%d\n", ret);
//        return ret;
//    }
    
    /* 设置模式为周期性定时器 */
    mode = HWTIMER_MODE_PERIOD;
    ret = rt_device_control(hw_dev, HWTIMER_CTRL_MODE_SET, &mode);
    if (ret != RT_EOK)
    {
        rt_kprintf("set mode failed, ret is :%d\n", ret);
        return ret;
    }
    
    /* 设置超时值为5秒，并启动定时器 */
    timeout_s.sec = 5;
    timeout_s.usec = 0;
    if (rt_device_write(hw_dev, 0, &timeout_s, sizeof(timeout_s)) != sizeof(timeout_s))
    {
        rt_kprintf("set timeout failed\n");
        return RT_ERROR;
    }
    
    /* 延时3500ms */
    rt_thread_mdelay(3500);
    
    /* 读取定时器当前值 */
    rt_device_read(hw_dev, 0, &timeout_s, sizeof(timeout_s));
    rt_kprintf("read: sec = %d, usec = %d\n", timeout_s.sec, timeout_s.usec);
    
    return ret;        
}
/* 导出到 msh 命令列表中 */
MSH_CMD_EXPORT(hwtimer_test, hwtimer sample);