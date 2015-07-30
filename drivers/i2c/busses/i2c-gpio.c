/*
 * Bitbanging I2C bus driver using the GPIO API
 *
 * Copyright (C) 2007 Atmel Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <common.h>
#include <driver.h>
#include <malloc.h>
#include <i2c/i2c.h>
#include <i2c/i2c-algo-bit.h>
#include <i2c/i2c-gpio.h>
#include <init.h>
#include <gpio.h>
#include <of_gpio.h>

struct i2c_gpio_private_data {
	struct i2c_adapter adap;
	struct i2c_algo_bit_data bit_data;
	struct i2c_gpio_platform_data pdata;
};

/* Toggle SDA by changing the direction of the pin */
static void i2c_gpio_setsda_dir(void *data, int state)
{
	struct i2c_gpio_platform_data *pdata = data;

	if (state)
		gpio_direction_input(pdata->sda_pin);
	else
		gpio_direction_output(pdata->sda_pin, 0);
}

/*
 * Toggle SDA by changing the output value of the pin. This is only
 * valid for pins configured as open drain (i.e. setting the value
 * high effectively turns off the output driver.)
 */
static void i2c_gpio_setsda_val(void *data, int state)
{
	struct i2c_gpio_platform_data *pdata = data;

	gpio_set_value(pdata->sda_pin, state);
}

/* Toggle SCL by changing the direction of the pin. */
static void i2c_gpio_setscl_dir(void *data, int state)
{
	struct i2c_gpio_platform_data *pdata = data;

	if (state)
		gpio_direction_input(pdata->scl_pin);
	else
		gpio_direction_output(pdata->scl_pin, 0);
}

/*
 * Toggle SCL by changing the output value of the pin. This is used
 * for pins that are configured as open drain and for output-only
 * pins. The latter case will break the i2c protocol, but it will
 * often work in practice.
 */
static void i2c_gpio_setscl_val(void *data, int state)
{
	struct i2c_gpio_platform_data *pdata = data;

	gpio_set_value(pdata->scl_pin, state);
}

static int i2c_gpio_getsda(void *data)
{
	struct i2c_gpio_platform_data *pdata = data;

	return gpio_get_value(pdata->sda_pin);
}

static int i2c_gpio_getscl(void *data)
{
	struct i2c_gpio_platform_data *pdata = data;

	return gpio_get_value(pdata->scl_pin);
}

static int of_i2c_gpio_probe(struct device_node *np,
			     struct i2c_gpio_platform_data *pdata)
{
	u32 reg;

	if (!IS_ENABLED(CONFIG_OFDEVICE))
		return -ENODEV;

	if (of_gpio_count(np) < 2)
		return -ENODEV;

	pdata->sda_pin = of_get_gpio(np, 0);
	pdata->scl_pin = of_get_gpio(np, 1);

	if (!gpio_is_valid(pdata->sda_pin) || !gpio_is_valid(pdata->scl_pin)) {
		pr_err("%s: invalid GPIO pins, sda=%d/scl=%d\n",
		       np->full_name, pdata->sda_pin, pdata->scl_pin);
		return -ENODEV;
	}

	of_property_read_u32(np, "i2c-gpio,delay-us", &pdata->udelay);

	if (!of_property_read_u32(np, "i2c-gpio,timeout-ms", &reg))
		pdata->timeout_ms = reg;

	pdata->sda_is_open_drain =
		of_property_read_bool(np, "i2c-gpio,sda-open-drain");
	pdata->scl_is_open_drain =
		of_property_read_bool(np, "i2c-gpio,scl-open-drain");
	pdata->scl_is_output_only =
		of_property_read_bool(np, "i2c-gpio,scl-output-only");

	return 0;
}

static int i2c_gpio_probe(struct device_d *dev)
{
	struct i2c_gpio_private_data *priv;
	struct i2c_gpio_platform_data *pdata;
	struct i2c_algo_bit_data *bit_data;
	struct i2c_adapter *adap;
	int ret;

	priv = xzalloc(sizeof(*priv));

	adap = &priv->adap;
	bit_data = &priv->bit_data;
	pdata = &priv->pdata;

	if (dev->device_node) {
		ret = of_i2c_gpio_probe(dev->device_node, pdata);
		if (ret)
			return ret;
	} else {
		if (!dev->platform_data)
			return -ENXIO;
		memcpy(pdata, dev->platform_data, sizeof(*pdata));
	}

	ret = gpio_request(pdata->sda_pin, "sda");
	if (ret)
		goto err_request_sda;
	ret = gpio_request(pdata->scl_pin, "scl");
	if (ret)
		goto err_request_scl;

	if (pdata->sda_is_open_drain) {
		gpio_direction_output(pdata->sda_pin, 1);
		bit_data->setsda = i2c_gpio_setsda_val;
	} else {
		gpio_direction_input(pdata->sda_pin);
		bit_data->setsda = i2c_gpio_setsda_dir;
	}

	if (pdata->scl_is_open_drain || pdata->scl_is_output_only) {
		gpio_direction_output(pdata->scl_pin, 1);
		bit_data->setscl = i2c_gpio_setscl_val;
	} else {
		gpio_direction_input(pdata->scl_pin);
		bit_data->setscl = i2c_gpio_setscl_dir;
	}

	if (!pdata->scl_is_output_only)
		bit_data->getscl = i2c_gpio_getscl;
	bit_data->getsda = i2c_gpio_getsda;

	if (pdata->udelay)
		bit_data->udelay = pdata->udelay;
	else if (pdata->scl_is_output_only)
		bit_data->udelay = 50;			/* 10 kHz */
	else
		bit_data->udelay = 5;			/* 100 kHz */

	if (pdata->timeout_ms)
		bit_data->timeout_ms = pdata->timeout_ms;
	else
		bit_data->timeout_ms = 100;		/* 100 ms */

	bit_data->data = pdata;

	adap->algo_data = bit_data;
	adap->dev.parent = dev;
	adap->dev.device_node = dev->device_node;
	adap->bus_recovery_info = xzalloc(sizeof(*adap->bus_recovery_info));
	adap->bus_recovery_info->scl_gpio = pdata->scl_pin;
	adap->bus_recovery_info->sda_gpio = pdata->sda_pin;
	adap->bus_recovery_info->get_sda = i2c_get_sda_gpio_value;
	adap->bus_recovery_info->get_scl = i2c_get_scl_gpio_value;
	adap->bus_recovery_info->set_scl = i2c_set_scl_gpio_value;
	adap->bus_recovery_info->recover_bus = i2c_generic_scl_recovery;

	adap->nr = dev->id;
	ret = i2c_bit_add_numbered_bus(adap);
	if (ret)
		goto err_add_bus;

	dev_info(dev, "using pins %u (SDA) and %u (SCL%s)\n",
		 pdata->sda_pin, pdata->scl_pin,
		 pdata->scl_is_output_only
		 ? ", no clock stretching" : "");

	return 0;

err_add_bus:
	free(adap->bus_recovery_info);
	gpio_free(pdata->scl_pin);
err_request_scl:
	gpio_free(pdata->sda_pin);
err_request_sda:
	free(priv);
	return ret;
}

static struct of_device_id i2c_gpio_dt_ids[] = {
	{ .compatible = "i2c-gpio", },
	{ /* sentinel */ }
};

static struct driver_d i2c_gpio_driver = {
	.name	= "i2c-gpio",
	.probe	= i2c_gpio_probe,
	.of_compatible = DRV_OF_COMPAT(i2c_gpio_dt_ids),
};
device_platform_driver(i2c_gpio_driver);
