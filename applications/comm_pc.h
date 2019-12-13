#ifndef __PC_COMM_H__
#define __PC_COMM_H__

#define PC_CMD_LEN_MAX          200
#define PC_CMD_ARGC_MAX         20

void pc_execute_thread_entry(void *parameter);
int  config_uart_pc(const char *name, rt_uint32_t bound);

void pc_checkin(char *argv1, char *argv2, char *argv3, char *argv4, char *argv5, char *argv6);
void pc_ds(char *argv1, char *argv2, char *argv3, char *argv4, char *argv5, char *argv6);
void pc_set_id(char *argv1, char *argv2, char *argv3, char *argv4, char *argv5, char *argv6);
void pc_set_baudrate(char *argv1, char *argv2, char *argv3, char *argv4, char *argv5, char *argv6);
void pc_set_mode(char *argv1, char *argv2, char *argv3, char *argv4, char *argv5, char *argv6);

void pc_set_fre(char *argv1, char *argv2, char *argv3, char *argv4, char *argv5, char *argv6);

void pc_takesample(char *argv1, char *argv2, char *argv3, char *argv4, char *argv5, char *argv6);
void pc_takesample_spo2(char *argv1, char *argv2, char *argv3, char *argv4, char *argv5, char *argv6);
void pc_takesample_hr(char *argv1, char *argv2, char *argv3, char *argv4, char *argv5, char *argv6);
void pc_takesample_temp(char *argv1, char *argv2, char *argv3, char *argv4, char *argv5, char *argv6);


#define PRINTF_LEN      200
void gc_printf (char *fmt, ...);
void pc_printf (char *fmt, ...);
void u1_printf (char *fmt, ...);
void u2_printf (char *fmt, ...);
void u3_printf (char *fmt, ...);
void u4_printf (char *fmt, ...);
void u5_printf (char *fmt, ...);
void lpu1_printf (char *fmt, ...);

#endif