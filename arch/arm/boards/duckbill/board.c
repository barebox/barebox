// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2010 Juergen Beisert <kernel@pengutronix.de>, Pengutronix
// SPDX-FileCopyrightText: 2011 Marc Kleine-Budde <mkl@pengutronix.de>, Pengutronix
// SPDX-FileCopyrightText: 2011 Wolfram Sang <w.sang@pengutronix.de>, Pengutronix

#include <common.h>
#include <environment.h>
#include <errno.h>
#include <gpio.h>
#include <init.h>
#include <io.h>
#include <net.h>

#include <mach/clock.h>
#include <mach/imx-regs.h>
#include <mach/iomux-imx28.h>
#include <mach/iomux.h>
#include <mach/ocotp.h>
#include <mach/devices.h>
#include <mach/usb.h>
#include <usb/fsl_usb2.h>

#include <asm/armlinux.h>
#include <asm/mmu.h>

static void duckbill_get_ethaddr(void)
{
	u8 mac_ocotp[3], mac[6];
	int ret;

	ret = mxs_ocotp_read(mac_ocotp, 3, 0);
	if (ret != 3) {
		pr_err("Reading MAC from OCOTP failed!\n");
		return;
	}

	mac[0] = 0x00;
	mac[1] = 0x04;
	mac[2] = 0x9f;
	mac[3] = mac_ocotp[2];
	mac[4] = mac_ocotp[1];
	mac[5] = mac_ocotp[0];

	eth_register_ethaddr(0, mac);
}

static struct fsl_usb2_platform_data usb_pdata = {
	.operating_mode	= FSL_USB2_DR_DEVICE,
	.phy_mode	= FSL_USB2_PHY_UTMI,
};

static int duckbill_devices_init(void)
{
	if (!of_machine_is_compatible("i2se,duckbill"))
		return 0;

	duckbill_get_ethaddr(); /* must be after registering ocotp */

	imx28_usb_phy0_enable();
	add_generic_device("fsl-udc", DEVICE_ID_DYNAMIC, NULL, IMX_USB0_BASE,
			   0x200, IORESOURCE_MEM, &usb_pdata);

	return 0;
}
fs_initcall(duckbill_devices_init);

static int duckbill_console_init(void)
{
	if (!of_machine_is_compatible("i2se,duckbill"))
		return 0;

	barebox_set_model("I2SE Duckbill");
	barebox_set_hostname("duckbill");

	return 0;
}
console_initcall(duckbill_console_init);
