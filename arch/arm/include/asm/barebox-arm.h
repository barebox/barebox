/*
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 *
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Alex Zuepke <azu@sysgo.de>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
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
 */

#ifndef _BAREBOX_ARM_H_
#define _BAREBOX_ARM_H_

/* cpu/.../cpu.c */
int	cleanup_before_linux(void);

/* arch/<arch>board(s)/.../... */
int	board_init(void);
int	dram_init (void);

extern char __exceptions_start[], __exceptions_stop[];

void board_init_lowlevel(void);
void __noreturn board_init_lowlevel_return(void);
uint32_t get_runtime_offset(void);

void setup_c(void);
void __noreturn barebox_arm_entry(uint32_t membase, uint32_t memsize, uint32_t boarddata);

#endif	/* _BAREBOX_ARM_H_ */
