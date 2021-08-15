#ifndef __TIMER_A0_H__
#define __TIMER_A0_H__

#include "proj.h"

//#define  _10ms  6666 // 65535*0.01/(12/8000000*65535) aka 0.01/(12/8000000)
#define  _1ms   250
#define  _2ms   500
#define  _4ms   1000 // 0.004/(32/8000000)
#define  _10ms  2500 // 65535*0.01/(32/8000000*65535) aka 0.01/(32/8000000)
#define  _20ms  5000
#define  _260ms  65000

#define TIMER_A0_EVENT_NONE  0
#define TIMER_A0_EVENT_CCR1  0x1
#define TIMER_A0_EVENT_CCR2  0x2
#define  TIMER_A0_EVENT_IFG  0x4

void timer_a0_init(void);
void timer_a0_halt(void);
void timer_a0_delay_noblk_ccr1(const uint16_t ticks);
void timer_a0_delay_noblk_ccr2(const uint16_t ticks);
void timer_a0_set_ccr2_extra_ticks(const uint32_t ticks);
void timer_a0_cancel_ccr2(void);
void timer_a0_sleep_nonblock(const uint16_t ms);
void uart0_disable_rx(void);

uint8_t timer_a0_get_event(void);
void timer_a0_rst_event(void);

#endif
