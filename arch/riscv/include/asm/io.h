#ifndef __ASM_RISCV_IO_H
#define __ASM_RISCV_IO_H

#define IO_SPACE_LIMIT 0

#include <asm-generic/io.h>

static inline void *phys_to_virt(unsigned long phys)
{
	return (void *)phys;
}

static inline unsigned long virt_to_phys(volatile void *mem)
{
	return (unsigned long)mem;
}

#endif /* __ASM_RISCV_IO_H */
