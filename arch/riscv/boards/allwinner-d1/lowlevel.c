// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <debug_ll.h>
#include <asm/barebox-riscv.h>

#define DRAM_BASE	0x40000000

ENTRY_FUNCTION(start_allwinner_d1, a0, a1, a2)
{
	barebox_riscv_supervisor_entry(DRAM_BASE, SZ_1G, a0, (void *)a1);
}
