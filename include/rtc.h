/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * (C) Copyright 2001
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 */

/*
 * Generic RTC interface.
 */
#ifndef _RTC_H_
#define _RTC_H_

#include <linux/err.h>

/*
 * The struct used to pass data from the generic interface code to
 * the hardware dependend low-level code ande vice versa. Identical
 * to struct rtc_time used by the Linux kernel.
 *
 * Note that there are small but significant differences to the
 * common "struct time":
 *
 *		struct time:		struct rtc_time:
 * tm_mon	0 ... 11		1 ... 12
 * tm_year	years since 1900	years since 0
 */

struct rtc_time {
	int tm_sec;
	int tm_min;
	int tm_hour;
	int tm_mday;
	int tm_mon;
	int tm_year;
	int tm_wday;
	int tm_yday;
	int tm_isdst;
};

void GregorianDay (struct rtc_time *);
void to_tm (int, struct rtc_time *);
unsigned long mktime (unsigned int, unsigned int, unsigned int,
		      unsigned int, unsigned int, unsigned int);

struct rtc_device;
#ifdef CONFIG_RTC_CLASS
extern struct rtc_device *rtc_lookup(const char *name);
#else
static inline struct rtc_device *rtc_lookup(const char *name)
{
	return ERR_PTR(-ENODEV);
}
#endif

const char *time_str(struct rtc_time *tm);

#endif	/* _RTC_H_ */
