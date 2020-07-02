#ifndef __ASM_I386_TYPES_H
#define __ASM_I386_TYPES_H

#include <asm-generic/int-ll64.h>

#ifdef __x86_64__
/*
 * This is used in dlmalloc. On X86_64 we need it to be
 * 64 bit
 */
#define INTERNAL_SIZE_T unsigned long

#endif

#endif
