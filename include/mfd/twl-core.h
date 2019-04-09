/*
 * Copyright (C) 2011 Alexander Aring <a.aring@phytec.de>
 *
 * Based on:
 * Copyright (C) 2010 Michael Grzeschik <mgr@pengutronix.de>
 * Copyright (C) 2010 Sascha Hauer <sha@pengutronix.de>
 *
 * This file is released under the GPLv2
 *
 */

#ifndef __MFD_TWLCORE_H__
#define __MFD_TWLCORE_H__

#include <common.h>
#include <i2c/i2c.h>
#include <linux/err.h>

struct twlcore {
	struct cdev		cdev;
	struct i2c_client	*client;
};

extern struct cdev_operations twl_fops;

int twlcore_reg_read(struct twlcore *twlcore, u16 reg, u8 *val);
int twlcore_reg_write(struct twlcore *twlcore, u16 reg, u8 val);
int twlcore_set_bits(struct twlcore *twlcore, u16 reg, u8 mask, u8 val);
struct twlcore *twlcore_get(void);

#endif /* __MFD_TWLCORE_H__ */
