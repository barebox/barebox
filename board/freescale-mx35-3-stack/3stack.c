/*
 * Copyright (C) 2007 Sascha Hauer, Pengutronix 
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
#include <cfi_flash.h>
#include <init.h>
#include <environment.h>
#include <asm/arch/imx-regs.h>
#include <fec.h>
#include <asm/arch/gpio.h>
#include <asm/armlinux.h>
#include <asm/mach-types.h>
#include <asm/arch/pmic.h>
#include <partition.h>
#include <fs.h>
#include <fcntl.h>
#include <nand.h>
#include <spi/spi.h>
#include <asm/io.h>
#include <asm/arch/imx-nand.h>

static struct device_d cfi_dev = {
	.name     = "cfi_flash",
	.id       = "nor0",

	.map_base = 0xa0000000,
	.size     = 64 * 1024 * 1024,
};

static struct device_d sdram_dev = {
	.name     = "ram",
	.id       = "ram0",

	.map_base = 0x80000000,
	.size     = 128 * 1024 * 1024,

	.type     = DEVICE_TYPE_DRAM,
};

static struct fec_platform_data fec_info = {
	.xcv_type = MII100,
};

static struct device_d fec_dev = {
	.name     = "fec_imx",
	.id       = "eth0",
	.map_base = 0x50038000,
	.platform_data	= &fec_info,
	.type     = DEVICE_TYPE_ETHER,
};

/*
 * SMSC 9217 network controller
 */
static struct device_d smc911x_dev = {
	.name     = "smc911x",
	.id       = "eth0",
	.map_base = IMX_CS5_BASE,
	.size     = IMX_CS5_RANGE,	/* area size */
	.type     = DEVICE_TYPE_ETHER,
};

static int f3s_devices_init(void)
{
	imx_gpio_mode(MUX_FEC_TX_CLK_FEC_TX_CLK);
	imx_gpio_mode(MUX_FEC_RX_CLK_FEC_RX_CLK);
	imx_gpio_mode(MUX_FEC_RX_DV_FEC_RX_DV);
	imx_gpio_mode(MUX_FEC_COL_FEC_COL);
	imx_gpio_mode(MUX_FEC_TX_EN_FEC_TX_EN);
	imx_gpio_mode(MUX_FEC_MDC_FEC_MDC);
	imx_gpio_mode(MUX_FEC_MDIO_FEC_MDIO);
	imx_gpio_mode(MUX_FEC_TX_ERR_FEC_TX_ERR);
	imx_gpio_mode(MUX_FEC_RX_ERR_FEC_RX_ERR);
	imx_gpio_mode(MUX_FEC_CRS_FEC_CRS);
	imx_gpio_mode(MUX_FEC_RDATA0_FEC_RDATA0);
	imx_gpio_mode(MUX_FEC_TDATA0_FEC_TDATA0);
	imx_gpio_mode(MUX_FEC_RDATA1_FEC_RDATA1);
	imx_gpio_mode(MUX_FEC_TDATA1_FEC_TDATA1);
	imx_gpio_mode(MUX_FEC_RDATA2_FEC_RDATA2);
	imx_gpio_mode(MUX_FEC_TDATA2_FEC_TDATA2);
	imx_gpio_mode(MUX_FEC_RDATA3_FEC_RDATA3);
	imx_gpio_mode(MUX_FEC_TDATA3_FEC_TDATA3);

	register_device(&cfi_dev);
	register_device(&sdram_dev);
	register_device(&smc911x_dev);
	/* FEC is currently broken. It seems to work
	 * shortly but after a few moments the board
	 * goes to nirvana
	 */
//	register_device(&fec_dev);

	/*
	 * Create partitions that should be
	 * not touched by any regular user
	 */
#ifdef CONFIG_PARTITION
	dev_add_partition(&cfi_dev, 0x00000, 0x40000, PARTITION_FIXED, "self");	/* ourself */
	dev_add_partition(&cfi_dev, 0x40000, 0x20000, PARTITION_FIXED, "env");	/* environment */
#endif

	armlinux_set_bootparams((void *)0x80000100);
	armlinux_set_architecture(MACH_TYPE_PCM037); /* FIXME */

	return 0;
}

device_initcall(f3s_devices_init);

static struct device_d f3s_serial_device = {
	.name     = "imx_serial",
	.id       = "cs0",
	.map_base = IMX_UART1_BASE,
	.size     = 4096,
	.type     = DEVICE_TYPE_CONSOLE,
};

static int f3s_console_init(void)
{
	register_device(&f3s_serial_device);
	return 0;
}

console_initcall(f3s_console_init);

static int f3s_core_setup(void)
{
	u32 tmp;

	writel(0x0000D843, CSCR_U(5)); /* CS5: smc9117 */
	writel(0x22252521, CSCR_L(5));
	writel(0x22220A00, CSCR_A(5));

	/* FIXME: The rest is currently done in Assembler. Remove assembler
	 *        config once the board is running stable
	 */
	return 0;

	/* AIPS setup - Only setup MPROTx registers. The PACR default values are good.*/
	/*
	 * Set all MPROTx to be non-bufferable, trusted for R/W,
	 * not forced to user-mode.
	 */
	writel(0x77777777, IMX_AIPS1_BASE);
	writel(0x77777777, IMX_AIPS1_BASE + 0x4);
	writel(0x77777777, IMX_AIPS2_BASE);
	writel(0x77777777, IMX_AIPS2_BASE + 0x4);

	/*
	 * Clear the on and off peripheral modules Supervisor Protect bit
	 * for SDMA to access them. Did not change the AIPS control registers
	 * (offset 0x20) access type
	 */
	writel(0x0, IMX_AIPS1_BASE + 0x40);
	writel(0x0, IMX_AIPS1_BASE + 0x44);
	writel(0x0, IMX_AIPS1_BASE + 0x48);
	writel(0x0, IMX_AIPS1_BASE + 0x4C);
	tmp = readl(IMX_AIPS1_BASE + 0x50);
	tmp &= 0x00FFFFFF;
	writel(tmp, IMX_AIPS1_BASE + 0x50);

	writel(0x0, IMX_AIPS2_BASE + 0x40);
	writel(0x0, IMX_AIPS2_BASE + 0x44);
	writel(0x0, IMX_AIPS2_BASE + 0x48);
	writel(0x0, IMX_AIPS2_BASE + 0x4C);
	tmp = readl(IMX_AIPS2_BASE + 0x50);
	tmp &= 0x00FFFFFF;
	writel(tmp, IMX_AIPS2_BASE + 0x50);

	/* MAX (Multi-Layer AHB Crossbar Switch) setup */

	/* MPR - priority is M4 > M2 > M3 > M5 > M0 > M1 */
#define MAX_PARAM1 0x00302154
	writel(MAX_PARAM1, IMX_MAX_BASE + 0x0);   /* for S0 */
	writel(MAX_PARAM1, IMX_MAX_BASE + 0x100); /* for S1 */
	writel(MAX_PARAM1, IMX_MAX_BASE + 0x200); /* for S2 */
	writel(MAX_PARAM1, IMX_MAX_BASE + 0x300); /* for S3 */
	writel(MAX_PARAM1, IMX_MAX_BASE + 0x400); /* for S4 */

	/* SGPCR - always park on last master */
	writel(0x10, IMX_MAX_BASE + 0x10);	/* for S0 */
	writel(0x10, IMX_MAX_BASE + 0x110);	/* for S1 */
	writel(0x10, IMX_MAX_BASE + 0x210);	/* for S2 */
	writel(0x10, IMX_MAX_BASE + 0x310);	/* for S3 */
	writel(0x10, IMX_MAX_BASE + 0x410);	/* for S4 */

	/* MGPCR - restore default values */
	writel(0x0, IMX_MAX_BASE + 0x800);	/* for M0 */
	writel(0x0, IMX_MAX_BASE + 0x900);	/* for M1 */
	writel(0x0, IMX_MAX_BASE + 0xa00);	/* for M2 */
	writel(0x0, IMX_MAX_BASE + 0xb00);	/* for M3 */
	writel(0x0, IMX_MAX_BASE + 0xc00);	/* for M4 */
	writel(0x0, IMX_MAX_BASE + 0xd00);	/* for M5 */

	return 0;
}

core_initcall(f3s_core_setup);

