/**
  * Copyright (c) 2019, Aprilhome
  * 温度传感器通讯
  * Change Logs:
  * Date           Author       Notes
  * 2020-04-15     Aprilhome    first version
**/

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>

#include "drv_board.h"
#include "conf.h"
#include "api_temp.h"

/* temp通讯相关:串口,信号量,缓存 */
static rt_device_t g_temp_uart_device = RT_NULL;
static rt_sem_t    g_temp_uart_sem = RT_NULL;
static uart_receive_t     g_temp_uart_receive = {{0}, 0, {0}, 0, STATE_IDLE};
rt_sem_t g_ts_temp_sem = RT_NULL;
temp_t g_temp_data = {{0}, 0};
rt_uint16_t data_len = 0;
/**
 * @function    temp串口接收中断回调函数,‘$’开头，‘\n’结尾，最大长度512字节
 * @para1       dev:设备名
 * @para2       size:
 * @return      0成功
 * @created     Aprilhome,2019/8/25 
**/
static rt_err_t temp_uart_receive_callback(rt_device_t dev,rt_size_t size)
{
    rt_uint8_t ch;
    rt_size_t i;
    for (i = 0; i < size; i++)
    {
        if (rt_device_read(dev, -1, &ch, 1))
        {            
            switch (g_temp_uart_receive.flag)
            {
            case STATE_IDLE:
                if (ch == 0xfd)
                {
                    g_temp_uart_receive.flag = STATE_RECEIVE;
                    g_temp_uart_receive.buff[0] = ch;
                    g_temp_uart_receive.len = 1;
                }
                break;
            case STATE_RECEIVE:
                if (g_temp_uart_receive.len >= MAX_RECEIVE_LEN_TEMP)
                {
                    g_temp_uart_receive.flag = STATE_IDLE;
                    g_temp_uart_receive.len  = 0;
                    break;
                }
                if (g_temp_uart_receive.len >= 5)
                {
                    g_temp_uart_receive.flag = STATE_IDLE;
                    g_temp_uart_receive.buff[g_temp_uart_receive.len++] = ch; 
                    rt_sem_release(g_temp_uart_sem);
                    break;
                }
                g_temp_uart_receive.buff[g_temp_uart_receive.len++] = ch;
                break; 
            default:
                g_temp_uart_receive.flag = STATE_IDLE;
                g_temp_uart_receive.len  = 0;
                break;
            }     
        } 
    }
    return RT_EOK; 
}

/**
 * @function    temp数据处理线程
                将数据存入缓存用于上位机要数；同时用于向sd存
 * @param       void *parameter
 * @return      void
 * @created     Aprilhome,2019/4/19 
**/
void temp_execute_thread_entry(void *parameter)
{
    while (1)
    {
        rt_sem_take(g_temp_uart_sem, RT_WAITING_FOREVER);
       
        //读TEMP，写到全局变量
        rt_uint16_t i = 0;
        for (i = 0; i < g_temp_uart_receive.len; i++)
        {
            g_temp_data.data[i] = g_temp_uart_receive.buff[i];
        }
        g_temp_data.len = i;
        g_temp_uart_receive.len = 0;
        rt_event_send(g_sample_event, EVENT_TEMP_RECV);
        rt_sem_release(g_ts_temp_sem);  
        rt_thread_mdelay(20);
    }
}

/**
  * @brief    与温度模块通讯串口初始化              
  * @param    const char *name:串口名
              rt_uint32_t bound:波特率
  * @retval   0成功;-1失败   
  * @logs   
  * date        author        notes
  * 2019/8/21  Aprilhome
**/
int init_uart_temp(const char *name, rt_uint32_t bound)
{
    //1查找设备
    g_temp_uart_device = rt_device_find(name);
    if (g_temp_uart_device == RT_NULL)
    {
        rt_kprintf("find device %d failed!\r\n", name);
        return RT_ERROR;
    }
    //2配置设备
    struct serial_configure cfg = RT_SERIAL_CONFIG_DEFAULT;
    cfg.baud_rate = bound;
    cfg.bufsz = MAX_RECEIVE_LEN_TEMP;
    if (RT_EOK != rt_device_control(g_temp_uart_device, RT_DEVICE_CTRL_CONFIG,(void *)&cfg)) 
    {
        rt_kprintf("control device %d failed!\r\n", name);
        return RT_ERROR;
    }
    //3打开设备中断接收轮询发送
    if (RT_EOK != rt_device_open(g_temp_uart_device, RT_DEVICE_FLAG_INT_RX))
    {
        rt_kprintf("open device %d failed!\r\n", name);
        return RT_ERROR;
    }
    //4设置回调函数
    if (RT_EOK != rt_device_set_rx_indicate(g_temp_uart_device, temp_uart_receive_callback))
    {
        rt_kprintf("indicate device %d failed!\r\n", name);
        return RT_ERROR;
    }
    //5初始化信号量
    g_temp_uart_sem = rt_sem_create("temp_uart_sem", 0, RT_IPC_FLAG_FIFO);
    g_ts_temp_sem = rt_sem_create("temp_ts_sem", 0, RT_IPC_FLAG_FIFO);
    
    pm_printf("temp uart initialized ok\r\n");
    return RT_EOK;   
}
