// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * pwm backlight support for barebox
 *
 * (C) Copyright 2014 Sascha Hauer, Pengutronix
 */
#include <common.h>
#include <malloc.h>
#include <init.h>
#include <video/backlight.h>
#include <pwm.h>
#include <linux/err.h>
#include <of.h>
#include <regulator.h>
#include <linux/device.h>
#include <linux/gpio/consumer.h>
#include <linux/math64.h>

struct pwm_backlight {
	struct backlight_device backlight;
	struct pwm_device *pwm;
	struct regulator *power;
	uint32_t period;
	unsigned int *levels;
	struct gpio_desc *enable_gpio;
	int enabled;
	unsigned int scale;
};

static int backlight_pwm_enable(struct pwm_backlight *pwm_backlight)
{
	int ret;

	if (pwm_backlight->enabled)
		return 0;

	ret = pwm_enable(pwm_backlight->pwm);
	if (ret)
		return ret;

	regulator_enable(pwm_backlight->power);

	gpiod_direction_output(pwm_backlight->enable_gpio, true);

	pwm_backlight->enabled = 1;

	return 0;
}

static int backlight_pwm_disable(struct pwm_backlight *pwm_backlight)
{
	int ret;

	if (!pwm_backlight->enabled)
		return 0;

	ret = gpiod_direction_output(pwm_backlight->enable_gpio, false);
	if (!ret) {
		regulator_disable(pwm_backlight->power);

		/*
		 * Only disable PWM when an enable gpio is present.
		 * The output of the PWM is undefined when the PWM
		 * is disabled.
		 */
		pwm_disable(pwm_backlight->pwm);
		pwm_backlight->enabled = 0;
	}

	return 0;
}

static int compute_duty_cycle(struct pwm_backlight *pwm_backlight, int brightness)
{
	int duty_cycle;

	if (pwm_backlight->levels)
		duty_cycle = pwm_backlight->levels[brightness];
	else
		duty_cycle = brightness;

	return duty_cycle * pwm_backlight->period / pwm_backlight->scale;
}

static int backlight_pwm_set(struct backlight_device *backlight,
		int brightness)
{
	struct pwm_backlight *pwm_backlight = container_of(backlight,
			struct pwm_backlight, backlight);

	pwm_config(pwm_backlight->pwm, compute_duty_cycle(pwm_backlight, brightness),
		   pwm_backlight->period);

	if (brightness)
		return backlight_pwm_enable(pwm_backlight);
	else
		return backlight_pwm_disable(pwm_backlight);
}

static int pwm_backlight_parse_dt(struct device *dev,
				  struct pwm_backlight *pwm_backlight)
{
	struct device_node *node = dev->of_node;
	struct property *prop;
	unsigned int length;
	unsigned int num_steps = 0;
	unsigned int *table;
	u32 value;
	int ret, i;

	if (!node)
		return -ENODEV;

	ret = of_property_read_u32(node, "default-brightness-level",
					   &value);
	if (ret < 0)
		return ret;

	pwm_backlight->backlight.brightness_default = value;

	/* determine the number of brightness levels */
	prop = of_find_property(node, "brightness-levels", &length);
	if (!prop)
		return -EINVAL;

	length /= sizeof(u32);

	/* read brightness levels from DT property */
	if (length > 0) {
		size_t size = sizeof(*pwm_backlight->levels) * length;

		pwm_backlight->levels = xzalloc(size);

		ret = of_property_read_u32_array(node, "brightness-levels",
						 pwm_backlight->levels,
						 length);
		if (ret < 0)
			return ret;

		/*
		 * This property is optional, if is set enables linear
		 * interpolation between each of the values of brightness levels
		 * and creates a new pre-computed table.
		 */
		of_property_read_u32(node, "num-interpolated-steps",
				     &num_steps);

		if (num_steps) {
			unsigned int num_input_levels = length;
			unsigned int i;
			u32 x1, x2, x, dx;
			u32 y1, y2;
			s64 dy;

			/*
			 * Make sure that there is at least two entries in the
			 * brightness-levels table, otherwise we can't interpolate
			 * between two points.
			 */
			if (num_input_levels < 2) {
				dev_err(dev, "can't interpolate\n");
				return -EINVAL;
			}

			/*
			 * Recalculate the number of brightness levels, now
			 * taking in consideration the number of interpolated
			 * steps between two levels.
			 */
			length = (num_input_levels - 1) * num_steps + 1;
			dev_dbg(dev, "new number of brightness levels: %d\n",
				length);

			/*
			 * Create a new table of brightness levels with all the
			 * interpolated steps.
			 */
			table = devm_kcalloc(dev, length, sizeof(*table),
					     GFP_KERNEL);
			if (!table)
				return -ENOMEM;
			/*
			 * Fill the interpolated table[x] = y
			 * by draw lines between each (x1, y1) to (x2, y2).
			 */
			dx = num_steps;
			for (i = 0; i < num_input_levels - 1; i++) {
				x1 = i * dx;
				x2 = x1 + dx;
				y1 = pwm_backlight->levels[i];
				y2 = pwm_backlight->levels[i + 1];
				dy = (s64)y2 - y1;

				for (x = x1; x < x2; x++) {
					table[x] =
						y1 + div_s64(dy * (x - x1), dx);
				}
			}
			/* Fill in the last point, since no line starts here. */
			table[x2] = y2;

			/*
			 * As we use interpolation lets remove current
			 * brightness levels table and replace for the
			 * new interpolated table.
			 */
			devm_kfree(dev, pwm_backlight->levels);
			pwm_backlight->levels = table;
		}

		pwm_backlight->backlight.brightness_max = length - 1;

		for (i = 0; i < length; i++)
			if (pwm_backlight->levels[i] > pwm_backlight->scale)
				pwm_backlight->scale = pwm_backlight->levels[i];

		if (pwm_backlight->scale == 0)
			return -EINVAL;
	} else {
		/* We implicitly assume here a linear levels array { 0, 1, 2, ... 100 } */
		pwm_backlight->scale = 100;
		pwm_backlight->backlight.brightness_max = pwm_backlight->scale;
	}

	pwm_backlight->enable_gpio = gpiod_get_optional(dev, "enable", GPIOD_ASIS);

	return 0;
}

static int backlight_pwm_of_probe(struct device *dev)
{
	int ret;
	struct pwm_backlight *pwm_backlight;
	struct pwm_device *pwm;

	pwm = of_pwm_request(dev->of_node, NULL);
	if (IS_ERR(pwm))
		return dev_errp_probe(dev, pwm, "Cannot find PWM device\n");

	pwm_backlight = xzalloc(sizeof(*pwm_backlight));
	pwm_backlight->pwm = pwm;
	pwm_backlight->period = pwm_get_period(pwm);

	ret = pwm_backlight_parse_dt(dev, pwm_backlight);
	if (ret)
		return ret;

	pwm_backlight->power = regulator_get_optional(dev, "power");
	if (IS_ERR(pwm_backlight->power)) {
		if (PTR_ERR(pwm_backlight->power) != -ENODEV)
			return dev_errp_probe(dev, pwm_backlight->power,
					      "power supply\n");
		pwm_backlight->power = NULL;
	}

	pwm_backlight->backlight.slew_time_ms = 100;
	pwm_backlight->backlight.brightness_set = backlight_pwm_set;
	pwm_backlight->backlight.dev.parent = dev;
	pwm_backlight->backlight.node = dev->of_node;

	ret = backlight_register(&pwm_backlight->backlight);
	if (ret)
		return ret;

	return 0;
}

static struct of_device_id backlight_pwm_of_ids[] = {
	{
		.compatible = "pwm-backlight",
	}, {
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(of, backlight_pwm_of_ids);

static struct driver backlight_pwm_of_driver = {
	.name  = "pwm-backlight",
	.probe = backlight_pwm_of_probe,
	.of_compatible = DRV_OF_COMPAT(backlight_pwm_of_ids),
};
device_platform_driver(backlight_pwm_of_driver);
