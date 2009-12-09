/*
 * Copyright (C) 2009 Marc Kleine-Budde <mkl@pengutronix.de>
 *
 * This file is released under the GPLv2
 *
 * Derived from:
 * - mcu_max8660-bus.h --  contains interface of the mc9sdz60 and max8660
 *   Copyright 2004-2009 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 */

#ifndef __ASM_ARCH_MC9SDZ60_H
#define __ASM_ARCH_MC9SDZ60_H

enum mc9sdz60_reg {
	MC9SDZ60_REG_VERSION		= 0x00,
	MC9SDZ60_REG_SECS		= 0x01,
	MC9SDZ60_REG_MINS		= 0x02,
	MC9SDZ60_REG_HRS		= 0x03,
	MC9SDZ60_REG_DAY		= 0x04,
	MC9SDZ60_REG_DATE		= 0x05,
	MC9SDZ60_REG_MONTH		= 0x06,
	MC9SDZ60_REG_YEAR		= 0x07,
	MC9SDZ60_REG_ALARM_SECS		= 0x08,
	MC9SDZ60_REG_ALARM_MINS		= 0x09,
	MC9SDZ60_REG_ALARM_HRS		= 0x0a,
	MC9SDZ60_REG_TS_CONTROL		= 0x0b,
	MC9SDZ60_REG_X_LOW		= 0x0c,
	MC9SDZ60_REG_Y_LOW		= 0x0d,
	MC9SDZ60_REG_XY_HIGH		= 0x0e,
	MC9SDZ60_REG_X_LEFT_LOW		= 0x0f,
	MC9SDZ60_REG_X_LEFT_HIGH	= 0x10,
	MC9SDZ60_REG_X_RIGHT		= 0x11,
	MC9SDZ60_REG_Y_TOP_LOW		= 0x12,
	MC9SDZ60_REG_Y_TOP_HIGH		= 0x13,
	MC9SDZ60_REG_Y_BOTTOM		= 0x14,
	MC9SDZ60_REG_RESET_1		= 0x15,
	MC9SDZ60_REG_RESET_2		= 0x16,
	MC9SDZ60_REG_POWER_CTL		= 0x17,
	MC9SDZ60_REG_DELAY_CONFIG	= 0x18,
	MC9SDZ60_REG_GPIO_1		= 0x19,
	MC9SDZ60_REG_GPIO_2		= 0x1a,
	MC9SDZ60_REG_KPD_1		= 0x1b,
	MC9SDZ60_REG_KPD_2		= 0x1c,
	MC9SDZ60_REG_KPD_CONTROL	= 0x1d,
	MC9SDZ60_REG_INT_ENABLE_1	= 0x1e,
	MC9SDZ60_REG_INT_ENABLE_2	= 0x1f,
	MC9SDZ60_REG_INT_FLAG_1		= 0x20,
	MC9SDZ60_REG_INT_FLAG_2		= 0x21,
	MC9SDZ60_REG_DES_FLAG		= 0x22,
};

struct mc9sdz60 {
	struct cdev		cdev;
	struct i2c_client	*client;
};

extern struct mc9sdz60 *mc9sdz60_get(void);

extern int mc9sdz60_reg_read(struct mc9sdz60 *priv, enum mc9sdz60_reg reg, u8 *val);
extern int mc9sdz60_reg_write(struct mc9sdz60 *priv, enum mc9sdz60_reg reg, u8 val);
extern int mc9sdz60_set_bits(struct mc9sdz60 *priv, enum mc9sdz60_reg reg, u8 mask, u8 val);

#endif /* __ASM_ARCH_MC9SDZ60_H */
