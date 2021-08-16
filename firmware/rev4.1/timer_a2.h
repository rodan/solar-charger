#ifndef __TIMER_A2_H__
#define __TIMER_A2_H__

#include "proj.h"

// tick interval is about 10ms
// 327*(1/32768) = .009979248046875 s
#define            TIMER_A2_TICK  327

// 1ms in clock ticks
// 65535*0.001/(32/8000000*65535) = 0.001/(32/8000000) = 250

#define      TIMER_A2_EVENT_NONE  0x0
#define      TIMER_A2_EVENT_CCR1  0x1

#define    TIMER_A2_EVENT_ENABLE  0x1
#define   TIMER_A2_EVENT_DISABLE  0x0

#define     TIMER_A2_SLOTS_COUNT  16
#define          TIMER_A2_SLOT_0  0
#define          TIMER_A2_SLOT_1  1
#define          TIMER_A2_SLOT_2  2
#define          TIMER_A2_SLOT_3  3
#define          TIMER_A2_SLOT_4  4
#define          TIMER_A2_SLOT_5  5
#define          TIMER_A2_SLOT_6  6
#define          TIMER_A2_SLOT_7  7
#define          TIMER_A2_SLOT_8  8
#define          TIMER_A2_SLOT_9  9
#define         TIMER_A2_SLOT_10  10
#define         TIMER_A2_SLOT_11  11
#define         TIMER_A2_SLOT_12  12
#define         TIMER_A2_SLOT_13  13
#define         TIMER_A2_SLOT_14  14
#define         TIMER_A2_SLOT_15  15


void timer_a2_init(void);

uint16_t timer_a2_get_event(void);
void timer_a2_rst_event(void);
uint16_t timer_a2_get_event_schedule(void);
void timer_a2_rst_event_schedule(void);
uint32_t timer_a2_get_trigger_next(void);

uint32_t systime(void);

uint8_t timer_a2_set_trigger_slot(const uint16_t slot, const uint32_t trigger, const uint8_t flag);
uint8_t timer_a2_get_trigger_slot(const uint16_t slot, uint32_t *trigger, uint8_t *flag);
void timer_a2_scheduler_handler(void);

#endif
