// SPDX-License-Identifier: GPL-2.0
/*
 * Multiplexer subsystem
 *
 * Copyright (C) 2017 Axentia Technologies AB
 *
 * Author: Peter Rosin <peda@axentia.se>
 */

#define pr_fmt(fmt) "mux-core: " fmt

#include <linux/ktime.h>
#include <driver.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/export.h>
#include <init.h>
#include <module.h>
#include <linux/mux/consumer.h>
#include <linux/mux/driver.h>
#include <of.h>
#include <stdio.h>
#include <malloc.h>

/*
 * The idle-as-is "state" is not an actual state that may be selected, it
 * only implies that the state should not be changed. So, use that state
 * as indication that the cached state of the multiplexer is unknown.
 */
#define MUX_CACHE_UNKNOWN MUX_IDLE_AS_IS

/**
 * struct mux_state -	Represents a mux controller state specific to a given
 *			consumer.
 * @mux:		Pointer to a mux controller.
 * @state:		State of the mux to be selected.
 *
 * This structure is specific to the consumer that acquires it and has
 * information specific to that consumer.
 */
struct mux_state {
	struct mux_control *mux;
	unsigned int state;
};

DEFINE_DEV_CLASS(mux_class, "mux");

#define for_each_mux(mux) \
	class_for_each_container_of_device(&mux_class, mux, dev)

/**
 * mux_chip_alloc() - Allocate a mux-chip.
 * @dev: The parent device implementing the mux interface.
 * @controllers: The number of mux controllers to allocate for this chip.
 * @sizeof_priv: Size of extra memory area for private use by the caller.
 *
 * After allocating the mux-chip with the desired number of mux controllers
 * but before registering the chip, the mux driver is required to configure
 * the number of valid mux states in the mux_chip->mux[N].states members and
 * the desired idle state in the returned mux_chip->mux[N].idle_state members.
 * The default idle state is MUX_IDLE_AS_IS. The mux driver also needs to
 * provide a pointer to the operations struct in the mux_chip->ops member
 * before registering the mux-chip with mux_chip_register.
 *
 * Return: A pointer to the new mux-chip, or an ERR_PTR with a negative errno.
 */
struct mux_chip *mux_chip_alloc(struct device *dev,
				unsigned int controllers, size_t sizeof_priv)
{
	struct mux_chip *mux_chip;
	int i;

	if (WARN_ON(!dev || !controllers))
		return ERR_PTR(-EINVAL);

	mux_chip = kzalloc(sizeof(*mux_chip) +
			   controllers * sizeof(*mux_chip->mux) +
			   sizeof_priv, GFP_KERNEL);
	if (!mux_chip)
		return ERR_PTR(-ENOMEM);


	mux_chip->mux = (struct mux_control *)(mux_chip + 1);
	mux_chip->dev.parent = dev;
	mux_chip->dev.of_node = dev->of_node;
	mux_chip->dev.priv = mux_chip;

	mux_chip->controllers = controllers;
	for (i = 0; i < controllers; ++i) {
		struct mux_control *mux = &mux_chip->mux[i];

		mux->chip = mux_chip;
		mux->cached_state = MUX_CACHE_UNKNOWN;
		mux->idle_state = MUX_IDLE_AS_IS;
		mux->idle = true;
		mux->last_change = ktime_get();
		mux->dev.id = i;
		mux->dev.parent = &mux_chip->dev;
	}

	return mux_chip;
}
EXPORT_SYMBOL_GPL(mux_chip_alloc);

static int mux_control_set(struct mux_control *mux, int state)
{
	int ret = mux->chip->ops->set(mux, state);

	mux->cached_state = ret < 0 ? MUX_CACHE_UNKNOWN : state;
	if (ret >= 0)
		mux->last_change = ktime_get();

	return ret;
}

static int mux_control_set_param(struct param_d *param, void *priv)
{
	struct mux_control *mux = priv;
	int state = mux->cached_state;

	if (state < 0 || state >= mux->states)
		return -EINVAL;

	return mux_control_set(mux, state);
}

/**
 * mux_chip_register() - Register a mux-chip, thus readying the controllers
 *			 for use.
 * @mux_chip: The mux-chip to register.
 *
 * Return: Zero on success or a negative errno on error.
 */
int mux_chip_register(struct mux_chip *mux_chip)
{
	int i;
	int ret;

	for (i = 0; i < mux_chip->controllers; ++i) {
		struct mux_control *mux = &mux_chip->mux[i];

		if (mux->idle_state == mux->cached_state)
			continue;

		ret = mux_control_set(mux, mux->idle_state);
		if (ret < 0) {
			dev_err(&mux_chip->dev, "unable to set idle state\n");
			return ret;
		}
	}

	ret = class_register_device(&mux_class, &mux_chip->dev, "muxchip");
	if (ret < 0) {
		dev_err(&mux_chip->dev,
			"register_device failed in %s: %d\n", __func__, ret);
		return ret;
	}

	if (IS_ENABLED(CONFIG_PARAMETER)) {
		struct device *dev = NULL;

		if (mux_chip->controllers == 1)
			dev = &mux_chip->dev;

		for (i = 0; i < mux_chip->controllers; ++i) {
			struct mux_control *mux = &mux_chip->mux[i];
			struct device *dev;

			if (mux_chip->controllers == 1) {
				dev = &mux_chip->dev;
			} else  {
				dev = &mux->dev;
				dev->parent = &mux_chip->dev;
				dev->id = i;
				dev_set_name(dev, "%s.", dev_name(&mux_chip->dev));
				if (register_device(dev))
					continue;
			}

			dev_add_param_bool_ro(dev, "idle", &mux->idle);
			dev_add_param_int(dev, "state", mux_control_set_param,
					  NULL, &mux->cached_state, "%d", mux);
		}
	}

	return 0;
}
EXPORT_SYMBOL_GPL(mux_chip_register);

/**
 * mux_control_states() - Query the number of multiplexer states.
 * @mux: The mux-control to query.
 *
 * Return: The number of multiplexer states.
 */
unsigned int mux_control_states(struct mux_control *mux)
{
	return mux->states;
}
EXPORT_SYMBOL_GPL(mux_control_states);

/*
 * The mux->idle must be clear when calling this function.
 */
static int __mux_control_select(struct mux_control *mux, int state)
{
	int ret;

	if (WARN_ON(state < 0 || state >= mux->states))
		return -EINVAL;

	if (mux->cached_state == state)
		return 0;

	ret = mux_control_set(mux, state);
	if (ret >= 0)
		return 0;

	/* The mux update failed, try to revert if appropriate... */
	if (mux->idle_state != MUX_IDLE_AS_IS)
		mux_control_set(mux, mux->idle_state);

	return ret;
}

static void mux_control_delay(struct mux_control *mux, unsigned int delay_us)
{
	ktime_t delayend;
	s64 remaining;

	if (!delay_us)
		return;

	delayend = ktime_add_us(mux->last_change, delay_us);
	remaining = ktime_us_delta(delayend, ktime_get());
	if (remaining > 0)
		udelay(remaining);
}

/**
 * mux_control_try_select_delay() - Try to select the given multiplexer state.
 * @mux: The mux-control to request a change of state from.
 * @state: The new requested state.
 * @delay_us: The time to delay (in microseconds) if the mux state is changed.
 *
 * On successfully selecting the mux-control state, it will be locked until
 * mux_control_deselect() is called.
 *
 * Therefore, make sure to call mux_control_deselect() when the operation is
 * complete and the mux-control is free for others to use, but do not call
 * mux_control_deselect() if mux_control_try_select() fails.
 *
 * Return: 0 when the mux-control state has the requested state or a negative
 * errno on error. Specifically -EBUSY if the mux-control is contended.
 */
int mux_control_try_select_delay(struct mux_control *mux, unsigned int state,
				 unsigned int delay_us)
{
	int ret;

	if (!mux->idle)
		return -EBUSY;
	mux->idle = false;

	ret = __mux_control_select(mux, state);
	if (ret >= 0)
		mux_control_delay(mux, delay_us);

	if (ret < 0)
		mux->idle = true;

	return ret;
}
EXPORT_SYMBOL_GPL(mux_control_try_select_delay);

/**
 * mux_state_try_select_delay() - Try to select the given multiplexer state.
 * @mstate: The mux-state to select.
 * @delay_us: The time to delay (in microseconds) if the mux state is changed.
 *
 * On successfully selecting the mux-state, its mux-control will be locked
 * until mux_state_deselect() is called.
 *
 * Therefore, make sure to call mux_state_deselect() when the operation is
 * complete and the mux-control is free for others to use, but do not call
 * mux_state_deselect() if mux_state_try_select() fails.
 *
 * Return: 0 when the mux-state has been selected or a negative errno on
 * error. Specifically -EBUSY if the mux-control is contended.
 */
int mux_state_try_select_delay(struct mux_state *mstate, unsigned int delay_us)
{
	return mux_control_try_select_delay(mstate->mux, mstate->state, delay_us);
}
EXPORT_SYMBOL_GPL(mux_state_try_select_delay);

/**
 * mux_control_deselect() - Deselect the previously selected multiplexer state.
 * @mux: The mux-control to deselect.
 *
 * It is required that a single call is made to mux_control_deselect() for
 * each and every successful call made to either of mux_control_select() or
 * mux_control_try_select().
 *
 * Return: 0 on success and a negative errno on error. An error can only
 * occur if the mux has an idle state. Note that even if an error occurs, the
 * mux-control is unlocked and is thus free for the next access.
 */
int mux_control_deselect(struct mux_control *mux)
{
	int ret = 0;

	if (mux->idle_state != MUX_IDLE_AS_IS &&
	    mux->idle_state != mux->cached_state)
		ret = mux_control_set(mux, mux->idle_state);

	mux->idle = true;

	return ret;
}
EXPORT_SYMBOL_GPL(mux_control_deselect);

/**
 * mux_state_deselect() - Deselect the previously selected multiplexer state.
 * @mstate: The mux-state to deselect.
 *
 * It is required that a single call is made to mux_state_deselect() for
 * each and every successful call made to either of mux_state_select() or
 * mux_state_try_select().
 *
 * Return: 0 on success and a negative errno on error. An error can only
 * occur if the mux has an idle state. Note that even if an error occurs, the
 * mux-control is unlocked and is thus free for the next access.
 */
int mux_state_deselect(struct mux_state *mstate)
{
	return mux_control_deselect(mstate->mux);
}
EXPORT_SYMBOL_GPL(mux_state_deselect);

/* Note this function returns a reference to the mux_chip dev. */
static struct mux_chip *of_find_mux_chip_by_node(struct device_node *np)
{
	struct mux_chip *chip;

	of_device_ensure_probed(np);

	for_each_mux(chip) {
		if (chip->dev.device_node == np)
			return chip;
	}

	return NULL;
}

/*
 * mux_get() - Get the mux-control for a device.
 * @dev: The device that needs a mux-control.
 * @mux_name: The name identifying the mux-control.
 * @state: Pointer to where the requested state is returned, or NULL when
 *         the required multiplexer states are handled by other means.
 *
 * Return: A pointer to the mux-control, or an ERR_PTR with a negative errno.
 */
static struct mux_control *mux_get(struct device *dev, const char *mux_name,
				   unsigned int *state)
{
	struct device_node *np = dev->of_node;
	struct of_phandle_args args;
	struct mux_chip *mux_chip;
	unsigned int controller;
	int index = 0;
	int ret;

	if (mux_name) {
		if (state)
			index = of_property_match_string(np, "mux-state-names",
							 mux_name);
		else
			index = of_property_match_string(np, "mux-control-names",
							 mux_name);
		if (index < 0) {
			dev_err(dev, "mux controller '%s' not found\n",
				mux_name);
			return ERR_PTR(index);
		}
	}

	if (state)
		ret = of_parse_phandle_with_args(np,
						 "mux-states", "#mux-state-cells",
						 index, &args);
	else
		ret = of_parse_phandle_with_args(np,
						 "mux-controls", "#mux-control-cells",
						 index, &args);
	if (ret) {
		dev_err(dev, "%pOF: failed to get mux-%s %s(%i)\n",
			np, state ? "state" : "control", mux_name ?: "", index);
		return ERR_PTR(ret);
	}

	mux_chip = of_find_mux_chip_by_node(args.np);
	of_node_put(args.np);
	if (!mux_chip)
		return ERR_PTR(-EPROBE_DEFER);

	controller = 0;
	if (state) {
		if (args.args_count > 2 || args.args_count == 0 ||
		    (args.args_count < 2 && mux_chip->controllers > 1)) {
			dev_err(dev, "%pOF: wrong #mux-state-cells for %pOF\n",
				np, args.np);
			put_device(&mux_chip->dev);
			return ERR_PTR(-EINVAL);
		}

		if (args.args_count == 2) {
			controller = args.args[0];
			*state = args.args[1];
		} else {
			*state = args.args[0];
		}

	} else {
		if (args.args_count > 1 ||
		    (!args.args_count && mux_chip->controllers > 1)) {
			dev_err(dev, "%pOF: wrong #mux-control-cells for %pOF\n",
				np, args.np);
			put_device(&mux_chip->dev);
			return ERR_PTR(-EINVAL);
		}

		if (args.args_count)
			controller = args.args[0];
	}

	if (controller >= mux_chip->controllers) {
		dev_err(dev, "%pOF: bad mux controller %u specified in %pOF\n",
			np, controller, args.np);
		put_device(&mux_chip->dev);
		return ERR_PTR(-EINVAL);
	}

	return &mux_chip->mux[controller];
}

/**
 * mux_control_get() - Get the mux-control for a device.
 * @dev: The device that needs a mux-control.
 * @mux_name: The name identifying the mux-control.
 *
 * Return: A pointer to the mux-control, or an ERR_PTR with a negative errno.
 */
struct mux_control *mux_control_get(struct device *dev, const char *mux_name)
{
	return mux_get(dev, mux_name, NULL);
}
EXPORT_SYMBOL_GPL(mux_control_get);

MODULE_DESCRIPTION("Multiplexer subsystem");
MODULE_AUTHOR("Peter Rosin <peda@axentia.se>");
MODULE_LICENSE("GPL v2");
