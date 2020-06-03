/**
  * Copyright (c) 2019, Aprilhome
  * 上位机通讯
  * -指令格式应为可见字符起始，回车或换行结束，指令参数间以空格分隔；
  * -可执行列表中两类指令，设置类指令需在待机状态下，读取类指令可任意时刻执行；
  * -指令异常时可以报错，根据提示信息判断操作是否有误;
  * -调用时仅需在config_uart_pm配置stm32串口号和波特率即可，打印调用pm_printf;
  * Change Logs:
  * Date           Author       Notes
  * 2019-04-20     Aprilhome    first version
**/

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>

#include <dfs_posix.h>
#include <string.h>
#include <time.h>

#include "drv_board.h"
#include "conf.h"
#include "api_pm.h"
#include "api_gps.h"
#include "api_timer.h"
#include "api_hr.h"
#include "drv_max30205.h"

extern char g_sample[1024];
extern rt_uint16_t g_sample_len;
const struct uart_execute g_pm_get_cmd[] =
{
    {"$checkin",          pm_checkin},
    {"$ds",               pm_ds},
    {"$gettime",          pm_gettime},
    {"$setmode",          pm_set_mode},
    {"$ts_hr",            pm_ts_hr},    
    {"$ts_temp",          pm_ts_temp},
    {"$getgps",           pm_getgps},
    {"$ts",               pm_ts},
};

const struct uart_execute g_pm_set_cmd[] = 
{
    {"$setid",            pm_set_id},
    {"$setbaudrate",      pm_set_baudrate},
    {"$settime",          pm_set_time},
    {"$setinterval",      pm_set_interval},
    {"$export",           pm_export},    
    {"$format",           pm_format},
    {"$ls",               pm_list_files},
};

/* 上位机通讯相关:串口,信号量,接收缓存 */
static rt_device_t g_pm_uart_device = RT_NULL;
static rt_sem_t    g_pm_uart_sem = RT_NULL;
uart_receive_t     g_pm_uart_receive = {{0}, 0, {0}, 0, STATE_IDLE};

/**
 * @function    上位机串口接收中断回调函数
 * @para1       dev:设备名
 * @para2       size:
 * @return      0成功
 * @created     Aprilhome,2019/8/25 
**/
static rt_err_t pm_uart_receive_callback(rt_device_t dev,rt_size_t size)
{
    rt_uint8_t ch;
    rt_size_t i;
    for (i = 0; i < size; i++)
    {
        if (rt_device_read(dev, -1, &ch, 1))
        {            
            switch (g_pm_uart_receive.flag)
            {
            case STATE_IDLE:
                if ((ch > 32) && (ch < 127))
                {
                    g_pm_uart_receive.flag = STATE_RECEIVE;
                    g_pm_uart_receive.argv[0] = g_pm_uart_receive.buff;
                    g_pm_uart_receive.argc = 0;
                    g_pm_uart_receive.buff[0] = ch;
                    g_pm_uart_receive.len = 1;
                }
                break;
            case STATE_RECEIVE:
                if ((ch == 0x0d) || (ch == 0x0a))
                {
                    g_pm_uart_receive.flag = STATE_IDLE;
                    g_pm_uart_receive.buff[g_pm_uart_receive.len++] = 0; 
                    g_pm_uart_receive.argc++;
                    rt_sem_release(g_pm_uart_sem);
                    break;
                }
                if ((g_pm_uart_receive.len > MAX_RECEIVE_LEN_PM) || (g_pm_uart_receive.argc > MAX_RECEIVE_ARGC_PM))
                {
                    g_pm_uart_receive.flag = STATE_IDLE;
                    g_pm_uart_receive.argc = 0;
                    g_pm_uart_receive.len  = 0;
                    break;
                }
                if ((ch < 32) || (ch > 127))
                {
                    g_pm_uart_receive.flag = STATE_IDLE;
                    g_pm_uart_receive.argc = 0;
                    g_pm_uart_receive.len  = 0;
                    break;
                }
                if (ch == ' ')
                {
                    g_pm_uart_receive.flag = STATE_SUSPEND;
                    g_pm_uart_receive.buff[g_pm_uart_receive.len++] = 0;
                    g_pm_uart_receive.argc++;
                    break;
                }
                g_pm_uart_receive.buff[g_pm_uart_receive.len++] = ch;
                break;
            case STATE_SUSPEND:
                if ((ch == 0x0d) || (ch == 0x0a))
                {
                    g_pm_uart_receive.flag = STATE_IDLE;
                    g_pm_uart_receive.argv[g_pm_uart_receive.argc] = g_pm_uart_receive.buff + g_pm_uart_receive.len;
                    g_pm_uart_receive.buff[g_pm_uart_receive.len++] = 0;
                    rt_sem_release(g_pm_uart_sem);
                    break;
                }
                if ((g_pm_uart_receive.len > MAX_RECEIVE_LEN_PM) || (g_pm_uart_receive.argc > MAX_RECEIVE_ARGC_PM))
                {
                    g_pm_uart_receive.flag = STATE_IDLE;
                    g_pm_uart_receive.argc = 0;
                    g_pm_uart_receive.len  = 0;
                    break;
                }
                if ((ch < 32)||(ch > 127))
                {
                    g_pm_uart_receive.flag = STATE_IDLE;
                    g_pm_uart_receive.argc = 0;
                    g_pm_uart_receive.len  = 0;
                    break;
                }
                if (ch == ' ')
                {
                    break;
                }
                g_pm_uart_receive.flag = STATE_RECEIVE;
                g_pm_uart_receive.argv[g_pm_uart_receive.argc] = g_pm_uart_receive.buff + g_pm_uart_receive.len;
                g_pm_uart_receive.buff[g_pm_uart_receive.len++] = ch;
                break;          
            }     
        } 
    }
    return RT_EOK; 
}

/**
 * @function    pm指令执行线程
 * @param       void *parameter
 * @return      void
 * @created     Aprilhome,2019/4/19 
**/
void pm_uart_execute_thread_entry(void *parameter)
{
    while (1)
    {
        rt_sem_take(g_pm_uart_sem, RT_WAITING_FOREVER);

        config_pm_t(1);
        int i = 0;
        for (i = sizeof(g_pm_get_cmd) / sizeof(*g_pm_get_cmd) - 1; i >= 0; i--)
        {
            if (strcmp(g_pm_get_cmd[i].uart_execute_cmd, g_pm_uart_receive.argv[0]) == 0)
            {
                (*(g_pm_get_cmd[i].uart_execute_function))((g_pm_uart_receive.argv[1]),
                                                           (g_pm_uart_receive.argv[2]),
                                                           (g_pm_uart_receive.argv[3]),
                                                           (g_pm_uart_receive.argv[4]),
                                                           (g_pm_uart_receive.argv[5]),
                                                           (g_pm_uart_receive.argv[6]));
                break;
            }        
        }
        int j = 0;
        for (j = sizeof(g_pm_set_cmd) / sizeof(*g_pm_set_cmd) - 1; j >= 0; j--)
        {
            if (strcmp(g_pm_set_cmd[j].uart_execute_cmd, g_pm_uart_receive.argv[0]) == 0)
            {
                if (SYSINFO.mode != MODE_SLEEP)
                {
                    pm_printf("$err %d\r\n",ERR_MODE);
                    break;
                }
                (*(g_pm_set_cmd[j].uart_execute_function))((g_pm_uart_receive.argv[1]),
                                                           (g_pm_uart_receive.argv[2]),
                                                           (g_pm_uart_receive.argv[3]),
                                                           (g_pm_uart_receive.argv[4]),
                                                           (g_pm_uart_receive.argv[5]),
                                                           (g_pm_uart_receive.argv[6]));
                 break;
            }        
        }
        if ((i < 0) && (j < 0))
        {
            pm_printf("$err %d\r\n", ERR_CMD);
        }
        g_pm_uart_receive.argc = 0;
        g_pm_uart_receive.len = 0;
        g_pm_uart_receive.flag = STATE_IDLE;
        
        config_pm_t(0);
        
        rt_thread_mdelay(20);
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
int init_uart_pm(const char *name, rt_uint32_t bound)
{
    //1查找设备
    g_pm_uart_device = rt_device_find(name);
    if (g_pm_uart_device == RT_NULL)
    {
        rt_kprintf("find device %d failed!\r\n", name);
        return RT_ERROR;
    }
    //2配置设备
    if ((bound != 2400) && (bound != 4800) && (bound != 9600) && (bound != 19200) && 
        (bound != 38400) && (bound != 57600) && (bound != 115200) && (bound != 230400) && 
        (bound != 460800))
    {
        bound = 115200;
        g_sysinfo.bound = bound;
        if (config_sysinfo(&g_sysinfo) == -1)
        {
            rt_kprintf("init uart pm failed\r\n");
        }
    }
    
    struct serial_configure cfg = RT_SERIAL_CONFIG_DEFAULT;
    cfg.baud_rate = bound;
    cfg.bufsz = MAX_RECEIVE_LEN_PM;
    if (RT_EOK != rt_device_control(g_pm_uart_device, RT_DEVICE_CTRL_CONFIG,(void *)&cfg)) 
    {
        rt_kprintf("control device %d failed!\r\n", name);
        return RT_ERROR;
    }
    //3打开设备中断接收轮询发送
    if (RT_EOK != rt_device_open(g_pm_uart_device, RT_DEVICE_FLAG_INT_RX))
    {
        rt_kprintf("open device %d failed!\r\n", name);
        return RT_ERROR;
    }
    //4设置回调函数
    if (RT_EOK != rt_device_set_rx_indicate(g_pm_uart_device, pm_uart_receive_callback))
    {
        rt_kprintf("indicate device %d failed!\r\n", name);
        return RT_ERROR;
    }
    //5初始化信号量
    g_pm_uart_sem = rt_sem_create("pm_uart_receive_sem", 0, RT_IPC_FLAG_FIFO);

    pm_printf("$pm uart initialized ok\r\n");
    
    return RT_EOK;   
}

/*******************************************************************************
 *pm指令处理函数1*
 *检查仪器状态、参数合法性、写入是否成功，并从FLASH中读取后输出
 ******************************************************************************/
void pm_checkin(char *argv1, char *argv2, char *argv3, char *argv4, char *argv5, char *argv6)
{
    pm_printf("$IoT_humansensor %d\r\n", SYSINFO.id); 
}
     
void pm_set_id(char *argv1, char *argv2, char *argv3, char *argv4, char *argv5, char *argv6)
{
    g_sysinfo.id = atoi(argv1);
    if (config_sysinfo(&g_sysinfo) == -1)
    {
        pm_printf("$err %d\r\n",ERR_FLASH);
        return;
    }
    pm_printf("$node %d\r\n", SYSINFO.id);       
}

void pm_set_time(char *argv1, char *argv2, char *argv3, char *argv4, char *argv5, char *argv6)
{
    rt_err_t ret = RT_EOK;
    uchar year, month, date, hour, minute, second;
    year   = atoi(argv1);
    month  = atoi(argv1 + 3);
    date   = atoi(argv1 + 6);
    hour   = atoi(argv2);
    minute = atoi(argv2 + 3);
    second = atoi(argv2 + 6);
    
    ret = set_date(year + 2000, month, date);
    if (ret != RT_EOK)
    {
        pm_printf("$err %d\r\n",ERR_PROCESS);
        return;
    }
    ret = set_time(hour, minute, second);
    if (ret != RT_EOK)
    {
        pm_printf("$err %d\r\n",ERR_PROCESS);
        return;
    }
    
    /* 获取时间 */
    time_t now;
    now = time(RT_NULL);
    struct tm *time;   
    time = localtime(&now);    
    pm_printf("ctime:%s\r\n", ctime(&now)); 

    char buff[80] = {0};
    strftime(buff, 80, "%y-%m-%d %H-%M-%S", time);
    pm_printf("strf:%s\r\n", buff);
    pm_printf("%02d-%02d-%02d %02d:%02d:%02d\r\n", 
              time->tm_year - 100, time->tm_mon + 1, time->tm_mday,
              time->tm_hour, time->tm_min, time->tm_sec);
    
}

void pm_gettime(char *argv1, char *argv2, char *argv3, char *argv4, char *argv5, char *argv6)
{   
    time_t now;
    /* output current time */
    now = time(RT_NULL);
    pm_printf("%s", ctime(&now));
}



void pm_getlast(char *argv1, char *argv2, char *argv3, char *argv4, char *argv5, char *argv6)
{
//    char data[1024] = {0};
//    rt_uint16_t data_len = 0;
//    data_len = sprintf(data, "$GNRMC %02d-%02d-%02d %02d:%02d:%02d %f %c %f %c %f %f %c\r\n",
//                       g_gps_data.year, g_gps_data.month, g_gps_data.day, 
//                       g_gps_data.hour, g_gps_data.minute, g_gps_data.second, 
//                       g_gps_data.latitude, g_gps_data.ns, g_gps_data.longitude, g_gps_data.ew, 
//                       g_gps_data.speed, g_gps_data.direction, g_gps_data.mode);
//    
//    memcpy(data + data_len, g_hr_data.data, g_hr_data.len);
//    for (rt_uint16_t i = 0; i < data_len + g_hr_data.len; i++)
//    {
//        pm_printf("%c", data[i]);
//    }
}

void pm_set_baudrate(char *argv1, char *argv2, char *argv3, char *argv4, char *argv5, char *argv6)
{
    uint32_t bound = atol(argv1);
    if ((bound != 2400) && (bound != 4800) && (bound != 9600) && (bound != 19200) && 
        (bound != 38400) && (bound != 57600) && (bound != 115200) && (bound != 230400) && 
        (bound != 460800))
    {
        pm_printf("$err %d\r\n",ERR_PARA);
        return;
    }
    struct serial_configure pm_uart_config = 
    {
        bound,            
        DATA_BITS_8,      /* 8 databits */
        STOP_BITS_1,      /* 1 stopbit */
        PARITY_NONE,      /* No parity  */ 
        BIT_ORDER_LSB,    /* LSB first sent */
        NRZ_NORMAL,       /* Normal mode */
        MAX_RECEIVE_LEN_PM,   /* Buffer size */
        0       
    };
    if (RT_EOK != rt_device_control(g_pm_uart_device, RT_DEVICE_CTRL_CONFIG, (void *)&pm_uart_config)) 
    {
        pm_printf("$err %d\r\n", ERR_PROCESS);
    }
    else 
    {
        g_sysinfo.bound = bound;
        rt_thread_mdelay(100);
        if (config_sysinfo(&g_sysinfo) == -1)
        {
            pm_printf("$err %d\r\n",ERR_FLASH);
            return;
        }
        pm_printf("$baudrate %ld\r\n", SYSINFO.bound);
    }
}

void pm_set_interval (char *argv1, char *argv2, char *argv3, char *argv4, char *argv5, char *argv6)
{
    g_sysinfo.interval = atof(argv1);
    if (config_sysinfo(&g_sysinfo) == -1)
    {
        pm_printf("$err %d\r\n",ERR_FLASH);
        return;
    }
    pm_printf("$interval %.1fs\r\n", SYSINFO.interval);  
}

/**
  * @brief   导出特定文件数据，输入指令时参数2必须填写，且填写正确的文件名（可通过$ls查询）     
  * @param        
  * @retval      
  * @logs   
  * date        author        notes
  * 2020/5/18  Aprilhome
**/
void pm_export(char *argv1, char *argv2, char *argv3, char *argv4, char *argv5, char *argv6)
{
    if (g_pm_uart_receive.argc != 2)
    {
        pm_printf("$err %d\r\n",ERR_PARA);
        return;
    }
    
    //取出文件名字，由于未开启相对路径，故需添加绝对路径名
    char name[SD_FILE_NAME_LEN] = "/";
    strcat(name, argv1);
    
    //串口输出数据
    uint32_t length;
    char buffer[81];
    int fd;
    /* 以读方式打开文件 */
    fd = open(name, O_RDONLY);
    if (fd < 0)
    {
        pm_printf("$err %d\r\n",ERR_SD);
        return;
    }    
    do
    {
        memset(buffer, 0, sizeof(buffer));
        length = read(fd, buffer, sizeof(buffer) - 1);
        rt_thread_mdelay(1);
        if (length > 0)
        {
            for (uint16_t i = 0; i < length; i++)
            {
                pm_printf("%c", buffer[i]);
            }
        }
    }
    while (length > 0);    
    close(fd);    
    pm_printf("$upload done\r\n");
}

void pm_format(char *argv1, char *argv2, char *argv3, char *argv4, char *argv5, char *argv6)
{
    int ret = 0;
    ret = dfs_mkfs("elm", "sd0");  
    if (ret == 0)
    {
        pm_printf("$format successed\r\n");
    }
    else
    {
        pm_printf("$err %d\r\n", ERR_SD);
    }    
}

void pm_list_files(char *argv1, char *argv2, char *argv3, char *argv4, char *argv5, char *argv6)
{
    /* sd卡内容读取 */
    DIR *dirp;
    struct dirent *d;   
    dirp = opendir("/");
    if (dirp == RT_NULL)
    {
        rt_kprintf("open directory error!\n");
    }
    else
    {
        pm_printf("files:\r\n");
        rt_uint8_t flag = 0;
        struct stat buf;
        char name[SD_FILE_NAME_LEN] = "/";
        /* 读取目录 */
        while ((d = readdir(dirp)) != RT_NULL)
        {
            flag = 1;
            memset(&buf, 0, sizeof(struct stat));
            memset(name, 0, sizeof(name));
            name[0] = '/';
            strcat(name, d->d_name);
            stat(name, &buf);
            pm_printf("%-20s %-25lu\r\n", d->d_name, buf.st_size);
//            pm_printf("%s\r\n", d->d_name);
        }
        if (flag == 0)
        {
            pm_printf("none\r\n");
        }

        /* 关闭目录 */
        closedir(dirp);
    }    
}

void pm_ds(char *argv1, char *argv2, char *argv3, char *argv4, char *argv5, char *argv6)
{
    /* 获取时间 */
    time_t now;
    now = time(RT_NULL);
    struct tm *time;   
    time = localtime(&now);    
    
    char buff[80] = {0};
    strftime(buff, 80, "%y-%m-%d %H:%M:%S", time);
        
    pm_printf("\r\n========IoT_humansensor V"VERSION"========\r\n");
    pm_printf("Hardver:"HARDWARE_VERSION"    ""Softver:"SOFTWARE_VERSION"\r\n");
    pm_printf("%s\r\n", buff);
    pm_printf("SN:%5d         bound:%ld\r\n", SYSINFO.id, SYSINFO.bound);
    pm_printf("mode:%d           interval:%.1f\r\n", SYSINFO.mode, SYSINFO.interval);
}

void pm_set_mode(char *argv1, char *argv2, char *argv3, char *argv4, char *argv5, char *argv6)
{
    uchar mode = atoi(argv1);
    switch (mode)
    {
    case MODE_SLEEP:
        g_sysinfo.mode = MODE_SLEEP;
        if (config_sysinfo(&g_sysinfo) == -1)
        {
            pm_printf("$err %d\r\n",ERR_FLASH);
            break;
        }
        config_hwtimer(0, 5);
        break;
    case MODE_NORMAL:
        g_sysinfo.mode = MODE_NORMAL;
        if (config_sysinfo(&g_sysinfo) == -1)
        {
            pm_printf("$err %d\r\n",ERR_FLASH);
            break;
        }
        EN_GPS(1);
        EN_TEMP(1);
        EN_HR(1);
        EN_SD(1);
        config_hwtimer(0, SYSINFO.interval);
        break;
    case MODE_AUTO:
        g_sysinfo.mode = MODE_AUTO;
        if (config_sysinfo(&g_sysinfo) == -1)
        {
            pm_printf("$err %d\r\n",ERR_FLASH);
            break;
        }
        EN_GPS(1);
        EN_TEMP(1);
        EN_HR(1);
        EN_SD(1); 
        config_hwtimer(1, SYSINFO.interval);
        break;
    case MODE_SM:
        g_sysinfo.mode = MODE_SM;
        if (config_sysinfo(&g_sysinfo) == -1)
        {
            pm_printf("$err %d\r\n",ERR_FLASH);
            break;
        }
        break;
    default:
        pm_printf("$err %d\r\n",ERR_PARA);
        return;
    }
    pm_printf("$mode %d\r\n",SYSINFO.mode);
}

/**
  * @brief    采集一次心率血压数据，指令响应时间小于20ms
              佩戴好后约10秒后可以正常输出 
              显示为0或255说明没有接触人体
  * @param        
  * @retval      
  * @logs   
  * date        author        notes
  * 2020/6/3  Aprilhome
**/
void pm_ts_hr(char *argv1, char *argv2, char *argv3, char *argv4, char *argv5, char *argv6)
{
    //发送读取心率指令
    char a[6] = {0xfd, 0, 0, 0, 0, 0};
    for (uint i = 0; i < 6; i++)
    {
        hr_printf("%c", a[i]);
    }
    
    //等待数据，超时500ms
    if (rt_sem_take(g_ts_hr_sem, 500) == RT_EOK)
    {
        //等待后把数据发出来
        pm_printf("%3d %3d %3d\r\n", g_hr_data.systolic_pressure, g_hr_data.diastolic_pressure, g_hr_data.hr);
    }
    else
    {
        pm_printf("$err %d\r\n", ERR_HR);
    }
}

void pm_ts_temp(char *argv1, char *argv2, char *argv3, char *argv4, char *argv5, char *argv6)
{
    float temp = 0;
    temp = read_max30205_temperature();
    if ((temp + 100) < 1)
    {
        pm_printf("$err %d\r\n", ERR_TEMP);
    }
    else
    {
        pm_printf("%.1f\r\n", temp);
    }
}

void pm_getgps(char *argv1, char *argv2, char *argv3, char *argv4, char *argv5, char *argv6)
{   
    rt_uint32_t e;
    rt_event_recv(g_sample_event, EVENT_GPS_RECV, RT_EVENT_FLAG_AND , 
                  rt_tick_from_millisecond(1500), &e);
    
    pm_printf("gps %c\r\n%02d-%02d-%02d %02d:%02d:%02d\r\n", g_gps_data.status,
              g_gps_data.year, g_gps_data.month, g_gps_data.day,
              g_gps_data.hour, g_gps_data.minute, g_gps_data.second);
    pm_printf("latitude:%lf, %c\r\n", g_gps_data.latitude, g_gps_data.ns);
    pm_printf("longitude:%lf, %c\r\n", g_gps_data.longitude, g_gps_data.ew);
    pm_printf("speed:%f, %f\r\n", g_gps_data.speed, g_gps_data.direction);
    pm_printf("pos_mode:%c\r\n", g_gps_data.mode);
}

/**
  * @brief    读取hr,temp,gps数据并输出
              若hr或temp接收异常，则输出为0
  * @param        
  * @retval      
  * @logs   
  * date        author        notes
  * 2020/6/3  Aprilhome
**/
void pm_ts(char *argv1, char *argv2, char *argv3, char *argv4, char *argv5, char *argv6)
{
    //发送读取心率指令
    char a[6] = {0xfd, 0, 0, 0, 0, 0};
    for (uint i = 0; i < 6; i++)
    {
        hr_printf("%c", a[i]);
    }
    
    //发送读取温度指令
    float temp = 0;
    temp = read_max30205_temperature();
    if  ((temp + 100) < 1)
    {
        temp = 0.0;
    }
    
    /* 获取时间 */
    time_t now;
    now = time(RT_NULL);
    struct tm *time;   
    time = localtime(&now);   
    
    //等待数据，超时500ms
    if (rt_sem_take(g_ts_hr_sem, 500) == RT_EOK)
    {
        //等待后把数据发出来
        pm_printf("%.1f %3d %3d %3d %lf %c %lf %c ", 
                  temp, g_hr_data.systolic_pressure, g_hr_data.diastolic_pressure, g_hr_data.hr,
                  g_gps_data.latitude, g_gps_data.ns, g_gps_data.longitude, g_gps_data.ew);
    }
    else
    {
         //等待后把数据发出来
        pm_printf("%.1f %3d %3d %3d %lf %c %lf %c\r\n", 
                  temp, 0, 0, 0,
                  g_gps_data.latitude, g_gps_data.ns, g_gps_data.longitude, g_gps_data.ew);

    }
    
    pm_printf("%02d-%02d-%02d %02d:%02d:%02d\r\n", 
              time->tm_year - 100, time->tm_mon + 1, time->tm_mday,
              time->tm_hour, time->tm_min, time->tm_sec);
}


