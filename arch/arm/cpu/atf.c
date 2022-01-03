// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <asm/atf_common.h>
#include <asm/system.h>

static inline void raw_write_daif(unsigned int daif)
{
	__asm__ __volatile__("msr DAIF, %0\n\t" : : "r" (daif) : "memory");
}

void bl31_entry(uintptr_t bl31_entry, uintptr_t bl32_entry,
		uintptr_t bl33_entry, uintptr_t fdt_addr)
{
	struct atf_image_info bl31_image_info = {
		.h = {
			.type = ATF_PARAM_IMAGE_BINARY,
			.version = ATF_VERSION_1,
			.size = sizeof(bl31_image_info),
		},
	};
	struct atf_image_info bl32_image_info = {
		.h = {
			.type = ATF_PARAM_IMAGE_BINARY,
			.version = ATF_VERSION_1,
			.size = sizeof(bl32_image_info),
		},
	};
	struct entry_point_info bl32_ep_info = {
		.h = {
			.type = ATF_PARAM_EP,
			.version = ATF_VERSION_1,
			.attr = ATF_EP_SECURE,
			.size = sizeof(bl32_ep_info),
		},
		.pc = bl32_entry,
		.spsr = SPSR_64(MODE_EL1, MODE_SP_ELX, DISABLE_ALL_EXECPTIONS),
		.args = {
			.arg3 = fdt_addr,
		},
	};
	struct atf_image_info bl33_image_info = {
		.h = {
			.type = ATF_PARAM_IMAGE_BINARY,
			.version = ATF_VERSION_1,
			.size = sizeof(bl33_image_info),
		},
	};
	struct entry_point_info bl33_ep_info = {
		.h = {
			.type = ATF_PARAM_EP,
			.version = ATF_VERSION_1,
			.attr = ATF_EP_NON_SECURE,
			.size = sizeof(bl33_ep_info),
		},
		.pc = bl33_entry,
		.spsr = SPSR_64(MODE_EL2, MODE_SP_ELX, DISABLE_ALL_EXECPTIONS),
		.args = {
			/* BL33 expects to receive the primary CPU MPID (through x0) */
			.arg0 = 0xffff & read_mpidr(),
		},
	};
	struct bl31_params bl31_params = {
		.h = {
			.type = ATF_PARAM_BL31,
			.version = ATF_VERSION_1,
			.size = sizeof(bl31_params),
		},
		.bl31_image_info = &bl31_image_info,
		.bl32_ep_info = &bl32_ep_info,
		.bl32_image_info = &bl32_image_info,
		.bl33_ep_info = &bl33_ep_info,
		.bl33_image_info = &bl33_image_info,
	};
	void (*atf_entry)(struct bl31_params *params, uintptr_t plat_params);

	raw_write_daif(SPSR_EXCEPTION_MASK);

	atf_entry = (void *)bl31_entry;

	atf_entry(&bl31_params, fdt_addr);
}
