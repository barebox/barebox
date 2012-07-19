/*
 * start-arm.c
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
#include <asm/barebox-arm.h>
#include <asm/barebox-arm-head.h>
#include <asm/system.h>
#include <asm-generic/memory_layout.h>
#include <asm/sections.h>

void __naked __section(.text_entry) start(void)
{
	barebox_arm_head();
}

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

/*
 * Board code can jump here by either returning from board_init_lowlevel
 * or by calling this function directly.
 */
void __naked __section(.text_ll_return) board_init_lowlevel_return(void)
{
	uint32_t r, addr, offset;

	/*
	 * Get runtime address of this function. Do not
	 * put any code above this.
	 */
	__asm__ __volatile__("1: adr %0, 1b":"=r"(addr));

	/* Setup the stack */
	r = STACK_BASE + STACK_SIZE - 16;
	__asm__ __volatile__("mov sp, %0" : : "r"(r));

	/* Get offset between linked address and runtime address */
	offset = (uint32_t)__ll_return - addr;

	/* relocate to link address if necessary */
	if (offset)
		memcpy((void *)_text, (void *)(_text - offset),
				__bss_start - _text);

	/* clear bss */
	memset(__bss_start, 0, __bss_stop - __bss_start);

	/* flush I-cache before jumping to the copied binary */
	__asm__ __volatile__("mcr p15, 0, %0, c7, c5, 0" : : "r" (0));

	/* call start_barebox with its absolute address */
	r = (unsigned int)&start_barebox;
	__asm__ __volatile__("mov pc, %0" : : "r"(r));
}

