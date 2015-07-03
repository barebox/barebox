#ifndef __VIDEO_BACKLIGHT_H
#define __VIDEO_BACKLIGHT_H

struct backlight_device {
	int brightness;
	int brightness_cur;
	int brightness_max;
	int brightness_default;
	int (*brightness_set)(struct backlight_device *, int brightness);
	struct list_head list;
	struct device_d dev;
	struct device_node *node;
};

int backlight_set_brightness(struct backlight_device *, int brightness);
int backlight_set_brightness_default(struct backlight_device *);
int backlight_register(struct backlight_device *);
struct backlight_device *of_backlight_find(struct device_node *node);

#endif /* __VIDEO_BACKLIGHT_H */
