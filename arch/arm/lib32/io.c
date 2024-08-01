// SPDX-License-Identifier: GPL-2.0-only

#include <module.h>
#include <linux/types.h>
#include <asm/unaligned.h>
#include <linux/align.h>
#include <io.h>

/*
 * Copy data from IO memory space to "real" memory space.
 */
void memcpy_fromio(void *to, const volatile void __iomem *from, size_t count)
{
	while (count && !PTR_IS_ALIGNED(from, 4)) {
		*(u8 *)to = __raw_readb(from);
		from++;
		to++;
		count--;
	}

	while (count >= 4) {
		put_unaligned(__raw_readl(from), (u32 *)to);
		from += 4;
		to += 4;
		count -= 4;
	}

	while (count) {
		*(u8 *)to = __raw_readb(from);
		from++;
		to++;
		count--;
	}
}

/*
 * Copy data from "real" memory space to IO memory space.
 */
void memcpy_toio(volatile void __iomem *to, const void *from, size_t count)
{
	while (count && !IS_ALIGNED((unsigned long)to, 4)) {
		__raw_writeb(*(u8 *)from, to);
		from++;
		to++;
		count--;
	}

	while (count >= 4) {
		__raw_writel(get_unaligned((u32 *)from), to);
		from += 4;
		to += 4;
		count -= 4;
	}

	while (count) {
		__raw_writeb(*(u8 *)from, to);
		from++;
		to++;
		count--;
	}
}

/*
 * "memset" on IO memory space.
 */
void memset_io(volatile void __iomem *dst, int c, size_t count)
{
	u32 qc = (u8)c;

	qc |= qc << 8;
	qc |= qc << 16;

	while (count && !PTR_IS_ALIGNED(dst, 4)) {
		__raw_writeb(c, dst);
		dst++;
		count--;
	}

	while (count >= 4) {
		__raw_writel(qc, dst);
		dst += 4;
		count -= 4;
	}

	while (count) {
		__raw_writeb(c, dst);
		dst++;
		count--;
	}
}

EXPORT_SYMBOL(memcpy_fromio);
EXPORT_SYMBOL(memcpy_toio);
EXPORT_SYMBOL(memset_io);
