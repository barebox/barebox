/*
 * Copyright (C) 2015 Pengutronix, Philipp Zabel <p.zabel@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <driver.h>
#include <restart.h>
#include <init.h>
#include <i2c/i2c.h>
#include <malloc.h>
#include <notifier.h>
#include <reset_source.h>
#include <watchdog.h>

struct da9063 {
	struct restart_handler	restart;
	struct watchdog		wd;
	struct i2c_client	*client;
	struct device_d		*dev;
};

/* System Control and Event Registers */
#define DA9063_REG_FAULT_LOG	0x05
#define DA9063_REG_CONTROL_D	0x11
#define DA9063_REG_CONTROL_F	0x13

/* DA9063_REG_FAULT_LOG (addr=0x05) */
#define DA9063_TWD_ERROR	0x01
#define DA9063_POR		0x02
#define DA9063_NSHUTDOWN	0x40

/* DA9063_REG_CONTROL_D (addr=0x11) */
#define DA9063_TWDSCALE_MASK	0x07

/* DA9063_REG_CONTROL_F (addr=0x13) */
#define DA9063_SHUTDOWN		0x02

static int da9063_watchdog_set_timeout(struct watchdog *wd, unsigned timeout)
{
	struct da9063 *priv = container_of(wd, struct da9063, wd);
	struct device_d *dev = priv->dev;
	unsigned int scale = 0;
	int ret;
	u8 val;

	if (timeout > 131)
		return -EINVAL;

	if (timeout) {
		timeout *= 1000; /* convert to ms */
		scale = 0;
		while (timeout > (2048 << scale) && scale <= 6)
			scale++;
		dev_dbg(dev, "calculated TWDSCALE=%u (req=%ims calc=%ims)\n",
				scale, timeout, 2048 << scale);
		scale++; /* scale 0 disables the WD */
	}

	ret = i2c_read_reg(priv->client, DA9063_REG_CONTROL_D, &val, 1);
	if (ret < 0)
		return ret;

	val &= ~DA9063_TWDSCALE_MASK;
	val |= scale;

	ret = i2c_write_reg(priv->client, DA9063_REG_CONTROL_D, &val, 1);
	if (ret < 0)
		return ret;

	return 0;
}

static void da9063_detect_reset_source(struct da9063 *priv)
{
	int ret;
	u8 val;
	unsigned int priority;
	enum reset_src_type type;

	ret = i2c_read_reg(priv->client, DA9063_REG_FAULT_LOG, &val, 1);
	if (ret < 0)
		return;

	/* Write one to clear */
	i2c_write_reg(priv->client, DA9063_REG_FAULT_LOG, &val, 1);

	if (val & DA9063_TWD_ERROR)
		type = RESET_WDG;
	else if (val & DA9063_POR)
		type = RESET_POR;
	else if (val & DA9063_NSHUTDOWN)
		type = RESET_RST;
	else
		return;

	priority = of_get_reset_source_priority(priv->dev->device_node);

	reset_source_set_priority(type, priority);
}

static void da9063_restart(struct restart_handler *rst)
{
	struct da9063 *priv = container_of(rst, struct da9063, restart);

	u8 val = DA9063_SHUTDOWN;

	i2c_write_reg(priv->client, DA9063_REG_CONTROL_F, &val, 1);
}

static int da9063_probe(struct device_d *dev)
{
	struct da9063 *priv = NULL;
	int ret;

	priv = xzalloc(sizeof(struct da9063));
	priv->wd.priority = of_get_watchdog_priority(dev->device_node);
	priv->wd.set_timeout = da9063_watchdog_set_timeout;
	priv->wd.hwdev = dev;
	priv->client = to_i2c_client(dev);
	priv->dev = dev;

	ret = watchdog_register(&priv->wd);
	if (ret)
		goto on_error;

	da9063_detect_reset_source(priv);

	priv->restart.priority = of_get_restart_priority(dev->device_node);
	priv->restart.name = "da9063";
	priv->restart.restart = &da9063_restart;

	restart_handler_register(&priv->restart);

	return 0;

on_error:
	if (priv)
		free(priv);
	return ret;
}

static struct platform_device_id da9063_id[] = {
        { "da9063", },
	{ }
};

static struct driver_d da9063_driver = {
	.name = "da9063",
	.probe = da9063_probe,
	.id_table = da9063_id,
};

static int da9063_init(void)
{
	return i2c_driver_register(&da9063_driver);
}

device_initcall(da9063_init);
