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

static inline struct gpio_keys_platform_data *
poller_to_gk_pdata(struct poller_struct *poller)
{
	return container_of(poller, struct gpio_keys_platform_data, poller);
}

static inline struct gpio_keys_platform_data *
cdev_to_gk_pdata(struct console_device *cdev)
{
	return container_of(cdev, struct gpio_keys_platform_data, cdev);
}

static void gpio_key_poller(struct poller_struct *poller)
{
	struct gpio_keys_platform_data *pdata = poller_to_gk_pdata(poller);
	struct gpio_keys_button *gb;
	int i, val;

	for (i = 0; i < pdata->nbuttons; i++) {

		gb = &pdata->buttons[i];
		val = gpio_get_value(gb->gpio);

		if (val != gb->previous_state && val != gb->active_low) {
			kfifo_put(pdata->recv_fifo, (u_char*)&gb->code, sizeof(int));
			debug("pressed gpio(%d) as %d\n", gb->gpio, gb->code);
		}
		gb->previous_state = val;
	}
}

static int gpio_keys_tstc(struct console_device *cdev)
{
	struct gpio_keys_platform_data *pdata = cdev_to_gk_pdata(cdev);

	return (kfifo_len(pdata->recv_fifo) == 0) ? 0 : 1;
}

static int gpio_keys_getc(struct console_device *cdev)
{
	int code = 0;
	struct gpio_keys_platform_data *pdata = cdev_to_gk_pdata(cdev);

	kfifo_get(pdata->recv_fifo, (u_char*)&code, sizeof(int));
	return code;
}

static int __init gpio_keys_probe(struct device_d *dev)
{
	int ret, i, gpio;
	struct gpio_keys_platform_data *pdata;
	struct console_device *cdev;

	pdata = dev->platform_data;

	if (!pdata) {
		/* small (so we copy it) but critical! */
		pr_err("missing platform_data\n");
		return -ENODEV;
	}

	if (!pdata->fifo_size)
		pdata->fifo_size = 50;

	pdata->recv_fifo = kfifo_alloc(pdata->fifo_size);

	for (i = 0; i < pdata->nbuttons; i++) {
		gpio = pdata->buttons[i].gpio;
		ret = gpio_request(gpio, "gpio_keys");
		if (ret) {
			pr_err("gpio_keys: (%d) can not be requested\n", gpio);
			return ret;
		}
		gpio_direction_input(gpio);
		pdata->buttons[i].previous_state =
			pdata->buttons[i].active_low;
	}

	pdata->poller.func = gpio_key_poller;

	cdev = &pdata->cdev;
	dev->type_data = cdev;
	cdev->dev = dev;
	cdev->tstc = gpio_keys_tstc;
	cdev->getc = gpio_keys_getc;

	console_register(&pdata->cdev);

	return poller_register(&pdata->poller);
}

static struct driver_d gpio_keys_driver = {
	.name	= "gpio_keys",
	.probe	= gpio_keys_probe,
};
device_platform_driver(gpio_keys_driver);
