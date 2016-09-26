/*
 * Copyright 2012 GE Intelligent Platforms, Inc.
 * Copyright 2009-2011 Freescale Semiconductor, Inc.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
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

#define VSC7385_RST_SET		0x00080000
#define SLIC_RST_SET		0x00040000
#define SGMII_PHY_RST_SET	0x00020000
#define PCIE_RST_SET		0x00010000
#define RGMII_PHY_RST_SET	0x02000000

#define USB_RST_CLR		0x04000000

#define GPIO_DIR		0x060f0000

#define BOARD_PERI_RST_SET	(VSC7385_RST_SET | SLIC_RST_SET | \
				SGMII_PHY_RST_SET | PCIE_RST_SET | \
				RGMII_PHY_RST_SET)

#define SYSCLK_MASK	0x00200000
#define BOARDREV_MASK	0x10100000
#define BOARDREV_B	0x10100000
#define BOARDREV_C	0x00100000
#define BOARDREV_D	0x00000000

#define SYSCLK_66	66666666
#define SYSCLK_50	50000000
#define SYSCLK_100	100000000

/* Define attributes for eTSEC2 and eTSEC3 */
static struct gfar_info_struct gfar_info[] = {
	{
		.phyaddr = 0,
		.tbiana = 0x1a0,
		.tbicr = 0x9140,
		.mdiobus_tbi = 1,
	},
	{
		.phyaddr = 1,
		.tbiana = 0,
		.tbicr = 0,
		.mdiobus_tbi = 2,
	},
};

/* I2C busses. */
struct i2c_platform_data i2cplat = {
	.bitrate = 400000,
};

static int devices_init(void)
{
	add_cfi_flash_device(DEVICE_ID_DYNAMIC, CFG_FLASH_BASE, 16 << 20, 0);
	devfs_add_partition("nor0", 0xf60000, 0x8000, DEVFS_PARTITION_FIXED,
			"env0");
	devfs_add_partition("nor0", 0xf80000, 0x80000, DEVFS_PARTITION_FIXED,
			"self0");
	add_generic_device("i2c-fsl", 0, NULL, I2C1_BASE_ADDR,
			0x100, IORESOURCE_MEM, &i2cplat);
	add_generic_device("i2c-fsl", 1, NULL, I2C2_BASE_ADDR,
			0x100, IORESOURCE_MEM, &i2cplat);

	fsl_eth_init(2, &gfar_info[0]);
	fsl_eth_init(3, &gfar_info[1]);

	if (IS_ENABLED(CONFIG_DEFAULT_ENVIRONMENT_GENERIC))
		defaultenv_append_directory(defaultenv_freescale_p2020rdb);

	return 0;
}

device_initcall(devices_init);

static struct NS16550_plat serial_plat = {
	.clock = 0,
	.shift = 0,
};

static int p2020_console_init(void)
{
	barebox_set_model("Freescale P2020RDB");
	barebox_set_hostname("p2020rdb");

	serial_plat.clock = fsl_get_bus_freq(0);

	add_ns16550_device(DEVICE_ID_DYNAMIC, 0xffe04500, 16,
			   IORESOURCE_MEM | IORESOURCE_MEM_8BIT,
			   &serial_plat);
	return 0;
}

console_initcall(p2020_console_init);

static int mem_init(void)
{
	barebox_add_memory_bank("ram0", 0x0, 1024 << 20);

	return 0;
}
mem_initcall(mem_init);

/*
 * fixed_sdram: fixed sdram settings.
 */
phys_size_t fixed_sdram(void)
{
	void __iomem *regs = (void __iomem *)(MPC85xx_DDR_ADDR);
	int sdram_cfg = (SDRAM_CFG_MEM_EN | SDRAM_CFG_SREN |
			 SDRAM_CFG_SDRAM_TYPE_DDR2);
	phys_size_t dram_size;

	/* If already enabled (running from RAM), get out */
	if (in_be32(regs + DDR_OFF(SDRAM_CFG)) & SDRAM_CFG_MEM_EN)
		return fsl_get_effective_memsize();

	out_be32(regs + DDR_OFF(CS0_BNDS), CFG_SYS_DDR_CS0_BNDS);
	out_be32(regs + DDR_OFF(CS0_CONFIG), CFG_SYS_DDR_CS0_CONFIG);
	out_be32(regs + DDR_OFF(TIMING_CFG_3), CFG_SYS_DDR_TIMING_3);
	out_be32(regs + DDR_OFF(TIMING_CFG_0), CFG_SYS_DDR_TIMING_0);
	out_be32(regs + DDR_OFF(TIMING_CFG_1), CFG_SYS_DDR_TIMING_1);
	out_be32(regs + DDR_OFF(TIMING_CFG_2), CFG_SYS_DDR_TIMING_2);
	out_be32(regs + DDR_OFF(SDRAM_CFG_2), CFG_SYS_DDR_CONTROL2);
	out_be32(regs + DDR_OFF(SDRAM_MODE), CFG_SYS_DDR_MODE_1);
	out_be32(regs + DDR_OFF(SDRAM_MODE_2), CFG_SYS_DDR_MODE_2);
	out_be32(regs + DDR_OFF(SDRAM_MD_CNTL), CFG_SYS_MD_CNTL);
	/* Basic refresh rate (7.8us),high temp is 3.9us  */
	out_be32(regs + DDR_OFF(SDRAM_INTERVAL),
			CFG_SYS_DDR_INTERVAL);
	out_be32(regs + DDR_OFF(SDRAM_DATA_INIT),
			CFG_SYS_DDR_DATA_INIT);
	out_be32(regs + DDR_OFF(SDRAM_CLK_CNTL),
			CFG_SYS_DDR_CLK_CTRL);

	out_be32(regs + DDR_OFF(SDRAM_INIT_ADDR), 0);
	out_be32(regs + DDR_OFF(SDRAM_INIT_ADDR_EXT), 0);
	/*
	 * Wait 200us for the DDR clock to stabilize.
	 */
	early_udelay(200);
	asm volatile ("sync;isync");

	out_be32(regs + DDR_OFF(SDRAM_CFG), sdram_cfg);

	dram_size = fsl_get_effective_memsize();
	if (fsl_set_ddr_laws(0, dram_size, LAW_TRGT_IF_DDR) < 0)
		return 0;

	return dram_size;
}

unsigned long get_board_sys_clk(ulong dummy)
{
	u32 val_gpdat, sysclk_gpio, board_rev_gpio;
	void __iomem *gpio_regs = (void __iomem *)MPC85xx_GPIO_ADDR;

	val_gpdat = in_be32(gpio_regs + MPC85xx_GPIO_GPDAT);
	sysclk_gpio = val_gpdat & SYSCLK_MASK;
	board_rev_gpio = val_gpdat & BOARDREV_MASK;

	if (board_rev_gpio == BOARDREV_C) {
		if (sysclk_gpio == 0)
			return SYSCLK_66;
		else
			return SYSCLK_100;
	} else if (board_rev_gpio == BOARDREV_B) {
		if (sysclk_gpio == 0)
			return SYSCLK_66;
		else
			return SYSCLK_50;
	} else if (board_rev_gpio == BOARDREV_D) {
		if (sysclk_gpio == 0)
			return SYSCLK_66;
		else
			return SYSCLK_100;
	}
	return 0;
}

static void checkboard(void)
{
	u32 val_gpdat, board_rev_gpio;
	void __iomem *gpio_regs = (void __iomem *)MPC85xx_GPIO_ADDR;

	val_gpdat = in_be32(gpio_regs + MPC85xx_GPIO_GPDAT);
	board_rev_gpio = val_gpdat & BOARDREV_MASK;

	if ((board_rev_gpio != BOARDREV_C) && (board_rev_gpio != BOARDREV_B) &&
	    (board_rev_gpio != BOARDREV_D))
		panic("Unexpected Board REV %x detected!!\n", board_rev_gpio);

	setbits_be32((gpio_regs + MPC85xx_GPIO_GPDIR), GPIO_DIR);

	/*
	 * Bringing the following peripherals out of reset via GPIOs
	 * 0 = reset and 1 = out of reset
	 * GPIO12 - Reset to Ethernet Switch
	 * GPIO13 - Reset to SLIC/SLAC devices
	 * GPIO14 - Reset to SGMII_PHY_N
	 * GPIO15 - Reset to PCIe slots
	 * GPIO6  - Reset to RGMII PHY
	 * GPIO5  - Reset to USB3300 devices 1 = reset and 0 = out of reset
	 */
	clrsetbits_be32((gpio_regs + MPC85xx_GPIO_GPDAT), USB_RST_CLR,
			BOARD_PERI_RST_SET);
}

static int board_init_r(void)
{
	const unsigned int flashbase = CFG_FLASH_BASE;
	const u8 flash_esel = e500_find_tlb_idx((void *)flashbase, 1);

	checkboard();

	/* Map the whole boot flash */
	fsl_set_lbc_br(0, BR_PHYS_ADDR(CFG_FLASH_BASE_PHYS) | BR_PS_16 | BR_V);
	fsl_set_lbc_or(0, 0xff000ff7);

	/* Flush d-cache and invalidate i-cache of any FLASH data */
	flush_dcache();
	invalidate_icache();

	/* invalidate existing TLB entry for flash */
	e500_disable_tlb(flash_esel);

	/*
	 * Remap Boot flash region to caching-inhibited
	 * so that flash can be erased properly.
	 */
	e500_set_tlb(1, flashbase, CFG_FLASH_BASE_PHYS,
		MAS3_SX|MAS3_SW|MAS3_SR, MAS2_I|MAS2_G,
		0, flash_esel, BOOKE_PAGESZ_16M, 1);

	fsl_l2_cache_init();

	return 0;
}
core_initcall(board_init_r);
