// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <init.h>
#include <io.h>
#include <bbu.h>
#include <mach/arria10-system-manager.h>

static int aa1_init(void)
{
	int pbl_index = 0;
	uint32_t flag_barebox1 = 0;
	uint32_t flag_barebox2 = 0;

	if (!of_machine_is_compatible("enclustra,mercury-aa1"))
		return 0;

	pbl_index = readl(ARRIA10_SYSMGR_ROM_INITSWLASTLD);

	pr_debug("Current barebox instance %d\n", pbl_index);

	switch (pbl_index) {
	case 0:
		flag_barebox1 |= BBU_HANDLER_FLAG_DEFAULT;
		break;
	case 1:
		flag_barebox2 |= BBU_HANDLER_FLAG_DEFAULT;
		break;
	};

	bbu_register_std_file_update("emmc-barebox1-xload", flag_barebox1,
					"/dev/mmc0.barebox1-xload",
					filetype_socfpga_xload);

	bbu_register_std_file_update("emmc-barebox1", 0,
					"/dev/mmc0.barebox1",
					filetype_arm_barebox);

	bbu_register_std_file_update("emmc-barebox2-xload", flag_barebox2,
					"/dev/mmc0.barebox2-xload",
					filetype_socfpga_xload);

	bbu_register_std_file_update("emmc-barebox2", 0,
					"/dev/mmc0.barebox2",
					filetype_arm_barebox);
	return 0;
}
postcore_initcall(aa1_init);
