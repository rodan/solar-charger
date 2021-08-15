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

#define            LIPO_THRESH  378 // 90% of 4.2V (*100) - dont start charging the lipo cell if it's above 90%
#define              LIPO_ALRM  300 // minimum lipo voltage
#define              LIPO_FULL  415 // charged lipo voltage
#define              PV_THRESH  620 // LM2675 needs a minimum of 6V-6.5V input voltage to work (*100)

void pwr_mng(struct adc_conv *adc_c);

#endif
