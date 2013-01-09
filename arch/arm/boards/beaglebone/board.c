/*
 * (C) Copyright 2008
 * Texas Instruments, <www.ti.com>
 * Raghavendra KH <r-khandenahally@ti.com>
 *
 * Copyright (C) 2012 Jan Luebbe <j.luebbe@pengutronix.de>
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

/**
 * @file
 * @brief BeagleBone Specific Board Initialization routines
 */

#include <common.h>
#include <console.h>
#include <init.h>
#include <driver.h>
#include <fs.h>
#include <linux/stat.h>
#include <environment.h>
#include <sizes.h>
#include <io.h>
#include <ns16550.h>
#include <net.h>
#include <asm/armlinux.h>
#include <generated/mach-types.h>
#include <mach/am33xx-silicon.h>
#include <mach/am33xx-clock.h>
#include <mach/sdrc.h>
#include <mach/sys_info.h>
#include <mach/syslib.h>
#include <mach/gpmc.h>
#include <mach/ehci.h>
#include <i2c/i2c.h>
#include <linux/err.h>
#include <linux/phy.h>
#include <usb/ehci.h>
#include <mach/xload.h>
#include <mach/am33xx-devices.h>
#include <mach/am33xx-mux.h>
#include <mach/wdt.h>
#include <mach/am33xx-generic.h>
#include <mach/cpsw.h>

/* UART Defines */
#define UART_SYSCFG_OFFSET  (0x54)
#define UART_SYSSTS_OFFSET  (0x58)

#define UART_RESET      (0x1 << 1)
#define UART_CLK_RUNNING_MASK   0x1
#define UART_SMART_IDLE_EN  (0x1 << 0x3)

/* AM335X EMIF Register values */
#define EMIF_SDMGT		0x80000000
#define EMIF_SDRAM		0x00004650
#define EMIF_PHYCFG		0x2
#define DDR_PHY_RESET		(0x1 << 10)
#define DDR_FUNCTIONAL_MODE_EN	0x1
#define DDR_PHY_READY		(0x1 << 2)
#define	VTP_CTRL_READY		(0x1 << 5)
#define VTP_CTRL_ENABLE		(0x1 << 6)
#define VTP_CTRL_LOCK_EN	(0x1 << 4)
#define VTP_CTRL_START_EN	(0x1)
#define DDR2_RATIO		0x80	/* for mDDR */
#define CMD_FORCE		0x00	/* common #def */
#define CMD_DELAY		0x00

#define EMIF_READ_LATENCY	0x05
#define EMIF_TIM1		0x0666B3D6
#define EMIF_TIM2		0x143731DA
#define	EMIF_TIM3		0x00000347
#define EMIF_SDCFG		0x43805332
#define EMIF_SDREF		0x0000081a
#define DDR2_DLL_LOCK_DIFF	0x0
#define DDR2_RD_DQS		0x12
#define DDR2_PHY_FIFO_WE	0x80

#define	DDR2_INVERT_CLKOUT	0x00
#define	DDR2_WR_DQS		0x00
#define	DDR2_PHY_WRLVL		0x00
#define	DDR2_PHY_GATELVL	0x00
#define	DDR2_PHY_WR_DATA	0x40
#define	PHY_RANK0_DELAY		0x01
#define PHY_DLL_LOCK_DIFF	0x0
#define DDR_IOCTRL_VALUE	0x18B

static void beaglebone_data_macro_config(int dataMacroNum)
{
	u32 BaseAddrOffset = 0x00;;

	if (dataMacroNum == 1)
		BaseAddrOffset = 0xA4;

	__raw_writel(((DDR2_RD_DQS<<30)|(DDR2_RD_DQS<<20)
		|(DDR2_RD_DQS<<10)|(DDR2_RD_DQS<<0)),
		(AM33XX_DATA0_RD_DQS_SLAVE_RATIO_0 + BaseAddrOffset));
	__raw_writel(DDR2_RD_DQS>>2,
		(AM33XX_DATA0_RD_DQS_SLAVE_RATIO_1 + BaseAddrOffset));
	__raw_writel(((DDR2_WR_DQS<<30)|(DDR2_WR_DQS<<20)
		|(DDR2_WR_DQS<<10)|(DDR2_WR_DQS<<0)),
		(AM33XX_DATA0_WR_DQS_SLAVE_RATIO_0 + BaseAddrOffset));
	__raw_writel(DDR2_WR_DQS>>2,
		(AM33XX_DATA0_WR_DQS_SLAVE_RATIO_1 + BaseAddrOffset));
	__raw_writel(((DDR2_PHY_WRLVL<<30)|(DDR2_PHY_WRLVL<<20)
		|(DDR2_PHY_WRLVL<<10)|(DDR2_PHY_WRLVL<<0)),
		(AM33XX_DATA0_WRLVL_INIT_RATIO_0 + BaseAddrOffset));
	__raw_writel(DDR2_PHY_WRLVL>>2,
		(AM33XX_DATA0_WRLVL_INIT_RATIO_1 + BaseAddrOffset));
	__raw_writel(((DDR2_PHY_GATELVL<<30)|(DDR2_PHY_GATELVL<<20)
		|(DDR2_PHY_GATELVL<<10)|(DDR2_PHY_GATELVL<<0)),
		(AM33XX_DATA0_GATELVL_INIT_RATIO_0 + BaseAddrOffset));
	__raw_writel(DDR2_PHY_GATELVL>>2,
		(AM33XX_DATA0_GATELVL_INIT_RATIO_1 + BaseAddrOffset));
	__raw_writel(((DDR2_PHY_FIFO_WE<<30)|(DDR2_PHY_FIFO_WE<<20)
		|(DDR2_PHY_FIFO_WE<<10)|(DDR2_PHY_FIFO_WE<<0)),
		(AM33XX_DATA0_FIFO_WE_SLAVE_RATIO_0 + BaseAddrOffset));
	__raw_writel(DDR2_PHY_FIFO_WE>>2,
		(AM33XX_DATA0_FIFO_WE_SLAVE_RATIO_1 + BaseAddrOffset));
	__raw_writel(((DDR2_PHY_WR_DATA<<30)|(DDR2_PHY_WR_DATA<<20)
		|(DDR2_PHY_WR_DATA<<10)|(DDR2_PHY_WR_DATA<<0)),
		(AM33XX_DATA0_WR_DATA_SLAVE_RATIO_0 + BaseAddrOffset));
	__raw_writel(DDR2_PHY_WR_DATA>>2,
		(AM33XX_DATA0_WR_DATA_SLAVE_RATIO_1 + BaseAddrOffset));
	__raw_writel(PHY_DLL_LOCK_DIFF,
		(AM33XX_DATA0_DLL_LOCK_DIFF_0 + BaseAddrOffset));
}

static void beaglebone_cmd_macro_config(void)
{
	__raw_writel(DDR2_RATIO, AM33XX_CMD0_CTRL_SLAVE_RATIO_0);
	__raw_writel(CMD_FORCE, AM33XX_CMD0_CTRL_SLAVE_FORCE_0);
	__raw_writel(CMD_DELAY, AM33XX_CMD0_CTRL_SLAVE_DELAY_0);
	__raw_writel(DDR2_DLL_LOCK_DIFF, AM33XX_CMD0_DLL_LOCK_DIFF_0);
	__raw_writel(DDR2_INVERT_CLKOUT, AM33XX_CMD0_INVERT_CLKOUT_0);

	__raw_writel(DDR2_RATIO, AM33XX_CMD1_CTRL_SLAVE_RATIO_0);
	__raw_writel(CMD_FORCE, AM33XX_CMD1_CTRL_SLAVE_FORCE_0);
	__raw_writel(CMD_DELAY, AM33XX_CMD1_CTRL_SLAVE_DELAY_0);
	__raw_writel(DDR2_DLL_LOCK_DIFF, AM33XX_CMD1_DLL_LOCK_DIFF_0);
	__raw_writel(DDR2_INVERT_CLKOUT, AM33XX_CMD1_INVERT_CLKOUT_0);

	__raw_writel(DDR2_RATIO, AM33XX_CMD2_CTRL_SLAVE_RATIO_0);
	__raw_writel(CMD_FORCE, AM33XX_CMD2_CTRL_SLAVE_FORCE_0);
	__raw_writel(CMD_DELAY, AM33XX_CMD2_CTRL_SLAVE_DELAY_0);
	__raw_writel(DDR2_DLL_LOCK_DIFF, AM33XX_CMD2_DLL_LOCK_DIFF_0);
	__raw_writel(DDR2_INVERT_CLKOUT, AM33XX_CMD2_INVERT_CLKOUT_0);
}

static void beaglebone_config_vtp(void)
{
	__raw_writel(__raw_readl(AM33XX_VTP0_CTRL_REG) | VTP_CTRL_ENABLE,
		AM33XX_VTP0_CTRL_REG);
	__raw_writel(__raw_readl(AM33XX_VTP0_CTRL_REG) & (~VTP_CTRL_START_EN),
		AM33XX_VTP0_CTRL_REG);
	__raw_writel(__raw_readl(AM33XX_VTP0_CTRL_REG) | VTP_CTRL_START_EN,
		AM33XX_VTP0_CTRL_REG);

	/* Poll for READY */
	while ((__raw_readl(AM33XX_VTP0_CTRL_REG) & VTP_CTRL_READY) != VTP_CTRL_READY);
}

static void beaglebone_config_emif_ddr2(void)
{
	u32 i;

	/*Program EMIF0 CFG Registers*/
	__raw_writel(EMIF_READ_LATENCY, AM33XX_EMIF4_0_REG(DDR_PHY_CTRL_1));
	__raw_writel(EMIF_READ_LATENCY, AM33XX_EMIF4_0_REG(DDR_PHY_CTRL_1_SHADOW));
	__raw_writel(EMIF_READ_LATENCY, AM33XX_EMIF4_0_REG(DDR_PHY_CTRL_2));
	__raw_writel(EMIF_TIM1, AM33XX_EMIF4_0_REG(SDRAM_TIM_1));
	__raw_writel(EMIF_TIM1, AM33XX_EMIF4_0_REG(SDRAM_TIM_1_SHADOW));
	__raw_writel(EMIF_TIM2, AM33XX_EMIF4_0_REG(SDRAM_TIM_2));
	__raw_writel(EMIF_TIM2, AM33XX_EMIF4_0_REG(SDRAM_TIM_2_SHADOW));
	__raw_writel(EMIF_TIM3, AM33XX_EMIF4_0_REG(SDRAM_TIM_3));
	__raw_writel(EMIF_TIM3, AM33XX_EMIF4_0_REG(SDRAM_TIM_3_SHADOW));

	__raw_writel(EMIF_SDCFG, AM33XX_EMIF4_0_REG(SDRAM_CONFIG));
	__raw_writel(EMIF_SDCFG, AM33XX_EMIF4_0_REG(SDRAM_CONFIG2));

	/* __raw_writel(EMIF_SDMGT, EMIF0_0_SDRAM_MGMT_CTRL);
	__raw_writel(EMIF_SDMGT, EMIF0_0_SDRAM_MGMT_CTRL_SHD); */
	__raw_writel(0x00004650, AM33XX_EMIF4_0_REG(SDRAM_REF_CTRL));
	__raw_writel(0x00004650, AM33XX_EMIF4_0_REG(SDRAM_REF_CTRL_SHADOW));

	for (i = 0; i < 5000; i++) {

	}

	/* __raw_writel(EMIF_SDMGT, EMIF0_0_SDRAM_MGMT_CTRL);
	__raw_writel(EMIF_SDMGT, EMIF0_0_SDRAM_MGMT_CTRL_SHD); */
	__raw_writel(EMIF_SDREF, AM33XX_EMIF4_0_REG(SDRAM_REF_CTRL));
	__raw_writel(EMIF_SDREF, AM33XX_EMIF4_0_REG(SDRAM_REF_CTRL_SHADOW));

	__raw_writel(EMIF_SDCFG, AM33XX_EMIF4_0_REG(SDRAM_CONFIG));
	__raw_writel(EMIF_SDCFG, AM33XX_EMIF4_0_REG(SDRAM_CONFIG2));
}

static void beaglebone_config_ddr(void)
{
	enable_ddr_clocks();

	beaglebone_config_vtp();

	beaglebone_cmd_macro_config();
	beaglebone_data_macro_config(0);
	beaglebone_data_macro_config(1);

	__raw_writel(PHY_RANK0_DELAY, AM33XX_DATA0_RANK0_DELAYS_0);
	__raw_writel(PHY_RANK0_DELAY, AM33XX_DATA1_RANK0_DELAYS_0);

	__raw_writel(DDR_IOCTRL_VALUE, AM33XX_DDR_CMD0_IOCTRL);
	__raw_writel(DDR_IOCTRL_VALUE, AM33XX_DDR_CMD1_IOCTRL);
	__raw_writel(DDR_IOCTRL_VALUE, AM33XX_DDR_CMD2_IOCTRL);
	__raw_writel(DDR_IOCTRL_VALUE, AM33XX_DDR_DATA0_IOCTRL);
	__raw_writel(DDR_IOCTRL_VALUE, AM33XX_DDR_DATA1_IOCTRL);

	__raw_writel(__raw_readl(AM33XX_DDR_IO_CTRL) & 0xefffffff, AM33XX_DDR_IO_CTRL);
	__raw_writel(__raw_readl(AM33XX_DDR_CKE_CTRL) | 0x00000001, AM33XX_DDR_CKE_CTRL);

	beaglebone_config_emif_ddr2();
}

/*
 * early system init of muxing and clocks.
 */
void beaglebone_sram_init(void)
{
	u32 regVal, uart_base;

	/* Setup the PLLs and the clocks for the peripherals */
	pll_init();

	beaglebone_config_ddr();

	/* UART softreset */
	uart_base = AM33XX_UART0_BASE;

	regVal = __raw_readl(uart_base + UART_SYSCFG_OFFSET);
	regVal |= UART_RESET;
	__raw_writel(regVal, (uart_base + UART_SYSCFG_OFFSET) );
	while ((__raw_readl(uart_base + UART_SYSSTS_OFFSET) &
		UART_CLK_RUNNING_MASK) != UART_CLK_RUNNING_MASK);

	/* Disable smart idle */
	regVal = __raw_readl((uart_base + UART_SYSCFG_OFFSET));
	regVal |= UART_SMART_IDLE_EN;
	__raw_writel(regVal, (uart_base + UART_SYSCFG_OFFSET));
}


/**
 * @brief The basic entry point for board initialization.
 *
 * This is called as part of machine init (after arch init).
 * This is again called with stack in SRAM, so not too many
 * constructs possible here.
 *
 * @return void
 */
static int beaglebone_board_init(void)
{
	int in_sdram = running_in_sdram();

	/* WDT1 is already running when the bootloader gets control
	 * Disable it to avoid "random" resets
	 */
	__raw_writel(WDT_DISABLE_CODE1, AM33XX_WDT_REG(WSPR));
	while(__raw_readl(AM33XX_WDT_REG(WWPS)) != 0x0);
	__raw_writel(WDT_DISABLE_CODE2, AM33XX_WDT_REG(WSPR));
	while(__raw_readl(AM33XX_WDT_REG(WWPS)) != 0x0);

	/* Dont reconfigure SDRAM while running in SDRAM! */
	if (!in_sdram)
		beaglebone_sram_init();

	/* Enable pin mux */
	enable_uart0_pin_mux();

	return 0;
}
pure_initcall(beaglebone_board_init);

/******************** Board Run Time *******************/

#ifdef CONFIG_DRIVER_SERIAL_NS16550

/**
 * @brief UART serial port initialization - remember to enable COM clocks in
 * arch
 *
 * @return result of device registration
 */
static int beaglebone_console_init(void)
{
	am33xx_add_uart0();

	return 0;
}
console_initcall(beaglebone_console_init);
#endif /* CONFIG_DRIVER_SERIAL_NS16550 */

static int beaglebone_mem_init(void)
{
	omap_add_ram0(256 * 1024 * 1024);

	return 0;
}
mem_initcall(beaglebone_mem_init);

static struct cpsw_slave_data cpsw_slaves[] = {
	{
		.phy_id		= 0,
		.phy_if		= PHY_INTERFACE_MODE_MII,
	},
};

static struct cpsw_platform_data cpsw_data = {
	.slave_data		= cpsw_slaves,
	.num_slaves		= ARRAY_SIZE(cpsw_slaves),
};

static void beaglebone_eth_init(void)
{
	am33xx_register_ethaddr(0, 0);

	writel(0, AM33XX_MAC_MII_SEL);

	enable_mii1_pin_mux();

	am33xx_add_cpsw(&cpsw_data);
}

static int beaglebone_devices_init(void)
{
	am33xx_add_mmc0(NULL);

	enable_i2c0_pin_mux();
	beaglebone_eth_init();

	armlinux_set_bootparams((void *)0x80000100);
	armlinux_set_architecture(MACH_TYPE_BEAGLEBONE);

	return 0;
}
device_initcall(beaglebone_devices_init);

#ifdef CONFIG_DEFAULT_ENVIRONMENT
static int beaglebone_env_init(void)
{
	struct stat s;
	char *diskdev = "/dev/disk0.0";
	int ret;

	ret = stat(diskdev, &s);
	if (ret) {
		printf("device %s not found. Using default environment\n", diskdev);
		return 0;
	}

	mkdir ("/boot", 0666);
	ret = mount(diskdev, "fat", "/boot");
	if (ret) {
		printf("failed to mount %s\n", diskdev);
		return 0;
	}

	if (IS_ENABLED(CONFIG_OMAP_BUILD_IFT))
		default_environment_path = "/dev/defaultenv";
	else
		default_environment_path = "/boot/barebox.env";

	return 0;
}
late_initcall(beaglebone_env_init);
#endif
