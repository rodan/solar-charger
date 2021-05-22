#ifndef __PROJ_H__
#define __PROJ_H__

#include <msp430.h>
#include <stdlib.h>
#include "config.h"

#define USB_MCLK_FREQ       4000000

#define I2C_MASTER_DIR      P6DIR
#define I2C_MASTER_OUT      P6OUT
#define I2C_MASTER_IN       P6IN

//port pins
#define I2C_MASTER_SCL      BIT2
#define I2C_MASTER_SDA      BIT3

//Start and Stop delay, most devices need this
#define I2C_MASTER_SDLY    0x01
//for long lines or very fast MCLK, unremark and set
#define I2C_MASTER_DDLY    0x02

#define charge_enable       P2OUT |= BIT0
#define charge_disable      P2OUT &= ~BIT0
#define opt_power_enable    P1OUT &= ~BIT6
#define opt_power_disable   P1OUT |= BIT6; I2C_MASTER_DIR &= ~(I2C_MASTER_SCL + I2C_MASTER_SDA);

#define SYS_MSG_NULL 0
#define SYS_MSG_RTC_MINUTE 0x1

void main_init(void);
void sleep(void);
void wake_up(void);
void check_events(void);

#endif
