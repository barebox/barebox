#ifndef __ASM_ARM_IO_H
#define __ASM_ARM_IO_H

#include <asm-generic/io.h>

/*
 * String version of IO memory access ops:
 */
extern void memcpy_fromio(void *, const volatile void __iomem *, size_t);
extern void memcpy_toio(volatile void __iomem *, const void *, size_t);
extern void memset_io(volatile void __iomem *, int, size_t);

#endif	/* __ASM_ARM_IO_H */
