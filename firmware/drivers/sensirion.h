#ifndef __SENSIRION_H__
#define __SENSIRION_H__

#include <stdint.h>

// coefficients for 12bit rh, 14bit temperature, 3.3V Vdd
#define C1      -2.0468
#define C2       0.0367
#define C3      -1.5955E-6
#define T1       0.01
#define T2       0.00008
#define D1     -39.7
#define D2       0.01

uint8_t sht_get_status(uint8_t *data);
uint8_t sht_get_meas(uint16_t *temp, uint16_t *rh);

// pseudo i2c protocol used by sensirion sensors

// read 'length' bytes into 'data' from 'address'
uint8_t i2csens_rxfrom(const uint8_t slave_address, uint8_t* data, uint16_t length);

// send a 'start' sequence
uint8_t i2csens_start(void);

// send a 'stop' sequence
void i2csens_stop(void);

// reset serial connection
void i2csens_reset(void);


#endif
