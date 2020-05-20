#ifndef __API_GPS_H__
#define __API_GPS_H__

typedef struct
{
    rt_int8_t  state;
    
    rt_uint8_t	year;
    rt_uint8_t 	month;
    rt_uint8_t	day;
    rt_uint8_t	hour;
    rt_uint8_t	minute;
    rt_uint8_t	second;
    rt_uint8_t	millisec;
    rt_uint8_t  status;
    double       latitude;
    double       longitude;
    rt_uint8_t  ns;
    rt_uint8_t  ew;
    float       speed;
    float       direction;
    rt_uint8_t  mode;    
}gps_t;

#define SEPARATOR_GPS               0x30

extern gps_t g_gps_data;

void gps_execute_thread_entry(void *parameter);
int init_uart_gps(const char *name, rt_uint32_t bound);

#endif