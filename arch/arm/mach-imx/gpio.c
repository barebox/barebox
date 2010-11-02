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
#include <errno.h>
#include <asm/io.h>
#include <mach/imx-regs.h>
#include <mach/gpio.h>

#if defined CONFIG_ARCH_IMX1 || defined CONFIG_ARCH_IMX21 || defined CONFIG_ARCH_IMX27
#define GPIO_DR		0x1c
#define GPIO_GDIR	0x00
#define GPIO_PSR	0x24
#define GPIO_ICR1	0x28
#define GPIO_ICR2	0x2C
#define GPIO_IMR	0x30
#define GPIO_ISR	0x34
#else
#define GPIO_DR		0x00
#define GPIO_GDIR	0x04
#define GPIO_PSR	0x08
#define GPIO_ICR1	0x0C
#define GPIO_ICR2	0x10
#define GPIO_IMR	0x14
#define GPIO_ISR	0x18
#define GPIO_ISR	0x18
#endif

extern void __iomem *imx_gpio_base[];
extern int imx_gpio_count;

static void __iomem *gpio_get_base(unsigned gpio)
{
	if (gpio >= imx_gpio_count)
		return NULL;

	return imx_gpio_base[gpio / 32];
}

void gpio_set_value(unsigned gpio, int value)
{
	void __iomem *base = gpio_get_base(gpio);
	int shift = gpio % 32;
	u32 val;

	if (!base)
		return;

	val = readl(base + GPIO_DR);

	if (value)
		val |= 1 << shift;
	else
		val &= ~(1 << shift);

	writel(val, base + GPIO_DR);
}

int gpio_direction_input(unsigned gpio)
{
	void __iomem *base = gpio_get_base(gpio);
	int shift = gpio % 32;
	u32 val;

	if (!base)
		return -EINVAL;

	val = readl(base + GPIO_GDIR);
	val &= ~(1 << shift);
	writel(val, base + GPIO_GDIR);

	return 0;
}


int gpio_direction_output(unsigned gpio, int value)
{
	void __iomem *base = gpio_get_base(gpio);
	int shift = gpio % 32;
	u32 val;

	if (!base)
		return -EINVAL;

	gpio_set_value(gpio, value);

	val = readl(base + GPIO_GDIR);
	val |= 1 << shift;
	writel(val, base + GPIO_GDIR);

	return 0;
}

int gpio_get_value(unsigned gpio)
{
	void __iomem *base = gpio_get_base(gpio);
	int shift = gpio % 32;
	u32 val;

	if (!base)
		return -EINVAL;

	val = readl(base + GPIO_PSR);

	return val & (1 << shift) ? 1 : 0;
}

