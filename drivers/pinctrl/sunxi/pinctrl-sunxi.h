/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Allwinner A1X SoCs pinctrl driver.
 *
 * Copyright (C) 2012 Maxime Ripard
 *
 * Maxime Ripard <maxime.ripard@free-electrons.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __PINCTRL_SUNXI_H
#define __PINCTRL_SUNXI_H

#include <pinctrl.h>
#include <gpio.h>

#define PA_BASE	0
#define PB_BASE	32
#define PC_BASE	64
#define PD_BASE	96
#define PE_BASE	128
#define PF_BASE	160
#define PG_BASE	192
#define PH_BASE	224
#define PI_BASE	256
#define PL_BASE	352
#define PM_BASE	384
#define PN_BASE	416

#define SUNXI_PIN_NAME_MAX_LEN	5

#define BANK_MEM_SIZE		0x24
#define MUX_REGS_OFFSET		0x0
#define DATA_REGS_OFFSET	0x10
#define DLEVEL_REGS_OFFSET	0x14
#define PULL_REGS_OFFSET	0x1c

#define PINS_PER_BANK		32
#define MUX_PINS_PER_REG	8
#define MUX_PINS_BITS		4
#define MUX_PINS_MASK		0x0f
#define DATA_PINS_PER_REG	32
#define DATA_PINS_BITS		1
#define DATA_PINS_MASK		0x01
#define DLEVEL_PINS_PER_REG	16
#define DLEVEL_PINS_BITS	2
#define DLEVEL_PINS_MASK	0x03
#define PULL_PINS_PER_REG	16
#define PULL_PINS_BITS		2
#define PULL_PINS_MASK		0x03

#define GRP_CFG_REG		0x300

#define IO_BIAS_MASK		GENMASK(3, 0)

#define SUN4I_FUNC_INPUT	0
#define SUN4I_FUNC_IRQ		6

#define PINCTRL_SUN5I_A10S	BIT(1)
#define PINCTRL_SUN5I_A13	BIT(2)
#define PINCTRL_SUN5I_GR8	BIT(3)
#define PINCTRL_SUN6I_A31	BIT(4)
#define PINCTRL_SUN6I_A31S	BIT(5)
#define PINCTRL_SUN4I_A10	BIT(6)
#define PINCTRL_SUN7I_A20	BIT(7)
#define PINCTRL_SUN8I_R40	BIT(8)
#define PINCTRL_SUN8I_V3	BIT(9)
#define PINCTRL_SUN8I_V3S	BIT(10)

#define PIO_POW_MOD_SEL_REG	0x340

enum sunxi_desc_bias_voltage {
	BIAS_VOLTAGE_NONE,
	/*
	 * Bias voltage configuration is done through
	 * Pn_GRP_CONFIG registers, as seen on A80 SoC.
	 */
	BIAS_VOLTAGE_GRP_CONFIG,
	/*
	 * Bias voltage is set through PIO_POW_MOD_SEL_REG
	 * register, as seen on H6 SoC, for example.
	 */
	BIAS_VOLTAGE_PIO_POW_MODE_SEL,
};

struct sunxi_desc_function {
	const char	*name;
	u8		muxval;
};

struct sunxi_desc_pin {
	struct {
		const char		name[6];
		u16			number;
	} pin;
	const struct sunxi_desc_function	*functions;
};

struct sunxi_pinctrl_desc {
	const struct sunxi_desc_pin	*pins;
	size_t				npins;
	unsigned			pin_base;
	bool				disable_strict_mode;
	enum sunxi_desc_bias_voltage	io_bias_cfg_variant;
};

struct sunxi_pinctrl {
	void __iomem			*base;
	struct gpio_chip		chip;
	struct pinctrl_device		pdev;
	const struct sunxi_pinctrl_desc	*desc;
};

#define SUNXI_PIN(_pin, ...)					\
	{							\
		.pin = _pin,					\
		.functions = (struct sunxi_desc_function[]){	\
			__VA_ARGS__, { } },			\
	}

#define SUNXI_PINCTRL_PIN(bank, pin)				\
	{							\
		.name = "P" #bank #pin,				\
		.number = P ## bank ## _BASE + (pin)		\
	}

#define SUNXI_FUNCTION(_val, _name)				\
	{							\
		.name = _name,					\
		.muxval = _val,					\
	}

#define SUNXI_FUNCTION_IRQ_BANK(...)  {}

/*
 * The sunXi PIO registers are organized as is:
 * 0x00 - 0x0c	Muxing values.
 *		8 pins per register, each pin having a 4bits value
 * 0x10		Pin values
 *		32 bits per register, each pin corresponding to one bit
 * 0x14 - 0x18	Drive level
 *		16 pins per register, each pin having a 2bits value
 * 0x1c - 0x20	Pull-Up values
 *		16 pins per register, each pin having a 2bits value
 *
 * This is for the first bank. Each bank will have the same layout,
 * with an offset being a multiple of 0x24.
 *
 * The following functions calculate from the pin number the register
 * and the bit offset that we should access.
 */
static inline u32 sunxi_mux_reg(u16 pin)
{
	u8 bank = pin / PINS_PER_BANK;
	u32 offset = bank * BANK_MEM_SIZE;
	offset += MUX_REGS_OFFSET;
	offset += pin % PINS_PER_BANK / MUX_PINS_PER_REG * 0x04;
	return round_down(offset, 4);
}

static inline u32 sunxi_mux_offset(u16 pin)
{
	u32 pin_num = pin % MUX_PINS_PER_REG;
	return pin_num * MUX_PINS_BITS;
}

static inline u32 sunxi_data_reg(u16 pin)
{
	u8 bank = pin / PINS_PER_BANK;
	u32 offset = bank * BANK_MEM_SIZE;
	offset += DATA_REGS_OFFSET;
	offset += pin % PINS_PER_BANK / DATA_PINS_PER_REG * 0x04;
	return round_down(offset, 4);
}

static inline u32 sunxi_data_offset(u16 pin)
{
	u32 pin_num = pin % DATA_PINS_PER_REG;
	return pin_num * DATA_PINS_BITS;
}

static inline u32 sunxi_dlevel_reg(u16 pin)
{
	u8 bank = pin / PINS_PER_BANK;
	u32 offset = bank * BANK_MEM_SIZE;
	offset += DLEVEL_REGS_OFFSET;
	offset += pin % PINS_PER_BANK / DLEVEL_PINS_PER_REG * 0x04;
	return round_down(offset, 4);
}

static inline u32 sunxi_dlevel_offset(u16 pin)
{
	u32 pin_num = pin % DLEVEL_PINS_PER_REG;
	return pin_num * DLEVEL_PINS_BITS;
}

static inline u32 sunxi_pull_reg(u16 pin)
{
	u8 bank = pin / PINS_PER_BANK;
	u32 offset = bank * BANK_MEM_SIZE;
	offset += PULL_REGS_OFFSET;
	offset += pin % PINS_PER_BANK / PULL_PINS_PER_REG * 0x04;
	return round_down(offset, 4);
}

static inline u32 sunxi_pull_offset(u16 pin)
{
	u32 pin_num = pin % PULL_PINS_PER_REG;
	return pin_num * PULL_PINS_BITS;
}

static inline u32 sunxi_grp_config_reg(u16 pin)
{
	u8 bank = pin / PINS_PER_BANK;

	return GRP_CFG_REG + bank * 0x4;
}

int sunxi_pinctrl_probe(struct device *dev);

#endif /* __PINCTRL_SUNXI_H */
