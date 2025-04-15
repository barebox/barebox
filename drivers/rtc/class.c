// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * RTC barebox subsystem, base class
 *
 * Copyright (C) 2014 Antony Pavlov <antonynpavlov@gmail.com>
 */

#include <linux/err.h>
#include <rtc.h>
#include <linux/rtc.h>
#include <device.h>

#define for_each_rtc(rtc) \
	class_for_each_container_of_device(&rtc_class, rtc, class_dev)

DEFINE_DEV_CLASS(rtc_class, "rtc");

struct rtc_device *rtc_lookup(const char *name)
{
	struct rtc_device *r;

	for_each_rtc(r) {
		if (!name || !strcmp(dev_name(&r->class_dev), name))
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
	struct device *dev = &rtcdev->class_dev;

	if (!rtcdev->ops)
		return -EINVAL;

	dev->id = DEVICE_ID_DYNAMIC;
	dev_set_name(dev, "rtc");
	if (rtcdev->dev)
		dev->parent = rtcdev->dev;
	platform_device_register(dev);

	class_add_device(&rtc_class, &rtcdev->class_dev);

	return 0;
}
EXPORT_SYMBOL(rtc_register);
