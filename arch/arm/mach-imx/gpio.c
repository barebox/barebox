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
#include <asm/arch/imx-regs.h>

void imx_gpio_mode(int gpio_mode)
{
	unsigned int pin = gpio_mode & GPIO_PIN_MASK;
	unsigned int port = (gpio_mode & GPIO_PORT_MASK) >> GPIO_PORT_SHIFT;
	unsigned int ocr = (gpio_mode & GPIO_OCR_MASK) >> GPIO_OCR_SHIFT;
	unsigned int aout = (gpio_mode & GPIO_AOUT_MASK) >> GPIO_AOUT_SHIFT;
	unsigned int bout = (gpio_mode & GPIO_BOUT_MASK) >> GPIO_BOUT_SHIFT;
	unsigned int tmp;

	/* Pullup enable */
	if(gpio_mode & GPIO_PUEN)
		PUEN(port) |= (1 << pin);
	else
		PUEN(port) &= ~(1 << pin);

	/* Data direction */
	if(gpio_mode & GPIO_OUT)
		DDIR(port) |= 1 << pin;
	else
		DDIR(port) &= ~(1 << pin);

	/* Primary / alternate function */
	if(gpio_mode & GPIO_AF)
		GPR(port) |= (1 << pin);
	else
		GPR(port) &= ~(1 << pin);

	/* use as gpio? */
	if(!(gpio_mode & (GPIO_PF | GPIO_AF)))
		GIUS(port) |= (1 << pin);
	else
		GIUS(port) &= ~(1 << pin);

	/* Output / input configuration */
	if (pin < 16) {
		tmp = OCR1(port);
		tmp &= ~(3 << (pin * 2));
		tmp |= (ocr << (pin * 2));
		OCR1(port) = tmp;

		ICONFA1(port) &= ~(3 << (pin * 2));
		ICONFA1(port) |= aout << (pin * 2);
		ICONFB1(port) &= ~(3 << (pin * 2));
		ICONFB1(port) |= bout << (pin * 2);
	} else {
		pin -= 16;

		tmp = OCR2(port);
		tmp &= ~(3 << (pin * 2));
		tmp |= (ocr << (pin * 2));
		OCR2(port) = tmp;

		ICONFA2(port) &= ~(3 << (pin * 2));
		ICONFA2(port) |= aout << (pin * 2);
		ICONFB2(port) &= ~(3 << (pin * 2));
		ICONFB2(port) |= bout << (pin * 2);
	}

}

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


