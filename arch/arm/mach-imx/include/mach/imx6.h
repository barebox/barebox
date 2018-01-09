#ifndef __MACH_IMX6_H
#define __MACH_IMX6_H

#include <io.h>
#include <mach/generic.h>
#include <mach/imx6-regs.h>
#include <mach/revision.h>

void __noreturn imx6_pm_stby_poweroff(void);

#define IMX6_ANATOP_SI_REV 0x260
#define IMX6SL_ANATOP_SI_REV 0x280

#define IMX6_CPUTYPE_IMX6SL	0x160
#define IMX6_CPUTYPE_IMX6S	0x161
#define IMX6_CPUTYPE_IMX6DL	0x261
#define IMX6_CPUTYPE_IMX6SX	0x462
#define IMX6_CPUTYPE_IMX6D	0x263
#define IMX6_CPUTYPE_IMX6Q	0x463
#define IMX6_CPUTYPE_IMX6UL	0x164
#define IMX6_CPUTYPE_IMX6ULL	0x165

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
	/* non-MX6-standard SI_REV reg offset for MX6SL */
	if (IS_ENABLED(CONFIG_ARCH_IMX6SL) &&
	    val < (IMX6_CPUTYPE_IMX6S & 0xff)) {
		uint32_t tmp;
		tmp = readl(MX6_ANATOP_BASE_ADDR + IMX6SL_ANATOP_SI_REV);
		tmp = (tmp >> 16) & 0xff;
		if ((IMX6_CPUTYPE_IMX6SL & 0xff) == tmp)
			/* intentionally skip scu_get_core_count() for MX6SL */
			return IMX6_CPUTYPE_IMX6SL;
	}

	val |= scu_get_core_count() << 8;

	return val;
}

static inline int imx6_cpu_type(void)
{
	if (!cpu_is_mx6())
		return 0;

	return __imx6_cpu_type();
}

#define DEFINE_MX6_CPU_TYPE(str, type)					\
	static inline int cpu_mx6_is_##str(void)			\
	{								\
		return __imx6_cpu_type() == type;			\
	}								\
									\
	static inline int cpu_is_##str(void)				\
	{								\
		if (!cpu_is_mx6())					\
			return 0;					\
		return cpu_mx6_is_##str();				\
	}

DEFINE_MX6_CPU_TYPE(mx6s, IMX6_CPUTYPE_IMX6S);
DEFINE_MX6_CPU_TYPE(mx6dl, IMX6_CPUTYPE_IMX6DL);
DEFINE_MX6_CPU_TYPE(mx6q, IMX6_CPUTYPE_IMX6Q);
DEFINE_MX6_CPU_TYPE(mx6d, IMX6_CPUTYPE_IMX6D);
DEFINE_MX6_CPU_TYPE(mx6sx, IMX6_CPUTYPE_IMX6SX);
DEFINE_MX6_CPU_TYPE(mx6sl, IMX6_CPUTYPE_IMX6SL);
DEFINE_MX6_CPU_TYPE(mx6ul, IMX6_CPUTYPE_IMX6UL);
DEFINE_MX6_CPU_TYPE(mx6ull, IMX6_CPUTYPE_IMX6ULL);

static inline int __imx6_cpu_revision(void)
{

	uint32_t rev;
	uint32_t si_rev_offset = IMX6_ANATOP_SI_REV;

	if (IS_ENABLED(CONFIG_ARCH_IMX6SL) && cpu_mx6_is_mx6sl())
		si_rev_offset = IMX6SL_ANATOP_SI_REV;

	rev = readl(MX6_ANATOP_BASE_ADDR + si_rev_offset);

	switch (rev & 0xfff) {
	case 0x00:
		return IMX_CHIP_REV_1_0;
	case 0x01:
		return IMX_CHIP_REV_1_1;
	case 0x02:
		return IMX_CHIP_REV_1_2;
	case 0x03:
		return IMX_CHIP_REV_1_3;
	case 0x04:
		return IMX_CHIP_REV_1_4;
	case 0x05:
		return IMX_CHIP_REV_1_5;
	case 0x06:
		return IMX_CHIP_REV_1_6;
	case 0x100:
		return IMX_CHIP_REV_2_0;
	}

	return IMX_CHIP_REV_UNKNOWN;
}

static inline int imx6_cpu_revision(void)
{
	if (!cpu_is_mx6())
		return 0;

	return __imx6_cpu_revision();
}

#endif /* __MACH_IMX6_H */
