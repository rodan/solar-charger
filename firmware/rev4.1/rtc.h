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

// ae_flags  - alarm enable flags
#define    AE_MIN  0x01
#define   AE_HOUR  0x02
#define    AE_DAY  0x04
#define    AE_DOW  0x08

/* the ev variable holds the time event, see enum rtca_tevent for more info.
please add -fshort-enums to CFLAGS to store rtca_tevent as only a byte */
void rtca_init(void);
void rtca_set_time(struct rtca_tm *t);
void rtca_get_time(struct rtca_tm *t);

uint8_t rtca_get_event(void);
void rtca_rst_event(void);

void rtca_get_alarm(struct rtca_tm *t, uint8_t *ae_flags);
void rtca_set_alarm(struct rtca_tm *t, const uint8_t ae_flags);
void rtca_enable_alarm();
void rtca_disable_alarm();

#endif
