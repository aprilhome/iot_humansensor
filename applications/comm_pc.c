/**
  * Copyright (c) 2019, Aprilhome
  * 上位机通讯
  * Change Logs:
  * Date           Author       Notes
  * 2019-04-20     Aprilhome    first version
**/

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "comm_pc.h"
#include "conf.h"
//#include "drv_board.h"
//#include "drv_max30102.h"
//#include "drv_max30205.h"
//#include "main.h"

const struct cmd g_pc_get_cmd[] =
{
    {"$checkin",          pc_checkin},
    {"$ds",               pc_ds},
    {"$setmode",          pc_set_mode},
    {"$ts",               pc_takesample},
    {"$ts_spo2",          pc_takesample_spo2},
    {"$ts_hr",            pc_takesample_hr},
    {"$ts_temp",          pc_takesample_temp},
};

const struct cmd g_pc_set_cmd[] = 
{
    {"$setid",            pc_set_id},
    {"$setbaudrate",      pc_set_baudrate},
    {"$setsensor",        pc_set_sensor},
};

extern sys_info_t g_sysinfo;

/* 上位机通讯相关:串口,信号量,接收缓存 */
static rt_device_t g_pc_uart_device = RT_NULL;
static rt_sem_t    g_pc_uart_sem;
uart_receive_t     g_pc_uart_receive = {{0}, 0, {0}, 0, STATE_IDLE};

extern rt_sem_t    g_max30102_acquire_sem;

/**
 * @function    上位机串口接收中断回调函数
 * @para1       dev:设备名
 * @para2       size:
 * @return      0成功
 * @created     Aprilhome,2019/8/25 
**/
static rt_err_t pc_uart_receive_callback(rt_device_t dev,rt_size_t size)
{
    rt_uint8_t ch;
    rt_size_t i;
    for (i = 0; i < size; i++)
    {
        if (rt_device_read(dev, -1, &ch, 1))
        {            
            switch (g_pc_uart_receive.flag)
            {
            case STATE_IDLE:
                if ((ch > 32)&&(ch < 127))
                {
                    g_pc_uart_receive.flag = STATE_RECEIVE;
                    g_pc_uart_receive.argv[0] = g_pc_uart_receive.buff;
                    g_pc_uart_receive.argc = 0;
                    g_pc_uart_receive.buff[0] = ch;
                    g_pc_uart_receive.len = 1;
                }
                break;
            case STATE_RECEIVE:
                if ((ch == 0x0d)||(ch == 0x0a))
                {
                    g_pc_uart_receive.flag = STATE_IDLE;
                    g_pc_uart_receive.buff[g_pc_uart_receive.len++] = 0; 
                    g_pc_uart_receive.argc++;
                    rt_sem_release(g_pc_uart_sem);
                    break;
                }
                if ((g_pc_uart_receive.len > PC_CMD_LEN_MAX) || (g_pc_uart_receive.argc > PC_CMD_ARGC_MAX))
                {
                    g_pc_uart_receive.flag = STATE_IDLE;
                    g_pc_uart_receive.argc = 0;
                    g_pc_uart_receive.len  = 0;
                    break;
                }
                if ((ch < 32)||(ch > 127))
                {
                    g_pc_uart_receive.flag = STATE_IDLE;
                    g_pc_uart_receive.argc = 0;
                    g_pc_uart_receive.len  = 0;
                    break;
                }
                if (ch == ' ')
                {
                    g_pc_uart_receive.flag = STATE_SUSPEND;
                    g_pc_uart_receive.buff[g_pc_uart_receive.len++] = 0;
                    g_pc_uart_receive.argc++;
                    break;
                }
                g_pc_uart_receive.buff[g_pc_uart_receive.len++] = ch;
                break;
            case STATE_SUSPEND:
                if ((ch == 0x0d)||(ch == 0x0a))
                {
                    g_pc_uart_receive.flag = STATE_IDLE;
                    g_pc_uart_receive.argv[g_pc_uart_receive.argc] = g_pc_uart_receive.buff + g_pc_uart_receive.len;
                    g_pc_uart_receive.buff[g_pc_uart_receive.len++] = 0;
                    rt_sem_release(g_pc_uart_sem);
                    break;
                }
                if ((g_pc_uart_receive.len > PC_CMD_LEN_MAX) || (g_pc_uart_receive.argc > PC_CMD_ARGC_MAX))
                {
                    g_pc_uart_receive.flag = STATE_IDLE;
                    g_pc_uart_receive.argc = 0;
                    g_pc_uart_receive.len  = 0;
                    break;
                }
                if ((ch < 32)||(ch > 127))
                {
                    g_pc_uart_receive.flag = STATE_IDLE;
                    g_pc_uart_receive.argc = 0;
                    g_pc_uart_receive.len  = 0;
                    break;
                }
                if (ch == ' ')
                {
                    break;
                }
                g_pc_uart_receive.flag = STATE_RECEIVE;
                g_pc_uart_receive.argv[g_pc_uart_receive.argc] = g_pc_uart_receive.buff + g_pc_uart_receive.len;
                g_pc_uart_receive.buff[g_pc_uart_receive.len++] = ch;
                break;          
            }     
        } 
    }
    return RT_EOK; 
}

/**
 * @function    pc指令执行线程
 * @param       void *parameter
 * @return      void
 * @created     Aprilhome,2019/4/19 
**/
void pc_execute_thread_entry(void *parameter)
{
    while (1)
    {
        rt_sem_take(g_pc_uart_sem, RT_WAITING_FOREVER);
        int i = 0;
        for (i = sizeof(g_pc_get_cmd)/sizeof(*g_pc_get_cmd) - 1; i >= 0; i--)
        {
            if (strcmp(g_pc_get_cmd[i].cmd_name, g_pc_uart_receive.argv[0]) == 0)
            {
                (*(g_pc_get_cmd[i].cmd_function))((g_pc_uart_receive.argv[1]),
                                                  (g_pc_uart_receive.argv[2]),
                                                  (g_pc_uart_receive.argv[3]),
                                                  (g_pc_uart_receive.argv[4]),
                                                  (g_pc_uart_receive.argv[5]),
                                                  (g_pc_uart_receive.argv[6]));
                break;
            }        
        }
        int j = 0;
        for (j = sizeof(g_pc_set_cmd)/sizeof(*g_pc_set_cmd) - 1; j >= 0; j--)
        {
            if (strcmp(g_pc_set_cmd[j].cmd_name, g_pc_uart_receive.argv[0]) == 0)
            {
                if ((SYSINFO.mode != MODE_SLEEP) && (SYSINFO.mode != MODE_FAST))
                {
                    rt_kprintf("$err %d\r\n",ERR_MODE);
                    break;
                }
                (*(g_pc_set_cmd[j].cmd_function))((g_pc_uart_receive.argv[1]),
                                                  (g_pc_uart_receive.argv[2]),
                                                  (g_pc_uart_receive.argv[3]),
                                                  (g_pc_uart_receive.argv[4]),
                                                  (g_pc_uart_receive.argv[5]),
                                                  (g_pc_uart_receive.argv[6]));
                 break;
            }        
        }
        if ((i < 0) && (j < 0))
        {
            rt_kprintf("$err %d\r\n",ERR_CMD);
        }
        g_pc_uart_receive.argc = 0;
        g_pc_uart_receive.len = 0;
        g_pc_uart_receive.flag = STATE_IDLE;
        
        rt_thread_mdelay(500);
    }
}

/**
  * @brief    与上位机通讯串口初始化              
  * @param    const char *name:串口名
              rt_uint32_t bound:波特率
  * @retval   0成功;-1失败   
  * @logs   
  * date        author        notes
  * 2019/8/21  Aprilhome
**/
int config_uart_pc(const char *name, rt_uint32_t bound)
{
    //1查找设备
    g_pc_uart_device = rt_device_find(name);
    if (g_pc_uart_device == RT_NULL)
    {
        rt_kprintf("find device %d failed!\r\n", name);
        return RT_ERROR;
    }
    //2配置设备
    struct serial_configure cfg = RT_SERIAL_CONFIG_DEFAULT;
    cfg.baud_rate = bound;
    if (RT_EOK != rt_device_control(g_pc_uart_device, RT_DEVICE_CTRL_CONFIG,(void *)&cfg)) 
    {
        rt_kprintf("control device %d failed!\r\n", name);
        return RT_ERROR;
    }
    //3打开设备中断接收轮询发送
    if (RT_EOK != rt_device_open(g_pc_uart_device, RT_DEVICE_FLAG_INT_RX))
    {
        rt_kprintf("open device %d failed!\r\n", name);
        return RT_ERROR;
    }
    //4设置回调函数
    if (RT_EOK != rt_device_set_rx_indicate(g_pc_uart_device, pc_uart_receive_callback))
    {
        rt_kprintf("indicate device %d failed!\r\n", name);
        return RT_ERROR;
    }
    //5初始化信号量
    g_pc_uart_sem = rt_sem_create("pc_uart_sem", 0, RT_IPC_FLAG_FIFO);

    pc_printf("$IoT human sensor%d\r\n", SYSINFO.id);
    return RT_EOK;   
}

/*******************************************************************************
 *pc指令处理函数1*
 *检查仪器状态、参数合法性、写入是否成功，并从FLASH中读取后输出
 ******************************************************************************/
void pc_checkin(char *argv1, char *argv2, char *argv3, char *argv4, char *argv5, char *argv6)
{
    pc_printf("$IoT human sensor%d\r\n", SYSINFO.id); 
}
     
void pc_set_id(char *argv1, char *argv2, char *argv3, char *argv4, char *argv5, char *argv6)
{
    g_sysinfo.id = atoi(argv1);
    if (config_sysinfo(&g_sysinfo) == -1)
    {
        pc_printf("$err %d\r\n",ERR_FLASH);
        return;
    }
    pc_printf("$node %d\r\n", SYSINFO.id);       
}

void pc_set_baudrate(char *argv1, char *argv2, char *argv3, char *argv4, char *argv5, char *argv6)
{
    uint32_t bound = atol(argv1);
    if ((bound != 1200) && (bound != 2400) && (bound != 4800) && (bound != 9600) &&
        (bound != 19200) && (bound != 38400) && (bound != 57600) && (bound != 115200) &&
        (bound != 230400) && (bound != 460800))
    {
        pc_printf("$err %d\r\n",ERR_PARA);
        return;
    }
    struct serial_configure pc_uart_config = 
    {
        bound,            
        DATA_BITS_8,      /* 8 databits */
        STOP_BITS_1,      /* 1 stopbit */
        PARITY_NONE,      /* No parity  */ 
        BIT_ORDER_LSB,    /* LSB first sent */
        NRZ_NORMAL,       /* Normal mode */
        PC_CMD_LEN_MAX,   /* Buffer size */
        0       
    };
    if (RT_EOK != rt_device_control(g_pc_uart_device, RT_DEVICE_CTRL_CONFIG,(void *)&pc_uart_config)) 
    {
        pc_printf("control device %d failed!\r\n");
        return;
    }
    else 
    {
        g_sysinfo.bound = bound;
        rt_thread_mdelay(500);
        pc_printf("$baudrate %ld\r\n", SYSINFO.bound);
    }
}

void pc_set_raw(char *argv1, char *argv2, char *argv3, char *argv4, char *argv5, char *argv6)
{
    g_sysinfo.raw = atoi(argv1);
    if (config_sysinfo(&g_sysinfo) == -1)
    {
        pc_printf("$err %d\r\n",ERR_FLASH);
    }
    pc_printf("$raw %d\r\n", SYSINFO.raw);    
}

void pc_set_sensor(char *argv1, char *argv2, char *argv3, char *argv4, char *argv5, char *argv6)
{
    uchar sensor = atoi(argv1);
    if ((sensor < 1) || (sensor > 5))
    {
        pc_printf("$err %d\r\n", ERR_PARA);
        return; 
    }
    g_sysinfo.sensor = sensor;
    if (config_sysinfo(&g_sysinfo) == -1)
    {
        pc_printf("$err %d\r\n",ERR_FLASH);
    }
    pc_printf("$sensor %d\r\n", SYSINFO.sensor);
}


/*******************************************************************************
 *pc指令处理函数2*需通用采集模块响应
 *直接转发给GC，在GC数据处理线程中，存入FLASH，并打印出来
 ******************************************************************************/
void pc_ds(char *argv1, char *argv2, char *argv3, char *argv4, char *argv5, char *argv6)
{
    pc_printf("\r\n========SZP1-1U V"VERSION"========\r\n");
    pc_printf("Hardver:"HARDWARE_VERSION"    ""Softver:"SOFTWARE_VERSION"\r\n");
    pc_printf("SN:%5d         Mode:%d\r\n",SYSINFO.id,SYSINFO.mode);
    pc_printf("Sensor:%d         Pressure:%d\r\n",SYSINFO.sensor,SYSINFO.pressure);
}

void pc_set_mode(char *argv1, char *argv2, char *argv3, char *argv4, char *argv5, char *argv6)
{
    uchar mode = atoi(argv1);
    switch (mode)
    {
    case MODE_SLEEP:
        g_sysinfo.mode = MODE_SLEEP;
        if (config_sysinfo(&g_sysinfo) == -1)
        {
            pc_printf("$err %d\r\n",ERR_FLASH);
        }
        gc_printf("setmode %d\r\n", 3);
        break;
    case MODE_FAST:
        g_sysinfo.mode = MODE_FAST;
        if (config_sysinfo(&g_sysinfo) == -1)
        {
            pc_printf("$err %d\r\n",ERR_FLASH);
        }
        if (SYSINFO.pressure == PRESSURE_KELLER)
        {
//            EN_PRE(1);
//            delay_ms(600);
//            pre_init();
        }
        gc_printf("setmode %d\r\n", 2);
        break;
    case MODE_AUTO:
        g_sysinfo.mode = MODE_AUTO;
        if (config_sysinfo(&g_sysinfo) == -1)
        {
            pc_printf("$err %d\r\n",ERR_FLASH);
        }
        if (SYSINFO.pressure == PRESSURE_KELLER)
        {
//            EN_PRE(1);
//            delay_ms(600);
//            pre_init();
        }
        gc_printf("setmode %d\r\n", 1);
        break;
    case MODE_SM:
        g_sysinfo.mode = MODE_SLEEP;
        if (config_sysinfo(&g_sysinfo) == -1)
        {
            pc_printf("$err %d\r\n",ERR_FLASH);
        }
        gc_printf("setmode %d\r\n", 3);
        break;
    default:
        printf("$err %d\r\n",ERR_PARA);
        return;
    }
}

void pc_takesample(char *argv1, char *argv2, char *argv3, char *argv4, char *argv5, char *argv6)
{
    pc_printf("takesample_all\r\n");
    rt_uint8_t reg[33] = {0};
    read_max30102_reg(reg);
    for (rt_uint8_t i = 0; i < 33; i++)
    {
        pc_printf("%02x ", reg[i]);
    }
}

void pc_takesample_spo2(char *argv1, char *argv2, char *argv3, char *argv4, char *argv5, char *argv6)
{
//    pc_printf("takesample_spo2\r\n");
//    rt_uint8_t reg[6] = {0};
//    read_max30102_fifo(reg);
//    for (rt_uint8_t i = 0; i < 6; i++)
//    {
//        pc_printf("%02x ", reg[i]);
//    }
    pc_printf("takesample_hr&spo2\r\n");
//    rt_sem_release(g_max30102_acquire_sem);
    max30102_acq();
}

void pc_takesample_hr(char *argv1, char *argv2, char *argv3, char *argv4, char *argv5, char *argv6)
{
    pc_printf("takesample_hr\r\n");
    rt_uint8_t reg[6] = {0};
    read_max30102_fifo(reg);
    for (rt_uint8_t i = 0; i < 6; i++)
    {
        pc_printf("%02x ", reg[i]);
    }
}

void pc_takesample_temp(char *argv1, char *argv2, char *argv3, char *argv4, char *argv5, char *argv6)
{
    pc_printf("takesample_temp\r\n");
//    rt_uint8_t reg[2] = {0};
//    read_max30102_temp(reg);
//    for (rt_uint8_t i = 0; i < 2; i++)
//    {
//        pc_printf("%02x ", reg[i]);
//    }
//    float t = reg[0] + 0.0625 * reg[1];
//    pc_printf("%f ", t);
    float temp = read_max30205_temperature();
    pc_printf("temp:%f", temp);
}



void pc_set_fre(char *argv1, char *argv2, char *argv3, char *argv4, char *argv5, char *argv6)
{
    gc_printf("setfre %f\r\n", atof(argv1));    
}

void pc_set_pump(char *argv1, char *argv2, char *argv3, char *argv4, char *argv5, char *argv6)
{
    gc_printf("setpump %d\r\n", atoi(argv1));    
}

void pc_set_st_pump(char *argv1, char *argv2, char *argv3, char *argv4, char *argv5, char *argv6)
{
    gc_printf("setst_pump %f\r\n", atof(argv1));    
}

void pc_set_fre_thr(char *argv1, char *argv2, char *argv3, char *argv4, char *argv5, char *argv6)
{
    gc_printf("setfre_thr %f\r\n", atof(argv1));    
}

/****************
 *printf重定向函数*
 ****************/
/**
 * @function    重定向fput函数，在USART1中使用printf函数
 * @para1       ch:输出字符
 * @para2       f:文件指针
 * @return      
 * @created     Aprilhome,2019/8/25 
**/
int fputc (int ch, FILE *f)
{
    while((USART2->ISR & 0x40) == 0); //循环发送，直到发送完毕
    USART2->TDR = (unsigned char) ch;
    return ch;
}

void gc_printf (char *fmt, ...)
{
    char bufffer[PRINTF_LEN + 1] = {0};  // CMD_BUFFER_LEN长度自己定义吧
    unsigned int i = 0;
    unsigned int len = 0;
    
    va_list arg_ptr;
    va_start(arg_ptr, fmt);  
    len = vsnprintf(bufffer, PRINTF_LEN + 1, fmt, arg_ptr);
    while ((i < PRINTF_LEN) && (i < len) && (len > 0))
    {
        while((USART2->ISR & 0x40) == 0); //循环发送，直到发送完毕
        USART2->TDR = bufffer[i++];
    }
    va_end(arg_ptr);
}

void pc_printf (char *fmt, ...)
{
    char bufffer[PRINTF_LEN + 1] = {0};  // CMD_BUFFER_LEN长度自己定义吧
    unsigned int i = 0;
    unsigned int len = 0;
    
    va_list arg_ptr;
    va_start(arg_ptr, fmt);  
    len = vsnprintf(bufffer, PRINTF_LEN + 1, fmt, arg_ptr);
    while ((i < PRINTF_LEN) && (i < len) && (len > 0))
    {
        while((PC_UART->ISR & 0x40) == 0); //循环发送，直到发送完毕
        PC_UART->TDR = bufffer[i++];
    }
    va_end(arg_ptr);
}

void u1_printf (char *fmt, ...)
{
    char bufffer[PRINTF_LEN + 1] = {0};  // CMD_BUFFER_LEN长度自己定义吧
    unsigned int i = 0;
    unsigned int len = 0;
    
    va_list arg_ptr;
    va_start(arg_ptr, fmt);  
    len = vsnprintf(bufffer, PRINTF_LEN + 1, fmt, arg_ptr);
    while ((i < PRINTF_LEN) && (i < len) && (len > 0))
    {
        while((USART1->ISR & 0x40) == 0); //循环发送，直到发送完毕
        USART1->TDR = bufffer[i++];
    }
    va_end(arg_ptr);
}

void u3_printf (char *fmt, ...)
{
    char bufffer[PRINTF_LEN + 1] = {0};  // CMD_BUFFER_LEN长度自己定义吧
    unsigned int i = 0;
    unsigned int len = 0;
    
    va_list arg_ptr;
    va_start(arg_ptr, fmt);  
    len = vsnprintf(bufffer, PRINTF_LEN + 1, fmt, arg_ptr);
    while ((i < PRINTF_LEN) && (i < len) && (len > 0))
    {
        while((USART3->ISR & 0x40) == 0); //循环发送，直到发送完毕
        USART3->TDR = bufffer[i++];
    }
    va_end(arg_ptr);
}

void u4_printf (char *fmt, ...)
{
    char bufffer[PRINTF_LEN + 1] = {0};  // CMD_BUFFER_LEN长度自己定义吧
    unsigned int i = 0;
    unsigned int len = 0;
    
    va_list arg_ptr;
    va_start(arg_ptr, fmt);  
    len = vsnprintf(bufffer, PRINTF_LEN + 1, fmt, arg_ptr);
    while ((i < PRINTF_LEN) && (i < len) && (len > 0))
    {
        while((UART4->ISR & 0x40) == 0); //循环发送，直到发送完毕
        UART4->TDR = bufffer[i++];
    }
    va_end(arg_ptr);
}

void u5_printf (char *fmt, ...)
{
    char bufffer[PRINTF_LEN + 1] = {0};  // CMD_BUFFER_LEN长度自己定义吧
    unsigned int i = 0;
    unsigned int len = 0;
    
    va_list arg_ptr;
    va_start(arg_ptr, fmt);  
    len = vsnprintf(bufffer, PRINTF_LEN + 1, fmt, arg_ptr);
    while ((i < PRINTF_LEN) && (i < len) && (len > 0))
    {
        while((UART5->ISR & 0x40) == 0); //循环发送，直到发送完毕
        UART5->TDR = bufffer[i++];
    }
    va_end(arg_ptr);
}

void lpu1_printf (char *fmt, ...)
{
    char bufffer[PRINTF_LEN + 1] = {0};  // CMD_BUFFER_LEN长度自己定义吧
    unsigned int i = 0;
    unsigned int len = 0;
    
    va_list arg_ptr;
    va_start(arg_ptr, fmt);  
    len = vsnprintf(bufffer, PRINTF_LEN + 1, fmt, arg_ptr);
    while ((i < PRINTF_LEN) && (i < len) && (len > 0))
    {
        while((LPUART1->ISR & 0x40) == 0); //循环发送，直到发送完毕
        LPUART1->TDR = bufffer[i++];
    }
    va_end(arg_ptr);
}