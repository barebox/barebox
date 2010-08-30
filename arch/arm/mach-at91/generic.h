/*
 * linux/arch/arm/mach-at91/generic.h
 *
 *  Copyright (C) 2005 David Brownell
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

 /* Clocks */
extern int __init at91_clock_init(unsigned long main_clock);
struct device_d;
extern void __init at91_clock_associate(const char *id, struct device_d *dev, const char *func);
