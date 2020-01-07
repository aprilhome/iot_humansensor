#ifndef __DRV_MAX30205_H__
#define __DRV_MAX30205_H__

#define MAX30205_I2C_BUS_NAME     "i2c3"
#define MAX30205_ADDR             0x90

// Registers
#define MAX30205_TEMPERATURE    0x00  //  get temperature ,Read only
#define MAX30205_CONFIGURATION  0x01  //
#define MAX30205_THYST          0x02  //
#define MAX30205_TOS            0x03  //

void max30205_init(void);
float read_max30205_temperature(void);

#endif