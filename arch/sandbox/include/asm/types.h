#ifndef __ASM_I386_TYPES_H
#define __ASM_I386_TYPES_H

#include <asm-generic/int-ll64.h>

#ifdef __x86_64__
/*
 * This is used in dlmalloc. On X86_64 we need it to be
 * 64 bit
 */
#define INTERNAL_SIZE_T unsigned long

/*
 * This is a Kconfig variable in the Kernel, but we want to detect
 * this during compile time, so we set it here.
 */
#define CONFIG_PHYS_ADDR_T_64BIT

#endif

#endif
