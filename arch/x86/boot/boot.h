/* -*- linux-c -*- ------------------------------------------------------- *
 *
 *   Copyright (C) 1991, 1992 Linus Torvalds
 *   Copyright 2007 rPath, Inc. - All Rights Reserved
 *   Copyright 2009 Intel Corporation; author H. Peter Anvin
 *
 *   This file is part of the Linux kernel, and is made available under
 *   the terms of the GNU General Public License version 2.
 *
 * ----------------------------------------------------------------------- */

/**
 * @file
 * @brief Main declarations for the real mode code
 */

#ifndef BOOT_BOOT_H
#define BOOT_BOOT_H

#define STACK_SIZE	512	/* Minimum number of bytes for stack */

/** Carry flag */
#define X86_EFLAGS_CF	0x00000001

/** PE flag */
#define X86_CR0_PE      0x00000001

#ifndef __ASSEMBLY__

#include <types.h>

/* we are still in real mode here! */
#define THIS_IS_REALMODE_CODE asm(".code16gcc");

struct biosregs {
	union {
		struct {
			uint32_t edi;
			uint32_t esi;
			uint32_t ebp;
			uint32_t _esp;
			uint32_t ebx;
			uint32_t edx;
			uint32_t ecx;
			uint32_t eax;
			uint32_t _fsgs;
			uint32_t _dses;
			uint32_t eflags;
		};
		struct {
			uint16_t di, hdi;
			uint16_t si, hsi;
			uint16_t bp, hbp;
			uint16_t _sp, _hsp;
			uint16_t bx, hbx;
			uint16_t dx, hdx;
			uint16_t cx, hcx;
			uint16_t ax, hax;
			uint16_t gs, fs;
			uint16_t es, ds;
			uint16_t flags, hflags;
		};
		struct {
			uint8_t dil, dih, edi2, edi3;
			uint8_t sil, sih, esi2, esi3;
			uint8_t bpl, bph, ebp2, ebp3;
			uint8_t _spl, _sph, _esp2, _esp3;
			uint8_t bl, bh, ebx2, ebx3;
			uint8_t dl, dh, edx2, edx3;
			uint8_t cl, ch, ecx2, ecx3;
			uint8_t al, ah, eax2, eax3;
		};
	};
};

/* functions in the realmode part */
extern int enable_a20(void);
extern void initregs(struct biosregs *regs);
extern void intcall(uint8_t int_no, const struct biosregs *ireg, struct biosregs *oreg);
extern void boot_puts(char*);
extern void __attribute__((noreturn)) die(void);
extern void __attribute__((noreturn)) protected_mode_jump(void);

struct gdt_ptr {
	uint16_t len;
	uint32_t ptr;
} __attribute__((packed));

/* These functions are used to reference data in other segments. */

static inline uint16_t ds(void)
{
	uint16_t seg;
	asm("movw %%ds,%0" : "=rm" (seg));
	return seg;
}

static inline void set_fs(uint16_t seg)
{
	asm volatile("movw %0,%%fs" : : "rm" (seg));
}

static inline uint16_t fs(void)
{
	uint16_t seg;
	asm volatile("movw %%fs,%0" : "=rm" (seg));
	return seg;
}

static inline void set_gs(uint16_t seg)
{
	asm volatile("movw %0,%%gs" : : "rm" (seg));
}

static inline uint16_t gs(void)
{
	uint16_t seg;
	asm volatile("movw %%gs,%0" : "=rm" (seg));
	return seg;
}

typedef unsigned int addr_t;

static inline uint8_t rdfs8(addr_t addr)
{
	uint8_t v;
	asm volatile("movb %%fs:%1,%0" : "=q" (v) : "m" (*(uint8_t *)addr));
	return v;
}
static inline uint16_t rdfs16(addr_t addr)
{
	uint16_t v;
	asm volatile("movw %%fs:%1,%0" : "=r" (v) : "m" (*(uint16_t *)addr));
	return v;
}
static inline uint32_t rdfs32(addr_t addr)
{
	uint32_t v;
	asm volatile("movl %%fs:%1,%0" : "=r" (v) : "m" (*(uint32_t *)addr));
	return v;
}

static inline void wrfs8(uint8_t v, addr_t addr)
{
	asm volatile("movb %1,%%fs:%0" : "+m" (*(uint8_t *)addr) : "qi" (v));
}
static inline void wrfs16(uint16_t v, addr_t addr)
{
	asm volatile("movw %1,%%fs:%0" : "+m" (*(uint16_t *)addr) : "ri" (v));
}
static inline void wrfs32(uint32_t v, addr_t addr)
{
	asm volatile("movl %1,%%fs:%0" : "+m" (*(uint32_t *)addr) : "ri" (v));
}

static inline uint8_t rdgs8(addr_t addr)
{
	uint8_t v;
	asm volatile("movb %%gs:%1,%0" : "=q" (v) : "m" (*(uint8_t *)addr));
	return v;
}
static inline uint16_t rdgs16(addr_t addr)
{
	uint16_t v;
	asm volatile("movw %%gs:%1,%0" : "=r" (v) : "m" (*(uint16_t *)addr));
	return v;
}
static inline uint32_t rdgs32(addr_t addr)
{
	uint32_t v;
	asm volatile("movl %%gs:%1,%0" : "=r" (v) : "m" (*(uint32_t *)addr));
	return v;
}

static inline void wrgs8(uint8_t v, addr_t addr)
{
	asm volatile("movb %1,%%gs:%0" : "+m" (*(uint8_t *)addr) : "qi" (v));
}
static inline void wrgs16(uint16_t v, addr_t addr)
{
	asm volatile("movw %1,%%gs:%0" : "+m" (*(uint16_t *)addr) : "ri" (v));
}
static inline void wrgs32(uint32_t v, addr_t addr)
{
	asm volatile("movl %1,%%gs:%0" : "+m" (*(uint32_t *)addr) : "ri" (v));
}

/** use the built in memset function for the real mode code */
#define memset(d,c,l) __builtin_memset(d,c,l)

#endif /* __ASSEMBLY__ */

#endif /* BOOT_BOOT_H */
