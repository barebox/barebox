// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2006 ARM Ltd.

/*
 * i2c-versatile.c
 *
 * written by Russell King, Deep Blue Solutions Ltd.
 */

#include <common.h>
#include <driver.h>
#include <i2c/i2c.h>
#include <i2c/i2c-algo-bit.h>
#include <init.h>
#include <malloc.h>
#include <io.h>
#include <linux/err.h>

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

static int i2c_versatile_probe(struct device *dev)
{
	struct resource *iores;
	struct i2c_versatile *i2c;
	int ret;

	i2c = kzalloc(sizeof(struct i2c_versatile), GFP_KERNEL);
	if (!i2c) {
		ret = -ENOMEM;
		goto err_release;
	}

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores)) {
		ret = PTR_ERR(iores);
		goto err_free;
	}
	i2c->base = IOMEM(iores->start);

	writel(SCL | SDA, i2c->base + I2C_CONTROLS);

	i2c->adap.algo_data = &i2c->algo;
	i2c->adap.dev.parent = dev;
	i2c->adap.dev.of_node = dev->of_node;
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

static struct of_device_id i2c_versatile_match[] = {
	{ .compatible = "arm,versatile-i2c", },
	{},
};
MODULE_DEVICE_TABLE(of, i2c_versatile_match);

static struct driver i2c_versatile_driver = {
	.name	= "versatile-i2c",
	.probe	= i2c_versatile_probe,
	.of_compatible = DRV_OF_COMPAT(i2c_versatile_match),
};
device_platform_driver(i2c_versatile_driver);
