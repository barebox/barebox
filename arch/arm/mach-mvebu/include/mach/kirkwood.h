/*
 * Copyright (C) 2013 Thomas Petazzoni <thomas.petazzoni@free-electrons.com>
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

#ifndef __MACH_KIRKWOOD_H
#define __MACH_KIRKWOOD_H

int kirkwood_add_uart0(void);
void __naked __noreturn kirkwood_barebox_entry(void);

#endif /* __MACH_KIRKWOOD_H */
