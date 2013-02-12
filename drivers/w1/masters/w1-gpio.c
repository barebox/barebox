/*
 * w1-gpio - GPIO w1 bus master driver
 *
 * Copyright (C) 2007 Ville Syrjala <syrjala@sci.fi>
 * Copyright (c) 2012 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 */

#include <common.h>
#include <init.h>
#include <malloc.h>
#include <xfuncs.h>
#include <driver.h>
#include <linux/w1-gpio.h>
#include <gpio.h>

#include "../w1.h"

static void w1_gpio_write_bit_dir(struct w1_bus *bus, u8 bit)
{
	struct w1_gpio_platform_data *pdata = bus->data;

	if (bit)
		gpio_direction_input(pdata->pin);
	else
		gpio_direction_output(pdata->pin, 0);
}

static void w1_gpio_write_bit_val(struct w1_bus *bus, u8 bit)
{
	struct w1_gpio_platform_data *pdata = bus->data;

	gpio_set_value(pdata->pin, bit);
}

static u8 w1_gpio_read_bit(struct w1_bus *bus)
{
	struct w1_gpio_platform_data *pdata = bus->data;

	return gpio_get_value(pdata->pin) ? 1 : 0;
}

static int __init w1_gpio_probe(struct device_d *dev)
{
	struct w1_bus *master;
	struct w1_gpio_platform_data *pdata;
	int err;

	pdata = dev->platform_data;

	if (!pdata)
		return -ENXIO;

	master = xzalloc(sizeof(struct w1_bus));

	err = gpio_request(pdata->pin, "w1");
	if (err)
		goto free_master;

	if (gpio_is_valid(pdata->ext_pullup_enable_pin)) {
		err = gpio_request(pdata->pin, "w1 pullup");
		if (err < 0)
			goto free_gpio;

		gpio_direction_output(pdata->pin, 0);
	}

	master->data = pdata;
	master->read_bit = w1_gpio_read_bit;

	if (pdata->is_open_drain) {
		gpio_direction_output(pdata->pin, 1);
		master->write_bit = w1_gpio_write_bit_val;
	} else {
		gpio_direction_input(pdata->pin);
		master->write_bit = w1_gpio_write_bit_dir;
	}

	master->parent = dev;

	err = w1_bus_register(master);
	if (err)
		goto free_gpio_ext_pu;

	if (pdata->enable_external_pullup)
		pdata->enable_external_pullup(1);

	if (gpio_is_valid(pdata->ext_pullup_enable_pin))
		gpio_set_value(pdata->ext_pullup_enable_pin, 1);

	return 0;

 free_gpio_ext_pu:
	if (gpio_is_valid(pdata->ext_pullup_enable_pin))
		gpio_free(pdata->ext_pullup_enable_pin);
 free_gpio:
	gpio_free(pdata->pin);
 free_master:
	kfree(master);

	return err;
}

static struct driver_d w1_gpio_driver = {
	.name	= "w1-gpio",
	.probe	= w1_gpio_probe,
};
device_platform_driver(w1_gpio_driver);
