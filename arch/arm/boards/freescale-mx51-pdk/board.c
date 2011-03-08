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
	.size     = 512 * 1024 * 1024,
	.platform_data = &ram_pdata,
};

static struct fec_platform_data fec_info = {
	.xcv_type = MII100,
};

static struct pad_desc f3s_pads[] = {
	MX51_PAD_EIM_EB2__FEC_MDIO,
	MX51_PAD_EIM_EB3__FEC_RDATA1,
	MX51_PAD_EIM_CS2__FEC_RDATA2,
	MX51_PAD_EIM_CS3__FEC_RDATA3,
	MX51_PAD_EIM_CS4__FEC_RX_ER,
	MX51_PAD_EIM_CS5__FEC_CRS,
	MX51_PAD_NANDF_RB2__FEC_COL,
	MX51_PAD_NANDF_RB3__FEC_RX_CLK,
	MX51_PAD_NANDF_RB7__FEC_TX_ER,
	MX51_PAD_NANDF_CS3__FEC_MDC,
	MX51_PAD_NANDF_CS4__FEC_TDATA1,
	MX51_PAD_NANDF_CS5__FEC_TDATA2,
	MX51_PAD_NANDF_CS6__FEC_TDATA3,
	MX51_PAD_NANDF_CS7__FEC_TX_EN,
	MX51_PAD_NANDF_RDY_INT__FEC_TX_CLK,
	MX51_PAD_NANDF_D11__FEC_RX_DV,
	MX51_PAD_NANDF_RB6__FEC_RDATA0,
	MX51_PAD_NANDF_D8__FEC_TDATA0,
	MX51_PAD_CSPI1_SS0__CSPI1_SS0,
	MX51_PAD_CSPI1_MOSI__CSPI1_MOSI,
	MX51_PAD_CSPI1_MISO__CSPI1_MISO,
	MX51_PAD_CSPI1_RDY__CSPI1_RDY,
	MX51_PAD_CSPI1_SCLK__CSPI1_SCLK,
	MX51_PAD_EIM_A20__GPIO2_14, /* LAN8700 reset pin */
	IOMUX_PAD(0x60C, 0x21C, 3, 0x0, 0, 0x85), /* FIXME: needed? */
};

#ifdef CONFIG_MMU
static void babbage_mmu_init(void)
{
	mmu_init();

	arm_create_section(0x90000000, 0x90000000, 512, PMD_SECT_DEF_CACHED);
	arm_create_section(0xb0000000, 0x90000000, 512, PMD_SECT_DEF_UNCACHED);

	setup_dma_coherent(0x20000000);

	mmu_enable();
}
#else
static void babbage_mmu_init(void)
{
}
#endif

//extern int babbage_power_init(void);

#define BABBAGE_ECSPI1_CS0	(3 * 32 + 24)
static int spi_0_cs[] = {BABBAGE_ECSPI1_CS0};

static struct spi_imx_master spi_0_data = {
	.chipselect = spi_0_cs,
	.num_chipselect = ARRAY_SIZE(spi_0_cs),
};

static const struct spi_board_info mx51_babbage_spi_board_info[] = {
	{
		.name = "mc13892-spi",
		.max_speed_hz = 300000,
		.bus_num = 0,
		.chip_select = 0,
	},
};

#define MX51_CCM_CACRR 0x10

static void babbage_power_init(void)
{
	struct mc13892 *mc13892;
	u32 val;

	mc13892 = mc13892_get();
	if (!mc13892) {
		printf("could not get mc13892\n");
		return;
	}

	/* Write needed to Power Gate 2 register */
	mc13892_reg_read(mc13892, 34, &val);
	val &= ~0x10000;
	mc13892_reg_write(mc13892, 34, val);

	/* Write needed to update Charger 0 */
	mc13892_reg_write(mc13892, 48, 0x0023807F);

	/* power up the system first */
	mc13892_reg_write(mc13892, 34, 0x00200000);

	if (imx_silicon_revision() < MX51_CHIP_REV_3_0) {
		/* Set core voltage to 1.1V */
		mc13892_reg_read(mc13892, 24, &val);
		val &= ~0x1f;
		val |= 0x14;
		mc13892_reg_write(mc13892, 24, val);

		/* Setup VCC (SW2) to 1.25 */
		mc13892_reg_read(mc13892, 25, &val);
		val &= ~0x1f;
		val |= 0x1a;
		mc13892_reg_write(mc13892, 25, val);

		/* Setup 1V2_DIG1 (SW3) to 1.25 */
		mc13892_reg_read(mc13892, 26, &val);
		val &= ~0x1f;
		val |= 0x1a;
		mc13892_reg_write(mc13892, 26, val);
		udelay(50);
		/* Raise the core frequency to 800MHz */
		writel(0x0, MX51_CCM_BASE_ADDR + MX51_CCM_CACRR);
	} else {
		/* Setup VCC (SW2) to 1.225 */
		mc13892_reg_read(mc13892, 25, &val);
		val &= ~0x1f;
		val |= 0x19;
		mc13892_reg_write(mc13892, 25, val);

		/* Setup 1V2_DIG1 (SW3) to 1.2 */
		mc13892_reg_read(mc13892, 26, &val);
		val &= ~0x1f;
		val |= 0x18;
		mc13892_reg_write(mc13892, 26, val);
	}

	if (mc13892_get_revision(mc13892) < MC13892_REVISION_2_0) {
		/* Set switchers in PWM mode for Atlas 2.0 and lower */
		/* Setup the switcher mode for SW1 & SW2*/
		mc13892_reg_read(mc13892, 28, &val);
		val &= ~0x3c0f;
		val |= 0x1405;
		mc13892_reg_write(mc13892, 28, val);

		/* Setup the switcher mode for SW3 & SW4 */
		mc13892_reg_read(mc13892, 29, &val);
		val &= ~0xf0f;
		val |= 0x505;
		mc13892_reg_write(mc13892, 29, val);
	} else {
		/* Set switchers in Auto in NORMAL mode & STANDBY mode for Atlas 2.0a */
		/* Setup the switcher mode for SW1 & SW2*/
		mc13892_reg_read(mc13892, 28, &val);
		val &= ~0x3c0f;
		val |= 0x2008;
		mc13892_reg_write(mc13892, 28, val);

		/* Setup the switcher mode for SW3 & SW4 */
		mc13892_reg_read(mc13892, 29, &val);
		val &= ~0xf0f;
		val |= 0x808;
		mc13892_reg_write(mc13892, 29, val);
	}

	/* Set VDIG to 1.65V, VGEN3 to 1.8V, VCAM to 2.5V */
	mc13892_reg_read(mc13892, 30, &val);
	val &= ~0x34030;
	val |= 0x10020;
	mc13892_reg_write(mc13892, 30, val);

	/* Set VVIDEO to 2.775V, VAUDIO to 3V, VSD to 3.15V */
	mc13892_reg_read(mc13892, 31, &val);
	val &= ~0x1FC;
	val |= 0x1F4;
	mc13892_reg_write(mc13892, 31, val);

	/* Configure VGEN3 and VCAM regulators to use external PNP */
	val = 0x208;
	mc13892_reg_write(mc13892, 33, val);
	udelay(200);
#define GPIO_LAN8700_RESET	(1 * 32 + 14)

	/* Reset the ethernet controller over GPIO */
	gpio_direction_output(GPIO_LAN8700_RESET, 0);

	/* Enable VGEN3, VCAM, VAUDIO, VVIDEO, VSD regulators */
	val = 0x49249;
	mc13892_reg_write(mc13892, 33, val);

	udelay(500);

	gpio_set_value(GPIO_LAN8700_RESET, 1);
}

static int f3s_devices_init(void)
{
	babbage_mmu_init();

	register_device(&sdram_dev);
	imx51_add_fec(&fec_info);
	imx51_add_mmc0(NULL);

	spi_register_board_info(mx51_babbage_spi_board_info,
			ARRAY_SIZE(mx51_babbage_spi_board_info));
	imx51_add_spi0(&spi_0_data);

	babbage_power_init();

	armlinux_add_dram(&sdram_dev);
	armlinux_set_bootparams((void *)0x90000100);
	armlinux_set_architecture(MACH_TYPE_MX51_BABBAGE);

	return 0;
}

device_initcall(f3s_devices_init);

static int f3s_part_init(void)
{
	devfs_add_partition("disk0", 0x00000, 0x40000, PARTITION_FIXED, "self0");
	devfs_add_partition("disk0", 0x40000, 0x20000, PARTITION_FIXED, "env0");

	return 0;
}
late_initcall(f3s_part_init);

static int f3s_console_init(void)
{
	mxc_iomux_v3_setup_multiple_pads(f3s_pads, ARRAY_SIZE(f3s_pads));

	writel(0, 0x73fa8228);
	writel(0, 0x73fa822c);
	writel(0, 0x73fa8230);
	writel(0, 0x73fa8234);

	imx51_add_uart0();
	return 0;
}

console_initcall(f3s_console_init);

