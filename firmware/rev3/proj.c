
//  sensor control board based on a MSP430F5510 uC
//   - compatible with hardware rev 03 -
//
//  author:          Petre Rodan <petre.rodan@simplex.ro>
//  available from:  https://github.com/rodan/
//  license:         GNU GPLv3

#include <stdio.h>
#include <string.h>

#include "proj.h"
#include "calib.h"
#include "drivers/sys_messagebus.h"
#include "drivers/pmm.h"
#include "drivers/rtc.h"
#include "drivers/timer_a0.h"
#include "drivers/uart1.h"
#include "drivers/diskio.h"
#include "drivers/mmc.h"
#include "drivers/adc.h"
#include "drivers/hal_sdcard.h"

// DIR is defined as "0x0001 - USB Data Response Bit" in msp430 headers
// but it's also used by fatfs
#undef DIR
#include "fatfs/ff.h"

char str_temp[64];
FATFS fatfs;
DIR dir;
FIL f;

float v_bat, v_pv, i_ch, t_th, t_int;

// acts register
// 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
//                                   |  |  |  |  |
//                                   |  |  |  |   --> first start
//                                   |  |  |   -----> charge stopped, batt overcharged
//                                   |  |   --------> charge stopped, pv too low
//                                   |   -----------> 
//                                    --------------> enable charging

uint16_t acts;
uint16_t acts_last;

void die(uint8_t loc, FRESULT rc)
{
    sprintf(str_temp, "l=%d rc=%u\r\n", loc, rc);
    uart1_tx_str(str_temp, strlen(str_temp));
}

#ifdef CALIBRATION
static void do_calib(enum sys_message msg)
{
    uint16_t q_bat = 0, q_pv = 0, q_t_int = 0, q_ch = 0, q_th = 0;

    opt_power_enable;
    timer_a0_delay(300000);

    adc10_read(0, &q_bat, REFVSEL_2);
    v_bat = q_bat * VREF_2_5_6_0 * DIV_BAT;
    adc10_read(1, &q_pv, REFVSEL_2);
    v_pv = q_pv * VREF_2_5_6_1 * DIV_PV;
    
    adc10_read(10, &q_t_int, REFVSEL_0);
    //t_int = ((q_t_int * VREF_1_5) / 102.3 - 6.88) * 396.8;
    t_int = 10.0 * ( q_t_int * T_INT_B + T_INT_A );

    adc10_read(9, &q_ch, REFVSEL_2);
    i_ch = 100.0 * ((q_ch * VREF_2_5_5_1 / 1023) * INA168_B + INA168_A);
    adc10_read(8, &q_th, REFVSEL_2);
    t_th = 10.0 * ((q_th * VREF_2_5_5_0 / 1023) * TH_B + TH_A);

    adc10_halt();
    opt_power_disable;

    //-21.3 | bat 1023 22.22 | pv 1023 22.22DA0
    snprintf(str_temp, 42,
             "%s%d.%1d | bat % 4d %02d.%02d | pv % 4d %02d.%02d\r\n",
             t_int < 0 ? "-": "", abs((int16_t) t_int / 10), abs((int16_t) t_int % 10),
             q_bat, (uint16_t) v_bat / 100, (uint16_t) v_bat % 100, q_pv,
             (uint16_t) v_pv / 100, (uint16_t) v_pv % 100);
    uart1_tx_str(str_temp, strlen(str_temp));

    snprintf(str_temp, 42,
             "ch: % 4d %01d.%02d | th % 4d  %s%d.%02d\r\n",
             q_ch, (uint16_t) i_ch / 100, (uint16_t) i_ch % 100,
             q_th, t_th < 0 ? "-": "", abs((int16_t) t_th / 10), abs((int16_t) t_th % 10),
    uart1_tx_str(str_temp, strlen(str_temp));

    snprintf(str_temp, 35,
             "t_int: % 4d 30: %d, 85: %d \r\n",
             q_t_int,
             *(uint16_t *)0x1a1a, *(uint16_t *)0x1a1c);
    uart1_tx_str(str_temp, strlen(str_temp));

    // blinky
    //P4OUT ^= BIT7;
}
#else
static void main_loop(enum sys_message msg)
{
    uint16_t q_bat = 0;
    uint16_t q_pv = 0, q_t_int = 0;
    uint16_t q_ch = 0, q_th = 0;

    acts = 0;

    adc10_read(0, &q_bat, REFVSEL_2);
    v_bat = q_bat * VREF_2_5_6_0 * DIV_BAT;

    // values are multiplied by 100 for snprintf
    if (v_bat < 350) {
        // do nothing since we run on the 3v Li cell
        adc10_halt();
        return;
    } else if (v_bat > 1400) {
        charge_disable;
        acts |= BIT1;
    }

    adc10_read(1, &q_pv, REFVSEL_2);
    v_pv = q_pv * VREF_2_5_6_1 * DIV_PV;
    adc10_read(9, &q_ch, REFVSEL_2);
    i_ch = 100.0 * ((q_ch * VREF_2_5_5_1 / 1023) * INA168_B + INA168_A);
    adc10_read(10, &q_t_int, REFVSEL_0);
    //t_int = ((q_t_int * VREF_1_5) / 102.3 - 6.88) * 396.8;
    t_int = 10.0 * ( q_t_int * T_INT_B + T_INT_A );
    opt_power_enable;
    timer_a0_delay(100000);
    adc10_read(8, &q_th, REFVSEL_2);
    t_th = ((q_th * VREF_2_5_5_0 / 1023) * TH_B + TH_A) * 10.0;
    adc10_halt();

    if (v_pv < v_bat + 100) {
        charge_disable;
        acts |= BIT2;
    }

    if ((v_pv > 1570) && (v_pv < 3000) && (v_bat < 1400)) {
        charge_enable;
        acts |= BIT4;
    }

    if (acts == 0) {
        acts = acts_last;
    }

    if ((acts != acts_last) || (rtca_time.min % 10 == 0)) {
    //if (true) {

        FRESULT rc;
        f_mount(&fatfs, "", 0);
        disk_initialize(0);
        rc = detectCard();

        if (rc) {
            uint16_t bw;
            f_opendir(&dir, "/");
            snprintf(str_temp, 5, "%d", rtca_time.year);
            rc = f_mkdir(str_temp);
            if ((rc == FR_OK) || (rc == FR_EXIST)) {
                snprintf(str_temp, 9, "/%d/%02d", rtca_time.year, rtca_time.mon);
                rc = f_open(&f, str_temp, FA_WRITE | FA_OPEN_ALWAYS);
                if (!rc) {
                    f_lseek(&f, f_size(&f));
                    //20130620 07:12 -12.2 -34.5 12.30 13.40 1.22 0xffffDA0//53
                    snprintf(str_temp, 63,
                             "%04d%02d%02d %02d:%02d %s%d.%1d %s%d.%1d %02d.%02d %02d.%02d %01d.%02d 0x%04x\r\n",
                             rtca_time.year, rtca_time.mon, rtca_time.day,
                             rtca_time.hour, rtca_time.min,
                             t_int < 0 ? "-": "", abs((int16_t) t_int / 10), abs((int16_t) t_int % 10),
                             t_th < 0 ? "-": "", abs((int16_t) t_th / 10), abs((int16_t) t_th % 10),
                             (uint16_t) v_bat / 100, (uint16_t) v_bat % 100,
                             (uint16_t) v_pv / 100, (uint16_t) v_pv % 100,
                             (uint16_t) i_ch / 100, (uint16_t) i_ch % 100, 
                             acts);
                    f_write(&f, str_temp, strlen(str_temp), &bw);
                    f_close(&f);
                    uart1_tx_str(str_temp, strlen(str_temp));
                } else {
                    die(2, rc);
                }
            } else {
                die(3, rc);
            }
        } else {
            die(1, rc);
        }
        f_mount(NULL, "", 0);
        SDCard_end();
        opt_power_disable;
        acts_last = acts;
    }
}
#endif                          // !CALIBRATION

int main(void)
{
    main_init();
    uart1_init();
#ifdef IR_REMOTE
    ir_init();
#endif
#ifdef INTERTECHNO
    it_init();
#endif

    charge_disable;

#ifdef CALIBRATION
    sys_messagebus_register(&do_calib, SYS_MSG_RTC_MINUTE);
#else
    sys_messagebus_register(&main_loop, SYS_MSG_RTC_MINUTE);
#endif

    while (1) {
        sleep();
        __no_operation();
        //wake_up();
#ifdef USE_WATCHDOG
        // reset watchdog counter
        WDTCTL = (WDTCTL & 0xff) | WDTPW | WDTCNTCL;
#endif
        check_events();
    }
}

void main_init(void)
{

    // watchdog triggers after 4 minutes when not cleared
#ifdef USE_WATCHDOG
    WDTCTL = WDTPW + WDTIS__8192K + WDTSSEL__ACLK + WDTCNTCL;
#else
    WDTCTL = WDTPW + WDTHOLD;
#endif
    SetVCore(3);

    // select XT1 and XT2 ports
    // select 12pF, enable both crystals
    P5SEL |= BIT5 + BIT4;
    
    // hf crystal
    /*
    uint16_t timeout = 5000;

    P5SEL |= BIT3 + BIT2;
    //UCSCTL6 |= XCAP0 | XCAP1;
    UCSCTL6 &= ~(XT1OFF + XT2OFF);
    UCSCTL3 = SELREF__XT2CLK;
    UCSCTL4 = SELA__XT1CLK | SELS__XT2CLK | SELM__XT2CLK;
    // wait until clocks settle
    do {
        UCSCTL7 &= ~(XT2OFFG + XT1LFOFFG + DCOFFG);
        SFRIFG1 &= ~OFIFG;
        timeout--;
    } while ((SFRIFG1 & OFIFG) && timeout);
    // decrease power
    //UCSCTL6 &= ~(XT2DRIVE0 + XT1DRIVE0);
    */
    UCSCTL6 &= ~(XT1OFF | XT1DRIVE0);

    P1SEL = 0x0;
    P1DIR = 0xf0;
    P1OUT = 0x40;
    P1REN = 0x0f;

    P2SEL = 0x0;
    P2DIR = 0x1;
    P2OUT = 0x0;

    P3SEL = 0x0;
    P3DIR = 0x1f;
    P3OUT = 0x0;

    P4SEL = 0x0e;
    P4DIR = 0xfe;
    P4REN = 0x01;
    P4OUT = 0x0;

    //P5SEL is set above
    P5SEL |= 0x3;
    P5DIR = 0x0;
    P5OUT = 0x0;

    P6SEL = 0x3;
    P6DIR = 0x0;
    P6OUT = 0x0;

    PJOUT = 0x00;
    PJDIR = 0xFF;

#ifdef CALIBRATION
    // send MCLK to P4.0
    __disable_interrupt();
    // get write-access to port mapping registers
    //PMAPPWD = 0x02D52;
    PMAPPWD = PMAPKEY;
    PMAPCTL = PMAPRECFG;
    // MCLK set out to 4.0
    P4MAP0 = PM_MCLK;
    //P4MAP0 = PM_RTCCLK;
    PMAPPWD = 0;
    __enable_interrupt();
    P4DIR |= BIT0;
    P4SEL |= BIT0;

    // send ACLK to P1.0
    P1DIR |= BIT0;
    P1SEL |= BIT0;
#endif

    // disable VUSB LDO and SLDO
    USBKEYPID = 0x9628;
    USBPWRCTL &= ~(SLDOEN + VUSBEN);
    USBKEYPID = 0x9600;

    acts_last = -1L;
    v_bat = 0;

    rtca_init();
    timer_a0_init();
}

void sleep(void)
{
    opt_power_disable;
    // turn off internal VREF, XT2, i2c power
    //UCSCTL6 |= XT2OFF;
    //PMMCTL0_H = 0xA5;
    //SVSMHCTL &= ~SVMHE;
    //SVSMLCTL &= ~(SVSLE+SVMLE);
    //PMMCTL0_H = 0x00;
    _BIS_SR(LPM3_bits + GIE);
    __no_operation();
}

/*
void wake_up(void)
{
    uint16_t timeout = 5000;
    UCSCTL6 &= ~XT2OFF;
    // wait until clocks settle
    do {
        //UCSCTL7 &= ~(XT2OFFG + XT1LFOFFG + DCOFFG);
        UCSCTL7 &= ~(XT1LFOFFG + DCOFFG);
        SFRIFG1 &= ~OFIFG;
        timeout--;
    } while ((SFRIFG1 & OFIFG) && timeout);
}
*/

void check_events(void)
{
    struct sys_messagebus *p = messagebus;
    enum sys_message msg = 0;

    // drivers/rtca
    if (rtca_last_event) {
        msg |= rtca_last_event;
        rtca_last_event = 0;
    }
    while (p) {
        // notify listener if he registered for any of these messages
        if (msg & p->listens) {
            p->fn(msg);
        }
        p = p->next;
    }
}

