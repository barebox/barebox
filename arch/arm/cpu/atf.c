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

struct bl2_to_bl31_params_mem_v2 *bl2_plat_get_bl31_params_v2(uintptr_t bl32_entry,
					uintptr_t bl33_entry, uintptr_t fdt_addr)
{
	static struct bl2_to_bl31_params_mem_v2 p = {
		.bl_params = {
			.h = {
				.type = ATF_PARAM_BL_PARAMS,
				.version = ATF_VERSION_2,
				.size = sizeof(struct bl_params),
				.attr = 0,
			},
			.head = &p.bl31_params_node,
		},
		.bl31_params_node = {
			.image_id = ATF_BL31_IMAGE_ID,
			.image_info = &p.bl31_image_info,
			.ep_info = &p.bl31_ep_info,
			.next_params_info = &p.bl32_params_node,
		},
		.bl32_params_node = {
			.image_id = ATF_BL32_IMAGE_ID,
			.image_info = &p.bl32_image_info,
			.ep_info = &p.bl32_ep_info,
			.next_params_info = &p.bl33_params_node,
		},
		.bl33_params_node = {
			.image_id = ATF_BL33_IMAGE_ID,
			.image_info = &p.bl33_image_info,
			.ep_info = &p.bl33_ep_info,
			.next_params_info = NULL,
		},
		.bl32_ep_info = {
			.h = {
				.type = ATF_PARAM_EP,
				.version = ATF_VERSION_2,
				.size = sizeof(struct entry_point_info),
				.attr = ATF_EP_SECURE,
			},
			.spsr = SPSR_64(MODE_EL1, MODE_SP_ELX, DISABLE_ALL_EXECPTIONS),
		},
		.bl33_ep_info = {
			.h = {
				.type = ATF_PARAM_EP,
				.version = ATF_VERSION_2,
				.size = sizeof(struct entry_point_info),
				.attr = ATF_EP_NON_SECURE,
			},
			.spsr = SPSR_64(MODE_EL2, MODE_SP_ELX, DISABLE_ALL_EXECPTIONS),
		},
		.bl33_image_info = {
			.h = {
				.type = ATF_PARAM_IMAGE_BINARY,
				.version = ATF_VERSION_2,
				.size = sizeof(struct atf_image_info),
				.attr = 0,
			},
		},
		.bl32_image_info = {
			.h = {
				.type = ATF_PARAM_IMAGE_BINARY,
				.version = ATF_VERSION_2,
				.size = sizeof(struct atf_image_info),
				.attr = ATF_EP_SECURE,
			},
		},
		.bl31_image_info = {
			.h = {
				.type = ATF_PARAM_IMAGE_BINARY,
				.version = ATF_VERSION_2,
				.size = sizeof(struct atf_image_info),
				.attr = 0,
			},
		},
	};

	p.bl33_ep_info.args.arg0 = 0xffff & read_mpidr();
	p.bl33_ep_info.pc = bl33_entry;
	p.bl32_ep_info.args.arg3 = fdt_addr;
	p.bl32_ep_info.pc = bl32_entry;

	return &p;
}

void bl31_entry_v2(uintptr_t bl31_entry, struct bl_params *params, void *fdt_addr)
{
	void (*atf_entry)(struct bl_params *params, uintptr_t plat_params);

	raw_write_daif(SPSR_EXCEPTION_MASK);

	atf_entry = (void *)bl31_entry;

	atf_entry(params, (uintptr_t)fdt_addr);
}
