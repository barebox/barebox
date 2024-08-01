/* SPDX-License-Identifier: GPL-2.0-only */

/*
 * Stolen from the linux-2.6/include/asm-generic/io.h
 */

/**
 * @file
 * @brief mips IO access functions
 */

#ifndef __ASM_MIPS_IO_H
#define __ASM_MIPS_IO_H

#include <linux/compiler.h>
#include <asm/types.h>
#include <asm/addrspace.h>
#include <asm/byteorder.h>

void dma_flush_range(unsigned long, unsigned long);
void dma_inv_range(unsigned long, unsigned long);

/*
 *     virt_to_phys - map virtual addresses to physical
 *     @address: address to remap
 *
 *     The returned physical address is the physical (CPU) mapping for
 *     the memory address given.
 */
#define virt_to_phys virt_to_phys
static inline unsigned long virt_to_phys(const volatile void *address)
{
	return CPHYSADDR((unsigned long)address);
}

/*
 *     phys_to_virt - map physical address to virtual
 *     @address: address to remap
 *
 *     The returned virtual address is a current CPU mapping for
 *     the memory address given.
 */
#define phys_to_virt phys_to_virt
static inline void *phys_to_virt(unsigned long address)
{
	if (IS_ENABLED(CONFIG_MMU)) {
		return (void *)CKSEG0ADDR(address);
	}

	return (void *)CKSEG1ADDR(address);
}

#define	IO_SPACE_LIMIT	0

#ifdef CONFIG_64BIT
#define IOMEM(addr)	((void __force __iomem *)PHYS_TO_XKSEG_UNCACHED(addr))
#else
#define IOMEM(addr)	((void __force __iomem *)CKSEG1ADDR(addr))
#endif

#include <asm-generic/io.h>

#endif	/* __ASM_MIPS_IO_H */
