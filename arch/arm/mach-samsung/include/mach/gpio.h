/*
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

#if defined(CONFIG_CPU_S3C2440) || defined(CONFIG_CPU_S3C2410)
# include <mach/iomux-s3c24x0.h>
#endif

void gpio_set_value(unsigned, int);
int gpio_direction_input(unsigned);
int gpio_direction_output(unsigned, int);
int gpio_get_value(unsigned);
void s3c_gpio_mode(unsigned);

#endif /* __ASM_MACH_GPIO_H */
