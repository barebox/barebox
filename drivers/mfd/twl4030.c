/*
 * Copyright (C) 2010 Michael Grzeschik <mgr@pengutronix.de>
 *
 * This file is released under the GPLv2
 *
 */

#include <common.h>
#include <driver.h>
#include <xfuncs.h>
#include <errno.h>

#include <i2c/i2c.h>
#include <mfd/twl4030.h>

#define DRIVERNAME		"twl4030"

#define to_twl4030(a)		container_of(a, struct twl4030, cdev)

static struct twl4030 *twl_dev;

struct twl4030 *twl4030_get(void)
{
	if (!twl_dev)
		return NULL;

	return twl_dev;
}
EXPORT_SYMBOL(twl4030_get);

static int twl_probe(struct device_d *dev)
{
	if (twl_dev)
		return -EBUSY;

	twl_dev = xzalloc(sizeof(struct twl4030));
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
device_i2c_driver(twl_driver);
