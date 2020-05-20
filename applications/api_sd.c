#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>

#include <dfs_posix.h>

#include "conf.h"

int write_sysinfo(void)
{
    int ret = RT_EOK;
    int fd, size;
    char buff[256];
    
    /* 以读写方式打开文件，若不存在则创建该文件 */
    fd = open("/sysinfo.md", O_RDWR | O_CREAT);
    //打开成功
    if (fd >= 0)
    {
        write(fd, &SYSINFO, sizeof(sys_info_t));
        rt_kprintf("write sd done\n");
        
        size = read(fd, buff, sizeof(buff));
        close(fd);
        rt_kprintf("Read from file : %s \n", buff);
        if (size < 0)
            return 1;
    }
        
    return ret;
}

rt_int8_t write_sd(char *data, rt_uint16_t len)
{
    /* 以读写方式打开文件，若不存在则创建该文件 */
    int fd = open("/1.dat", O_WRONLY | O_CREAT | O_APPEND);
    //打开成功
    if (fd >= 0)
    {
        write(fd, data, len);
        rt_kprintf("write sd done\n");
        close(fd);
        return 0;
    }  
    else
        return -1;
}

rt_int8_t read_sd(char *data, rt_uint16_t len)
{
    rt_int16_t size = 0;
    /* 以读写方式打开文件，若不存在则创建该文件 */
    int fd = open("/1.dat", O_RDONLY);
    //打开成功
    if (fd >= 0)
    {
        size = read(fd, data, len);
        close(fd);
        if (size <= 0)
            return -2;
        else
            return 0;
    } 
    else
        return -1;
}