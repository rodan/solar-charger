
//  code for sensirion sht 1x humidity and temperature sensors
//  author:          Petre Rodan <petre.rodan@simplex.ro>
//  available from:  https://github.com/rodan/
//  license:         GNU GPLv3

#include "sensirion.h"
#include "serial_bitbang.h"

uint8_t sht_get_status(uint8_t* data)
{
    i2csens_rxfrom(0x7, data, 1);
    return 0;
}

uint8_t sht_get_meas(uint16_t *temp, uint16_t *rh)
{
    uint8_t rv;
    uint8_t raw_temp[3];
    uint8_t raw_rh[3];
    uint16_t raw_temp_l, raw_rh_l;
    float temp_f, rh_f, rhc_f;

    rv = i2csens_rxfrom(0x3, raw_temp, 3);
    if (rv != I2C_ACK) {
        return rv;
    }
    rv = i2csens_rxfrom(0x5, raw_rh, 3);
    if (rv != I2C_ACK) {
        return rv;
    }
    raw_temp_l = (raw_temp[0] << 8) + raw_temp[1];
    raw_rh_l = (raw_rh[0] << 8) + raw_rh[1];
    temp_f = (D1 + (D2 * (float) (raw_temp_l))) * 100.0;
    rh_f = C1 + (C2 * raw_rh_l) + (C3 * raw_rh_l * raw_rh_l);
    // temperature compensation
    rhc_f = ((temp_f/100.0 - 25.0) * (T1 + (T2 * raw_rh_l)) + rh_f ) * 100.0; 
    *temp = (uint16_t) temp_f;
    *rh = (uint16_t) rhc_f;

    return rv;
}


uint8_t i2csens_rxfrom(const uint8_t slave_address, uint8_t* data, uint16_t length)
{
    uint8_t rv;
    rv = i2csens_start();
    if (rv != I2C_OK) {
        return rv;
    }
    rv = i2cm_tx(slave_address, I2C_NO_ADDR_SHIFT);
    if (rv == I2C_ACK) {
        i2cm_rx(data, length, I2C_SDA_WAIT);
    }
    i2csens_stop();
    return rv;
}

// sensirion's take on i2c start
uint8_t i2csens_start(void)
{
    uint8_t rv = 0;
    I2C_MASTER_DIR &= ~(I2C_MASTER_SCL + I2C_MASTER_SDA);
    I2C_MASTER_OUT &= ~(I2C_MASTER_SDA | I2C_MASTER_SCL);
    sda_high;
    scl_high; // let SCL float to verify if it gets pulled high
    if (!(I2C_MASTER_IN & I2C_MASTER_SDA)) {
        rv |= I2C_MISSING_SDA_PULLUP;
    }
    if (!(I2C_MASTER_IN & I2C_MASTER_SCL)) {
        rv |= I2C_MISSING_SCL_PULLUP;
    }
    if (rv) {
        return rv;
    }
    scl_low;
    delay_c;
    delay_c;
    scl_high;
    sda_low;
    scl_low;
    scl_high;
    sda_high;
    scl_low;
    return I2C_OK;
}

void i2csens_stop(void)
{
    sda_high;
    scl_high;
}

void i2csens_reset(void)
{
    uint8_t i;
    sda_high;
    delay_c;
    for (i=0;i<30;i++) {
        scl_low;
        scl_high;
    }
}


