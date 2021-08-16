
//   timer a2 handling
//
//   author:          Petre Rodan <2b4eda@subdimension.ro>
//   available from:  https://github.com/rodan/
//   license:         BSD

#include "timer_a2.h"

volatile uint16_t timer_a2_last_event;        // bitwise flag of current timer_a2 interrupt events
volatile uint16_t timer_a2_trigger_schedule;  // bitwise flag of which slots are currently active
uint16_t timer_a2_last_event_schedule;        // bitwise flag of which slots have currently triggered

volatile uint32_t timer_a2_systime;           // time in 10ms increments since poweron
volatile uint32_t timer_a2_trigger_next;      // systime when timer_a2's scheduler is supposed to trigger next
uint32_t timer_a2_trigger_slot[TIMER_A2_SLOTS_COUNT]; // systimes at which slots are set to trigger

void timer_a2_init(void)
{
    __disable_interrupt();
    timer_a2_systime = 0;
    TA2CTL = TASSEL__ACLK + MC__CONTINOUS + TACLR;
    TA2EX0 = 0;
    __enable_interrupt();

    // ccr1 will count up to 10ms
    TA2CCTL1 = 0;
    TA2CCR1 = TA2R + TIMER_A2_TICK;
    TA2CCTL1 = CCIE;

    timer_a2_trigger_schedule = 0;
    timer_a2_trigger_next = -1;
    timer_a2_last_event = TIMER_A2_EVENT_NONE;
    timer_a2_systime = 0;
}


// slot is between 0 and TIMER_A2_SLOTS_COUNT-1 inclusive
uint8_t timer_a2_set_trigger_slot(const uint16_t slot, const uint32_t trigger, const uint8_t flag)
{
    if (slot > TIMER_A2_SLOTS_COUNT - 1) {
        return EXIT_FAILURE;
    }

    timer_a2_trigger_slot[slot] = trigger;
    if (flag == TIMER_A2_EVENT_ENABLE) {
        timer_a2_trigger_schedule |= (1UL << slot);
    } else {
        timer_a2_trigger_schedule &= ~(1UL << slot);
    }

    if (timer_a2_trigger_next > trigger) {
        timer_a2_trigger_next = trigger;
    }

    return EXIT_SUCCESS;
}

uint8_t timer_a2_get_trigger_slot(const uint16_t slot, uint32_t *trigger, uint8_t *flag)
{
    if (slot > TIMER_A2_SLOTS_COUNT - 1) {
        return EXIT_FAILURE;
    }

    *trigger = timer_a2_trigger_slot[slot];
    if (timer_a2_trigger_schedule & (1UL << slot)) {
        *flag = 1;
    } else {
        *flag = 0;
    }

    return EXIT_SUCCESS;
}

uint16_t timer_a2_get_event(void)
{
    return timer_a2_last_event;
}

void timer_a2_rst_event(void)
{
    timer_a2_last_event = TIMER_A2_EVENT_NONE;
}

uint16_t timer_a2_get_event_schedule(void)
{
    return timer_a2_last_event_schedule;
}

void timer_a2_rst_event_schedule(void)
{
    timer_a2_last_event_schedule = TIMER_A2_EVENT_NONE;
}

uint32_t timer_a2_get_trigger_next(void)
{
    return timer_a2_trigger_next;
}

uint32_t systime(void)
{
    return timer_a2_systime;
}

void timer_a2_scheduler_handler(void)
{
    uint16_t c;
    uint16_t shift;
    uint32_t schedule_trigger_next = -1;

    if (timer_a2_trigger_schedule) {
        for (c = 0; c < TIMER_A2_SLOTS_COUNT; c++) {
            shift = 1UL << c;
            if (timer_a2_trigger_schedule & shift) {
                // signal if event time was reached
                if (timer_a2_trigger_slot[c] <= timer_a2_systime) {
                    timer_a2_trigger_schedule &= ~shift;
                    timer_a2_last_event_schedule |= shift;
                } else 
                // prepare the next trigger
                if (timer_a2_trigger_slot[c] < schedule_trigger_next) {
                    schedule_trigger_next = timer_a2_trigger_slot[c];
                }
            }
        }
        timer_a2_trigger_next = schedule_trigger_next;
    }
}

__attribute__ ((interrupt(TIMER2_A1_VECTOR)))
void timer2_A1_ISR(void)
{
    uint16_t iv = TA2IV;

#ifdef LED_SYSTEM_STATES
    sig2_on;
#endif
    if (iv == TA2IV_TACCR1) {
        // disable interrupt
        TA2CCTL1 &= ~CCIE;
        TA2CCTL1 = 0;
        TA2CCR1 = TA2R + TIMER_A2_TICK;
        TA2CCTL1 = CCIE;
        timer_a2_systime++;

        if (timer_a2_trigger_next <= timer_a2_systime) {
            timer_a2_last_event |= TIMER_A2_EVENT_CCR1;
            _BIC_SR_IRQ(LPM3_bits);
        }
    } else if (iv == TA2IV_TAIFG) {
        // overflow
        TA2CTL &= ~TAIFG;
    }
#ifdef LED_SYSTEM_STATES
    sig2_off;
#endif
}
