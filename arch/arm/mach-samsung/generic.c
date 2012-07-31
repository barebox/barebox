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
#include <io.h>
#include <mach/s3c-iomap.h>
#include <mach/s3c-generic.h>

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
