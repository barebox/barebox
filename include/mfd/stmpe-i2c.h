/*
 * Copyright (C) 2012 Pengutronix
 * Steffen Trumtrar <s.trumtrar@pengutronix.de>
 *
 * This file is released under the GPLv2
 *
 */

#ifndef __ASM_ARCH_STMPE_H
#define __ASM_ARCH_STMPE_H

enum stmpe_revision {
	STMPE610,
	STMPE801,
	STMPE811,
	STMPE1601,
	STMPE2401,
	STMPE2403,
	STMPE_NBR_PARTS
};

enum stmpe_blocks {
	STMPE_BLOCK_GPIO	= 1 << 0,
	STMPE_BLOCK_KEYPAD      = 1 << 1,
	STMPE_BLOCK_TOUCHSCREEN = 1 << 2,
	STMPE_BLOCK_ADC         = 1 << 3,
	STMPE_BLOCK_PWM         = 1 << 4,
	STMPE_BLOCK_ROTATOR     = 1 << 5,
};

struct stmpe_platform_data {
	enum	stmpe_revision	revision;
	enum	stmpe_blocks	blocks;
	int			gpio_base;
};

struct stmpe {
	struct cdev		cdev;
	struct i2c_client	*client;
	struct stmpe_platform_data *pdata;
};

struct stmpe_client_info {
	struct stmpe *stmpe;
	int (*read_reg)(struct stmpe *stmpe, u32 reg, u8 *val);
	int (*write_reg)(struct stmpe *stmpe, u32 reg, u8 val);
};

int stmpe_reg_read(struct stmpe *priv, u32 reg, u8 *val);
int stmpe_reg_write(struct stmpe *priv, u32 reg, u8 val);
int stmpe_set_bits(struct stmpe *priv, u32 reg, u8 mask, u8 val);

#endif /* __ASM_ARCH_STMPE_H */
