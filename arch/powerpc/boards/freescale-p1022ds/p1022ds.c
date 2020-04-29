/*
 * Copyright 2014 GE Intelligent Platforms, Inc.
 * Copyright 2009-2011 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <init.h>
#include <driver.h>
#include <platform_data/serial-ns16550.h>
#include <net.h>
#include <types.h>
#include <i2c/i2c.h>
#include <partition.h>
#include <memory.h>
#include <envfs.h>
#include <asm/cache.h>
#include <asm/fsl_ddr_sdram.h>
#include <asm/fsl_law.h>
#include <mach/mpc85xx.h>
#include <mach/mmu.h>
#include <mach/immap_85xx.h>
#include <mach/gianfar.h>
#include <mach/clock.h>
#include <mach/early_udelay.h>

/* Define attributes for eTSEC1 and eTSEC2 */
static struct gfar_info_struct gfar_info[] = {
	{
		.phyaddr = 1,
		.tbiana = 0,
		.tbicr = 0,
		.mdiobus_tbi = 0,
	},
	{
		.phyaddr = 2,
		.tbiana = 0,
		.tbicr = 0,
		.mdiobus_tbi = 0,
	},
};

struct i2c_platform_data i2cplat[] = {
	{ .bitrate = 400000, },
	{ .bitrate = 400000, },
};

void p1022ds_lbc_early_init(void)
{
	void __iomem *gur = IOMEM(MPC85xx_GUTS_ADDR);
	void __iomem *lbc = LBC_BASE_ADDR;

	/* Set the local bus monitor timeout value to the maximum */
	clrsetbits_be32(lbc + FSL_LBC_LBCR_OFFSET, 0xff0f, 0xf);
	/* Set the pin muxing to enable ETSEC2. */
	clrbits_be32(gur + MPC85xx_GUTS_PMUXCR2_OFFSET, 0x001f8000);
	/* Set pmuxcr to allow both i2c1 and i2c2 */
	setbits_be32(gur + MPC85xx_GUTS_PMUXCR_OFFSET, 0x1000);

	/* Map the boot flash and FPGA */
	fsl_set_lbc_br(0, BR_PHYS_ADDR(CFG_FLASH_BASE_PHYS) | BR_PS_16 | BR_V);
	fsl_set_lbc_or(0, 0xf8000ff7);
	fsl_set_lbc_br(2, BR_PHYS_ADDR(CFG_PIXIS_BASE_PHYS) | BR_PS_8 | BR_V);
	fsl_set_lbc_or(2, 0xffff8ff7);
}

static void board_eth_init(void)
{
	struct i2c_adapter *adapter;
	struct i2c_client client;
	char mac[6];
	int ret, ix;

	adapter = i2c_get_adapter(1);
	client.addr = 0x57;
	client.adapter = adapter;

	for (ix = 0; ix < 2; ix++) {
		int mac_offset;

		mac_offset = 0x42 + (sizeof(mac) * ix);
		ret = i2c_read_reg(&client, mac_offset, mac, sizeof(mac));
		if (ret != sizeof(mac))
			pr_err("Fail to retrieve MAC address\n");
		else
			eth_register_ethaddr(ix, mac);
	}

	fsl_eth_init(1, &gfar_info[0]);
	fsl_eth_init(2, &gfar_info[1]);
}

static int p1022ds_devices_init(void)
{
	add_cfi_flash_device(DEVICE_ID_DYNAMIC, CFG_FLASH_BASE, 128 << 20, 0);
	devfs_add_partition("nor0", 0x7f80000, 0x80000, DEVFS_PARTITION_FIXED,
				    "self0");
	devfs_add_partition("nor0", 0x7f00000, 0x10000, DEVFS_PARTITION_FIXED,
				    "env0");
	add_generic_device("i2c-fsl", 0, NULL, I2C1_BASE_ADDR, 0x100,
			IORESOURCE_MEM, &i2cplat[0]);
	add_generic_device("i2c-fsl", 1, NULL, I2C2_BASE_ADDR, 0x100,
			IORESOURCE_MEM, &i2cplat[1]);

	board_eth_init();

	if (IS_ENABLED(CONFIG_DEFAULT_ENVIRONMENT_GENERIC))
		defaultenv_append_directory(defaultenv_freescale_p1022ds);

	return 0;
}

device_initcall(p1022ds_devices_init);

static struct NS16550_plat serial_plat = {
	.clock = 0,
	.shift = 0,
};

static int p1022ds_console_init(void)
{
	barebox_set_model("Freescale P1022DS");
	barebox_set_hostname("p1022ds");

	serial_plat.clock = fsl_get_bus_freq(0);
	add_ns16550_device(DEVICE_ID_DYNAMIC, CFG_IMMR + 0x4500, 16,
			   IORESOURCE_MEM | IORESOURCE_MEM_8BIT, &serial_plat);
	return 0;
}

console_initcall(p1022ds_console_init);

static int p1022ds_mem_init(void)
{
	barebox_add_memory_bank("ram0", 0x0, fsl_get_effective_memsize());
	return 0;
}

mem_initcall(p1022ds_mem_init);

static int p1022ds_board_init_r(void)
{
	void __iomem *fpga = IOMEM(CFG_PIXIS_BASE);
	const uint32_t flashbase = CFG_BOOT_BLOCK;
	const u8 flash_esel = e500_find_tlb_idx((void *)flashbase, 1);

	/* Enable SPI */
	out_8(fpga + 8, (in_8(fpga + 8) & ~(0xc0)) | (0x80));

	/* Map the NAND flash */
	fsl_set_lbc_br(1, BR_PHYS_ADDR(0xff800000) | BR_PS_8 |
			(2 << BR_DECC_SHIFT) | BR_MS_FCM | BR_V);
	fsl_set_lbc_or(1, 0xffff8796);

	/* Flush d-cache and invalidate i-cache of any FLASH data */
	flush_dcache();
	invalidate_icache();

	/* invalidate existing TLB entry for flash */
	e500_disable_tlb(flash_esel);

	/*
	 * Remap Boot flash region to caching-inhibited
	 * so that flash can be erased properly.
	 */
	e500_set_tlb(1, flashbase, CFG_BOOT_BLOCK_PHYS,
			MAS3_SX | MAS3_SW | MAS3_SR, MAS2_I | MAS2_G,
			0, flash_esel, BOOKE_PAGESZ_256M, 1);

	fsl_l2_cache_init();

	return 0;
}

core_initcall(p1022ds_board_init_r);
