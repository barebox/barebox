// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2011 Robert Jarzmik <robert.jarzmik@free.fr>

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

#define ECOR            0x8000
#define ECOR_RESET      0x80
#define ECOR_LEVEL_IRQ  0x40
#define ECOR_WR_ATTRIB  0x04
#define ECOR_ENABLE     0x01

#define ECSR            0x8002
#define ECSR_IOIS8      0x20
#define ECSR_PWRDWN     0x04
#define ECSR_INT        0x02

static struct smc91c111_pdata smsc91x_pdata = {
	.control_setup = 0x0800,
	.config_setup = 0x10b2,
	.bus_width = 16,
	.addr_shift = 2,
};

static unsigned long lubbock_pin_config[] = {
	GPIO15_nCS_1,	/* CS1 - Flash */
	GPIO78_nCS_2,	/* CS2 - Baseboard FGPA + SRAM */
	GPIO79_nCS_3,	/* CS3 - SMC ethernet */
	GPIO80_nCS_4,	/* CS4 - SA1111 */

	/* LCD - 16bpp DSTN */
	GPIOxx_LCD_DSTN_16BPP,

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

static int lubbock_devices_init(void)
{
	void *nor0_iospace;

	armlinux_set_architecture(MACH_TYPE_LUBBOCK);

	pxa_add_uart((void *)0x40100000, 0);
	pxa_add_pwm((void *)0x40b00000, 0);

	nor0_iospace = map_io_sections(0x0, (void *)0xe0000000, SZ_64M);
	add_cfi_flash_device(0, (ulong)nor0_iospace, SZ_64M, 0);
	add_cfi_flash_device(1, 0x04000000, SZ_64M, 0);
	devfs_add_partition("nor0", SZ_2M, SZ_256K, DEVFS_PARTITION_FIXED,
			    "env0");
	add_generic_device("smc91c111", DEVICE_ID_DYNAMIC, NULL,
			   0x0c000300, 0xff4000, IORESOURCE_MEM,
			   &smsc91x_pdata);
	return 0;
}

device_initcall(lubbock_devices_init);

static void smc_init(void)
{
	/* SMC91c96 */
	void __iomem *attaddr = (void __iomem *)0x0e000000;

	writel(ECOR_RESET, attaddr + (ECOR << 2));
	mdelay(100);
	writel(0, attaddr + (ECOR << 2));
	writel(ECOR_ENABLE, attaddr + (ECOR << 2));

	/* force 16-bit mode */
	writel(0, attaddr + (ECSR << 2));
	mdelay(100);
}

static int lubbock_coredevice_init(void)
{
	barebox_set_model("Lubbock PXA25x");
	barebox_set_hostname("lubbock");
	pxa2xx_mfp_config(ARRAY_AND_SIZE(lubbock_pin_config));
	smc_init();
	return 0;
}
coredevice_initcall(lubbock_coredevice_init);

static int lubbock_mem_init(void)
{
	arm_add_mem_device("ram0", 0xa0000000, SZ_64M);
	add_mem_device("sram0", 0x0a000000, SZ_1M, IORESOURCE_MEM_WRITEABLE);
	return 0;
}
mem_initcall(lubbock_mem_init);
