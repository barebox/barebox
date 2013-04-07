/*
 * Generic pwmlib implementation
 *
 * Copyright (C) 2011 Sascha Hauer <s.hauer@pengutronix.de>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 */
#include <common.h>
#include <errno.h>
#include <malloc.h>
#include <pwm.h>
#include <linux/list.h>
#include <linux/err.h>

struct pwm_device {
	struct			pwm_chip *chip;
	unsigned long		flags;
#define FLAG_REQUESTED	0
#define FLAG_ENABLED	1
	struct list_head	node;
	struct device_d		*dev;

	unsigned int		duty_ns;
	unsigned int		period_ns;
	unsigned int		p_enable;
};

static LIST_HEAD(pwm_list);

static struct pwm_device *_find_pwm(const char *devname)
{
	struct pwm_device *pwm;

	list_for_each_entry(pwm, &pwm_list, node) {
		if (!strcmp(pwm->chip->devname, devname))
			return pwm;
	}

	return NULL;
}

static int set_duty_period_ns(struct param_d *p, void *priv)
{
	struct pwm_device *pwm = priv;

	pwm_config(pwm, pwm->chip->duty_ns, pwm->chip->period_ns);

	return 0;
}

static int set_enable(struct param_d *p, void *priv)
{
	struct pwm_device *pwm = priv;

	if (pwm->p_enable)
		pwm_enable(pwm);
	else
		pwm_disable(pwm);

	return 0;
}

/**
 * pwmchip_add() - register a new pwm
 * @chip: the pwm
 *
 * register a new pwm. pwm->devname must be initialized, usually
 * from dev_name(dev) from the hardware driver.
 */
int pwmchip_add(struct pwm_chip *chip, struct device_d *dev)
{
	struct pwm_device *pwm;
	struct param_d *p;

	if (_find_pwm(chip->devname))
		return -EBUSY;

	pwm = xzalloc(sizeof(*pwm));
	pwm->chip = chip;
	pwm->dev = dev;

	list_add_tail(&pwm->node, &pwm_list);

	p = dev_add_param_int(dev, "duty_ns", set_duty_period_ns,
			NULL, &pwm->chip->duty_ns, "%u", pwm);
	if (IS_ERR(p))
		return PTR_ERR(p);

	p = dev_add_param_int(dev, "period_ns", set_duty_period_ns,
			NULL, &pwm->chip->period_ns, "%u", pwm);
	if (IS_ERR(p))
		return PTR_ERR(p);

	p = dev_add_param_bool(dev, "enable", set_enable,
			NULL, &pwm->p_enable, pwm);
	if (IS_ERR(p))
		return PTR_ERR(p);

	return 0;
}
EXPORT_SYMBOL_GPL(pwmchip_add);

/**
 * pwmchip_remove() - remove a pwm
 * @chip: the pwm
 *
 * remove a pwm. This function may return busy if the pwm is still requested.
 */
int pwmchip_remove(struct pwm_chip *chip)
{
	struct pwm_device *pwm;

	pwm = _find_pwm(chip->devname);
	if (!pwm)
		return -ENOENT;

	if (test_bit(FLAG_REQUESTED, &pwm->flags))
		return -EBUSY;

	list_del(&pwm->node);

	kfree(pwm);

	return 0;
}
EXPORT_SYMBOL_GPL(pwmchip_remove);

/*
 * pwm_request - request a PWM device
 */
struct pwm_device *pwm_request(const char *devname)
{
	struct pwm_device *pwm;
	int ret;

	pwm = _find_pwm(devname);
	if (!pwm)
		return NULL;

	if (test_bit(FLAG_REQUESTED, &pwm->flags))
		return NULL;

	if (pwm->chip->ops->request) {
		ret = pwm->chip->ops->request(pwm->chip);
		if (ret)
			return NULL;
	}

	set_bit(FLAG_REQUESTED, &pwm->flags);

	return pwm;
}
EXPORT_SYMBOL_GPL(pwm_request);

/*
 * pwm_free - free a PWM device
 */
void pwm_free(struct pwm_device *pwm)
{
	if (!test_and_clear_bit(FLAG_REQUESTED, &pwm->flags))
		return;
}
EXPORT_SYMBOL_GPL(pwm_free);

/*
 * pwm_config - change a PWM device configuration
 */
int pwm_config(struct pwm_device *pwm, int duty_ns, int period_ns)
{
	pwm->chip->duty_ns = duty_ns;
	pwm->chip->period_ns = period_ns;

	if (period_ns == 0)
		return -EINVAL;

	if (duty_ns > period_ns)
		return -EINVAL;

	return pwm->chip->ops->config(pwm->chip, duty_ns, period_ns);
}
EXPORT_SYMBOL_GPL(pwm_config);

/*
 * pwm_enable - start a PWM output toggling
 */
int pwm_enable(struct pwm_device *pwm)
{
	if (!test_and_set_bit(FLAG_ENABLED, &pwm->flags))
		return pwm->chip->ops->enable(pwm->chip);

	return 0;
}
EXPORT_SYMBOL_GPL(pwm_enable);

/*
 * pwm_disable - stop a PWM output toggling
 */
void pwm_disable(struct pwm_device *pwm)
{
	if (test_and_clear_bit(FLAG_ENABLED, &pwm->flags))
		pwm->chip->ops->disable(pwm->chip);
}
EXPORT_SYMBOL_GPL(pwm_disable);
