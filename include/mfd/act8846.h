/*
 * Copyright (C) 2009 Marc Kleine-Budde <mkl@pengutronix.de>
 *
 * This file is released under the GPLv2
 *
 * Derived from mc9sdz60.h
 */

#ifndef _ACT8846_H
#define _ACT8846_H

enum act8846_reg {
	ACT8846_SYS_MODE	= 0x00,
	ACT8846_SYS_CTRL	= 0x01,
	ACT8846_DCDC1_VSET1	= 0x10,
	ACT8846_DCDC1_CTRL	= 0x12,
	ACT8846_DCDC2_VSET1	= 0x20,
	ACT8846_DCDC2_VSET2	= 0x21,
	ACT8846_DCDC2_CTRL	= 0x22,
	ACT8846_DCDC3_VSET1	= 0x30,
	ACT8846_DCDC3_VSET2	= 0x31,
	ACT8846_DCDC3_CTRL	= 0x32,
	ACT8846_DCDC4_VSET1	= 0x40,
	ACT8846_DCDC4_VSET2	= 0x41,
	ACT8846_DCDC4_CTRL	= 0x42,
	ACT8846_LDO5_VSET	= 0x50,
	ACT8846_LDO5_CTRL	= 0x51,
	ACT8846_LDO6_VSET	= 0x58,
	ACT8846_LDO6_CTRL	= 0x59,
	ACT8846_LDO7_VSET	= 0x60,
	ACT8846_LDO7_CTRL	= 0x61,
	ACT8846_LDO8_VSET	= 0x68,
	ACT8846_LDO8_CTRL	= 0x69,
	ACT8846_LDO9_VSET	= 0x70,
	ACT8846_LDO9_CTRL	= 0x71,
	ACT8846_LDO10_VSET	= 0x80,
	ACT8846_LDO10_CTRL	= 0x81,
	ACT8846_LDO11_VSET	= 0x90,
	ACT8846_LDO11_CTRL	= 0x91,
	ACT8846_LDO12_VSET	= 0xA0,
	ACT8846_LDO12_CTRL	= 0xA1,
};

struct act8846 {
	struct cdev		cdev;
	struct i2c_client	*client;
};

struct act8846 *act8846_get(void);

int act8846_reg_read(struct act8846 *priv, enum act8846_reg reg, u8 *val);
int act8846_reg_write(struct act8846 *priv, enum act8846_reg reg, u8 val);
int act8846_set_bits(struct act8846 *priv, enum act8846_reg reg,
		     u8 mask, u8 val);

#endif /* _ACT8846_H */
