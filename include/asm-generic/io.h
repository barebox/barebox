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
#ifndef __ASM_GENERIC_IO_H
#define __ASM_GENERIC_IO_H

#include <linux/string.h> /* for memset() and memcpy() */
#include <linux/compiler.h>
#include <linux/types.h>
#include <asm/byteorder.h>
#include <asm-generic/io-typeconfused.h>

#ifdef CONFIG_64BIT
static inline u64 __raw_readq(const volatile void __iomem *addr)
{
	return *(const volatile u64 __force *) addr;
}
#define readq(addr) __le64_to_cpu(__raw_readq(addr))

static inline void __raw_writeq(u64 b, volatile void __iomem *addr)
{
	*(volatile u64 __force *) addr = b;
}
#define writeq(b,addr) __raw_writeq(__cpu_to_le64(b),addr)
#endif

#ifndef PCI_IOBASE
#define PCI_IOBASE ((void __iomem *)RELOC_HIDE((void *)0, 0))
#endif

#ifndef IO_SPACE_LIMIT
#define IO_SPACE_LIMIT 0xffff
#endif

/*
 * {in,out}{b,w,l}() access little endian I/O. {in,out}{b,w,l}_p() can be
 * implemented on hardware that needs an additional delay for I/O accesses to
 * take effect.
 */

#ifndef inb
#define inb inb
static inline u8 inb(unsigned long addr)
{
	return readb(PCI_IOBASE + addr);
}
#endif

#ifndef inw
#define inw inw
static inline u16 inw(unsigned long addr)
{
	return readw(PCI_IOBASE + addr);
}
#endif

#ifndef inl
#define inl inl
static inline u32 inl(unsigned long addr)
{
	return readl(PCI_IOBASE + addr);
}
#endif

#ifndef outb
#define outb outb
static inline void outb(u8 value, unsigned long addr)
{
	writeb(value, PCI_IOBASE + addr);
}
#endif

#ifndef outw
#define outw outw
static inline void outw(u16 value, unsigned long addr)
{
	writew(value, PCI_IOBASE + addr);
}
#endif

#ifndef outl
#define outl outl
static inline void outl(u32 value, unsigned long addr)
{
	writel(value, PCI_IOBASE + addr);
}
#endif

#ifndef inb_p
#define inb_p inb_p
static inline u8 inb_p(unsigned long addr)
{
	return inb(addr);
}
#endif

#ifndef inw_p
#define inw_p inw_p
static inline u16 inw_p(unsigned long addr)
{
	return inw(addr);
}
#endif

#ifndef inl_p
#define inl_p inl_p
static inline u32 inl_p(unsigned long addr)
{
	return inl(addr);
}
#endif

#ifndef outb_p
#define outb_p outb_p
static inline void outb_p(u8 value, unsigned long addr)
{
	outb(value, addr);
}
#endif

#ifndef outw_p
#define outw_p outw_p
static inline void outw_p(u16 value, unsigned long addr)
{
	outw(value, addr);
}
#endif

#ifndef outl_p
#define outl_p outl_p
static inline void outl_p(u32 value, unsigned long addr)
{
	outl(value, addr);
}
#endif

/*
 * {read,write}s{b,w,l,q}() repeatedly access the same memory address in
 * native endianness in 8-, 16-, 32- or 64-bit chunks (@count times).
 */
#ifndef readsb
#define readsb readsb
static inline void readsb(const volatile void __iomem *addr, void *buffer,
			  unsigned int count)
{
	if (count) {
		u8 *buf = buffer;

		do {
			u8 x = __raw_readb(addr);
			*buf++ = x;
		} while (--count);
	}
}
#endif

#ifndef readsw
#define readsw readsw
static inline void readsw(const volatile void __iomem *addr, void *buffer,
			  unsigned int count)
{
	if (count) {
		u16 *buf = buffer;

		do {
			u16 x = __raw_readw(addr);
			*buf++ = x;
		} while (--count);
	}
}
#endif

#ifndef readsl
#define readsl readsl
static inline void readsl(const volatile void __iomem *addr, void *buffer,
			  unsigned int count)
{
	if (count) {
		u32 *buf = buffer;

		do {
			u32 x = __raw_readl(addr);
			*buf++ = x;
		} while (--count);
	}
}
#endif

#ifdef CONFIG_64BIT
#ifndef readsq
#define readsq readsq
static inline void readsq(const volatile void __iomem *addr, void *buffer,
			  unsigned int count)
{
	if (count) {
		u64 *buf = buffer;

		do {
			u64 x = __raw_readq(addr);
			*buf++ = x;
		} while (--count);
	}
}
#endif
#endif /* CONFIG_64BIT */

#ifndef writesb
#define writesb writesb
static inline void writesb(volatile void __iomem *addr, const void *buffer,
			   unsigned int count)
{
	if (count) {
		const u8 *buf = buffer;

		do {
			__raw_writeb(*buf++, addr);
		} while (--count);
	}
}
#endif

#ifndef writesw
#define writesw writesw
static inline void writesw(volatile void __iomem *addr, const void *buffer,
			   unsigned int count)
{
	if (count) {
		const u16 *buf = buffer;

		do {
			__raw_writew(*buf++, addr);
		} while (--count);
	}
}
#endif

#ifndef writesl
#define writesl writesl
static inline void writesl(volatile void __iomem *addr, const void *buffer,
			   unsigned int count)
{
	if (count) {
		const u32 *buf = buffer;

		do {
			__raw_writel(*buf++, addr);
		} while (--count);
	}
}
#endif

#ifdef CONFIG_64BIT
#ifndef writesq
#define writesq writesq
static inline void writesq(volatile void __iomem *addr, const void *buffer,
			   unsigned int count)
{
	if (count) {
		const u64 *buf = buffer;

		do {
			__raw_writeq(*buf++, addr);
		} while (--count);
	}
}
#endif
#endif /* CONFIG_64BIT */

/*
 * {in,out}{b,w,l}() access little endian I/O. {in,out}{b,w,l}_p() can be
 * implemented on hardware that needs an additional delay for I/O accesses to
 * take effect.
 */

#if !defined(inb) && !defined(_inb)
#define _inb _inb
static inline u8 _inb(unsigned long addr)
{
	return __raw_readb(PCI_IOBASE + addr);
}
#endif

#if !defined(inw) && !defined(_inw)
#define _inw _inw
static inline u16 _inw(unsigned long addr)
{
	return __le16_to_cpu((__le16 __force)__raw_readw(PCI_IOBASE + addr));
}
#endif

#if !defined(inl) && !defined(_inl)
#define _inl _inl
static inline u32 _inl(unsigned long addr)
{
	return __le32_to_cpu((__le32 __force)__raw_readl(PCI_IOBASE + addr));
}
#endif

#if !defined(outb) && !defined(_outb)
#define _outb _outb
static inline void _outb(u8 value, unsigned long addr)
{
	return __raw_writeb(value, PCI_IOBASE + addr);
}
#endif

#if !defined(outw) && !defined(_outw)
#define _outw _outw
static inline void _outw(u16 value, unsigned long addr)
{
	return __raw_writew((u16 __force)cpu_to_le16(value), PCI_IOBASE + addr);
}
#endif

#if !defined(outl) && !defined(_outl)
#define _outl _outl
static inline void _outl(u32 value, unsigned long addr)
{
	return __raw_writel((u32 __force)cpu_to_le32(value), PCI_IOBASE + addr);
}
#endif

#ifndef inb
#define inb _inb
#endif

#ifndef inw
#define inw _inw
#endif

#ifndef inl
#define inl _inl
#endif

#ifndef outb
#define outb _outb
#endif

#ifndef outw
#define outw _outw
#endif

#ifndef outl
#define outl _outl
#endif

#ifndef inb_p
#define inb_p inb_p
static inline u8 inb_p(unsigned long addr)
{
	return inb(addr);
}
#endif

#ifndef inw_p
#define inw_p inw_p
static inline u16 inw_p(unsigned long addr)
{
	return inw(addr);
}
#endif

#ifndef inl_p
#define inl_p inl_p
static inline u32 inl_p(unsigned long addr)
{
	return inl(addr);
}
#endif

#ifndef outb_p
#define outb_p outb_p
static inline void outb_p(u8 value, unsigned long addr)
{
	outb(value, addr);
}
#endif

#ifndef outw_p
#define outw_p outw_p
static inline void outw_p(u16 value, unsigned long addr)
{
	outw(value, addr);
}
#endif

#ifndef outl_p
#define outl_p outl_p
static inline void outl_p(u32 value, unsigned long addr)
{
	outl(value, addr);
}
#endif

/*
 * {in,out}s{b,w,l}{,_p}() are variants of the above that repeatedly access a
 * single I/O port multiple times.
 */

#ifndef insb
#define insb insb
static inline void insb(unsigned long addr, void *buffer, unsigned int count)
{
	readsb(PCI_IOBASE + addr, buffer, count);
}
#endif

#ifndef insw
#define insw insw
static inline void insw(unsigned long addr, void *buffer, unsigned int count)
{
	readsw(PCI_IOBASE + addr, buffer, count);
}
#endif

#ifndef insl
#define insl insl
static inline void insl(unsigned long addr, void *buffer, unsigned int count)
{
	readsl(PCI_IOBASE + addr, buffer, count);
}
#endif

#ifndef outsb
#define outsb outsb
static inline void outsb(unsigned long addr, const void *buffer,
			 unsigned int count)
{
	writesb(PCI_IOBASE + addr, buffer, count);
}
#endif

#ifndef outsw
#define outsw outsw
static inline void outsw(unsigned long addr, const void *buffer,
			 unsigned int count)
{
	writesw(PCI_IOBASE + addr, buffer, count);
}
#endif

#ifndef outsl
#define outsl outsl
static inline void outsl(unsigned long addr, const void *buffer,
			 unsigned int count)
{
	writesl(PCI_IOBASE + addr, buffer, count);
}
#endif

#ifndef insb_p
#define insb_p insb_p
static inline void insb_p(unsigned long addr, void *buffer, unsigned int count)
{
	insb(addr, buffer, count);
}
#endif

#ifndef insw_p
#define insw_p insw_p
static inline void insw_p(unsigned long addr, void *buffer, unsigned int count)
{
	insw(addr, buffer, count);
}
#endif

#ifndef insl_p
#define insl_p insl_p
static inline void insl_p(unsigned long addr, void *buffer, unsigned int count)
{
	insl(addr, buffer, count);
}
#endif

#ifndef outsb_p
#define outsb_p outsb_p
static inline void outsb_p(unsigned long addr, const void *buffer,
			   unsigned int count)
{
	outsb(addr, buffer, count);
}
#endif

#ifndef outsw_p
#define outsw_p outsw_p
static inline void outsw_p(unsigned long addr, const void *buffer,
			   unsigned int count)
{
	outsw(addr, buffer, count);
}
#endif

#ifndef outsl_p
#define outsl_p outsl_p
static inline void outsl_p(unsigned long addr, const void *buffer,
			   unsigned int count)
{
	outsl(addr, buffer, count);
}
#endif

#ifndef ioread8
#define ioread8 ioread8
static inline u8 ioread8(const volatile void __iomem *addr)
{
	return readb(addr);
}
#endif

#ifndef ioread16
#define ioread16 ioread16
static inline u16 ioread16(const volatile void __iomem *addr)
{
	return readw(addr);
}
#endif

#ifndef ioread32
#define ioread32 ioread32
static inline u32 ioread32(const volatile void __iomem *addr)
{
	return readl(addr);
}
#endif

#ifdef CONFIG_64BIT
#ifndef ioread64
#define ioread64 ioread64
static inline u64 ioread64(const volatile void __iomem *addr)
{
	return readq(addr);
}
#endif
#endif /* CONFIG_64BIT */

#ifndef iowrite8
#define iowrite8 iowrite8
static inline void iowrite8(u8 value, volatile void __iomem *addr)
{
	writeb(value, addr);
}
#endif

#ifndef iowrite16
#define iowrite16 iowrite16
static inline void iowrite16(u16 value, volatile void __iomem *addr)
{
	writew(value, addr);
}
#endif

#ifndef iowrite32
#define iowrite32 iowrite32
static inline void iowrite32(u32 value, volatile void __iomem *addr)
{
	writel(value, addr);
}
#endif

#ifdef CONFIG_64BIT
#ifndef iowrite64
#define iowrite64 iowrite64
static inline void iowrite64(u64 value, volatile void __iomem *addr)
{
	writeq(value, addr);
}
#endif
#endif /* CONFIG_64BIT */

#ifndef ioread16be
#define ioread16be ioread16be
static inline u16 ioread16be(const volatile void __iomem *addr)
{
	return swab16(readw(addr));
}
#endif

#ifndef ioread32be
#define ioread32be ioread32be
static inline u32 ioread32be(const volatile void __iomem *addr)
{
	return swab32(readl(addr));
}
#endif

#ifdef CONFIG_64BIT
#ifndef ioread64be
#define ioread64be ioread64be
static inline u64 ioread64be(const volatile void __iomem *addr)
{
	return swab64(readq(addr));
}
#endif
#endif /* CONFIG_64BIT */

#ifndef iowrite16be
#define iowrite16be iowrite16be
static inline void iowrite16be(u16 value, void volatile __iomem *addr)
{
	writew(swab16(value), addr);
}
#endif

#ifndef iowrite32be
#define iowrite32be iowrite32be
static inline void iowrite32be(u32 value, volatile void __iomem *addr)
{
	writel(swab32(value), addr);
}
#endif

#ifdef CONFIG_64BIT
#ifndef iowrite64be
#define iowrite64be iowrite64be
static inline void iowrite64be(u64 value, volatile void __iomem *addr)
{
	writeq(swab64(value), addr);
}
#endif
#endif /* CONFIG_64BIT */

#ifndef ioread8_rep
#define ioread8_rep ioread8_rep
static inline void ioread8_rep(const volatile void __iomem *addr, void *buffer,
			       unsigned int count)
{
	readsb(addr, buffer, count);
}
#endif

#ifndef ioread16_rep
#define ioread16_rep ioread16_rep
static inline void ioread16_rep(const volatile void __iomem *addr,
				void *buffer, unsigned int count)
{
	readsw(addr, buffer, count);
}
#endif

#ifndef ioread32_rep
#define ioread32_rep ioread32_rep
static inline void ioread32_rep(const volatile void __iomem *addr,
				void *buffer, unsigned int count)
{
	readsl(addr, buffer, count);
}
#endif

#ifdef CONFIG_64BIT
#ifndef ioread64_rep
#define ioread64_rep ioread64_rep
static inline void ioread64_rep(const volatile void __iomem *addr,
				void *buffer, unsigned int count)
{
	readsq(addr, buffer, count);
}
#endif
#endif /* CONFIG_64BIT */

#ifndef iowrite8_rep
#define iowrite8_rep iowrite8_rep
static inline void iowrite8_rep(volatile void __iomem *addr,
				const void *buffer,
				unsigned int count)
{
	writesb(addr, buffer, count);
}
#endif

#ifndef iowrite16_rep
#define iowrite16_rep iowrite16_rep
static inline void iowrite16_rep(volatile void __iomem *addr,
				 const void *buffer,
				 unsigned int count)
{
	writesw(addr, buffer, count);
}
#endif

#ifndef iowrite32_rep
#define iowrite32_rep iowrite32_rep
static inline void iowrite32_rep(volatile void __iomem *addr,
				 const void *buffer,
				 unsigned int count)
{
	writesl(addr, buffer, count);
}
#endif

#ifdef CONFIG_64BIT
#ifndef iowrite64_rep
#define iowrite64_rep iowrite64_rep
static inline void iowrite64_rep(volatile void __iomem *addr,
				 const void *buffer,
				 unsigned int count)
{
	writesq(addr, buffer, count);
}
#endif
#endif /* CONFIG_64BIT */



/*
 * Change virtual addresses to physical addresses and vv.
 * These are pretty trivial
 */
#ifndef virt_to_phys
#define virt_to_phys virt_to_phys
static inline unsigned long virt_to_phys(const volatile void *mem)
{
	return (unsigned long)mem;
}
#endif

#ifndef phys_to_virt
#define phys_to_virt phys_to_virt
static inline void *phys_to_virt(unsigned long phys)
{
	return (void *)phys;
}
#endif

#ifndef IOMEM
#ifndef __ASSEMBLY__
#define IOMEM(addr)	((void __force __iomem *)(addr))
#else
#define IOMEM(addr)	addr
#endif
#endif

#define __io_virt(x) ((void __force *)(x))

#ifndef memset_io
#define memset_io memset_io
/**
 * memset_io	Set a range of I/O memory to a constant value
 * @addr:	The beginning of the I/O-memory range to set
 * @val:	The value to set the memory to
 * @count:	The number of bytes to set
 *
 * Set a range of I/O memory to a given value.
 */
static inline void memset_io(volatile void __iomem *addr, int value,
			     size_t size)
{
	memset(__io_virt(addr), value, size);
}
#endif

#ifndef memcpy_fromio
#define memcpy_fromio memcpy_fromio
/**
 * memcpy_fromio	Copy a block of data from I/O memory
 * @dst:		The (RAM) destination for the copy
 * @src:		The (I/O memory) source for the data
 * @count:		The number of bytes to copy
 *
 * Copy a block of data from I/O memory.
 */
static inline void memcpy_fromio(void *buffer,
				 const volatile void __iomem *addr,
				 size_t size)
{
	memcpy(buffer, __io_virt(addr), size);
}
#endif

#ifndef memcpy_toio
#define memcpy_toio memcpy_toio
/**
 * memcpy_toio		Copy a block of data into I/O memory
 * @dst:		The (I/O memory) destination for the copy
 * @src:		The (RAM) source for the data
 * @count:		The number of bytes to copy
 *
 * Copy a block of data to I/O memory.
 */
static inline void memcpy_toio(volatile void __iomem *addr, const void *buffer,
			       size_t size)
{
	memcpy(__io_virt(addr), buffer, size);
}
#endif

#include <asm-generic/bitio.h>

#define IOMEM_ERR_PTR(err) (__force void __iomem *)ERR_PTR(err)

#ifndef readq_relaxed
#define readq_relaxed(addr) readq(addr)
#endif

#ifndef readl_relaxed
#define readl_relaxed(addr) readl(addr)
#endif

#ifndef readw_relaxed
#define readw_relaxed(addr) readw(addr)
#endif

#ifndef readb_relaxed
#define readb_relaxed(addr) readb(addr)
#endif

#ifndef writeq_relaxed
#define writeq_relaxed(val, addr) writeq((val), (addr))
#endif

#ifndef writel_relaxed
#define writel_relaxed(val, addr) writel((val), (addr))
#endif

#ifndef writew_relaxed
#define writew_relaxed(val, addr) writew((val), (addr))
#endif

#ifndef writeb_relaxed
#define writeb_relaxed(val, addr) writeb((val), (addr))
#endif

#endif /* __ASM_GENERIC_IO_H */
