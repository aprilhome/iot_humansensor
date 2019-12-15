/**
  * Copyright (c) 2019, Aprilhome
  * GPS通讯
  * Change Logs:
  * Date           Author       Notes
  * 2019-04-20     Aprilhome    first version
**/

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>

#include "conf.h"
#include "drv_gps.h"
#include "comm_pc.h"

const struct serial_data_long g_gps_data[] =
{
    {"$GNRMC",          gps_gnrmc},
};

/* gps通讯相关:串口,信号量,缓存 */
static rt_device_t g_gps_uart_device = RT_NULL;
static rt_sem_t    g_gps_uart_sem;
uart_receive_t     g_gps_uart_receive = {{0}, 0, {0}, 0, STATE_IDLE};

/**
 * @function    gps串口接收中断回调函数
 * @para1       dev:设备名
 * @para2       size:
 * @return      0成功
 * @created     Aprilhome,2019/8/25 
**/
static rt_err_t gps_uart_receive_callback(rt_device_t dev,rt_size_t size)
{
    rt_uint8_t ch;
    rt_size_t i;
    for (i = 0; i < size; i++)
    {
        if (rt_device_read(dev, -1, &ch, 1))
        {            
            switch (g_gps_uart_receive.flag)
            {
            case STATE_IDLE:
                if (ch == '$')
                {
                    g_gps_uart_receive.flag = STATE_RECEIVE;
                    g_gps_uart_receive.argv[0] = g_gps_uart_receive.buff;
                    g_gps_uart_receive.argc = 0;
                    g_gps_uart_receive.buff[0] = ch;
                    g_gps_uart_receive.len = 1;
                }
                break;
            case STATE_RECEIVE:
                if ((ch == 0x0d)||(ch == 0x0a))
                {
                    g_gps_uart_receive.flag = STATE_IDLE;
                    g_gps_uart_receive.buff[g_gps_uart_receive.len++] = ch; 
                    g_gps_uart_receive.argc++;
                    rt_sem_release(g_gps_uart_sem);
                    break;
                }
                if ((g_gps_uart_receive.len > GPS_CMD_LEN_MAX) || (g_gps_uart_receive.argc > GPS_CMD_ARGC_MAX))
                {
                    g_gps_uart_receive.flag = STATE_IDLE;
                    g_gps_uart_receive.argc = 0;
                    g_gps_uart_receive.len  = 0;
                    break;
                }
                if ((ch < 32)||(ch > 127))
                {
                    g_gps_uart_receive.flag = STATE_IDLE;
                    g_gps_uart_receive.argc = 0;
                    g_gps_uart_receive.len  = 0;
                    break;
                }
                if (ch == ',')
                {
                    g_gps_uart_receive.flag = STATE_SUSPEND;
                    g_gps_uart_receive.buff[g_gps_uart_receive.len++] = 0;
                    g_gps_uart_receive.argc++;
                    break;
                }
                g_gps_uart_receive.buff[g_gps_uart_receive.len++] = ch;
                break;
            case STATE_SUSPEND:
                if ((ch == 0x0d)||(ch == 0x0a))
                {
                    g_gps_uart_receive.flag = STATE_IDLE;
                    g_gps_uart_receive.argv[g_gps_uart_receive.argc] = g_gps_uart_receive.buff + g_gps_uart_receive.len;
                    g_gps_uart_receive.buff[g_gps_uart_receive.len++] = ch;
                    rt_sem_release(g_gps_uart_sem);
                    break;
                }
                if ((g_gps_uart_receive.len > GPS_CMD_LEN_MAX) || (g_gps_uart_receive.argc > GPS_CMD_ARGC_MAX))
                {
                    g_gps_uart_receive.flag = STATE_IDLE;
                    g_gps_uart_receive.argc = 0;
                    g_gps_uart_receive.len  = 0;
                    break;
                }
                if ((ch < 32)||(ch > 127))
                {
                    g_gps_uart_receive.flag = STATE_IDLE;
                    g_gps_uart_receive.argc = 0;
                    g_gps_uart_receive.len  = 0;
                    break;
                }
                if (ch == ',')
                {
                    break;
                }
                g_gps_uart_receive.flag = STATE_RECEIVE;
                g_gps_uart_receive.argv[g_gps_uart_receive.argc] = g_gps_uart_receive.buff + g_gps_uart_receive.len;
                g_gps_uart_receive.buff[g_gps_uart_receive.len++] = 0;
                break;          
            }     
        } 
    }
    return RT_EOK; 
}

/**
 * @function    gps数据处理线程
 * @param       void *parameter
 * @return      void
 * @created     Aprilhome,2019/4/19 
**/
void gps_execute_thread_entry(void *parameter)
{
    while (1)
    {
        rt_sem_take(g_gps_uart_sem, RT_WAITING_FOREVER);
       
        int i = 0;
        for (i = sizeof(g_gps_data)/sizeof(*g_gps_data) - 1; i >= 0; i--)
        {
            if (strcmp(g_gps_data[i].serial_data_name, g_gps_uart_receive.argv[0]) == 0)
            {
                (*(g_gps_data[i].serial_data_function))();
                break;
            }        
        }
        g_gps_uart_receive.argc = 0;
        g_gps_uart_receive.len = 0;
        g_gps_uart_receive.flag = STATE_IDLE;
                
        rt_thread_mdelay(500);
    }
}

/**
  * @brief    与GPS模块通讯串口初始化              
  * @param    const char *name:串口名
              rt_uint32_t bound:波特率
  * @retval   0成功;-1失败   
  * @logs   
  * date        author        notes
  * 2019/8/21  Aprilhome
**/
int config_uart_gps(const char *name, rt_uint32_t bound)
{
    //1查找设备
    g_gps_uart_device = rt_device_find(name);
    if (g_gps_uart_device == RT_NULL)
    {
        rt_kprintf("find device %d failed!\r\n", name);
        return RT_ERROR;
    }
    //2配置设备
    struct serial_configure cfg = RT_SERIAL_CONFIG_DEFAULT;
    cfg.baud_rate = bound;
    if (RT_EOK != rt_device_control(g_gps_uart_device, RT_DEVICE_CTRL_CONFIG,(void *)&cfg)) 
    {
        rt_kprintf("control device %d failed!\r\n", name);
        return RT_ERROR;
    }
    //3打开设备中断接收轮询发送
    if (RT_EOK != rt_device_open(g_gps_uart_device, RT_DEVICE_FLAG_INT_RX))
    {
        rt_kprintf("open device %d failed!\r\n", name);
        return RT_ERROR;
    }
    //4设置回调函数
    if (RT_EOK != rt_device_set_rx_indicate(g_gps_uart_device, gps_uart_receive_callback))
    {
        rt_kprintf("indicate device %d failed!\r\n", name);
        return RT_ERROR;
    }
    //5初始化信号量
    g_gps_uart_sem = rt_sem_create("gps_uart_sem", 0, RT_IPC_FLAG_FIFO);

    printf("Pandora\r\n");
    return RT_EOK;   
}

/*******************************************************************************
 *gps处理函数1*
 *检查仪器状态、参数合法性、写入是否成功，并从FLASH中读取后输出
 ******************************************************************************/
void gps_gnrmc(void)
{
    for (rt_uint8_t i = 0; i < g_gps_uart_receive.len; i++)
    {
        pc_printf("%c", g_gps_uart_receive.buff[i]);
    }    
}
