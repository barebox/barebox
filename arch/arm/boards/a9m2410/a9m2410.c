/*
 * Copyright (C) 2009 Juergen Beisert, Pengutronix
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
 *
 */

/**
 * @file
 * @brief a9m2410 Specific Board Initialization routines
 *
 */

#include <common.h>
#include <driver.h>
#include <init.h>
#include <asm/armlinux.h>
#include <generated/mach-types.h>
#include <asm/sections.h>
#include <partition.h>
#include <nand.h>
#include <io.h>
#include <mach/devices-s3c24xx.h>
#include <mach/s3c-iomap.h>
#include <mach/s3c24xx-nand.h>
#include <mach/s3c-generic.h>
#include <mach/s3c-busctl.h>
#include <mach/s3c24xx-gpio.h>

// {"NAND 1MiB 3,3V 8-bit", 0xec, 256, 1, 0x1000, 0},
static struct s3c24x0_nand_platform_data nand_info = {
	.nand_timing = CALC_NFCONF_TIMING(A9M2410_TACLS, A9M2410_TWRPH0, A9M2410_TWRPH1)
};

static int a9m2410_mem_init(void)
{
	resource_size_t size;

	/*
	 * Note: On this card the second SDRAM page is not used
	 */
	s3c24xx_disable_second_sdram_bank();
	size = s3c24xx_get_memory_size();

	/* ---------- configure the GPIOs ------------- */
	writel(0x007FFFFF, S3C_GPACON);
	writel(0x00000000, S3C_GPCCON);
	writel(0x00000000, S3C_GPCUP);
	writel(0x00000000, S3C_GPDCON);
	writel(0x00000000, S3C_GPDUP);
	writel(0xAAAAAAAA, S3C_GPECON);
	writel(0x0000E03F, S3C_GPEUP);
	writel(0x00000000, S3C_GPBCON);	/* all inputs */
	writel(0x00000007, S3C_GPBUP);	/* pullup disabled for GPB0..3 */
	writel(0x00009000, S3C_GPFCON);	/* GPF7 CLK_INT#, GPF6 Debug-LED */
	writel(0x000000FF, S3C_GPFUP);
	writel(readl(S3C_GPGDAT) | 0x0010, S3C_GPGDAT);	/* switch off LCD backlight */
	writel(0xFF00A938, S3C_GPGCON);	/* switch off USB device */
	writel(0x0000F000, S3C_GPGUP);
	writel(readl(S3C_GPHDAT) | 0x100, S3C_GPHDAT);	/* switch BOOTINT/GPIO_ON# to high */
	writel(0x000007FF, S3C_GPHUP);
	writel(0x0029FAAA, S3C_GPHCON);
	/*
	 * USB port1 normal, USB port0 normal, USB1 pads for device
	 * PCLK output on CLKOUT0, UPLL CLK output on CLKOUT1,
	 * 2nd SDRAM bank off (only bank 1 is used)
	 */
	writel(0x40140, S3C_MISCCR);

	arm_add_mem_device("ram0", S3C_SDRAM_BASE, size);

	return 0;
}
mem_initcall(a9m2410_mem_init);

static int a9m2410_devices_init(void)
{
	uint32_t reg;

	/* ----------- configure the access to the outer space ---------- */
	reg = readl(S3C_BWSCON);

	/* CS#1 to access the network controller */
	reg &= ~0xf0;
	reg |= 0xe0;
	writel(0x1350, S3C_BANKCON1);

	/* CS#2 to the dual 16550 UART */
	reg &= ~0xf00;
	reg |= 0x400;
	writel(0x0d50, S3C_BANKCON2);

	writel(reg, S3C_BWSCON);

	/* release the reset signal to the network and UART device */
	reg = readl(S3C_MISCCR);
	reg |= 0x10000;
	writel(reg, S3C_MISCCR);

	/* ----------- the devices the boot loader should work with -------- */
	s3c24xx_add_nand(&nand_info);
	/*
	 * SMSC 91C111 network controller on the baseboard
	 * connected to CS line 1 and interrupt line
	 * GPIO3, data width is 32 bit
	 */
	add_generic_device("smc91c111", DEVICE_ID_DYNAMIC, NULL, S3C_CS1_BASE + 0x300,
			16, IORESOURCE_MEM, NULL);

#ifdef CONFIG_NAND
	/* ----------- add some vital partitions -------- */
	devfs_add_partition("nand0", 0x00000, 0x40000, DEVFS_PARTITION_FIXED, "self_raw");
	dev_add_bb_dev("self_raw", "self0");

	devfs_add_partition("nand0", 0x40000, 0x20000, DEVFS_PARTITION_FIXED, "env_raw");
	dev_add_bb_dev("env_raw", "env0");
#endif

	armlinux_set_bootparams((void*)S3C_SDRAM_BASE + 0x100);
	armlinux_set_architecture(MACH_TYPE_A9M2410);

	return 0;
}

device_initcall(a9m2410_devices_init);

static int a9m2410_console_init(void)
{
	barebox_set_model("Digi A9M2410");
	barebox_set_hostname("a9m2410");

	s3c24xx_add_uart1();
	return 0;
}

console_initcall(a9m2410_console_init);

/** @page a9m2410 DIGI's a9m2410

This CPU card is based on a Samsung S3C2410 CPU. The card is shipped with:

- S3C2410\@200 MHz (ARM920T/ARMv4T)
- 12MHz crystal reference
- SDRAM 32 MiB
   - Samsung K4M563233E-EE1H
   - 2M x 32Bit x 4 Banks Mobile SDRAM
   - 90 pin FBGA
   - CL3\@133MHz, CL2\@100MHz (CAS/RAS delay 19ns)
   - four banks
   - 32 bit data bits
   - row address size is 11
   - Row cycle time: 69ns
   - collumn address size is 9 bits
   - Extended temperature range (-25°C...85°C)
   - 64ms refresh period (4k)
- NAND Flash 32 MiB
   - Samsung KM29U256T
   - 32MiB 3,3V 8-bit
   - ID: 0xEC, 0x75, 0x??, 0xBD
   - 30ns/40ns/20ns
- I2C interface, 100KHz and 400KHz
  - Real Time Clock
    - Dallas DS1337
    - address 0x68
  - EEPROM
    - ST M24LC64
    - address 0x50
    - 16bit addressing
- LCD interface
- Touch Screen interface
- Camera interface
- I2S interface
- AC97 Audio-CODEC interface
- SD card interface
- 3 serial RS232 interfaces
- Host and device USB interface, USB1.1 compliant
- Ethernet interface
  - 10Mbps, Cirrus Logic, CS8900A (on the CPU card) or
  - 10/100Mbps, SMSC 91C111 (on the baseboard)
- SPI interface
- JTAG interface

How to get the binary image:

Using the default configuration:

@code
make ARCH=arm a9m2410_defconfig
@endcode

Build the binary image:

@code
make ARCH=arm CROSS_COMPILE=armv4compiler
@endcode

@note replace the armv4compiler with your ARM v4 cross compiler.
*/
