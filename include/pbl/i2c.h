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
struct pbl_i2c *imx6_i2c_early_init(void __iomem *regs);
struct pbl_i2c *ls1046_i2c_init(void __iomem *regs);

static inline int i2c_dev_probe(struct pbl_i2c *i2c, int addr, bool onebyte)
{
	u8 buf[1];
	struct i2c_msg msgs[] = {
		{
			.addr = addr,
			.buf = buf,
			.flags = I2C_M_RD,
			.len = onebyte,
		},
	};

	return pbl_i2c_xfer(i2c, msgs, 1) == 1 ? 0 : -ENODEV;
}


#endif /* __PBL_I2C_H */
