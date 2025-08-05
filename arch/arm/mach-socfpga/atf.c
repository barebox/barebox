// SPDX-License-Identifier: GPL-2.0-only
#include <asm/sections.h>
#include <common.h>
#include <firmware.h>
#include <asm-generic/memory_layout.h>
#include <asm/atf_common.h>
#include <asm/barebox-arm.h>
#include <mach/socfpga/atf.h>
#include <mach/socfpga/generic.h>
#include <mach/socfpga/soc64-regs.h>

void __noreturn agilex5_load_and_start_image_via_tfa(unsigned long memsize)
{
	unsigned long atf_dest = AGILEX5_ATF_BL31_BASE_ADDR;
	void __noreturn (*bl31)(void) = (void *)atf_dest;
	const void *tfa;
	size_t tfa_size;

	pr_debug("Load TFA\n");

	memcpy((void *)AGILEX5_ATF_BL33_BASE_ADDR, __image_start, barebox_image_size);

	get_builtin_firmware(agilex5_bl31_bin, &tfa, &tfa_size);

	memcpy(bl31, tfa, tfa_size);

	asm volatile("msr sp_el2, %0" : :
		     "r" (AGILEX5_ATF_BL33_BASE_ADDR - 16) :
		     "cc");

	pr_debug("Jumping to @0x%08lx\n", atf_dest);
	bl31_entry((uintptr_t)bl31, 0, AGILEX5_ATF_BL33_BASE_ADDR, 0);
	__builtin_unreachable();
}
