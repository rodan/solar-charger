
/*
    program that tests the functionality of the EUSCI A0 UART 

    tweak the baud rate in config.h
*/

#include <msp430.h>
#include <stdio.h>
#include <string.h>

#include "proj.h"
#include "driverlib.h"
#include "glue.h"
#include "ui.h"
#include "adc.h"
#include "rtc.h"
#include "pwr_mng.h"
#include "rtca_now.h"
#include "timer_a0.h"
#include "timer_a2.h"

struct adc_conv adc;

void port_init(void)
{
    // analog inputs
    P5SEL |= BIT1;
    P6SEL |= BIT1 | BIT2 | BIT3;

    // digital outputs
    P1DIR |= BIT1 | BIT2;
    P5DIR |= BIT0;
    P4DIR |= BIT2;
    P6DIR |= BIT0;

    // digital inputs
    P1DIR &= ~BIT5;
    // enable pullup resistor
    P1REN |= BIT5;
    P1OUT |= BIT5;
}

static void uart1_rx_irq(uint32_t msg)
{
    parse_user_input();
    uart1_set_eol();
}

void check_events(void)
{
    uint16_t msg = SYS_MSG_NULL;
    uint16_t ev;

    // RTC
    ev = rtca_get_event();
    if (ev) {
        if (rtca_get_event() & RTCA_EV_MINUTE) {
            msg |= SYS_MSG_RTC_MINUTE;
        }
        if (rtca_get_event() & RTCA_EV_ALARM) {
            msg |= SYS_MSG_RTC_ALARM;
        }
        rtca_rst_event();
    }

    // uart RX
    if (uart1_get_event() == UART1_EV_RX) {
        msg |= SYS_MSG_UART1_RX;
        uart1_rst_event();
    }

    // timer_a2
    ev = timer_a2_get_event();
    if (ev) {
        if (ev & TIMER_A2_EVENT_CCR1) {
            msg |= SYS_MSG_TIMERA2_CCR1;
        }
        timer_a2_rst_event();
    }

    // timer_a2-based scheduler
    ev = timer_a2_get_event_schedule();
    if (ev) {
        if (ev & (1 << SCHEDULE_LED_ON)) {
            msg |= SYS_MSG_SCH_LED_ON;
        }
        if (ev & (1 << SCHEDULE_LED_OFF)) {
            msg |= SYS_MSG_SCH_LED_OFF;
        }
        timer_a2_rst_event_schedule();
    }

    eh_exec(msg);
}

static void scheduler_irq(uint32_t msg)
{
    timer_a2_scheduler_handler();
}

static void led_on_irq(uint32_t msg)
{
    st_on;
    timer_a2_set_trigger_slot(SCHEDULE_LED_OFF, systime() + 200, TIMER_A2_EVENT_ENABLE);
}

static void led_off_irq(uint32_t msg)
{
    st_off;
    timer_a2_set_trigger_slot(SCHEDULE_LED_ON, systime() + 200, TIMER_A2_EVENT_ENABLE);
}

static void rtc_alarm(uint32_t msg)
{
    uart1_print("alarm!");
    uart1_print("\r\n");
    
    //led_on_irq(0);
    //st_on;
    //timer_a2_set_trigger_slot(SCHEDULE_LED_OFF, systime() + 100, TIMER_A2_EVENT_ENABLE);
}

static void main_loop(uint32_t msg)
{
    float ftemp;
    char itoa_buf[CONV_BASE_10_BUF_SZ];

    //sig0_on;
    adc10_read(1, &adc.lipo, REFVSEL_0);
    //adc10_read(2, &q_vbat, REFVSEL_0);
    adc10_read(3, &adc.pv, REFVSEL_0);
    //adc10_read(9, &q_th, REFVSEL_0);
    adc10_read(10, &adc.t_internal, REFVSEL_0);
    adc10_halt();
    //sig0_off;

    ftemp = (float) adc.lipo.counts_calib * LIPO_SLOPE;
    adc.lipo.conv = (uint16_t) ftemp;
    adc.lipo.calib = adc.lipo.conv; // FIXME

    ftemp = (float) adc.pv.counts_calib * PV_SLOPE;
    adc.pv.conv = (uint16_t) ftemp;
    adc.pv.calib = adc.pv.conv; // FIXME

    pwr_mng(&adc);

    uart1_print("lipo ");
    uart1_print(_utoa(itoa_buf, adc.lipo.counts));
    uart1_print(" ");
    uart1_print(_utoa(itoa_buf, adc.lipo.counts_calib));
    uart1_print(" ");
    uart1_print(_utoa(itoa_buf, adc.lipo.conv));
    uart1_print(", pv ");
    uart1_print(_utoa(itoa_buf, adc.pv.counts));
    uart1_print(" ");
    uart1_print(_utoa(itoa_buf, adc.pv.counts_calib));
    uart1_print(" ");
    uart1_print(_utoa(itoa_buf, adc.pv.conv));
    uart1_print(", tint ");
    uart1_print(_utoa(itoa_buf, adc.t_internal.counts));
    uart1_print(" ");
    uart1_print(_utoa(itoa_buf, adc.t_internal.conv));
    uart1_print("\r\n");
}

int main(void)
{
    // stop watchdog
    WDTCTL = WDTPW | WDTHOLD;
    msp430_hal_init();
    port_init();
    st_on;
    pve_on;

    clock_port_init();
    clock_init();

    rtca_init();

    uart1_port_init();
    uart1_init();

#ifdef UART1_RX_USES_RINGBUF
    uart1_set_rx_irq_handler(uart1_rx_ringbuf_handler);
#else
    uart1_set_rx_irq_handler(uart1_rx_simple_handler);
#endif

    timer_a0_init();
    timer_a2_init();

    st_off;
    //sig1_off;
    //sig2_off;
    //sig3_off;
#ifdef LED_SYSTEM_STATES
    sig4_on;
#else
    //sig4_off;
#endif

    eh_init();
    eh_register(&main_loop, SYS_MSG_RTC_MINUTE);
    eh_register(&rtc_alarm, SYS_MSG_RTC_ALARM);
    eh_register(&uart1_rx_irq, SYS_MSG_UART1_RX);

    eh_register(&scheduler_irq, SYS_MSG_TIMERA2_CCR1);
    eh_register(&led_off_irq, SYS_MSG_SCH_LED_OFF);
    eh_register(&led_on_irq, SYS_MSG_SCH_LED_ON);

    rtca_set_alarm(COMPILE_HOUR, COMPILE_MIN + 2);
    rtca_enable_alarm();

    display_menu();

    led_on_irq(0);

    while (1) {
        // sleep
#ifdef LED_SYSTEM_STATES
        sig4_off;
#endif
        _BIS_SR(LPM3_bits + GIE);
#ifdef LED_SYSTEM_STATES
        sig4_on;
#endif
        __no_operation();
//#ifdef USE_WATCHDOG
//        WDTCTL = (WDTCTL & 0xff) | WDTPW | WDTCNTCL;
//#endif
        check_events();
        check_events();
        check_events();
    }
}
