#ifndef __PROJ_H__
#define __PROJ_H__

#include <msp430.h>
#include <stdlib.h>
#include <inttypes.h>
#include "adc.h"

#define sig0_on              P1OUT |= BIT2
#define sig0_off             P1OUT &= ~BIT2
#define sig0_switch          P1OUT ^= BIT2

#define st_on                P1OUT |= BIT2
#define st_off               P1OUT &= ~BIT2
#define st_switch            P1OUT ^= BIT2

#define pve_on               P1OUT |= BIT1
#define pve_off              P1OUT &= ~BIT1

// CE is active low
#define ce_on                P6OUT &= ~BIT0
#define ce_off               P6OUT |= BIT0

#define true                1
#define false               0

/*!
	\brief List of possible message types for the message bus.
	\sa sys_messagebus_register()
*/

#define                SYS_MSG_NULL  0
#define        SYS_MSG_TIMERA2_CCR1  0x1   // timer_a2 scheduler
#define          SYS_MSG_SCH_LED_ON  0x2   // timer_a2 schedule slot
#define         SYS_MSG_SCH_LED_OFF  0x4   // timer_a2 schedule slot
#define            SYS_MSG_UART1_RX  0x8   // UART received something
#define          SYS_MSG_RTC_MINUTE  0x10  // every minute
#define           SYS_MSG_RTC_ALARM  0x20  // for every alarm

#define             SCHEDULE_LED_ON  TIMER_A2_SLOT_0
#define            SCHEDULE_LED_OFF  TIMER_A2_SLOT_1

#define             LIPO_SLOPE  0.4460606
#define               PV_SLOPE  1.9061584

struct adc_conv {
    adc_channel lipo;
    adc_channel lead;
    adc_channel pv;
    adc_channel t_internal;
};

#endif
