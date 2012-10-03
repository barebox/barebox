/*
 * Copyright 2012 GE Intelligent Platforms, Inc.
 *
 * Copyright 2008-2009 Freescale Semiconductor, Inc.
 *
 * (C) Copyright 2000
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <asm/processor.h>
#include <asm/config.h>
#include <asm/bitops.h>
#include <mach/mmu.h>

void e500_invalidate_tlb(u8 tlb)
{
	if (tlb == 0)
		mtspr(MMUCSR0, 0x4);
	if (tlb == 1)
		mtspr(MMUCSR0, 0x2);
}

void e500_init_tlbs(void)
{
	int i;

	for (i = 0; i < num_tlb_entries; i++) {
		e500_write_tlb(tlb_table[i].mas0,
			  tlb_table[i].mas1,
			  tlb_table[i].mas2,
			  tlb_table[i].mas3,
			  tlb_table[i].mas7);
	}

	return ;
}

static int e500_find_free_tlbcam(void)
{
	int ix;
	u32 _mas1;
	unsigned int num_cam = mfspr(SPRN_TLB1CFG) & 0xfff;

	for (ix = 0; ix < num_cam; ix++) {
		mtspr(MAS0, FSL_BOOKE_MAS0(1, ix, 0));
		asm volatile("tlbre;isync");
		_mas1 = mfspr(MAS1);
		if (!(_mas1 & MAS1_VALID))
			return ix;
	}

	if (ix >= NUM_TLBCAMS)
		panic("No more free TLBs");

	return ix;
}

void e500_set_tlb(u8 tlb, u32 epn, u64 rpn,
	     u8 perms, u8 wimge,
	     u8 ts, u8 esel, u8 tsize, u8 iprot)
{
	u32 _mas0, _mas1, _mas2, _mas3, _mas7;

	_mas0 = FSL_BOOKE_MAS0(tlb, esel, 0);
	_mas1 = FSL_BOOKE_MAS1(1, iprot, 0, ts, tsize);
	_mas2 = FSL_BOOKE_MAS2(epn, wimge);
	_mas3 = FSL_BOOKE_MAS3(rpn, 0, perms);
	_mas7 = FSL_BOOKE_MAS7(rpn);

	e500_write_tlb(_mas0, _mas1, _mas2, _mas3, _mas7);
}

void e500_disable_tlb(u8 esel)
{
	mtspr(MAS0, FSL_BOOKE_MAS0(1, esel, 0));
	mtspr(MAS1, 0);
	mtspr(MAS2, 0);
	mtspr(MAS3, 0);
	asm volatile("isync;msync;tlbwe;isync");
}

static inline void tlbsx(const unsigned *addr)
{
	__asm__ __volatile__ ("tlbsx 0,%0" : : "r" (addr), "m" (*addr));
}

int e500_find_tlb_idx(void *addr, u8 tlbsel)
{
	u32 _mas0, _mas1;

	/* zero out Search PID, AS */
	mtspr(MAS6, 0);
	tlbsx(addr);

	_mas0 = mfspr(MAS0);
	_mas1 = mfspr(MAS1);

	/* we found something, and its in the TLB we expect */
	if ((MAS1_VALID & _mas1) &&
		(MAS0_TLBSEL(tlbsel) == (_mas0 & MAS0_TLBSEL_MSK))) {
		return (_mas0 & MAS0_ESEL_MSK) >> 16;
	}

	panic("Address 0x%p not found in TLB %d\n", addr, tlbsel);
}

static unsigned int e500_setup_ddr_tlbs_phys(phys_addr_t p_addr,
					unsigned int memsize_in_meg)
{
	int i;
	unsigned int tlb_size;
	unsigned int wimge = 0;
	unsigned int ram_tlb_address = (unsigned int)CFG_SDRAM_BASE;
	unsigned int max_cam = (mfspr(SPRN_TLB1CFG) >> 16) & 0xf;
	u64 size, memsize = (u64)memsize_in_meg << 20;

	size = min((u64)memsize, (u64)MAX_MEM_MAPPED);

	/* Convert (4^max) kB to (2^max) bytes */
	max_cam = (max_cam * 2) + 10;

	for (i = 0; size && (i < 8); i++) {
		int ram_tlb_index = e500_find_free_tlbcam();
		u32 camsize = __ilog2_u64(size) & ~1U;
		u32 align = __ilog2(ram_tlb_address) & ~1U;

		if (align == -2)
			align = max_cam;
		if (camsize > align)
			camsize = align;

		if (camsize > max_cam)
			camsize = max_cam;

		tlb_size = (camsize - 10) / 2;

		e500_set_tlb(1, ram_tlb_address, p_addr,
			MAS3_SX|MAS3_SW|MAS3_SR, wimge,
			0, ram_tlb_index, tlb_size, 1);

		size -= 1ULL << camsize;
		memsize -= 1ULL << camsize;
		ram_tlb_address += 1UL << camsize;
		p_addr += 1UL << camsize;
	}

	if (memsize)
		printf("%lld left unmapped\n", memsize);

	return memsize_in_meg;
}

inline unsigned int e500_setup_ddr_tlbs(unsigned int memsize_in_meg)
{
	return e500_setup_ddr_tlbs_phys(CFG_SDRAM_BASE, memsize_in_meg);
}
