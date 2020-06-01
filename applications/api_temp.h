#ifndef __API_TEMP_H__
#define __API_TEMP_H__

typedef struct
{
    char data[MAX_RECEIVE_LEN_TEMP];
    rt_uint16_t len;
}temp_t;

extern temp_t g_temp_data;
extern rt_sem_t g_ts_temp_sem;
void temp_execute_thread_entry(void *parameter);
int init_uart_temp(const char *name, rt_uint32_t bound);

#endif