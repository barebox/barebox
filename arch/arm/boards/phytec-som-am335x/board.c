/*
 * Copyright (C) 2015 Wadim Egorov, PHYTEC Messtechnik GmbH
 *
 * Device initialization for the following modules and board variants:
 *   - phyCORE: PCM-953, phyBOARD-MAIA, phyBOARD-WEGA
 *   - phyFLEX: PBA-B-01
 *   - phyCARD: PCA-A-XS1
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
 *
 */

#include <bootsource.h>
#include <common.h>
#include <nand.h>
#include <net.h>
#include <init.h>
#include <io.h>
#include <linux/sizes.h>
#include <envfs.h>
#include <state.h>
#include <asm/armlinux.h>
#include <generated/mach-types.h>
#include <linux/phy.h>
#include <linux/micrel_phy.h>
#include <mach/am33xx-generic.h>
#include <mach/am33xx-silicon.h>
#include <mach/bbu.h>

static int physom_coredevice_init(void)
{
	if (!of_machine_is_compatible("phytec,am335x-som"))
		return 0;

	am33xx_register_ethaddr(0, 0);

	return 0;
}
coredevice_initcall(physom_coredevice_init);

static struct omap_barebox_part physom_barebox_part = {
	.nand_offset = SZ_512K,
	.nand_size = SZ_512K,
	.nand_bkup_offset = SZ_1M,
	.nand_bkup_size = SZ_512K,
	.nor_offset = SZ_128K,
	.nor_size = SZ_512K,
};

static char *xloadslots[] = {
	"/dev/nand0.xload.bb",
	"/dev/nand0.xload_backup1.bb",
	"/dev/nand0.xload_backup2.bb",
	"/dev/nand0.xload_backup3.bb"
};

static char *nandslots[] = {
	"/dev/nand0.barebox.bb",
	"/dev/nand0.barebox_backup.bb",
};

#define ETH_COUNT 2
static const char *eth_names[ETH_COUNT] = {"mac0", "mac1"};

static int physom_devices_init(void)
{
	struct state *state;
	uint8_t mac[6];
	int state_ret;
	int state_i;

	if (!of_machine_is_compatible("phytec,am335x-som"))
		return 0;

	switch (bootsource_get()) {
	case BOOTSOURCE_SPI:
		of_device_enable_path("/chosen/environment-spi");
		break;
	case BOOTSOURCE_MMC:
		if (bootsource_get_instance() == 0)
			omap_set_bootmmc_devname("mmc0");
		else
			omap_set_bootmmc_devname("mmc1");
		break;
	default:
		of_device_enable_path("/chosen/environment-nand");
		break;
	}

	omap_set_barebox_part(&physom_barebox_part);
	defaultenv_append_directory(defaultenv_physom_am335x);

	/* Special module set up */
	if (of_machine_is_compatible("phytec,phycore-am335x-som")) {
		armlinux_set_architecture(MACH_TYPE_PCM051);
		barebox_set_hostname("pcm051");
	}

	if (of_machine_is_compatible("phytec,phyflex-am335x-som")) {
		armlinux_set_architecture(MACH_TYPE_PFLA03);
		am33xx_select_rmii2_crs_dv();
		barebox_set_hostname("pfla03");
	}

	if (of_machine_is_compatible("phytec,phycard-am335x-som")) {
		armlinux_set_architecture(MACH_TYPE_PCAAXS1);
		barebox_set_hostname("pcaaxs1");
	}

	/* Register update handler */
	am33xx_bbu_spi_nor_mlo_register_handler("MLO.spi", "/dev/m25p0.xload");
	am33xx_bbu_spi_nor_register_handler("spi", "/dev/m25p0.barebox");
	am33xx_bbu_nand_xloadslots_register_handler("MLO.nand",
		xloadslots, ARRAY_SIZE(xloadslots));
	am33xx_bbu_nand_slots_register_handler("nand", nandslots,
				ARRAY_SIZE(nandslots));
	am33xx_bbu_emmc_mlo_register_handler("MLO.emmc", "/dev/mmc1");

	state = state_by_name("am335x_phytec_mac_state");
	for (state_i = 0; state_i < 2; state_i++) {
		state_ret = state_read_mac(state, eth_names[state_i], &mac[0]);
		if (!state_ret && is_valid_ether_addr(&mac[0]))
			eth_register_ethaddr(state_i, mac);
	}

	if (IS_ENABLED(CONFIG_PHYTEC_SOM_AM335X_OF_AUTOENABLE)) {
		/* Enable NAND */
		of_autoenable_device_by_path("/ocp/gpmc@50000000");
		/* Enable eMMC */
		of_autoenable_device_by_path("/ocp/mmc@481d8000");
		/* Enable SPI NOR */
		of_autoenable_device_by_path("/ocp/spi@48030000/m25p80@0");

		of_autoenable_i2c_by_component("/ocp/i2c@44e0b000/temp@4b");
		of_autoenable_i2c_by_component("/ocp/i2c@44e0b000/eeprom@52");
		of_autoenable_i2c_by_component("/ocp/i2c@44e0b000/rtc@68");
	}

	if (IS_ENABLED(CONFIG_SHELL_NONE))
		return am33xx_of_register_bootdevice();

	return 0;
}
device_initcall(physom_devices_init);
