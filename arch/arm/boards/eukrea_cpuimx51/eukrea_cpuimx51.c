/*
 * Copyright (C) 2007 Sascha Hauer, Pengutronix
 * (c) 2011 Eukrea Electromatique, Eric Bénard <eric@eukrea.com>
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 */

#include <common.h>
#include <net.h>
#include <init.h>
#include <environment.h>
#include <mach/imx-regs.h>
#include <fec.h>
#include <mach/gpio.h>
#include <asm/armlinux.h>
#include <generated/mach-types.h>
#include <partition.h>
#include <fs.h>
#include <fcntl.h>
#include <nand.h>
#include <spi/spi.h>
#include <mfd/mc13892.h>
#include <asm/io.h>
#include <asm/mmu.h>
#include <mach/imx-nand.h>
#include <mach/spi.h>
#include <mach/generic.h>
#include <mach/iomux-mx51.h>
#include <mach/devices-imx51.h>

static struct memory_platform_data ram_pdata = {
	.name = "ram0",
	.flags = DEVFS_RDWR,
};

static struct device_d sdram_dev = {
	.id       = -1,
	.name     = "mem",
	.map_base = 0x90000000,
	.size     = 256 * 1024 * 1024,
	.platform_data = &ram_pdata,
};

static struct fec_platform_data fec_info = {
	.xcv_type = MII100,
};

struct imx_nand_platform_data nand_info = {
	.width		= 1,
	.hw_ecc		= 1,
	.flash_bbt	= 1,
};

static struct pad_desc eukrea_cpuimx51_pads[] = {
	/* FEC */
	MX51_PAD_DISP2_DAT1__FEC_RX_ER,
	MX51_PAD_DISP2_DAT15__FEC_TDATA0,
	MX51_PAD_DISP2_DAT6__FEC_TDATA1,
	MX51_PAD_DISP2_DAT7__FEC_TDATA2,
	MX51_PAD_DISP2_DAT8__FEC_TDATA3,
	MX51_PAD_DISP2_DAT9__FEC_TX_EN,
	MX51_PAD_DISP2_DAT10__FEC_COL,
	MX51_PAD_DISP2_DAT11__FEC_RX_CLK,
	MX51_PAD_DISP2_DAT12__FEC_RX_DV,
	MX51_PAD_DISP2_DAT13__FEC_TX_CLK,
	MX51_PAD_DI2_PIN4__FEC_CRS,
	MX51_PAD_DI2_PIN2__FEC_MDC,
	MX51_PAD_DI2_PIN3__FEC_MDIO,
	MX51_PAD_DISP2_DAT14__FEC_RDATA0,
	MX51_PAD_DI2_DISP_CLK__FEC_RDATA1,
	MX51_PAD_DI_GP4__FEC_RDATA2,
	MX51_PAD_DISP2_DAT0__FEC_RDATA3,
	MX51_PAD_DI_GP3__FEC_TX_ER,
	MX51_PAD_EIM_DTACK__GPIO2_31, /* LAN8700 reset pin */
	/* NAND */
	MX51_PAD_NANDF_D7__NANDF_D7,
	MX51_PAD_NANDF_D6__NANDF_D6,
	MX51_PAD_NANDF_D5__NANDF_D5,
	MX51_PAD_NANDF_D4__NANDF_D4,
	MX51_PAD_NANDF_D3__NANDF_D3,
	MX51_PAD_NANDF_D2__NANDF_D2,
	MX51_PAD_NANDF_D1__NANDF_D1,
	MX51_PAD_NANDF_D0__NANDF_D0,
	MX51_PAD_NANDF_RB0__NANDF_RB0,
	MX51_PAD_NANDF_RB1__NANDF_RB1,
	MX51_PAD_NANDF_CS0__NANDF_CS0,
	MX51_PAD_NANDF_CS1__NANDF_CS1,
	/* LCD BL */
	MX51_PAD_DI1_D1_CS__GPIO3_4,
};

#define GPIO_LAN8700_RESET	(1 * 32 + 31)
#define GPIO_LCD_BL		(2 * 32 + 4)

#ifdef CONFIG_MMU
static void eukrea_cpuimx51_mmu_init(void)
{
	mmu_init();

	arm_create_section(0x90000000, 0x90000000, 256, PMD_SECT_DEF_CACHED);
	arm_create_section(0xa0000000, 0x90000000, 256, PMD_SECT_DEF_UNCACHED);

	setup_dma_coherent(0x10000000);

#if TEXT_BASE & (0x100000 - 1)
#warning cannot create vector section. Adjust TEXT_BASE to a 1M boundary
#else
	arm_create_section(0x0,        TEXT_BASE,   1, PMD_SECT_DEF_UNCACHED);
#endif

	mmu_enable();
}
#else
static void eukrea_cpuimx51_mmu_init(void)
{
}
#endif

static int eukrea_cpuimx51_devices_init(void)
{
	eukrea_cpuimx51_mmu_init();

	register_device(&sdram_dev);
	imx51_add_fec(&fec_info);
#ifdef CONFIG_MCI_IMX_ESDHC
	imx51_add_mmc0(NULL);
#endif
	imx51_add_nand(&nand_info);

	devfs_add_partition("nand0", 0x00000, 0x40000, PARTITION_FIXED, "self_raw");
	dev_add_bb_dev("self_raw", "self0");
	devfs_add_partition("nand0", 0x40000, 0x20000, PARTITION_FIXED, "env_raw");
	dev_add_bb_dev("env_raw", "env0");

	gpio_direction_output(GPIO_LAN8700_RESET, 0);
	gpio_set_value(GPIO_LAN8700_RESET, 1);
	gpio_direction_output(GPIO_LCD_BL, 0);

	armlinux_add_dram(&sdram_dev);
	armlinux_set_bootparams((void *)0x90000100);
	armlinux_set_architecture(MACH_TYPE_EUKREA_CPUIMX51SD);

	return 0;
}

device_initcall(eukrea_cpuimx51_devices_init);

static int eukrea_cpuimx51_console_init(void)
{
	mxc_iomux_v3_setup_multiple_pads(eukrea_cpuimx51_pads, ARRAY_SIZE(eukrea_cpuimx51_pads));

	writel(0, 0x73fa8228);
	writel(0, 0x73fa822c);
	writel(0, 0x73fa8230);
	writel(0, 0x73fa8234);

	imx51_add_uart0();
	return 0;
}

console_initcall(eukrea_cpuimx51_console_init);
