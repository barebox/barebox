/*
 * (C) Copyright 2000-2003
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
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

#include <common.h>
#include <mach/mpc5xxx.h>
#include <types.h>

/*
 * Breath some life into the CPU...
 *
 * initialize a bunch of registers.
 */
int cpu_init(void)
{
	/* enable timebase */
	*(vu_long *)(MPC5XXX_XLBARB + 0x40) |= (1 << 13);

	/* Enable snooping for RAM */
	*(vu_long *)(MPC5XXX_XLBARB + 0x40) |= (1 << 15);
	*(vu_long *)(MPC5XXX_XLBARB + 0x70) = 0 | 0x1d;

	/* Configure the XLB Arbiter */
	*(vu_long *)MPC5XXX_XLBARB_MPRIEN = 0xff;
	*(vu_long *)MPC5XXX_XLBARB_MPRIVAL = 0x11111111;

	/* mask all interrupts */
	*(vu_long *)MPC5XXX_ICTL_PER_MASK = 0xffffff00;

	*(vu_long *)MPC5XXX_ICTL_CRIT |= 0x0001ffff;
	*(vu_long *)MPC5XXX_ICTL_EXT &= ~0x00000f00;
	/* route critical ints to normal ints */
	*(vu_long *)MPC5XXX_ICTL_EXT |= 0x00000001;

	return 0;
}

