
#include <inttypes.h>
#include "uart1.h"
#include "timer_a0.h"

volatile char uart1_rx_buf[UART1_RXBUF_SZ];     // receive buffer
volatile uint8_t uart1_p;
volatile uint8_t uart1_rx_enable;

volatile enum uart1_tevent uart1_last_event;

void uart1_init(uint16_t speed)
{
    UCA1CTL1 |= UCSWRST;        // put state machine in reset
    UCA1CTL1 |= UCSSEL_1;       // use ACLK

    if (speed == 9600) {
        UCA1BR0 = 0x03;
        UCA1BR1 = 0x00;
        UCA1MCTL = UCBRS_3 + UCBRF_0;       // modulation UCBRSx=3, UCBRFx=0
    } else if (speed == 2400) {
        UCA1BR0 = 0x0D;
        UCA1BR1 = 0x00;
        UCA1MCTL |= UCBRS_6 + UCBRF_0;            // Modulation UCBRSx=6, UCBRFx=0
    }

    UCA1CTL1 &= ~UCSWRST;       // initialize USCI state machine
    UCA1IE |= UCRXIE;           // enable USCI_A0 RX interrupt
    uart1_p = 0;
    uart1_rx_enable = 1;
}

uint8_t uart1_get_event(void)
{
    return uart1_last_event;
}

void uart1_rst_event(void)
{
    uart1_last_event = UART1_EV_NULL;
}

char *uart1_get_rx_buf(void)
{
    if (uart1_p) {
        return (char *)uart1_rx_buf;
    } else {
        return NULL;
    }
}

uint16_t uart1_tx_str(char *str, const uint16_t size)
{
    uint16_t p = 0;
    while (p < size) {
        while (!(UCA1IFG & UCTXIFG)) ;  // USCI_A0 TX buffer ready?
        UCA1TXBUF = str[p];
        p++;
    }
    return p;
}

__attribute__ ((interrupt(USCI_A1_VECTOR)))
void USCI_A1_ISR(void)
{
    uint16_t iv = UCA1IV;
    register char rx;
    enum uart1_tevent ev = 0;

    // iv is 2 for RXIFG, 4 for TXIFG
    switch (iv) {
    case 2:
        rx = UCA1RXBUF;
        if (uart1_rx_enable && (uart1_p < UART1_RXBUF_SZ-2)) {
                if (uart1_p > UART1_RXBUF_SZ-5) {
                    // use hardware flow control to stop the remote equipment
                    // from sending more data
                    //SIM900_RTS_HIGH;
                    uart1_rx_buf[uart1_p + 1] = 0; // not really working
                }
                if (uart1_p == 0) {
                    //sim900.console = TTY_RX_PENDING;
                    // set up timer that will end the buffer
                    //timer_a0_delay_noblk_ccr3(RXBUF_TMOUT);
                }
                uart1_rx_buf[uart1_p] = rx;
                uart1_p++;
                timer_a0_delay_noblk_ccr3(INTRCHAR_TMOUT);
        } 
        break;
    case 4:
        ev = UART1_EV_TX;
        break;
    default:
        break;
    }
    uart1_last_event |= ev;
}
