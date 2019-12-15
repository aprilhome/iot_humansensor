#ifndef __DRV_GPS_H__
#define __DRV_GPS_H__

#define GPS_CMD_LEN_MAX         200
#define GPS_CMD_ARGC_MAX        20

void gps_execute_thread_entry(void *parameter);
int config_uart_gps(const char *name, rt_uint32_t bound);
void gps_gnrmc(void);

#endif