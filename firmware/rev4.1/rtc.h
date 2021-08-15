#ifndef __RTCA_H__
#define __RTCA_H__

#include "proj.h"

enum rtca_tevent {
    RTCA_EV_NONE = 0,
    RTCA_EV_ALARM = BIT0,
    RTCA_EV_SECOND = BIT1,
    RTCA_EV_MINUTE = BIT2,
    RTCA_EV_HOUR = BIT3,
    RTCA_EV_DAY = BIT4,
    RTCA_EV_MONTH = BIT5,
    RTCA_EV_YEAR = BIT6
};

struct rtca_tm {
    uint32_t sys;   // system time: number of seconds since power on
    uint16_t year;  // year 1901-2099
    uint8_t mon;    // month 1-12
    uint8_t day;    // day of month 1-31
    uint8_t dow;    // day of week 1 Monday, 2 Tuesday
    uint8_t hour;   // hour
    uint8_t min;    // minute
    uint8_t sec;    // second
};

#define rtca_stop()		(RTCCTL01 |=  RTCHOLD)
#define rtca_start()	(RTCCTL01 &= ~RTCHOLD)

/* the ev variable holds the time event, see enum rtca_tevent for more info.
please add -fshort-enums to CFLAGS to store rtca_tevent as only a byte */
void rtca_init(void);
void rtca_set_time(struct rtca_tm *t);
void rtca_get_time(struct rtca_tm *t);

uint8_t rtca_get_event(void);
void rtca_rst_event(void);

void rtca_get_alarm(uint8_t *hour, uint8_t *min);
void rtca_set_alarm(uint8_t hour, uint8_t min);
void rtca_enable_alarm();
void rtca_disable_alarm();

#endif
