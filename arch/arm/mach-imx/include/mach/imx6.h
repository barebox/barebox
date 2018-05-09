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
#define IMX6_CPUTYPE_IMX6DP	0x1263
#define IMX6_CPUTYPE_IMX6Q	0x463
#define IMX6_CPUTYPE_IMX6QP	0x1463
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

#define SI_REV_CPUTYPE(s)	(((s) >> 16) & 0xff)
#define SI_REV_MAJOR(s)		(((s) >> 8) & 0xf)
#define SI_REV_MINOR(s)		((s) & 0xf)

static inline uint32_t __imx6_read_si_rev(void)
{
	uint32_t si_rev;
	uint32_t cpu_type;

	si_rev = readl(MX6_ANATOP_BASE_ADDR + IMX6_ANATOP_SI_REV);
	cpu_type = SI_REV_CPUTYPE(si_rev);

	if (cpu_type >= 0x61 && cpu_type <= 0x65)
		return si_rev;

	/* try non-MX6-standard SI_REV reg offset for MX6SL */
	si_rev = readl(MX6_ANATOP_BASE_ADDR + IMX6SL_ANATOP_SI_REV);
	cpu_type = SI_REV_CPUTYPE(si_rev);

	if (si_rev == 0x60)
		return si_rev;

	return 0;
}

static inline int __imx6_cpu_type(void)
{
	uint32_t si_rev = __imx6_read_si_rev();
	uint32_t cpu_type = SI_REV_CPUTYPE(si_rev);

	/* intentionally skip scu_get_core_count() for MX6SL */
	if (cpu_type == IMX6_CPUTYPE_IMX6SL)
		return IMX6_CPUTYPE_IMX6SL;

	cpu_type |= scu_get_core_count() << 8;

	if ((cpu_type == IMX6_CPUTYPE_IMX6D || cpu_type == IMX6_CPUTYPE_IMX6Q) &&
	    SI_REV_MAJOR(si_rev) >= 1)
		cpu_type |= 0x1000;

	return cpu_type;
}

int imx6_cpu_type(void);

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

/*
 * Below are defined:
 *
 * cpu_is_mx6s(), cpu_is_mx6dl(), cpu_is_mx6q(), cpu_is_mx6qp(), cpu_is_mx6d(),
 * cpu_is_mx6dp(), cpu_is_mx6sx(), cpu_is_mx6sl(), cpu_is_mx6ul(),
 * cpu_is_mx6ull()
 */
DEFINE_MX6_CPU_TYPE(mx6s, IMX6_CPUTYPE_IMX6S);
DEFINE_MX6_CPU_TYPE(mx6dl, IMX6_CPUTYPE_IMX6DL);
DEFINE_MX6_CPU_TYPE(mx6q, IMX6_CPUTYPE_IMX6Q);
DEFINE_MX6_CPU_TYPE(mx6qp, IMX6_CPUTYPE_IMX6QP);
DEFINE_MX6_CPU_TYPE(mx6d, IMX6_CPUTYPE_IMX6D);
DEFINE_MX6_CPU_TYPE(mx6dp, IMX6_CPUTYPE_IMX6DP);
DEFINE_MX6_CPU_TYPE(mx6sx, IMX6_CPUTYPE_IMX6SX);
DEFINE_MX6_CPU_TYPE(mx6sl, IMX6_CPUTYPE_IMX6SL);
DEFINE_MX6_CPU_TYPE(mx6ul, IMX6_CPUTYPE_IMX6UL);
DEFINE_MX6_CPU_TYPE(mx6ull, IMX6_CPUTYPE_IMX6ULL);

static inline int __imx6_cpu_revision(void)
{
	uint32_t si_rev = __imx6_read_si_rev();
	u8 major_part, minor_part;

	major_part = (si_rev >> 8) & 0xf;
	minor_part = si_rev & 0xf;

	return ((major_part + 1) << 4) | minor_part;
}

int imx6_cpu_revision(void);

#endif /* __MACH_IMX6_H */
