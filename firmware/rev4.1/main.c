
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

//float v_bat, v_pv, t_th, t_int;
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
    uint16_t local_ev;

    // RTC
    local_ev = rtca_get_event();
    if (local_ev) {
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

    eh_exec(msg);
}

static void rtc_alarm(uint32_t msg)
{
    uart1_print("alarm!");
    uart1_print("\r\n");
}

static void main_loop(uint32_t msg)
{
    float ftemp;
    char itoa_buf[CONV_BASE_10_BUF_SZ];

    sig0_on;
    adc10_read(1, &adc.lipo.raw, REFVSEL_0);
    //adc10_read(2, &q_vbat, REFVSEL_0);
    adc10_read(3, &adc.pv.raw, REFVSEL_0);
    //adc10_read(9, &q_th, REFVSEL_0);
    adc10_read(10, &adc.t_internal.raw, REFVSEL_0);
    adc10_halt();
    sig0_off;

    ftemp = (float) adc.lipo.raw * LIPO_SLOPE;
    adc.lipo.conv = (uint16_t) ftemp;
    adc.lipo.calib = adc.lipo.conv; // FIXME

    ftemp = (float) adc.pv.raw * PV_SLOPE;
    adc.pv.conv = (uint16_t) ftemp;
    adc.pv.calib = adc.pv.conv; // FIXME

    pwr_mng(&adc);

    uart1_print("lipo ");
    uart1_print(_utoa(itoa_buf, adc.lipo.raw));
    uart1_print(" ");
    uart1_print(_utoa(itoa_buf, adc.lipo.conv));
    uart1_print(", pv ");
    uart1_print(_utoa(itoa_buf, adc.pv.raw));
    uart1_print(" ");
    uart1_print(_utoa(itoa_buf, adc.pv.conv));
    uart1_print(", tint ");
    uart1_print(_utoa(itoa_buf, adc.t_internal.raw));
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

    //timer_a0_init();

    uart1_port_init();
    uart1_init();

#ifdef UART1_RX_USES_RINGBUF
    uart1_set_rx_irq_handler(uart1_rx_ringbuf_handler);
#else
    uart1_set_rx_irq_handler(uart1_rx_simple_handler);
#endif

//#if (defined(USE_XT2) && defined(SMCLK_FREQ_16M)) || defined(UART1_TX_USES_IRQ)
    // an external high frequency crystal can't be woken up quickly enough
    // from LPM, so make sure that SMCLK never powers down

    // also the uart tx irq ain't working without this for some reason
    timer_a0_init();
//#endif

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

    rtca_set_alarm(COMPILE_HOUR, COMPILE_MIN + 2);
    rtca_enable_alarm();

    display_menu();

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
