/**
 * @file
 * @brief Wrapper to call board level initialization routine
 *
 * FileName: arch/arm/boards/omap/platform.S
 *
 * board_init_lowlevel is defined here. This calls board_init which
 * is linked to the binary - the board_init only has a SRAM stack.
 * so it needs to be careful about the usage of global variables
 * and the likes. Enabled only if CONFIG_MACH_DO_LOWLEVEL_INIT is
 * defined
 */
/*
 * (C) Copyright 2006-2008
 * Texas Instruments, <www.ti.com>
 * Nishanth Menon <x0nishan@ti.com>
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

#include <config.h>
#include <mach/silicon.h>

#ifdef CONFIG_MACH_DO_LOWLEVEL_INIT
/**
 * @fn void board_init_lowlevel(void)
 *
 * @brief This provides a assembly wrapper setting up SRAM before calling
 * board_init
 *
 * @return void
 */
.globl board_init_lowlevel
board_init_lowlevel:
	/* Setup a temporary stack so that we can call C functions
	 * Yes. this might have been already done by arch code.
	 * No harm in being a bit redundant to avoid future complications
	 */
	ldr	sp,	SRAM_STACK
        str     ip,	[sp]    /* stash old link register */
        str     lr,	[sp]    /* stash current link register */
	mov	ip,	lr	/* save link reg across call */
	/* Do the pin muxes, sdram init etc..board-xxx.c */
	bl	board_init
        ldr     lr,	[sp]    /* restore current link register */
        ldr     ip,	[sp]    /* restore save ip */
	/* back to arch calling code */
	mov	pc,	lr
SRAM_STACK:
	.word	OMAP_SRAM_STACK

#endif /* CONFIG_MACH_DO_LOWLEVEL_INIT */
