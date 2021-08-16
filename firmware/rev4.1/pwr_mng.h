#ifndef __PWR_MNG_H__
#define __PWR_MNG_H__

enum pwr_states {
IGNORE,
ON,
OFF
};

enum charging_state {
IS_NOT_CHARGING,
IS_CHARGING
};

struct pwr_mng_blinky {
    uint8_t flags;
    uint16_t on;
    uint16_t off;
};

// state_flags
#define            PWR_ST_IDLE  0x00  // 0on, 0off
#define   PWR_ST_LIPO_CHARGING  0x02  // depends on charge

#define      PWR_ST_DEACTIVATE  0x00
#define        PWR_ST_ACTIVATE  0x01

#define            LIPO_THRESH  378 // 90% of 4.2V (*100) - dont start charging the lipo cell if it's above 90%
#define              LIPO_ALRM  300 // minimum lipo voltage
#define              LIPO_FULL  415 // charged lipo voltage
#define              PV_THRESH  620 // LM2675 needs a minimum of 6V-6.5V input voltage to work (*100)
#define          PV_CHG_THRESH 1400 // minimal voltage needed to start lipo charging 

void pwr_mng_init(void);
void pwr_mng(struct adc_conv *adc_c);
void pwr_mng_check_pv(struct adc_conv *adc_c);
void pwr_mng_refresh_vis(struct adc_conv *adc_c);

void pwr_mng_state_display(const uint8_t state_flags, const uint8_t activate);
struct pwr_mng_blinky *pwr_mng_bliky_p(void);
uint8_t pwr_mng_get_lipo_charge(void);

#endif
