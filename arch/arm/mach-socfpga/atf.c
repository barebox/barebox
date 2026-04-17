// SPDX-License-Identifier: GPL-2.0-only
#include <asm/sections.h>
#include <common.h>
#include <firmware.h>
#include <asm-generic/memory_layout.h>
#include <asm/atf_common.h>
#include <asm/barebox-arm.h>
#include <mach/socfpga/atf.h>
#include <mach/socfpga/generic.h>
#include <mach/socfpga/mailbox_s10.h>
#include <mach/socfpga/soc64-regs.h>
#include <mach/socfpga/soc64-sdram.h>
#include <mach/socfpga/soc64-system-manager.h>

static void __noreturn agilex5_load_and_start_image_via_tfa(void)
{
	void *bl31 = (void *)AGILEX5_ATF_BL31_BASE_ADDR;
	void *bl33 = (void *)AGILEX5_ATF_BL33_BASE_ADDR;
	struct fwobj tfa;

	pr_debug("copy bl33 to %p from %p (%zu bytes)\n",
		 bl33, __image_start, barebox_image_size);
	memcpy(bl33, __image_start, barebox_image_size);

	get_builtin_firmware(agilex5_bl31_bin, &tfa);

	pr_debug("copy bl31 to %p from %p (%zu bytes)\n",
		 bl31, tfa.data, tfa.size);
	memcpy(bl31, tfa.data, tfa.size);

	asm volatile("msr sp_el2, %0" : :
		     "r" (bl33 - 16) :
		     "cc");

	pr_debug("Jump to bl31 0x%p\n", bl31);
	bl31_entry((uintptr_t)bl31, 0, (uintptr_t)bl33, 0);
	__builtin_unreachable();
}

void __noreturn agilex5_barebox_entry(void *fdt)
{
	phys_addr_t membase;
	phys_size_t memsize;

	if (current_el() == 3) {
		agilex5_initialize_security_policies();
		pr_debug("Security policies initialized\n");

		/*
		 * need to set the bank select enable before the
		 * agilex5_ddr_init_full() otherwise the serial doesn't show
		 * anything.
		 */
		if (!IS_ENABLED(CONFIG_DEBUG_LL))
			writel(LCR_BKSE, SOCFPGA_UART0_ADDRESS + LCR);
		agilex5_ddr_init_full();

		socfpga_mailbox_s10_init();
		socfpga_mailbox_s10_qspi_open();

		agilex5_load_and_start_image_via_tfa();
		__builtin_unreachable();
	}

	membase = agilex5_mpfe_sdram_base();
	memsize = agilex5_mpfe_sdram_size();

	if (membase < SOCFPGA_AGILEX5_DDR_BASE || memsize == 0) {
		pr_err("Invalid firewall configuration\n");
		hang();
	}

	barebox_arm_entry(membase, memsize, fdt);
}
