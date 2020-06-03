#ifndef __API_HR_H__
#define __API_HR_H__

typedef struct
{
    char data[MAX_RECEIVE_LEN_HR];
    rt_uint16_t len;
    uint8_t head;
    uint8_t systolic_pressure;
    uint8_t diastolic_pressure;
    uint8_t hr;
}hr_t;

extern hr_t g_hr_data;
extern rt_sem_t g_ts_hr_sem;
void hr_execute_thread_entry(void *parameter);
int init_uart_hr(const char *name, rt_uint32_t bound);

#endif