// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2012 Antony Pavlov <antonynpavlov@gmail.com>
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
