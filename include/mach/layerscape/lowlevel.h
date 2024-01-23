/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __MACH_LOWLEVEL_H
#define __MACH_LOWLEVEL_H

void ls1046a_init_lowlevel(void);
void ls1028a_init_lowlevel(void);
void ls1046a_init_l2_latency(void);
void ls102xa_init_lowlevel(void);

unsigned long ls1028a_tzc400_init(unsigned long memsize);

#endif /* __MACH_LOWLEVEL_H */
