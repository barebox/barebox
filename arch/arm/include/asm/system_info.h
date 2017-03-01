#ifndef __ASM_ARM_SYSTEM_INFO_H
#define __ASM_ARM_SYSTEM_INFO_H

#include <asm/cputype.h>

#define CPU_ARCH_UNKNOWN	0
#define CPU_ARCH_ARMv3		1
#define CPU_ARCH_ARMv4		2
#define CPU_ARCH_ARMv4T		3
#define CPU_ARCH_ARMv5		4
#define CPU_ARCH_ARMv5T		5
#define CPU_ARCH_ARMv5TE	6
#define CPU_ARCH_ARMv5TEJ	7
#define CPU_ARCH_ARMv6		8
#define CPU_ARCH_ARMv7		9
#define CPU_ARCH_ARMv8		10

#define CPU_IS_ARM720		0x41007200
#define CPU_IS_ARM720_MASK	0xff00fff0

#define CPU_IS_ARM920		0x41009200
#define CPU_IS_ARM920_MASK	0xff00fff0

#define CPU_IS_ARM926		0x41069260
#define CPU_IS_ARM926_MASK	0xff0ffff0

#define CPU_IS_ARM1176		0x410fb760
#define CPU_IS_ARM1176_MASK	0xff0ffff0

#define CPU_IS_CORTEX_A8	0x410fc080
#define CPU_IS_CORTEX_A8_MASK	0xff0ffff0

#define CPU_IS_CORTEX_A5	0x410fc050
#define CPU_IS_CORTEX_A5_MASK	0xff0ffff0

#define CPU_IS_CORTEX_A9	0x410fc090
#define CPU_IS_CORTEX_A9_MASK	0xff0ffff0

#define CPU_IS_CORTEX_A7	0x410fc070
#define CPU_IS_CORTEX_A7_MASK	0xff0ffff0

#define CPU_IS_CORTEX_A15	0x410fc0f0
#define CPU_IS_CORTEX_A15_MASK	0xff0ffff0

#define CPU_IS_CORTEX_A53	0x410fd034
#define CPU_IS_CORTEX_A53_MASK	0xff0ffff0

#define CPU_IS_CORTEX_A57	0x411fd070
#define CPU_IS_CORTEX_A57_MASK	0xff0ffff0

#define CPU_IS_PXA250		0x69052100
#define CPU_IS_PXA250_MASK	0xfffff7f0

#define CPU_IS_PXA255		0x69052d00
#define CPU_IS_PXA255_MASK	0xfffffff0

#define CPU_IS_PXA270		0x69054110
#define CPU_IS_PXA270_MASK	0xfffff7f0

#define cpu_is_arm(core) ((read_cpuid_id() & CPU_IS_##core##_MASK) == CPU_IS_##core)

#ifdef CONFIG_CPU_32v4T
#ifdef ARM_ARCH
#define ARM_MULTIARCH
#else
#define ARM_ARCH CPU_ARCH_ARMv4T
#endif
#define cpu_is_arm720()	cpu_is_arm(ARM720)
#define cpu_is_arm920()	cpu_is_arm(ARM920)
#define cpu_is_pxa250() cpu_is_arm(PXA250)
#define cpu_is_pxa255() cpu_is_arm(PXA255)
#define cpu_is_pxa270() cpu_is_arm(PXA270)
#else
#define cpu_is_arm720() (0)
#define cpu_is_arm920() (0)
#define cpu_is_pxa250() (0)
#define cpu_is_pxa255() (0)
#define cpu_is_pxa270() (0)
#endif

#ifdef CONFIG_CPU_32v5
#ifdef ARM_ARCH
#define ARM_MULTIARCH
#else
#define ARM_ARCH CPU_ARCH_ARMv5
#endif
#define cpu_is_arm926() cpu_is_arm(ARM926)
#else
#define cpu_is_arm926() (0)
#endif

#ifdef CONFIG_CPU_32v6
#ifdef ARM_ARCH
#define ARM_MULTIARCH
#else
#define ARM_ARCH CPU_ARCH_ARMv6
#endif
#define cpu_is_arm1176()	cpu_is_arm(ARM1176)
#else
#define cpu_is_arm1176()	(0)
#endif

#ifdef CONFIG_CPU_32v7
#ifdef ARM_ARCH
#define ARM_MULTIARCH
#else
#define ARM_ARCH CPU_ARCH_ARMv7
#endif
#define cpu_is_cortex_a8() cpu_is_arm(CORTEX_A8)
#define cpu_is_cortex_a5() cpu_is_arm(CORTEX_A5)
#define cpu_is_cortex_a9() cpu_is_arm(CORTEX_A9)
#define cpu_is_cortex_a7() cpu_is_arm(CORTEX_A7)
#define cpu_is_cortex_a15() cpu_is_arm(CORTEX_A15)
#else
#define cpu_is_cortex_a8() (0)
#define cpu_is_cortex_a5() (0)
#define cpu_is_cortex_a9() (0)
#define cpu_is_cortex_a7() (0)
#define cpu_is_cortex_a15() (0)
#endif

#ifdef CONFIG_CPU_64v8
#ifdef ARM_ARCH
#define ARM_MULTIARCH
#else
#define ARM_ARCH CPU_ARCH_ARMv8
#endif
#define cpu_is_cortex_a53() cpu_is_arm(CORTEX_A53)
#define cpu_is_cortex_a57() cpu_is_arm(CORTEX_A57)
#else
#define cpu_is_cortex_a53() (0)
#define cpu_is_cortex_a57() (0)
#endif

#ifndef __ASSEMBLY__

#ifdef ARM_MULTIARCH
/*
 * Early version to get the ARM cpu architecture. Only needed during
 * early startup when the C environment is not yet fully initialized.
 * Normally you should use cpu_architecture() instead.
 */
static inline int arm_early_get_cpu_architecture(void)
{
	int cpu_arch;

	if ((read_cpuid_id() & 0x0008f000) == 0) {
		cpu_arch = CPU_ARCH_UNKNOWN;
	} else if ((read_cpuid_id() & 0x0008f000) == 0x00007000) {
		cpu_arch = (read_cpuid_id() & (1 << 23)) ? CPU_ARCH_ARMv4T : CPU_ARCH_ARMv3;
	} else if ((read_cpuid_id() & 0x00080000) == 0x00000000) {
		cpu_arch = (read_cpuid_id() >> 16) & 7;
		if (cpu_arch)
			cpu_arch += CPU_ARCH_ARMv3;
	} else if ((read_cpuid_id() & 0x000f0000) == 0x000f0000) {
#ifdef CONFIG_CPU_64v8
		unsigned int isar2;

		__asm__ __volatile__(
			"mrs	%0, id_isar2_el1\n"
			: "=r" (isar2)
			:
			: "memory");


		/* Check Load/Store acquire to check if ARMv8 or not */

		if (isar2 & 0x2)
			cpu_arch = CPU_ARCH_ARMv8;
		else
			cpu_arch = CPU_ARCH_UNKNOWN;
#else
		unsigned int mmfr0;

		/* Revised CPUID format. Read the Memory Model Feature
		 * Register 0 and check for VMSAv7 or PMSAv7 */
		asm("mrc	p15, 0, %0, c0, c1, 4"
		    : "=r" (mmfr0));
		if ((mmfr0 & 0x0000000f) >= 0x00000003 ||
		    (mmfr0 & 0x000000f0) >= 0x00000030)
			cpu_arch = CPU_ARCH_ARMv7;
		else if ((mmfr0 & 0x0000000f) == 0x00000002 ||
			 (mmfr0 & 0x000000f0) == 0x00000020)
			cpu_arch = CPU_ARCH_ARMv6;
		else
			cpu_arch = CPU_ARCH_UNKNOWN;
	} else
		cpu_arch = CPU_ARCH_UNKNOWN;
#endif

	/*
	 * Special case for ARMv6 (K/Z) (has v7 compatible MMU, but is v6
	 * otherwise). The below check just matches all ARMv6, as done in the
	 * Linux kernel.
	 */
	if ((read_cpuid_id() & 0x7f000) == 0x7b000)
		cpu_arch = CPU_ARCH_ARMv6;

	return cpu_arch;
}

extern int __pure cpu_architecture(void);
#else
static inline int __pure arm_early_get_cpu_architecture(void)
{
	return ARM_ARCH;
}

static inline int __pure cpu_architecture(void)
{
	return ARM_ARCH;
}
#endif

#endif /* !__ASSEMBLY__ */

#endif /* __ASM_ARM_SYSTEM_INFO_H */
