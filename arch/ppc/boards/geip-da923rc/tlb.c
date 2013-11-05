/*
 * Copyright 2013 GE Intelligent Platforms, Inc.
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
 */

#include <common.h>
#include <mach/mmu.h>

struct fsl_e_tlb_entry tlb_table[] = {
	/* TLB 0 - for temp stack in cache */
	FSL_SET_TLB_ENTRY(0, CFG_INIT_RAM_ADDR, CFG_INIT_RAM_ADDR,
			  MAS3_SX | MAS3_SW | MAS3_SR, 0,
			  0, 0, BOOKE_PAGESZ_4K, 0),
	FSL_SET_TLB_ENTRY(0, CFG_INIT_RAM_ADDR + (4 * 1024),
			  CFG_INIT_RAM_ADDR + (4 * 1024),
			  MAS3_SX | MAS3_SW | MAS3_SR, 0,
			  0, 0, BOOKE_PAGESZ_4K, 0),
	FSL_SET_TLB_ENTRY(0, CFG_INIT_RAM_ADDR + (8 * 1024),
			  CFG_INIT_RAM_ADDR + (8 * 1024),
			  MAS3_SX | MAS3_SW | MAS3_SR, 0,
			  0, 0, BOOKE_PAGESZ_4K, 0),
	FSL_SET_TLB_ENTRY(0, CFG_INIT_RAM_ADDR + (12 * 1024),
			  CFG_INIT_RAM_ADDR + (12 * 1024),
			  MAS3_SX | MAS3_SW | MAS3_SR, 0,
			  0, 0, BOOKE_PAGESZ_4K, 0),
	/*
	 * TLB 0/1:     2x16M     Cache inhibited, guarded
	 * CPLD and NAND in cache-inhibited area.
	 */
	FSL_SET_TLB_ENTRY(1, BOOT_BLOCK, BOOT_BLOCK,
			  MAS3_SX | MAS3_SW | MAS3_SR,
			  MAS2_W | MAS2_I | MAS2_G | MAS2_M,
			  0, 0, BOOKE_PAGESZ_16M, 1),
	FSL_SET_TLB_ENTRY(1, BOOT_BLOCK + 0x1000000,
			  BOOT_BLOCK + 0x1000000,
			  MAS3_SX | MAS3_SW | MAS3_SR,
			  MAS2_W | MAS2_I | MAS2_G | MAS2_M,
			  0, 1, BOOKE_PAGESZ_16M, 1),
	/*
	 * The boot flash is mapped with the cache enabled.
	 * TLB 2/3:     2x16M     Cacheable Write-through, guarded
	 */
	FSL_SET_TLB_ENTRY(1, BOOT_BLOCK + (2 * 0x1000000),
			  BOOT_BLOCK + (2 * 0x1000000),
			  MAS3_SX | MAS3_SW | MAS3_SR,
			  MAS2_W | MAS2_G | MAS2_M,
			  0, 2, BOOKE_PAGESZ_16M, 1),
	FSL_SET_TLB_ENTRY(1, BOOT_BLOCK + (3 * 0x1000000),
			  BOOT_BLOCK + (3 * 0x1000000),
			  MAS3_SX | MAS3_SW | MAS3_SR,
			  MAS2_W | MAS2_G | MAS2_M,
			  0, 3, BOOKE_PAGESZ_16M, 1),

	FSL_SET_TLB_ENTRY(1, CFG_CCSRBAR, CFG_CCSRBAR_PHYS,
			  MAS3_SX | MAS3_SW | MAS3_SR,
			  MAS2_I | MAS2_G,
			  0, 4, BOOKE_PAGESZ_64M, 1),
};

int num_tlb_entries = ARRAY_SIZE(tlb_table);
