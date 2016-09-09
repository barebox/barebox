#ifndef __MACH_MX5_H
#define __MACH_MX5_H

void imx50_init_lowlevel(unsigned int cpufreq_mhz);
void imx51_init_lowlevel(unsigned int cpufreq_mhz);
void imx53_init_lowlevel(unsigned int cpufreq_mhz);
void imx53_init_lowlevel_early(unsigned int cpufreq_mhz);
void imx5_init_lowlevel(void);

void imx5_setup_pll(void __iomem *base, int freq, u32 op, u32 mfd, u32 mfn);

#define imx5_setup_pll_1000(base)	imx5_setup_pll((base), 1000, ((10 << 4) + ((1 - 1) << 0)), (12 - 1), 5)
#define imx5_setup_pll_864(base)	imx5_setup_pll((base),  864, (( 8 << 4) + ((1 - 1) << 0)), (180 - 1), 180)
#define imx5_setup_pll_800(base)	imx5_setup_pll((base),  800, (( 8 << 4) + ((1 - 1) << 0)), (3 - 1), 1)
#define imx5_setup_pll_665(base)	imx5_setup_pll((base),  665, (( 6 << 4) + ((1 - 1) << 0)), (96 - 1), 89)
#define imx5_setup_pll_600(base)	imx5_setup_pll((base),  600, (( 6 << 4) + ((1 - 1) << 0)), ( 4 - 1),  1)
#define imx5_setup_pll_455(base)	imx5_setup_pll((base),  455, (( 9 << 4) + ((2 - 1) << 0)), (48 - 1), 23)
#define imx5_setup_pll_400(base)	imx5_setup_pll((base),  400, (( 8 << 4) + ((2 - 1) << 0)), (3 - 1), 1)
#define imx5_setup_pll_216(base)	imx5_setup_pll((base),  216, (( 6 << 4) + ((3 - 1) << 0)), (4 - 1), 3)

#endif /* __MACH_MX53_H */
