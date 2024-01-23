// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2014 Robert Jarzmik <robert.jarzmik@free.fr>

#include <common.h>

#include <driver.h>
#include <environment.h>
#include <fs.h>
#include <gpio.h>
#include <init.h>
#include <led.h>
#include <platform_data/eth-smc91111.h>
#include <platform_data/mtd-nand-mrvl.h>
#include <pwm.h>
#include <linux/clk.h>
#include <linux/clkdev.h>
#include <linux/sizes.h>

#include <mach/pxa/devices.h>
#include <mach/pxa/mfp-pxa3xx.h>
#include <mach/pxa/pxa-regs.h>

#include <asm/armlinux.h>
#include <asm/io.h>
#include <asm/mmu.h>
#include <asm/mach-types.h>

static struct smc91c111_pdata smsc91x_pdata;
static struct mrvl_nand_platform_data nand_pdata = {
	.keep_config = 0,
	.flash_bbt = 1,
};

static mfp_cfg_t pxa310_mfp_cfg[] = {
	/* FFUART */
	MFP_CFG_LPM(GPIO99,  AF1, FLOAT),	/* GPIO99_UART1_RXD */
	MFP_CFG_LPM(GPIO100, AF1, FLOAT),	/* GPIO100_UART1_RXD */
	MFP_CFG_LPM(GPIO101, AF1, FLOAT),	/* GPIO101_UART1_CTS */
	MFP_CFG_LPM(GPIO106, AF1, FLOAT),	/* GPIO106_UART1_CTS */

	/* Ethernet */
	MFP_CFG(GPIO2,  AF1),			/* GPIO2_nCS3 */
};

static int zylonite_devices_init(void)
{
	struct clk *clk;

	armlinux_set_architecture(MACH_TYPE_ZYLONITE);
	pxa_add_uart((void *)0x40100000, 0);
	add_generic_device("smc91c111", DEVICE_ID_DYNAMIC, NULL,
			   0x14000300, 0x100000, IORESOURCE_MEM,
			   &smsc91x_pdata);
	clk = clk_get_sys("nand", NULL);
	if (!IS_ERR(clk))
		clkdev_add_physbase(clk, 0x43100000, NULL);
	add_generic_device("mrvl_nand", DEVICE_ID_DYNAMIC, NULL,
			   0x43100000, 0x1000, IORESOURCE_MEM, &nand_pdata);
	devfs_add_partition("nand0", SZ_1M, SZ_256K, DEVFS_PARTITION_FIXED,
			    "env0");
	return 0;
}
device_initcall(zylonite_devices_init);

static int zylonite_coredevice_init(void)
{
	barebox_set_model("Zylonite");
	barebox_set_hostname("zylonite");

	mfp_init();
	if (cpu_is_pxa310())
		pxa3xx_mfp_config(pxa310_mfp_cfg, ARRAY_SIZE(pxa310_mfp_cfg));
	CKENA |= CKEN_NAND | CKEN_SMC | CKEN_FFUART | CKEN_GPIO;
	/*
	 * Configure Ethernet controller :
	 * MCS1: setup VLIO on nCS3, with 15 DF_SCLK cycles (max) for hold,
	 *       setup and assertion times
	 * CSADRCFG3: DFI AA/D multiplexing VLIO, addr split at bit <16>, full
	 * latched mode, 7 DF_SCLK cycles (max) for nLUA and nLLA.
	 */
	MSC1 = 0x7ffc0000 | (MSC1 & 0x0000ffff);
	CSADRCFG3 = 0x003e080b;

	return 0;
}
coredevice_initcall(zylonite_coredevice_init);

static int zylonite_mem_init(void)
{
	arm_add_mem_device("ram0", 0x80000000, 64 * 1024 * 1024);
	return 0;
}
mem_initcall(zylonite_mem_init);
