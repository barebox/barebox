/* -*- linux-c -*- ------------------------------------------------------- *
 *
 *   Copyright (C) 1991, 1992 Linus Torvalds
 *   Copyright 2007 rPath, Inc. - All Rights Reserved
 *
 *   This file is part of the Linux kernel, and is made available under
 *   the terms of the GNU General Public License version 2.
 *
 * ----------------------------------------------------------------------- */

/*
 * Prepare the machine for transition to protected mode.
 */
#include <asm/segment.h>
#include <asm/modes.h>
#include <io.h>
#include "boot.h"

/* be aware of: */
THIS_IS_REALMODE_CODE

/*
 * While we are in flat mode, we can't handle interrupts. But we can't
 * switch them off for ever in the PIC, because we need them again while
 * entering real mode code again and again....
 */
static void __bootcode realmode_switch_hook(void)
{
	asm volatile("cli");
	outb(0x80, 0x70); /* Disable NMI */
	io_delay();
}

/*
 * Reset IGNNE# if asserted in the FPU.
 */
static void __bootcode reset_coprocessor(void)
{
	outb(0, 0xf0);
	io_delay();
	outb(0, 0xf1);
	io_delay();
}

/**
 * Setup and register the global descriptor table (GDT)
 *
 * @note This is for the first time only
 */
static void __bootcode setup_gdt(void)
{
	/* Xen HVM incorrectly stores a pointer to the gdt_ptr, instead
	   of the gdt_ptr contents.  Thus, make it static so it will
	   stay in memory, at least long enough that we switch to the
	   proper kernel GDT. */
	static struct gdt_ptr __bootdata gdt_ptr;

	gdt_ptr.len = gdt_size - 1;
	gdt_ptr.ptr = (uint32_t)&gdt + (ds() << 4);

	asm volatile("lgdtl %0" : : "m" (gdt_ptr));
}

static char a20_message[] __bootdata = "A20 gate not responding, unable to boot...\n";

/*
 * Actual invocation sequence
 */
void __bootcode start_pre_uboot(void)
{
	/* Hook before leaving real mode, also disables interrupts */
	realmode_switch_hook();

	/* Enable the A20 gate */
	if (enable_a20()) {
		boot_puts(a20_message);
		die();
	}

	/* Reset coprocessor (IGNNE#) */
	reset_coprocessor();

	setup_gdt();
	/* Actual transition to protected mode... */
	protected_mode_jump();
}
