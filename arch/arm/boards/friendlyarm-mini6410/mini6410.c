/*
 * Copyright (C) 2012 Juergen Beisert
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
#include <common.h>
#include <driver.h>
#include <init.h>
#include <platform_data/eth-dm9000.h>
#include <gpio.h>
#include <envfs.h>
#include <generated/mach-types.h>
#include <asm/armlinux.h>
#include <mach/s3c-iomap.h>
#include <mach/devices-s3c64xx.h>
#include <mach/s3c-generic.h>
#include <mach/iomux.h>

/*
 * dm9000 network controller onboard
 * Connected to CS line 1 and interrupt line EINT7,
 * data width is 16 bit
 * Area 1: Offset 0x300...0x301
 * Area 2: Offset 0x304...0x305
 */
static struct dm9000_platform_data dm9000_data = {
	.srom = 0,	/* no serial ROM for the ethernet address */
};

static const unsigned pin_usage[] = {
	/* UART2 (spare, 3,3 V TTL level only) */
	GPA4_RXD1 | ENABLE_PU,
	GPA5_TXD1,
	GPA6_NCTS1 | ENABLE_PU,
	GPA7_NRTS1,
	/* UART3 (spare, 3,3 V TTL level only) */
	GPB0_RXD2 | ENABLE_PU,
	GPB1_TXD2,
	/* UART4 (spare, 3,3 V TTL level only) */
	GPB2_RXD3 | ENABLE_PU,
	GPB3_TXD3,

	GPB4_GPIO | GPIO_IN | ENABLE_PU,

	/* I2C bus */
	GPB5_IIC0_SCL,	/* external PU */
	GPB6_IIC0_SDA,	/* external PU */

	GPC0_SPI0_MISO | ENABLE_PU,
	GPC1_SPI0_CLK,
	GPC2_SPI0_MOSI,
	GPC3_SPI0_NCS,

	GPC4_SPI1_MISO | ENABLE_PU,
	GPC5_SPI1_CLK,
	GPC6_SPI1_MOSI,
	GPC7_SPI1_NCS,

	GPD0_AC97_BITCLK,
	GPD1_AC97_NRST,
	GPD2_AC97_SYNC,
	GPD3_AC97_SDI | ENABLE_PU,
	GPD4_AC97_SDO,

	GPE0_GPIO | GPIO_OUT | GPIO_VAL(0), /* LCD backlight off */
	GPE1_GPIO | GPIO_IN | ENABLE_PU,
	GPE2_GPIO | GPIO_IN | ENABLE_PU,
	GPE3_GPIO | GPIO_IN | ENABLE_PU,
	GPE4_GPIO | GPIO_IN | ENABLE_PU,

	/* keep all camera signals at reasonable values */
	GPF0_GPIO | GPIO_IN | ENABLE_PU,
	GPF1_GPIO | GPIO_IN | ENABLE_PU,
	GPF2_GPIO | GPIO_IN | ENABLE_PU,
	GPF3_GPIO | GPIO_IN | ENABLE_PU,
	GPF4_GPIO | GPIO_IN | ENABLE_PU,
	GPF5_GPIO | GPIO_IN | ENABLE_PU,
	GPF6_GPIO | GPIO_IN | ENABLE_PU,
	GPF7_GPIO | GPIO_IN | ENABLE_PU,
	GPF8_GPIO | GPIO_IN | ENABLE_PU,
	GPF9_GPIO | GPIO_IN | ENABLE_PU,
	GPF10_GPIO | GPIO_IN | ENABLE_PU,
	GPF11_GPIO | GPIO_IN | ENABLE_PU,
	GPF12_GPIO | GPIO_IN | ENABLE_PU,
	GPF13_GPIO | GPIO_OUT | GPIO_VAL(0), /* USB power off */
#if 0
	GPF14_CLKOUT, /* for testing purposes, but very noisy */
#else
	GPF14_GPIO | GPIO_OUT | GPIO_VAL(0), /* Buzzer off */
#endif
	GPF15_GPIO | GPIO_OUT | GPIO_VAL(0), /* Backlight PWM inactive */

	/* SD card slot (all signals have external 10k PU) */
	GPG0_MMC0_CLK,
	GPG1_MMC0_CMD,
	GPG2_MMC0_DAT0,
	GPG3_MMC0_DAT1,
	GPG4_MMC0_DAT2,
	GPG5_MMC0_DAT3,
	GPG6_MMC0_NCD,

	/* SDIO slot (all used signals have external PU) */
	GPH0_GPIO | GPIO_IN,	/* CLK */
	GPH1_GPIO | GPIO_IN,	/* CMD */
	GPH2_GPIO | GPIO_IN,	/* DAT0 */
	GPH3_GPIO | GPIO_IN,	/* DAT1 */
	GPH4_GPIO | GPIO_IN,	/* DAT2 */
	GPH5_GPIO | GPIO_IN,	/* DAT3 */
	GPH6_GPIO | GPIO_IN | ENABLE_PU, /* nowhere connected */
	GPH7_GPIO | GPIO_IN | ENABLE_PU, /* nowhere connected */
	GPH8_GPIO | GPIO_IN | ENABLE_PU, /* nowhere connected */
	GPH9_GPIO | GPIO_IN | ENABLE_PU, /* nowhere connected */

	/* as long as we are not using the LCD controller, disable the pins */
	GPI0_GPIO | GPIO_IN | ENABLE_PD,
	GPI1_GPIO | GPIO_IN | ENABLE_PD,
	GPI2_GPIO | GPIO_IN | ENABLE_PD,
	GPI3_GPIO | GPIO_IN | ENABLE_PD,
	GPI4_GPIO | GPIO_IN | ENABLE_PD,
	GPI5_GPIO | GPIO_IN | ENABLE_PD,
	GPI6_GPIO | GPIO_IN | ENABLE_PD,
	GPI7_GPIO | GPIO_IN | ENABLE_PD,
	GPI8_GPIO | GPIO_IN | ENABLE_PD,
	GPI9_GPIO | GPIO_IN | ENABLE_PD,
	GPI10_GPIO | GPIO_IN | ENABLE_PD,
	GPI11_GPIO | GPIO_IN | ENABLE_PD,
	GPI12_GPIO | GPIO_IN | ENABLE_PD,
	GPI13_GPIO | GPIO_IN | ENABLE_PD,
	GPI14_GPIO | GPIO_IN | ENABLE_PD,
	GPI15_GPIO | GPIO_IN | ENABLE_PD,
	GPJ0_GPIO | GPIO_IN | ENABLE_PD,
	GPJ1_GPIO | GPIO_IN | ENABLE_PD,
	GPJ2_GPIO | GPIO_IN | ENABLE_PD,
	GPJ3_GPIO | GPIO_IN | ENABLE_PD,
	GPJ4_GPIO | GPIO_IN | ENABLE_PD,
	GPJ5_GPIO | GPIO_IN | ENABLE_PD,
	GPJ6_GPIO | GPIO_IN | ENABLE_PD,
	GPJ7_GPIO | GPIO_IN | ENABLE_PD,
	GPJ8_GPIO | GPIO_IN | ENABLE_PD,
	GPJ9_GPIO | GPIO_IN | ENABLE_PD,
	GPJ10_GPIO | GPIO_IN | ENABLE_PD,
	GPJ11_GPIO | GPIO_IN | ENABLE_PD,

	GPK0_GPIO | GPIO_IN | ENABLE_PU,	/* not used */
	GPK1_GPIO | GPIO_IN | ENABLE_PU,	/* not used */
	GPK2_GPIO | GPIO_IN,
	GPK3_GPIO | GPIO_IN | ENABLE_PU,	/* not used */
	GPK4_GPIO | GPIO_OUT | GPIO_VAL(1),	/* LED #1 (high = LED off) */
	GPK5_GPIO | GPIO_OUT | GPIO_VAL(1),	/* LED #2 (high = LED off) */
	GPK6_GPIO | GPIO_OUT | GPIO_VAL(1),	/* LED #3 (high = LED off) */
	GPK7_GPIO | GPIO_OUT | GPIO_VAL(1),	/* LED #4 (high = LED off) */
	GPK8_GPIO | GPIO_IN, 			/* (external PU) */
	GPK9_GPIO | GPIO_IN | ENABLE_PU,	/* not used */
	GPK10_GPIO | GPIO_IN | ENABLE_PU,	/* not used */
	GPK11_GPIO | GPIO_IN | ENABLE_PU,	/* not used */
	GPK12_GPIO | GPIO_IN,			/* OCT_DET */
	GPK13_GPIO | GPIO_IN,			/* WIFI power (external PU) */
	GPK14_GPIO | GPIO_IN | ENABLE_PU,	/* not used */
	GPK15_GPIO | GPIO_IN | ENABLE_PU,	/* not used */

	GPL0_GPIO | GPIO_IN | ENABLE_PU,	/* not used */
	GPL1_GPIO | GPIO_IN | ENABLE_PU,	/* not used */
	GPL2_GPIO | GPIO_IN | ENABLE_PU,	/* not used */
	GPL3_GPIO | GPIO_IN | ENABLE_PU,	/* not used */
	GPL4_GPIO | GPIO_IN | ENABLE_PU,	/* not used */
	GPL5_GPIO | GPIO_IN | ENABLE_PU,	/* not used */
	GPL6_GPIO | GPIO_IN | ENABLE_PU,	/* not used */
	GPL7_GPIO | GPIO_IN | ENABLE_PU,	/* not used */
	GPL8_GPIO | GPIO_IN,			/* EINT16 (external PU) */
	GPL9_GPIO | GPIO_IN | ENABLE_PU,	/* EINT17 */
	GPL10_GPIO | GPIO_IN | ENABLE_PU,	/* EINT18 */
	GPL11_GPIO | GPIO_IN,			/* EINT19 + K7 (external PU) */
	GPL12_GPIO | GPIO_IN,			/* EINT20 + K6 (external PU) */
	GPL13_GPIO | GPIO_IN,			/* SD0 WP (external PU) */
	GPL14_GPIO | GPIO_IN,			/* SD1 WP (external PU) */

	GPM0_GPIO | GPIO_IN,			/* (external PU) */
	GPM1_GPIO | GPIO_IN,			/* (external PU) */
	GPM2_GPIO | GPIO_IN,			/* (external PU) */
	GPM3_GPIO | GPIO_IN,			/* (external PU) */
	GPM4_GPIO | GPIO_IN,			/* (external PU) */
	GPM5_GPIO | GPIO_IN,			/* (external PU) */

	GPN0_GPIO | GPIO_IN,			/* EINT0 (external PU) */
	GPN1_GPIO | GPIO_IN,			/* EINT1 (external PU) */
	GPN2_GPIO | GPIO_IN,			/* EINT2 (external PU) */
	GPN3_GPIO | GPIO_IN,			/* EINT3 (external PU) */
	GPN4_GPIO | GPIO_IN,			/* EINT4 (external PU) */
	GPN5_GPIO | GPIO_IN,			/* EINT5 (external PU) */
	GPN6_GPIO | GPIO_IN,			/* EINT6 (external PU) */
	GPN7_GPIO | GPIO_IN | ENABLE_PU,	/* EINT7 DM9000 interrupt */
	GPN8_GPIO | GPIO_IN,			/* EINT8 USB detect (external PU) */
	GPN9_GPIO | GPIO_IN,			/* EINT9 (external PU) */
	GPN10_GPIO | GPIO_IN,			/* SD1 CD (external PU) */
	GPN11_GPIO | GPIO_IN,			/* EINT11 (external PU) */
	GPN12_GPIO | GPIO_IN,			/* EINT12 IR in (external PU) */
	GPN13_GPIO | GPIO_IN,			/* BOOT0/EINT13 (externally fixed) */
	GPN14_GPIO | GPIO_IN,			/* BOOT1/EINT14 (externally fixed) */
	GPN15_GPIO | GPIO_IN,			/* BOOT2/EINT15 (externally fixed) */

	GPO0_NCS2,				/* NAND */
	GPO1_NCS3,				/* NAND */
	GPO2_NCS4,				/* CON5 */
	GPO3_GPIO | GPIO_IN | ENABLE_PU,	/* not used */
	GPO4_GPIO | GPIO_IN | ENABLE_PU,	/* CON5 pin 8 */
	GPO5_GPIO | GPIO_IN | ENABLE_PU,	/* not used */
	GPO6_ADDR6,				/* CON5 */
	GPO7_GPIO | GPIO_IN | ENABLE_PU,	/* not used */
	GPO8_GPIO | GPIO_IN | ENABLE_PU,	/* not used */
	GPO9_GPIO | GPIO_IN | ENABLE_PU,	/* not used */
	GPO10_GPIO | GPIO_IN | ENABLE_PU,	/* not used */
	GPO11_GPIO | GPIO_IN | ENABLE_PU,	/* not used */
	GPO12_GPIO | GPIO_IN | ENABLE_PU,	/* not used */
	GPO13_GPIO | GPIO_IN | ENABLE_PU,	/* not used */
	GPO14_GPIO | GPIO_IN | ENABLE_PU,	/* not used */
	GPO15_GPIO | GPIO_IN | ENABLE_PU,	/* not used */

	GPP0_GPIO | GPIO_IN | ENABLE_PU,	/* not used */
	GPP1_GPIO | GPIO_IN | ENABLE_PU,	/* not used */
	GPP2_NWAIT | ENABLE_PU,			/* CON5 */
	GPP3_FALE,				/* NAND */
	GPP4_FCLE,				/* NAND */
	GPP5_FWE,				/* NAND */
	GPP6_FRE,				/* NAND */
	GPP7_RNB,				/* NAND (external PU) */
	GPP8_GPIO | GPIO_IN | ENABLE_PU,	/* not used */
	GPP9_GPIO | GPIO_IN | ENABLE_PU,	/* not used */
	GPP10_GPIO | GPIO_IN,			/* (external PU) */
	GPP11_GPIO | GPIO_IN,			/* (external PU) */
	GPP12_GPIO | GPIO_IN | ENABLE_PU,	/* not used */
	GPP13_GPIO | GPIO_IN | ENABLE_PU,	/* not used */
	GPP14_GPIO | GPIO_IN | ENABLE_PU,	/* not used */

	GPQ0_GPIO | GPIO_IN | ENABLE_PU,	/* not used as LADDR18 */
	GPQ1_GPIO | GPIO_IN,			/* (external PU) */
	GPQ2_GPIO | GPIO_IN,			/* (external PU) */
	GPQ3_GPIO | GPIO_IN,			/* (external PU) */
	GPQ4_GPIO | GPIO_IN,			/* (external PU) */
	GPQ5_GPIO | GPIO_IN,			/* (external PU) */
	GPQ6_GPIO | GPIO_IN,			/* (external PU) */
	GPQ7_GPIO | GPIO_IN | ENABLE_PU,	/* not used as LADDR17 */
	GPQ8_GPIO | GPIO_IN | ENABLE_PU,	/* not used as LADDR16 */
};

static int mini6410_mem_init(void)
{
	arm_add_mem_device("ram0", S3C_SDRAM_BASE, s3c6410_get_memory_size());

	return 0;
}
mem_initcall(mini6410_mem_init);

static const struct s3c6410_chipselect dm900_cs = {
	.adr_setup_t = 0,
	.access_setup_t = 0,
	.access_t = 20,
	.cs_hold_t = 3,
	.adr_hold_t = 20, /* CS must be de-asserted for at least 20 ns */
	.width = 16,
};

static void mini6410_setup_dm9000_cs(void)
{
	s3c6410_setup_chipselect(1, &dm900_cs);
}

static int mini6410_devices_init(void)
{
	int i;

	/* ----------- configure the access to the outer space ---------- */
	for (i = 0; i < ARRAY_SIZE(pin_usage); i++)
		s3c_gpio_mode(pin_usage[i]);

	mini6410_setup_dm9000_cs();
	add_dm9000_device(0, S3C_CS1_BASE + 0x300, S3C_CS1_BASE + 0x304,
				IORESOURCE_MEM_16BIT, &dm9000_data);

	armlinux_set_architecture(MACH_TYPE_MINI6410);

	if (IS_ENABLED(CONFIG_DEFAULT_ENVIRONMENT_GENERIC))
		defaultenv_append_directory(defaultenv_friendlyarm_mini6410);

	return 0;
}

device_initcall(mini6410_devices_init);

static int mini6410_console_init(void)
{
	s3c_gpio_mode(GPA0_RXD0 | ENABLE_PU);
	s3c_gpio_mode(GPA1_TXD0);
	s3c_gpio_mode(GPA2_NCTS0 | ENABLE_PU);
	s3c_gpio_mode(GPA3_NRTS0);

	barebox_set_model("Friendlyarm mini6410");
	barebox_set_hostname("mini6410");

	s3c64xx_add_uart1();

	return 0;
}

console_initcall(mini6410_console_init);
