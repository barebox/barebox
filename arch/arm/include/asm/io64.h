/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Based on arch/arm/include/asm/io.h
 *
 * Copyright (C) 1996-2000 Russell King
 * Copyright (C) 2012 ARM Ltd.
 */
#ifndef __ASM_IO64_H
#define __ASM_IO64_H

#include <linux/types.h>
#include <linux/compiler.h>
#include <asm/byteorder.h>

/*
 * Generic IO read/write.  These perform native-endian accesses.
 */
static __always_inline void ___raw_writeb(u8 val, volatile void __iomem *addr)
{
	volatile u8 __iomem *ptr = addr;
	asm volatile("strb %w0, %1" : : "rZ" (val), "Qo" (*ptr));
}

static __always_inline void ___raw_writew(u16 val, volatile void __iomem *addr)
{
	volatile u16 __iomem *ptr = addr;
	asm volatile("strh %w0, %1" : : "rZ" (val), "Qo" (*ptr));
}

static __always_inline void ___raw_writel(u32 val, volatile void __iomem *addr)
{
	volatile u32 __iomem *ptr = addr;
	asm volatile("str %w0, %1" : : "rZ" (val), "Qo" (*ptr));
}

static __always_inline void ___raw_writeq(u64 val, volatile void __iomem *addr)
{
	volatile u64 __iomem *ptr = addr;
	asm volatile("str %x0, %1" : : "rZ" (val), "Qo" (*ptr));
}

static __always_inline u8 ___raw_readb(const volatile void __iomem *addr)
{
	u8 val;
	asm volatile("ldrb %w0, [%1]" : "=r" (val) : "r" (addr));
	return val;
}

static __always_inline u16 ___raw_readw(const volatile void __iomem *addr)
{
	u16 val;

	asm volatile("ldrh %w0, [%1]" : "=r" (val) : "r" (addr));
	return val;
}

static __always_inline u32 ___raw_readl(const volatile void __iomem *addr)
{
	u32 val;
	asm volatile("ldr %w0, [%1]" : "=r" (val) : "r" (addr));
	return val;
}

static __always_inline u64 ___raw_readq(const volatile void __iomem *addr)
{
	u64 val;
	asm volatile("ldr %0, [%1]" : "=r" (val) : "r" (addr));
	return val;
}


#ifdef __LINUX_IO_STRICT_PROTOTYPES__
#define __IOMEM(a)	(a)
#else
#define __IOMEM(a)	((void __force __iomem *)(a))
#endif

#define __raw_writeb(v, a) ___raw_writeb(v, __IOMEM(a))
#define __raw_writew(v, a) ___raw_writew(v, __IOMEM(a))
#define __raw_writel(v, a) ___raw_writel(v, __IOMEM(a))
#define __raw_writeq(v, a) ___raw_writeq(v, __IOMEM(a))

#define __raw_readb(a) ___raw_readb(__IOMEM(a))
#define __raw_readw(a) ___raw_readw(__IOMEM(a))
#define __raw_readl(a) ___raw_readl(__IOMEM(a))
#define __raw_readq(a) ___raw_readq(__IOMEM(a))

/*
 * io{read,write}{16,32,64}be() macros
 */
#define ioread16be(p)		({ __u16 __v = be16_to_cpu((__force __be16)__raw_readw(p)); __v; })
#define ioread32be(p)		({ __u32 __v = be32_to_cpu((__force __be32)__raw_readl(p)); __v; })
#define ioread64be(p)		({ __u64 __v = be64_to_cpu((__force __be64)__raw_readq(p)); __v; })

#define iowrite16be(v,p)	({ __raw_writew((__force __u16)cpu_to_be16(v), p); })
#define iowrite32be(v,p)	({ __raw_writel((__force __u32)cpu_to_be32(v), p); })
#define iowrite64be(v,p)	({ __raw_writeq((__force __u64)cpu_to_be64(v), p); })

#endif	/* __ASM_IO64_H */
