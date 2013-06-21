#ifndef __MACH_IMX6_H
#define __MACH_IMX6_H

#include <io.h>
#include <mach/generic.h>
#include <mach/imx6-regs.h>

void imx6_init_lowlevel(void);

#define IMX6_ANATOP_SI_REV 0x260

#define IMX6_CPUTYPE_IMX6Q	0x63
#define IMX6_CPUTYPE_IMX6DL	0x61

static inline int imx6_cpu_type(void)
{
	uint32_t val;

	if (!cpu_is_mx6())
		return 0;

	val = readl(MX6_ANATOP_BASE_ADDR + IMX6_ANATOP_SI_REV);

	return (val >> 16) & 0xff;
}

static inline int cpu_is_mx6q(void)
{
	return imx6_cpu_type() == IMX6_CPUTYPE_IMX6Q;
}

static inline int cpu_is_mx6dl(void)
{
	return imx6_cpu_type() == IMX6_CPUTYPE_IMX6DL;
}

#endif /* __MACH_IMX6_H */
