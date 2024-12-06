// SPDX-License-Identifier: GPL-2.0-only
#include <common.h>
#include <init.h>
#include <asm/optee.h>
#include <asm-generic/memory_layout.h>
#include <mach/rockchip/rockchip.h>
#include <mach/rockchip/bootrom.h>

static int __rockchip_soc;

int rockchip_soc(void)
{
	if (__rockchip_soc)
		return __rockchip_soc;

	if (of_machine_is_compatible("rockchip,rk3188"))
		__rockchip_soc = 3188;
	else if (of_machine_is_compatible("rockchip,rk3288"))
		__rockchip_soc = 3288;
	else if (of_machine_is_compatible("rockchip,rk3566"))
		__rockchip_soc = 3566;
	else if (of_machine_is_compatible("rockchip,rk3568"))
		__rockchip_soc = 3568;
	else if (of_machine_is_compatible("rockchip,rk3588"))
		__rockchip_soc = 3588;

	return __rockchip_soc;
}

static int rockchip_init(void)
{
	const struct optee_header *hdr = rk_scratch_get_optee_hdr();

	if (IS_ENABLED(CONFIG_PBL_OPTEE) && optee_verify_header(hdr) == 0) {
		static struct of_optee_fixup_data optee_fixup_data = {
			.shm_size = OPTEE_SHM_SIZE,
			.method = "smc",
		};

		optee_set_membase(hdr);
		of_optee_fixup(of_get_root_node(), &optee_fixup_data);
		of_register_fixup(of_optee_fixup, &optee_fixup_data);
	}

	switch (rockchip_soc()) {
	case 3188:
		return rk3188_init();
	case 3288:
		return rk3288_init();
	case 3566:
		return rk3568_init();
	case 3568:
		return rk3568_init();
	case 3588:
		return rk3588_init();
	}

	return 0;
}
postcore_initcall(rockchip_init);
