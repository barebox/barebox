/* Generic I/O port emulation, based on MN10300 code
 *
 * Copyright (C) 2007 Red Hat, Inc. All Rights Reserved.
 * Written by David Howells (dhowells@redhat.com)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public Licence
 * as published by the Free Software Foundation; either version
 * 2 of the Licence, or (at your option) any later version.
 */
#ifndef __ASM_GENERIC_IO_TYPECONFUSED_H
#define __ASM_GENERIC_IO_TYPECONFUSED_H

#include <linux/string.h> /* for memset() and memcpy() */
#include <linux/compiler.h>
#include <linux/types.h>
#include <asm/byteorder.h>

/*****************************************************************************/
/*
 * Unlike the definitions in <asm-generic/io.h>, these macros don't complain
 * about integer arguments and just silently cast them to pointers. This is
 * a common cause of bugs, but lots of existing code depends on this, so
 * this header is provided as a transitory measure.
 */

#ifndef __raw_readb
#define __raw_readb(a)		(__chk_io_ptr(a), *(volatile unsigned char __force  *)(a))
#endif

#ifndef __raw_readw
#define __raw_readw(a)		(__chk_io_ptr(a), *(volatile unsigned short __force *)(a))
#endif

#ifndef __raw_readl
#define __raw_readl(a)		(__chk_io_ptr(a), *(volatile unsigned int __force   *)(a))
#endif

#ifndef readb
#define readb __raw_readb
#endif

#ifndef readw
#define readw(addr) __le16_to_cpu(__raw_readw(addr))
#endif

#ifndef readl
#define readl(addr) __le32_to_cpu(__raw_readl(addr))
#endif

#ifndef __raw_writeb
#define __raw_writeb(v,a)	(__chk_io_ptr(a), *(volatile unsigned char __force  *)(a) = (v))
#endif

#ifndef __raw_writew
#define __raw_writew(v,a)	(__chk_io_ptr(a), *(volatile unsigned short __force *)(a) = (v))
#endif

#ifndef __raw_writel
#define __raw_writel(v,a)	(__chk_io_ptr(a), *(volatile unsigned int __force   *)(a) = (v))
#endif

#ifndef writeb
#define writeb __raw_writeb
#endif

#ifndef writew
#define writew(b,addr) __raw_writew(__cpu_to_le16(b),addr)
#endif

#ifndef writel
#define writel(b,addr) __raw_writel(__cpu_to_le32(b),addr)
#endif

#endif /* __ASM_GENERIC_IO_TYPECONFUSED_H */
