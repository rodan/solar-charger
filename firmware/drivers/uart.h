#ifndef __UART_H__
#define __UART_H__

#include "sc.h"

enum uart_tevent {
    UART_EV_RX = BIT0,
    UART_EV_TX = BIT1
};

void uart_init();
uint16_t uart_tx_str(char *str, uint16_t size);

volatile enum uart_tevent uart_last_event;

#endif
