/*
 * This was automagically generated from arch/m68k/tools/mach-types!
 * Do NOT edit
 */

#ifndef __ASM_M68K_MACH_TYPE_H
#define __ASM_M68K_MACH_TYPE_H

#ifndef __ASSEMBLY__
/* The type of machine we're running on */
extern unsigned int __machine_arch_type;
#endif

/* see arch/m68k/kernel/arch.c for a description of these */
#define MACH_TYPE_GENERIC       0
#define MACH_TYPE_MCF54xx       1
#define MACH_TYPE_MCF5445x      2

#ifdef CONFIG_ARCH_MCF54xx
# ifdef machine_arch_type
#  undef machine_arch_type
#  define machine_arch_type	__machine_arch_type
# else
#  define machine_arch_type	MACH_TYPE_MCF54xx
# endif
# define machine_is_mcf54xx()	(machine_arch_type == MACH_TYPE_MCF54xx)
#else
# define machine_is_mcf54xx()	(0)
#endif

#ifdef CONFIG_ARCH_MCF5445x
# ifdef machine_arch_type
#  undef machine_arch_type
#  define machine_arch_type	__machine_arch_type
# else
#  define machine_arch_type	MACH_TYPE_MCF5445x
# endif
# define machine_is_mcf5445x()	(machine_arch_type == MACH_TYPE_MCF5445x)
#else
# define machine_is_mcf5445x()	(0)
#endif


#ifndef machine_arch_type
#define machine_arch_type	__machine_arch_type
#endif

#endif
