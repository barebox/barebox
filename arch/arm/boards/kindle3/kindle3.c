// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2007 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
// SPDX-FileCopyrightText: 2016 Alexander Kurz <akurz@blala.de>

/* Board support for the Amazon Kindle 3rd generation */

#include <common.h>
#include <command.h>
#include <driver.h>
#include <init.h>
#include <bootsource.h>
#include <io.h>
#include <environment.h>
#include <generated/mach-types.h>
#include <asm/armlinux.h>
#include <asm/mmu.h>
#include <asm/setup.h>
#include <mach/imx35-regs.h>
#include <mach/imx-pll.h>
#include <mach/iomux-mx35.h>
#include <mach/devices-imx35.h>
#include <mach/generic.h>
#include <usb/fsl_usb2.h>
#include <mach/usb.h>
#include <mach/spi.h>
#include <spi/spi.h>
#include <magicvar.h>

/* 16 byte id for serial number */
#define ATAG_SERIAL16   0x5441000a
/* 16 byte id for a board revision */
#define ATAG_REVISION16 0x5441000b

struct char16_tag {
	char data[16];
};

static struct tag *setup_16char_tag(struct tag *params, uint32_t tag,
				    const char *value)
{
	struct char16_tag *target;
	target = ((void *) params) + sizeof(struct tag_header);
	params->hdr.tag = tag;
	params->hdr.size = tag_size(char16_tag);
	memcpy(target->data, value, sizeof target->data);
	return tag_next(params);
}

static const char *get_env_16char_tag(const char *tag)
{
	static const char *default16 = "0000000000000000";
	const char *value;
	value = getenv(tag);
	if (!value) {
		printf("env var %s not found, using default\n", tag);
		return default16;
	}
	if (strlen(value) != 16) {
		printf("env var %s: expecting 16 characters, using default\n",
			tag);
		return default16;
	}
	printf("%s: %s\n", tag, value);
	return value;
}

BAREBOX_MAGICVAR(global.board.serial16,
	"Pass the kindle Serial as vendor-specific ATAG to linux");
BAREBOX_MAGICVAR(global.board.revision16,
	"Pass the kindle BoardId as vendor-specific ATAG to linux");

/* The Kindle3 Kernel expects two custom ATAGs, ATAG_REVISION16 describing
 * the board and ATAG_SERIAL16 to identify the individual device.
 */
static struct tag *kindle3_append_atags(struct tag *params)
{
	params = setup_16char_tag(params, ATAG_SERIAL16,
				get_env_16char_tag("global.board.serial16"));
	params = setup_16char_tag(params, ATAG_REVISION16,
				get_env_16char_tag("global.board.revision16"));
	return params;
}

static struct fsl_usb2_platform_data kindle3_usb_info = {
	.operating_mode = FSL_USB2_DR_DEVICE,
	.phy_mode = FSL_USB2_PHY_UTMI,
};

/* SPI master devices. */
static int kindle3_spi0_internal_chipselect[] = {
	IMX_GPIO_NR(1, 18),
};

static struct spi_imx_master kindle3_spi0_info = {
	.chipselect	= kindle3_spi0_internal_chipselect,
	.num_chipselect	= ARRAY_SIZE(kindle3_spi0_internal_chipselect),
};

static const struct spi_board_info kindle3_spi_board_info[] = {
	{
		.name		= "mc13892",
		.bus_num	= 0,
		.chip_select	= 0,
		.mode		= SPI_CS_HIGH,
	},
};

static int kindle3_mmu_init(void)
{
	l2x0_init((void __iomem *)0x30000000, 0x00030024, 0x00000000);

	return 0;
}
postmmu_initcall(kindle3_mmu_init);

static int kindle3_devices_init(void)
{
	imx35_add_mmc0(NULL);

	if (IS_ENABLED(CONFIG_USB_GADGET)) {
		unsigned int tmp;
		/* Workaround ENGcm09152 */
		tmp = readl(MX35_USB_OTG_BASE_ADDR + 0x608);
		writel(tmp | (1 << 23), MX35_USB_OTG_BASE_ADDR + 0x608);
		add_generic_device("fsl-udc", DEVICE_ID_DYNAMIC, NULL,
				   MX35_USB_OTG_BASE_ADDR, 0x200,
				   IORESOURCE_MEM, &kindle3_usb_info);
	}

	/* The kindle3 related linux patch published by amazon bluntly
	 * renamed MACH_MX35_3DS to MACH_MX35_LUIGI
	 */
	armlinux_set_architecture(MACH_TYPE_MX35_3DS);

	/* Compatibility ATAGs for original kernel */
	armlinux_set_atag_appender(kindle3_append_atags);
	return 0;
}
device_initcall(kindle3_devices_init);

#define FIVEWAY_PAD_CTL (PAD_CTL_PUS_100K_UP | PAD_CTL_HYS | PAD_CTL_DVS)
static iomux_v3_cfg_t kindle3_pads[] = {
	/* UART1 */
	MX35_PAD_RXD1__UART1_RXD_MUX,
	MX35_PAD_TXD1__UART1_TXD_MUX,

	/* eMMC */
	MX35_PAD_SD1_CMD__ESDHC1_CMD,
	MX35_PAD_SD1_CLK__ESDHC1_CLK,
	MX35_PAD_SD1_DATA0__ESDHC1_DAT0,
	MX35_PAD_SD1_DATA1__ESDHC1_DAT1,
	MX35_PAD_SD1_DATA2__ESDHC1_DAT2,
	MX35_PAD_SD1_DATA3__ESDHC1_DAT3,

	/* USB */
	MX35_PAD_USBOTG_PWR__USB_TOP_USBOTG_PWR,
	MX35_PAD_USBOTG_OC__USB_TOP_USBOTG_OC,

	/* I2C 1+2 */
	MX35_PAD_I2C1_CLK__I2C1_SCL,
	MX35_PAD_I2C1_DAT__I2C1_SDA,
	MX35_PAD_I2C2_CLK__I2C2_SCL,
	MX35_PAD_I2C2_DAT__I2C2_SDA,

	/* SPI */
	MX35_PAD_CSPI1_SS0__GPIO1_18,
	MX35_PAD_CSPI1_SCLK__CSPI1_SCLK,
	MX35_PAD_CSPI1_MOSI__CSPI1_MOSI,
	MX35_PAD_CSPI1_MISO__CSPI1_MISO,
	MX35_PAD_CSPI1_SPI_RDY__CSPI1_RDY,

	/* fiveway device: up, down, left, right, select */
	IOMUX_PAD(0x718, 0x2b4, 5, 0x8b4, 1, FIVEWAY_PAD_CTL),
	IOMUX_PAD(0x71c, 0x2b8, 5, 0x8b8, 1, FIVEWAY_PAD_CTL),
	IOMUX_PAD(0x59c, 0x158, 5, 0x830, 0, FIVEWAY_PAD_CTL),
	IOMUX_PAD(0x724, 0x2c0, 5, 0x8c4, 1, FIVEWAY_PAD_CTL),
	IOMUX_PAD(0x728, 0x2c4, 5, 0x8c8, 1, FIVEWAY_PAD_CTL),

	/* Volume keys: up, down */
	MX35_PAD_SCKR__GPIO1_4,
	MX35_PAD_FSR__GPIO1_5,

};

static int kindle3_part_init(void)
{
	devfs_add_partition("disk0", SZ_1K, 2 * SZ_1K,
			    DEVFS_PARTITION_FIXED, "disk0.imx_header");
	devfs_add_partition("disk0", 4 * SZ_1K, (192 - 1) * SZ_1K,
			    DEVFS_PARTITION_FIXED, "disk0.self");
	devfs_add_partition("disk0", (192 + 3) * SZ_1K, SZ_64K,
			    DEVFS_PARTITION_FIXED, "env0");
	devfs_add_partition("disk0", (256 + 3) * SZ_1K, SZ_1K,
			    DEVFS_PARTITION_FIXED, "disk0.serial");
	devfs_add_partition("disk0", (256 + 4) * SZ_1K, 3407872,
			    DEVFS_PARTITION_FIXED, "disk0.kernel");
	devfs_add_partition("disk0", 3674112, SZ_256K,
			    DEVFS_PARTITION_FIXED, "disk0.waveform");
	return 0;
}

late_initcall(kindle3_part_init);

static int imx35_console_init(void)
{
	mxc_iomux_v3_setup_multiple_pads(kindle3_pads,
					 ARRAY_SIZE(kindle3_pads));

	barebox_set_model("Kindle3");
	barebox_set_hostname("kindle3");

	imx35_add_uart0();

	spi_register_board_info(kindle3_spi_board_info,
				ARRAY_SIZE(kindle3_spi_board_info));
	imx35_add_spi0(&kindle3_spi0_info);

	imx35_add_i2c0(NULL);
	imx35_add_i2c1(NULL);
	return 0;
}
console_initcall(imx35_console_init);

static int kindle3_core_setup(void)
{
	u32 tmp;

	/* AIPS setup - Only setup MPROTx registers.
	 * The PACR default values are good.
	 */
	/*
	 * Set all MPROTx to be non-bufferable, trusted for R/W,
	 * not forced to user-mode.
	 */
	writel(0x77777777, MX35_AIPS1_BASE_ADDR);
	writel(0x77777777, MX35_AIPS1_BASE_ADDR + 0x4);
	writel(0x77777777, MX35_AIPS2_BASE_ADDR);
	writel(0x77777777, MX35_AIPS2_BASE_ADDR + 0x4);

	/*
	 * Clear the on and off peripheral modules Supervisor Protect bit
	 * for SDMA to access them. Did not change the AIPS control registers
	 * (offset 0x20) access type
	 */
	writel(0x0, MX35_AIPS1_BASE_ADDR + 0x40);
	writel(0x0, MX35_AIPS1_BASE_ADDR + 0x44);
	writel(0x0, MX35_AIPS1_BASE_ADDR + 0x48);
	writel(0x0, MX35_AIPS1_BASE_ADDR + 0x4C);
	tmp = readl(MX35_AIPS1_BASE_ADDR + 0x50);
	tmp &= 0x00FFFFFF;
	writel(tmp, MX35_AIPS1_BASE_ADDR + 0x50);

	writel(0x0, MX35_AIPS2_BASE_ADDR + 0x40);
	writel(0x0, MX35_AIPS2_BASE_ADDR + 0x44);
	writel(0x0, MX35_AIPS2_BASE_ADDR + 0x48);
	writel(0x0, MX35_AIPS2_BASE_ADDR + 0x4C);
	tmp = readl(MX35_AIPS2_BASE_ADDR + 0x50);
	tmp &= 0x00FFFFFF;
	writel(tmp, MX35_AIPS2_BASE_ADDR + 0x50);

	/* MAX (Multi-Layer AHB Crossbar Switch) setup */

	/* MPR - priority is M4 > M2 > M3 > M5 > M0 > M1 */
#define MAX_PARAM1 0x00302154
	writel(MAX_PARAM1, MX35_MAX_BASE_ADDR + 0x0);   /* for S0 */
	writel(MAX_PARAM1, MX35_MAX_BASE_ADDR + 0x100); /* for S1 */
	writel(MAX_PARAM1, MX35_MAX_BASE_ADDR + 0x200); /* for S2 */
	writel(MAX_PARAM1, MX35_MAX_BASE_ADDR + 0x300); /* for S3 */
	writel(MAX_PARAM1, MX35_MAX_BASE_ADDR + 0x400); /* for S4 */

	/* SGPCR - always park on last master */
	writel(0x10, MX35_MAX_BASE_ADDR + 0x10);	/* for S0 */
	writel(0x10, MX35_MAX_BASE_ADDR + 0x110);	/* for S1 */
	writel(0x10, MX35_MAX_BASE_ADDR + 0x210);	/* for S2 */
	writel(0x10, MX35_MAX_BASE_ADDR + 0x310);	/* for S3 */
	writel(0x10, MX35_MAX_BASE_ADDR + 0x410);	/* for S4 */

	/* MGPCR - restore default values */
	writel(0x0, MX35_MAX_BASE_ADDR + 0x800);	/* for M0 */
	writel(0x0, MX35_MAX_BASE_ADDR + 0x900);	/* for M1 */
	writel(0x0, MX35_MAX_BASE_ADDR + 0xa00);	/* for M2 */
	writel(0x0, MX35_MAX_BASE_ADDR + 0xb00);	/* for M3 */
	writel(0x0, MX35_MAX_BASE_ADDR + 0xc00);	/* for M4 */
	writel(0x0, MX35_MAX_BASE_ADDR + 0xd00);	/* for M5 */

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
	writel(0x40, MX35_M3IF_BASE_ADDR);

	return 0;
}

core_initcall(kindle3_core_setup);
