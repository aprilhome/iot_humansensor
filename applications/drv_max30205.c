/**
  * Copyright (c) 2019, Aprilhome
  * 
  * Change Logs:
  * Date           Author       Notes
  * 2019-04-20     Aprilhome    first version
**/

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>

#include <drv_log.h>
#include "drv_soft_i2c.h"

#include "drv_max30205.h"

static struct rt_i2c_bus_device *g_i2c_bus_max30205 = RT_NULL;     /* I2C总线设备句柄 */
/* 信号量、线程 */
//rt_sem_t    g_max30205_acquire_sem;
//static rt_sem_t    g_max30205_calculate_sem;
/**
  * @brief   读若干长度寄存器值     
  * @param   
        rt_uint8_t reg： 要读取的第一个寄存器地址
        rt_uint8_t len： 要读取的寄存器长度
        rt_uint8_t *buf：读出寄存器值的缓存数组指针
  * @retval  0成功；-1失败    
  * @logs   
  * date        author        notes
  * 2019/11/18  Aprilhome
**/
static rt_err_t max30205_read_reg(rt_uint8_t reg, rt_uint8_t len, rt_uint8_t *buf)
{
    struct rt_i2c_msg msgs[2];

    msgs[0].addr  = MAX30205_ADDR >> 1;
    msgs[0].flags = RT_I2C_WR;
    msgs[0].buf   = &reg;
    msgs[0].len   = 1;

    msgs[1].addr  = MAX30205_ADDR >> 1;
    msgs[1].flags = RT_I2C_RD;
    msgs[1].buf   = buf;
    msgs[1].len   = len;
    
    /* 调用I2C设备接口传输数据 */
    if (rt_i2c_transfer(g_i2c_bus_max30205, msgs, 2) == 2)
    {
        return RT_EOK;
    }
    else
    {
        return -RT_ERROR;
    }
}

/**
  * @brief   向寄存器写入若干长度寄存器值，最大一次连续写寄存器长度63     
  * @param   
        rt_uint8_t reg： 要写的第一个寄存器地址
        rt_uint8_t len： 要写的寄存器长度
        rt_uint8_t *buf：要写的寄存器值的缓存数组指针
  * @retval  0成功；-1失败    
  * @logs   
  * date        author        notes
  * 2019/11/18  Aprilhome
**/
static rt_err_t max30205_write_reg(rt_uint8_t reg, rt_uint8_t len, rt_uint8_t *data)
{
    rt_uint8_t buf[64] = {0};
    struct rt_i2c_msg msgs;

    buf[0] = reg;
    for (rt_uint8_t i = 0; i < len; i++)
    {
        buf[1 + i] = data[i];
    }
    
    msgs.addr  = MAX30205_ADDR >> 1;
    msgs.flags = RT_I2C_WR;
    msgs.buf   = buf;
    msgs.len   = len + 1;
    
    /* 调用I2C设备接口传输数据 */
    if (rt_i2c_transfer(g_i2c_bus_max30205, &msgs, 1) == 1)
    {
        return RT_EOK;
    }
    else
    {
        return -RT_ERROR;
    }
}

/**
  * @brief    max30205初始化    
  * @param    void    
  * @retval   void   
  * @logs   
  * date        author        notes
  * 2019/11/18  Aprilhome
**/
void max30205_init(void)
{
   /* 查找I2C总线设备，获取I2C总线设备句柄 */
    g_i2c_bus_max30205 = (struct rt_i2c_bus_device *)rt_device_find(MAX30205_I2C_BUS_NAME);
    if (g_i2c_bus_max30205 == RT_NULL)
    {
        rt_kprintf("find i2c3 failure\r\n");
    }
    else
    {
        //依次执行：复位，设置参数
        rt_uint8_t data_reset = 0x00;        
        max30205_write_reg(MAX30205_CONFIGURATION, 1, &data_reset);    //config
    }
}

/**
  * @brief    读FIFO寄存器值，一次采样2个通道，每个通道3字节，共6字节    
  * @param    rt_uint8_t *regf:reg[6]    
  * @retval      
  * @logs   
  * date        author        notes
  * 2019/11/18  Aprilhome
**/
float read_max30205_temperature(void)
{
    rt_uint8_t read[2] = {0};
    int ret = 0;
    ret = max30205_read_reg(MAX30205_TEMPERATURE, 2, read);

    if (ret == RT_EOK)
    {
        rt_int16_t raw = read[0] << 8 | read[1];
        float temperature = raw * 0.00390625;
        return temperature;
    }
    else
    {        
        return -100;
    }
}
