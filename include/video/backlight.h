#ifndef __VIDEO_BACKLIGHT_H
#define __VIDEO_BACKLIGHT_H

#ifdef CONFIG_DRIVER_VIDEO_BACKLIGHT
struct backlight_device {
	int brightness;
	int brightness_cur;
	int brightness_max;
	int brightness_default;
	int slew_time_ms; /* time to stretch brightness changes */
	int (*brightness_set)(struct backlight_device *, int brightness);
	struct list_head list;
	struct device_d dev;
	struct device_node *node;
};

int backlight_set_brightness(struct backlight_device *, int brightness);
int backlight_set_brightness_default(struct backlight_device *);
int backlight_register(struct backlight_device *);
struct backlight_device *of_backlight_find(struct device_node *node);
#else
struct backlight_device ;

static inline int
backlight_set_brightness(struct backlight_device *dev, int brightness)
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

#endif /* __VIDEO_BACKLIGHT_H */
