/*
 * pwm backlight support for barebox
 *
 * (C) Copyright 2014 Sascha Hauer, Pengutronix
 *
 * See file CREDITS for list of people who contributed to this
 * project.
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
#include <common.h>
#include <malloc.h>
#include <init.h>
#include <video/backlight.h>
#include <pwm.h>
#include <linux/err.h>
#include <of.h>
#include <regulator.h>
#include <gpio.h>
#include <of_gpio.h>
#include <asm-generic/div64.h>

struct pwm_backlight {
	struct backlight_device backlight;
	struct pwm_device *pwm;
	struct regulator *power;
	uint32_t period;
	unsigned int *levels;
	int enable_gpio;
	int enable_active_high;
	int max_value;
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

	if (gpio_is_valid(pwm_backlight->enable_gpio)) {
		gpio_direction_output(pwm_backlight->enable_gpio,
				pwm_backlight->enable_active_high);
	}

	pwm_backlight->enabled = 1;

	return 0;
}

static int backlight_pwm_disable(struct pwm_backlight *pwm_backlight)
{
	if (!pwm_backlight->enabled)
		return 0;

	if (gpio_is_valid(pwm_backlight->enable_gpio)) {
		gpio_direction_output(pwm_backlight->enable_gpio,
				!pwm_backlight->enable_active_high);

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

static int pwm_backlight_parse_dt(struct device_d *dev,
		struct pwm_backlight *pwm_backlight)
{
	struct device_node *node = dev->device_node;
	struct property *prop;
	int length;
	u32 value;
	int ret, i;
	enum of_gpio_flags flags;

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
		if (!pwm_backlight->levels)
			return -ENOMEM;

		ret = of_property_read_u32_array(node, "brightness-levels",
						 pwm_backlight->levels,
						 length);
		if (ret < 0)
			return ret;

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

	pwm_backlight->enable_gpio = of_get_named_gpio_flags(node, "enable-gpios", 0, &flags);

	if (gpio_is_valid(pwm_backlight->enable_gpio)) {
		if (!(flags & OF_GPIO_ACTIVE_LOW))
			pwm_backlight->enable_active_high = 1;
	}

	return 0;
}

static int backlight_pwm_of_probe(struct device_d *dev)
{
	int ret;
	struct pwm_backlight *pwm_backlight;
	struct pwm_device *pwm;

	pwm = of_pwm_request(dev->device_node, NULL);
	if (IS_ERR(pwm)) {
		dev_err(dev, "Cannot find PWM device\n");
		return PTR_ERR(pwm);
	}

	pwm_backlight = xzalloc(sizeof(*pwm_backlight));
	pwm_backlight->pwm = pwm;
	pwm_backlight->period = pwm_get_period(pwm);

	ret = pwm_backlight_parse_dt(dev, pwm_backlight);
	if (ret)
		return ret;

	pwm_backlight->power = regulator_get(dev, "power");
	if (IS_ERR(pwm_backlight->power)) {
		dev_err(dev, "Cannot find regulator\n");
		return PTR_ERR(pwm_backlight->power);
	}

	pwm_backlight->period = pwm_get_period(pwm_backlight->pwm);

	pwm_backlight->backlight.brightness_set = backlight_pwm_set;
	pwm_backlight->backlight.node = dev->device_node;

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

static struct driver_d backlight_pwm_of_driver = {
	.name  = "pwm-backlight",
	.probe = backlight_pwm_of_probe,
	.of_compatible = DRV_OF_COMPAT(backlight_pwm_of_ids),
};
device_platform_driver(backlight_pwm_of_driver);
