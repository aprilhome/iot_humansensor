#ifndef __DRV_BOARD_H__
#define __DRV_BOARD_H__

/*LED指示*/
#define LED_R_PIN           GET_PIN(C, 6)
#define LED_G_PIN           GET_PIN(C, 7)
#define LED_B_PIN           GET_PIN(B, 0)

/*电源使能*/
#define EN_TEMP_PIN         GET_PIN(B, 4)
#define EN_HR_PIN           GET_PIN(A, 12)
#define EN_SD_PIN           GET_PIN(C, 13)

/*通讯接口发送使能*/
#define EN_PC_T_PIN         GET_PIN(C, 3)
#define EN_FINSH_T_PIN      GET_PIN(B, 2)
#define EN_4G_T_PIN         GET_PIN(C, 3)




void config_switch(void);

#endif