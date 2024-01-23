// SPDX-License-Identifier: GPL-2.0-or-later

/*
 * Copyright (C) 2014 Andrey Panov <rockford@yandex.ru>
 *
 * based on arch/arm/boards/freescale-mx6-sabresd/board.c
 * Copyright (C) 2013 Hubert Feurstein <h.feurstein@gmail.com>
 *
 * based on arch/arm/boards/freescale-mx6-sabrelite/board.c
 * Copyright (C) 2012 Steffen Trumtrar, Pengutronix
 */

#include <common.h>
#include <init.h>
#include <environment.h>
#include <mach/imx/imx6-regs.h>
#include <asm/armlinux.h>
#include <asm/mach-types.h>
#include <linux/phy.h>
#include <asm/io.h>
#include <asm/mmu.h>
#include <mach/imx/generic.h>
#include <linux/sizes.h>
#include <net.h>
#include <mach/imx/imx6.h>
#include <mach/imx/devices-imx6.h>
#include <mach/imx/iomux-mx6.h>
#include <spi/spi.h>
#include <mach/imx/spi.h>
#include <mach/imx/usb.h>
#include <envfs.h>
#include <bootsource.h>
#include <bbu.h>
#include <mach/imx/bbu.h>

#define PHY_ID_RTL8211E	0x001cc915
#define PHY_ID_MASK	0xffffffff

/*
 * This should reset a PHY. Taken from E9 U-Boot/Linux source.
 */
static int rtl8211e_phy_fixup(struct phy_device *dev)
{
	phy_write(dev, 0x00, 0x3140);
	mdelay(10);
	phy_write(dev, 0x00, 0x3340);
	mdelay(10);

	return 0;
}

static int e9_devices_init(void)
{
	int ret;
	char *environment_path;

	if (!of_machine_is_compatible("embedsky,e9"))
		return 0;

	armlinux_set_architecture(3980);

	environment_path = basprintf("/chosen/environment-mmc%d",
				       bootsource_get_instance());

	ret = of_device_enable_path(environment_path);

	if (ret < 0)
		pr_warn("Failed to enable environment partition '%s' (%d)\n",
			environment_path, ret);

	free(environment_path);

	defaultenv_append_directory(defaultenv_e9);

	imx6_bbu_internal_mmc_register_handler("sd", "/dev/mmc1",
		BBU_HANDLER_FLAG_DEFAULT);

	imx6_bbu_internal_mmc_register_handler("emmc", "/dev/mmc3",
		BBU_HANDLER_FLAG_DEFAULT);

	return 0;
}
device_initcall(e9_devices_init);

static int e9_coredevices_init(void)
{
	if (!of_machine_is_compatible("embedsky,e9"))
		return 0;

	phy_register_fixup_for_uid(PHY_ID_RTL8211E, PHY_ID_MASK,
			rtl8211e_phy_fixup);

	return 0;
}
coredevice_initcall(e9_coredevices_init);
