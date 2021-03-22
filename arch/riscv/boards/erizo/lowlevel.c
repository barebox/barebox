// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2016 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * This file is part of barebox.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <memory.h>
#include <asm-generic/memory_layout.h>
#include <asm/sections.h>

void main_entry(void);

/**
 * Called plainly from assembler code
 *
 * @note The C environment isn't initialized yet
 */
void main_entry(void)
{
	/* clear the BSS first */
	memset(__bss_start, 0x00, __bss_stop - __bss_start);

	mem_malloc_init((void *)MALLOC_BASE,
			(void *)(MALLOC_BASE + MALLOC_SIZE - 1));

	start_barebox();
}
