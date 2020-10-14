// SPDX-License-Identifier: GPL-2.0-only

#ifndef __ASM_GENERIC_BITIO_H
#define __ASM_GENERIC_BITIO_H

#include <asm/io.h>

/*
 * Clear and set bits in one shot. These macros can be used to clear and
 * set multiple bits in a register using a single call. These macros can
 * also be used to set a multiple-bit bit pattern using a mask, by
 * specifying the mask in the 'clear' parameter and the new bit pattern
 * in the 'set' parameter.
 */

#ifndef out_arch
#define out_arch(type,endian,a,v)	__raw_write##type(cpu_to_##endian(v),a)
#endif

#ifndef in_arch
#define in_arch(type,endian,a)		endian##_to_cpu(__raw_read##type(a))
#endif

#ifndef out_le64
#define out_le64(a,v)	out_arch(q,le64,a,v)
#endif

#ifndef out_le32
#define out_le32(a,v)	out_arch(l,le32,a,v)
#endif

#ifndef out_le16
#define out_le16(a,v)	out_arch(w,le16,a,v)
#endif

#ifndef in_le64
#define in_le64(a)	in_arch(q,le64,a)
#endif

#ifndef in_le32
#define in_le32(a)	in_arch(l,le32,a)
#endif

#ifndef in_le16
#define in_le16(a)	in_arch(w,le16,a)
#endif

#ifndef out_be32
#define out_be32(a,v)	out_arch(l,be32,a,v)
#endif

#ifndef out_be16
#define out_be16(a,v)	out_arch(w,be16,a,v)
#endif

#ifndef in_be32
#define in_be32(a)	in_arch(l,be32,a)
#endif

#ifndef in_be16
#define in_be16(a)	in_arch(w,be16,a)
#endif

#ifndef out_8
#define out_8(a,v)	__raw_writeb(v,a)
#endif

#ifndef in_8
#define in_8(a)		__raw_readb(a)
#endif

#ifndef clrbits
#define clrbits(type, addr, clear) \
	out_##type((addr), in_##type(addr) & ~(clear))
#endif

#ifndef setbits
#define setbits(type, addr, set) \
	out_##type((addr), in_##type(addr) | (set))
#endif

#define clrsetbits(type, addr, clear, set) \
	out_##type((addr), (in_##type(addr) & ~(clear)) | (set))

#ifndef clrbits_be32
#define clrbits_be32(addr, clear) clrbits(be32, addr, clear)
#endif

#ifndef setbits_be32
#define setbits_be32(addr, set) setbits(be32, addr, set)
#endif

#ifndef clrsetbits_be32
#define clrsetbits_be32(addr, clear, set) clrsetbits(be32, addr, clear, set)
#endif

#ifndef clrbits_le32
#define clrbits_le32(addr, clear) clrbits(le32, addr, clear)
#endif

#ifndef setbits_le32
#define setbits_le32(addr, set) setbits(le32, addr, set)
#endif

#ifndef clrsetbits_le32
#define clrsetbits_le32(addr, clear, set) clrsetbits(le32, addr, clear, set)
#endif

#ifndef clrbits_be16
#define clrbits_be16(addr, clear) clrbits(be16, addr, clear)
#endif

#ifndef setbits_be16
#define setbits_be16(addr, set) setbits(be16, addr, set)
#endif

#ifndef clrsetbits_be16
#define clrsetbits_be16(addr, clear, set) clrsetbits(be16, addr, clear, set)
#endif

#ifndef clrbits_le16
#define clrbits_le16(addr, clear) clrbits(le16, addr, clear)
#endif

#ifndef setbits_le16
#define setbits_le16(addr, set) setbits(le16, addr, set)
#endif

#ifndef clrsetbits_le16
#define clrsetbits_le16(addr, clear, set) clrsetbits(le16, addr, clear, set)
#endif

#ifndef clrbits_8
#define clrbits_8(addr, clear) clrbits(8, addr, clear)
#endif

#ifndef setbits_8
#define setbits_8(addr, set) setbits(8, addr, set)
#endif

#ifndef clrsetbits_8
#define clrsetbits_8(addr, clear, set) clrsetbits(8, addr, clear, set)
#endif

#endif /* __ASM_GENERIC_IO_H */
