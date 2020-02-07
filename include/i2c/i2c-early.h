#ifndef __I2C_EARLY_H
#define __I2C_EARLY_H

#include <i2c/i2c.h>

int i2c_fsl_xfer(void *ctx, struct i2c_msg *msgs, int num);

void *imx8m_i2c_early_init(void __iomem *regs);
void *ls1046_i2c_init(void __iomem *regs);

#endif /* __I2C_EARLY_H */
