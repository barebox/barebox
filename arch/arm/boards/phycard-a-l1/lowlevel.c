/*
 * (C) Copyright 2011
 * Phytec Messtechnik GmbH <www.phytec.de>
 * Juergen Kilb <j.kilb@phytec.de>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
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
#include <common.h>
#include <asm/barebox-arm.h>
#include <mach/sdrc.h>
#include <mach/omap3-silicon.h>

void __naked board_init_lowlevel(void)
{
	uint32_t r;

	/* setup a stack */
	r = OMAP_SRAM_STACK;
	__asm__ __volatile__("mov sp, %0" : : "r"(r));

	board_init();

	board_init_lowlevel_return();
}
