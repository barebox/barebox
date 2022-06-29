// SPDX-License-Identifier: GPL-2.0-only

#include <linux/types.h>
#include <mach/core.h>
#include <asm/system_info.h>

void __iomem *bcm2835_get_mmio_base_by_cpuid(void)
{
	static u32 cpuid;

	if (!cpuid) {
		cpuid = read_cpuid_id();
		pr_debug("ARM CPUID: %08x\n", cpuid);
	}

	/* We know ARM1167, Cortex A-7, A-53 and A-72 CPUID mask is identical */
	switch(cpuid & CPU_IS_ARM1176_MASK) {
	case CPU_IS_ARM1176:	/* bcm2835 */
		return IOMEM(0x20000000);
	case CPU_IS_CORTEX_A7:	/* bcm2836 */
	case CPU_IS_CORTEX_A53:	/* bcm2837 */
		return IOMEM(0x3f000000);
	case CPU_IS_CORTEX_A72:	/* bcm2711 */
		return IOMEM(0xfe000000);
	}

	pr_err("Couldn't determine rpi by CPUID %08x\n", cpuid);
	return NULL;
}
