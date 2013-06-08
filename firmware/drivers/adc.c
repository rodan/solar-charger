
//  read adc conversions from any of the P6 ports
//
//  at least a 1ms delay should be inserted between two adc10_read()s or
//  between an adc10_read(port, &rv) and the use of rv.
//
//  author:          Petre Rodan <petre.rodan@simplex.ro>
//  available from:  https://github.com/rodan/
//  license:         GNU GPLv3

#include "adc.h"

volatile uint16_t *adc10_rv;

// port: 0 = P6.A0, 1 = P6.A1, .., 0xa = P6.A10 = internal temp sensor
void adc10_read(const uint8_t port, uint16_t *rv)
{
    P6SEL |= 1 << port;
    // if ref or adc10 are busy then wait
    while (REFCTL0 & REFGENBUSY);
    while (ADC10CTL1 & ADC10BUSY);
    // enable 2.5V reference
    REFCTL0 |= REFMSTR + REFVSEL_2 + REFON;
    ADC10CTL0 &= ~ADC10ENC;
    // enable ADC10_A, single channel single conversion
    ADC10CTL0 = ADC10SHT_2 + ADC10ON;
    ADC10CTL1 = ADC10SHP;
    // use internal Vref(+) AVss (-)
    ADC10MCTL0 = ADC10SREF_1 + port;
    ADC10CTL2 |= ADC10PDIV_2 + ADC10SR;
    adc10_rv = rv;
    // trigger conversion
    ADC10IE = ADC10IE0;
    ADC10CTL0 |= ADC10ENC + ADC10SC;
}

void adc10_halt(void)
{
    ADC10CTL0 = 0;
    REFCTL0 &= ~REFON;
}

__attribute__ ((interrupt(ADC10_VECTOR)))
void adc10_ISR(void)
{
    uint16_t iv = ADC10IV;
    if (iv == ADC10IV_ADC10IFG) {
        *adc10_rv = ADC10MEM0;
    }
}
