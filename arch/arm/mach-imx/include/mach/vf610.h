#ifndef __MACH_VF610_H
#define __MACH_VF610_H

#include <io.h>
#include <mach/generic.h>
#include <mach/vf610-regs.h>
#include <mach/revision.h>

#define VF610_CPUTYPE_VFx10	0x010

#define VF610_CPUTYPE_VF610	0x610
#define VF610_CPUTYPE_VF600	0x600
#define VF610_CPUTYPE_VF510	0x510
#define VF610_CPUTYPE_VF500	0x500

#define VF610_ROM_VERSION_OFFSET	0x80

static inline int __vf610_cpu_type(void)
{
	void __iomem *mscm = IOMEM(VF610_MSCM_BASE_ADDR);
	const u32 cpxcount = readl(mscm + VF610_MSCM_CPxCOUNT);
	const u32 cpxcfg1  = readl(mscm + VF610_MSCM_CPxCFG1);
	int cpu_type;

	cpu_type = cpxcount ? VF610_CPUTYPE_VF600 : VF610_CPUTYPE_VF500;

	return cpxcfg1 ? cpu_type | VF610_CPUTYPE_VFx10 : cpu_type;
}

static inline int vf610_cpu_type(void)
{
	if (!cpu_is_vf610())
		return 0;

	return __vf610_cpu_type();
}

static inline int vf610_cpu_revision(void)
{
	if (!cpu_is_vf610())
		return IMX_CHIP_REV_UNKNOWN;

	/*
	 * There doesn't seem to be a documented way of retreiving
	 * silicon revision on VFxxx cpus, so we just report Mask ROM
	 * version instead
	 */
	return readl(VF610_ROM_VERSION_OFFSET) & 0xff;
}

#endif
