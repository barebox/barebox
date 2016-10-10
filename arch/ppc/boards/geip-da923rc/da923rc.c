/*
 * Copyright 2013 GE Intelligent Platforms, Inc.
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
 * GEIP DA923RC/GBX460 board support.
 */

#include <common.h>
#include <console.h>
#include <init.h>
#include <memory.h>
#include <driver.h>
#include <asm/io.h>
#include <net.h>
#include <gpio.h>
#include <envfs.h>
#include <platform_data/serial-ns16550.h>
#include <partition.h>
#include <environment.h>
#include <i2c/i2c.h>
#include <asm/fsl_ddr_sdram.h>
#include <asm/cache.h>
#include <mach/mmu.h>
#include <mach/mpc85xx.h>
#include <mach/immap_85xx.h>
#include <mach/gianfar.h>
#include <mach/gpio.h>
#include <mach/clock.h>
#include <mach/fsl_i2c.h>
#include "product_data.h"

static struct gfar_info_struct gfar_info[] = {
	{
	 .phyaddr = 7,
	 .tbiana = 0,
	 .tbicr = 0,
	 .mdiobus_tbi = 0,
	 },
};

struct i2c_platform_data i2cplat[] = {
	{
	 .bitrate = 400000,
	 },
	{
	 .bitrate = 400000,
	 },
};

static struct board_info binfo;

static int board_eth_init(void)
{
	void __iomem *gur = IOMEM(MPC85xx_GUTS_ADDR);
	struct ge_product_data product;
	int st;

	/* Toggle eth0 reset pin */
	gpio_set_value(4, 0);
	udelay(5);
	gpio_set_value(4, 1);

	/* Disable eTSEC3 */
	out_be32(gur + MPC85xx_DEVDISR_OFFSET,
		 in_be32(gur + MPC85xx_DEVDISR_OFFSET) &
		 ~MPC85xx_DEVDISR_TSEC3);

	st = ge_get_product_data(&product);
	if (((product.v2.mac.count > 0) && (product.v2.mac.count <= MAX_MAC))
	    && (st == 0))
		eth_register_ethaddr(0, (const char *)&product.v2.mac.mac[0]);

	fsl_eth_init(1, &gfar_info[0]);

	return 0;
}

static int da923rc_devices_init(void)
{
	add_cfi_flash_device(0, 0xfe000000, 32 << 20, 0);
	devfs_add_partition("nor0", 0x0, 0x8000, DEVFS_PARTITION_FIXED, "env0");
	devfs_add_partition("nor0", 0x1f80000, 8 << 16, DEVFS_PARTITION_FIXED,
			    "self0");
	add_generic_device("i2c-fsl", 0, NULL, I2C1_BASE_ADDR, 0x100,
			   IORESOURCE_MEM, &i2cplat[0]);
	add_generic_device("i2c-fsl", 1, NULL, I2C2_BASE_ADDR, 0x100,
			   IORESOURCE_MEM, &i2cplat[1]);

	board_eth_init();

	if (IS_ENABLED(CONFIG_DEFAULT_ENVIRONMENT_GENERIC))
		defaultenv_append_directory(defaultenv_geip_da923rc);

	return 0;
}

device_initcall(da923rc_devices_init);

static struct NS16550_plat serial_plat = {
	.clock = 0,
	.shift = 0,
};

static int da923rc_console_init(void)
{
	if (binfo.bid == BOARD_TYPE_DA923)
		barebox_set_model("DA923RC");
	else if (binfo.bid == BOARD_TYPE_GBX460)
		barebox_set_model("GBX460");
	else
		barebox_set_model("unknown");

	serial_plat.clock = fsl_get_bus_freq(0);
	add_ns16550_device(1, CFG_CCSRBAR + 0x4600, 16,
			   IORESOURCE_MEM | IORESOURCE_MEM_8BIT,
			   &serial_plat);
	return 0;
}

console_initcall(da923rc_console_init);

static int da923rc_mem_init(void)
{
	barebox_add_memory_bank("ram0", 0x0, fsl_get_effective_memsize());
	return 0;
}

mem_initcall(da923rc_mem_init);

static int da923rc_board_init_r(void)
{
	void __iomem *lbc = LBC_BASE_ADDR;
	void __iomem *ecm = IOMEM(MPC85xx_ECM_ADDR);
	void __iomem *pci = IOMEM(PCI1_BASE_ADDR);
	const unsigned int flashbase = (BOOT_BLOCK + 0x2000000);
	uint8_t flash_esel;

	da923rc_boardinfo_get(&binfo);

	flush_dcache();
	invalidate_icache();

	/* Clear LBC error interrupts */
	out_be32(lbc + FSL_LBC_LTESR_OFFSET, 0xffffffff);
	/* Enable LBC error interrupts */
	out_be32(lbc + FSL_LBC_LTEIR_OFFSET, 0xffffffff);
	/* Clear ecm errors */
	out_be32(ecm + MPC85xx_ECM_EEDR_OFFSET, 0xffffffff);
	/* Enable ecm errors */
	out_be32(ecm + MPC85xx_ECM_EEER_OFFSET, 0xffffffff);

	/* Re-map boot flash */
	fsl_set_lbc_br(0, BR_PHYS_ADDR(0xfe000000) | BR_PS_16 | BR_V);
	fsl_set_lbc_or(0, 0xfe006e21);

	/* Invalidate TLB entry for boot block */
	flash_esel = e500_find_tlb_idx((void *)flashbase, 1);
	e500_disable_tlb(flash_esel);
	flash_esel = e500_find_tlb_idx((void *)(flashbase + 0x1000000), 1);
	e500_disable_tlb(flash_esel);

	/* Boot block back to cache inhibited. */
	e500_set_tlb(1, BOOT_BLOCK + (2 * 0x1000000),
		     BOOT_BLOCK + (2 * 0x1000000),
		     MAS3_SX | MAS3_SW | MAS3_SR, MAS2_I | MAS2_G | MAS2_M,
		     0, 2, BOOKE_PAGESZ_16M, 1);
	e500_set_tlb(1, BOOT_BLOCK + (3 * 0x1000000),
		     BOOT_BLOCK + (3 * 0x1000000),
		     MAS3_SX | MAS3_SW | MAS3_SR, MAS2_I | MAS2_G | MAS2_M,
		     0, 3, BOOKE_PAGESZ_16M, 1);

	fsl_l2_cache_init();

	fsl_enable_gpiout();
	/* Enable NOR low voltage programming (gpio 2) and write (gpio 3). */
	gpio_set_value(2, 1);
	gpio_set_value(3, 1);

	/* Enable write to NAND flash */
	if (binfo.bid == BOARD_TYPE_GBX460) {
		/* Map CPLD */
		fsl_set_lbc_br(3, BR_PHYS_ADDR(0xfc010000) | BR_PS_16 | BR_V);
		fsl_set_lbc_or(3, 0xffffe001);
		/* Enable all reset */
		out_be16(IOMEM(0xfc010044), 0xffff);
		gpio_set_value(6, 1);
	}

	/* Board reset and PHY reset. Disable CS3. */
	if (binfo.bid == BOARD_TYPE_DA923) {
		gpio_set_value(0, 0);
		gpio_set_value(1, 1);
		/* De-assert Board reset */
		udelay(1000);
		gpio_set_value(0, 1);
	}

	/* Enable PCI error reporting */
	out_be32(pci + 0xe00, 0x80000040);
	out_be32(pci + 0xe08, 0x6bf);
	out_be32(pci + 0xe0c, 0xbb1fa001);
	/* 32-bytes cacheline size */
	out_be32(pci, 0x8000000c);
	out_le32(pci + 4, 0x00008008);

	return 0;
}

postcore_initcall(da923rc_board_init_r);
