
//  code for Honeywell TruStability HSC and SSC pressure sensors
//  author:          Petre Rodan <petre.rodan@simplex.ro>
//  available from:  https://github.com/rodan/
//  license:         GNU GPLv3
//
//  Honeywell High Accuracy Ceramic (HSC) and Standard Accuracy Ceramic
//  (SSC) Series are piezoresistive silicon pressure sensors.

#include "sc.h"
#include "hsc_ssc_i2c.h"
#include "serial_bitbang.h"

/* you must define the slave address. you can find it based on the part number:

    _SC_________XA_
    where X can be one of:

    S  - spi (this is not the right library for spi opperation)
    2  - i2c slave address 0x28
    3  - i2c slave address 0x38
    4  - i2c slave address 0x48
    5  - i2c slave address 0x58
    6  - i2c slave address 0x68
    7  - i2c slave address 0x78
*/

// returns  0 if all is fine
//          1 if chip is in command mode
//          2 if old data is being read
//          3 if a diagnostic fault is triggered in the chip
//          I2C err levels if sensor is not properly hooked up

uint8_t ps_get_raw(const uint8_t slave_addr, struct cs_raw *raw)
{
    uint8_t val[4]={ 0, 0, 0, 0};
    uint8_t rv;

    rv = i2cm_rxfrom(slave_addr, val, 4);
    if (rv != I2C_ACK) {
        return rv;
    }
    raw->status = (val[0] & 0xc0) >> 6;  // first 2 bits from first byte
    raw->bridge_data = ((val[0] & 0x3f) << 8) + val[1];
    raw->temperature_data = ((val[2] << 8) + (val[3] & 0xe0)) >> 5;
    return rv;
}

uint8_t ps_convert(const struct cs_raw raw, uint32_t *pressure, uint16_t *temperature,
                   const uint16_t output_min, const uint16_t output_max, const float pressure_min,
                   const float pressure_max)
{
    float t,p;
    p = 1.0 * (raw.bridge_data - output_min) * (pressure_max - pressure_min) / (output_max - output_min) - pressure_min;
    t = (raw.temperature_data * 0.0977) - 50;

    *pressure = p;
    *temperature = 100*t;
    return 0;
}

