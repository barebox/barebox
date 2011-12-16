#ifndef __MACH_MX5_H
#define __MACH_MX5_H

void imx53_init_lowlevel(void);
void imx5_setup_pll(void __iomem *base, int freq, u32 op, u32 mfd, u32 mfn);
void imx5_init_lowlevel(void);

#endif /* __MACH_MX53_H */
