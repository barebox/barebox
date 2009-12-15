/*
 * Copyright (c) 2008 Carsten Schlote <c.schlote@konzeptpark.de>
 * See file CREDITS for list of people who contributed to this project.
 *
 * This file is part of barebox.
 *
 * barebox is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * barebox is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with barebox.  If not, see <http://www.gnu.org/licenses/>.
 */

/** @file
 *  Early InitData support routines
 */
#include <common.h>
#include <command.h>
#include <console.h>
#include <reloc.h>
#include <init.h>

#ifdef CONFIG_HAS_EARLY_INIT

/** Returns relocation offset to early init data
 */
unsigned long reloc_offset(void)
{
	//extern char __early_init_data_begin[];
	//FIXME: return (unsigned long)init_data_ptr - (unsigned long)__early_init_data_begin;
	return 0;
}

#endif
