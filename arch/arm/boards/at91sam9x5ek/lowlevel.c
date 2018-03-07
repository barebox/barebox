#include <common.h>
#include <linux/sizes.h>
#include <mach/at91sam9_ddrsdr.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <io.h>
#include <debug_ll.h>

extern char __dtb_at91sam9x5ek_start[];

ENTRY_FUNCTION(start_at91sam9x5ek, r0, r1, r2)
{
	void *fdt;

	arm_cpu_lowlevel_init();
	arm_setup_stack(AT91SAM9X5_SRAM_BASE + AT91SAM9X5_SRAM_SIZE - 16);

	fdt = __dtb_at91sam9x5ek_start + get_runtime_offset();

	barebox_arm_entry(AT91_CHIPSELECT_1, at91sam9x5_get_ddram_size(), fdt);
}
