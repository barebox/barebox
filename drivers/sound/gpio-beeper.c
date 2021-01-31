// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *  Copyright (C) 2021, Ahmad Fatoum
 */

#include <common.h>
#include <regulator.h>
#include <sound.h>
#include <of.h>
#include <gpio.h>
#include <of_gpio.h>

struct gpio_beeper {
	int gpio;
	struct sound_card card;
};

static int gpio_beeper_beep(struct sound_card *card, unsigned freq, unsigned duration)
{
	struct gpio_beeper *beeper = container_of(card, struct gpio_beeper, card);

	gpio_set_active(beeper->gpio, freq);
	return 0;
}

static int gpio_beeper_probe(struct device_d *dev)
{
	struct device_node *np = dev->device_node;
	struct gpio_beeper *beeper;
	struct sound_card *card;
	enum of_gpio_flags of_flags;
	unsigned long gpio_flags = GPIOF_OUT_INIT_ACTIVE;
	int ret, gpio;

	gpio = of_get_named_gpio_flags(np, "gpios", 0, &of_flags);
	if (!gpio_is_valid(gpio))
		return gpio;

	if (of_flags & OF_GPIO_ACTIVE_LOW)
		gpio_flags |= GPIOF_ACTIVE_LOW;

	ret = gpio_request_one(gpio, gpio_flags, "gpio-beeper");
	if (ret) {
		dev_err(dev, "failed to request gpio %d: %d\n", gpio, ret);
		return ret;
	}

	beeper = xzalloc(sizeof(*beeper));
	beeper->gpio = gpio;
	dev->priv = beeper;

	card = &beeper->card;
	card->name = np->full_name;
	card->beep = gpio_beeper_beep;

	return sound_card_register(card);
}

static void gpio_beeper_suspend(struct device_d *dev)
{
	struct gpio_beeper *beeper = dev->priv;

	gpio_beeper_beep(&beeper->card, 0, 0);
}

static const struct of_device_id gpio_beeper_match[] = {
	{ .compatible = "gpio-beeper", },
	{ },
};

static struct driver_d gpio_beeper_driver = {
	.name		= "gpio-beeper",
	.probe		= gpio_beeper_probe,
	.remove		= gpio_beeper_suspend,
	.of_compatible	= gpio_beeper_match,
};
device_platform_driver(gpio_beeper_driver);
