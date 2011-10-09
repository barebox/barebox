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

static inline void outb(unsigned char value, int port)
{
	asm volatile("outb %b0, %w1" : : "a"(value), "Nd"(port));
}

static inline void outw(unsigned short value, int port)
{
	asm volatile("outw %w0, %w1" : : "a"(value), "Nd"(port));
}

static inline void outl(unsigned long value, int port)
{
	asm volatile("outl %0, %w1" : : "a"(value), "Nd"(port));
}

static inline unsigned char inb(int port)
{
	unsigned char value;
	asm volatile("inb %w1, %b0" : "=a"(value) : "Nd"(port));
	return value;
}

static inline unsigned short inw(int port)
{
	unsigned short value;
	asm volatile("inw %w1, %w0" : "=a"(value) : "Nd"(port));
	return value;
}

static inline unsigned long inl(int port)
{
	unsigned long value;
	asm volatile("inl %w1, %0" : "=a"(value) : "Nd"(port));
	return value;
}

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

/* do a tiny io delay */
static inline void io_delay(void)
{
	inb(0x80);
}

#endif	/* __ASM_X86_IO_H */
