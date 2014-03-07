/*
 * pwm LED support for barebox
 *
 * (C) Copyright 2010 Sascha Hauer, Pengutronix
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
#include <led.h>
#include <pwm.h>
#include <of.h>
#include <asm-generic/div64.h>

struct pwmled {
	bool active_low;
	struct led led;
	struct pwm_device *pwm;
	uint32_t period;
};

static void led_pwm_set(struct led *led, unsigned int brightness)
{
	struct pwmled *pwmled = container_of(led, struct pwmled, led);
	unsigned long long duty =  pwmled->period;
	unsigned int max = pwmled->led.max_value;

        duty *= brightness;
        do_div(duty, max);

	pwm_config(pwmled->pwm, duty, pwmled->period);
}

static int led_pwm_of_probe(struct device_d *dev)
{
	struct device_node *child;
	int ret;

	for_each_child_of_node(dev->device_node, child) {
		struct pwmled *pwmled;
		struct pwm_device *pwm;

		pwm = of_pwm_request(child, NULL);
		if (pwm < 0)
			continue;

		pwmled = xzalloc(sizeof(*pwmled));
		pwmled->led.name = xstrdup(child->name);
		pwmled->pwm = pwm;

		of_property_read_u32(child, "max-brightness", &pwmled->led.max_value);

		pwmled->period = pwm_get_period(pwmled->pwm);

		pwmled->led.set = led_pwm_set;

		pwm_config(pwmled->pwm, 0, pwmled->period);
		pwm_enable(pwmled->pwm);

		ret = led_register(&pwmled->led);
		if (ret)
			return ret;

		led_of_parse_trigger(&pwmled->led, child);
	}

	return 0;
}

static struct of_device_id led_pwm_of_ids[] = {
	{ .compatible = "pwm-leds", },
	{ }
};

static struct driver_d led_pwm_of_driver = {
	.name  = "pwm-leds",
	.probe = led_pwm_of_probe,
	.of_compatible = DRV_OF_COMPAT(led_pwm_of_ids),
};
device_platform_driver(led_pwm_of_driver);
