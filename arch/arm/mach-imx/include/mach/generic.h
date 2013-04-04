#ifndef __MACH_GENERIC_H
#define __MACH_GENERIC_H

u64 imx_uid(void);

void imx25_boot_save_loc(void __iomem *ccm_base);
void imx35_boot_save_loc(void __iomem *ccm_base);
void imx27_boot_save_loc(void __iomem *sysctrl_base);
void imx51_boot_save_loc(void __iomem *src_base);
void imx53_boot_save_loc(void __iomem *src_base);
void imx6_boot_save_loc(void __iomem *src_base);

/* There's a off-by-one betweem the gpio bank number and the gpiochip */
/* range e.g. GPIO_1_5 is gpio 5 under linux */
#define IMX_GPIO_NR(bank, nr)		(((bank) - 1) * 32 + (nr))

#ifdef CONFIG_ARCH_IMX1
#define cpu_is_mx1()	(1)
#else
#define cpu_is_mx1()	(0)
#endif

#ifdef CONFIG_ARCH_IMX21
#define cpu_is_mx21()	(1)
#else
#define cpu_is_mx21()	(0)
#endif

#ifdef CONFIG_ARCH_IMX25
#define cpu_is_mx25()	(1)
#else
#define cpu_is_mx25()	(0)
#endif

#ifdef CONFIG_ARCH_IMX27
#define cpu_is_mx27()	(1)
#else
#define cpu_is_mx27()	(0)
#endif

#ifdef CONFIG_ARCH_IMX31
#define cpu_is_mx31()	(1)
#else
#define cpu_is_mx31()	(0)
#endif

#ifdef CONFIG_ARCH_IMX35
#define cpu_is_mx35()	(1)
#else
#define cpu_is_mx35()	(0)
#endif

#ifdef CONFIG_ARCH_IMX51
#define cpu_is_mx51()	(1)
#else
#define cpu_is_mx51()	(0)
#endif

#ifdef CONFIG_ARCH_IMX53
#define cpu_is_mx53()	(1)
#else
#define cpu_is_mx53()	(0)
#endif

#ifdef CONFIG_ARCH_IMX6
#define cpu_is_mx6()	(1)
#else
#define cpu_is_mx6()	(0)
#endif

#define cpu_is_mx23()	(0)
#define cpu_is_mx28()	(0)

#endif /* __MACH_GENERIC_H */
