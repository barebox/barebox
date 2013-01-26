#ifndef __ASM_MIPS_MEMORY_H
#define __ASM_MIPS_MEMORY_H

#include <memory.h>
#include <asm/addrspace.h>

static inline void mips_add_ram0(resource_size_t size)
{
	barebox_add_memory_bank("kseg0_ram0", KSEG0, size);
	barebox_add_memory_bank("kseg1_ram0", KSEG1, size);
}
#endif	/* __ASM_MIPS_MEMORY_H */
