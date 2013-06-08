
//  sensor control board based on a MSP430F5510 uC
//  author:          Petre Rodan <petre.rodan@simplex.ro>
//  available from:  https://github.com/rodan/
//  license:         GNU GPLv3


#include <stdio.h>
#include <string.h>

#include "sc.h"
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

// DIR is defined as "0x0001 - USB Data Response Bit" in msp430 headers
// but it's also used by fatfs
#undef DIR
#include "fatfs/ff.h"

char str_temp[64];
FATFS fatfs;
FIL f;

float vref=2.4405;

void die(uint8_t loc, FRESULT rc)
{
    sprintf(str_temp, "l=%d rc=%u\r\n", loc, rc);
    uart_tx_str(str_temp, strlen(str_temp));
}

static void do_smth(enum sys_message msg)
{
    //sprintf(str_temp, "%02d:%02d:%02d ", rtca_time.hour, rtca_time.min, rtca_time.sec);
    //uart_tx_str(str_temp, strlen(str_temp));

    //
    // test of ADC
    //

    uint16_t a2 = 0, int_temp = 0, int_temp_c;
    float vin2;
    //adc10_read(10, &int_temp);
    //timer_a0_delay(1000);
    //int_temp_c = ((int_temp*1.4662)-688)*3.968; // see temperature sensor transfer function
                                                // in slau208 datasheet page ~707
    adc10_read(2, &a2);
    timer_a0_delay(1000);
    //a2 *= 0.4586; // Vao = a2 * (Vref / 1023) * ((R1+R2) / R2 ) / 10
                 //   where Vref=1500, R1=2000000, R2=240000
    //a2 *= 0.00228;
    vin2 = a2 * vref * 0.9225;
    adc10_halt();
    //sprintf(str_temp, "a2=%d.%02dV temp=%d.%01ddC\r\n", a2/100, a2%100, int_temp_c/10, int_temp_c%10);
    sprintf(str_temp, "a2=%d vin=%d.%02dV\r\n", a2, (uint16_t) vin2/100, (uint16_t) vin2%100);
            //temp=%d.%01ddC\r\n", a2/100, a2%100, int_temp_c/10, int_temp_c%10);
    uart_tx_str(str_temp, strlen(str_temp));

    /*
    //
    // test of uSD card
    //

    FRESULT rc;
    f_mount(0, &fatfs);

    if (detectCard()) {
        uint16_t bw;
        rc = f_open(&f, "20130126.LOG", FA_WRITE | FA_OPEN_ALWAYS);
        if (!rc) {
            f_lseek(&f, f_size(&f));
            f_write(&f, str_temp, strlen(str_temp), &bw);
            f_close(&f);
        } else {
            die(2, rc);
        }

        DIR dir;
        FILINFO fno;
        rc = f_opendir(&dir, "");
        if (!rc) {
            for (;;) {
                rc = f_readdir(&dir, &fno);
                if (rc || !fno.fname[0])
                    break;
                if (fno.fattrib & AM_DIR) {
                    sprintf(str_temp, "   <dir>  %s\r\n", fno.fname);
                } else {
                    sprintf(str_temp, "%8lu  %s\r\n", fno.fsize, fno.fname);
                }
                uart_tx_str(str_temp, strlen(str_temp));
            }
        } else {
            die(1, rc);
        }
    }
    */

    /*
    //
    // test of LCD
    //

    // increase drive strenght of P5.1 (LCD-PWR)
    //P5DS  = 0x02;
    //P5OUT |= BIT1;
    //LCD_Send_STR(1, str_temp);
    */

    P4OUT ^= BIT7;              // blink led

    // oled

#ifdef HSC_SSC
    uint16_t t;
    uint32_t p;
    struct cs_raw ps;
    uint8_t rv1;

    rv1 = ps_get_raw(PS_SLAVE_ADDR, &ps);
    if (rv1 == I2C_ACK) {
        ps_convert(ps, &p, &t, OUTPUT_MIN, OUTPUT_MAX, PRESSURE_MIN, PRESSURE_MAX);

        sprintf(str_temp, "stat %d\r\n", ps.status);
        uart_tx_str(str_temp, strlen(str_temp));
        sprintf(str_temp, "%06ld\r\n", p);
        uart_tx_str(str_temp, strlen(str_temp));
        sprintf(str_temp, "%02d.%02d\r\n", t / 100, t % 100);
        uart_tx_str(str_temp, strlen(str_temp));
    } else {
        sprintf(str_temp, "p %d\r\n", rv1);
        uart_tx_str(str_temp, strlen(str_temp));
    }
#endif

#ifdef SHT15
    uint16_t t2, rh;
    uint8_t rv2;
    rv2 = sht_get_meas(&t2, &rh);
    if (rv2 == I2C_ACK) {
        sprintf(str_temp, "t2 %05d\r\n", t2);
        uart_tx_str(str_temp, strlen(str_temp));
        sprintf(str_temp, "rh %05d\r\n", rh);
        uart_tx_str(str_temp, strlen(str_temp));
    } else {
        sprintf(str_temp, "h %d\r\n", rv2);
        uart_tx_str(str_temp, strlen(str_temp));
    }
#endif

#ifdef INTERTECHNO
    uint8_t family = 0xb;       // this translates as family 'L' on the rotary switch
    uint8_t device = 0x7;
    uint8_t prefix = ( family << 4 ) + device;
    // ding dong
    rf_tx_cmd(prefix, INTERTECHNO_CMD_SP);
#endif
}

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
    //opt_power_enable();
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

    //disk_initialize(0);
    //LCD_Init();
    //sys_messagebus_register(&do_smth, SYS_MSG_RTC_MINUTE);

    sys_messagebus_register(&do_smth, SYS_MSG_RTC_SECOND);

    while (1) {
        sleep();
        __no_operation();
        wake_up();
        //check_ir();
        //timer0_delay(1000, LPM3_bits);
        //P4OUT ^= BIT7;              // blink led
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

    // watchdog triggers after 16 seconds when not cleared
#ifdef USE_WATCHDOG
    WDTCTL = WDTPW + WDTIS__512K + WDTSSEL__ACLK;
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

    P4SEL = 0x0;
    P4DIR = 0xfe;
    P4OUT = 0x0;
    P4REN = 0x1;

    //P5SEL is set above
    P5DIR = 0x0;
    P5OUT = 0x0;

    P6SEL = 0x5;
    P6DIR = 0xFA;
    P6OUT = 0x0;

    PJOUT = 0x00;
    PJDIR = 0xFF;

    /*
       __disable_interrupt();
       // get write-access to port mapping registers
       PMAPPWD = 0x02D52;
       PMAPCTL = PMAPRECFG;
       // MCLK set out to 4.6
       P4MAP6 = PM_MCLK;
       PMAPPWD = 0;
       __enable_interrupt();

       P4DIR |= BIT6;
       P4SEL |= BIT6;
     */

    rtca_init();
    timer_a0_init();
}

void sleep(void)
{
    // turn off internal VREF, XT2, i2c power
    //REFCTL0 = 0;
    UCSCTL6 |= XT2OFF;
    //opt_power_disable();
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
    //opt_power_enable();
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
    P1DIR |= BIT6;
    P1OUT &= ~BIT6;
}

void opt_power_disable()
{
    P1DIR &= ~BIT6;
    P5DIR &= ~(BIT0 + BIT1);
}
