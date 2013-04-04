#ifndef __ASM_SECTIONS_H
#define __ASM_SECTIONS_H

#ifndef __ASSEMBLY__
#include <asm-generic/sections.h>

/*
 * Access a linker supplied variable. Use this if your code might not be running
 * at the address it is linked at.
 */
#define ld_var(name) ({ \
	unsigned long __ld_var_##name(void); \
	__ld_var_##name(); \
})

#else

/*
 * Access a linker supplied variable, assembler macro version
 */
.macro ld_var name, reg, scratch
	1000:
		ldr \reg, 1001f
		ldr \scratch, =1000b
		add \reg, \reg, \scratch
		b 1002f
	1001:
		.word \name - 1000b
	1002:
.endm

#endif

#endif /* __ASM_SECTIONS_H */
