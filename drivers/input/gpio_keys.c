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
	return code;
}

static int __init gpio_keys_probe(struct device_d *dev)
{
	int ret, i, gpio;
	struct gpio_keys_platform_data *pdata;
	struct console_device *cdev;
	struct gpio_keys *gk;

	pdata = dev->platform_data;

	if (!pdata) {
		/* small (so we copy it) but critical! */
		pr_err("missing platform_data\n");
		return -ENODEV;
	}

	gk = xzalloc(sizeof(*gk));

	gk->fifo_size = 50;

	if (pdata->fifo_size)
		gk->fifo_size = pdata->fifo_size;

	gk->recv_fifo = kfifo_alloc(pdata->fifo_size);

	gk->buttons = xzalloc(pdata->nbuttons * sizeof(*gk->buttons));
	gk->nbuttons = pdata->nbuttons;

	for (i = 0; i < gk->nbuttons; i++) {
		gk->buttons[i].gpio = pdata->buttons[i].gpio;
		gk->buttons[i].code = pdata->buttons[i].code;
		gk->buttons[i].active_low = pdata->buttons[i].active_low;
	}

	for (i = 0; i < pdata->nbuttons; i++) {
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

static struct driver_d gpio_keys_driver = {
	.name	= "gpio_keys",
	.probe	= gpio_keys_probe,
};
device_platform_driver(gpio_keys_driver);
