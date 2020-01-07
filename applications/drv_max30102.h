#ifndef __DRV_MAX30102_H__
#define __DRV_MAX30102_H__

#define MAX30102_INT    GET_PIN(A, 8)

#define MAX30102_I2C_BUS_NAME     "i2c2"
#define MAX30102_ADDR             0xae

//register addresses
#define REG_INTR_STATUS_1       0x00
#define REG_INTR_STATUS_2       0x01
#define REG_INTR_ENABLE_1       0x02
#define REG_INTR_ENABLE_2       0x03
#define REG_FIFO_WR_PTR         0x04
#define REG_OVF_COUNTER         0x05
#define REG_FIFO_RD_PTR         0x06
#define REG_FIFO_DATA           0x07
#define REG_FIFO_CONFIG         0x08
#define REG_MODE_CONFIG         0x09
#define REG_SPO2_CONFIG         0x0A
#define REG_LED1_PA             0x0C
#define REG_LED2_PA             0x0D
#define REG_PILOT_PA            0x10
#define REG_MULTI_LED_CTRL1     0x11
#define REG_MULTI_LED_CTRL2     0x12
#define REG_TEMP_INTR           0x1F
#define REG_TEMP_FRAC           0x20
#define REG_TEMP_CONFIG         0x21
#define REG_PROX_INT_THRESH     0x30
#define REG_REV_ID              0xFE
#define REG_PART_ID             0xFF


void config_switch(void);
void max30102_init(void);
void read_max30102_reg(rt_uint8_t *reg);
void read_max30102_fifo(rt_uint8_t *reg);
void read_max30102_temp(rt_uint8_t *reg);

void max30102_acq(void);
void max30102_acquire_thread_entry(void *parameter);
#endif