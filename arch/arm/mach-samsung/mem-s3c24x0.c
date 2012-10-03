/*
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
 */
/**
 * @file
 * @brief Basic clock, sdram and timer handling for S3C24xx CPUs
 */

#include <config.h>
#include <common.h>
#include <init.h>
#include <clock.h>
#include <io.h>
#include <sizes.h>
#include <mach/s3c-iomap.h>
#include <mach/s3c-generic.h>
#include <mach/s3c-busctl.h>
#include <mach/s3c24xx-gpio.h>

/**
 * Calculate the amount of connected and available memory
 * @return Memory size in bytes
 */
uint32_t s3c24xx_get_memory_size(void)
{
	uint32_t reg, size;

	/*
	 * detect the current memory size
	 */
	reg = readl(S3C_BANKSIZE);

	switch (reg & 0x7) {
	case 0:
		size = SZ_32M;
		break;
	case 1:
		size = SZ_64M;
		break;
	case 2:
		size = SZ_128M;
		break;
	case 4:
		size = SZ_2M;
		break;
	case 5:
		size = SZ_4M;
		break;
	case 6:
		size = SZ_8M;
		break;
	default:
		size = SZ_16M;
		break;
	}

	/*
	 * Is bank7 also configured for SDRAM usage?
	 */
	if ((readl(S3C_BANKCON7) & (0x3 << 15)) == (0x3 << 15))
		size <<= 1;	/* also count this bank */

	return size;
}

void s3c24xx_disable_second_sdram_bank(void)
{
	writel(readl(S3C_BANKCON7) & ~(0x3 << 15), S3C_BANKCON7);
	writel(readl(S3C_MISCCR) | (1 << 18), S3C_MISCCR); /* disable its clock */
}

/**

@page dev_s3c24xx_arch Samsung's S3C24xx Platforms in barebox

@section s3c24xx_boards Boards using S3C24xx Processors

@li @subpage arch/arm/boards/a9m2410/a9m2410.c
@li @subpage arch/arm/boards/a9m2440/a9m2440.c

@section s3c24xx_arch Documentation for S3C24xx Architectures Files

@li @subpage arch/arm/mach-s3c24xx/generic.c

@section s3c24xx_mem_map SDRAM Memory Map

SDRAM starts at address 0x3000.0000 up to the available amount of connected
SDRAM memory. Physically this CPU can handle up to 256MiB (two areas with
up to 128MiB each).

@subsection s3c24xx_mem_generic_map Generic Map
- 0x0000.0000 Start of the internal SRAM when booting from NAND flash memory or CS signal to a NOR flash memory.
- 0x0800.0000 Start of I/O space.
- 0x3000.0000 Start of SDRAM area.
  - 0x3000.0100 Start of the TAG list area.
  - 0x3000.8000 Start of the linux kernel (physical address).
- 0x4000.0000 Start of internal SRAM, when booting from NOR flash memory
- 0x4800.0000 Start of the internal I/O area

@section s3c24xx_asm_arm include/asm-arm/arch-s3c24xx directory guidelines
All S3C24xx common headers are located here.

@note Do not add board specific header files/information here.
*/

/** @page dev_s3c24xx_mach Samsung's S3C24xx based platforms

@par barebox Map

The location of the @a barebox itself depends on the available amount of
installed SDRAM memory:

- 0x30fc.0000 Start of @a barebox when 16MiB SDRAM is available
- 0x31fc.0000 Start of @a barebox when 32MiB SDRAM is available
- 0x33fc.0000 Start of @a barebox when 64MiB SDRAM is available

Adjust the @p CONFIG_TEXT_BASE/CONFIG_ARCH_TEXT_BASE symbol in accordance to
the available memory.

@note The RAM based filesystem and the stack resides always below the
@a barebox start address.

@li @subpage dev_s3c24xx_wd_handling
@li @subpage dev_s3c24xx_pll_handling
@li @subpage dev_s3c24xx_sdram_handling
@li @subpage dev_s3c24xx_nandboot_handling
*/
