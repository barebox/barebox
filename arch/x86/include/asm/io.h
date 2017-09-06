/*
 * Mostly stolen from the linux kernel
 */

/**
 * @file
 * @brief x86 IO access functions
 */

#ifndef __ASM_X86_IO_H
#define __ASM_X86_IO_H

#include <asm/byteorder.h>

#define build_mmio_read(name, size, type, reg, barrier) \
 static inline type name(const volatile void *addr) \
 { type ret; asm volatile("mov" size " %1,%0":reg (ret) \
 :"m" (*(volatile type*)addr) barrier); return ret; }

build_mmio_read(readb, "b", unsigned char, "=q", :"memory")
build_mmio_read(readw, "w", unsigned short, "=r", :"memory")
build_mmio_read(readl, "l", unsigned int, "=r", :"memory")

#define build_mmio_write(name, size, type, reg, barrier) \
 static inline void name(type val, volatile void *addr) \
 { asm volatile("mov" size " %0,%1": :reg (val), \
 "m" (*(volatile type*)addr) barrier); }

build_mmio_write(writeb, "b", unsigned char, "q", :"memory")
build_mmio_write(writew, "w", unsigned short, "r", :"memory")
build_mmio_write(writel, "l", unsigned int, "r", :"memory")

#define BUILDIO(bwl, bw, type)						\
static inline void out##bwl(unsigned type value, int port)		\
{									\
	asm volatile("out" #bwl " %" #bw "0, %w1"			\
		     : : "a"(value), "Nd"(port));			\
}									\
									\
static inline unsigned type in##bwl(int port)				\
{									\
	unsigned type value;						\
	asm volatile("in" #bwl " %w1, %" #bw "0"			\
		     : "=a"(value) : "Nd"(port));			\
	return value;							\
}									\
									\
static inline void outs##bwl(int port, const void *addr, unsigned long count) \
{									\
	asm volatile("rep; outs" #bwl					\
		     : "+S"(addr), "+c"(count) : "d"(port));		\
}									\
									\
static inline void ins##bwl(int port, void *addr, unsigned long count)	\
{									\
	asm volatile("rep; ins" #bwl					\
		     : "+D"(addr), "+c"(count) : "d"(port));		\
}

BUILDIO(b, b, char)
BUILDIO(w, w, short)
BUILDIO(l, , int)

#define outb	outb
#define outw	outw
#define outl	outb
#define inb	inb
#define inw	inw
#define inl	inl
#define outsb	outsb
#define outsw	outsw
#define outsl	outsb
#define insb	insb
#define insw	insw
#define insl	insl

#define  IO_SPACE_LIMIT  0xffff

/* do a tiny io delay */
static inline void io_delay(void)
{
	inb(0x80);
}

#include <asm-generic/io.h>

#endif	/* __ASM_X86_IO_H */
