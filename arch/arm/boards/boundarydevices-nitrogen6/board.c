/*
 * Copyright (C) 2014 Lucas Stach, Pengutronix
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <common.h>
#include <init.h>
#include <environment.h>
#include <mach/bbu.h>
#include <linux/phy.h>
#include <linux/micrel_phy.h>
#include <mach/imx6.h>

static int nitrogen6x_devices_init(void)
{
	if (!of_machine_is_compatible("boundary,imx6dl-nitrogen6x") &&
	    !of_machine_is_compatible("boundary,imx6q-nitrogen6x") &&
	    !of_machine_is_compatible("boundary,imx6qp-nitrogen6_max"))
		return 0;

	imx6_bbu_internal_spi_i2c_register_handler("spiflash", "/dev/m25p0.barebox",
			BBU_HANDLER_FLAG_DEFAULT);

	if (of_machine_is_compatible("boundary,imx6qp-nitrogen6_max"))
		barebox_set_hostname("nitrogen6max");
	else
		barebox_set_hostname("nitrogen6x");

	return 0;
}
device_initcall(nitrogen6x_devices_init);

static int ksz9021rn_phy_fixup(struct phy_device *dev)
{
	phy_write(dev, 0x09, 0x0f00);

	/* do same as linux kernel */
	/* min rx data delay */
	phy_write(dev, 0x0b, 0x8105);
	phy_write(dev, 0x0c, 0x0000);

	/* max rx/tx clock delay, min rx/tx control delay */
	phy_write(dev, 0x0b, 0x8104);
	phy_write(dev, 0x0c, 0xf0f0);
	phy_write(dev, 0x0b, 0x104);

	return 0;
}

static int nitrogen6x_coredevices_init(void)
{
	if (!of_machine_is_compatible("boundary,imx6dl-nitrogen6x") &&
	    !of_machine_is_compatible("boundary,imx6q-nitrogen6x") &&
	    !of_machine_is_compatible("boundary,imx6qp-nitrogen6_max"))
		return 0;

	phy_register_fixup_for_uid(PHY_ID_KSZ9021, MICREL_PHY_ID_MASK,
					   ksz9021rn_phy_fixup);
	return 0;
}
coredevice_initcall(nitrogen6x_coredevices_init);
