// SPDX-License-Identifier: GPL-2.0-only

#include <linux/sizes.h>
#include <asm/barebox-arm.h>
#include <asm/reloc.h>
#include <compressed-dtb.h>
#include <debug_ll.h>
#include <console.h>
#include <stdio.h>
#include <debug_ll/pl011.h>
#include <pbl.h>

#define RAM_BASE	0x40000000
#define PL011_BASE	IOMEM(0x9000000)

extern char __dtb_qemu_virt32_start[];
extern char __dtb_qemu_virt64_start[];

static void *find_fdt(void *r0)
{
	void *fdt, *ram = (void *)RAM_BASE;
	const char *origin;

	if (blob_is_fdt(ram)) {
		origin = "-bios";
		fdt = ram;
	} else if (r0 >= ram && blob_is_fdt(r0)) {
		origin = "-kernel";
		fdt = r0;
	} else if (IS_ENABLED(CONFIG_ARM32)) {
		origin = "built-in";
		fdt = __dtb_qemu_virt32_start;
	} else {
		origin = "built-in";
		fdt = __dtb_qemu_virt64_start;
	}

	pr_info("Using %s device tree at %p\n", origin, fdt);
	return fdt;
}

/*
 * Entry point for QEMU virt firmware boot (-bios option).
 *
 * Memory layout:
 *   0x00000000 - 0x08000000: Flash/ROM
 *   0x08000000 - 0x40000000: Peripherals
 *   0x40000000 - ...........: RAM
 */
static noinline void continue_qemu_virt_bios(ulong r0)
{
	ulong membase = RAM_BASE, memsize = SZ_32M - SZ_4M;
	void *fdt;

	pbl_set_putc(debug_ll_pl011_putc, PL011_BASE);

	/* QEMU may put a DTB at the start of RAM */
	fdt = find_fdt((void *)r0);

	fdt_find_mem(fdt, &membase, &memsize);

	barebox_arm_entry(membase, memsize, fdt);
}

ENTRY_FUNCTION_WITHSTACK(start_qemu_virt_bios, RAM_BASE + SZ_32M, r0, r1, r2)
{

	arm_cpu_lowlevel_init();

	putc_ll('>');

	if (get_pc() >= RAM_BASE)
		relocate_to_current_adr();
	else
		relocate_to_adr_full(RAM_BASE + SZ_4M);

	setup_c();

	continue_qemu_virt_bios(r0);
}
