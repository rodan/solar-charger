
//   code that generates command sequences that once sent to an OOK rf stage are able
//   to control intertechno radio switches.
//
//   author:          Petre Rodan <petre.rodan@simplex.ro>
//   available from:  https://github.com/rodan/
//   license:         GNU GPLv3

#include "intertechno.h"
#include "sc.h"
#include "timer_a0.h"

void it_init()
{
    OOK_SEL &= ~OOK_PIN;
    OOK_DIR |= OOK_PIN;
    OOK_OUT &= ~OOK_PIN;
}

// a HI LO sequence 
void rf_ook_pulse(const uint16_t on, const uint16_t off)
{
    OOK_OUT |= OOK_PIN;
    timer_a0_delay(on);
    OOK_OUT &= ~OOK_PIN;
    timer_a0_delay(off);
}

// 0 is 10001000
void rf_tx_0()
{
    // each ook_pulse sends 4 bits
    rf_ook_pulse(BIT_DURATION, 3 * BIT_DURATION);
    rf_ook_pulse(BIT_DURATION, 3 * BIT_DURATION);
}

// 1 is 10001110
void rf_tx_1()
{
    // each ook_pulse sends 4 bits
    rf_ook_pulse(BIT_DURATION, 3 * BIT_DURATION);
    rf_ook_pulse(3 * BIT_DURATION, BIT_DURATION);
}

// sync sequence is 1000000000000000000000000000000
void rf_tx_sync()
{
    rf_ook_pulse(BIT_DURATION, 31 * BIT_DURATION);
}

uint8_t rf_tx_cmd(const uint8_t prefix, const uint8_t cmd)
{
    int8_t i, j;
    uint8_t rprefix;
    rprefix = rotate_byte(prefix);

    // any message has to be sent 4 times
    for (j = 0; j < 4; j++) {
        for (i = 7; i >= 0; i--) {
            switch (rprefix & (1 << i)) {
            case 0:
                rf_tx_0();
                break;
            default:
                rf_tx_1();
                break;
            }
        }
        for (i = 3; i >= 0; i--) {
            switch (cmd & (1 << i)) {
            case 0:
                rf_tx_0();
                break;
            default:
                rf_tx_1();
                break;
            }
        }
        rf_tx_sync();
    }
    return 0;
}

uint8_t rotate_byte(const uint8_t in)
{
    uint8_t rv = 0;
    rv += (in & 0x10) << 3;
    rv += (in & 0x20) << 1;
    rv += (in & 0x40) >> 1;
    rv += (in & 0x80) >> 3;
    rv += (in & 0x1) << 3;
    rv += (in & 0x2) << 1;
    rv += (in & 0x4) >> 1;
    rv += (in & 0x8) >> 3;
    return rv;
}
