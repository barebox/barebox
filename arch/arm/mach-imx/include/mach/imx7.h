#ifndef __MACH_IMX7_H
#define __MACH_IMX7_H

#include <io.h>
#include <mach/generic.h>
#include <mach/imx7-regs.h>
#include <mach/revision.h>

void imx7_init_lowlevel(void);

#define ANADIG_DIGPROG_IMX7	0x800

#define IMX7_CPUTYPE_IMX7S	0x71
#define IMX7_CPUTYPE_IMX7D	0x72

static inline int __imx7_cpu_type(void)
{
	void __iomem *ocotp = IOMEM(MX7_OCOTP_BASE_ADDR);

	if (readl(ocotp + 0x450) & 1)
		return IMX7_CPUTYPE_IMX7S;
	else
		return IMX7_CPUTYPE_IMX7D;
}

static inline int imx7_cpu_type(void)
{
	if (!cpu_is_mx7())
		return 0;

	return __imx7_cpu_type();
}

static inline int imx7_cpu_revision(void)
{
	if (!cpu_is_mx7())
		return IMX_CHIP_REV_UNKNOWN;

	/* register value has the format of the IMX_CHIP_REV_* macros */
	return readl(MX7_ANATOP_BASE_ADDR + ANADIG_DIGPROG_IMX7) & 0xff;
}

#define DEFINE_MX7_CPU_TYPE(str, type)					\
	static inline int cpu_mx7_is_##str(void)			\
	{								\
		return __imx7_cpu_type() == type;			\
	}								\
									\
	static inline int cpu_is_##str(void)				\
	{								\
		if (!cpu_is_mx7())					\
			return 0;					\
		return cpu_mx7_is_##str();				\
	}

DEFINE_MX7_CPU_TYPE(mx7s, IMX7_CPUTYPE_IMX7S);
DEFINE_MX7_CPU_TYPE(mx7d, IMX7_CPUTYPE_IMX7D);

#endif /* __MACH_IMX7_H */