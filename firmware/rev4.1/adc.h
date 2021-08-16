#ifndef __ADC_H__
#define __ADC_H__

typedef struct __adc_channel {
    uint16_t counts;   // raw counts received from the ADC
    uint16_t counts_calib; // corrected counts (using internal TLV calibration)
    uint16_t conv;  // raw value converted to Volts (*100)
    uint16_t calib; // converted value after calibration (*100)
} adc_channel;

void adc10_read(const uint8_t port, adc_channel *ch, const uint8_t vref);
void adc10_halt(void);

#endif
