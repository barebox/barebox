// SPDX-License-Identifier: GPL-2.0-only
/*
 * rtc and date/time utility functions
 *
 * This code was ported from linux-3.15 kernel by Antony Pavlov.
 *
 * Copyright (C) 2005-06 Tower Technologies
 * Author: Alessandro Zummo <a.zummo@towertech.it>
 *
 * based on arch/arm/common/rtctime.c and other bits
 */

#include <rtc.h>
#include <linux/rtc.h>

static const unsigned char rtc_days_in_month[] = {
	31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

#define LEAPS_THRU_END_OF(y) ((y)/4 - (y)/100 + (y)/400)

/*
 * The number of days in the month.
 */
int rtc_month_days(unsigned int month, unsigned int year)
{
	return rtc_days_in_month[month] + (is_leap_year(year) && month == 1);
}
EXPORT_SYMBOL(rtc_month_days);

/*
 * Convert seconds since 01-01-1970 00:00:00 to Gregorian date.
 */
void rtc_time_to_tm(unsigned long time, struct rtc_time *tm)
{
	unsigned int month, year;
	int days;

	days = time / 86400;
	time -= (unsigned int) days * 86400;

	/* day of the week, 1970-01-01 was a Thursday */
	tm->tm_wday = (days + 4) % 7;

	year = 1970 + days / 365;
	days -= (year - 1970) * 365
		+ LEAPS_THRU_END_OF(year - 1)
		- LEAPS_THRU_END_OF(1970 - 1);
	if (days < 0) {
		year -= 1;
		days += 365 + is_leap_year(year);
	}
	tm->tm_year = year - 1900;
	tm->tm_yday = days + 1;

	for (month = 0; month < 11; month++) {
		int newdays;

		newdays = days - rtc_month_days(month, year);
		if (newdays < 0)
			break;
		days = newdays;
	}
	tm->tm_mon = month;
	tm->tm_mday = days + 1;

	tm->tm_hour = time / 3600;
	time -= tm->tm_hour * 3600;
	tm->tm_min = time / 60;
	tm->tm_sec = time - tm->tm_min * 60;

	tm->tm_isdst = 0;
}
EXPORT_SYMBOL(rtc_time_to_tm);

// SPDX-SnippetBegin
// SPDX-Snippet-Comment: Origin-URL: https://github.com/u-boot/u-boot/blob/467382ca03758e4f3f13107e3a83669e93a7461e/lib/date.c

static int month_offset[] = {
	0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
};

/*
 * This only works for the Gregorian calendar - i.e. after 1752 (in the UK)
 */
int rtc_calc_weekday(struct rtc_time *tm)
{
	int leaps_to_date;
	int last_year;
	int day;

	if (tm->tm_year < 1753)
		return -1;
	last_year = tm->tm_year - 1;

	/* Number of leap corrections to apply up to end of last year */
	leaps_to_date = last_year / 4 - last_year / 100 + last_year / 400;

	/*
	 * This year is a leap year if it is divisible by 4 except when it is
	 * divisible by 100 unless it is divisible by 400
	 *
	 * e.g. 1904 was a leap year, 1900 was not, 1996 is, and 2000 is.
	 */
	if (tm->tm_year % 4 == 0 &&
	    ((tm->tm_year % 100 != 0) || (tm->tm_year % 400 == 0)) &&
	    tm->tm_mon > 2) {
		/* We are past Feb. 29 in a leap year */
		day = 1;
	} else {
		day = 0;
	}

	day += last_year * 365 + leaps_to_date + month_offset[tm->tm_mon - 1] +
			tm->tm_mday;
	tm->tm_wday = day % 7;

	return 0;
}
EXPORT_SYMBOL(rtc_calc_weekday);

// SPDX-SnippetEnd

/*
 * Does the rtc_time represent a valid date/time?
 */
int rtc_valid_tm(struct rtc_time *tm)
{
	if (tm->tm_year < 70
		|| ((unsigned)tm->tm_mon) >= 12
		|| tm->tm_wday < 0
		|| tm->tm_wday > 6
		|| tm->tm_mday < 1
		|| tm->tm_mday > rtc_month_days(tm->tm_mon, tm->tm_year + 1900)
		|| ((unsigned)tm->tm_hour) >= 24
		|| ((unsigned)tm->tm_min) >= 60
		|| ((unsigned)tm->tm_sec) >= 60)
		return -EINVAL;

	return 0;
}
EXPORT_SYMBOL(rtc_valid_tm);

/*
 * Convert Gregorian date to seconds since 01-01-1970 00:00:00.
 */
int rtc_tm_to_time(struct rtc_time *tm, unsigned long *time)
{
	*time = mktime(tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
			tm->tm_hour, tm->tm_min, tm->tm_sec);
	return 0;
}
EXPORT_SYMBOL(rtc_tm_to_time);
