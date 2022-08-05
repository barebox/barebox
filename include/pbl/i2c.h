/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __PBL_I2C_H
#define __PBL_I2C_H

#include <i2c/i2c.h>

struct pbl_i2c {
	int (*xfer)(struct pbl_i2c *, struct i2c_msg *msgs, int num);
};

static inline int pbl_i2c_xfer(struct pbl_i2c *i2c,
			       struct i2c_msg *msgs, int num)
{
	return i2c->xfer(i2c, msgs, num);
}

struct pbl_i2c *imx8m_i2c_early_init(void __iomem *regs);
struct pbl_i2c *ls1046_i2c_init(void __iomem *regs);

#endif /* __I2C_EARLY_H */
