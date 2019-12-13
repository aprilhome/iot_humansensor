#ifndef __DRV_BOARD_H__
#define __DRV_BOARD_H__

/*LED指示*/
#define LED_R_PIN           GET_PIN(C, 6)
#define LED_G_PIN           GET_PIN(C, 7)
#define LED_B_PIN           GET_PIN(B, 0)

/*电源使能*/
#define EN_TEMP_PIN         GET_PIN(B, 4)
#define EN_HR_PIN           GET_PIN(A, 12)
#define EN_4G_PIN           GET_PIN(B, 8)
#define EN_GPS_PIN          GET_PIN(B, 9)
#define EN_SD_PIN           GET_PIN(C, 13)

/*通讯接口发送使能*/
#define EN_PC_T_PIN         GET_PIN(C, 3)
#define EN_FINSH_T_PIN      GET_PIN(B, 2)
#define EN_4G_T_PIN         GET_PIN(C, 3)

/*----IO开关宏----*/
#define LED_R(n)        (n?rt_pin_write(LED_R_PIN, PIN_LOW):rt_pin_write(LED_R_PIN, PIN_HIGH))
#define LED_G(n)        (n?rt_pin_write(LED_G_PIN, PIN_LOW):rt_pin_write(LED_G_PIN, PIN_HIGH))
#define LED_B(n)        (n?rt_pin_write(LED_B_PIN, PIN_LOW):rt_pin_write(LED_B_PIN, PIN_HIGH))

#define EN_TEMP(n)      (n?rt_pin_write(EN_TEMP_PIN, PIN_HIGH):rt_pin_write(EN_TEMP_PIN, PIN_LOW))
#define EN_HR(n)        (n?rt_pin_write(EN_HR_PIN, PIN_HIGH):rt_pin_write(EN_HR_PIN, PIN_LOW))
#define EN_4G(n)        (n?rt_pin_write(EN_4G_PIN, PIN_HIGH):rt_pin_write(EN_4G_PIN, PIN_LOW))
#define EN_GPS(n)       (n?rt_pin_write(EN_GPS_PIN, PIN_HIGH):rt_pin_write(EN_GPS_PIN, PIN_LOW))
#define EN_SD(n)        (n?rt_pin_write(EN_SD_PIN, PIN_HIGH):rt_pin_write(EN_SD_PIN, PIN_LOW))

#define EN_PC_T(n)      (n?rt_pin_write(EN_PC_T_PIN, PIN_HIGH):rt_pin_write(EN_PC_T_PIN, PIN_LOW))
#define EN_FINSH_T(n)   (n?rt_pin_write(EN_FINSH_T_PIN, PIN_HIGH):rt_pin_write(EN_FINSH_T_PIN, PIN_LOW))
#define EN_4G_T(n)      (n?rt_pin_write(EN_4G_T_PIN, PIN_HIGH):rt_pin_write(EN_4G_T_PIN, PIN_LOW))


void config_switch(void);

#endif