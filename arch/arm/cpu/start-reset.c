/*
 * start-reset.c
 *
 * Copyright (c) 2010 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <common.h>
#include <init.h>
#include <asm/system.h>
#include <asm/barebox-arm.h>
#include <asm/sections.h>

/*
 * The actual reset vector. This code is position independent and usually
 * does not run at the address it's linked at.
 */
void __naked __bare_init reset(void)
{
	uint32_t r;

	/* set the cpu to SVC32 mode */
	__asm__ __volatile__("mrs %0, cpsr":"=r"(r));
	r &= ~0x1f;
	r |= 0xd3;
	__asm__ __volatile__("msr cpsr, %0" : : "r"(r));

#ifdef CONFIG_ARCH_HAS_LOWLEVEL_INIT
	arch_init_lowlevel();
#endif

	/* disable MMU stuff and caches */
	r = get_cr();
	r &= ~(CR_M | CR_C | CR_B | CR_S | CR_R | CR_V);
	r |= CR_I;

#if __LINUX_ARM_ARCH__ >= 6
	r |= CR_U;
#else
	r |= CR_A;
#endif

#ifdef __ARMEB__
	r |= CR_B;
#endif
	set_cr(r);

#ifdef CONFIG_MACH_DO_LOWLEVEL_INIT
	board_init_lowlevel();
#endif
	board_init_lowlevel_return();
}
