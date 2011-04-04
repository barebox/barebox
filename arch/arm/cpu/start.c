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
#include <asm/system.h>
#include <asm-generic/memory_layout.h>
#include <asm/sections.h>

void __naked __section(.text_entry) exception_vectors(void)
{
	__asm__ __volatile__ (
		"b reset\n"				/* reset */
#ifdef CONFIG_ARM_EXCEPTIONS
		"ldr pc, =undefined_instruction\n"	/* undefined instruction */
		"ldr pc, =software_interrupt\n"		/* software interrupt (SWI) */
		"ldr pc, =prefetch_abort\n"		/* prefetch abort */
		"ldr pc, =data_abort\n"			/* data abort */
		"ldr pc, =not_used\n"			/* (reserved) */
		"ldr pc, =irq\n"			/* irq (interrupt) */
		"ldr pc, =fiq\n"			/* fiq (fast interrupt) */
#else
		"1: bne 1b\n"				/* undefined instruction */
		"1: bne 1b\n"				/* software interrupt (SWI) */
		"1: bne 1b\n"				/* prefetch abort */
		"1: bne 1b\n"				/* data abort */
		"1: bne 1b\n"				/* (reserved) */
		"1: bne 1b\n"				/* irq (interrupt) */
		"1: bne 1b\n"				/* fiq (fast interrupt) */
#endif
		".word 0x65726162\n"			/* 'bare' */
		".word 0x00786f62\n"			/* 'box' */
		".word _text\n"				/* text base. If copied there,
							 * barebox can skip relocation
							 */
		".word _barebox_image_size\n"		/* image size to copy */
	);
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
	__asm__ __volatile__ (
		"bl __mmu_cache_flush;"
		:
		:
		: "r0", "r1", "r2", "r3", "r6", "r10", "r12", "cc", "memory"
	);

	/* disable MMU stuff and caches */
	r = get_cr();
	r &= ~(CR_M | CR_C | CR_B | CR_S | CR_R | CR_V);
	r |= CR_A | CR_I;
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
 * or by calling this funtion directly.
 */
void __naked __bare_init board_init_lowlevel_return(void)
{
	uint32_t r, addr;

	/*
	 * Get runtime address of this function. Do not
	 * put any code above this.
	 */
	__asm__ __volatile__("1: adr %0, 1b":"=r"(addr));

	/* Setup the stack */
	r = STACK_BASE + STACK_SIZE - 16;
	__asm__ __volatile__("mov sp, %0" : : "r"(r));

	/* Get start of binary image */
	addr -= (uint32_t)&board_init_lowlevel_return - TEXT_BASE;

	/* relocate to link address if necessary */
	if (addr != TEXT_BASE)
		memcpy((void *)TEXT_BASE, (void *)addr,
				(unsigned int)&__bss_start - TEXT_BASE);

	/* clear bss */
	memset(__bss_start, 0, __bss_stop - __bss_start);

	/* call start_barebox with its absolute address */
	r = (unsigned int)&start_barebox;
	__asm__ __volatile__("mov pc, %0" : : "r"(r));
}

