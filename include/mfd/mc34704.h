/*
 * Copyright (C) 2009 Marc Kleine-Budde <mkl@pengutronix.de>
 * Copyright (C) 2010 Baruch Siach <baruch@tkos.co.il>
 *
 * This file is released under the GPLv2
 *
 * Derived from:
 * - arch-mxc/pmic_external.h --  contains interface of the PMIC protocol driver
 *   Copyright 2008-2009 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 */

#ifndef __MFD_MC34704_H
#define __MFD_MC34704_H

struct mc34704 {
	struct cdev		cdev;
	struct i2c_client	*client;
};

extern struct mc34704 *mc34704_get(void);

extern int mc34704_reg_read(struct mc34704 *mc34704, u8 reg, u8 *val);
extern int mc34704_reg_write(struct mc34704 *mc34704, u8 reg, u8 val);

#endif /* __MFD_MC34704_H */
