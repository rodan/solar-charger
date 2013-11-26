#ifndef __PROJ_H__
#define __PROJ_H__

#include <msp430.h>
#include <stdlib.h>
#include "config.h"

#define USB_MCLK_FREQ       4000000

#define charge_enable       P2OUT |= BIT0
#define charge_disable      P2OUT &= ~BIT0
#define opt_power_enable    P1OUT &= ~BIT6
#define opt_power_disable   P1OUT |= BIT6

void main_init(void);
void sleep(void);
void wake_up(void);
void check_events(void);

#endif
