#ifndef __UART1_H__
#define __UART1_H__

#include "proj.h"

enum uart1_tevent {
    UART1_EV_RX = BIT0,
    UART1_EV_TX = BIT1
};

// SMSs are 160bytes + ~60bytes header + 10 cr|lf
#define UART1_RXBUF_SZ     255

// if after the last char received INTRCHAR_TMOUT elapses 
// without any more communication then we end the buffer and
// send it to be parsed

#define INTRCHAR_TMOUT     _10ms    // ~10ms in ticks
#define REPLY_TMOUT        _1s      // ~1s in ticks

#define  UART1_EV_NULL 0
#define    UART1_EV_RX 0x1
#define    UART1_EV_TX 0x2

void uart1_init(uint16_t speed);
uint16_t uart1_tx_str(char *str, const uint16_t size);

#endif
