// SPDX-License-Identifier: GPL-2.0-only

#include <init.h>
#include <envfs.h>
#include <bbu.h>

static int giantboard_device_init(void)
{
	bbu_register_std_file_update("microSD", BBU_HANDLER_FLAG_DEFAULT,
				     "/mnt/mmc1.0/barebox.bin",
				     filetype_arm_barebox);

	defaultenv_append_directory(defaultenv_giantboard);

	return 0;
}
device_initcall(giantboard_device_init);
