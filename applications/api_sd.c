#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>

#include <dfs_posix.h>

#include "conf.h"
#include "api_sd.h"

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

/**
  * @brief     将环形缓冲区数据写入sd卡，注意阈值和环形缓冲区大小是宏定义   
  * @param     struct rt_ringbuffer *buff：环形缓冲区   
  * @retval    0：成功；-1失败  
  * @logs   
  * date        author        notes
  * 2020/5/22  Aprilhome
**/
int write_data_sd(struct rt_ringbuffer *buff)
{
    rt_uint8_t  writebuffer[THRESHOLD] = {0};
    rt_size_t size;
    
    //获取文件名，每天一个文件
    time_t now;
    now = time(RT_NULL);
    struct tm *time;   
    time = localtime(&now);
    
    char name[20] = {0};
    sprintf(name, "/%02d%02d%02d.dat", time->tm_year - 100,  time->tm_mon + 1, time->tm_mday);
    
    //打开或新建文件
    int fd = open(name, O_WRONLY | O_CREAT | O_APPEND);
    if (fd >= 0)
    {
        while(rt_ringbuffer_data_len(buff))
        {
            size = rt_ringbuffer_get(buff, (rt_uint8_t *)writebuffer, THRESHOLD);
            write(fd, writebuffer, size);
        }       
        close(fd);
    }  
    else
    {
        return -1;
    }  
    return 0;
}