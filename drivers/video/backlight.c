// SPDX-License-Identifier: GPL-2.0-only
#include <common.h>
#include <driver.h>
#include <video/backlight.h>

static DEFINE_DEV_CLASS(backlight_class, "backlight");

int backlight_set_brightness(struct backlight_device *bl, unsigned brightness)
{
	int ret, step, i, num_steps;

	if (brightness > bl->brightness_max)
		brightness = bl->brightness_max;

	if (brightness == bl->brightness_cur)
		return 0;

	if (!bl->slew_time_ms) {
		ret = bl->brightness_set(bl, brightness);
		if (ret)
			return ret;

		bl->brightness_cur = bl->brightness = brightness;
		return 0;
	}

	if (brightness > bl->brightness_cur)
		step = 1;
	else
		step = -1;

	i = bl->brightness_cur;

	num_steps = abs(brightness - bl->brightness_cur);

	while (1) {
		i += step;

		ret = bl->brightness_set(bl, i);
		if (ret)
			return ret;

		if (i == brightness)
			break;

		udelay(bl->slew_time_ms * 1000 / num_steps);
	}

	bl->brightness_cur = bl->brightness = brightness;

	return ret;
}

int backlight_set_brightness_default(struct backlight_device *bl)
{
	int ret;

	ret = backlight_set_brightness(bl, bl->brightness_default);

	return ret;
}

static int backlight_brightness_set(struct param_d *p, void *priv)
{
	struct backlight_device *bl = priv;

	return backlight_set_brightness(bl, bl->brightness);
}

int backlight_register(struct backlight_device *bl)
{
	struct param_d *param;
	int ret;

	dev_set_name(&bl->dev, "backlight");
	bl->dev.id = DEVICE_ID_DYNAMIC;

	ret = register_device(&bl->dev);
	if (ret)
		return ret;

	param = dev_add_param_uint32(&bl->dev, "brightness", backlight_brightness_set,
				     NULL, &bl->brightness, "%u", bl);
	param_int_set_scale(param, bl->brightness_max);

	dev_add_param_uint32(&bl->dev, "slew_time_ms", NULL, NULL,
			     &bl->slew_time_ms, "%u", NULL);

	class_add_device(&backlight_class, &bl->dev);

	return ret;
}

struct backlight_device *of_backlight_find(struct device_node *node)
{
	struct backlight_device *bl;

	of_device_ensure_probed(node);

	class_for_each_container_of_device(&backlight_class, bl, dev)
		if (bl->node == node)
			return bl;

	return NULL;
}
