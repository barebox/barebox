// SPDX-License-Identifier: GPL-2.0

#include <common.h>
#include <asm/sections.h>
#include <linux/sizes.h>
#include <debug_ll.h>
#include <pbl.h>
#include <fdt.h>

#if __riscv_xlen == 64
#define IMAGE_LOAD_OFFSET 0x200000 /* Image load offset(2MB) from start of RAM */
#else
#define IMAGE_LOAD_OFFSET 0x400000 /* Image load offset(4MB) from start of RAM */
#endif

/* because we can depend on being loaded at an offset, we can just use
 * our load address as stack top
 */
#define __barebox_riscv_head() \
	__barebox_riscv_header("auipc sp, 0", IMAGE_LOAD_OFFSET, \
			       RISCV_HEADER_VERSION, "RISCV", RISCV_IMAGE_MAGIC2)

#include <asm/barebox-riscv.h>

ENTRY_FUNCTION(start_dt_2nd, a0, _fdt, a2)
{
	unsigned long membase, memsize, endmem, endfdt, uncompressed_len;
	struct fdt_header *fdt = (void *)_fdt;

	if (!fdt)
		hang();

	relocate_to_current_adr();
	setup_c();

	fdt_find_mem(fdt, &membase, &memsize);
	endmem = membase + memsize;
	endfdt = _fdt + be32_to_cpu(fdt->totalsize);

	/*
	 * QEMU likes to place the FDT at the end of RAM, where barebox
	 * would normally extract itself to. Accommodate this by moving
	 * memory end, so it doesn't overlap FDT
	 */
	uncompressed_len = input_data_len() + MAX_BSS_SIZE;

	if (riscv_mem_barebox_image(membase, endmem, uncompressed_len) < endfdt &&
	    _fdt < riscv_mem_stack_top(membase, endmem))
		memsize = ALIGN_DOWN(_fdt - membase, SZ_1M);

	barebox_riscv_entry(membase, memsize, fdt);
}
