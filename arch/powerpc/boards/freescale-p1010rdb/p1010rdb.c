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
#include <gpio.h>
#include <envfs.h>
#include <partition.h>
#include <memory.h>
#include <asm/cache.h>
#include <asm/fsl_ddr_sdram.h>
#include <asm/fsl_law.h>
#include <asm/fsl_ifc.h>
#include <mach/mpc85xx.h>
#include <mach/mmu.h>
#include <mach/immap_85xx.h>
#include <mach/gianfar.h>
#include <mach/clock.h>
#include <mach/early_udelay.h>
#include <of.h>

static struct gfar_info_struct gfar_info[] = {
	{
		.phyaddr = 1,
		.tbiana = 0,
		.tbicr = 0,
		.mdiobus_tbi = 0,
	},
	{
		.phyaddr = 0,
		.tbiana = 0x1a0,
		.tbicr = 0x9140,
		.mdiobus_tbi = 1,
	},
	{
		.phyaddr = 2,
		.tbiana = 0x1a0,
		.tbicr = 0x9140,
		.mdiobus_tbi = 2,
	},
};

struct i2c_platform_data i2cplat[] = {
	{ .bitrate = 400000, },
	{ .bitrate = 400000, },
};

void p1010rdb_early_init(void)
{
	void __iomem *ifc = IFC_BASE_ADDR;
	void __iomem *gur = IOMEM(MPC85xx_GUTS_ADDR);

	/* Clock configuration to access CPLD using IFC(GPCM) */
	setbits_be32(ifc + FSL_IFC_GCR_OFFSET,
			1 << IFC_GCR_TBCTL_TRN_TIME_SHIFT);

	/* Erratum A003549 */
	setbits_be32(gur + MPC85xx_GUTS_PMUXCR_OFFSET,
			MPC85xx_PMUXCR_LCLK_IFC_CS3);

	/* Update CS0 timings to access boot flash */
	set_ifc_ftim(IFC_CS0, IFC_FTIM0, FTIM0_NOR_TACSE(0x4) |
			FTIM0_NOR_TEADC(0x5) | FTIM0_NOR_TEAHC(0x5));
	set_ifc_ftim(IFC_CS0, IFC_FTIM1, FTIM1_NOR_TACO(0x1e) |
			FTIM1_NOR_TRAD_NOR(0x0f));
	set_ifc_ftim(IFC_CS0, IFC_FTIM2, FTIM2_NOR_TCS(0x4) |
			FTIM2_NOR_TCH(0x4) |  FTIM2_NOR_TWP(0x1c));
	set_ifc_ftim(IFC_CS0, IFC_FTIM3, 0);

	/* Map the CPLD */
	set_ifc_cspr(IFC_CS3, CSPR_PHYS_ADDR(CFG_CPLD_BASE_PHYS) |
			CSPR_PORT_SIZE_8 | CSPR_MSEL_GPCM | CSPR_V);
	set_ifc_csor(IFC_CS3, 0);
	set_ifc_amask(IFC_CS3, IFC_AMASK(64*1024));
	set_ifc_ftim(IFC_CS3, IFC_FTIM0, FTIM0_GPCM_TACSE(0xe) |
			FTIM0_GPCM_TEADC(0x0e) | FTIM0_GPCM_TEAHC(0x0e));
	set_ifc_ftim(IFC_CS3, IFC_FTIM1, FTIM1_GPCM_TACO(0x1e) |
			FTIM1_GPCM_TRAD(0x0f));
	set_ifc_ftim(IFC_CS3, IFC_FTIM2, FTIM2_GPCM_TCS(0xe) |
			FTIM2_GPCM_TCH(0) |  FTIM2_GPCM_TWP(0x1f));
	set_ifc_ftim(IFC_CS3, IFC_FTIM3, 0);

	/*  PCIe reset through GPIO 4 */
	gpio_direction_output(4, 1);
}

static void board_eth_init(void)
{
	fsl_eth_init(1, &gfar_info[0]);
	fsl_eth_init(2, &gfar_info[1]);
	fsl_eth_init(3, &gfar_info[2]);
}

static int p1010rdb_devices_init(void)
{
	add_cfi_flash_device(DEVICE_ID_DYNAMIC, CFG_FLASH_BASE, 32 << 20, 0);
	devfs_add_partition("nor0", 0x1f80000, 0x80000, DEVFS_PARTITION_FIXED,
				    "self0");
	devfs_add_partition("nor0", 0x1f60000, 0x10000, DEVFS_PARTITION_FIXED,
				    "env0");
	add_generic_device("i2c-fsl", 0, NULL, I2C1_BASE_ADDR, 0x100,
			IORESOURCE_MEM, &i2cplat[0]);
	add_generic_device("i2c-fsl", 1, NULL, I2C2_BASE_ADDR, 0x100,
			IORESOURCE_MEM, &i2cplat[1]);
	board_eth_init();

	if (IS_ENABLED(CONFIG_DEFAULT_ENVIRONMENT_GENERIC))
		defaultenv_append_directory(defaultenv_freescale_p1010rdb);

	return 0;
}

device_initcall(p1010rdb_devices_init);

static struct NS16550_plat serial_plat = {
	.clock = 0,
	.shift = 0,
};

static int p1010rdb_console_init(void)
{
	barebox_set_model("Freescale P1010RDB");
	barebox_set_hostname("p1010rdb");

	serial_plat.clock = fsl_get_bus_freq(0);
	add_ns16550_device(1, CFG_IMMR + 0x4500, 16,
			   IORESOURCE_MEM | IORESOURCE_MEM_8BIT, &serial_plat);
	return 0;
}

console_initcall(p1010rdb_console_init);

static int p1010rdb_mem_init(void)
{
	barebox_add_memory_bank("ram0", 0x0, fsl_get_effective_memsize());
	return 0;
}

mem_initcall(p1010rdb_mem_init);

static int p1010rdb_board_init_r(void)
{
	const uint32_t flashbase = CFG_BOOT_BLOCK;
	const u8 flash_esel = e500_find_tlb_idx((void *)flashbase, 1);

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

core_initcall(p1010rdb_board_init_r);

static int fdt_board_setup(struct device_node *blob, void *unused)
{
	struct device_node *node;

	node = of_find_compatible_node(blob, NULL, "fsl,esdhc");
	if (node)
		of_delete_node(node);

	node = of_find_compatible_node(blob, NULL, "fsl,starlite-tdm");
	if (node)
		of_delete_node(node);

	node = of_find_compatible_node(blob, NULL, "fsl,p1010-flexcan");
	if (node)
		of_delete_node(node);

	node = of_find_compatible_node(blob, NULL, "fsl,p1010-flexcan");
	if (node)
		of_delete_node(node);

	return 0;
}

static int of_register_p1010rdb_fixup(void)
{
	return of_register_fixup(fdt_board_setup, NULL);
}
late_initcall(of_register_p1010rdb_fixup);
