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

/**
 * struct pwm_args - board-dependent PWM arguments
 * @period_ns: reference period
 * @polarity: reference polarity
 *
 * This structure describes board-dependent arguments attached to a PWM
 * device. These arguments are usually retrieved from the PWM lookup table or
 * device tree.
 *
 * Do not confuse this with the PWM state: PWM arguments represent the initial
 * configuration that users want to use on this PWM device rather than the
 * current PWM hardware state.
 */

struct pwm_args {
	unsigned int period_ns;
	unsigned int polarity;
};

struct pwm_device {
	struct			pwm_chip *chip;
	unsigned long		flags;
#define FLAG_REQUESTED	0
	struct list_head	node;
	struct device_d		*hwdev;
	struct device_d		dev;

	struct pwm_state	params;
	struct pwm_args		args;
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

static int apply_params(struct param_d *p, void *priv)
{
	struct pwm_device *pwm = priv;

	return pwm_apply_state(pwm, &pwm->params);
}

static int set_enable(struct param_d *p, void *priv)
{
	struct pwm_device *pwm = priv;

	if (pwm->params.p_enable)
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
	int ret;

	if (_find_pwm(chip->devname))
		return -EBUSY;

	pwm = xzalloc(sizeof(*pwm));
	pwm->chip = chip;
	pwm->hwdev = dev;

	dev_set_name(&pwm->dev, chip->devname);
	pwm->dev.id = DEVICE_ID_SINGLE;
	pwm->dev.parent = dev;

	ret = register_device(&pwm->dev);
	if (ret)
		return ret;

	list_add_tail(&pwm->node, &pwm_list);

	p = dev_add_param_uint32(&pwm->dev, "duty_ns", apply_params,
			NULL, &pwm->params.duty_ns, "%u", pwm);
	if (IS_ERR(p))
		return PTR_ERR(p);

	p = dev_add_param_uint32(&pwm->dev, "period_ns", apply_params,
			NULL, &pwm->params.period_ns, "%u", pwm);
	if (IS_ERR(p))
		return PTR_ERR(p);

	p = dev_add_param_bool(&pwm->dev, "enable", set_enable,
			NULL, &pwm->params.p_enable, pwm);
	if (IS_ERR(p))
		return PTR_ERR(p);

	p = dev_add_param_bool(&pwm->dev, "inverted", apply_params,
			       NULL, &pwm->params.polarity, pwm);
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

static int __pwm_request(struct pwm_device *pwm)
{
	int ret;

	if (test_bit(FLAG_REQUESTED, &pwm->flags))
		return -EBUSY;

	if (pwm->chip->ops->request) {
		ret = pwm->chip->ops->request(pwm->chip);
		if (ret)
			return ret;
	}

	set_bit(FLAG_REQUESTED, &pwm->flags);

	return 0;
}

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

	ret = __pwm_request(pwm);
	if (ret)
		return NULL;

	return pwm;
}
EXPORT_SYMBOL_GPL(pwm_request);

static struct pwm_device *of_node_to_pwm_device(struct device_node *np, int id)
{
	struct pwm_device *pwm;

	list_for_each_entry(pwm, &pwm_list, node) {
		if (pwm->hwdev && pwm->hwdev->device_node == np &&
				pwm->chip->id == id)
			return pwm;
	}

        return ERR_PTR(-ENODEV);
}

struct pwm_device *of_pwm_request(struct device_node *np, const char *con_id)
{
	struct of_phandle_args args;
	int index = 0;
	struct pwm_device *pwm;
	struct pwm_state state;
	int ret;

	if (con_id)
		return ERR_PTR(-EINVAL);

	ret = of_parse_phandle_with_args(np, "pwms", "#pwm-cells", index,
			&args);
	if (ret) {
		pr_debug("%s(): can't parse \"pwms\" property\n", __func__);
		return ERR_PTR(ret);
	}

	pwm = of_node_to_pwm_device(args.np, args.args[0]);
	if (IS_ERR(pwm)) {
		pr_debug("%s(): PWM chip not found\n", __func__);
		return pwm;
	}

	if (args.args_count > 1)
		pwm->args.period_ns = args.args[1];

	pwm->args.polarity = PWM_POLARITY_NORMAL;

	if (args.args_count > 2 && args.args[2] & PWM_POLARITY_INVERTED)
		pwm->args.polarity = PWM_POLARITY_INVERTED;

	ret = __pwm_request(pwm);
	if (ret)
		return ERR_PTR(ret);

	pwm_init_state(pwm, &state);

	ret = pwm_apply_state(pwm, &state);
	if (ret)
		return ERR_PTR(ret);

	return pwm;
}
EXPORT_SYMBOL_GPL(of_pwm_request);

/*
 * pwm_free - free a PWM device
 */
void pwm_free(struct pwm_device *pwm)
{
	if (!test_and_clear_bit(FLAG_REQUESTED, &pwm->flags))
		return;
}
EXPORT_SYMBOL_GPL(pwm_free);

void pwm_get_state(const struct pwm_device *pwm, struct pwm_state *state)
{
	*state = pwm->chip->state;
}
EXPORT_SYMBOL_GPL(pwm_get_state);

static void pwm_get_args(const struct pwm_device *pwm, struct pwm_args *args)
{
	*args = pwm->args;
}

/**
 * pwm_init_state() - prepare a new state to be applied with pwm_apply_state()
 * @pwm: PWM device
 * @state: state to fill with the prepared PWM state
 *
 * This functions prepares a state that can later be tweaked and applied
 * to the PWM device with pwm_apply_state(). This is a convenient function
 * that first retrieves the current PWM state and the replaces the period
 * and polarity fields with the reference values defined in pwm->args.
 * Once the function returns, you can adjust the ->enabled and ->duty_cycle
 * fields according to your needs before calling pwm_apply_state().
 *
 * ->duty_cycle is initially set to zero to avoid cases where the current
 * ->duty_cycle value exceed the pwm_args->period one, which would trigger
 * an error if the user calls pwm_apply_state() without adjusting ->duty_cycle
 * first.
 */
void pwm_init_state(const struct pwm_device *pwm,
		    struct pwm_state *state)
{
	struct pwm_args args;

	/* First get the current state. */
	pwm_get_state(pwm, state);

	/* Then fill it with the reference config */
	pwm_get_args(pwm, &args);

	state->period_ns = args.period_ns;
	state->polarity = args.polarity;
	state->duty_ns = 0;
}
EXPORT_SYMBOL_GPL(pwm_init_state);

int pwm_apply_state(struct pwm_device *pwm, const struct pwm_state *state)
{
	struct pwm_chip *chip = pwm->chip;
	int ret = -EINVAL;

	if (state->period_ns == 0)
		goto err;

	if (state->duty_ns > state->period_ns)
		goto err;

	ret = chip->ops->apply(chip, state);
err:
	if (ret == 0)
		chip->state = *state;

	pwm->params = chip->state;
	return ret;
}

/*
 * pwm_config - change a PWM device configuration
 */
int pwm_config(struct pwm_device *pwm, int duty_ns, int period_ns)
{
	struct pwm_state state;

	if (duty_ns < 0 || period_ns < 0)
		return -EINVAL;

	pwm_get_state(pwm, &state);
	if (state.duty_ns == duty_ns && state.period_ns == period_ns)
		return 0;

	state.duty_ns = duty_ns;
	state.period_ns = period_ns;
	return pwm_apply_state(pwm, &state);
}
EXPORT_SYMBOL_GPL(pwm_config);

unsigned int pwm_get_period(struct pwm_device *pwm)
{
	return pwm->chip->state.period_ns;
}

/*
 * pwm_enable - start a PWM output toggling
 */
int pwm_enable(struct pwm_device *pwm)
{
	struct pwm_state state;

	pwm_get_state(pwm, &state);
	if (state.p_enable)
		return 0;

	state.p_enable = true;
	return pwm_apply_state(pwm, &state);
}
EXPORT_SYMBOL_GPL(pwm_enable);

/*
 * pwm_disable - stop a PWM output toggling
 */
void pwm_disable(struct pwm_device *pwm)
{
	struct pwm_state state;

	pwm_get_state(pwm, &state);
	if (!state.p_enable)
		return;

	state.p_enable = false;
	pwm_apply_state(pwm, &state);
}
EXPORT_SYMBOL_GPL(pwm_disable);
