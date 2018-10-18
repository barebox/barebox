/*
 * Copyright 2018 (C) Pengutronix, Michael Grzeschik <mgr@pengutronix.de>
 *
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#define pr_fmt(fmt) "samx6i: " fmt

#include <malloc.h>
#include <envfs.h>
#include <environment.h>
#include <bootsource.h>
#include <common.h>
#include <init.h>
#include <of.h>
#include <mach/bbu.h>
#include <mach/esdctl.h>

#include <asm/armlinux.h>

#include "mem.h"

/*
 * On this board the SDRAM size is always configured by pin selection.
 */
static int samx6i_sdram_fixup(void)
{
	if (!(of_machine_is_compatible("kontron,imx6q-samx6i") ||
		of_machine_is_compatible("kontron,imx6dl-samx6i")))
		return 0;

	imx_esdctl_disable();

	return 0;
}
postcore_initcall(samx6i_sdram_fixup);

static int samx6i_mem_init(void)
{
	resource_size_t size = 0;

	if (!(of_machine_is_compatible("kontron,imx6q-samx6i") ||
		of_machine_is_compatible("kontron,imx6dl-samx6i")))
		return 0;

	size = samx6i_get_size();
	if (size)
		arm_add_mem_device("ram0", 0x10000000, size);

	return 0;
}
mem_initcall(samx6i_mem_init);

static int samx6i_devices_init(void)
{
	int ret;
	char *environment_path, *envdev;
	int flag_spi = 0, flag_mmc = 0;

	if (!(of_machine_is_compatible("kontron,imx6q-samx6i") ||
		of_machine_is_compatible("kontron,imx6dl-samx6i")))
		return 0;

	barebox_set_hostname("samx6i");

	switch (bootsource_get()) {
	case BOOTSOURCE_MMC:
		environment_path = basprintf("/chosen/environment-sd%d",
					       bootsource_get_instance() + 1);
		envdev = "MMC";
		flag_mmc = BBU_HANDLER_FLAG_DEFAULT;
		break;
	default:
		environment_path = basprintf("/chosen/environment-spinor");
		envdev = "SPI NOR flash";
		flag_spi = BBU_HANDLER_FLAG_DEFAULT;
		break;
	}

	ret = of_device_enable_path(environment_path);
	if (ret < 0)
		pr_warn("Failed to enable environment partition '%s' (%d)\n",
			environment_path, ret);
	free(environment_path);

	pr_notice("Using environment in %s\n", envdev);

	imx6_bbu_internal_spi_i2c_register_handler("m25p80",
					"/dev/m25p0.bootloader",
					flag_spi);

	imx6_bbu_internal_mmc_register_handler("mmc3",
					"/dev/mmc3.bootloader",
					flag_mmc);

	return 0;
}
device_initcall(samx6i_devices_init);
