/**
  * Copyright (c) 2019, Aprilhome
  * GPS通讯,GPS输出为$GPRMC或$GNRMC
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

#include "drv_board.h"
#include "conf.h"
#include "api_gps.h"

/* gps通讯相关:串口,信号量,缓存 */
static rt_device_t g_gps_uart_device = RT_NULL;
static rt_sem_t    g_gps_uart_sem = RT_NULL;
static uart_receive_t     g_gps_uart_receive = {{0}, 0, {0}, 0, STATE_IDLE};

gps_t g_gps_data = {0};

/**
 * @function  将一个字符串中部分字符串替换为其他字符串  
 * @para      char *str：待操作字符串
              char* find：待替换
              char *replace：替换为
 * @return    操作完后的字符串  
 * @created     Aprilhome,2020/5/5 
**/
static char* strrpl(char *str, char* find, char *replace)
{
    int i;
    char *pt = strstr(str, find);	
	char *firstStr=NULL;
   
    if(pt == NULL){
        printf("cannot find string \n");
        return NULL;
    }
	
	int len = strlen(str)+1+strlen(replace)-strlen(find);
	firstStr = (char* )malloc(len);
	memset(firstStr,0,len);

    // copy just until i find what i need to replace
    // i tried to specify the length of firstStr just with pt - str

    strncpy(firstStr, str, strlen(str) - strlen(pt)); 
    strcat(firstStr, replace);
    strcat(firstStr, pt + strlen(find));

    for(i = 0; i < strlen(firstStr); i++)
        str[i] = firstStr[i];
	
	free(firstStr);
	firstStr = NULL;
    return str;
}

/**
 * @function  解析GPS数据格式中的UTC时间，存入gps_t类型变量中，UTC变为UTC+8待处理
              5.6在analyse函数中处理
 * @para      char *buff:要解析的UTC时间
              gps_t *gps:解析完成后存入的变量
 * @return      
 * @created     Aprilhome,2020/5/5 
**/
static void get_gps_time(char *buff, gps_t *gps)
{
    char tmp[8] = {0};
    memcpy(tmp, buff, 2);
    gps->hour = atoi(tmp) + 8; //北京时间需在UTC+8
    memcpy(tmp, buff + 2, 2);
    gps->minute = atoi(tmp);
    memcpy(tmp, buff + 4, 2);
    gps->second = atoi(tmp);
    memcpy(tmp, buff + 7, 3);
    if(tmp[2] < '0' || tmp[2] > '9')
        tmp[2] = '0';
    gps->millisec = atoi(tmp);    
}

static void get_gps_date(char *buff, gps_t *gps)
{
    char tmp[8] = {0};
    memcpy(tmp, buff, 2);
    gps->day = atoi(tmp);
    memcpy(tmp, buff + 2, 2);
    gps->month = atoi(tmp);
    memcpy(tmp, buff + 4, 2);
    gps->year = atoi(tmp);  
}

/**
 * @function   解析GPS中经纬度（ddmm.mmmm）为float型 
 * @para       double *v：解析完的float地址 
               const char *buf：经纬度字符串地址
               int len：经纬度字符串大小
 * @return      
 * @created     Aprilhome,2020/5/5 
**/
static void get_gps_lat_long_to_double(double *v, const char *buf, int len)
{
    char	tmpbuf[16];
    double	tmpf, tmpi;
    
    memset(tmpbuf, 0, sizeof(tmpbuf));
    memcpy(tmpbuf, buf, len );
    tmpf = strtod( buf, (char**) NULL );
    tmpi = ((int)tmpf)/100;
    tmpf -= tmpi*100;
    tmpf = tmpf/60;
    tmpf += tmpi;
    *v = tmpf;
}

/**
  * @brief  计算GPS数组的校验和是否正确，通过所有参数做异或操作     
  * @param  rt_uint8_t *data：要计算的数组地址
            rt_uint16_t len:  要计算的数组长度
  * @retval 0，成功；-1失败     
  * @logs   
  * date        author        notes
  * 2020/4/19  Aprilhome
**/
static rt_int8_t checksum(char *data, rt_uint16_t len)
{
    rt_uint16_t sum = 0;
    char ch[2] = {0};
    for (rt_uint16_t i = 0; i < len; i++)
    {
        sum = sum ^ *(data + i);        
    }
    
    sprintf(ch, "%02X", sum);
    if (memcmp(ch, data + len + 1, 2) != 0)
        return -1;
    return 0;
}

/**
 * @function  解析GPS数据，$GNRMC或$GPRMC。数据长度是不固定的(最大70字节），因为没有的时候会直接到下
              一个','。 这样在替换时的缓存数据要超过70，若所有参数都没有，会添加出很多@。
              5.7修改不在判断V后直接返回，而是到结束，就是为了读到日期和时间
 * @para      char *buff：GPS原始数据缓存地址  
              rt_uint16_t len：GPS原始数据长度
              gps_t *gps_data：解析后存储的地址
 * @return      
 * @created     Aprilhome,2020/5/5 
**/

static rt_int8_t analyse_gps(char *buff, rt_uint16_t len, gps_t *gps_data)
{
    rt_int8_t ret = RT_EOK;
    char *ptr = NULL;
    //校验帧头，若buff有帧头，则将帧头的地址赋值给prt
    if (NULL == (ptr = strstr(buff, "$GPRMC")) && NULL == (ptr = strstr(buff,"$GNRMC")))
    {
        rt_kprintf("gps receive head failed\n");
        return -1;        
    }
    //校验和（$和*之间的字符）
    if (checksum(buff + 1, len - 6) != 0)
    {
        rt_kprintf("gps checksum failed\n");
        return -2;
    }
    //将连续逗号",,"替换为",@,"，便于分解字符串
    char tmp[MAX_RECEIVE_LEN_GPS] = {0};
    memcpy(tmp, ptr, strlen(ptr));
    while (strstr(tmp, ",,"))
    {
        strrpl(tmp, ",,", ",@,");
    }
//    pm_printf("%s", tmp);
    //分解字符串
    //0ID
    char *pch = strtok(tmp, ",");
    
    //1 time
    pch = strtok(NULL, ",");
    get_gps_time(pch, gps_data);
    
    //2 status
    pch = strtok(NULL, ",");
    gps_data->status = *pch;
    if (*pch == 'V')
    {
//        rt_kprintf("gps positioning failed\n");
        ret = -3;
    }
    
    //3 latitude
    pch = strtok(NULL, ",");
    get_gps_lat_long_to_double(&gps_data->latitude, pch, strlen(pch));
    
    //4 latitude direction
    pch = strtok(NULL, ",");
    gps_data->ns = *pch;
    
    //5 longitude
    pch = strtok(NULL, ",");
    get_gps_lat_long_to_double(&gps_data->longitude, pch, strlen(pch));
    
    //6 longitude direction
    pch = strtok(NULL, ",");
    gps_data->ew = *pch;
    
    //7 speed
    pch = strtok(NULL, ",");
    gps_data->speed = 1.852 * strtof(pch, (char **) NULL ) / 3.6;
    
    //8 direction
    pch = strtok(NULL, ",");
    gps_data->direction = strtof(pch, (char**)NULL);
    
    //9 UTC日期，ddmmyy,处理前面UTC+8
    pch = strtok(NULL, ",");
    get_gps_date(pch, gps_data);
    if(gps_data->hour > 23)
    {
        gps_data->hour -= 24;
        gps_data->day += 1;
    }
    
    //10 磁偏角,000.0-180.0,不处理
    pch = strtok(NULL, ",");
    
    //11 磁偏角方向,E/W,不处理
    pch = strtok(NULL, ",");
    
    //12 模式指示A,D,E,N
    pch = strtok(NULL, ",");
    gps_data->mode = *pch;
    
//    pm_printf("")
    
    return ret;    
}

/**
 * @function    gps串口接收中断回调函数，数据存入缓存中，长度为GPS实际发送长度-2（不包括[CRLF]）
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
                    g_gps_uart_receive.buff[0] = ch;
                    g_gps_uart_receive.len = 1;
                }
                break;
            case STATE_RECEIVE:
                if (g_gps_uart_receive.len >= MAX_RECEIVE_LEN_GPS)
                {
                    g_gps_uart_receive.flag = STATE_IDLE;
                    g_gps_uart_receive.len  = 0;
                    break;
                }
                if (ch == 0x0a)
                {
                    g_gps_uart_receive.flag = STATE_IDLE;
                    g_gps_uart_receive.buff[g_gps_uart_receive.len++] = ch;
                    g_gps_uart_receive.buff[g_gps_uart_receive.len] = 0;
                    rt_sem_release(g_gps_uart_sem);
                    break;
                }
                g_gps_uart_receive.buff[g_gps_uart_receive.len++] = ch;
                break;
            default:
                g_gps_uart_receive.flag = STATE_IDLE;
                g_gps_uart_receive.len  = 0;
                break;
            }     
        } 
    }
    return RT_EOK; 
}

/**
 * @function    gps数据处理线程
                5.7添加上电后校时，和正点校时，并修改判断校时条件由定位成功改为有正确时间信息
 * @param       void *parameter
 * @return      void
 * @created     Aprilhome,2019/4/19 
**/
void gps_execute_thread_entry(void *parameter)
{
    while (1)
    {
        rt_sem_take(g_gps_uart_sem, RT_WAITING_FOREVER);
        
        //解析GPS数据包，并存入全局变量
        g_gps_data.state = analyse_gps(g_gps_uart_receive.buff, g_gps_uart_receive.len, &g_gps_data);
        
        //对系统进行校时，前提解析有正确的日期和时间
        if ((g_gps_data.month >= 1) && (g_gps_data.month <= 12) && (g_gps_data.minute < 60))
        {            
            static rt_uint8_t first = 0; // 静态变量使上电第一次运行时进行校时
            // 上电,24点/12点校时
            if ((first == 0) || 
                (((g_gps_data.hour == 12) || (g_gps_data.hour == 24)) && 
                 (g_gps_data.minute == 0) && 
                 (g_gps_data.second == 0)))
            {
                set_date(g_gps_data.year + 2000, g_gps_data.month, g_gps_data.day);
                set_time(g_gps_data.hour, g_gps_data.minute, g_gps_data.second);
                rt_kprintf("calibrate time\n");
            }
            first = 1;
        }
//        rt_kprintf("gps received\n");
        rt_event_send(g_sample_event, EVENT_GPS_RECV);
        rt_thread_mdelay(20);
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
int init_uart_gps(const char *name, rt_uint32_t bound)
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
    cfg.bufsz = MAX_RECEIVE_LEN_GPS;
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

    pm_printf("gps uart initialized ok\r\n");
    return RT_EOK;   
}
