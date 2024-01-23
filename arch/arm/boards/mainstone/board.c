// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2015 Robert Jarzmik <robert.jarzmik@free.fr>

#include <common.h>
#include <driver.h>
#include <environment.h>
#include <fs.h>
#include <init.h>
#include <led.h>
#include <gpio.h>
#include <pwm.h>
#include <linux/sizes.h>

#include <mach/pxa/devices.h>
#include <mach/pxa/mfp-pxa27x.h>
#include <mach/pxa/pxa-regs.h>
#include <mach/pxa/udc_pxa2xx.h>
#include <mach/pxa/mci_pxa2xx.h>

#include <platform_data/eth-smc91111.h>
#include <asm/armlinux.h>
#include <asm/io.h>
#include <asm/mmu.h>

#include <asm/mach-types.h>

static struct smc91c111_pdata smsc91x_pdata = {
	.word_aligned_short_writes = true,
};

static unsigned long mainstone_pin_config[] = {
	GPIO15_nCS_1,	/* CS1 - Flash */
	GPIO78_nCS_2,	/* CS2 - Baseboard FGPA + SRAM */
	GPIO80_nCS_4,	/* CS4 - SMC ethernet */

	/* Ethernet: static memory VLIO */
	GPIO18_RDY,

	/* PC Card */
	GPIO48_nPOE,
	GPIO49_nPWE,
	GPIO50_nPIOR,
	GPIO51_nPIOW,
	GPIO85_nPCE_1,
	GPIO54_nPCE_2,
	GPIO79_PSKTSEL,
	GPIO55_nPREG,
	GPIO56_nPWAIT,
	GPIO57_nIOIS16,

	/* MMC */
	GPIO32_MMC_CLK,
	GPIO112_MMC_CMD,
	GPIO92_MMC_DAT_0,
	GPIO109_MMC_DAT_1,
	GPIO110_MMC_DAT_2,
	GPIO111_MMC_DAT_3,

	/* LCD - 16bpp DSTN */
	GPIOxx_LCD_TFT_16BPP,

	/* Backlight */
	GPIO16_PWM0_OUT,

	/* FFUART */
	GPIO34_FFUART_RXD,
	GPIO35_FFUART_CTS,
	GPIO36_FFUART_DCD,
	GPIO37_FFUART_DSR,
	GPIO38_FFUART_RI,
	GPIO39_FFUART_TXD,
	GPIO40_FFUART_DTR,
	GPIO41_FFUART_RTS,
};

static int mainstone_devices_init(void)
{
	void *nor0_iospace;

	armlinux_set_architecture(MACH_TYPE_MAINSTONE);

	pxa_add_uart((void *)0x40100000, 0);
	pxa_add_pwm((void *)0x40b00000, 0);

	nor0_iospace = map_io_sections(0x0, (void *)0xe0000000, SZ_64M);
	add_cfi_flash_device(0, (ulong)nor0_iospace, SZ_64M, 0);
	add_cfi_flash_device(1, 0x04000000, SZ_64M, 0);
	devfs_add_partition("nor0", SZ_2M, SZ_256K, DEVFS_PARTITION_FIXED,
			    "env0");
	add_generic_device("smc91c111", DEVICE_ID_DYNAMIC, NULL,
			   0x10000300, 0xff4000, IORESOURCE_MEM,
			   &smsc91x_pdata);
	return 0;
}

device_initcall(mainstone_devices_init);

static int mainstone_coredevice_init(void)
{
	/*
	 * Put the board in superspeed (520 MHz) to speed-up logo/OS loading.
	 */
	CCCR = CCCR_A | 0x20290;

	barebox_set_model("Mainstone PXA27x");
	barebox_set_hostname("mainstone");
	pxa2xx_mfp_config(ARRAY_AND_SIZE(mainstone_pin_config));
	return 0;
}
coredevice_initcall(mainstone_coredevice_init);

static int mainstone_mem_init(void)
{
	arm_add_mem_device("ram0", 0xa0000000, SZ_64M);
	add_mem_device("sram0", 0x0a000000, SZ_1M, IORESOURCE_MEM_WRITEABLE);
	return 0;
}
mem_initcall(mainstone_mem_init);
