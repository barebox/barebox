#ifndef __MACH_IMX6_H
#define __MACH_IMX6_H

#include <io.h>
#include <mach/generic.h>
#include <mach/imx6-regs.h>

void imx6_init_lowlevel(void);

#define IMX6_ANATOP_SI_REV 0x260

#define IMX6_CPUTYPE_IMX6S	0x161
#define IMX6_CPUTYPE_IMX6DL	0x261
#define IMX6_CPUTYPE_IMX6SX	0x462
#define IMX6_CPUTYPE_IMX6D	0x263
#define IMX6_CPUTYPE_IMX6Q	0x463

#define SCU_CONFIG              0x04

static inline int scu_get_core_count(void)
{
	unsigned long base;
	unsigned int ncores;

	asm("mrc p15, 4, %0, c15, c0, 0" : "=r" (base));

	ncores = readl(base + SCU_CONFIG);
	return (ncores & 0x03) + 1;
}

static inline int __imx6_cpu_type(void)
{
	uint32_t val;

	val = readl(MX6_ANATOP_BASE_ADDR + IMX6_ANATOP_SI_REV);
	val = (val >> 16) & 0xff;

	val |= scu_get_core_count() << 8;

	return val;
}

static inline int imx6_cpu_type(void)
{
	if (!cpu_is_mx6())
		return 0;

	return __imx6_cpu_type();
}

static inline int cpu_is_mx6s(void)
{
	return imx6_cpu_type() == IMX6_CPUTYPE_IMX6S;
}

static inline int cpu_is_mx6dl(void)
{
	return imx6_cpu_type() == IMX6_CPUTYPE_IMX6DL;
}

static inline int cpu_is_mx6d(void)
{
	return imx6_cpu_type() == IMX6_CPUTYPE_IMX6D;
}

static inline int cpu_is_mx6q(void)
{
	return imx6_cpu_type() == IMX6_CPUTYPE_IMX6Q;
}

static inline int cpu_is_mx6sx(void)
{
	return imx6_cpu_type() == IMX6_CPUTYPE_IMX6SX;
}

#endif /* __MACH_IMX6_H */
