
#include <stdio.h>
#include <string.h>

#include "glue.h"
#include "rtc.h"
#include "ui.h"
#include "version.h"
#include "timer_a2.h"

static const char menu_head[]="\r\n batt charger rev4.1 build ";
static const char menu_str[]="\
 --- available commands:\r\n\r\n\
\e[33;1mdate\e[0m - RTC read\r\n\
\e[33;1mchg\e[0m  - CHG status\r\n\
\e[33;1m?\e[0m    - show menu\r\n";

void display_menu(void)
{
    char itoa_buf[CONV_BASE_10_BUF_SZ];

    uart1_print(menu_head);
    uart1_print(_utoa(&itoa_buf[0], BUILD));
    uart1_print(menu_str);
}

void display_schedule(void)
{
    uint8_t c;
    uint32_t trigger;
    uint8_t flag;
    char itoa_buf[CONV_BASE_10_BUF_SZ];

    for (c = 0; c < TIMER_A2_SLOTS_COUNT; c++) {
        timer_a2_get_trigger_slot(c, &trigger, &flag);
        uart1_print(_utoa(itoa_buf, c));
        uart1_print(" \t");
        uart1_print(_utoa(itoa_buf, trigger));
        uart1_print(" \t");
        uart1_print(_utoa(itoa_buf, flag));
        uart1_print("\r\n");
    }
    trigger = timer_a2_get_trigger_next();
    uart1_print("sch next ");
    uart1_print(_utoa(itoa_buf, trigger));
    uart1_print(" sys ");
    uart1_print(_utoa(itoa_buf, systime()));
    uart1_print("\r\n");
}

#define PARSER_CNT 16

void parse_user_input(void)
{
#ifdef uart1_RX_USES_RINGBUF
    struct ringbuf *rbr = uart1_get_rx_ringbuf();
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
    char *input = uart1_get_rx_buf();
#endif
    struct rtca_tm t;
    char f = input[0];
    char itoa_buf[CONV_BASE_10_BUF_SZ];

    if (f == '?') {
        display_menu();
    } else if (strstr(input, "chg")) {
        uart1_print("P1IN.5 ");
        uart1_print(_itoa(itoa_buf, (P1IN & BIT5)));
        uart1_print("\r\n");
    } else if (strstr(input, "sch")) {
        display_schedule();
    } else if (strstr(input, "date")) {
        rtca_get_time(&t);
        uart1_print(_itoa(itoa_buf, t.hour));
        uart1_print(":");
        uart1_print(_itoa(itoa_buf, t.min));
        uart1_print(":");
        uart1_print(_itoa(itoa_buf, t.sec));
        uart1_print(" ");
        uart1_print(_itoa(itoa_buf, t.year));
        uart1_print(".");
        uart1_print(_itoa(itoa_buf, t.mon));
        uart1_print(".");
        uart1_print(_itoa(itoa_buf, t.day));
        uart1_print(" sys ");
        uart1_print(_itoa(itoa_buf, t.sys));
        uart1_print("\r\n");
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
