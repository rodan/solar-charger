
#include <stdio.h>
#include <string.h>

#include "glue.h"
#include "rtc.h"
#include "ui.h"

static const char menu_str[]="\
\r\n batt charger rev4.1 --- available commands:\r\n\r\n\
\e[33;1mr\e[0m  - RTC read\r\n\
\e[33;1md\e[0m  - DAC read\r\n\
\e[33;1m?\e[0m  - show menu\r\n";
//static const char err_conv_str[]="error during str_to_int32()\r\n";
//static const char received_str[]="received ";

void display_menu(void)
{
    uart1_print(menu_str);
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
    //uint32_t in=0;
    //int32_t si=0;

    if (f == '?') {
        display_menu();
    } else if (strstr(input, "chg")) {
        uart1_print("P1IN.5 ");
        uart1_print(_itoa(itoa_buf, (P1IN & BIT5)));
        uart1_print("\r\n");
    } else if (strstr(input, "rtc")) {
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
    } else if (strstr(input, "st0")) {
        st_off;
    } else if (strstr(input, "st1")) {
        st_on;
    } else if (f == '!') {
        uart1_print(_itoa(itoa_buf, P6SEL));
        uart1_print("\r\n");
#if 0
    } else if (f == 'd') {
        if (str_to_int32(input, &si, 1, strlen(input) - 1, -2147483648, 2147483647) == EXIT_FAILURE) {
            uart1_print(err_conv_str);
        }
        uart1_print(received_str);
        uart1_print(_itoa(itoa_buf, si));
        uart1_print("\r\n");
    } else if (f == 'h') {
        if (str_to_uint32(input, &in, 1, strlen(input) - 1, 0, -1) == EXIT_FAILURE) {
            uart1_print(err_conv_str);
        }
        uart1_print(received_str);
        uart1_print(_utoh(itoa_buf, in));
        uart1_print("\r\n");
    } else if (f == 'a') {
        uart1_print("123456789\r\n");
#endif
    } else {
        //uart1_tx_str("\r\n", 2);
    }
}
