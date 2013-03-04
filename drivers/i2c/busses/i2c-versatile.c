/*
 *  i2c-versatile.c
 *
 *  Copyright (C) 2006 ARM Ltd.
 *  written by Russell King, Deep Blue Solutions Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <common.h>
#include <driver.h>
#include <i2c/i2c.h>
#include <i2c/i2c-algo-bit.h>
#include <init.h>
#include <malloc.h>
#include <io.h>

#define I2C_CONTROL	0x00
#define I2C_CONTROLS	0x00
#define I2C_CONTROLC	0x04
#define SCL		(1 << 0)
#define SDA		(1 << 1)

struct i2c_versatile {
	struct i2c_adapter	 adap;
	struct i2c_algo_bit_data algo;
	void __iomem		 *base;
};

static void i2c_versatile_setsda(void *data, int state)
{
	struct i2c_versatile *i2c = data;

	writel(SDA, i2c->base + (state ? I2C_CONTROLS : I2C_CONTROLC));
}

static void i2c_versatile_setscl(void *data, int state)
{
	struct i2c_versatile *i2c = data;

	writel(SCL, i2c->base + (state ? I2C_CONTROLS : I2C_CONTROLC));
}

static int i2c_versatile_getsda(void *data)
{
	struct i2c_versatile *i2c = data;
	return !!(readl(i2c->base + I2C_CONTROL) & SDA);
}

static int i2c_versatile_getscl(void *data)
{
	struct i2c_versatile *i2c = data;
	return !!(readl(i2c->base + I2C_CONTROL) & SCL);
}

static struct i2c_algo_bit_data i2c_versatile_algo = {
	.setsda	= i2c_versatile_setsda,
	.setscl = i2c_versatile_setscl,
	.getsda	= i2c_versatile_getsda,
	.getscl = i2c_versatile_getscl,
	.udelay	= 30,
	.timeout_ms = 100,
};

static int i2c_versatile_probe(struct device_d *dev)
{
	struct i2c_versatile *i2c;
	int ret;

	i2c = kzalloc(sizeof(struct i2c_versatile), GFP_KERNEL);
	if (!i2c) {
		ret = -ENOMEM;
		goto err_release;
	}

	i2c->base = dev_request_mem_region(dev, 0);
	if (!i2c->base) {
		ret = -ENOMEM;
		goto err_free;
	}

	writel(SCL | SDA, i2c->base + I2C_CONTROLS);

	i2c->adap.algo_data = &i2c->algo;
	i2c->adap.dev.parent = dev;
	i2c->algo = i2c_versatile_algo;
	i2c->algo.data = i2c;

	i2c->adap.nr = dev->id;
	ret = i2c_bit_add_numbered_bus(&i2c->adap);
	if (ret >= 0) {
		return 0;
	}

 err_free:
	kfree(i2c);
 err_release:
	return ret;
}

static struct driver_d i2c_versatile_driver = {
	.name	= "versatile-i2c",
	.probe	= i2c_versatile_probe,
};
device_platform_driver(i2c_versatile_driver);
