#ifndef _CONFIG_H_
#define _CONFIG_H_

#define USE_XT1
//#define USE_XT2

//#define SMCLK_FREQ_1M
#define SMCLK_FREQ_4M
//#define SMCLK_FREQ_8M
//#define SMCLK_FREQ_16M

#define USE_UART1

//#define BAUD_9600
//#define BAUD_19200
//#define BAUD_38400
#define BAUD_57600
//#define BAUD_115200

#define UART1_RXBUF_SZ 16
//#define UART1_RX_USES_RINGBUF

#define UART1_TXBUF_SZ 32
//#define UART1_TX_USES_IRQ

//#define LED_SYSTEM_STATES

//#define USE_ITOA_LUT
//#define CONFIG_DEBUG

#endif
