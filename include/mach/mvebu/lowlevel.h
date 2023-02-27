/* SPDX-License-Identifier: GPL-2.0-or-later */
/* SPDX-FileCopyrightText: 2013 Thomas Petazzoni <thomas.petazzoni@free-electrons.com> */
/* SPDX-FileCopyrightText: 2013 Sebastian Hesselbarth <sebastian.hesselbarth@gmail.com> */

#ifndef __MACH_LOWLEVEL_H__
#define __MACH_LOWLEVEL_H__

void mvebu_barebox_entry(void *boarddata);
void dove_barebox_entry(void *boarddata);
void kirkwood_barebox_entry(void *boarddata);
void armada_370_xp_barebox_entry(void *boarddata);

#endif
