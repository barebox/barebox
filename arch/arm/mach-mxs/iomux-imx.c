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
 */

#include <common.h>
#include <init.h>
#include <gpio.h>
#include <errno.h>
#include <io.h>
#include <mach/iomux.h>
#include <stmp-device.h>
#include <mach/imx-regs.h>

#define HW_PINCTRL_CTRL 0x000
#define HW_PINCTRL_MUXSEL0 0x100

#ifdef CONFIG_ARCH_IMX23
#define HW_PINCTRL_DRIVE0 0x200
#define HW_PINCTRL_PULL0 0x400
#define HW_PINCTRL_DOUT0 0x500
#define HW_PINCTRL_DIN0 0x600
#define HW_PINCTRL_DOE0 0x700

#define MAX_GPIO_NO 95
#endif

#ifdef CONFIG_ARCH_IMX28
#define HW_PINCTRL_DRIVE0 0x300
#define HW_PINCTRL_PULL0 0x600
#define HW_PINCTRL_DOUT0 0x700
#define HW_PINCTRL_DIN0 0x900
#define HW_PINCTRL_DOE0 0xb00

#define MAX_GPIO_NO 159
#endif

static unsigned calc_mux_reg(unsigned no)
{
	/* each register controls 16 pads */
	return ((no >> 4) << 4) + HW_PINCTRL_MUXSEL0;
}

static unsigned calc_strength_reg(unsigned no)
{
	/* each register controls 8 pads */
	return  ((no >> 3) << 4) + HW_PINCTRL_DRIVE0;
}

static unsigned calc_pullup_reg(unsigned no)
{
	/* each register controls 32 pads */
	return  ((no >> 5) << 4) + HW_PINCTRL_PULL0;
}

static unsigned calc_output_enable_reg(unsigned no)
{
	/* each register controls 32 pads */
	return  ((no >> 5) << 4) + HW_PINCTRL_DOE0;
}

static unsigned calc_output_reg(unsigned no)
{
	/* each register controls 32 pads */
	return  ((no >> 5) << 4) + HW_PINCTRL_DOUT0;
}

static unsigned calc_input_reg(unsigned no)
{
	/* each register controls 32 pads */
	return  ((no >> 5) << 4) + HW_PINCTRL_DIN0;
}

/**
 * @param[in] m One pin define per call from iomux-mx23.h/iomux-mx28.h
 */
void imx_gpio_mode(uint32_t m)
{
	uint32_t reg;
	unsigned gpio_pin, reg_offset;

	gpio_pin = GET_GPIO_NO(m);

	/* configure the pad to its function (always) */
	reg_offset = calc_mux_reg(gpio_pin);
	reg = readl(IMX_IOMUXC_BASE + reg_offset) & ~(0x3 << ((gpio_pin % 16) << 1));
	reg |= GET_FUNC(m) << ((gpio_pin % 16) << 1);
	writel(reg, IMX_IOMUXC_BASE + reg_offset);

	/* some pins are disabled when configured for GPIO */
	if ((gpio_pin > MAX_GPIO_NO) && (GET_FUNC(m) == IS_GPIO)) {
		printf("Cannot configure pad %d to GPIO\n", gpio_pin);
		return;
	}

	if (SE_PRESENT(m)) {
		reg_offset = calc_strength_reg(gpio_pin);
		reg = readl(IMX_IOMUXC_BASE + reg_offset) & ~(0x3 << ((gpio_pin % 8) << 2));
		reg |= GET_STRENGTH(m) << ((gpio_pin % 8) << 2);
		writel(reg, IMX_IOMUXC_BASE + reg_offset);
	}

	if (VE_PRESENT(m)) {
		reg_offset = calc_strength_reg(gpio_pin);
		if (GET_VOLTAGE(m) == 1)
			writel(0x1 << (((gpio_pin % 8) << 2) + 2),
				IMX_IOMUXC_BASE + reg_offset + STMP_OFFSET_REG_SET);
		else
			writel(0x1 << (((gpio_pin % 8) << 2) + 2),
				IMX_IOMUXC_BASE + reg_offset + STMP_OFFSET_REG_CLR);
	}

	if (PE_PRESENT(m)) {
		reg_offset = calc_pullup_reg(gpio_pin);
		writel(0x1 << (gpio_pin % 32), IMX_IOMUXC_BASE + reg_offset +
				(GET_PULLUP(m) == 1 ?
				 STMP_OFFSET_REG_SET : STMP_OFFSET_REG_CLR));
	}

	if (BK_PRESENT(m)) {
		reg_offset = calc_pullup_reg(gpio_pin);
		writel(0x1 << (gpio_pin % 32), IMX_IOMUXC_BASE + reg_offset +
				(GET_BITKEEPER(m) == 1 ?
				 STMP_OFFSET_REG_CLR : STMP_OFFSET_REG_SET));
	}

	if (GET_FUNC(m) == IS_GPIO) {
		if (GET_GPIODIR(m) == 1) {
			/* first set the output value */
			reg_offset = calc_output_reg(gpio_pin);
			writel(0x1 << (gpio_pin % 32), IMX_IOMUXC_BASE +
				reg_offset + (GET_GPIOVAL(m) == 1 ?
					STMP_OFFSET_REG_SET : STMP_OFFSET_REG_CLR));
			/* then the direction */
			reg_offset = calc_output_enable_reg(gpio_pin);
			writel(0x1 << (gpio_pin % 32),
				IMX_IOMUXC_BASE + reg_offset + STMP_OFFSET_REG_SET);
		} else {
			/* then the direction */
			reg_offset = calc_output_enable_reg(gpio_pin);
			writel(0x1 << (gpio_pin % 32),
				IMX_IOMUXC_BASE + reg_offset + STMP_OFFSET_REG_CLR);
		}
	}
}

int gpio_direction_input(unsigned gpio)
{
	unsigned reg_offset;

	if (gpio > MAX_GPIO_NO)
		return -EINVAL;

	reg_offset = calc_output_enable_reg(gpio);
	writel(0x1 << (gpio % 32), IMX_IOMUXC_BASE + reg_offset + STMP_OFFSET_REG_CLR);

	return 0;
}

int gpio_direction_output(unsigned gpio, int val)
{
	unsigned reg_offset;

	if (gpio > MAX_GPIO_NO)
		return -EINVAL;

	/* first set the output value... */
	reg_offset = calc_output_reg(gpio);
	writel(0x1 << (gpio % 32), IMX_IOMUXC_BASE +
		reg_offset + (val != 0 ? STMP_OFFSET_REG_SET : STMP_OFFSET_REG_CLR));
	/* ...then the direction */
	reg_offset = calc_output_enable_reg(gpio);
	writel(0x1 << (gpio % 32), IMX_IOMUXC_BASE + reg_offset + STMP_OFFSET_REG_SET);

	return 0;
}

void gpio_set_value(unsigned gpio, int val)
{
	unsigned reg_offset;

	reg_offset = calc_output_reg(gpio);
	writel(0x1 << (gpio % 32), IMX_IOMUXC_BASE +
				reg_offset + (val != 0 ?
					STMP_OFFSET_REG_SET : STMP_OFFSET_REG_CLR));
}

int gpio_get_value(unsigned gpio)
{
	uint32_t reg;
	unsigned reg_offset;

	reg_offset = calc_input_reg(gpio);
	reg = readl(IMX_IOMUXC_BASE + reg_offset);
	if (reg & (0x1 << (gpio % 32)))
		return 1;

	return 0;
}
