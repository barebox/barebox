// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2008 Raghavendra KH <r-khandenahally@ti.com>, Texas Instruments (http://www.ti.com/)
// SPDX-FileCopyrightText: 2012 Jan Luebbe <j.luebbe@pengutronix.de>

/**
 * @file
 * @brief OnRISC Baltos Specific Board Initialization routines
 */

#include <common.h>
#include <init.h>
#include <driver.h>
#include <envfs.h>
#include <environment.h>
#include <globalvar.h>
#include <linux/sizes.h>
#include <net.h>
#include <bootsource.h>
#include <asm/armlinux.h>
#include <generated/mach-types.h>
#include <mach/am33xx-generic.h>
#include <mach/am33xx-silicon.h>
#include <mach/sys_info.h>
#include <mach/syslib.h>
#include <mach/gpmc.h>
#include <linux/err.h>
#include <mach/bbu.h>
#include <libfile.h>
#include <gpio.h>

static struct omap_barebox_part baltos_barebox_part = {
	.nand_offset = SZ_512K,
	.nand_size = 0x1e0000,
};

#ifndef CONFIG_OMAP_BUILD_IFT
struct bsp_vs_hwparam {
	uint32_t Magic;
	uint32_t HwRev;
	uint32_t SerialNumber;
	char PrdDate[11];
	uint16_t SystemId;
	uint8_t MAC1[6];
	uint8_t MAC2[6];
	uint8_t MAC3[6];
} __attribute__ ((packed));

static uint8_t get_dip_switch(uint16_t id, uint32_t rev)
{
	uint16_t maj, min;
	uint8_t dip = 0;

	maj = rev >> 16;
	min = rev & 0xffff;

	if ((id == 220 || id == 222) && (maj == 1 && min == 2))
		id = 214;

	switch(id) {
		case 214:
		case 215:
			dip = !gpio_get_value(44);
			dip += !gpio_get_value(45) << 1;
			dip += !gpio_get_value(46) << 2;
			dip += !gpio_get_value(47) << 3;
			break;
		case 212:
		case 221:
		case 223:
		case 224:
		case 225:
		case 226:
		case 227:
		case 230:
			dip = !gpio_get_value(82);
			dip += !gpio_get_value(83) << 1;
			dip += !gpio_get_value(105) << 2;
			dip += !gpio_get_value(106) << 3;
			break;
	}

	return dip;
}

static int baltos_read_eeprom(void)
{
	struct bsp_vs_hwparam hw_param;
	size_t size;
	char *buf, var_buf[32];
	int rc;
	unsigned char mac_addr[6];
	uint8_t dip;

	if (!of_machine_is_compatible("vscom,onrisc"))
		return 0;

	rc = read_file_2("/dev/eeprom0",
			 &size,
			 (void *)&buf,
			 sizeof(hw_param));
	if (rc && rc != -EFBIG)
		return rc;

	memcpy(&hw_param, buf, sizeof(hw_param));

	free(buf);

	if (hw_param.Magic == 0xDEADBEEF) {
		/* setup MAC1 */
		mac_addr[0] = hw_param.MAC1[0];
		mac_addr[1] = hw_param.MAC1[1];
		mac_addr[2] = hw_param.MAC1[2];
		mac_addr[3] = hw_param.MAC1[3];
		mac_addr[4] = hw_param.MAC1[4];
		mac_addr[5] = hw_param.MAC1[5];

		eth_register_ethaddr(0, mac_addr);

		/* setup MAC2 */
		mac_addr[0] = hw_param.MAC2[0];
		mac_addr[1] = hw_param.MAC2[1];
		mac_addr[2] = hw_param.MAC2[2];
		mac_addr[3] = hw_param.MAC2[3];
		mac_addr[4] = hw_param.MAC2[4];
		mac_addr[5] = hw_param.MAC2[5];

		eth_register_ethaddr(1, mac_addr);
	} else {
		printf("Baltos: incorrect magic number (0x%x) "
		       "in EEPROM\n",
		       hw_param.Magic);

		hw_param.SystemId = 0;
	}

	sprintf(var_buf, "%d", hw_param.SystemId);
	globalvar_add_simple("board.id", var_buf);

	/* enable mPCIe slot */
	gpio_direction_output(100, 1);

	/* configure output signals of the external GPIO controller */
	if (hw_param.SystemId == 210 || hw_param.SystemId == 211) {
		gpio_direction_output(132, 0);
		gpio_direction_output(133, 0);
		gpio_direction_output(134, 0);
		gpio_direction_output(135, 0);
	}

	dip = get_dip_switch(hw_param.SystemId, hw_param.HwRev);
	sprintf(var_buf, "%02x", dip);
	globalvar_add_simple("board.dip", var_buf);

	return 0;
}
environment_initcall(baltos_read_eeprom);
#endif

static int baltos_devices_init(void)
{
	if (!of_machine_is_compatible("vscom,onrisc"))
		return 0;

	globalvar_add_simple("board.variant", "baltos");

	if (bootsource_get() == BOOTSOURCE_MMC)
		omap_set_bootmmc_devname("mmc0");

	omap_set_barebox_part(&baltos_barebox_part);

	if (IS_ENABLED(CONFIG_SHELL_NONE))
		return am33xx_of_register_bootdevice();

	return 0;
}
coredevice_initcall(baltos_devices_init);
