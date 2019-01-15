#ifndef __ASM_BITSPERLONG_H
#define __ASM_BITSPERLONG_H

#ifdef __riscv64
#define BITS_PER_LONG 64
#else
#define BITS_PER_LONG 32
#endif

#endif /* __ASM_BITSPERLONG_H */
