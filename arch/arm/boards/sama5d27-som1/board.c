// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <linux/sizes.h>
#include <init.h>
#include <asm/memory.h>
#include <bbu.h>
#include <envfs.h>
#include <bootsource.h>
#include <of.h>

static int ek_device_init(void)
{
	int flags_sd = 0, flags_usd = 0;
	if (!of_machine_is_compatible("atmel,sama5d27-som1-ek"))
		return 0;

	if (bootsource_get() == BOOTSOURCE_MMC) {
		if (bootsource_get_instance() == 0) {
			flags_sd = BBU_HANDLER_FLAG_DEFAULT;
			of_device_enable_path("/chosen/environment-sd");
		} else {
			flags_usd = BBU_HANDLER_FLAG_DEFAULT;
			of_device_enable_path("/chosen/environment-microsd");
		}
	} else {
		of_device_enable_path("/chosen/environment-qspi");
	}

	bbu_register_std_file_update("SD", flags_sd, "/mnt/mmc0.0/barebox.bin",
				     filetype_arm_barebox);
	bbu_register_std_file_update("microSD", flags_usd, "/mnt/mmc1.0/barebox.bin",
				     filetype_arm_barebox);

	defaultenv_append_directory(defaultenv_sama5d27_som1);

	return 0;
}
device_initcall(ek_device_init);
