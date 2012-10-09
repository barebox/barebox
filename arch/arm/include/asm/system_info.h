#ifndef __ASM_ARM_SYSTEM_INFO_H
#define __ASM_ARM_SYSTEM_INFO_H

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

#ifdef CONFIG_CPU_32v4T
#ifdef ARM_ARCH
#define ARM_MULTIARCH
#else
#define ARM_ARCH CPU_ARCH_ARMv4T
#endif
#endif

#ifdef CONFIG_CPU_32v5
#ifdef ARM_ARCH
#define ARM_MULTIARCH
#else
#define ARM_ARCH CPU_ARCH_ARMv5
#endif
#endif

#ifdef CONFIG_CPU_32v6
#ifdef ARM_ARCH
#define ARM_MULTIARCH
#else
#define ARM_ARCH CPU_ARCH_ARMv6
#endif
#endif

#ifdef CONFIG_CPU_32v7
#ifdef ARM_ARCH
#define ARM_MULTIARCH
#else
#define ARM_ARCH CPU_ARCH_ARMv7
#endif
#endif

#ifndef __ASSEMBLY__

#ifdef ARM_MULTIARCH
extern int __pure cpu_architecture(void);
#else
static inline int __pure cpu_architecture(void)
{
	return ARM_ARCH;
}
#endif

#endif /* !__ASSEMBLY__ */

#endif /* __ASM_ARM_SYSTEM_INFO_H */
