// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2016, 2018 Antony Pavlov <antonynpavlov@gmail.com>
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
#include <init.h>
#include <of.h>

extern char __dtb_start[];

static int of_riscv_init(void)
{
	barebox_register_fdt(__dtb_start);

	return 0;
}
core_initcall(of_riscv_init);
