#ifndef __API_PM_H__
#define __API_PM_H__

void pm_uart_execute_thread_entry(void *parameter);
int  init_uart_pm(const char *name, rt_uint32_t bound);

void pm_checkin(char *argv1, char *argv2, char *argv3, char *argv4, char *argv5, char *argv6);
void pm_ds(char *argv1, char *argv2, char *argv3, char *argv4, char *argv5, char *argv6);
void pm_set_id(char *argv1, char *argv2, char *argv3, char *argv4, char *argv5, char *argv6);
void pm_set_time(char *argv1, char *argv2, char *argv3, char *argv4, char *argv5, char *argv6);
void pm_set_baudrate(char *argv1, char *argv2, char *argv3, char *argv4, char *argv5, char *argv6);
void pm_set_mode(char *argv1, char *argv2, char *argv3, char *argv4, char *argv5, char *argv6);
void pm_takesample(char *argv1, char *argv2, char *argv3, char *argv4, char *argv5, char *argv6);
void pm_gettime(char *argv1, char *argv2, char *argv3, char *argv4, char *argv5, char *argv6);
void pm_getgps(char *argv1, char *argv2, char *argv3, char *argv4, char *argv5, char *argv6);
void pm_export(char *argv1, char *argv2, char *argv3, char *argv4, char *argv5, char *argv6);
void pm_set_interval (char *argv1, char *argv2, char *argv3, char *argv4, char *argv5, char *argv6);
void pm_list_files(char *argv1, char *argv2, char *argv3, char *argv4, char *argv5, char *argv6);
void pm_format(char *argv1, char *argv2, char *argv3, char *argv4, char *argv5, char *argv6);
void pm_getlast(char *argv1, char *argv2, char *argv3, char *argv4, char *argv5, char *argv6);
#endif

