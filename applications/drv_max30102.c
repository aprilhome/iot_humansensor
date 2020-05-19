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

#include "drv_max30102.h"
#include "algorithm.h"
//#include "conf.h"
#include "comm_pc.h"


static struct rt_i2c_bus_device *g_i2c_bus_max30102 = RT_NULL;     /* I2C总线设备句柄 */
/* 信号量、线程 */
rt_sem_t    g_max30102_acquire_sem;
static rt_sem_t    g_max30102_calculate_sem;
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
static rt_err_t max30102_read_reg(rt_uint8_t reg, rt_uint8_t len, rt_uint8_t *buf)
{
    struct rt_i2c_msg msgs[2];

    msgs[0].addr  = MAX30102_ADDR >> 1;
    msgs[0].flags = RT_I2C_WR;
    msgs[0].buf   = &reg;
    msgs[0].len   = 1;

    msgs[1].addr  = MAX30102_ADDR >> 1;
    msgs[1].flags = RT_I2C_RD;
    msgs[1].buf   = buf;
    msgs[1].len   = len;
    
    /* 调用I2C设备接口传输数据 */
    if (rt_i2c_transfer(g_i2c_bus_max30102, msgs, 2) == 2)
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
static rt_err_t max30102_write_reg(rt_uint8_t reg, rt_uint8_t len, rt_uint8_t *data)
{
    rt_uint8_t buf[64] = {0};
    struct rt_i2c_msg msgs;

    buf[0] = reg;
    for (rt_uint8_t i = 0; i < len; i++)
    {
        buf[1 + i] = data[i];
    }
    
    msgs.addr  = MAX30102_ADDR >> 1;
    msgs.flags = RT_I2C_WR;
    msgs.buf   = buf;
    msgs.len   = len + 1;
    
    /* 调用I2C设备接口传输数据 */
    if (rt_i2c_transfer(g_i2c_bus_max30102, &msgs, 1) == 1)
    {
        return RT_EOK;
    }
    else
    {
        return -RT_ERROR;
    }
}

/**
  * @brief    max30102初始化    
  * @param    void    
  * @retval   void   
  * @logs   
  * date        author        notes
  * 2019/11/18  Aprilhome
**/
void max30102_init(void)
{
    rt_pin_mode(MAX30102_INT, PIN_MODE_INPUT_PULLUP);
    //初始化信号量
    g_max30102_acquire_sem = rt_sem_create("acquire_sem", 0, RT_IPC_FLAG_FIFO);
    g_max30102_calculate_sem = rt_sem_create("calculate_sem", 0, RT_IPC_FLAG_FIFO);
    /* 查找I2C总线设备，获取I2C总线设备句柄 */
    g_i2c_bus_max30102 = (struct rt_i2c_bus_device *)rt_device_find(MAX30102_I2C_BUS_NAME);
    if (g_i2c_bus_max30102 == RT_NULL)
    {
        rt_kprintf("find i2c1 failure\r\n");
    }
    else
    {
        //依次执行：复位2次，设置参数，使能温度
        rt_uint8_t data_reset = 0x40;        
        max30102_write_reg(REG_MODE_CONFIG, 1, &data_reset);    //reset
        max30102_write_reg(REG_MODE_CONFIG, 1, &data_reset);    //reset
        
        rt_uint8_t reg_enable[5] = {0xc0, 0x00, 0x00, 0x00, 0x00};
        rt_uint8_t reg_config[3] = {0x0f, 0x03, 0x27};
        rt_uint8_t reg_led[2] = {0x24, 0x24};
        rt_uint8_t reg_pilot[1] = {0x7f};
        max30102_write_reg(REG_INTR_ENABLE_1, 5, reg_enable); 
        max30102_write_reg(REG_FIFO_CONFIG, 3, reg_config);
        max30102_write_reg(REG_LED1_PA, 2, reg_led);
        max30102_write_reg(REG_PILOT_PA, 1, reg_pilot);
        
        rt_uint8_t data_temp = 0x01;
        max30102_write_reg(REG_TEMP_CONFIG, 1, &data_temp);
    }
}

/**
  * @brief    读所有寄存器值（调试用）    
  * @param    rt_uint8_t *regf:reg[33]    
  * @retval   void   
  * @logs   
  * date        author        notes
  * 2019/11/18  Aprilhome
**/
void read_max30102_reg(rt_uint8_t *reg)
{
    max30102_read_reg(0x08, 33, reg);
}

/**
  * @brief    读FIFO寄存器值，一次采样2个通道，每个通道3字节，共6字节    
  * @param    rt_uint8_t *regf:reg[6]    
  * @retval      
  * @logs   
  * date        author        notes
  * 2019/11/18  Aprilhome
**/
void read_max30102_fifo(rt_uint8_t *reg)
{
    rt_uint8_t temp;
    //读取并清除中断状态寄存器
    max30102_read_reg(REG_INTR_STATUS_1, 1, &temp);
    max30102_read_reg(REG_INTR_STATUS_2, 1, &temp);
    
    //读取FIFO6个字节，前3个为红灯，后三个为红外
    max30102_read_reg(REG_FIFO_DATA, 6, reg);
}

#define MAX_BRIGHTNESS 255
uint32_t aun_ir_buffer[500];        //IR LED sensor data
int32_t n_ir_buffer_length;         //data length
uint32_t aun_red_buffer[500];       //Red LED sensor data
int32_t n_sp02;                     //SPO2 value
int8_t ch_spo2_valid;   //indicator to show if the SP02 calculation is valid
int32_t n_heart_rate;   //heart rate value
int8_t  ch_hr_valid;    //indicator to show if the heart rate calculation is valid

void max30102_acq(void)
{    
    uint32_t un_min, un_max, un_prev_data;  
    int i;
    int32_t n_brightness;
    float f_temp;
//    uint8_t temp_num=0;
    uint8_t temp[6] = {0};
//    uint8_t str[100];
//    uint8_t dis_hr=0, dis_spo2=0;
    
    un_min = 0x3FFFF;
    un_max = 0;
    n_ir_buffer_length = 500; //buffer length of 100 stores 5 seconds of samples running at 100sps
    
    //read the first 500 samples, and determine the signal range
    for(i=0;i<n_ir_buffer_length;i++)
    {
        while (rt_pin_read(MAX30102_INT));   //wait until the interrupt pin asserts
        
        read_max30102_fifo(temp);
        aun_red_buffer[i] = (long)((long)((long)temp[0]&0x03)<<16) | (long)temp[1]<<8 | (long)temp[2];  // Combine values to get the actual number
        aun_ir_buffer[i]  = (long)((long)((long)temp[3]&0x03)<<16) | (long)temp[4]<<8 | (long)temp[5];  // Combine values to get the actual number
        
        if(un_min>aun_red_buffer[i])
            un_min=aun_red_buffer[i];    //update signal min
        if(un_max<aun_red_buffer[i])
            un_max=aun_red_buffer[i];    //update signal max
    }
    un_prev_data=aun_red_buffer[i];
    
    //calculate heart rate and SpO2 after first 500 samples (first 5 seconds of samples)
    maxim_heart_rate_and_oxygen_saturation(aun_ir_buffer, n_ir_buffer_length, aun_red_buffer, &n_sp02, &ch_spo2_valid, &n_heart_rate, &ch_hr_valid); 
    
//    while(1)
//    {
        i=0;
        un_min=0x3FFFF;
        un_max=0;
        
        //dumping the first 100 sets of samples in the memory and shift the last 400 sets of samples to the top
        for(i=100;i<500;i++)
        {
            aun_red_buffer[i-100]=aun_red_buffer[i];
            aun_ir_buffer[i-100]=aun_ir_buffer[i];
            
            //update the signal min and max
            if(un_min>aun_red_buffer[i])
                un_min=aun_red_buffer[i];
            if(un_max<aun_red_buffer[i])
                un_max=aun_red_buffer[i];
        }
        //take 100 sets of samples before calculating the heart rate.
        for(i=400;i<500;i++)
        {
            un_prev_data=aun_red_buffer[i-1];
            while (rt_pin_read(MAX30102_INT));
            read_max30102_fifo(temp);
            aun_red_buffer[i] = (long)((long)((long)temp[0]&0x03)<<16) | (long)temp[1]<<8 | (long)temp[2];   // Combine values to get the actual number
            aun_ir_buffer[i]  = (long)((long)((long)temp[3]&0x03)<<16) | (long)temp[4]<<8 | (long)temp[5];   // Combine values to get the actual number
            
            if(aun_red_buffer[i]>un_prev_data)
            {
                f_temp=aun_red_buffer[i]-un_prev_data;
                f_temp/=(un_max-un_min);
                f_temp*=MAX_BRIGHTNESS;
                n_brightness-=(int)f_temp;
                if(n_brightness<0)
                    n_brightness=0;
            }
            else
            {
                f_temp=un_prev_data-aun_red_buffer[i];
                f_temp/=(un_max-un_min);
                f_temp*=MAX_BRIGHTNESS;
                n_brightness+=(int)f_temp;
                if(n_brightness>MAX_BRIGHTNESS)
                    n_brightness=MAX_BRIGHTNESS;
            }
            //send samples and calculation result to terminal program through UART
//            if(ch_hr_valid == 1 && n_heart_rate<120)//**/ ch_hr_valid == 1 && ch_spo2_valid ==1 && n_heart_rate<120 && n_sp02<101
//            {
//                dis_hr = n_heart_rate;
//                dis_spo2 = n_sp02;
//            }
//            else
//            {
//                dis_hr = 0;
//                dis_spo2 = 0;
//            }
//            pc_printf("red=%i, ir=%i\r\n", aun_red_buffer[i], aun_ir_buffer[i]); 
//            pc_printf("HR=%i, HRvalid=%i\r\n", n_heart_rate, ch_hr_valid); 
//            pc_printf("SpO2=%i, SPO2Valid=%i\r\n", n_sp02, ch_spo2_valid);
        }
        maxim_heart_rate_and_oxygen_saturation(aun_ir_buffer, n_ir_buffer_length, aun_red_buffer, &n_sp02, &ch_spo2_valid, &n_heart_rate, &ch_hr_valid);
        pc_printf("HR=%i, SpO2=%i\r\n", n_heart_rate, n_sp02);
//    }
}

/**
  * @brief    读温度寄存器值，注意每次先使能    
  * @param        
  * @retval      
  * @logs   
  * date        author        notes
  * 2019/11/18  Aprilhome
**/
void read_max30102_temp(rt_uint8_t *reg)
{
    rt_uint8_t data_temp = 0x01;
    max30102_write_reg(REG_TEMP_CONFIG, 1, &data_temp);
//    rt_uint8_t temp;
//    max30102_read_reg(REG_INTR_STATUS_1, 1, &temp);
//    max30102_read_reg(REG_INTR_STATUS_2, 1, &temp);
    max30102_read_reg(REG_TEMP_INTR, 2, reg);     
}




/**
 * @function    max30102读传感器线程
 * @param       void *parameter
 * @return      void
 * @created     Aprilhome,2019/11/18 
**/
//void max30102_acquire_thread_entry(void *parameter)
//{
//    while (1)
//    {
//        rt_sem_take(g_max30102_acquire_sem, RT_WAITING_FOREVER);
//        
//        rt_uint8_t reg[6] = {0};
//        rt_uint32_t red_min = 0x3FFFF;
//        rt_uint32_t red_max = 0;
//        rt_uint16_t len = 500;
//        rt_uint16_t i = 0;
//        rt_uint32_t red_buff[500] = {0};
//        rt_uint32_t ir_buff[500]  = {0};
//        rt_int32_t n_sp02; //SPO2 value
//        rt_int8_t ch_spo2_valid;   //indicator to show if the SP02 calculation is valid
//        rt_int32_t n_heart_rate;   //heart rate value
//        rt_int8_t  ch_hr_valid;    //indicator to show if the heart rate calculation is valid
//        for (i = 0; i < len; i++)
//        {
////            while (rt_pin_read(MAX30102_INT))
////                ;
//            rt_thread_mdelay(5500);
//            read_max30102_fifo(reg);
//            red_buff[i] = (long)((long)((long)reg[0] & 0x03) << 16) | 
//                          (long)reg[1] << 8 | 
//                          (long)reg[2];
//            ir_buff[i]  = (long)((long)((long)reg[3] & 0x03) << 16) | 
//                          (long)reg[4] << 8 | 
//                          (long)reg[5];
//            if (red_buff[i] < red_min)
//            {
//                red_min = red_buff[i];
//            }
//            if (red_buff[i] > red_max)
//            {
//                red_max = red_buff[i];
//            }
//        }
//        
//        maxim_heart_rate_and_oxygen_saturation(ir_buff, len, red_buff, &n_sp02, &ch_spo2_valid, &n_heart_rate, &ch_hr_valid); 
//
//        pc_printf("hr:%i spo2:%i\r\n",n_heart_rate, n_sp02);
//        
//    }
//}

/**
 * @function    计算max30102传感器线程
 * @param       void *parameter
 * @return      void
 * @created     Aprilhome,2019/11/18 
**/
void max30102_calculate_thread_entry(void *parameter)
{
    while (1)
    {
        rt_sem_take(g_max30102_calculate_sem, RT_WAITING_FOREVER);

    }
}

