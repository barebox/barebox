// SPDX-License-Identifier: GPL-2.0-only

#define pr_fmt(fmt)     "layerscape-tfa: " fmt

#include <firmware.h>
#include <asm/atf_common.h>
#include <asm/cache.h>
#include <mach/layerscape/layerscape.h>
#include <mach/layerscape/xload.h>

void ls1046_start_tfa(void *barebox, struct dram_regions_info *dram_info)
{
	void (*bl31)(void) = (void *)0xfbe00000;
	size_t bl31_size;
	void *bl31_image;
	struct bl2_to_bl31_params_mem_v2 *params;

	get_builtin_firmware_ext(ls1046a_bl31_bin, barebox, &bl31_image, &bl31_size);
	memcpy(bl31, bl31_image, bl31_size);

	sync_caches_for_execution();

	/* Setup an initial stack for EL2 */
	asm volatile("msr sp_el2, %0" : : "r" ((unsigned long)barebox) : "cc");

	params = bl2_plat_get_bl31_params_v2(0, (uintptr_t)barebox, 0);
	params->bl31_ep_info.args.arg3 = (unsigned long)dram_info;

	pr_debug("Starting bl31\n");

	bl31_entry_v2((uintptr_t)bl31, &params->bl_params, NULL);
}
