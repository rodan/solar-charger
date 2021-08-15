
#include "proj.h"
#include "pwr_mng.h"

enum charging_state CHG_status(void)
{
    if (P1IN & BIT5) {
        return IS_NOT_CHARGING;
    } else {
        return IS_CHARGING;
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

    // normal working parameters
    if (adc_c->pv.calib > PV_THRESH) {
        pve = ON;
        if (adc_c->lipo.calib < LIPO_THRESH) {
            ce = ON;
        } // else {} - no need to stop the charging process
    } else {
        pve = OFF;
        ce = OFF;
    }

    if (adc_c->lipo.calib < LIPO_ALRM) {
        // cornercase - lipo cell connection is broken
        pve = ON;
        ce = ON;
    }

    // if BQ24072 signals an end-of-charge condition
    if ((adc_c->lipo.calib > LIPO_FULL) && (CHG_status() == IS_NOT_CHARGING)) {
        ce = OFF;
    }

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

