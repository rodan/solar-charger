
//  read adc conversions from any of the P6 ports
//
//  at least a 1ms delay should be inserted between two adc10_read()s or
//  between an adc10_read(port, &rv) and the use of rv.
//
//  author:          Petre Rodan <2b4eda@subdimension.ro>
//  available from:  https://github.com/rodan/
//  license:         GNU GPLv3

#include <inttypes.h>
#include "tlv.h"
#include "adc.h"
#include "proj.h"

volatile uint16_t *adc10_rv;
volatile uint8_t adcready;

uint16_t adc10_apply_adc_correction(const uint16_t counts);
int16_t adc10_conv_temp_counts(const uint16_t counts);

// port: 0 = P6.A0, 1 = P6.A1, .., 0xa = P6.A10 = internal temp sensor
// vref is one of:  REFVSEL_0  - 1.5v vref
//                  REFVSEL_1  - 2.0v vref
//                  REFVSEL_2  - 2.5v vref
void adc10_read(const uint8_t port, adc_channel *ch, const uint8_t vref)
{
    //*((uint16_t *)portreg) |= 1 << port;
    // if ref or adc10 are busy then wait
    while (REFCTL0 & REFGENBUSY) ;
    while (ADC10CTL1 & ADC10BUSY) ;
    // enable reference
    if ((REFCTL0 & 0x30) != vref) {
        // need to change vref
        REFCTL0 &= ~(0x30 + REFON);
        REFCTL0 |= REFMSTR + vref + REFON;
    } else {
        REFCTL0 |= REFMSTR + REFON;
    }
    ADC10CTL0 &= ~ADC10ENC;
    // enable ADC10_A, single channel single conversion
    ADC10CTL0 = ADC10SHT_2 + ADC10ON;
    ADC10CTL1 = ADC10SHP + ADC10DIV1 + ADC10DIV0;
    // use internal Vref(+) AVss (-)
    ADC10MCTL0 = ADC10SREF_1 + port;
    ADC10CTL2 |= ADC10PDIV_2 + ADC10SR;
    adcready = 0;
    adc10_rv = &ch->counts;
    // trigger conversion
    ADC10IE = ADC10IE0;
    ADC10CTL0 |= ADC10ENC + ADC10SC;
    while (!adcready) ;

    if (port != 10) {
        ch->counts_calib = adc10_apply_adc_correction(ch->counts);
    } else {
        ch->conv = adc10_conv_temp_counts(ch->counts);
    }
}

void adc10_halt(void)
{
    ADC10CTL0 &= ~ADC10ON;
    REFCTL0 &= ~REFON;
}

int16_t adc10_conv_temp_counts(const uint16_t counts)
{
    uint8_t tag_length;
    uint16_t *tag_pointer;
    uint16_t cal_adc_t30, cal_adc_t85; // ideal counts at 30, 85 dC saved in the TLV adc10 calibration structure
    int32_t tmp;
    int16_t rv;

    TLV_getInfo(TLV_ADC10CAL, 0, &tag_length, (uint16_t **)&tag_pointer );

    if (tag_pointer != 0) {
        cal_adc_t30 = tag_pointer[2];
        cal_adc_t85 = tag_pointer[3];
        tmp = ((int32_t)counts - (int32_t)cal_adc_t30) * (85-30) * 100 / ((int32_t)cal_adc_t85 - (int32_t)cal_adc_t30) + 30 * 100;
        rv = tmp;
    } else {
        rv = -25500;
    }

    return rv;
}


uint16_t adc10_apply_adc_correction(const uint16_t counts)
{
    int32_t tmp;
    uint16_t rv;
    uint16_t adc_gain_factor;
    int16_t adc_offset;
    uint8_t tag_length;
    uint16_t *tag_pointer;

    TLV_getInfo(TLV_ADC10CAL, 0, &tag_length, (uint16_t **)&tag_pointer );

    if (tag_pointer != 0) {
        adc_gain_factor = tag_pointer[0];
        adc_offset = tag_pointer[1];
        tmp = ((uint32_t) counts * (uint32_t) adc_gain_factor) >> 15;
        tmp += adc_offset;
        if (tmp < 0) {
            tmp = 0;
        }
        rv = tmp;
    } else {
        rv = counts;
    }

    return rv;
}

__attribute__ ((interrupt(ADC10_VECTOR)))
void adc10_ISR(void)
{
    uint16_t iv = ADC10IV;
    if (iv == ADC10IV_ADC10IFG) {
        *adc10_rv = ADC10MEM0;
        adcready = 1;
    }
}
