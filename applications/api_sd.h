#ifndef __API_SD_H__
#define __API_SD_H__

#define SD_FILE_NAME_LEN      256
#define RINGBUFFERSIZE      (4069)                  /* ringbuffer缓冲区大小 */
#define THRESHOLD           (RINGBUFFERSIZE / 2)    /* ringbuffer缓冲区阈值 */

int write_sysinfo(void);
int write_data_sd(struct rt_ringbuffer *buff);

#endif