
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

uint8_t port1_last_event;
struct adc_conv adc;
uart_descriptor bc; // backchannel uart interface

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

    // IRQ triggers on rising edge
    P1IES &= ~BIT5;
    // Reset IRQ flags
    P1IFG &= ~BIT5;
    // Enable button interrupts
    P1IE |= BIT5;
}

static void uart_rx_irq(uint32_t msg)
{
    parse_user_input();
    uart_set_eol(&bc);
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
    if (uart_get_event(&bc) == UART_EV_RX) {
        msg |= SYS_MSG_UART1_RX;
        uart_rst_event(&bc);
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
        if (timer_a2_get_event_schedule() & (1 << SCHEDULE_LED_ON)) {
            msg |= SYS_MSG_SCH_LED_ON;
        }
        if (timer_a2_get_event_schedule() & (1 << SCHEDULE_LED_OFF)) {
            msg |= SYS_MSG_SCH_LED_OFF;
        }
        if (timer_a2_get_event_schedule() & (1 << SCHEDULE_RELAY_OFF)) {
            msg |= SYS_MSG_SCH_RELAY_OFF;
        }
        if (timer_a2_get_event_schedule() & (1 << SCHEDULE_CHECK_PV)) {
            msg |= SYS_MSG_SCH_CHECK_PV;
        }
        if (timer_a2_get_event_schedule() & (1 << SCHEDULE_REFRESH_VIS)) {
            msg |= SYS_MSG_SCH_REFRESH_VIS;
        }

        timer_a2_rst_event_schedule();
    }
    // p1.5 rising edge trigger
    if (port1_last_event) {
        msg |= SYS_MSG_P1IFG;
        port1_last_event = 0;
    }

    eh_exec(msg);
}

static void scheduler_handler(uint32_t msg)
{
    timer_a2_scheduler_handler();
}

void led_on_handler(uint32_t msg)
{
    struct pwr_mng_blinky *blinky = pwr_mng_bliky_p();
    st_on;
    timer_a2_set_trigger_slot(SCHEDULE_LED_OFF, systime() + blinky->on, TIMER_A2_EVENT_ENABLE);
}

static void led_off_handler(uint32_t msg)
{
    struct pwr_mng_blinky *blinky = pwr_mng_bliky_p();
    st_off;
    if (blinky->off) {
        timer_a2_set_trigger_slot(SCHEDULE_LED_ON, systime() + blinky->off, TIMER_A2_EVENT_ENABLE);
    }
}

static void relay_off_handler(uint32_t msg)
{
    st_off;
    relay_off;
}

static void rtc_alarm(uint32_t msg)
{
    relay_on;
    timer_a2_set_trigger_slot(SCHEDULE_RELAY_OFF, systime() + 30, TIMER_A2_EVENT_ENABLE);
}

void update_adc(void)
{
    uint32_t tmp;
    //sig0_on;
    adc10_read(1, &adc.lipo, REFVSEL_0);
    //adc10_read(2, &q_vbat, REFVSEL_0);
    adc10_read(3, &adc.pv, REFVSEL_0);
    //adc10_read(9, &q_th, REFVSEL_0);
    adc10_read(10, &adc.t_internal, REFVSEL_0);
    adc10_halt();
    //sig0_off;

    tmp = (uint32_t) ((uint32_t) adc.lipo.counts_calib * (uint32_t) LIPO_SLOPE) >> 12;
    adc.lipo.conv = (uint16_t) tmp;
    adc.lipo.calib = adc.lipo.conv;     // FIXME

    tmp = (uint32_t) ((uint32_t) adc.pv.counts_calib * (uint32_t) PV_SLOPE) >> 12;
    adc.pv.conv = (uint16_t) tmp;
    adc.pv.calib = adc.pv.conv; // FIXME
}

static void check_pv_handler(uint32_t msg)
{
    update_adc();
    if (pwr_mng_check_pv(&adc) == PWR_PV_CHECK_RERUN) {
        timer_a2_set_trigger_slot(SCHEDULE_CHECK_PV, systime() + 150, TIMER_A2_EVENT_ENABLE);
    }
}

static void refresh_vis_handler(uint32_t msg)
{
    pwr_mng_refresh_vis(&adc);
}

static void main_loop(uint32_t msg)
{
    update_adc();
    pwr_mng(&adc);
    timer_a2_set_trigger_slot(SCHEDULE_CHECK_PV, systime() + 150, TIMER_A2_EVENT_ENABLE);
    timer_a2_set_trigger_slot(SCHEDULE_REFRESH_VIS, systime() + 350, TIMER_A2_EVENT_ENABLE);

#ifdef CONFIG_DEBUG
    char itoa_buf[CONV_BASE_10_BUF_SZ];

    uart_print(&bc, "lipo ");
    uart_print(&bc, _utoa(itoa_buf, adc.lipo.counts));
    uart_print(&bc, " ");
    uart_print(&bc, _utoa(itoa_buf, adc.lipo.counts_calib));
    uart_print(&bc, " ");
    uart_print(&bc, _utoa(itoa_buf, adc.lipo.conv));
    uart_print(&bc, ", pv ");
    uart_print(&bc, _utoa(itoa_buf, adc.pv.counts));
    uart_print(&bc, " ");
    uart_print(&bc, _utoa(itoa_buf, adc.pv.counts_calib));
    uart_print(&bc, " ");
    uart_print(&bc, _utoa(itoa_buf, adc.pv.conv));
    uart_print(&bc, ", tint ");
    uart_print(&bc, _utoa(itoa_buf, adc.t_internal.counts));
    uart_print(&bc, " ");
    uart_print(&bc, _utoa(itoa_buf, adc.t_internal.conv));
    uart_print(&bc, "\r\n");
#endif

}

int main(void)
{
    struct rtca_tm t;

    // watchdog triggers after 25sec when not cleared
#ifdef USE_WATCHDOG
    WDTCTL = WDTPW + WDTIS__512K + WDTSSEL__ACLK + WDTCNTCL;
#else
    WDTCTL = WDTPW + WDTHOLD;
#endif
    msp430_hal_init(HAL_GPIO_DIR_OUTPUT | HAL_GPIO_OUT_LOW);
    port_init();
    st_on;

    clock_pin_init();
    clock_init();

    rtca_init();
    //t.hour = COMPILE_HOUR;
    //t.min = COMPILE_MIN + 2;
    t.hour = 10;
    t.min = 0;
    rtca_set_alarm(&t, AE_MIN | AE_HOUR);
    rtca_enable_alarm();

    bc.baseAddress = USCI_A1_BASE;
    bc.baudrate = BAUDRATE_57600;
    uart_pin_init(&bc);
    uart_init(&bc);
#if defined UART_RX_USES_RINGBUF
    uart_set_rx_irq_handler(&bc, uart_rx_ringbuf_handler);
#else
    uart_set_rx_irq_handler(&bc, uart_rx_simple_handler);
#endif

    timer_a0_init();
    timer_a2_init();

    pwr_mng_init();

    eh_init();
    eh_register(&main_loop, SYS_MSG_RTC_MINUTE);
    eh_register(&rtc_alarm, SYS_MSG_RTC_ALARM);
    eh_register(&uart_rx_irq, SYS_MSG_UART1_RX);
    //eh_register(&main_loop, SYS_MSG_P1IFG);
    eh_register(&scheduler_handler, SYS_MSG_TIMERA2_CCR1);
    eh_register(&led_off_handler, SYS_MSG_SCH_LED_OFF);
    eh_register(&led_on_handler, SYS_MSG_SCH_LED_ON);
    eh_register(&relay_off_handler, SYS_MSG_SCH_RELAY_OFF);
    eh_register(&check_pv_handler, SYS_MSG_SCH_CHECK_PV);
    eh_register(&refresh_vis_handler, SYS_MSG_SCH_REFRESH_VIS);

    st_off;                     // init ended

    //display_menu();

    while (1) {
        // sleep
        _BIS_SR(LPM3_bits + GIE);
        __no_operation();
#ifdef USE_WATCHDOG
        WDTCTL = (WDTCTL & 0xff) | WDTPW | WDTCNTCL;
#endif
        check_events();
        check_events();
        check_events();
    }
}

// Port 1 interrupt service routine
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=PORT1_VECTOR
__interrupt void Port_1(void)
#elif defined(__GNUC__)
__attribute__((interrupt(PORT1_VECTOR)))
void Port1_ISR(void)
#else
#error Compiler not supported!
#endif
{
    uint16_t iv = P1IFG;
    if (iv & P1IV_P1IFG5) {
        port1_last_event = BIT5;
        P1IFG &= ~P1IV_P1IFG5;
        _BIC_SR_IRQ(LPM3_bits);
    }
    P1IFG = 0;
}
