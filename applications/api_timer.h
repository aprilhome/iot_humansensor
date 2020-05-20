#ifndef __API_TIMER_H__
#define __API_TIMER_H__

rt_err_t timeout_callback(rt_device_t dev, rt_size_t size);
void hwtimer_thread_entry(void *parameter);
int init_timer(const char *name);
int config_hwtimer(rt_uint8_t a, float timeout);

#endif