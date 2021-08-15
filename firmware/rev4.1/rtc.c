
/*
    rtca.c: TI CC430 Hardware Realtime Clock (RTC_A)

    Copyright (C) 2011-2012 Angelo Arrifano <miknix@gmail.com>

				http://www.openchronos-ng.sourceforge.net

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <inttypes.h>
#include "rtc.h"
#include "rtca_now.h"
#include "proj.h" // FIXME remove

#ifdef CONFIG_RTC_DST
#include "rtc_dst.h"
#endif

volatile enum rtca_tevent rtca_last_event;
static struct rtca_tm rtca_time;

void rtca_init(void)
{
    rtca_time.year = COMPILE_YEAR;
    rtca_time.mon = COMPILE_MON;
    rtca_time.day = COMPILE_DAY;
    rtca_time.dow = COMPILE_DOW;
    rtca_time.hour = COMPILE_HOUR;
    rtca_time.min = COMPILE_MIN;
    rtca_time.sec = 59;

    /* Enable calendar mode (date/time registers are automatically reset)
       and enable read ready interrupts
       and set time event interrupts at each minute
       also enable alarm interrupts */
    RTCCTL01 |= RTCMODE | RTCRDYIE | RTCTEVIE;

    rtca_set_time(&rtca_time);

#ifdef CONFIG_RTC_DST
    /* initialize DST module */
    rtc_dst_init();
#endif

}

void rtca_set_time(struct rtca_tm *t)
{
    // Stop RTC timekeeping for a while
    rtca_stop();

    // update RTC registers
    RTCSEC = t->sec;
    RTCMIN = t->min;
    RTCHOUR = t->hour;
    RTCDAY = t->day;
    RTCDOW = t->dow;
    RTCMON = t->mon;
    RTCYEARL = t->year & 0xff;
    RTCYEARH = t->year >> 8;

    // Resume RTC time keeping
    rtca_start();
}

void rtca_get_time(struct rtca_tm *t)
{
    t->sys = rtca_time.sys;
    t->sec = rtca_time.sec;
    t->min = rtca_time.min;
    t->hour = rtca_time.hour;
    t->day = rtca_time.day;
    t->dow = rtca_time.dow;
    t->mon = rtca_time.mon;
    t->year = rtca_time.year;
}

uint8_t rtca_get_event(void)
{
    return rtca_last_event;
}

void rtca_rst_event(void)
{
    rtca_last_event = RTCA_EV_NONE;
}

void rtca_get_alarm(uint8_t *hour, uint8_t *min)
{
	*hour = RTCAHOUR & 0x1F;
	*min  = RTCAMIN  & 0x3F;
}

void rtca_set_alarm(uint8_t hour, uint8_t min)
{
    uint16_t state = RTCCTL01;
    RTCCTL01 &= ~(RTCAIE | RTCAIFG);
	RTCAMIN  = (RTCAMIN & RTCAE) | min;
	RTCAHOUR = (RTCAHOUR & RTCAE) | hour;
    RTCADOW = 0;
    RTCADAY = 0;
    RTCCTL01 = state;
}

void rtca_enable_alarm()
{
//	RTCCTL01 &= ~(RTCAIE);
//	RTCAHOUR |= RTCAE;
//	RTCAMIN  |= RTCAE;
    RTCCTL01 &= ~(RTCAIFG);
	RTCCTL01 |= RTCAIE;
}

void rtca_disable_alarm()
{
	RTCCTL01 &= ~RTCAIE;
	RTCAHOUR &= ~RTCAE;
	RTCAMIN  &= ~RTCAE;
}

__attribute__ ((interrupt(RTC_VECTOR)))
void RTC_A_ISR(void)
{
    /* the IV is cleared after a read, so we store it */
    uint16_t iv = RTCIV;

    // copy register values
    rtca_time.sec = RTCSEC;

    // count system time
    rtca_time.sys++;

    enum rtca_tevent ev = 0;

    /* second event (from the read ready interrupt flag) */
    if (iv == RTCIV_RTCRDYIFG) {
        ev = RTCA_EV_SECOND;
        goto finish;
    }

    if (iv == RTCIV_RTCAIFG) {
        sig0_on;
        ev = RTCA_EV_ALARM;
        goto finish;
    }

    {
        if (iv != RTCIV_RTCTEVIFG)      /* Minute changed! */
            goto finish;

        ev |= RTCA_EV_MINUTE;
        rtca_time.min = RTCMIN;

        if (rtca_time.min != 0) /* Hour changed */
            goto finish;

        ev |= RTCA_EV_HOUR;
        rtca_time.hour = RTCHOUR;

#ifdef CONFIG_RTC_DST
        rtc_dst_hourly_update();
#endif

        if (rtca_time.hour != 0)        /* Day changed */
            goto finish;

        ev |= RTCA_EV_DAY;
        rtca_time.day = RTCDAY;
        rtca_time.dow = RTCDOW;

        if (rtca_time.day != 1) /* Month changed */
            goto finish;

        ev |= RTCA_EV_MONTH;
        rtca_time.mon = RTCMON;

        if (rtca_time.mon != 1) /* Year changed */
            goto finish;

        ev |= RTCA_EV_YEAR;
        rtca_time.year = RTCYEARL | (RTCYEARH << 8);
#ifdef CONFIG_RTC_DST
        /* calculate new DST switch dates */
        rtc_dst_calculate_dates(rtca_time.year, rtca_time.mon, rtca_time.day,
                                rtca_time.hour);
#endif
    }

 finish:
    /* append events, since ISR could be triggered
       multipe times until rtca_last_event gets parsed */
    rtca_last_event |= ev;

    /* exit from LPM3, give execution back to mainloop */
    _BIC_SR_IRQ(LPM3_bits);
}
