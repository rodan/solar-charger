
#include <stdio.h>
#include <string.h>

#include "glue.h"
#include "rtc.h"
#include "ui.h"
#include "version.h"
#include "timer_a2.h"
#include "pwr_mng.h"

extern uart_descriptor bc;

static const char menu_head[]="\r\n batt charger rev4.1 build ";
static const char menu_str[]="\
 --- available commands:\r\n\r\n\
\e[33;1mpv[01]\e[0m - control photovoltaic cell switch\r\n\
\e[33;1mce[01]\e[0m - control charge enable switch\r\n\
\e[33;1mdate\e[0m   - RTC read\r\n\
\e[33;1mchg\e[0m    - CHG status\r\n\
\e[33;1msch\e[0m    - view schedule\r\n\
\e[33;1m?\e[0m      - show menu\r\n";

void display_menu(void)
{
    char itoa_buf[CONV_BASE_10_BUF_SZ];

    uart_print(&bc, menu_head);
    uart_print(&bc, _utoa(&itoa_buf[0], BUILD));
    uart_print(&bc, menu_str);
}

void display_schedule(void)
{
    uint8_t c;
    uint32_t trigger;
    uint8_t flag;
    char itoa_buf[CONV_BASE_10_BUF_SZ];

    for (c = 0; c < TIMER_A2_SLOTS_COUNT; c++) {
        timer_a2_get_trigger_slot(c, &trigger, &flag);
        uart_print(&bc, _utoa(itoa_buf, c));
        uart_print(&bc, " \t");
        uart_print(&bc, _utoa(itoa_buf, trigger));
        uart_print(&bc, " \t");
        uart_print(&bc, _utoa(itoa_buf, flag));
        uart_print(&bc, "\r\n");
    }
    trigger = timer_a2_get_trigger_next();
    uart_print(&bc, "sch next ");
    uart_print(&bc, _utoa(itoa_buf, trigger));
    uart_print(&bc, " sys ");
    uart_print(&bc, _utoa(itoa_buf, systime()));
    uart_print(&bc, "\r\n");
}

#define PARSER_CNT 16

void parse_user_input(void)
{
#if defined UART_RX_USES_RINGBUF
    struct ringbuf *rbr = uart_get_rx_ringbuf(&bc);
    uint8_t rx;
    uint8_t c = 0;
    char input[PARSER_CNT];

    memset(input, 0, PARSER_CNT);

    // read the entire ringbuffer
    while (ringbuf_get(rbr, &rx)) {
        if (c < PARSER_CNT-1) {
            input[c] = rx;
        }
        c++;
    }
#else
    char *input = uart_get_rx_buf(&bc);
#endif

    struct rtca_tm t;
    char f = input[0];
    char itoa_buf[CONV_BASE_10_BUF_SZ];

    if (f == '?') {
        display_menu();
    } else if (strstr(input, "chg")) {
        uart_print(&bc, "CHG ");
        if (P1IN & BIT5) {
            uart_print(&bc, "off");
        } else {
            uart_print(&bc, "on");
        }
        //uart_print(&bc, _itoa(itoa_buf, (P1IN & BIT5)));
        uart_print(&bc, ", lipo ");
        uart_print(&bc, _itoa(itoa_buf, pwr_mng_get_lipo_charge()));
        uart_print(&bc, "%\r\n");
    } else if (strstr(input, "sch")) {
        display_schedule();
    } else if (strstr(input, "date")) {
        rtca_get_time(&t);
        uart_print(&bc, _itoa(itoa_buf, t.hour));
        uart_print(&bc, ":");
        uart_print(&bc, _itoa(itoa_buf, t.min));
        uart_print(&bc, ":");
        uart_print(&bc, _itoa(itoa_buf, t.sec));
        uart_print(&bc, " ");
        uart_print(&bc, _itoa(itoa_buf, t.year));
        uart_print(&bc, ".");
        uart_print(&bc, _itoa(itoa_buf, t.mon));
        uart_print(&bc, ".");
        uart_print(&bc, _itoa(itoa_buf, t.day));
        uart_print(&bc, " sys ");
        uart_print(&bc, _itoa(itoa_buf, t.sys));
        uart_print(&bc, "\r\n");
    } else if (strstr(input, "ce0")) {
        ce_off;
    } else if (strstr(input, "ce1")) {
        ce_on;
    } else if (strstr(input, "pv0")) {
        pve_off;
    } else if (strstr(input, "pv1")) {
        pve_on;
    }
}
