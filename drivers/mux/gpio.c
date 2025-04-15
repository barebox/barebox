// SPDX-License-Identifier: GPL-2.0
/*
 * GPIO-controlled multiplexer driver
 *
 * Copyright (C) 2017 Axentia Technologies AB
 *
 * Author: Peter Rosin <peda@axentia.se>
 */

#include <linux/bitmap.h>
#include <linux/err.h>
#include <linux/gpio/consumer.h>
#include <linux/mod_devicetable.h>
#include <module.h>
#include <linux/mux/driver.h>
#include <driver.h>
#include <of.h>

struct mux_gpio {
	struct gpio_descs *gpios;
};

static int mux_gpio_set(struct mux_control *mux, int state)
{
	struct mux_gpio *mux_gpio = mux_chip_priv(mux->chip);
	DECLARE_BITMAP(values, BITS_PER_TYPE(state));
	u32 value = state;

	bitmap_from_arr32(values, &value, BITS_PER_TYPE(value));

	gpiod_set_array_value(mux_gpio->gpios->ndescs,
			      mux_gpio->gpios->desc,
			      mux_gpio->gpios->info, values);

	return 0;
}

static const struct mux_control_ops mux_gpio_ops = {
	.set = mux_gpio_set,
};

static const struct of_device_id mux_gpio_dt_ids[] = {
	{ .compatible = "gpio-mux", },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, mux_gpio_dt_ids);

static int mux_gpio_probe(struct device *dev)
{
	struct mux_chip *mux_chip;
	struct mux_gpio *mux_gpio;
	int pins;
	s32 idle_state;
	int ret;

	pins = gpiod_count(dev, "mux");
	if (pins < 0)
		return pins;

	mux_chip = mux_chip_alloc(dev, 1, sizeof(*mux_gpio));
	if (IS_ERR(mux_chip))
		return PTR_ERR(mux_chip);

	mux_gpio = mux_chip_priv(mux_chip);
	mux_chip->ops = &mux_gpio_ops;

	mux_gpio->gpios = gpiod_get_array(dev, "mux", GPIOD_OUT_LOW);
	if (IS_ERR(mux_gpio->gpios))
		return dev_err_probe(dev, PTR_ERR(mux_gpio->gpios),
				     "failed to get gpios\n");
	WARN_ON(pins != mux_gpio->gpios->ndescs);
	mux_chip->mux->states = BIT(pins);

	ret = of_property_read_u32(dev->of_node, "idle-state", (u32 *)&idle_state);
	if (ret >= 0 && idle_state != MUX_IDLE_AS_IS) {
		if (idle_state < 0 || idle_state >= mux_chip->mux->states) {
			dev_err(dev, "invalid idle-state %u\n", idle_state);
			return -EINVAL;
		}

		mux_chip->mux->idle_state = idle_state;
	}

	ret = mux_chip_register(mux_chip);
	if (ret < 0)
		return ret;

	dev_info(dev, "%u-way mux-controller registered\n",
		 mux_chip->mux->states);

	return 0;
}

static struct driver mux_gpio_driver = {
	.name = "gpio-mux",
	.of_match_table	= mux_gpio_dt_ids,
	.probe = mux_gpio_probe,
};
coredevice_platform_driver(mux_gpio_driver);

MODULE_DESCRIPTION("GPIO-controlled multiplexer driver");
MODULE_AUTHOR("Peter Rosin <peda@axentia.se>");
MODULE_LICENSE("GPL v2");
