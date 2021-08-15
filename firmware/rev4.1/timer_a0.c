
//   timer a0 handling
//   CCR1 is used for timer_a0_delay_noblk_ccr1()
//   CCR2 is used for timer_a0_delay_noblk_ccr2()
//
//   author:          Petre Rodan <2b4eda@subdimension.ro>
//   available from:  https://github.com/rodan/
//   license:         BSD

#include "timer_a0.h"

#define     TA0_SLEEP_MS_COMPENSATION  5       // number of ticks spent setting up the timer during timer_a0_sleep()

volatile uint32_t timer_a0_ccr2_extra_ticks;
volatile uint8_t timer_a0_last_event;
volatile uint16_t timer_a0_ovf;

void timer_a0_init(void)
{

    // ID__4 & TAIDEX_2
    // 1 tick is TA0_DIVIDER / MCLK == 12/8000000 = 1.5 us
    // overflow happens after .0015*65535 = 98.30 ms

    // ID__8 & TAIDEX_3
    // 1 tick is TA0_DIVIDER / MCLK == 32/8000000 = 4 us
    // overflow happens after .004*65535 = 262.14 ms

    __disable_interrupt();
    timer_a0_ovf = 0;
    timer_a0_ccr2_extra_ticks = 0;
    TA0CTL = TASSEL__SMCLK + MC__CONTINOUS + TACLR + ID__8; // divide SMCLK by 8
#if defined (SMCLK_FREQ_8M) 
    TA0EX0 = TAIDEX_3; // further divide SMCLK by 4
#elif defined (SMCLK_FREQ_16M)
    TA0EX0 = TAIDEX_7; // further divide SMCLK by 8
#endif
    __enable_interrupt();
}

void timer_a0_set_ccr2_extra_ticks(const uint32_t ticks)
{
    timer_a0_ccr2_extra_ticks = ticks;
}

void timer_a0_cancel_ccr2(void)
{
    TA0CCTL2 &= ~CCIE;
    TA0CCTL2 = 0;
    timer_a0_ccr2_extra_ticks = 0;
}

uint8_t timer_a0_get_event(void)
{
    return timer_a0_last_event;
}

void timer_a0_rst_event(void)
{
    timer_a0_last_event = TIMER_A0_EVENT_NONE;
}

void timer_a0_delay_noblk_ccr1(const uint16_t ticks)
{
    TA0CCTL1 &= ~CCIE;
    TA0CCTL1 = 0;
    TA0CCR1 = TA0R + ticks;
    TA0CCTL1 = CCIE;
}

void timer_a0_delay_noblk_ccr2(const uint16_t ticks)
{
    TA0CCTL2 &= ~CCIE;
    TA0CCTL2 = 0;
    TA0CCR2 = TA0R + ticks;
    TA0CCTL2 = CCIE;
}

void timer_a0_sleep_nonblock(const uint16_t ms)
{
    uint32_t ticks, d;

    timer_a0_cancel_ccr2();

    ticks = 250 * (uint32_t) ms;

    if (ticks > 65000) {
        d = ticks / 65000;
        timer_a0_set_ccr2_extra_ticks(ticks - (d * TA0_SLEEP_MS_COMPENSATION));
        timer_a0_delay_noblk_ccr2(10);
    } else {
        timer_a0_set_ccr2_extra_ticks(ticks - TA0_SLEEP_MS_COMPENSATION);
        timer_a0_delay_noblk_ccr2(10);
    }
}

__attribute__ ((interrupt(TIMER0_A1_VECTOR)))
void timer0_A1_ISR(void)
{
#ifdef LED_SYSTEM_STATES
    sig2_on;
#endif
    uint16_t iv = TA0IV;
#if defined TAIV__TACCR1
    if (iv == TAIV__TACCR1) {
#elif defined TA0IV_TACCR1
    if (iv == TA0IV_TACCR1) {
#endif
        // timer used by timer_a0_delay_noblk_ccr1()
        // disable interrupt
        TA0CCTL1 &= ~CCIE;
        TA0CCTL1 = 0;
        timer_a0_last_event |= TIMER_A0_EVENT_CCR1;
        //uart0_disable_rx();
        _BIC_SR_IRQ(LPM3_bits);
#if defined TAIV__TACCR2
    } else if (iv == TAIV__TACCR2) {
#elif defined TA0IV_TACCR2
    } else if (iv == TA0IV_TACCR2) {
#endif
        // timer used by timer_a0_delay_noblk_ccr2()
        if (timer_a0_ccr2_extra_ticks) {
            if (timer_a0_ccr2_extra_ticks > 65000) {
                TA0CCR2 = TA0R + 65000;
                timer_a0_ccr2_extra_ticks -= 65000;
            } else {
                TA0CCR2 = TA0R + timer_a0_ccr2_extra_ticks;
                timer_a0_ccr2_extra_ticks = 0;
            }
        } else {
            // disable interrupt
            TA0CCTL2 &= ~CCIE;
            TA0CCTL2 = 0;
            timer_a0_last_event |= TIMER_A0_EVENT_CCR2;
            _BIC_SR_IRQ(LPM3_bits);
        }
    } else if (iv == TA0IV_TA0IFG) {
        TA0CTL &= ~TAIFG;
        timer_a0_ovf++;
        //timer_a0_last_event |= TIMER_A0_EVENT_IFG;
        _BIC_SR_IRQ(LPM3_bits);
    }
#ifdef LED_SYSTEM_STATES
    sig2_off;
#endif
}
