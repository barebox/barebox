/*
 * Copyright (C) 2011 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * Under GPLv2
 */

#include <common.h>
#include <errno.h>
#include <init.h>
#include <clock.h>
#include <gpio_keys.h>
#include <poller.h>
#include <gpio.h>
#include <of_gpio.h>
#include <input/keyboard.h>

struct gpio_key {
	int code;

	int gpio;
	int active_low;

	int previous_state;
};

struct gpio_keys {
	struct gpio_key *buttons;
	int nbuttons;

	/* optional */
	int fifo_size;

	struct kfifo *recv_fifo;
	struct poller_struct poller;
	struct console_device cdev;

	int use_keycodes;
};

static inline struct gpio_keys *
poller_to_gk_pdata(struct poller_struct *poller)
{
	return container_of(poller, struct gpio_keys, poller);
}

static inline struct gpio_keys *
cdev_to_gk_pdata(struct console_device *cdev)
{
	return container_of(cdev, struct gpio_keys, cdev);
}

static void gpio_key_poller(struct poller_struct *poller)
{
	struct gpio_keys *gk = poller_to_gk_pdata(poller);
	struct gpio_key *gb;
	int i, val;

	for (i = 0; i < gk->nbuttons; i++) {

		gb = &gk->buttons[i];
		val = gpio_get_value(gb->gpio);

		if (val != gb->previous_state && val != gb->active_low) {
			kfifo_put(gk->recv_fifo, (u_char*)&gb->code, sizeof(int));
			debug("pressed gpio(%d) as %d\n", gb->gpio, gb->code);
		}
		gb->previous_state = val;
	}
}

static int gpio_keys_tstc(struct console_device *cdev)
{
	struct gpio_keys *gk = cdev_to_gk_pdata(cdev);

	return (kfifo_len(gk->recv_fifo) == 0) ? 0 : 1;
}

static int gpio_keys_getc(struct console_device *cdev)
{
	int code = 0;
	struct gpio_keys *gk = cdev_to_gk_pdata(cdev);

	kfifo_get(gk->recv_fifo, (u_char*)&code, sizeof(int));

	if (IS_ENABLED(CONFIG_OFDEVICE) && gk->use_keycodes)
		return keycode_bb_keys[code];
	else
		return code;
}

static int gpio_keys_probe_pdata(struct gpio_keys *gk, struct device_d *dev)
{
	struct gpio_keys_platform_data *pdata;
	int i;

	pdata = dev->platform_data;

	if (!pdata) {
		/* small (so we copy it) but critical! */
		dev_err(dev, "missing platform_data\n");
		return -ENODEV;
	}

	if (pdata->fifo_size)
		gk->fifo_size = pdata->fifo_size;

	gk->buttons = xzalloc(pdata->nbuttons * sizeof(*gk->buttons));
	gk->nbuttons = pdata->nbuttons;

	for (i = 0; i < gk->nbuttons; i++) {
		gk->buttons[i].gpio = pdata->buttons[i].gpio;
		gk->buttons[i].code = pdata->buttons[i].code;
		gk->buttons[i].active_low = pdata->buttons[i].active_low;
	}

	return 0;
}

static int gpio_keys_probe_dt(struct gpio_keys *gk, struct device_d *dev)
{
	struct device_node *npkey, *np = dev->device_node;
	int i = 0, ret;

	if (!IS_ENABLED(CONFIG_OFDEVICE))
		return -ENODEV;

	gk->nbuttons = of_get_child_count(np);
	gk->buttons = xzalloc(gk->nbuttons * sizeof(*gk->buttons));

	for_each_child_of_node(np, npkey) {
		enum of_gpio_flags gpioflags;
		uint32_t keycode;

		gk->buttons[i].gpio = of_get_named_gpio_flags(npkey, "gpios", 0, &gpioflags);
		if (gk->buttons[i].gpio < 0)
			return gk->buttons[i].gpio;

		if (gpioflags & OF_GPIO_ACTIVE_LOW)
			gk->buttons[i].active_low = 1;

		ret = of_property_read_u32(npkey, "linux,code", &keycode);
		if (ret)
			return ret;

		gk->buttons[i].code = keycode;

		i++;
	}

	gk->use_keycodes = 1;

	return 0;
}

static int __init gpio_keys_probe(struct device_d *dev)
{
	int ret, i, gpio;
	struct console_device *cdev;
	struct gpio_keys *gk;

	gk = xzalloc(sizeof(*gk));
	gk->fifo_size = 50;

	if (dev->device_node)
		ret = gpio_keys_probe_dt(gk, dev);
	else
		ret = gpio_keys_probe_pdata(gk, dev);

	if (ret)
		return ret;

	gk->recv_fifo = kfifo_alloc(gk->fifo_size);

	for (i = 0; i < gk->nbuttons; i++) {
		gpio = gk->buttons[i].gpio;
		ret = gpio_request(gpio, "gpio_keys");
		if (ret) {
			pr_err("gpio_keys: (%d) can not be requested\n", gpio);
			return ret;
		}
		gpio_direction_input(gpio);
		gk->buttons[i].previous_state = gk->buttons[i].active_low;
	}

	gk->poller.func = gpio_key_poller;

	cdev = &gk->cdev;
	dev->type_data = cdev;
	cdev->dev = dev;
	cdev->tstc = gpio_keys_tstc;
	cdev->getc = gpio_keys_getc;

	console_register(&gk->cdev);

	return poller_register(&gk->poller);
}

static struct of_device_id key_gpio_of_ids[] = {
	{ .compatible = "gpio-keys", },
	{ }
};

static struct driver_d gpio_keys_driver = {
	.name	= "gpio_keys",
	.probe	= gpio_keys_probe,
	.of_compatible = DRV_OF_COMPAT(key_gpio_of_ids),
};
device_platform_driver(gpio_keys_driver);
