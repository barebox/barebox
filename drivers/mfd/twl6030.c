/*
 * Copyright (C) 2011 Alexander Aring <a.aring@phytec.de>
 *
 * This file is released under the GPLv2
 */

#include <common.h>
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

	if (IS_ENABLED(CONFIG_DEBUG)) {
		u8 i, jtag_rev, eprom_rev;
		int r;
		u64 dieid;

		r = twl6030_reg_read(twl_dev, TWL6030_JTAG_JTAGVERNUM,
			&jtag_rev);
		r |= twl6030_reg_read(twl_dev, TWL6030_JTAG_EPROM_REV,
			&eprom_rev);
		for (i = 0; i < 8; i++)
			r |= twl6030_reg_read(twl_dev, TWL6030_DIEID_0+i,
				((u8 *)(&dieid))+i);
		if (r)
			dev_dbg(dev, "TWL6030 Error reading ID\n");
		else
			dev_dbg(dev, "TWL6030 JTAG REV: 0x%02X, "
				"EPROM REV: 0x%02X, "
				"DIE ID: 0x%016llX\n",
				(unsigned)jtag_rev, (unsigned)eprom_rev, dieid);
	}

	return 0;
}

static struct driver_d twl_driver = {
	.name  = DRIVERNAME,
	.probe = twl_probe,
};
device_i2c_driver(twl_driver);
