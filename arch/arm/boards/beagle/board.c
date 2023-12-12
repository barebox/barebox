// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2008 Raghavendra KH <r-khandenahally@ti.com>, Texas Instruments (http://www.ti.com/)

#include <common.h>
#include <console.h>
#include <init.h>
#include <driver.h>
#include <linux/sizes.h>
#include <io.h>
#include <bbu.h>
#include <filetype.h>
#include <envfs.h>
#include <asm/armlinux.h>
#include <asm/mach-types.h>
#include <mach/omap/gpmc.h>
#include <mach/omap/gpmc_nand.h>
#include <mach/omap/ehci.h>
#include <mach/omap/omap3-devices.h>
#include <i2c/i2c.h>
#include <linux/err.h>
#include <linux/usb/ehci.h>
#include <asm/barebox-arm.h>

#ifdef CONFIG_DRIVER_SERIAL_NS16550

/**
 * @brief UART serial port initialization - remember to enable COM clocks in
 * arch
 *
 * @return result of device registration
 */
static int beagle_console_init(void)
{
	if (barebox_arm_machine() != MACH_TYPE_OMAP3_BEAGLE)
		return 0;

	barebox_set_model("Texas Instruments beagle");
	barebox_set_hostname("beagle");

	omap3_add_uart3();

	return 0;
}
console_initcall(beagle_console_init);
#endif /* CONFIG_DRIVER_SERIAL_NS16550 */

#ifdef CONFIG_USB_EHCI_OMAP
static struct omap_hcd omap_ehci_pdata = {
	.port_mode[0] = EHCI_HCD_OMAP_MODE_PHY,
	.port_mode[1] = EHCI_HCD_OMAP_MODE_PHY,
	.port_mode[2] = EHCI_HCD_OMAP_MODE_UNKNOWN,
	.phy_reset  = 1,
	.reset_gpio_port[0]  = -EINVAL,
	.reset_gpio_port[1]  = 147,
	.reset_gpio_port[2]  = -EINVAL
};

static struct ehci_platform_data ehci_pdata = {
	.flags = 0,
};
#endif /* CONFIG_USB_EHCI_OMAP */

static struct i2c_board_info i2c_devices[] = {
	{
		I2C_BOARD_INFO("twl4030", 0x48),
	},
};

static struct gpmc_nand_platform_data nand_plat = {
	.device_width = 16,
	.ecc_mode = OMAP_ECC_HAMMING_CODE_HW_ROMCODE,
	.nand_cfg = &omap3_nand_cfg,
};

static int beagle_mem_init(void)
{
	if (barebox_arm_machine() != MACH_TYPE_OMAP3_BEAGLE)
		return 0;

	omap_add_ram0(SZ_128M);

	return 0;
}
mem_initcall(beagle_mem_init);

static int beagle_devices_init(void)
{
	if (barebox_arm_machine() != MACH_TYPE_OMAP3_BEAGLE)
		return 0;

	i2c_register_board_info(0, i2c_devices, ARRAY_SIZE(i2c_devices));
	omap3_add_i2c1(NULL);

#ifdef CONFIG_USB_EHCI_OMAP
	if (ehci_omap_init(&omap_ehci_pdata) >= 0)
		omap3_add_ehci(&ehci_pdata);
#endif /* CONFIG_USB_EHCI_OMAP */
#ifdef CONFIG_OMAP_GPMC
	/* WP is made high and WAIT1 active Low */
	gpmc_generic_init(0x10);
#endif
	omap_add_gpmc_nand_device(&nand_plat);

	omap3_add_mmc1(NULL);

	armlinux_set_architecture(MACH_TYPE_OMAP3_BEAGLE);

	bbu_register_std_file_update("nand-xload", 0,
			"/dev/nand0.xload.bb", filetype_ch_image);
	bbu_register_std_file_update("nand", 0,
			"/dev/nand0.barebox.bb", filetype_arm_barebox);

	defaultenv_append_directory(defaultenv_beagle);

	return 0;
}
device_initcall(beagle_devices_init);
