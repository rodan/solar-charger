
//  sensor control board based on a MSP430F5510 uC
//  author:          Petre Rodan <petre.rodan@simplex.ro>
//  available from:  https://github.com/rodan/
//  license:         GNU GPLv3

#include <stdio.h>
#include <string.h>

#include "sc.h"
#include "calib.h"
#include "drivers/sys_messagebus.h"
#include "drivers/pmm.h"
#include "drivers/rtc.h"
#include "drivers/timer_a1.h"
#include "drivers/timer_a0.h"
#include "drivers/lcd.h"
#include "drivers/uart.h"
#include "drivers/ir_remote.h"
#include "drivers/intertechno.h"
#include "drivers/serial_bitbang.h"
#include "drivers/hsc_ssc_i2c.h"
#include "drivers/sensirion.h"
#include "drivers/oled_128x64.h"
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

float v_bat, v_bat_old, v_pv, itemp;

uint8_t first_start, last_ch_ena;
uint8_t relay_ch_ena, relay_opt_ena;
uint32_t status_last;

// relay_acts register
// 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
//  opt deact |  opt act  | ch deact  | ch act
// bits[0-3]   - number of charging relay activations per 1min cycle
// bits[4-7]   - number of charging relay deactivations per 1min cycle
// bits[8-11]  - number of opt relay activations per 1min cycle
// bits[12-15] - number of opt relay deactivations per 1min cycle
//
// acts register
// 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
//                                   |  |  |  |  |
//                                   |  |  |  |   --> first start
//                                   |  |  |   -----> charge stopped, batt overcharged
//                                   |  |   --------> start up the charge controller
//                                   |   -----------> stop the charge to read v_pv
//                                    --------------> enable charging

uint16_t acts, relay_acts;

void die(uint8_t loc, FRESULT rc)
{
    sprintf(str_temp, "l=%d rc=%u\r\n", loc, rc);
    uart_tx_str(str_temp, strlen(str_temp));
}

#ifdef CALIBRATION
static void do_calib(enum sys_message msg)
{
    uint16_t q_bat = 0, q_pv = 0, q_itemp = 0;

    adc10_read(0, &q_bat, REFVSEL_2);
    v_bat = q_bat * VREF_2_5 * DIV_BAT;
    adc10_read(2, &q_pv, REFVSEL_2);
    v_pv = q_pv * VREF_2_5 * DIV_PV;
    adc10_read(10, &q_itemp, REFVSEL_0);
    itemp = ((q_itemp * VREF_1_5) / 102.3 - 6.88) * 396.8;
    adc10_halt();

    //-21.3 | bat 1023 22.22 | pv 1023 22.22DA0
    snprintf(str_temp, 42,
             "% 2d.%1d | bat % 4d %02d.%02d | pv % 4d %02d.%02d\r\n",
             (uint16_t) itemp / 10, (uint16_t) itemp % 10, q_bat,
             (uint16_t) v_bat / 100, (uint16_t) v_bat % 100, q_pv,
             (uint16_t) v_pv / 100, (uint16_t) v_pv % 100);
    uart_tx_str(str_temp, strlen(str_temp));

}
#else
static void do_smth(enum sys_message msg)
{
    uint16_t q_bat = 0;
    uint16_t q_pv = 0, q_itemp = 0;
    uint32_t status;

    relay_acts = 0;
    acts = 0;

    v_bat_old = v_bat;
    adc10_read(0, &q_bat, REFVSEL_2);
    v_bat = q_bat * VREF_2_5 * DIV_BAT;

    // this has to be done periodically because 
    // v_bat could be < RELAY_MIN_V during main_init()
    if (first_start) {
        relay_ch_ena = true;
        relay_opt_ena = true;
        sw_disable();
        charge_disable();
        acts |= BIT0;
    }
    // values are multiplied by 100 for snprintf
    if (v_bat < 300) {
        // do nothing since we run on the 3v Li cell
        adc10_halt();
        return;
    } else if (v_bat > 1410) {
        charge_disable();
        acts |= BIT1;
    } else if ((!relay_ch_ena) && (last_ch_ena != rtca_time.hour)) {
        // once an hour give some juice to the charge controller
        charge_enable();
        timer_a0_delay(50000);
        charge_disable();
        acts |= BIT2;
    }
    // once a while switch off the charge relay so we can read the PV voltage
    if (relay_ch_ena && ((rtca_time.min % 10 == 1) || (v_bat_old - v_bat > 10))) {
        charge_disable();
        // wait for cap to discharge
        timer_a0_delay(1000000);
        acts |= BIT3;
    }

    adc10_read(2, &q_pv, REFVSEL_2);
    v_pv = q_pv * VREF_2_5 * DIV_PV;
    // see temperature sensor transfer function
    // in slau208 datasheet page ~707
    adc10_read(10, &q_itemp, REFVSEL_0);
    itemp = ((q_itemp * VREF_1_5) / 102.3 - 6.88) * 396.8;
    adc10_halt();

    if (!relay_ch_ena) {
        if ((v_pv > v_bat) && (v_pv < 1900) && (v_bat < 1410)) {
            charge_enable();
            acts |= BIT4;
        }
    }

    status = relay_acts + ((uint32_t) acts << 16) + ((uint32_t) relay_ch_ena << 24) + ((uint32_t) relay_opt_ena << 25);

    if (status != status_last) {

        FRESULT rc;
        f_mount(0, &fatfs);

        opt_power_enable();
        timer_a0_delay(50000);
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
                    //20130620 07:12 -34.5 12.30 13.40 1 1 0xffff 0xffffDA0
                    snprintf(str_temp, 53,
                             "%04d%02d%02d %02d:%02d % 2d.%1d %02d.%02d %02d.%02d %d %d 0x%04x 0x%04x\r\n",
                             rtca_time.year, rtca_time.mon, rtca_time.day,
                             rtca_time.hour, rtca_time.min,
                             (uint16_t) itemp / 10, (uint16_t) itemp % 10,
                             (uint16_t) v_bat / 100, (uint16_t) v_bat % 100,
                             (uint16_t) v_pv / 100, (uint16_t) v_pv % 100,
                             relay_ch_ena, relay_opt_ena, relay_acts, acts);
                    f_write(&f, str_temp, strlen(str_temp), &bw);
                    f_close(&f);
                    uart_tx_str(str_temp, strlen(str_temp));
                } else {
                    die(2, rc);
                }
            } else {
                die(3, rc);
            }
        } else {
            die(1, rc);
        }
        f_mount(0, NULL);
        SDCard_end();
        opt_power_disable();
        status_last = status;
    }
    //P4OUT ^= BIT7;              // blink led
}
#endif                          // !CALIBRATION

void check_ir(void)
{
    if (ir_decode(&results)) {
        sprintf(str_temp, "%ld\r\n", results.value);
        uart_tx_str(str_temp, strlen(str_temp));
        ir_resume();            // Receive the next value
    }
}

int main(void)
{
    main_init();
    uart_init();
#ifdef IR_REMOTE
    ir_init();
#endif
#ifdef INTERTECHNO
    it_init();
#endif

    // oled display
    /*
       timer_a0_delay(10000);
       oled_128x64_init();
       oled_128x64_clear_display();          //clear the screen and set start position to top left corner
       oled_128x64_set_normal_display();      //Set display to normal mode (i.e non-inverse mode)
       oled_128x64_set_page_mode();           //Set addressing mode to Page Mode
       oled_128x64_set_text_xy(0,0);          //Set the cursor to Xth Page, Yth Column
       oled_128x64_put_string("Hello World!11"); //Print the String
     */

    //LCD_Init();
#ifdef CALIBRATION
    sys_messagebus_register(&do_calib, SYS_MSG_RTC_SECOND);
#else
    sys_messagebus_register(&do_smth, SYS_MSG_RTC_MINUTE);
#endif

    while (1) {
        sleep();
        __no_operation();
        wake_up();
        //check_ir();
#ifdef USE_WATCHDOG
        // reset watchdog counter
        WDTCTL = (WDTCTL & 0xff) | WDTPW | WDTCNTCL;
#endif
        check_events();
    }
}

void main_init(void)
{
    uint16_t timeout = 5000;

    // watchdog triggers after 4 minutes when not cleared
#ifdef USE_WATCHDOG
    WDTCTL = WDTPW + WDTIS__8192K + WDTSSEL__ACLK + WDTCNTCL;
#else
    WDTCTL = WDTPW + WDTHOLD;
#endif
    SetVCore(3);

    // select XT1 and XT2 ports
    // select 12pF, enable both crystals
    P5SEL |= BIT5 + BIT4 + BIT3 + BIT2;
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
    UCSCTL6 &= ~(XT2DRIVE0 + XT1DRIVE0);

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
    P5DIR = 0x0;
    P5OUT = 0x0;

    P6SEL = 0x5;
    P6DIR = 0xFA;
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

    relay_ch_ena = false;
    relay_opt_ena = false;
    first_start = true;
    last_ch_ena = 0;
    status_last = -1L;
    v_bat = 0;

    rtca_init();
    timer_a0_init();
}

void sleep(void)
{
    // turn off internal VREF, XT2, i2c power
    UCSCTL6 |= XT2OFF;
    // disable VUSB LDO and SLDO
    USBKEYPID = 0x9628;
    USBPWRCTL &= ~(SLDOEN + VUSBEN);
    USBKEYPID = 0x9600;
    //PMMCTL0_H = 0xA5;
    //SVSMHCTL &= ~SVMHE;
    //SVSMLCTL &= ~(SVSLE+SVMLE);
    //PMMCTL0_H = 0x00;
    _BIS_SR(LPM3_bits + GIE);
    __no_operation();
}

void wake_up(void)
{
    uint16_t timeout = 5000;
    UCSCTL6 &= ~XT2OFF;
    // wait until clocks settle
    do {
        UCSCTL7 &= ~(XT2OFFG + XT1LFOFFG + DCOFFG);
        SFRIFG1 &= ~OFIFG;
        timeout--;
    } while ((SFRIFG1 & OFIFG) && timeout);
}

void check_events(void)
{
    struct sys_messagebus *p = messagebus;
    enum sys_message msg = 0;

    // drivers/rtca
    if (rtca_last_event) {
        msg |= rtca_last_event;
        rtca_last_event = 0;
    }
    // drivers/timer1a
    if (timer_a1_last_event) {
        msg |= timer_a1_last_event << 7;
        timer_a1_last_event = 0;
    }
    while (p) {
        // notify listener if he registered for any of these messages
        if (msg & p->listens) {
            p->fn(msg);
        }
        p = p->next;
    }
}

void opt_power_enable()
{
    P1OUT &= ~BIT6;
}

void opt_power_disable()
{
    P1OUT |= BIT6;
    P5DIR &= ~(BIT0 + BIT1);
}

void charge_enable(void)
{
    if ((!relay_ch_ena) && (v_bat > RELAY_MIN_V)) {
        P1OUT &= ~BIT5;
        timer_a0_delay(10000);
        P1OUT |= BIT4;
        timer_a0_delay(50000);
        P1OUT &= ~BIT4;
        relay_ch_ena = true;
        relay_acts += 1;
        last_ch_ena = rtca_time.hour;
    }
}

void charge_disable(void)
{
    if (relay_ch_ena && (v_bat > RELAY_MIN_V)) {
        P1OUT &= ~BIT4;
        timer_a0_delay(10000);
        P1OUT |= BIT5;
        timer_a0_delay(50000);
        P1OUT &= ~BIT5;
        relay_ch_ena = false;
        relay_acts += 16;
        first_start = false;
    }
}

void sw_enable(void)
{
    if ((!relay_opt_ena) && (v_bat > RELAY_MIN_V)) {
        P2OUT &= ~BIT0;
        timer_a0_delay(10000);
        P1OUT |= BIT7;
        timer_a0_delay(50000);
        P1OUT &= ~BIT7;
        relay_opt_ena = true;
        relay_acts += 256;
    }
}

void sw_disable(void)
{

    if (relay_opt_ena && (v_bat > RELAY_MIN_V)) {
        P1OUT &= ~BIT7;
        timer_a0_delay(10000);
        P2OUT |= BIT0;
        timer_a0_delay(50000);
        P2OUT &= ~BIT0;
        relay_opt_ena = false;
        first_start = false;
        relay_acts += 4096;
    }
}
