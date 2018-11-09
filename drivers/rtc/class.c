/*
 * RTC barebox subsystem, base class
 *
 * Copyright (C) 2014 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/err.h>
#include <rtc.h>
#include <linux/rtc.h>

LIST_HEAD(rtc_list);
EXPORT_SYMBOL(rtc_list);

struct rtc_device *rtc_lookup(const char *name)
{
	struct rtc_device *r;

	if (!name)
		return ERR_PTR(-ENODEV);

	list_for_each_entry(r, &rtc_list, list) {
		if (!strcmp(dev_name(&r->class_dev), name))
			return r;
	}

	return ERR_PTR(-ENODEV);
}
EXPORT_SYMBOL(rtc_lookup);

int rtc_read_time(struct rtc_device *rtc, struct rtc_time *tm)
{
	return rtc->ops->read_time(rtc, tm);
}
EXPORT_SYMBOL(rtc_read_time);

int rtc_set_time(struct rtc_device *rtc, struct rtc_time *tm)
{
	struct rtc_time time;
	unsigned long secs;

	if (rtc_valid_tm(tm))
		return -EINVAL;

	rtc_tm_to_time(tm, &secs);
	rtc_time_to_tm(secs, &time);

	return rtc->ops->set_time(rtc, &time);
}
EXPORT_SYMBOL(rtc_set_time);

int rtc_register(struct rtc_device *rtcdev)
{
	struct device_d *dev = &rtcdev->class_dev;

	if (!rtcdev->ops)
		return -EINVAL;

	dev->id = DEVICE_ID_DYNAMIC;
	dev_set_name(dev, "rtc");
	if (rtcdev->dev)
		dev->parent = rtcdev->dev;
	platform_device_register(dev);

	list_add_tail(&rtcdev->list, &rtc_list);

	return 0;
}
EXPORT_SYMBOL(rtc_register);
