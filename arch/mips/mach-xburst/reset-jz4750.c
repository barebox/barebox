/*
 * Copyright (C) 2012 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * This file is part of barebox.
 * See file CREDITS for list of people who contributed to this project.
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
 */

/**
 * @file
 * @brief Resetting an JZ4755-based board
 */

#include <common.h>
#include <io.h>
#include <init.h>
#include <poweroff.h>
#include <mach/jz4750d_regs.h>

static void __noreturn jz4750d_halt(void)
{
	while (1) {
		__asm__(".set push;\n"
			".set mips3;\n"
			"wait;\n"
			".set pop;\n"
		);
	}

	unreachable();
}

static void __noreturn jz4750_poweroff(struct poweroff_handler *handler)
{
	u32 ctrl;

	shutdown_barebox();

	do {
		ctrl = readl((u32 *)RTC_RCR);
	} while (!(ctrl & RTC_RCR_WRDY));

	writel(RTC_HCR_PD, (u32 *)RTC_HCR);
	jz4750d_halt();
}

static int jz4750_init(void)
{
	poweroff_handler_register_fn(jz4750_poweroff);

	return 0;
}
coredevice_initcall(jz4750_init);
