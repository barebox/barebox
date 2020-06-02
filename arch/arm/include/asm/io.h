#ifndef __ASM_ARM_IO_H
#define __ASM_ARM_IO_H

#define	IO_SPACE_LIMIT	0

#include <asm-generic/io.h>
#include <asm-generic/bitio.h>

/*
 * String version of IO memory access ops:
 */
extern void memcpy_fromio(void *, const volatile void __iomem *, size_t);
extern void memcpy_toio(volatile void __iomem *, const void *, size_t);
extern void memset_io(volatile void __iomem *, int, size_t);

static inline void *phys_to_virt(unsigned long phys)
{
	return (void *)phys;
}

static inline unsigned long virt_to_phys(volatile void *mem)
{
	return (unsigned long)mem;
}

#endif	/* __ASM_ARM_IO_H */
