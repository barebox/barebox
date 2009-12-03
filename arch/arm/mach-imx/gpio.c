/*
 *  arch/arm/mach-imx/gpio.c
 *
 *  author: Sascha Hauer
 *  Created: april 20th, 2004
 *  Copyright: Synertronixx GmbH
 *
 *  Common code for i.MX machines
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <common.h>
#include <mach/imx-regs.h>

void gpio_set_value(unsigned gpio, int value)
{
	if(value)
		DR(gpio >> GPIO_PORT_SHIFT) |= (1 << (gpio & GPIO_PIN_MASK));
	else
		DR(gpio >> GPIO_PORT_SHIFT) &= ~(1 << (gpio & GPIO_PIN_MASK));
}

int gpio_direction_input(unsigned gpio)
{
	imx_gpio_mode(gpio | GPIO_IN | GPIO_GIUS | GPIO_GPIO);
	return 0;
}


int gpio_direction_output(unsigned gpio, int value)
{
	gpio_set_value(gpio, value);
	imx_gpio_mode(gpio | GPIO_OUT | GPIO_GIUS | GPIO_GPIO);
	return 0;
}


