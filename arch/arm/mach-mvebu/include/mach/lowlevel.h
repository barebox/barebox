/*
 * Copyright (C) 2013
 *  Thomas Petazzoni <thomas.petazzoni@free-electrons.com>
 *  Sebastian Hesselbarth <sebastian.hesselbarth@gmail.com>
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

#ifndef __MACH_LOWLEVEL_H__
#define __MACH_LOWLEVEL_H__

void mvebu_barebox_entry(void *boarddata);
void dove_barebox_entry(void *boarddata);
void kirkwood_barebox_entry(void *boarddata);
void armada_370_xp_barebox_entry(void *boarddata);

#endif
