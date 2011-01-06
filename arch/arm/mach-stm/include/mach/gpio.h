/*
 * (C) Copyright 2010 Juergen Beisert - Pengutronix
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef __ASM_MACH_GPIO_H
#define __ASM_MACH_GPIO_H

#include <types.h>

#if defined CONFIG_ARCH_IMX23
# include <mach/iomux-imx23.h>
#endif
#if defined CONFIG_ARCH_IMX28
# include <mach/iomux-imx28.h>
#endif

void imx_gpio_mode(uint32_t);
void gpio_set_value(unsigned, int);
int gpio_direction_input(unsigned);
int gpio_direction_output(unsigned, int);
int gpio_get_value(unsigned);

#endif /* __ASM_MACH_GPIO_H */
