#ifndef __ASM_RISCV_TYPES_H
#define __ASM_RISCV_TYPES_H

#include <asm-generic/int-ll64.h>

#if __riscv_xlen == 64
/*
 * This is used in dlmalloc. On RISCV64 we need it to be 64 bit
 */
#define INTERNAL_SIZE_T unsigned long

#endif

#endif /* __ASM_RISCV_TYPES_H */
