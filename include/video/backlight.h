/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __VIDEO_BACKLIGHT_H
#define __VIDEO_BACKLIGHT_H

#include <device.h>

#ifdef CONFIG_DRIVER_VIDEO_BACKLIGHT
struct backlight_device {
	int brightness;
	int brightness_cur;
	int brightness_max;
	int brightness_default;
	int slew_time_ms; /* time to stretch brightness changes */
	int (*brightness_set)(struct backlight_device *, int brightness);
	struct device dev;
	struct device_node *node;
};

int backlight_set_brightness(struct backlight_device *, unsigned brightness);
int backlight_set_brightness_default(struct backlight_device *);
int backlight_register(struct backlight_device *);
struct backlight_device *of_backlight_find(struct device_node *node);
#else
struct backlight_device ;

static inline int
backlight_set_brightness(struct backlight_device *dev, unsigned brightness)
{
	return 0;
}
static inline int
backlight_set_brightness_default(struct backlight_device *dev)
{
	return 0;
}
static inline struct backlight_device *
of_backlight_find(struct device_node *node) { return NULL; }
#endif

static inline int
backlight_enable(struct backlight_device *dev)
{
	return dev ? backlight_set_brightness_default(dev) : 0;
}

static inline int
backlight_disable(struct backlight_device *dev)
{
	return dev ? backlight_set_brightness(dev, 0) : 0;
}

#endif /* __VIDEO_BACKLIGHT_H */
