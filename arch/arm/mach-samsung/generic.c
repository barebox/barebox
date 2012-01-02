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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
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
#include <mach/s3c-iomap.h>
#include <mach/s3c-generic.h>

/**
 * Calculate the amount of connected and available memory
 * @return Memory size in bytes
 */
uint32_t s3c24x0_get_memory_size(void)
{
	uint32_t reg, size;

	/*
	 * detect the current memory size
	 */
	reg = readl(BANKSIZE);

	switch (reg & 0x7) {
	case 0:
		size = 32 * 1024 * 1024;
		break;
	case 1:
		size = 64 * 1024 * 1024;
		break;
	case 2:
		size = 128 * 1024 * 1024;
		break;
	case 4:
		size = 2 * 1024 * 1024;
		break;
	case 5:
		size = 4 * 1024 * 1024;
		break;
	case 6:
		size = 8 * 1024 * 1024;
		break;
	default:
		size = 16 * 1024 * 1024;
		break;
	}

	/*
	 * Is bank7 also configured for SDRAM usage?
	 */
	if ((readl(BANKCON7) & (0x3 << 15)) == (0x3 << 15))
		size <<= 1;	/* also count this bank */

	return size;
}

#define S3C_WTCON (S3C_WATCHDOG_BASE)
#define S3C_WTDAT (S3C_WATCHDOG_BASE + 0x04)
#define S3C_WTCNT (S3C_WATCHDOG_BASE + 0x08)

void __noreturn reset_cpu(unsigned long addr)
{
	/* Disable watchdog */
	writew(0x0000, S3C_WTCON);

	/* Initialize watchdog timer count register */
	writew(0x0001, S3C_WTCNT);

	/* Enable watchdog timer; assert reset at timer timeout */
	writew(0x0021, S3C_WTCON);

	/* loop forever and wait for reset to happen */
	while(1)
		;
}
EXPORT_SYMBOL(reset_cpu);

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
