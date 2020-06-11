// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2018 Christoph Fritz <chf.fritz@googlemail.com>

#include <common.h>
#include <init.h>
#include <platform_data/eth-fec.h>
#include <bootsource.h>
#include <mach/bbu.h>

static int ar8035_phy_fixup(struct phy_device *dev)
{
	u16 val;

	/* Ar803x phy SmartEEE feature cause link status generates glitch,
	 * which cause ethernet link down/up issue, so disable SmartEEE
	 */
	phy_write(dev, 0xd, 0x3);
	phy_write(dev, 0xe, 0x805d);
	phy_write(dev, 0xd, 0x4003);

	val = phy_read(dev, 0xe);
	phy_write(dev, 0xe, val & ~BIT(8));

	/* To enable AR8031 ouput a 125MHz clk from CLK_25M */
	phy_write(dev, 0xd, 0x7);
	phy_write(dev, 0xe, 0x8016);
	phy_write(dev, 0xd, 0x4007);

	val = phy_read(dev, 0xe);
	val &= 0xffe3;
	val |= 0x18;
	phy_write(dev, 0xe, val);

	/* introduce tx clock delay */
	phy_write(dev, 0x1d, 0x5);
	val = phy_read(dev, 0x1e);
	val |= 0x0100;
	phy_write(dev, 0x1e, val);

	return 0;
}

static int advantech_mx6_devices_init(void)
{
	int ret;
	char *environment_path, *envdev;

	if (!of_machine_is_compatible("advantech,imx6dl-rom-7421"))
		return 0;

	phy_register_fixup_for_uid(0x004dd072, 0xffffffef, ar8035_phy_fixup);

	switch (bootsource_get()) {
	case BOOTSOURCE_MMC:
		environment_path = basprintf("/chosen/environment-sd%d",
					     bootsource_get_instance() + 1);
		if (bootsource_get_instance() + 1 == 4)
			envdev = "eMMC";
		else if (bootsource_get_instance() + 1 == 2)
			envdev = "microSD";
		else
			envdev = "MMC";
		break;
	case BOOTSOURCE_SPI:
		envdev = "SPI";
		environment_path = basprintf("/chosen/environment-spi");
		break;
	default:
		environment_path = basprintf("/chosen/environment-sd4");
		envdev = "MMC";
		break;
	}

	if (environment_path) {
		ret = of_device_enable_path(environment_path);
		if (ret < 0)
			pr_warn("Failed to enable env partition '%s' (%d)\n",
				environment_path, ret);
		free(environment_path);
	}

	pr_notice("Using environment in %s\n", envdev);

	imx6_bbu_internal_mmc_register_handler("mmc3", "/dev/mmc3",
						BBU_HANDLER_FLAG_DEFAULT);

	return 0;
}
device_initcall(advantech_mx6_devices_init);
