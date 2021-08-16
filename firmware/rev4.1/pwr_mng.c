
#include "proj.h"
#include "pwr_mng.h"

uint8_t lipo_charge; // percentage of charge between LIPO_ALRM and LIPO_FULL
struct pwr_mng_blinky blinky;

enum charging_state CHG_status(void)
{
    if (P1IN & BIT5) {
        return IS_NOT_CHARGING;
    } else {
        return IS_CHARGING;
    }
}

enum charging_state CE_status(void)
{
    if (P6OUT & BIT0) {
        return IS_NOT_CHARGING;
    } else {
        return IS_CHARGING;
    }
}

void pwr_mng_init(void)
{
    lipo_charge = 20;
    blinky.flags = 0;
    pve_on;
    ce_off;
}

void pwr_mng_state_display(const uint8_t state_flags, const uint8_t activate)
{
    struct pwr_mng_blinky blinky_tmp;

    blinky_tmp.flags = blinky.flags;
    blinky_tmp.on = 0;
    blinky_tmp.off = 0;

    if (activate == PWR_ST_ACTIVATE) {
        blinky_tmp.flags |= state_flags;
    } else {
        blinky_tmp.flags &= ~state_flags;
    }

    // highest priority info must be the last one

    if (blinky_tmp.flags & PWR_ST_LIPO_CHARGING) {
        blinky_tmp.on = 102 - lipo_charge;
        blinky_tmp.off = lipo_charge;
    }

    blinky.on = blinky_tmp.on;
    blinky.off = blinky_tmp.off;

    if (blinky.on && (blinky_tmp.flags != blinky.flags))  {
        led_on_handler(0);
    }

    blinky.flags = blinky_tmp.flags;
}

struct pwr_mng_blinky *pwr_mng_bliky_p(void)
{
    return &blinky;
}

uint8_t pwr_mng_get_lipo_charge(void)
{
    return lipo_charge;
}

void pwr_mng_check_pv(struct adc_conv *adc_c)
{
    // if pv is weak then disable
    if (adc_c->pv.calib < PV_THRESH) {
        if (CE_status() == IS_NOT_CHARGING) {
            pve_off;
        } else {
            ce_off;
        }
    }

}

void pwr_mng_refresh_vis(struct adc_conv *adc_c)
{
    if (CHG_status() == IS_NOT_CHARGING) {
        pwr_mng_state_display(PWR_ST_LIPO_CHARGING, PWR_ST_DEACTIVATE);
    } else {
        pwr_mng_state_display(PWR_ST_LIPO_CHARGING, PWR_ST_ACTIVATE);
    }
}

void pwr_mng(struct adc_conv *adc_c)
{
    // controls: 
    //  CE  - charge enable - enable lipo charging circuitry
    //  PVE - photovoltaic switch - must be shut down if voltage below threshold

    // if PV below 6.2V then shut PVE and CE down
    // if lipo below 3.78 and PV above 6.2V then PVE and CE start
    enum pwr_states ce = IGNORE, pve = IGNORE;
    int32_t tmp;

    // normal working parameters
    if (adc_c->pv.calib > PV_THRESH) {
        pve = ON;
        if ((adc_c->lipo.calib < LIPO_THRESH) && (adc_c->pv.calib > PV_CHG_THRESH)) {
            ce = ON;
        } // else {} - no need to stop the charging process
    } else {
        if (CE_status() == IS_NOT_CHARGING) {
            pve = OFF;
        }
        ce = OFF;
    }

    // if BQ24072 signals an end-of-charge condition
    if ((adc_c->lipo.calib > LIPO_FULL) && (CHG_status() == IS_NOT_CHARGING)) {
        ce = OFF;
    }

    // get a percentage for lipo charge
    tmp = (uint32_t) (adc_c->lipo.calib - LIPO_ALRM) * 100 / (uint32_t) (LIPO_FULL - LIPO_ALRM);
    if (tmp < 0) {
        tmp = 0;
    } else if (tmp > 100) {
        tmp = 100;
    }
    lipo_charge = tmp;

    // apply mosfet configuration as a last step
    switch (ce) {
        case ON:
            ce_on;
            break;
        case OFF:
            ce_off;
            break;
        default:
            break;
    }

    switch (pve) {
        case ON:
            pve_on;
            break;
        case OFF:
            pve_off;
            break;
        default:
            break;
    }
}

