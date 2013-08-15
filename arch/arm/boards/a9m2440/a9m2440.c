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
 * @brief a9m2440 Specific Board Initialization routines
 *
 */

#include <common.h>
#include <driver.h>
#include <init.h>
#include <asm/armlinux.h>
#include <asm/sections.h>
#include <generated/mach-types.h>
#include <partition.h>
#include <nand.h>
#include <io.h>
#include <mach/devices-s3c24xx.h>
#include <mach/s3c-iomap.h>
#include <mach/s3c24xx-nand.h>
#include <mach/s3c-generic.h>
#include <mach/s3c-busctl.h>
#include <mach/s3c24xx-gpio.h>

#include "baseboards.h"

static struct s3c24x0_nand_platform_data nand_info = {
	.nand_timing = CALC_NFCONF_TIMING(A9M2440_TACLS, A9M2440_TWRPH0, A9M2440_TWRPH1)
};

static int a9m2440_check_for_ram(uint32_t addr)
{
	uint32_t tmp1, tmp2;
	int rc = 0;

	tmp1 = readl(addr);
	tmp2 = readl(addr + sizeof(uint32_t));

	writel(0xaaaaaaaa, addr);
	writel(0x55555555, addr + sizeof(uint32_t));
	if ((readl(addr) != 0xaaaaaaaa) || (readl(addr + sizeof(uint32_t)) != 0x55555555))
		rc = 1;	/* seems no RAM */

	writel(0x55555555, addr);
	writel(0xaaaaaaaa, addr + sizeof(uint32_t));
	if ((readl(addr) != 0x55555555) || (readl(addr + sizeof(uint32_t)) != 0xaaaaaaaa))
		rc = 1;	/* seems no RAM */

	writel(tmp1, addr);
	writel(tmp2, addr + sizeof(uint32_t));

	return rc;
}

static int a9m2440_mem_init(void)
{
	/*
	 * The special SDRAM setup code for this machine will always enable
	 * both SDRAM banks. But the second SDRAM device may not exists!
	 * So we must check here, if the second bank is populated to get the
	 * correct RAM size.
	 */
	switch (readl(S3C_BANKSIZE) & 0x7) {
	case 0:
		if (a9m2440_check_for_ram(S3C_SDRAM_BASE + 32 * 1024 * 1024))
			s3c24xx_disable_second_sdram_bank();
		break;
	case 1:
		if (a9m2440_check_for_ram(S3C_SDRAM_BASE + 64 * 1024 * 1024))
			s3c24xx_disable_second_sdram_bank();
		break;
	case 2:
		if (a9m2440_check_for_ram(S3C_SDRAM_BASE + 128 * 1024 * 1024))
			s3c24xx_disable_second_sdram_bank();
		break;
	case 4:
	case 5:
	case 6:		/* not supported on this machine */
		break;
	default:
		if (a9m2440_check_for_ram(S3C_SDRAM_BASE + 16 * 1024 * 1024))
			s3c24xx_disable_second_sdram_bank();
		break;
	}

	arm_add_mem_device("ram0", S3C_SDRAM_BASE, s3c24xx_get_memory_size());

	return 0;
}
mem_initcall(a9m2440_mem_init);

static int a9m2440_devices_init(void)
{
	uint32_t reg;

	/* ----------- configure the access to the outer space ---------- */
	reg = readl(S3C_BWSCON);

	/* CS#5 to access the network controller */
	reg &= ~0x00f00000;
	reg |=  0x00d00000;	/* 16 bit */
	writel(0x1f4c, S3C_BANKCON5);

	writel(reg, S3C_BWSCON);

#ifdef CONFIG_MACH_A9M2410DEV
	a9m2410dev_devices_init();
#endif

	/* release the reset signal to external devices */
	reg = readl(S3C_MISCCR);
	reg |= 0x10000;
	writel(reg, S3C_MISCCR);

	/* ----------- the devices the boot loader should work with -------- */
	s3c24xx_add_nand(&nand_info);
	/*
	 * cs8900 network controller onboard
	 * Connected to CS line 5 + A24 and interrupt line EINT9,
	 * data width is 16 bit
	 */
	add_generic_device("cs8900", DEVICE_ID_DYNAMIC, NULL,
			S3C_CS5_BASE + (1 << 24) + 0x300, 16, IORESOURCE_MEM, NULL);

#ifdef CONFIG_NAND
	/* ----------- add some vital partitions -------- */
	devfs_add_partition("nand0", 0x00000, 0x40000, DEVFS_PARTITION_FIXED, "self_raw");
	dev_add_bb_dev("self_raw", "self0");

	devfs_add_partition("nand0", 0x40000, 0x20000, DEVFS_PARTITION_FIXED, "env_raw");
	dev_add_bb_dev("env_raw", "env0");
#endif
	armlinux_set_bootparams((void*)S3C_SDRAM_BASE + 0x100);
	armlinux_set_architecture(MACH_TYPE_A9M2440);

	return 0;
}

device_initcall(a9m2440_devices_init);

static int a9m2440_console_init(void)
{
	barebox_set_model("Digi A9M2440");
	barebox_set_hostname("a9m2440");

	s3c24xx_add_uart1();
	return 0;
}

console_initcall(a9m2440_console_init);

/** @page a9m2440 DIGI's a9m2440

This CPU card is based on a Samsung S3C2440 CPU. The card is shipped with:

- S3C2440\@400 MHz or 533 MHz (ARM920T/ARMv4T)
- 16.9344 MHz crystal reference
- SDRAM 32/64/128 MiB
   - Samsung K4M563233E-EE1H (one or two devices for 32 MiB or 64 MiB)
     - 2M x 32bit x 4 Banks Mobile SDRAM
     - CL2\@100 MHz (CAS/RAS delay 19ns)
     - 105 MHz max
     - collumn address size is 9 bits
     - Row cycle time: 69ns
   - Samsung K4M513233C-DG75 (one or two devices for 64 MiB or 128 MiB)
     - 4M x 32bit x 4 Banks Mobile SDRAM
     - CL2\@100MHz (CAS/RAS delay 18ns)
     - 111 MHz max
     - collumn address size is 9 bits
     - Row cycle time: 63ns
   - 64ms refresh period (4k)
   - 90 pin FBGA
   - 32 bit data bits
   - Extended temperature range (-25°C...85°C)
- NAND Flash 32/64/128 MiB
   - Samsung KM29U512T (NAND01GW3A0AN6)
     - 64 MiB 3,3V 8-bit
     - ID: 0xEC, 0x76, 0x??, 0xBD
   - Samsung KM29U256T
     - 32 MiB 3,3V 8-bit
     - ID: 0xEC, 0x75, 0x??, 0xBD
   - ST Micro
     - 128 MiB 3,3V 8-bit
     - ID: 0x20, 0x79
   - 30ns/40ns/20ns
- I2C interface, 100 KHz and 400 KHz
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
  - 10Mbps, Cirrus Logic, CS8900A (on the CPU card)
- SPI interface
- JTAG interface

How to get the binary image:

Using the default configuration:

@code
make ARCH=arm a9m2440_defconfig
@endcode

Build the binary image:

@code
make ARCH=arm CROSS_COMPILE=armv4compiler
@endcode

@note replace the armv4compiler with your ARM v4 cross compiler.

*/
