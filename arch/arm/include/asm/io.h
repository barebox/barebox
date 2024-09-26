/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __ASM_ARM_IO_H
#define __ASM_ARM_IO_H

#include <linux/compiler.h>

#ifndef CONFIG_CPU_64
/*
 * Generic IO read/write.  These perform native-endian accesses.  Note
 * that some architectures will want to re-define __raw_{read,write}w.
 */
void __raw_writesb(volatile void __iomem *addr, const void *data, int bytelen);
void __raw_writesw(volatile void __iomem *addr, const void *data, int wordlen);
void __raw_writesl(volatile void __iomem *addr, const void *data, int longlen);

void __raw_readsb(const volatile void __iomem *addr, void *data, int bytelen);
void __raw_readsw(const volatile void __iomem *addr, void *data, int wordlen);
void __raw_readsl(const volatile void __iomem *addr, void *data, int longlen);

#define readsb(p,d,l)		__raw_readsb(p,d,l)
#define readsw(p,d,l)		__raw_readsw(p,d,l)
#define readsl(p,d,l)		__raw_readsl(p,d,l)

#define writesb(p,d,l)		__raw_writesb(p,d,l)
#define writesw(p,d,l)		__raw_writesw(p,d,l)
#define writesl(p,d,l)		__raw_writesl(p,d,l)
#endif

#define	IO_SPACE_LIMIT	0

#define memcpy_fromio memcpy_fromio
#define memcpy_toio memcpy_toio
#define memset_io memset_io

#include <asm-generic/io.h>

/*
 * String version of IO memory access ops:
 */
extern void memcpy_fromio(void *, const volatile void __iomem *, size_t);
extern void memcpy_toio(volatile void __iomem *, const void *, size_t);
extern void memset_io(volatile void __iomem *, int, size_t);

#endif	/* __ASM_ARM_IO_H */
