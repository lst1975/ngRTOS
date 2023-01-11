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
 *************************************************************************************
 */
#include "ng_channel.h"

#define MICROS_PER_SEC  1000
#define NICROS_PER_SEC  1000000

const char *const month_name[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", 
          "Aug", "Sep", "Oct", "Nov", "Dec"};
const char *const day_name[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

const char *const long_month_name[] = {"January", "February", "March", "April",
         "May", "June", "July", "August", "September",
         "October", "November", "December"};
const char *const long_day_name[] = {"Sunday", "Monday", "Tuesday", "Wednesday",
             "Thursday", "Friday", "Saturday"};

static const uint32_t __ng_YEARS[] = {
           0U,  31536000U,  63072000U,  94694400U, 126230400U, 157766400U, 
   189302400U, 220924800U, 252460800U, 283996800U, 315532800U, 347155200U, 
   378691200U, 410227200U, 441763200U, 473385600U, 504921600U, 536457600U, 
   567993600U, 599616000U, 631152000U, 662688000U, 694224000U, 725846400U,
   757382400U, 788918400U, 820454400U, 852076800U, 883612800U, 915148800U, 
   946684800U, 978307200U,1009843200U,1041379200U,1072915200U,1104537600U,
  1136073600U,1167609600U,1199145600U,1230768000U,1262304000U,1293840000U,
  1325376000U,1356998400U,1388534400U,1420070400U,1451606400U,1483228800U,
  1514764800U,1546300800U,1577836800U,1609459200U,1640995200U,1672531200U,
  1704067200U,1735689600U,1767225600U,1798761600U,1830297600U,1861920000U,
  1893456000U,1924992000U,1956528000U,1988150400U,2019686400U,2051222400U,
  2082758400U,2114380800U,2145916800U,2177452800U,2208988800U,2240611200U,
  2272147200U,2303683200U,2335219200U,2366841600U,2398377600U,2429913600U,
  2461449600U,2493072000U,2524608000U,2556144000U,2587680000U,2619302400U,
  2650838400U,2682374400U,2713910400U,2745532800U,2777068800U,2808604800U,
  2840140800U,2871763200U,2903299200U,2934835200U,2966371200U,2997993600U,
  3029529600U,3061065600U,3092601600U,3124224000U,3155760000U,3187296000U,
  3218832000U,3250454400U,3281990400U,3313526400U,3345062400U,3376684800U,
  3408220800U,3439756800U,3471292800U,3502915200U,3534451200U,3565987200U,
  3597523200U,3629145600U,3660681600U,3692217600U,3723753600U,3755376000U,
  3786912000U,3818448000U,3849984000U,3881606400U,3913142400U,3944678400U,
  3976214400U,4007836800U,
};

static const uint32_t __ng_MONTHS[] = {
  0U, 0U, 2678400U,  5097600U,  7776000U, 10368000U, 13046400U, 15638400U, 
       18316800U, 20995200U, 23587200U, 26265600U, 28857600U, 31536000};
static const uint32_t __ng_MONTHS_leap[] = {
  0U, 0U, 2678400U, 5184000U, 7862400U, 10454400U, 13132800U, 15724800U, 
  18403200U, 21081600U, 23673600U, 26352000U, 28944000U, 31622400};
static const uint32_t __ng_DAYS[] = {
  0U, 0U,    86400U,  172800U,  259200U,  345600U,  432000U,  518400U,  
  604800U,  691200U,  777600U,  864000U,  950400U, 1036800U, 1123200U, 
 1209600U, 1296000U, 1382400U, 1468800U, 1555200U, 1641600U, 1728000U, 
 1814400U, 1900800U, 1987200U, 2073600U, 2160000U, 2246400U, 2332800U, 
 2419200U, 2505600U, 2592000U, 2678400};
static const uint32_t __ng_HOURS[] = {
  0U, 
   3600U,  7200U, 10800U, 14400U, 18000U, 21600U, 25200U, 28800U, 32400U, 
  36000U, 39600U, 43200U, 46800U, 50400U, 54000U, 57600U, 61200U, 64800U, 
  68400U, 72000U, 75600U, 79200U, 82800U, 86400};
static const uint32_t __ng_MINUTES[] = {
  0U, 60U, 120U, 180U, 240U, 300U, 360U, 420U, 480U, 540U, 600U, 660U, 720U, 
  780U, 840U, 900U, 960U, 1020U, 1080U, 1140U, 1200U, 1260U, 1320U, 1380U, 
  1440U, 1500U, 1560U, 1620U, 1680U, 1740U, 1800U, 1860U, 1920U, 1980U, 
  2040U, 2100U, 2160U, 2220U, 2280U, 2340U, 2400U, 2460U, 2520U, 2580U, 
  2640U, 2700U, 2760U, 2820U, 2880U, 2940U, 3000U, 3060U, 3120U, 3180U, 
  3240U, 3300U, 3360U, 3420U, 3480U, 3540U, 3600};

/* Unix time is total number of seconds measured since  01-Jan-1970 00:00:00.  */
ng_result_e
ng_tm2unix_time(ng_rtc_time_s *tm, ng_time_t *t) 
{
  if(tm->tm_year < 1970 
      || tm->tm_year > 2100 
      || tm->tm_mon  > 13 
      || tm->tm_mday > 32 
      || tm->tm_hour > 24 
      || tm->tm_min  > 60 
      || tm->tm_sec  > 60)
    return NG_result_inval;

  *t =  __ng_YEARS[tm->tm_year - 1970] 
        + (IS_LEAP(tm->tm_year) ? 
              __ng_MONTHS_leap[tm->tm_mon] : 
              __ng_MONTHS[tm->tm_mon]) 
        + __ng_DAYS[tm->tm_mday] 
        + __ng_HOURS[tm->tm_hour] 
        + __ng_MINUTES[tm->tm_min] 
        + tm->tm_sec;
  return NG_result_ok;              
}

/*
 * Days in each month.  30 days hath September...
 */
#define  JAN  31
#define  FEB  28
#define  FEBLEAP  29
#define  MAR  31
#define  APR  30
#define  MAY  31
#define  JUN  30
#define  JUL  31
#define  AUG  31
#define  SEP  30
#define  OCT  31
#define  NOV  30
#define  DEC  31
/*
 * Table of day offsets within year (March-based)
 */
static const u_long month_offset[] = {0,
           MAR, 
           MAR+APR, 
           MAR+APR+MAY, 
           MAR+APR+MAY+JUN,
           MAR+APR+MAY+JUN+JUL,
           MAR+APR+MAY+JUN+JUL+AUG,
           MAR+APR+MAY+JUN+JUL+AUG+SEP,
           MAR+APR+MAY+JUN+JUL+AUG+SEP+OCT,
           MAR+APR+MAY+JUN+JUL+AUG+SEP+OCT+NOV,
           MAR+APR+MAY+JUN+JUL+AUG+SEP+OCT+NOV+DEC,
           MAR+APR+MAY+JUN+JUL+AUG+SEP+OCT+NOV+DEC+JAN
           };

/*
 * We deal in a 4 year cycle starting at March 1, 1900.  We assume
 * we will only want to deal with dates since then, and not to exceed
 * the rollover day in 2036.
 */
#define MILLISECSPERSEC (1000U)      /* milliseconds per second */
#define SECSPERMIN      (60U)      /* seconds per minute */
#define MINSPERHR       (60U)      /* minutes per hour */
#define HRSPERDAY       (24U)      /* hours per day */
#define DAYSPERWEEK     (7U)      /* days per week */
#define DAYSPERYEAR     (365U)      /* days per year */
#define DAYSPERLEAPYEAR (366U)      /* days per leap year */
#define YEARSPERCENTURY (100U)      /* years per century */
#define YEARSPERCYCLE   (4)
#define CYCLE23         (23U)
#define JAN11900DOW     (1U)  /* Jan 1, 1900 was a Monday */

#define __SECSPERHR       (SECSPERMIN * MINSPERHR)
#define __SECSPERDAY      (SECSPERMIN * MINSPERHR * HRSPERDAY)
#define __MILLISECSPERDAY (__SECSPERDAY * MILLISECSPERSEC)
#define __SECSPERYEAR     (DAYSPERYEAR * __SECSPERDAY)  /* regular year */
#define __SECSPERLEAPYEAR (DAYSPERLEAPYEAR * __SECSPERDAY)  /* leap year */

#define __DAYSPERCYCLE    (DAYSPERYEAR+DAYSPERYEAR+DAYSPERYEAR+DAYSPERLEAPYEAR)  /* 3 normal years plus leap */
#define __SECSPERCYCLE    (__DAYSPERCYCLE*__SECSPERDAY)

/*
 * Some important dates.
 */
#define __STARTCYCLE23    (CYCLE23*__SECSPERCYCLE)

/*
 * The length of January + February in leap and non-leap years.
 */
#define __JANFEBNOLEAP ((JAN + FEB) * __SECSPERDAY)
#define __JANFEBLEAP   ((JAN + FEBLEAP) * __SECSPERDAY)

#define __MAR1900 ((JAN + FEB) * __SECSPERDAY)        /* no leap year in 1900 */
#define __MAR1968 (__MAR1900 + (17 * __SECSPERCYCLE)) /* March 1, 1968 */
#define __MAR1970 (__MAR1968 + (2 * __SECSPERYEAR))   /* March 1, 1970 */
#define __JAN1970 (__MAR1970 - __JANFEBNOLEAP)        /* Jan 1, 1970 */
#define __MAR1992 (__STARTCYCLE23 + __MAR1900)
#define __MAR1993 (__MAR1992 + __SECSPERYEAR)         /* March 1, 1993 */
#define __JAN1993 (__MAR1993 - __JANFEBNOLEAP)        /* Jan 1, 1993 */
#define __18JAN2018 (1516233600U)        /* Jan 1, 1993 */

static const u_long SECSPERHR       = __SECSPERHR;
static const u_long SECSPERDAY      = __SECSPERDAY;
static const u_long MILLISECSPERDAY = __MILLISECSPERDAY;
static const u_long SECSPERYEAR     = __SECSPERYEAR;  /* regular year */
static const u_long SECSPERLEAPYEAR = __SECSPERLEAPYEAR;  /* leap year */

static const u_long DAYSPERCYCLE    = __DAYSPERCYCLE;  /* 3 normal years plus leap */
static const u_long SECSPERCYCLE    = __SECSPERCYCLE;

/*
 * Some important dates.
 */
static const u_long STARTCYCLE23    = __STARTCYCLE23;

/*
 * The length of January + February in leap and non-leap years.
 */
static const u_long JANFEBNOLEAP = __JANFEBNOLEAP;
static const u_long JANFEBLEAP   = __JANFEBLEAP;

static const u_long MAR1900 = __MAR1900; /* no leap year in 1900 */
static const u_long MAR1968 = __MAR1968; /* March 1, 1968 */
static const u_long MAR1970 = __MAR1970; /* March 1, 1970 */
static const u_long JAN1970 = __JAN1970; /* Jan 1, 1970 */
static const u_long MAR1992 = __MAR1992; /* March 1, 1992 */
static const u_long MAR1993 = __MAR1993; /* March 1, 1993 */
static const u_long JAN1993 = __JAN1993; /* Jan 1, 1993 */

static ng_channel_s *__rtc_ch = NULL;
typedef u_long (*ng_get_resid_f)(void);

/*
 * clock_epoch_to_rtc - convert NTP timestamp to a rtc
 *
 * Time less than January 1, 1970 is treated as if it wrapped past the maximum
 * supported by NTP.
 */
void 
clock_epoch_to_rtc(ng_clock_epoch_s *ntp, 
  ng_rtc_time_s *tv, long tz_offset)
{
  u_long cyclenum;
  u_long sec_in_cycle;
  u_long year_in_cycle;
  u_long sec_in_year;
  u_long day_in_year;
  u_long sec_in_day;
  u_long sec_in_hour;
  u_long month_in_year;
  u_long day_in_month;
  u_long day_in_epoch;
  u_llong secnum;
  u_llong unwrapped_secs;

  tv->tm_offset = tz_offset;

  unwrapped_secs = (ntp->epoch_secs < JAN1970) ? 
    ntp->epoch_secs + (ng_ULONG_MAX + 1ULL) : 
    ntp->epoch_secs;

  /* First figure out which cycle we're in. */
  secnum = unwrapped_secs + tz_offset - MAR1900;
  cyclenum = secnum / SECSPERCYCLE;
  sec_in_cycle = secnum % SECSPERCYCLE;

  /* Now what day since the beginning of NTP time, and 
     * the day of week. 
     */
  day_in_epoch = (unwrapped_secs + tz_offset) / SECSPERDAY;
  tv->tm_wday = (day_in_epoch + JAN11900DOW) % 7;

  /* Now the year in cycle. */
  year_in_cycle = sec_in_cycle / SECSPERYEAR;
  sec_in_year = sec_in_cycle % SECSPERYEAR;
  if (year_in_cycle == YEARSPERCYCLE) 
  {  
    /* Feb 29? */
    year_in_cycle--;
    sec_in_year += SECSPERYEAR;
  }

  /* Now the day in month and month in year. */
  day_in_year = sec_in_year / SECSPERDAY;
  sec_in_day = sec_in_year % SECSPERDAY;
  for (month_in_year = 1; month_in_year < 12; month_in_year++) 
  {
    if (day_in_year < month_offset[month_in_year])
      break;
  }
  month_in_year--;
  day_in_month = day_in_year - month_offset[month_in_year];

  /* Now we can figure out the calendar year. */
  tv->tm_year = 1900 + (YEARSPERCYCLE * cyclenum) + 
    year_in_cycle + (month_in_year >= 10);

  /* And the calendar month... */

  tv->tm_mon = ((month_in_year + 2) % 12) + 1;

  /* And the day... */

  tv->tm_mday = day_in_month + 1;

  /* And the hour... */

  tv->tm_hour = sec_in_day / SECSPERHR;
  sec_in_hour = sec_in_day % SECSPERHR;

  /* And the minute... */

  tv->tm_min = sec_in_hour / SECSPERMIN;

  /* And the second... */

  tv->tm_sec = sec_in_hour % SECSPERMIN;

  /* And the millisecond... */

  tv->millisecond = ntp->epoch_frac / (((1<<29) / 125) + 1);

  /*
   * Calculate the Julian date (day in year).  Note that this is mildly
   * nontrivial because our calculations are March-based, and we need
   * to start the year in January instead (how inconvenient).
   */
  if (month_in_year > 9) 
  {    
    /* Jan or Feb */
    tv->tm_yday = day_in_year - (DAYSPERYEAR - JAN - FEB) + 1;
  } 
  else 
  {
    tv->tm_yday = day_in_year + JAN + FEB + 1;
    if (year_in_cycle == 0) 
    {    /* Leap year? */
      tv->tm_yday++;
    }
  }
}

/*
 * clock_rtc_to_epoch - convert a rtc to an NTP timestamp
 */
void 
clock_rtc_to_epoch(ng_rtc_time_s *tv, ng_clock_epoch_s *ntp)
{
  ng_time_t tm;

  ntp->epoch_frac = tv->millisecond * (((1<<29) / 125) + 1);
  if (ng_tm2unix_time(tv, &tm) == NG_result_ok)
  {
    ntp->epoch_secs = tm - tv->tm_offset;
  }
  else
  {
    u_long local_month;
    u_long local_year;
    u_long day_in_year;
    u_long cyclenum;
    u_long year_in_cycle;
    u_long day_in_cycle;

    local_month     = (tv->tm_mon + 9) % 12;
    local_year      = tv->tm_year - (tv->tm_mon < 3);

    day_in_year     = tv->tm_mday - 1 + month_offset[local_month];
    cyclenum        = (local_year - 1900) / YEARSPERCYCLE;
    year_in_cycle   = (local_year - 1900) % YEARSPERCYCLE;
    day_in_cycle    = year_in_cycle * DAYSPERYEAR + day_in_year;
    ntp->epoch_secs = MAR1900 + (cyclenum * SECSPERCYCLE) + 
      (day_in_cycle * SECSPERDAY) + (tv->tm_hour * SECSPERHR) + 
      (tv->tm_min * SECSPERMIN) + tv->tm_sec - tv->tm_offset;
  }
}

/*
  * clock_rtc_to_unix_time
  *
  * Converts a rtc structure to a Unix-style timestamp
  *
  * Unix time is total number of seconds measured since  01-Jan-1970 00:00:00.  
  *
  */
u_long 
clock_rtc_to_unix_time (ng_rtc_time_s *tv)
{
  ng_clock_epoch_s epoch;

  /* Convert to an epoch, and then to Unix time. */

  clock_rtc_to_epoch(tv, &epoch);
  return (clock_epoch_to_unix_time(&epoch));
}

/*
 * unix_time_to_epoch
 *
 * Convert a unix-style timestamp (seconds since 1 Jan 1970) into a
 * clock_epoch structure.
 */
void 
unix_time_to_epoch(u_long unix_time, ng_clock_epoch_s *epoch_ptr)
{
  epoch_ptr->epoch_secs = unix_time + JAN1970;
  epoch_ptr->epoch_frac = 0;
}

/*
 * clock_epoch_to_unix_time
 *
 * Convert an epoch to a Unix-style timestamp (seconds since 0000 UTC
 * 1 Jan 1970).
 *
 * If the current time is previous to 1970, this routine will return
 * unpredictable results.
 */
u_long 
clock_epoch_to_unix_time (ng_clock_epoch_s *epoch_ptr)
{
  return (epoch_ptr->epoch_secs - JAN1970);
}

static ng_time_t        ng_calib_seconds  = 0;
static ng_clock_epoch_s ng_system_clock_epoch = { .epoch_secs=__18JAN2018, .epoch_frac=0 };

/*
 * Low-level System Clock support
 *
 * The system clock fits the "Unix Clock Model."
 *
 * The current time is kept at a resolution greater than the resolution
 * of the hardware clock, and incremented by some value whenever the hardware
 * clock ticks.  To provide time adjustments, two methods are available--for
 * large adjustments, the time is simply set to the desired time.  For small
 * adjustments, the clock is slewed in such a way that the time is always
 * increasing monotonically--the amount added to the clock for each hardware
 * tick is reduced or increased until the desired adjustment has been
 * completed.
 *
 * The time is stored as a 64 bit binary fraction, with the "decimal" point
 * in the middle (or, if you like, think of it as a 64-bit integer in units
 * of 2^-32), specifying the number of seconds since 0000 UTC, January 1,
 * 1900, for the convenience of the NTP protocol.
 *
 * At each tick of the hardware clock, we add 2^32/Hz to the fractional
 * part of the time (and check for rollover).  Note that, due to truncation
 * error, the clock will run slow by a minute amount (68 parts per billion
 * at 1000 Hz, 11 parts per billion at 250 Hz).  This amount is too small
 * to really care about, and in fact is negligible compared to the error
 * of the crystal (on the order of 5000 parts per billion or more).  The
 * NTP protocol will compensate for this anyhow.
 *
 * When getting the current time, we read the current value of the timer
 * clock so that we can interpolate between ticks.  The true value for
 * each timer tick would be the basic tick increment divided by the
 * timer frequency, so to calculate the offset accurately we would calculate
 *   (increment / timer_freq) * timer_value
 * This is inconvenient, however, since we don't really want to do division
 * at all.  Another approach would be to precalculate (increment / timer_freq)
 * and then multiply by timer_value, but this leads to truncation errors.
 * Instead, we calculate:
 *    ((increment / timer_freq) * K) * timer_value / K
 * where K is a power of two;  then we can precalculate 
 * ((increment / timer_freq) * K) and then do a multiply and a shift in real
 * time and get very high accuracy for cheap.  The value of K is chosen to be
 * less than Hz (but as large as possible otherwise) so we get the best
 * accuracy but don't get any arithmetic overflows.
 *
 * This code relies on a platform-dependent routine that returns information
 * about the hardware timer, including a pointer to a routine to read the
 * timer residual.  We assume that the residual counts downward from
 * (precision - 1) to 0.
 */

#define SLEW_RATE 2000    /* Slew rate (500 ppm) */

static u_long leap_second_epoch;    /* Time of leap second jump */
static int leap_second_offset;      /* Value to adjust at leap second */
static int adjust=0;                /* Outstanding clock adjustment */
static int max_delta;                /* Maximum slew per clock tick */
static int delta=0;                  /* Current slew per clock tick */
static u_long precision;            /* Clock precision (2**x Hz) */
static u_long prescalar_factor;      /* increment per timer tick * 1<<log2hz */
static u_long log2hz;                /* log2 of hz, truncated */
static u_long hz;                    /* Estimated clock tick frequency */
static u_long increment;            /* Current SW increment per HW tick */
static u_long basic_increment;      /* Initial SW increment per HW tick */
static u_long divisor;              /* timer clocks per tick */
static u_long freq_numerator;        /* Numerator of clock frequency */
static u_long freq_denominator;      /* Denominator of clock frequency */
static u_long timer_count;          /* The count-time count for the timer */
static u_long timer_freq;            /* The frequency of the timer */

static ng_get_resid_f get_residual; /* Routine to get timer residual */
static ng_clock_source_e clock_time_source = CLOCK_SOURCE_NONE;
static ngTickTypeT ns_one_tick;

/*
 * Set the software clock increment per hardware clock tick.  This is
 * broken out so that we can adjust this increment separately if need
 * be (such as to compensate more cleanly for a way-off crystal).
 */
void set_clock_increment (u_long incr)
{
  u_long freq;
  delta = 0;
  adjust = 0;
  increment = incr;
  
  /*
   * Recalculate the basic clock frequency.  Make sure we round
   * rather than truncate.
   */
  freq = hz = (NTP_RESOLUTION + (incr >> (NTP_SHIFT_FACTOR + 1))) / 
    (incr >> NTP_SHIFT_FACTOR);
  max_delta = incr / SLEW_RATE;
    
  /* Calculate the log2 of the frequency. */

  freq &= 0x7fffffff;    /* Ensure that the loop completes */
  for (log2hz = 0; freq; log2hz++)
    freq = freq >> 1;
  log2hz--;
  prescalar_factor = incr * (1 << log2hz) / divisor;
}

/*
 * clock_set_leap_second
 *
 * Set the leap second parameters.
 */
void clock_set_leap_second(ng_time_t epoch, int offset)
{
  leap_second_epoch = epoch;
  leap_second_offset = offset;
}

/*
 * clock_get_leap_second
 *
 * Returns the leap second parameters
 */
ng_time_t clock_get_leap_second (int *offset)
{
  *offset = leap_second_offset;
  return(leap_second_epoch);
}

/*
 * Call back to a clock freqency callback routine.
 *
 * Called when a module in another subsystem wants its clock frequency
 * set callback routine explicitly invoked.  This is typically done
 * when the process initializes since it probably missed the original setting
 * of the hardware clock frequency.
 */
void invoke_clock_set (void (*callback) (u_long, u_long))
{
  (*callback)(freq_numerator, freq_denominator);
}

void ng_getnanotime(ng_timespec_s *sp)
{
  ng_clock_epoch_s epock;
  clock_get_time(&epock);
  sp->tv_sec  = epock.epoch_secs;
  sp->tv_nsec = epock.epock_frac;
}

/*
 * Set the hardware clock frequency.  We recalculate all of our interesting
 * fudge factors.
 *
 * To all those that came before , numerator should be the cpu clock
 * frequency in HZ eg   40mhz is 40,000,000 .
 *
 * Inputs -
 * ========
 * numerator    : cpu clock Frequency .
 * denominator  : ticks per 4 milli-seconds  (as close as possible).
 *
 * IOS uses 4-ms ticks .
 * These values will be processor dependant .
 * This function is used by NTP only .
 *
 * Output -
 * =======
 *  First order approximation to allow clock adjust in ntp_loopfilter.c
 *  to lock on to the peer's timebase and adjust as required .
 *
 * Note -:
 * There are currently a number of fudged definitions in get_timer_parameters
 * which define frequency to be  Mhz 0,000,250 (not typo) and the denominator
 * to be '1' . Should these processor types exhibit problems then the
 * true Frequency and clock tick values should be provided as input to this
 * routine.
 */

static void 
set_hw_clock_freq (u_long numerator, u_long denominator, u_long div)
{
  u_long incr;
  u_llong tmp = 0;
  freq_denominator = denominator;   
  freq_numerator   = numerator;     
  divisor          = div;           
  tmp  = (u_llong)(NTP_RESOLUTION * (u_llong)denominator);
  tmp  = (tmp/numerator) << NTP_SHIFT_FACTOR;
  incr = (u_long)tmp;
  basic_increment = incr;
  set_clock_increment(incr);
}

/*
 * Slew the clock.  We return a nonzero value if the clock was already
 * being adjusted (and ignore the requested adjustment).
 *
 * The requested clock slew is in units of 2^-32 seconds, and the magnitude
 * must be less than 1/2 second (we're using 32 bit words, after all...).
 */
int clock_slew(int slew_value, ng_clock_source_e source)
{
  int return_value = 0;
  
  clock_set_time_source(source);

  if (delta !=0) 
  {
    return_value = 1;
  } 
  else 
  {
    adjust = slew_value;
    if (slew_value >= 0) 
    {
      delta = ng_min_t(max_delta, slew_value, int);
    }
    else 
    {
      delta = ng_max_t(-max_delta, slew_value, int);
    }
  }

  return (return_value);
}

/*
 * Return the current time source.
 */
static ng_clock_source_e current_time_source (void)
{
  return (clock_time_source);
}

/*
 * Set the current time source.
 */
static void clock_set_time_source (ng_clock_source_e time_source)
{
  clock_time_source = time_source;
}

/*
 * Get the exact time.  Read up the platform-dependent residual and
 * interpolate.
 *
 * Note that this routine is called from both interrupt and task threads,
 * so it must remain reentrant.
 *
 * Additionally, when reading the residual, it may roll over, causing the
 * main time to tick.  We avoid this by checking the time before and
 * after reading the timer;  if it has advanced, we do it a second time,
 * which should be OK.
 */
static void clock_get_time_exact (ng_clock_epoch_s *timeptr)
{
  int offset = 0;
  u_long last_frac;

  /* Get the system clock. */

  clock_get_time(timeptr);
  last_frac = timeptr->epoch_frac;

  /* Read the residual. */
  if (get_residual)
    offset = (*get_residual)();

  /* If the timer rolled over, do it again. */

  if (ng_system_clock_epoch.epoch_frac != last_frac) 
  {
    clock_get_time(timeptr);
    last_frac = timeptr->epoch_frac;
    if (get_residual)
      offset = (*get_residual)();
  }

   /* Now scale the offset. */
  timeptr->epoch_frac += (offset * prescalar_factor) >> log2hz;

  /* Finally, adjust for rollover. */
  if (timeptr->epoch_frac < last_frac) 
  {
    timeptr->epoch_secs++;
  }
}

/*
 * clock_get_microsecs
 *
 * Get the current time in terms of microseconds.  This is useless as a time
 * of day, but makes a dandy, fast-moving 32 bit value.
 */
u_long clock_get_microsecs (void)
{
  ng_clock_epoch_s time;

  clock_get_time_exact(&time);
  return ((time.epoch_secs % (ng_UINT_MAX/NICROS_PER_SEC+1)) * NICROS_PER_SEC +
    time.epoch_frac / (ng_UINT_MAX/NICROS_PER_SEC+1));
}

/*
 * clock_set
 *
 * Set the current time.  This is used for gross time steps only.  Reasonable
 * time adjustments should be made by calling clock_slew instead.
 */
static void clock_set (ng_clock_epoch_s *timeptr, ng_clock_source_e source)
{
  delta = 0;
  adjust = 0;
  ng_system_clock_epoch = *timeptr;
  clock_set_time_source(source);
}
void ngrtos_process_epoch(void)
{
  clock_tick();
}

/*
 * Initialize the kernel clock.  We start at March 1, 1993, and throw
 * in the current msclock to help randomize things that use the
 * system clock as a random seed.
 */
static void init_kernel_clock (void)
{
  u_long freq_numerator, freq_denominator, freq_divisor;
  
  /* A reasonable place to start. */
  ng_system_clock_epoch.epoch_secs = __18JAN2018;  
  get_timer_parameters(&freq_numerator, &freq_denominator, &freq_divisor,
     &get_residual, &precision);
  set_hw_clock_freq(freq_numerator, freq_denominator, freq_divisor);
}

/*
 * Initialize the clock system
 */
static void clock_init (void)
{
  clock_set_time_source(CLOCK_SOURCE_NONE);

  /* Initialize the kernel clock */
  init_kernel_clock();
}

static ng_result_e
read_calendar(ng_rtc_time_s *tv)
{
  if (__rtc_ch == NULL)
    return NG_result_nsupport;
  
  tv->tm_isdst  = ngrtos_FALSE;
  tv->tm_neg    = ngrtos_FALSE;
  tv->tm_offset = 0;
  
  if (0 > ng_ioctl(__rtc_ch, NG_DEV_RTC_GET_DATE_TIME, tv))
  {
    return NG_result_failed;
  }
  return NG_result_ok;
}

/*
 * Guts of calendar reading
 */
static ng_result_e do_read_calendar (void)
{
  ng_result_e r;
  ng_clock_source_e s;
  ng_clock_epoch_s ntp;
  ng_rtc_time_s tv;

  r = read_calendar(&tv);
  if (r == NG_result_ok)
  {
    tv.tm_offset   = 0;    /* Calendar runs on UTC */
    clock_rtc_to_epoch(&tv, &ntp);
    s = CLOCK_SOURCE_CALENDAR;
  }
  else
  {
    s = CLOCK_SOURCE_NONE;
  }

  if (ntp.epoch_secs < __18JAN2018)  /* Limit to January 18, 2018 */
      ntp.epoch_secs = __18JAN2018;
  clock_set(&ntp, s);
}

/* clockinit: setup to take the timer 0 interrupt */
static void __ng_clockinit(void)
{
  timer_freq = ____ng_set_sysclock_Freq();
  timer_count = (timer_freq + (ngRTOS_TICK_RATE_HZ>>1)) / ngRTOS_TICK_RATE_HZ;
  ns_one_tick = ng_ticks2ns(1);

  clock_init();
  do_read_calendar();
}

ng_result_e __ng_init_systime(void)
{
  __rtc_ch = ng_open("rtc");
  __ng_clockinit();
  return NG_result_ok;
}

/*
 * Call back to a clock freqency callback routine.
 *
 * Called when a module in another subsystem wants its clock frequency
 * set callback routine explicitly invoked.  This is typically done
 * when the process initializes since it probably missed the original setting
 * of the hardware clock frequency.
 */
void invoke_clock_set (void (*callback) (u_long, u_long))
{
  (*callback)(freq_numerator, freq_denominator);
}

static u_long
timer_residual()
{
  return timer_count - (u_long)____ng_get_systicks();
}

/*
 * get_timer_parameters
 *
 * Get information about the hardware timer (used by the system clock
 * support)
 */
void get_timer_parameters (u_long *freq_numerator, 
                           u_long *freq_denominator,
                           u_long *freq_divisor, 
                           ng_get_resid_f *get_residual,
                           u_long *precision)
{
  *freq_numerator   = timer_freq;
  *freq_denominator = timer_count;
  *freq_divisor     = timer_count;
  *get_residual     = timer_residual;
  *precision        = 6;
}

/*
 * clock_set
 *
 * Set the current time.  This is used for gross time steps only.  Reasonable
 * time adjustments should be made by calling clock_slew instead.
 */
void clock_set(ng_clock_epoch_s *timeptr, ng_clock_source_e source)
{
  delta = 0;
  adjust = 0;
  ng_system_clock_epoch = *timeptr;
  clock_set_time_source(source);
  check_for_timezone(NULL);
}

/*
 * Process a hardware clock tick.  This routine is called at interrupt
 * time!
 */
void clock_tick(void)
{
  u_long local_incr;
  int local_delta;
  int local_adjust;

  local_incr = increment;    /* Get 'em in registers */
  local_delta = delta;
  
#define ADD_UF_AUX(r_i, r_f, a_f) /* (r_i,r_f) += a_f */ \
  {\
      u_long local_frac = (r_f);\
      (r_f) += (a_f);\
      if ((r_f) < local_frac) {\
    (r_i)++;\
      }\
  }

  /*
   * If we are slewing the clock, include the current slew delta when
   * bumping the software clock, and keep track of how much we've slewed
   * thusfar.
   */

  if (local_delta != 0) 
  {
    local_incr += local_delta;
    ADD_UF_AUX(ng_system_clock_epoch.epoch_secs, 
      ng_system_clock_epoch.epoch_frac, local_incr);
    adjust -= local_delta;
    local_adjust = adjust;
    if (local_delta >= 0) {
      if (local_adjust < local_delta)
      delta = local_adjust;
    } else {
        if (local_adjust > local_delta)
      delta = local_adjust;
    }

  } 
  else 
  {
    /* Not slewing.  Do it cheaply. */
    ADD_UF_AUX(ng_system_clock_epoch.epoch_secs, 
      ng_system_clock_epoch.epoch_frac, local_incr);
  }

  /* Look for a leap second. */

  if (leap_second_epoch) 
  {   /* Something's coming */
    if (leap_second_epoch == ng_system_clock_epoch.epoch_secs) 
    { /* It's now! */
      ng_system_clock_epoch.epoch_secs += leap_second_offset; /* Adjust it */
      leap_second_epoch = 0;  /* All done. */
    }
  }
}

/*
 * clock_set_leap_second
 *
 * Set the leap second parameters.
 */
void clock_set_leap_second (u_long epoch, int offset)
{
  leap_second_epoch = epoch;
  leap_second_offset = offset;
}

/*
 * clock_get_leap_second
 *
 * Returns the leap second parameters
 */
u_long clock_get_leap_second (int *offset)
{
  *offset = leap_second_offset;
  return (leap_second_epoch);
}

/*
 * Guts of calendar update
 */
ng_result_e do_calendar_update (void)
{
  ng_rtc_time_s tv;
  ng_clock_epoch_s ntp_time;

  clock_get_time(&ntp_time);
  clock_epoch_to_rtc(&ntp_time, &tv, 0); /* Calendar runs on UTC */

  if (__rtc_ch == NULL)
    goto err0;
  
  if (0 > ng_ioctl(__rtc_ch, NG_DEV_RTC_SET_DATE_TIME, &tv))
  {
    goto err0;
  }
  return NG_result_ok;

err0:
  return NG_result_failed;
}

/* Default time zone information */

#define DEFAULT_TZ_OFFSET 0             /* UTC, of course */
#define DEFAULT_TZ_NAME "UTC"          /* Dave Mills wouldn't have it otherwise. */
#define DEFAULT_DST_OFFSET (SECSPERHR) /* Default DST offset in seconds */

static ng_clock_source_e clock_time_source;  /* Source of last time update */

struct timezone{
  long offset:30;
  long alloced:2;
  ng_string_s name;
};
typedef struct timezone timezone_s;

static timezone_s sys_tz = { 
  .offset  = DEFAULT_TZ_OFFSET,
  .alloced = ngrtos_FALSE,
  .name    = __DFS(DEFAULT_TZ_NAME),
};
static timezone_s standard_tz = { 
  .offset = DEFAULT_TZ_OFFSET,
  .alloced = ngrtos_FALSE,
  .name   = __DFS(DEFAULT_TZ_NAME)
};

struct summer_rule{
  ng_clock_epoch_s start;       /* Start of next or current summer time */
  ng_clock_epoch_s end;         /* End of next or current summer time */
  long offset;                  /* Offset for summer time */
  ng_list_s link;
};
typedef struct summer_rule summer_rule_s;
static ng_list_s summer_time_head = LIST_HEAD_INIT(summer_time_head);
ng_bool_t summer_time_enabled = ngrtos_FALSE;  /* True if summer time is enabled */

/*
 * Return the current timezone offset in integer seconds.
 */
long clock_timezone_offset (void)
{
  return sys_tz.offset;
}

/*
 * clock_timezone_name
 *
 * Returns a pointer to the current local timezone name.
 */
ng_string_s *clock_timezone_name (void)
{
  return &sys_tz.name;
}

/** @brief returns the day of the week for the given year/month/day
 *
 *  @param y year: 1 <= y <= 255 (2001 - 2255)
 *  @param m month: 1 <= m <= 12
 *  @param d day: 1 <= d <= 31
 *  @return day of week (Monday = 1, Sunday = 7)
 */
static const uint8_t dayofweek_table[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
static uint8_t __dayofweek(uint8_t y, uint8_t m, uint8_t d) 
{
  y -= m < 3;
  d = ((y + (y>>2) - (y/100) + 5 + dayofweek_table[m-1] + d) % 7);
  if (d == 0) { return 7; } else { return d; }
}

/** @brief returns days needed to get from the "current" day to the desired day of the week.
 *
 *  @param dayofweek_of_cur the "current" day of the week: 1 <= dayofweek_of_cur <= 7 (Monday = 1, Sunday = 7)
 *  @param dayofweek the desired day of the week: 1 <= dayofweek <= 7 (Monday = 1, Sunday = 7)
 *  @return number of days
 */
static uint8_t 
__next_dayofweek_offset(uint8_t dayofweek_of_cur, uint8_t dayofweek) 
{
  return (7 + dayofweek - dayofweek_of_cur) % 7;
}

static const uint8_t days_in_month_nonleap[13] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
static uint8_t __days_in_month(uint8_t y, uint8_t m) 
{
  if (m == 2 && __is_leap_year(y)) {
    return days_in_month_nonleap[m] + 1;
  } else {
    return days_in_month_nonleap[m];
  }
}

/** @brief unpack rule
 *
 *  @param rule_in pointer to packed rule
 *  @param cur_year year: 1 <= y <= 255 (2001 - 2255)
 *  @param rule_out pointer for the output unpacked rule
 *  @return void
 */
static void 
__unpack_rule(const ng_summer_cfg_s *rule_in, summer_rule_s *rule_out,
  ng_bool_t is_start_year, int tz_offset) 
{
  ng_rtc_time_s tv;

  tv.tm_year = is_start_year ? rule_in->from_year : rule_in->to_year;
  tv.tm_mon  = rule_in->month;

  if (rule_in->wday == 0) 
  { 
    /* format is DOM e.g. 22 */
    tv.tm_mday = rule_in->mday;
  } 
  else if (rule_in->mday == 0) 
  { 
    uint8_t first_dayofweek;
    uint8_t dayofweek_of_first_dayofmonth;
    /* format is lastDOW e.g. lastSun */
    dayofweek_of_first_dayofmonth = __dayofweek(tv.tm_year, rule_in->month, 1);
    first_dayofweek = __next_dayofweek_offset(dayofweek_of_first_dayofmonth, 
                          rule_in->on_dayofweek);
    tv.tm_mday = 1 + (7*3) + first_dayofweek;
    if (tv.tm_mday + 7 <= __days_in_month(tv.tm_year, rule_in->month)) 
    {
      tv.tm_mday += 7;
    }
  } 
  else 
  { 
    /* format is DOW >= DOM e.g. Sun>=22 */
    uint8_t dayofweek_of_dayofmonth;
    dayofweek_of_dayofmonth = __dayofweek(tv.tm_year, 
        rule_in->month, rule_in->mday);
    tv.tm_mday = rule_in->on_dayofmonth + 
        __next_dayofweek_offset(dayofweek_of_dayofmonth, 
                rule_in->wday);
  }

  tv.tm_hour   = rule_in->hours;
  tv.tm_min    = rule_in->minutes;
  tv.tm_offset = tz_offset;
  clock_rtc_to_epoch(&tv, is_start_year?&rule_out->start:&rule_out->end);
  rule_out->offset = rule_in->offset * 60;
}

ng_result_e 
ng_unpack_summer_rule(const ng_summer_cfg_s *rule_in) 
{
  summer_rule_s *rule_out;
  int tz_offset = standard_tz;

  rule_out = ng_malloc(sizeof(*rule_out));
  if (rule_out == NULL)
  {
    NGRTOS_ERROR("Failed to malloc summer_rule_s.\n");
    return NG_result_nmem;
  }
  __unpack_rule(rule_in, rule_out, ngrtos_TRUE, rule_out);
  __unpack_rule(rule_in, rule_out, ngrtos_FALSE, rule_out);
  list_add_tail(&rule_out->link, &summer_time_head);
  return NG_result_ok;
}

void 
check_for_summer_time (ng_clock_epoch_s *curtime)
{
  u_long cur_secs;
  
  cur_secs = curtime->epoch_secs + standard_tz;
  if (summer_time_enabled)
  {
    summer_rule_s *rule;
    static ng_list_s summer_time_head = LIST_HEAD_INIT(summer_time_head);
    
    /* If we're after the end of summer time, recalculate the epochs. */
    cur_secs = curtime->epoch_secs + standard_tz;
    list_for_each_entry(rule,summer_rule_s,&summer_time_head,link)
    {
      if (ngrtos_difftime_l(cur_secs, rule->start.epoch_secs) >= 0 
        && ngrtos_difftime_l(cur_secs, rule->end.epoch_secs) < 0)
      {
        cur_secs += rule->offset;
        break;
      }
    }
  }
  
  curtime->epoch_secs = cur_secs;
}

/*
 * Get timezone information given a time stamp.  We interpret the summer
 * time rules as necessary.
 *
 * Time less than January 1, 1970 is treated as if it wrapped past the maximum
 * supported by NTP.
 */
ng_bool_t 
clock_time_is_in_summer(ng_clock_epoch_s *ntp_time)
{
  ng_clock_epoch_s summer_start, summer_end;
  u_llong ntp_secs;
  u_llong end_secs;
  u_llong start_secs;

  /* If no summer time, we're outta here. */
  if (!summer_time_enabled)
    return ngrtos_FALSE;

  /* Get the summer time epochs. */
  do_summer_time_epochs(ntp_time, &summer_start, &summer_end);

  /* If it's summer time, use the summer time timezone. */
  ntp_secs = (ntp_time->epoch_secs < JAN1970) ?
      ntp_time->epoch_secs + (ULONG_MAX + 1ULL) : ntp_time->epoch_secs;
  end_secs = (summer_end.epoch_secs < JAN1970) ?
      summer_end.epoch_secs + (ULONG_MAX + 1ULL) : summer_end.epoch_secs;
  start_secs = (summer_start.epoch_secs < JAN1970) ?
      summer_start.epoch_secs + (ULONG_MAX + 1ULL) : summer_start.epoch_secs;
  if (ntp_secs > start_secs && ntp_secs <= end_secs)
    return ngrtos_TRUE;
  return ngrtos_FALSE;
}

/*
 * See if the caller is allowed to set the time.  This code establishes
 * the pecking order for time services.
 */
static ng_bool_t 
time_setting_allowed (ng_clock_source_e new_source)
{
  switch (current_time_source()) 
  {
  	/* Current is NTP. Allow manual and NTP time setting. */
    case CLOCK_SOURCE_NTP:
    	switch (new_source) {
      	case CLOCK_SOURCE_NTP:
      	case CLOCK_SOURCE_MANUAL:
        case CLOCK_SOURCE_GNSS:
    	    return ngrtos_TRUE;
      	default:
    	    return ngrtos_FALSE;
    	}
    	break;

  	/* Current is SNTP. Allow manual, NTP, and SNTP time setting. */
    case CLOCK_SOURCE_SNTP:
    	switch (new_source) {
      	case CLOCK_SOURCE_NTP:
      	case CLOCK_SOURCE_SNTP:
        case CLOCK_SOURCE_GNSS:
      	case CLOCK_SOURCE_MANUAL:
          return ngrtos_TRUE;
      	default:
          return ngrtos_FALSE;
    	}
    	break;
      
    case CLOCK_SOURCE_GNSS:
    	switch (new_source) {
      	case CLOCK_SOURCE_NTP:
      	case CLOCK_SOURCE_SNTP:
        case CLOCK_SOURCE_GNSS:
      	case CLOCK_SOURCE_MANUAL:
          return ngrtos_TRUE;
      	default:
          return ngrtos_FALSE;
    	}
    	break;

  	/* Anybody else:  allow anybody to set the time. */
    case CLOCK_SOURCE_NONE:
    case CLOCK_SOURCE_MANUAL:
    case CLOCK_SOURCE_CALENDAR:
    case CLOCK_SOURCE_VINES:
    case CLOCK_SOURCE_RFC868_TP:
    default:
      return ngrtos_TRUE;
  }
}

/*
 * Set the current time based on seconds since 1/1/1970.  For outside
 * callers.  Returns whether or not the clock was set (i.e., whether
 * the clock precedence rules allow us to set the clock).
 */
ng_bool_t 
clock_set_unix_time(u_long unixtime, ng_clock_source_e source)
{
  ng_clock_epoch_s epoch;
  if (!time_setting_allowed(source))
  	return ngrtos_FALSE;

  unix_time_to_epoch(unixtime, &epoch);
  clock_set(&epoch, source);
  return ngrtos_TRUE;
}
    
/*
 * This routine returns the time in seconds since January 1, 1970,
 * for those folks that like Un*x.  This time is always UTC relative.
 */
u_long 
clock_get_unix_time(void)
{
  ng_clock_epoch_s curtime;
  clock_get_time(&curtime);
  return clock_epoch_to_unix_time(&curtime);
}

/*
 * This routine returns the time in seconds and nanoseconds since January
 * 1, 1970, for DNSIX.  Always UTC relative.  Truncation error yields
 * millisecond accuracy, but that's tough.
 */
void 
secs_and_nsecs_since_jan_1_1970 (ng_timeval_s *result)
{
  ng_clock_epoch_s curtime;
  clock_get_time(&curtime);
  result->tv_sec = clock_epoch_to_unix_time(&curtime);
  result->tv_nsec = (curtime.epoch_frac / (1<<16)) *
    (1000000000/(1<<16));
}
    
/*
 * Set the timezone offset and name
 */
void clock_set_timezone (int tz, ng_string_s *tz_name)
{
  /* Free any malloced name string. */
  if (standard_tz.alloced) 
  {
    ng_free(standard_tz.name.ptr);
  }
  /* Set up the standard_tz, eg. offset. */
  standard_tz.alloced = ngrtos_TRUE;
  standard_tz.offset  = tz;
  standard_tz.name = *tz_name;

  /* Copy over to the system timezone variables. */
  sys_tz.offset  = standard_tz.offset;
  sys_tz.name    = standard_tz.name;
  sys_tz.alloced = ngrtos_FALSE;
}

/*
 * This routine gets the time as of the last NMI tick.  For exact time,
 * see above.
 */
static void __clock_get_time_raw(ng_clock_epoch_s *timeptr)
{
  ng_rtc_time_s date;
  
  if (NG_result_ok == read_calendar(&date))
  {
    clock_rtc_to_epoch(&date, timeptr);
  }
  else
  {
    *timeptr  = ng_system_clock_epoch;
  }
}

/*
 * This routine gets the time as of the last NMI tick.  For exact time,
 * see above.
 */
void clock_get_time(ng_clock_epoch_s *timeptr)
{
  __clock_get_time_raw(timeptr);
  /* Now update the summer time info as necessary. */
  check_for_summer_time(timeptr);
}

void clock_get_date(ng_rtc_time_s *dateptr)
{
  int tz_offset;
  ng_clock_epoch_s ntp;
  if (NG_result_ok == read_calendar(dateptr))
  {
    clock_rtc_to_epoch(dateptr, &ntp);
    check_for_summer_time(&ntp);
    clock_epoch_to_rtc(&ntp, dateptr, tz_offset);
  }
  else
  {
    clock_get_time(&ntp);
    clock_epoch_to_rtc(&ntp, dateptr, 0);
  }
}
