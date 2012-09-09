/*
 * Copyright (C) 2011 Alexander Aring <a.aring@phytec.de>
 *
 * This file is released under the GPLv2
 */

#include <common.h>
#include <init.h>
#include <driver.h>
#include <xfuncs.h>
#include <errno.h>

#include <i2c/i2c.h>
#include <mfd/twl6030.h>

#define DRIVERNAME		"twl6030"

#define to_twl6030(a)		container_of(a, struct twl6030, cdev)

static struct twl6030 *twl_dev;

struct twl6030 *twl6030_get(void)
{
	return twl_dev;
}
EXPORT_SYMBOL(twl6030_get);

static int twl_probe(struct device_d *dev)
{
	if (twl_dev)
		return -EBUSY;

	twl_dev = xzalloc(sizeof(struct twl6030));
	twl_dev->core.cdev.name = DRIVERNAME;
	twl_dev->core.client = to_i2c_client(dev);
	twl_dev->core.cdev.size = 1024;
	twl_dev->core.cdev.dev = dev;
	twl_dev->core.cdev.ops = &twl_fops;

	devfs_create(&(twl_dev->core.cdev));

	return 0;
}

static struct driver_d twl_driver = {
	.name  = DRIVERNAME,
	.probe = twl_probe,
};

static int twl_init(void)
{
	i2c_register_driver(&twl_driver);
	return 0;
}

device_initcall(twl_init);
