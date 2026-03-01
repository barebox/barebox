// SPDX-License-Identifier: GPL-2.0-only
/*
 * Board support for QEMU ppce500 virtual machine
 */

#include <barebox-info.h>
#include <deep-probe.h>
#include <init.h>
#include <of.h>
#include <linux/libfdt.h>
#include <asm/processor.h>
#include <asm/cache.h>
#include <mach/mpc85xx.h>
#include <mach/clock.h>
#include <debug_ll.h>

/* Global pointer to device tree */
static void *fdt;

/* Called from start-qemu.S assembly */
void qemu_e500_entry(void *fdt_blob);

/* Called from board initialization */
extern void board_init_r(ulong end_of_ram);

void qemu_e500_entry(void *fdt_blob)
{
	int cpus_off, cpu_off, len;
	const fdt32_t *prop;

	/* Initialize debug UART first */
	debug_ll_init();

	putc_ll('Q');
	putc_ll('E');
	putc_ll('M');
	putc_ll('U');
	putc_ll('\r');
	putc_ll('\n');

	/* Save FDT to global for later use */
	fdt = fdt_blob;

	/*
	 * Parse timebase-frequency from QEMU's FDT.  The MPC85xx GUTS
	 * PORPLLSR register is unimplemented in QEMU and reads as zero,
	 * so the normal register-based clock calculation fails.
	 */
	cpus_off = fdt_path_offset(fdt_blob, "/cpus");
	if (cpus_off >= 0) {
		cpu_off = fdt_first_subnode(fdt_blob, cpus_off);
		if (cpu_off >= 0) {
			prop = fdt_getprop(fdt_blob, cpu_off,
					   "timebase-frequency", &len);
			if (prop && len == sizeof(*prop))
				fsl_set_timebase_clock(fdt32_to_cpu(*prop));
		}
	}

	putc_ll('F');
	putc_ll('D');
	putc_ll('T');
	putc_ll(':');
	puthex_ll((unsigned long)fdt_blob);
	putc_ll('\r');
	putc_ll('\n');

	putc_ll('B');
	putc_ll('R');
	putc_ll('\r');
	putc_ll('\n');

	board_init_r(256 * 1024 * 1024);
}

static int qemu_e500_of_init(void)
{
	if (!fdt)
		return 0;

	barebox_set_model("QEMU ppce500");
	barebox_set_hostname("qemu-e500");

	return barebox_register_fdt(fdt);
}
core_initcall(qemu_e500_of_init);

static const struct of_device_id qemu_e500_of_match[] = {
	{ .compatible = "fsl,qemu-e500" },
	{ /* sentinel */ },
};
BAREBOX_DEEP_PROBE_ENABLE(qemu_e500_of_match);
