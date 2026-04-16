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
	void *bl31 = (void *)AGILEX5_ATF_BL31_BASE_ADDR;
	void *bl33 = (void *)AGILEX5_ATF_BL33_BASE_ADDR;
	const void *tfa;
	size_t tfa_size;

	pr_debug("copy bl33 to %p from %p (%zu bytes)\n",
		 bl33, __image_start, barebox_image_size);
	memcpy(bl33, __image_start, barebox_image_size);

	get_builtin_firmware(agilex5_bl31_bin, &tfa, &tfa_size);

	pr_debug("copy bl31 to %p from %p (%zu bytes)\n",
		 bl31, tfa, tfa_size);
	memcpy(bl31, tfa, tfa_size);

	asm volatile("msr sp_el2, %0" : :
		     "r" (bl33 - 16) :
		     "cc");

	pr_debug("Jump to bl31 0x%p\n", bl31);
	bl31_entry((uintptr_t)bl31, 0, (uintptr_t)bl33, 0);
	__builtin_unreachable();
}
