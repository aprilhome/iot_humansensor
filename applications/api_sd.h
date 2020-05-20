#ifndef __API_SD_H__
#define __API_SD_H__

int write_sysinfo(void);
rt_int8_t write_sd(char *data, rt_uint16_t len);
rt_int8_t read_sd(char *data, rt_uint16_t len);

#endif