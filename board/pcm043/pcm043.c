/*
 * (C) 2007 Pengutronix, Sascha Hauer <s.hauer@pengutronix.de>
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 *
 * Board support for Phytec's, i.MX35 based CPU card, called: PCM043
 */

#include <common.h>
#include <init.h>
#include <driver.h>
#include <environment.h>
#include <asm/arch/imx-regs.h>
#include <asm/armlinux.h>
#include <asm/arch/gpio.h>
#include <asm/io.h>
#include <partition.h>
#include <asm/mach-types.h>
#include <asm/arch/imx-nand.h>
#include <fec.h>

#define CYG_MACRO_START
#define CYG_MACRO_END
#define ARM_MMU_FIRST_LEVEL_SECTION_ID 0x2
#define ARM_MMU_FIRST_LEVEL_DESCRIPTOR_ADDRESS(ttb_base, table_index) \
        (unsigned long *)((unsigned long)(ttb_base) + ((table_index) << 2))

/*
 * Up to 32MiB NOR type flash, connected to
 * CS line 0, data width is 16 bit
 */
static struct device_d cfi_dev = {
	.name     = "cfi_flash",
	.id       = "nor0",
	.map_base = IMX_CS0_BASE,
	.size     = 32 * 1024 * 1024,	/* area size */
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

static struct device_d sdram0_dev = {
	.name     = "ram",
	.id       = "ram0",

	.map_base = IMX_SDRAM_CS0,
	.size     = 128 * 1024 * 1024,

	.type     = DEVICE_TYPE_DRAM,
};

struct imx_nand_platform_data nand_info = {
	.width	= 1,
	.hw_ecc	= 1,
};

static struct device_d nand_dev = {
	.name     = "imx_nand",
	.map_base = IMX_NAND_BASE,
	.platform_data	= &nand_info,
};

static int imx35_devices_init(void)
{
	/* CS0: Nor Flash */
	writel(0x0000cf03, CSCR_U(0));
	writel(0x10000d03, CSCR_L(0));
	writel(0x00720900, CSCR_A(0));

	/* setup pins for I2C1 (for EEPROM, RTC) */
	imx_gpio_mode(MUX_I2C1_CLK_I2C1_SLC);
	imx_gpio_mode(MUX_I2C1_DAT_I2C1_SDA);

	register_device(&cfi_dev);

	/*
	 * Create partitions that should be
	 * not touched by any regular user
	 */
#ifdef CONFIG_PARTITION
	dev_add_partition(&cfi_dev, 0x00000, 0x40000, PARTITION_FIXED, "self");	/* ourself */
	dev_add_partition(&cfi_dev, 0x40000, 0x20000, PARTITION_FIXED, "env");	/* environment */
#endif
	dev_protect(&cfi_dev, 0x20000, 0, 1);

	register_device(&nand_dev);
	
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

	register_device(&fec_dev);

	register_device(&sdram0_dev);

	armlinux_set_bootparams((void *)0x80000100);
	armlinux_set_architecture(MACH_TYPE_PCM043);

	return 0;
}

device_initcall(imx35_devices_init);

static struct device_d imx35_serial_device = {
	.name     = "imx_serial",
	.id       = "cs0",
	.map_base = IMX_UART1_BASE,
	.size     = 16 * 1024,
	.type     = DEVICE_TYPE_CONSOLE,
};

static int imx35_console_init(void)
{
	/* init gpios for serial port */
	imx_gpio_mode(MUX_RXD1_UART1_RXD_MUX);
	imx_gpio_mode(MUX_TXD1_UART1_TXD_MUX);
	imx_gpio_mode(MUX_RTS1_UART1_RTS_B);
	imx_gpio_mode(MUX_RTS1_UART1_CTS_B);

	register_device(&imx35_serial_device);
	return 0;
}

console_initcall(imx35_console_init);

static int pcm043_core_setup(void)
{
	u32 tmp;

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

	writel(0x0000DCF6, CSCR_U(0)); /* CS0: NOR Flash */
	writel(0x444A4541, CSCR_L(0));
	writel(0x44443302, CSCR_A(0));

	/*
	 * M3IF Control Register (M3IFCTL)
	 * MRRP[0] = L2CC0 not on priority list (0 << 0)	= 0x00000000
	 * MRRP[1] = MAX1 not on priority list (0 << 0)		= 0x00000000
	 * MRRP[2] = L2CC1 not on priority list (0 << 0)	= 0x00000000
	 * MRRP[3] = USB  not on priority list (0 << 0)		= 0x00000000
	 * MRRP[4] = SDMA not on priority list (0 << 0)		= 0x00000000
	 * MRRP[5] = GPU not on priority list (0 << 0)		= 0x00000000
	 * MRRP[6] = IPU1 on priority list (1 << 6)		= 0x00000040
	 * MRRP[7] = IPU2 not on priority list (0 << 0)		= 0x00000000
	 *                                                       ------------
	 *                                                        0x00000040
	 */
	writel(0x40, IMX_M3IF_BASE);

	return 0;
}

core_initcall(pcm043_core_setup);

