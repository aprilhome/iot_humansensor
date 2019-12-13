/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 * 
 * Change Logs:
 * Date           Author       Notes
 * 2018-11-06     SummerGift   first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>

#include "drv_board.h"
#include "conf.h"
#include "comm_pc.h"

int main(void)
{   
    config_switch();
    config_uart_pc("uart2", 115200);
    LED_R(0);
    pc_printf("$IoT human sensor\r\n");
    
    return RT_EOK;
}
