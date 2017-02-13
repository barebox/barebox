/*
 * Copyright (C) 2017 Sascha Hauer, Pengutronix
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
 *
 */

#include <common.h>
#include <init.h>
#include <driver.h>
#include <xfuncs.h>
#include <errno.h>
#include <malloc.h>
#include <of.h>
#include <regmap.h>
#include <mfd/pfuze.h>

#include <i2c/i2c.h>

#define DRIVERNAME		"pfuze"

#define MC13XXX_NUMREGS		0x3f

struct pfuze {
	struct device_d			*dev;
	struct regmap			*map;
	struct i2c_client		*client;
	int				revision;
};

struct pfuze_devtype {
	int	(*revision)(struct pfuze*);
};

#define to_pfuze(a)		container_of(a, struct pfuze, cdev)

static struct pfuze *pfuze_dev;

static void(*pfuze_init_callback)(struct regmap *map);

int pfuze_register_init_callback(void(*callback)(struct regmap *map))
{
	if (pfuze_init_callback)
		return -EBUSY;

	pfuze_init_callback = callback;

	if (pfuze_dev)
		pfuze_init_callback(pfuze_dev->map);

	return 0;
}

static int pfuze_i2c_reg_read(void *ctx, unsigned int reg, unsigned int *val)
{
	struct pfuze *pfuze = ctx;
	u8 buf[1];
	int ret;

	ret = i2c_read_reg(pfuze->client, reg, buf, 1);
	*val = buf[0];

	return ret == 1 ? 0 : ret;
}

static int pfuze_i2c_reg_write(void *ctx, unsigned int reg, unsigned int val)
{
	struct pfuze *pfuze = ctx;
	u8 buf[] = {
		val & 0xff,
	};
	int ret;

	ret = i2c_write_reg(pfuze->client, reg, buf, 1);

	return ret == 1 ? 0 : ret;
}

static struct regmap_bus regmap_pfuze_i2c_bus = {
	.reg_write = pfuze_i2c_reg_write,
	.reg_read = pfuze_i2c_reg_read,
};

static const struct regmap_config pfuze_regmap_i2c_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = 127,
};

static int __init pfuze_probe(struct device_d *dev)
{
	struct pfuze_devtype *devtype;
	int ret;

	if (pfuze_dev)
		return -EBUSY;

	ret = dev_get_drvdata(dev, (const void **)&devtype);
	if (ret)
		return ret;

	pfuze_dev = xzalloc(sizeof(*pfuze_dev));
	pfuze_dev->dev = dev;

	pfuze_dev->client = to_i2c_client(dev);
	pfuze_dev->map = regmap_init(dev, &regmap_pfuze_i2c_bus,
				     pfuze_dev, &pfuze_regmap_i2c_config);

	ret = regmap_register_cdev(pfuze_dev->map, NULL);
	if (ret)
		return ret;

	if (pfuze_init_callback)
		pfuze_init_callback(pfuze_dev->map);

	return 0;
}

static struct pfuze_devtype pfuze100_devtype = {
};

static struct pfuze_devtype pfuze200_devtype = {
};

static struct pfuze_devtype pfuze3000_devtype = {
};

static struct platform_device_id pfuze_ids[] = {
	{ .name = "pfuze100", .driver_data = (ulong)&pfuze100_devtype, },
	{ .name = "pfuze200", .driver_data = (ulong)&pfuze200_devtype, },
	{ .name = "pfuze3000", .driver_data = (ulong)&pfuze3000_devtype, },
	{ }
};

static __maybe_unused struct of_device_id pfuze_dt_ids[] = {
	{ .compatible = "fsl,pfuze100", .data = &pfuze100_devtype, },
	{ .compatible = "fsl,pfuze200", .data = &pfuze200_devtype, },
	{ .compatible = "fsl,pfuze3000", .data = &pfuze3000_devtype, },
	{ }
};

static struct driver_d pfuze_i2c_driver = {
	.name		= "pfuze-i2c",
	.probe		= pfuze_probe,
	.id_table	= pfuze_ids,
	.of_compatible	= DRV_OF_COMPAT(pfuze_dt_ids),
};

static int __init pfuze_init(void)
{
	int ret;

	ret = i2c_driver_register(&pfuze_i2c_driver);
	if (ret)
		return ret;

	return 0;

}
late_initcall(pfuze_init);
