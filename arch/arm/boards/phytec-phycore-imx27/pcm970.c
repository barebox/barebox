/*
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
 */

#include <common.h>
#include <gpio.h>
#include <init.h>
#include <io.h>
#include <platform_ide.h>
#include <sizes.h>
#include <mach/imx27-regs.h>
#include <mach/iomux-mx27.h>

#define GPIO_IDE_POWER	(GPIO_PORTE + 18)
#define GPIO_IDE_PCOE	(GPIO_PORTF + 7)
#define GPIO_IDE_RESET	(GPIO_PORTF + 10)

static struct resource pcm970_ide_resources[] = {
	DEFINE_RES_MEM(MX27_PCMCIA_MEM_BASE_ADDR, SZ_1K),
};

static void pcm970_ide_reset(int state)
{
	/* Switch reset line to low/high state */
	gpio_set_value(GPIO_IDE_RESET, !!state);
}

static struct ide_port_info pcm970_ide_pdata = {
	.ioport_shift	= 0,
	.reset		= &pcm970_ide_reset,
};

static struct device_d pcm970_ide_device = {
	.id		= DEVICE_ID_DYNAMIC,
	.name		= "ide_intf",
	.num_resources	= ARRAY_SIZE(pcm970_ide_resources),
	.resource	= pcm970_ide_resources,
	.platform_data	= &pcm970_ide_pdata,
};

static const unsigned int pcmcia_pins[] = {
	PF20_PF_PC_CD1,
	PF19_PF_PC_CD2,
	PF18_PF_PC_WAIT,
	PF17_PF_PC_READY,
	PF16_PF_PC_PWRON,
	PF14_PF_PC_VS1,
	PF13_PF_PC_VS2,
	PF12_PF_PC_BVD1,
	PF11_PF_PC_BVD2,
	PF9_PF_PC_IOIS16,
	PF8_PF_PC_RW,
	GPIO_IDE_PCOE | GPIO_GPIO | GPIO_OUT,	/* PCOE */
	GPIO_IDE_RESET | GPIO_GPIO | GPIO_OUT,	/* Reset */
	GPIO_IDE_POWER | GPIO_GPIO | GPIO_OUT,	/* Power */
};

static int pcm970_init(void)
{
	if (!of_machine_is_compatible("phytec,imx27-pcm970"))
		return 0;

	if (IS_ENABLED(CONFIG_DISK_INTF_PLATFORM_IDE)) {
		uint32_t i;

		for (i = 0; i < ARRAY_SIZE(pcmcia_pins); i++)
			imx_gpio_mode(pcmcia_pins[i] | GPIO_PUEN);

		/* Always set PCOE signal to low */
		gpio_set_value(GPIO_IDE_PCOE, 0);

		/* Assert RESET line */
		gpio_set_value(GPIO_IDE_RESET, 0);

		/* Power up CF-card (Also switched on User-LED) */
		gpio_set_value(GPIO_IDE_POWER, 1);
		mdelay(10);

		/* Reset PCMCIA Status Change Register */
		writel(0x00000fff, MX27_PCMCIA_CTL_BASE_ADDR + MX27_PCMCIA_PSCR);
		mdelay(10);

		/* Check PCMCIA Input Pins Register for Card Detect & Power */
		if ((readl(MX27_PCMCIA_CTL_BASE_ADDR + MX27_PCMCIA_PIPR) &
			   ((1 << 8) | (3 << 3))) != (1 << 8)) {
			printf("CF card not found. Driver not enabled.\n");
			return 0;
		}

		/* Disable all interrupts */
		writel(0, MX27_PCMCIA_CTL_BASE_ADDR + MX27_PCMCIA_PER);

		/* Disable all PCMCIA banks */
		for (i = 0; i < 5; i++)
			writel(0, MX27_PCMCIA_CTL_BASE_ADDR +
			       MX27_PCMCIA_POR(i));

		/* Not use internal PCOE */
		writel(0, MX27_PCMCIA_CTL_BASE_ADDR + MX27_PCMCIA_PGCR);

		/* Setup PCMCIA bank0 for Common memory mode */
		writel(0, MX27_PCMCIA_CTL_BASE_ADDR + MX27_PCMCIA_PBR(0));
		writel(0, MX27_PCMCIA_CTL_BASE_ADDR + MX27_PCMCIA_POFR(0));
		writel((0 << 25) | (17 << 17) | (4 << 11) | (3 << 5) | 0xf,
		       MX27_PCMCIA_CTL_BASE_ADDR + MX27_PCMCIA_POR(0));

		/* Clear PCMCIA General Status Register */
		writel(0x0000001f, MX27_PCMCIA_CTL_BASE_ADDR + MX27_PCMCIA_PGSR);

		/* Make PCMCIA bank0 valid */
		i = readl(MX27_PCMCIA_CTL_BASE_ADDR + MX27_PCMCIA_POR(0));
		writel(i | (1 << 29), MX27_PCMCIA_CTL_BASE_ADDR +
		       MX27_PCMCIA_POR(0));

		platform_device_register(&pcm970_ide_device);
	}

	return 0;
}
late_initcall(pcm970_init);
