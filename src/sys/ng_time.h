/*************************************************************************************
 *                               ngRTOS Kernel V2.0.1
 * Copyright (C) 2022 Songtao Liu, 980680431@qq.com.  All Rights Reserved.
 **************************************************************************************
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * THE ABOVE COPYRIGHT NOTICE AND THIS PERMISSION NOTICE SHALL BE INCLUDED IN ALL
 * COPIES OR SUBSTANTIAL PORTIONS OF THE SOFTWARE. WHAT'S MORE, A DECLARATION OF 
 * NGRTOS MUST BE DISPLAYED IN THE FINAL SOFTWARE OR PRODUCT RELEASE. NGRTOS HAS 
 * NOT ANY LIMITATION OF CONTRIBUTIONS TO IT, WITHOUT ANY LIMITATION OF CODING STYLE, 
 * DRIVERS, CORE, APPLICATIONS, LIBRARIES, TOOLS, AND ETC. ANY LICENSE IS PERMITTED 
 * UNDER THE ABOVE LICENSE. THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF 
 * ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO 
 * EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES 
 * OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS 
 * IN THE SOFTWARE.
 *
 *************************************************************************************
 *                              https://www.ngRTOS.org
 *                              https://github.com/ngRTOS
 **************************************************************************************
 */

#ifndef __ngRTOS_DEFS_H__
#define __ngRTOS_DEFS_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef int64_t   time64_t;
typedef uint64_t timeu64_t;

/*
 * Similar to the struct tm in userspace <time.h>, but it needs to be here so
 * that the kernel source is self contained.
 */
struct ng_rtc_time {
  /*
   * the number of seconds after the minute, normally in the range
   * 0 to 59, but can be up to 60 to allow for leap seconds
   */
  uint32_t tm_sec:6;
  /* Millisecond in second (0-999) */
  uint32_t millisecond:10;      
  /* the number of minutes after the hour, in the range 0 to 59*/
  uint32_t tm_min:6;
  /* the number of hours past midnight, in the range 0 to 23 */
  uint32_t tm_hour:5;
  /* the day of the month, in the range 1 to 31 */
  uint32_t tm_mday:5;
  
  /* the number of months since January, in the range 0 to 11 */
  uint32_t tm_mon:4;
  /* the number of years since 1900 */
  uint32_t tm_year:15;
  /* the number of days since Sunday, in the range 0 to 6 */
  uint32_t tm_wday:3;
  /* the number of days since January 1, in the range 0 to 365 */
  uint32_t tm_yday:9;
  uint32_t tm_isdst:1;

  int32_t tm_offset;
};
typedef struct ng_rtc_time ng_rtc_time_s;

/*
 * Structure to hold DST starting and ending information
 */
struct summer_cfg {
  uint32_t mday:5;		/* Day of month, 0: last Sunday in month */
  uint32_t wday:3;		/* Day of week, 0: use day of month*/
  uint32_t month:4;	  /* Month of year */
  uint32_t hours:5;   /* Time (hours) */
  uint32_t minutes:6; /* Time (minutes) */
  uint32_t week:7;		/* Week of month (-1 if last) */
  uint8_t  from_year;	/* Year start */
  uint8_t  to_year;	  /* Year end  */
  int16_t  offset;		/* Time offset in minutes */
};
typedef struct summer_cfg ng_summer_cfg_s;
 
struct ng_timeval{
  ng_time_t tv_sec;
  ng_time_t tv_nsec;
};
typedef struct ng_timeval ng_timeval_s;

struct ng_timespec{
  ng_time_t tv_sec;
  ng_time_t tv_nsec;
};
typedef struct ng_timespec ng_timespec_s;

void ng_getnanotime(ng_timespec_s *sp);

/*
 * The system time is kept as a 64 bit value, in seconds and fractional
 * seconds since 00:00 UTC 1 Jan 1900.  This is the same format as NTP
 * uses.
 */
struct clock_epoch_ {
  union {
    u_long Xl_ui;
    long Xl_i;
  } Ul_i;        /* Integer part */
  union {
    u_long Xl_uf;
    long Xl_f;
  } Ul_f;        /* Fraction part */
};
typedef struct clock_epoch_ ng_clock_epoch_s;

#define epoch_secs Ul_i.Xl_ui   /* integral seconds */
#define epoch_frac Ul_f.Xl_uf   /* fractional part  */

#define NTP_RESOLUTION 0x80000000 /* Frequency of NTP clock (in Hz) */
#define PIPS_PER_USEC (NTP_RESOLUTION/500000) /* 2^-32 secs per usec */
#define USEC_PER_SEC (1000000)		/* Microseconds per second */
#define NTP_SHIFT_FACTOR 1	/* Shift count to account for resolution */

/*
 * Potential clock sources
 */
enum clock_source_ {
  CLOCK_SOURCE_NTP,       /* Network Time Protocol */
  CLOCK_SOURCE_SNTP,      /* Simple Network Time Protocol */
  CLOCK_SOURCE_MANUAL,    /* Manual configuration */
  CLOCK_SOURCE_CALENDAR,  /* Hardware calendar */
  CLOCK_SOURCE_GNSS,      /* From GPS */
  CLOCK_SOURCE_NONE,      /* Unsynchronized */
};
typedef enum clock_source_ ng_clock_source_e;

ng_bool_t 
ng_set_zone_by_name(
  char *name, 
  int len, 
  ng_rtc_time_s *tm, 
  int *ret_tz_offtime
);

void 
clock_set_timezone(
  int tz, 
  ng_string_s *tz_name
);

void 
clock_rtc_to_epoch(
  ng_rtc_time_s *tv, 
  ng_clock_epoch_s *ntp
);

static inline ng_bool_t IS_LEAP(int y) 
{
  return ((((y) & 0x3) == 0 && ((y) % 100) != 0) || ((y) % 400) == 0);
}

#if defined(__ICCARM__) || defined(__CC_ARM) || defined(__GNUC__)
void PreSleepProcessing(uint32_t ulExpectedIdleTime);
void PostSleepProcessing(uint32_t ulExpectedIdleTime);
#endif /* defined(__ICCARM__) || defined(__CC_ARM) || defined(__GNUC__) */

/* 
  The configPRE_SLEEP_PROCESSING() and configPOST_SLEEP_PROCESSING() macros
  allow the application writer to add additional code before and after the 
  MCU is placed into the low power state respectively. 
 */
#if configUSE_TICKLESS_IDLE == 1
#define configPRE_SLEEP_PROCESSING(__x__) \
  do {                         \
    __x__ = 0;                 \
    PreSleepProcessing(__x__); \
  }while(0)
#define configPOST_SLEEP_PROCESSING  \
  PostSleepProcessing
#endif /* configUSE_TICKLESS_IDLE == 1 */

ng_result_e __ng_init_systime(void);
ng_time_t __ng_sys_gettime(void);

#ifdef __cplusplus
}
#endif

#endif

