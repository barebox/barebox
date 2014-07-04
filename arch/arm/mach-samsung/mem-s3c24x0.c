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
