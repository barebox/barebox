// SPDX-License-Identifier: GPL-2.0-only
/*
 * Rockchip pinctrl and gpio driver for Barebox
 *
 * Copyright (C) 2014 Beniamino Galvani <b.galvani@gmail.com>
 *
 * Based on Linux pinctrl-rockchip:
 *   Copyright (C) 2013 MundoReader S.L.
 *   Copyright (C) 2012 Samsung Electronics Co., Ltd.
 *   Copyright (C) 2012 Linaro Ltd
 *   Copyright (C) 2011-2012 Jean-Christophe PLAGNIOL-VILLARD
 */

#include <common.h>
#include <gpio.h>
#include <init.h>
#include <malloc.h>
#include <mfd/syscon.h>
#include <of.h>
#include <of_address.h>
#include <pinctrl.h>
#include <dt-bindings/pinctrl/rockchip.h>

#include <linux/basic_mmio_gpio.h>
#include <linux/clk.h>
#include <linux/err.h>

enum rockchip_pinctrl_type {
	RK2928,
	RK3066B,
	RK3188,
	RK3568,
};

enum rockchip_pin_bank_type {
	COMMON_BANK,
	RK3188_BANK0,
};

/**
 * Generate a bitmask for setting a value (v) with a write mask bit in hiword
 * register 31:16 area.
 */
#define WRITE_MASK_VAL(h, l, v) \
	(GENMASK(((h) + 16), ((l) + 16)) | (((v) << (l)) & GENMASK((h), (l))))

/*
 * Encode variants of iomux registers into a type variable
 */
#define IOMUX_GPIO_ONLY		BIT(0)
#define IOMUX_WIDTH_4BIT	BIT(1)
#define IOMUX_SOURCE_PMU	BIT(2)
#define IOMUX_UNROUTED		BIT(3)
#define IOMUX_WIDTH_3BIT	BIT(4)
#define IOMUX_WIDTH_2BIT	BIT(5)

/**
 * struct rockchip_iomux
 * @type: iomux variant using IOMUX_* constants
 * @offset: if initialized to -1 it will be autocalculated, by specifying
 *	    an initial offset value the relevant source offset can be reset
 *	    to a new value for autocalculating the following iomux registers.
 */
struct rockchip_iomux {
	int				type;
	int				offset;
};

/*
 * enum type index corresponding to rockchip_perpin_drv_list arrays index.
 */
enum rockchip_pin_drv_type {
	DRV_TYPE_IO_DEFAULT = 0,
	DRV_TYPE_IO_1V8_OR_3V0,
	DRV_TYPE_IO_1V8_ONLY,
	DRV_TYPE_IO_1V8_3V0_AUTO,
	DRV_TYPE_IO_3V3_ONLY,
	DRV_TYPE_MAX
};

/*
 * enum type index corresponding to rockchip_pull_list arrays index.
 */
enum rockchip_pin_pull_type {
	PULL_TYPE_IO_DEFAULT = 0,
	PULL_TYPE_IO_1V8_ONLY,
	PULL_TYPE_MAX
};

/**
 * struct rockchip_drv
 * @drv_type: drive strength variant using rockchip_perpin_drv_type
 * @offset: if initialized to -1 it will be autocalculated, by specifying
 *	    an initial offset value the relevant source offset can be reset
 *	    to a new value for autocalculating the following drive strength
 *	    registers. if used chips own cal_drv func instead to calculate
 *	    registers offset, the variant could be ignored.
 */
struct rockchip_drv {
	enum rockchip_pin_drv_type	drv_type;
	int				offset;
};

struct rockchip_pin_bank {
	void __iomem			*reg_base;
	struct clk			*clk;
	u32				pin_base;
	u8				nr_pins;
	char				*name;
	u8				bank_num;
	struct rockchip_iomux		iomux[4];
	struct rockchip_drv		drv[4];
	enum rockchip_pin_bank_type	bank_type;
	bool				valid;
	struct device_node		*of_node;
	struct rockchip_pinctrl		*drvdata;
	struct bgpio_chip		bgpio_chip;
	u32				route_mask;
};

#define PIN_BANK(id, pins, label)			\
	{						\
		.bank_num	= id,			\
		.nr_pins	= pins,			\
		.name		= label,		\
	}

#define PIN_BANK_IOMUX_FLAGS(id, pins, label, iom0, iom1, iom2, iom3)	\
	{								\
		.bank_num	= id,					\
		.nr_pins	= pins,					\
		.name		= label,				\
		.iomux		= {					\
			{ .type = iom0, .offset = -1 },			\
			{ .type = iom1, .offset = -1 },			\
			{ .type = iom2, .offset = -1 },			\
			{ .type = iom3, .offset = -1 },			\
		},							\
	}

#define PIN_BANK_MUX_ROUTE_FLAGS(ID, PIN, FUNC, REG, VAL, FLAG)		\
	{								\
		.bank_num	= ID,					\
		.pin		= PIN,					\
		.func		= FUNC,					\
		.route_offset	= REG,					\
		.route_val	= VAL,					\
		.route_location	= FLAG,					\
	}

#define RK_MUXROUTE_SAME(ID, PIN, FUNC, REG, VAL)	\
	PIN_BANK_MUX_ROUTE_FLAGS(ID, PIN, FUNC, REG, VAL, ROCKCHIP_ROUTE_SAME)

#define RK_MUXROUTE_GRF(ID, PIN, FUNC, REG, VAL)	\
	PIN_BANK_MUX_ROUTE_FLAGS(ID, PIN, FUNC, REG, VAL, ROCKCHIP_ROUTE_GRF)

#define RK_MUXROUTE_PMU(ID, PIN, FUNC, REG, VAL)	\
	PIN_BANK_MUX_ROUTE_FLAGS(ID, PIN, FUNC, REG, VAL, ROCKCHIP_ROUTE_PMU)

enum rockchip_mux_route_location {
	ROCKCHIP_ROUTE_SAME = 0,
	ROCKCHIP_ROUTE_PMU,
	ROCKCHIP_ROUTE_GRF,
};

/**
 * struct rockchip_mux_recalced_data: represent a pin iomux data.
 * @bank_num: bank number.
 * @pin: index at register or used to calc index.
 * @func: the min pin.
 * @route_location: the mux route location (same, pmu, grf).
 * @route_offset: the max pin.
 * @route_val: the register offset.
 */
struct rockchip_mux_route_data {
	u8 bank_num;
	u8 pin;
	u8 func;
	enum rockchip_mux_route_location route_location;
	u32 route_offset;
	u32 route_val;
};

struct rockchip_pin_ctrl {
	struct rockchip_pin_bank	*pin_banks;
	u32				nr_banks;
	u32				nr_pins;
	char				*label;
	enum rockchip_pinctrl_type	type;
	int				grf_mux_offset;
	int				pmu_mux_offset;
	int				grf_drv_offset;
	int				pmu_drv_offset;
	struct rockchip_mux_route_data *iomux_routes;
	u32				niomux_routes;
	void	(*pull_calc_reg)(struct rockchip_pin_bank *bank, int pin_num,
				 void __iomem **reg, u8 *bit);
	void	(*drv_calc_reg)(struct rockchip_pin_bank *bank,
				    int pin_num, void __iomem **reg, u8 *bit);
};

struct rockchip_pinctrl {
	void __iomem			*reg_base;
	void __iomem			*reg_pmu;
	struct pinctrl_device		pctl_dev;
	struct rockchip_pin_ctrl	*ctrl;
};

enum {
	RK_BIAS_DISABLE = 0,
	RK_BIAS_PULL_UP,
	RK_BIAS_PULL_DOWN,
	RK_BIAS_BUS_HOLD,
};

/* GPIO registers */
enum {
	RK_GPIO_SWPORT_DR	= 0x00,
	RK_GPIO_SWPORT_DDR	= 0x04,
	RK_GPIO_EXT_PORT	= 0x50,
};

/* GPIO registers */
enum {
	RK_GPIOV2_DR_L	= 0x00,
	RK_GPIOV2_DR_H	= 0x04,
	RK_GPIOV2_DDR_L	= 0x08,
	RK_GPIOV2_DDR_H	= 0x0c,
};

static struct rockchip_pin_bank *gc_to_rockchip_pinctrl(struct gpio_chip *gc)
{
	struct bgpio_chip *bgc = to_bgpio_chip(gc);

	return container_of(bgc, struct rockchip_pin_bank, bgpio_chip);
}

static int rockchip_gpiov2_direction_input(struct gpio_chip *gc, unsigned int gpio)
{
	struct rockchip_pin_bank *bank = gc_to_rockchip_pinctrl(gc);
	u32 mask;

	mask = 1 << (16 + (gpio % 16));

	if (gpio < 16)
		writel(mask, bank->reg_base + RK_GPIOV2_DDR_L);
	else
		writel(mask, bank->reg_base + RK_GPIOV2_DDR_H);

	return 0;
}

static int rockchip_gpiov2_get_direction(struct gpio_chip *gc, unsigned int gpio)
{
	struct rockchip_pin_bank *bank = gc_to_rockchip_pinctrl(gc);
	u32 r;

	if (gpio < 16)
		r = readl(bank->reg_base + RK_GPIOV2_DDR_L);
	else
		r = readl(bank->reg_base + RK_GPIOV2_DDR_H);

	return r & BIT(gpio % 16) ? GPIOF_DIR_OUT : GPIOF_DIR_IN;
}

static void rockchip_gpiov2_set_value(struct gpio_chip *gc, unsigned int gpio,
				      int val)
{
	struct rockchip_pin_bank *bank = gc_to_rockchip_pinctrl(gc);
	u32 mask, vval = 0;

	mask = 1 << (16 + (gpio % 16));
	if (val)
		vval = 1 << (gpio % 16);

	if (gpio < 16)
		writel(mask | vval, bank->reg_base + RK_GPIOV2_DR_L);
	else
		writel(mask | vval, bank->reg_base + RK_GPIOV2_DR_H);
}

static int rockchip_gpiov2_direction_output(struct gpio_chip *gc,
					    unsigned int gpio, int val)
{
	struct rockchip_pin_bank *bank = gc_to_rockchip_pinctrl(gc);
	u32 mask, out, vval = 0;

	mask = 1 << (16 + (gpio % 16));
	out = 1 << (gpio % 16);
	if (val)
		vval = 1 << (gpio % 16);

	if (gpio < 16) {
		writel(mask | vval, bank->reg_base + RK_GPIOV2_DR_L);
		writel(mask | out, bank->reg_base + RK_GPIOV2_DDR_L);
	} else {
		writel(mask | vval, bank->reg_base + RK_GPIOV2_DR_H);
		writel(mask | out, bank->reg_base + RK_GPIOV2_DDR_H);
	}

	return 0;
}

static int rockchip_gpiov2_get_value(struct gpio_chip *gc, unsigned int gpio)
{
	struct rockchip_pin_bank *bank = gc_to_rockchip_pinctrl(gc);
	u32 mask, r;

	mask = 1 << (gpio % 16);

	if (gpio < 16)
		r = readl(bank->reg_base + RK_GPIOV2_DR_L);
	else
		r = readl(bank->reg_base + RK_GPIOV2_DR_L);

	return r & mask ? 1 : 0;
}

static struct gpio_ops rockchip_gpio_ops = {
	.direction_input = rockchip_gpiov2_direction_input,
	.direction_output = rockchip_gpiov2_direction_output,
	.get = rockchip_gpiov2_get_value,
	.set = rockchip_gpiov2_set_value,
	.get_direction = rockchip_gpiov2_get_direction,
};

static int rockchip_gpio_probe(struct device_d *dev)
{
	struct rockchip_pinctrl *info = dev->parent->priv;
	struct rockchip_pin_ctrl *ctrl = info->ctrl;
	struct rockchip_pin_bank *bank;
	struct gpio_chip *gpio;
	void __iomem *reg_base;
	int ret, bankno;

	bankno = of_alias_get_id(dev->device_node, "gpio");
	if (bankno >= ctrl->nr_banks)
		bankno = -EINVAL;
	if (bankno < 0)
		return bankno;

	bank = &ctrl->pin_banks[bankno];
	gpio = &bank->bgpio_chip.gc;

	if (!bank->valid)
		dev_warn(dev, "bank %s is not valid\n", bank->name);

	reg_base = bank->reg_base;

	if (ctrl->type == RK3568) {
		gpio->ngpio = 32;
		gpio->dev = dev;
		gpio->ops = &rockchip_gpio_ops;
		gpio->base = bankno;
		if (gpio->base < 0)
			return -EINVAL;
		gpio->base *= 32;
	} else {
		ret = bgpio_init(&bank->bgpio_chip, dev, 4,
				 reg_base + RK_GPIO_EXT_PORT,
				 reg_base + RK_GPIO_SWPORT_DR, NULL,
				 reg_base + RK_GPIO_SWPORT_DDR, NULL, 0);
		if (ret)
			return ret;
	}

	bank->bgpio_chip.gc.dev = dev;

	bank->bgpio_chip.gc.ngpio = bank->nr_pins;
	ret = gpiochip_add(&bank->bgpio_chip.gc);
	if (ret) {
		dev_err(dev, "failed to register gpio_chip %s, error code: %d\n",
			bank->name, ret);
		return ret;
	}

	return 0;
}

static struct rockchip_pinctrl *to_rockchip_pinctrl(struct pinctrl_device *pdev)
{
	return container_of(pdev, struct rockchip_pinctrl, pctl_dev);
}

static struct rockchip_pin_bank *bank_num_to_bank(struct rockchip_pinctrl *info,
						  unsigned num)
{
	struct rockchip_pin_bank *b = info->ctrl->pin_banks;
	int i;

	for (i = 0; i < info->ctrl->nr_banks; i++, b++) {
		if (b->bank_num == num)
			return b;
	}

	return ERR_PTR(-EINVAL);
}

static int parse_bias_config(struct device_node *np)
{
	u32 val;

	if (of_property_read_u32(np, "bias-pull-up", &val) != -EINVAL)
		return RK_BIAS_PULL_UP;
	else if (of_property_read_u32(np, "bias-pull-down", &val) != -EINVAL)
		return RK_BIAS_PULL_DOWN;
	else if (of_property_read_u32(np, "bias-bus-hold", &val) != -EINVAL)
		return RK_BIAS_BUS_HOLD;
	else
		return RK_BIAS_DISABLE;
}


#define RK2928_PULL_OFFSET		0x118
#define RK2928_PULL_PINS_PER_REG	16
#define RK2928_PULL_BANK_STRIDE		8

static void rk2928_calc_pull_reg_and_bit(struct rockchip_pin_bank *bank,
					 int pin_num, void __iomem **reg,
					 u8 *bit)
{
	struct rockchip_pinctrl *info = bank->drvdata;

	*reg = info->reg_base + RK2928_PULL_OFFSET;
	*reg += bank->bank_num * RK2928_PULL_BANK_STRIDE;
	*reg += (pin_num / RK2928_PULL_PINS_PER_REG) * 4;

	*bit = pin_num % RK2928_PULL_PINS_PER_REG;
};

#define RK3188_PULL_OFFSET		0x164
#define RK3188_PULL_BITS_PER_PIN	2
#define RK3188_PULL_PINS_PER_REG	8
#define RK3188_PULL_BANK_STRIDE		16
#define RK3188_PULL_PMU_OFFSET		0x64

static void rk3188_calc_pull_reg_and_bit(struct rockchip_pin_bank *bank,
					 int pin_num, void __iomem **reg,
					 u8 *bit)
{
	struct rockchip_pinctrl *info = bank->drvdata;

	/* The first 12 pins of the first bank are located elsewhere */
	if (bank->bank_type == RK3188_BANK0 && pin_num < 12) {
		*reg = info->reg_pmu + RK3188_PULL_PMU_OFFSET +
				((pin_num / RK3188_PULL_PINS_PER_REG) * 4);
		*bit = pin_num % RK3188_PULL_PINS_PER_REG;
		*bit *= RK3188_PULL_BITS_PER_PIN;
	} else {
		*reg = info->reg_base + RK3188_PULL_OFFSET - 4;
		*reg += bank->bank_num * RK3188_PULL_BANK_STRIDE;
		*reg += ((pin_num / RK3188_PULL_PINS_PER_REG) * 4);

		/*
		 * The bits in these registers have an inverse ordering
		 * with the lowest pin being in bits 15:14 and the highest
		 * pin in bits 1:0
		 */
		*bit = 7 - (pin_num % RK3188_PULL_PINS_PER_REG);
		*bit *= RK3188_PULL_BITS_PER_PIN;
	}
}

#define RK3568_PULL_PMU_OFFSET		0x20
#define RK3568_PULL_GRF_OFFSET		0x80
#define RK3568_PULL_BITS_PER_PIN	2
#define RK3568_PULL_PINS_PER_REG	8
#define RK3568_PULL_BANK_STRIDE		0x10

static void rk3568_calc_pull_reg_and_bit(struct rockchip_pin_bank *bank,
					 int pin_num, void __iomem **reg,
					 u8 *bit)
{
	struct rockchip_pinctrl *info = bank->drvdata;

	if (bank->bank_num == 0) {
		*reg = info->reg_pmu + RK3568_PULL_PMU_OFFSET;
		*reg += bank->bank_num * RK3568_PULL_BANK_STRIDE;
		*reg += ((pin_num / RK3568_PULL_PINS_PER_REG) * 4);

		*bit = pin_num % RK3568_PULL_PINS_PER_REG;
		*bit *= RK3568_PULL_BITS_PER_PIN;
	} else {
		*reg = info->reg_base + RK3568_PULL_GRF_OFFSET;
		*reg += (bank->bank_num - 1) * RK3568_PULL_BANK_STRIDE;
		*reg += ((pin_num / RK3568_PULL_PINS_PER_REG) * 4);

		*bit = (pin_num % RK3568_PULL_PINS_PER_REG);
		*bit *= RK3568_PULL_BITS_PER_PIN;
	}
}

#define RK3568_DRV_PMU_OFFSET		0x70
#define RK3568_DRV_GRF_OFFSET		0x200
#define RK3568_DRV_BITS_PER_PIN		8
#define RK3568_DRV_PINS_PER_REG		2
#define RK3568_DRV_BANK_STRIDE		0x40

static void rk3568_calc_drv_reg_and_bit(struct rockchip_pin_bank *bank,
					int pin_num, void __iomem **reg, u8 *bit)
{
	struct rockchip_pinctrl *info = bank->drvdata;

	/* The first 32 pins of the first bank are located in PMU */
	if (bank->bank_num == 0) {
		*reg = info->reg_pmu + RK3568_DRV_PMU_OFFSET;
		*reg += ((pin_num / RK3568_DRV_PINS_PER_REG) * 4);

		*bit = pin_num % RK3568_DRV_PINS_PER_REG;
		*bit *= RK3568_DRV_BITS_PER_PIN;
	} else {
		*reg = info->reg_base + RK3568_DRV_GRF_OFFSET;
		*reg += (bank->bank_num - 1) * RK3568_DRV_BANK_STRIDE;
		*reg += ((pin_num / RK3568_DRV_PINS_PER_REG) * 4);

		*bit = (pin_num % RK3568_DRV_PINS_PER_REG);
		*bit *= RK3568_DRV_BITS_PER_PIN;
	}
}

static struct rockchip_mux_route_data rk3188_mux_route_data[] = {
	RK_MUXROUTE_SAME(0, RK_PD0, 1, 0xa0, BIT(16 + 11)), /* non-iomuxed emmc/flash pins on flash-dqs */
	RK_MUXROUTE_SAME(0, RK_PD0, 2, 0xa0, BIT(16 + 11) | BIT(11)), /* non-iomuxed emmc/flash pins on emmc-clk */
};

static bool rockchip_get_mux_route(struct rockchip_pin_bank *bank, int pin,
				   int mux, u32 *loc, u32 *reg, u32 *value)
{
	struct rockchip_pinctrl *info = bank->drvdata;
	struct rockchip_pin_ctrl *ctrl = info->ctrl;
	struct rockchip_mux_route_data *data;
	int i;

	for (i = 0; i < ctrl->niomux_routes; i++) {
		data = &ctrl->iomux_routes[i];
		if ((data->bank_num == bank->bank_num) &&
		    (data->pin == pin) && (data->func == mux))
			break;
	}

	if (i >= ctrl->niomux_routes)
		return false;

	*loc = data->route_location;
	*reg = data->route_offset;
	*value = data->route_val;

	return true;
}

static int rockchip_pinctrl_set_func(struct rockchip_pin_bank *bank, int pin,
				     int mux)
{
	struct rockchip_pinctrl *info = bank->drvdata;
	void __iomem *base, *reg;
	u8 bit;
	u32 data, route_location, route_reg, route_val;
	int iomux_num = (pin / 8);
	int mux_type;
	int mask;

	base = (bank->iomux[iomux_num].type & IOMUX_SOURCE_PMU)
				? info->reg_pmu : info->reg_base;

	/* get basic quadrupel of mux registers and the correct reg inside */
	mux_type = bank->iomux[iomux_num].type;
	reg = base + bank->iomux[iomux_num].offset;

	dev_dbg(info->pctl_dev.dev, "setting func of GPIO%d-%d to %d\n",
		 bank->bank_num, pin, mux);

	if (mux_type & IOMUX_WIDTH_4BIT) {
		if ((pin % 8) >= 4)
			reg += 0x4;
		bit = (pin % 4) * 4;
		mask = 0xf;
	} else if (mux_type & IOMUX_WIDTH_3BIT) {
		if ((pin % 8) >= 5)
			reg += 0x4;
		bit = (pin % 8 % 5) * 3;
		mask = 0x7;
	} else {
		bit = (pin % 8) * 2;
		mask = 0x3;
	}

	if (bank->route_mask & BIT(pin)) {
		if (rockchip_get_mux_route(bank, pin, mux, &route_location,
					   &route_reg, &route_val)) {
			void __iomem  *route = base;

			/* handle special locations */
			switch (route_location) {
			case ROCKCHIP_ROUTE_PMU:
				route = info->reg_pmu;
				break;
			case ROCKCHIP_ROUTE_GRF:
				route = info->reg_base;
				break;
			}

			writel(route_val, route + route_reg);
		}
	}

	data = 3 << (bit + 16);
	data |= (mux & 3) << bit;
	writel(data, reg);

	return 0;
}

static int rockchip_pinctrl_set_pull(struct rockchip_pin_bank *bank,
				     int pin_num, int pull)
{
	struct rockchip_pinctrl *info = bank->drvdata;
	struct rockchip_pin_ctrl *ctrl = info->ctrl;
	void __iomem *reg;
	u8 bit;
	u32 data;

	dev_dbg(info->pctl_dev.dev, "setting pull of GPIO%d-%d to %d\n",
		 bank->bank_num, pin_num, pull);

	/* rk3066b doesn't support any pulls */
	if (ctrl->type == RK3066B)
		return pull ? -EINVAL : 0;

	ctrl->pull_calc_reg(bank, pin_num, &reg, &bit);

	switch (ctrl->type) {
	case RK2928:
		data = BIT(bit + 16);
		if (pull == RK_BIAS_DISABLE)
			data |= BIT(bit);
		writel(data, reg);
		break;
	case RK3188:
	case RK3568:
		/*
		 * In the TRM, pull-up being 1 for everything except the GPIO0_D0-D6,
		 * where that pull up value becomes 3.
		 */
		if (ctrl->type == RK3568 &&
		    bank->bank_num == 0 &&
		    pin_num >= 27 &&
		    pin_num <= 30 &&
		    pull == RK_BIAS_PULL_UP)
			pull = 3;

		data = ((1 << RK3188_PULL_BITS_PER_PIN) - 1) << (bit + 16);
		data |= pull << bit;
		writel(data, reg);
		break;
	default:
		dev_err(info->pctl_dev.dev, "unsupported pinctrl type\n");
		return -EINVAL;
	}

	return 0;
}

static int rockchip_perpin_drv_list[DRV_TYPE_MAX][8] = {
	{ 2, 4, 8, 12, -1, -1, -1, -1 },
	{ 3, 6, 9, 12, -1, -1, -1, -1 },
	{ 5, 10, 15, 20, -1, -1, -1, -1 },
	{ 4, 6, 8, 10, 12, 14, 16, 18 },
	{ 4, 7, 10, 13, 16, 19, 22, 26 }
};

#define RK3288_DRV_BITS_PER_PIN		2
#define RK3399_DRV_3BITS_PER_PIN	3

static int rockchip_set_drive_perpin(struct rockchip_pin_bank *bank,
				     int pin_num, int strength)
{
	struct rockchip_pinctrl *info = bank->drvdata;
	struct rockchip_pin_ctrl *ctrl = info->ctrl;
	int ret, i;
	void __iomem *reg;
	u32 data, rmask, rmask_bits, temp, val;
	u8 bit;
	int drv_type = bank->drv[pin_num / 8].drv_type;

	if (!ctrl->drv_calc_reg)
		return -ENOTSUPP;

	dev_dbg(info->pctl_dev.dev, "setting drive of GPIO%d-%d to %d\n",
		bank->bank_num, pin_num, strength);

	ctrl->drv_calc_reg(bank, pin_num, &reg, &bit);
	if (ctrl->type == RK3568) {
		rmask_bits = RK3568_DRV_BITS_PER_PIN;
		ret = (1 << (strength + 1)) - 1;
		goto config;
	}

	ret = -EINVAL;
	for (i = 0; i < ARRAY_SIZE(rockchip_perpin_drv_list[drv_type]); i++) {
		if (rockchip_perpin_drv_list[drv_type][i] == strength) {
			ret = i;
			break;
		} else if (rockchip_perpin_drv_list[drv_type][i] < 0) {
			ret = rockchip_perpin_drv_list[drv_type][i];
			break;
		}
	}

	if (ret < 0) {
		dev_err(info->pctl_dev.dev, "unsupported driver strength %d\n",
			strength);
		return ret;
	}

	switch (drv_type) {
	case DRV_TYPE_IO_1V8_3V0_AUTO:
	case DRV_TYPE_IO_3V3_ONLY:
		rmask_bits = RK3399_DRV_3BITS_PER_PIN;
		switch (bit) {
		case 0 ... 12:
			/* regular case, nothing to do */
			break;
		case 15:
			/*
			 * drive-strength offset is special, as it is spread
			 * over 2 registers, the bit data[15] contains bit 0
			 * of the value while temp[1:0] contains bits 2 and 1
			 */
			data = (ret & 0x1) << 15;
			temp = (ret >> 0x1) & 0x3;

			rmask = BIT(15) | BIT(31);
			data |= BIT(31);

			val = readl(reg);
			val &= ~rmask;
			val |= data & rmask;
			writel(val, reg);

			rmask = 0x3 | (0x3 << 16);
			temp |= (0x3 << 16);
			reg += 0x4;

			val = readl(reg);
			val &= ~rmask;
			val |= temp & rmask;
			writel(val, reg);

			return ret;
		case 18 ... 21:
			/* setting fully enclosed in the second register */
			reg += 4;
			bit -= 16;
			break;
		default:
			dev_err(info->pctl_dev.dev, "unsupported bit: %d for pinctrl drive type: %d\n",
				bit, drv_type);
			return -EINVAL;
		}
		break;
	case DRV_TYPE_IO_DEFAULT:
	case DRV_TYPE_IO_1V8_OR_3V0:
	case DRV_TYPE_IO_1V8_ONLY:
		rmask_bits = RK3288_DRV_BITS_PER_PIN;
		break;
	default:
		dev_err(info->pctl_dev.dev, "unsupported pinctrl drive type: %d\n",
			drv_type);
		return -EINVAL;
	}

config:
	/* enable the write to the equivalent lower bits */
	data = ((1 << rmask_bits) - 1) << (bit + 16);
	rmask = data | (data >> 16);
	data |= (ret << bit);

	val = readl(reg);
	val &= ~rmask;
	val |= data & rmask;
	writel(val, reg);

	return ret;
}

static int rockchip_pinctrl_set_state(struct pinctrl_device *pdev,
				      struct device_node *np)
{
	struct rockchip_pinctrl *info = to_rockchip_pinctrl(pdev);
	const __be32 *list;
	int i, size, ret;
	int bank_num, pin_num, func;

	/*
	 * the binding format is rockchip,pins = <bank pin mux CONFIG>,
	 * do sanity check and calculate pins number
	 */
	list = of_get_property(np, "rockchip,pins", &size);
	size /= sizeof(*list);

	if (!size || size % 4) {
		dev_err(pdev->dev, "wrong pins number or pins and configs should be by 4\n");
		return -EINVAL;
	}

	for (i = 0; i < size; i += 4) {
		const __be32 *phandle;
		struct device_node *np_config;
		struct rockchip_pin_bank *bank;
		u32 drive_strength;

		bank_num = be32_to_cpu(*list++);
		pin_num = be32_to_cpu(*list++);
		func = be32_to_cpu(*list++);
		phandle = list++;

		if (!phandle)
			return -EINVAL;

		np_config = of_find_node_by_phandle(be32_to_cpup(phandle));
		bank = bank_num_to_bank(info, bank_num);
		rockchip_pinctrl_set_func(bank, pin_num, func);
		rockchip_pinctrl_set_pull(bank, pin_num,
					  parse_bias_config(np_config));

		ret = of_property_read_u32(np_config, "drive-strength", &drive_strength);
		if (!ret)
			rockchip_set_drive_perpin(bank, pin_num, drive_strength);
	}

	return 0;
}

static struct pinctrl_ops rockchip_pinctrl_ops = {
	.set_state = rockchip_pinctrl_set_state,
};

static int rockchip_get_bank_data(struct rockchip_pin_bank *bank,
				  struct device_d *dev)
{
	struct resource node_res, *res;

	if (of_address_to_resource(bank->of_node, 0, &node_res)) {
		dev_err(dev, "cannot find IO resource for bank\n");
		return -ENOENT;
	}

	res = request_iomem_region(dev_name(dev), node_res.start, node_res.end);
	if (IS_ERR(res)) {
		dev_err(dev, "cannot request iomem region %pa\n",
			&node_res.start);
		return PTR_ERR(res);
	}

	bank->reg_base = (void __iomem *)res->start;

	if (of_device_is_compatible(bank->of_node,
				    "rockchip,rk3188-gpio-bank0"))
		bank->bank_type = RK3188_BANK0;
	else
		bank->bank_type = COMMON_BANK;

	bank->clk = of_clk_get(bank->of_node, 0);
	if (IS_ERR(bank->clk))
		return PTR_ERR(bank->clk);

	return clk_enable(bank->clk);
}

static struct of_device_id rockchip_pinctrl_dt_match[];

static struct rockchip_pin_ctrl *rockchip_pinctrl_get_soc_data(
	struct rockchip_pinctrl *d, struct device_d *dev)
{
	const struct of_device_id *match;
	struct device_node *node = dev->device_node;
	struct device_node *np;
	struct rockchip_pin_ctrl *ctrl;
	struct rockchip_pin_bank *bank;
	int grf_offs, pmu_offs, drv_grf_offs, drv_pmu_offs, i, j;
	int gpio = 0;

	match = of_match_node(rockchip_pinctrl_dt_match, node);
	ctrl = (struct rockchip_pin_ctrl *)match->data;

	for_each_child_of_node(node, np) {
		int id;

		if (!of_find_property(np, "gpio-controller", NULL))
			continue;

		id = of_alias_get_id(np, "gpio");
		if (id < 0)
			id = gpio++;

		bank = ctrl->pin_banks;
		for (i = 0; i < ctrl->nr_banks; ++i, ++bank) {
			if (bank->bank_num == id) {
				bank->of_node = np;
				if (!rockchip_get_bank_data(bank, dev))
					bank->valid = true;

				break;
			}
		}
	}

	grf_offs = ctrl->grf_mux_offset;
	pmu_offs = ctrl->pmu_mux_offset;
	drv_pmu_offs = ctrl->pmu_drv_offset;
	drv_grf_offs = ctrl->grf_drv_offset;
	bank = ctrl->pin_banks;
	for (i = 0; i < ctrl->nr_banks; ++i, ++bank) {
		int bank_pins = 0;

		bank->drvdata = d;
		bank->pin_base = ctrl->nr_pins;
		ctrl->nr_pins += bank->nr_pins;

		/* calculate iomux and drv offsets */
		for (j = 0; j < 4; j++) {
			struct rockchip_iomux *iom = &bank->iomux[j];
			struct rockchip_drv *drv = &bank->drv[j];
			int inc;

			if (bank_pins >= bank->nr_pins)
				break;

			/* preset iomux offset value, set new start value */
			if (iom->offset >= 0) {
				if (iom->type & IOMUX_SOURCE_PMU)
					pmu_offs = iom->offset;
				else
					grf_offs = iom->offset;
			} else { /* set current iomux offset */
				iom->offset = (iom->type & IOMUX_SOURCE_PMU) ?
							pmu_offs : grf_offs;
			}

			/* preset drv offset value, set new start value */
			if (drv->offset >= 0) {
				if (iom->type & IOMUX_SOURCE_PMU)
					drv_pmu_offs = drv->offset;
				else
					drv_grf_offs = drv->offset;
			} else { /* set current drv offset */
				drv->offset = (iom->type & IOMUX_SOURCE_PMU) ?
						drv_pmu_offs : drv_grf_offs;
			}

			dev_dbg(dev, "bank %d, iomux %d has iom_offset 0x%x drv_offset 0x%x\n",
				i, j, iom->offset, drv->offset);

			/*
			 * Increase offset according to iomux width.
			 * 4bit iomux'es are spread over two registers.
			 */
			inc = (iom->type & (IOMUX_WIDTH_4BIT |
					    IOMUX_WIDTH_3BIT |
					    IOMUX_WIDTH_2BIT)) ? 8 : 4;
			if (iom->type & IOMUX_SOURCE_PMU)
				pmu_offs += inc;
			else
				grf_offs += inc;

			/*
			 * Increase offset according to drv width.
			 * 3bit drive-strenth'es are spread over two registers.
			 */
			if ((drv->drv_type == DRV_TYPE_IO_1V8_3V0_AUTO) ||
			    (drv->drv_type == DRV_TYPE_IO_3V3_ONLY))
				inc = 8;
			else
				inc = 4;

			if (iom->type & IOMUX_SOURCE_PMU)
				drv_pmu_offs += inc;
			else
				drv_grf_offs += inc;

			bank_pins += 8;
		}

		/* calculate the per-bank route_mask */
		for (j = 0; j < ctrl->niomux_routes; j++) {
			int pin = 0;

			if (ctrl->iomux_routes[j].bank_num == bank->bank_num) {
				pin = ctrl->iomux_routes[j].pin;
				bank->route_mask |= BIT(pin);
			}
		}
	}

	return ctrl;
}

static int rockchip_pinctrl_probe(struct device_d *dev)
{
	struct rockchip_pinctrl *info;
	struct rockchip_pin_ctrl *ctrl;
	int ret;

	info = xzalloc(sizeof(struct rockchip_pinctrl));

	ctrl = rockchip_pinctrl_get_soc_data(info, dev);
	if (!ctrl) {
		dev_err(dev, "driver data not available\n");
		return -EINVAL;
	}
	info->ctrl = ctrl;

	info->reg_base = syscon_base_lookup_by_phandle(dev->device_node,
						       "rockchip,grf");
	if (IS_ERR(info->reg_base)) {
		dev_err(dev, "Could not get grf syscon address\n");
		return -ENODEV;
	}

	info->reg_pmu = syscon_base_lookup_by_phandle(dev->device_node,
						      "rockchip,pmu");
	if (IS_ERR(info->reg_pmu)) {
		dev_err(dev, "Could not get pmu syscon address\n");
		return -ENODEV;
	}

	info->pctl_dev.dev = dev;
	info->pctl_dev.ops = &rockchip_pinctrl_ops;

	dev->priv = info;

	of_platform_populate(dev->device_node, NULL, dev);

	if (!IS_ENABLED(CONFIG_PINCTRL))
		return 0;

	ret = pinctrl_register(&info->pctl_dev);
	if (ret)
		return ret;

	return 0;
}

static struct rockchip_pin_bank rk2928_pin_banks[] = {
	PIN_BANK(0, 32, "gpio0"),
	PIN_BANK(1, 32, "gpio1"),
	PIN_BANK(2, 32, "gpio2"),
	PIN_BANK(3, 32, "gpio3"),
};

static struct rockchip_pin_ctrl rk2928_pin_ctrl = {
	.pin_banks	= rk2928_pin_banks,
	.nr_banks	= ARRAY_SIZE(rk2928_pin_banks),
	.type		= RK2928,
	.grf_mux_offset	= 0xa8,
	.pull_calc_reg	= rk2928_calc_pull_reg_and_bit,
};

static struct rockchip_pin_bank rk3066a_pin_banks[] = {
	PIN_BANK(0, 32, "gpio0"),
	PIN_BANK(1, 32, "gpio1"),
	PIN_BANK(2, 32, "gpio2"),
	PIN_BANK(3, 32, "gpio3"),
	PIN_BANK(4, 32, "gpio4"),
	PIN_BANK(6, 16, "gpio6"),
};

static struct rockchip_pin_ctrl rk3066a_pin_ctrl = {
	.pin_banks	= rk3066a_pin_banks,
	.nr_banks	= ARRAY_SIZE(rk3066a_pin_banks),
	.type		= RK2928,
	.grf_mux_offset	= 0xa8,
	.pull_calc_reg	= rk2928_calc_pull_reg_and_bit,
};

static struct rockchip_pin_bank rk3066b_pin_banks[] = {
	PIN_BANK(0, 32, "gpio0"),
	PIN_BANK(1, 32, "gpio1"),
	PIN_BANK(2, 32, "gpio2"),
	PIN_BANK(3, 32, "gpio3"),
};

static struct rockchip_pin_ctrl rk3066b_pin_ctrl = {
	.pin_banks	= rk3066b_pin_banks,
	.nr_banks	= ARRAY_SIZE(rk3066b_pin_banks),
	.type		= RK3066B,
	.grf_mux_offset	= 0x60,
};

static struct rockchip_pin_bank rk3188_pin_banks[] = {
	PIN_BANK_IOMUX_FLAGS(0, 32, "gpio0", IOMUX_GPIO_ONLY, 0, 0, 0),
	PIN_BANK(1, 32, "gpio1"),
	PIN_BANK(2, 32, "gpio2"),
	PIN_BANK(3, 32, "gpio3"),
};

static struct rockchip_pin_ctrl rk3188_pin_ctrl = {
	.pin_banks	= rk3188_pin_banks,
	.nr_banks	= ARRAY_SIZE(rk3188_pin_banks),
	.type		= RK3188,
	.grf_mux_offset	= 0x60,
	.iomux_routes	= rk3188_mux_route_data,
	.niomux_routes	= ARRAY_SIZE(rk3188_mux_route_data),
	.pull_calc_reg	= rk3188_calc_pull_reg_and_bit,
};

static struct rockchip_mux_route_data rk3568_mux_route_data[] = {
	RK_MUXROUTE_PMU(0, RK_PB7, 1, 0x0110, WRITE_MASK_VAL(1, 0, 0)), /* PWM0 IO mux M0 */
	RK_MUXROUTE_PMU(0, RK_PC7, 2, 0x0110, WRITE_MASK_VAL(1, 0, 1)), /* PWM0 IO mux M1 */
	RK_MUXROUTE_PMU(0, RK_PC0, 1, 0x0110, WRITE_MASK_VAL(3, 2, 0)), /* PWM1 IO mux M0 */
	RK_MUXROUTE_PMU(0, RK_PB5, 4, 0x0110, WRITE_MASK_VAL(3, 2, 1)), /* PWM1 IO mux M1 */
	RK_MUXROUTE_PMU(0, RK_PC1, 1, 0x0110, WRITE_MASK_VAL(5, 4, 0)), /* PWM2 IO mux M0 */
	RK_MUXROUTE_PMU(0, RK_PB6, 4, 0x0110, WRITE_MASK_VAL(5, 4, 1)), /* PWM2 IO mux M1 */
	RK_MUXROUTE_PMU(0, RK_PB3, 2, 0x0300, WRITE_MASK_VAL(0, 0, 0)), /* CAN0 IO mux M0 */
	RK_MUXROUTE_GRF(2, RK_PA1, 4, 0x0300, WRITE_MASK_VAL(0, 0, 1)), /* CAN0 IO mux M1 */
	RK_MUXROUTE_GRF(1, RK_PA1, 3, 0x0300, WRITE_MASK_VAL(2, 2, 0)), /* CAN1 IO mux M0 */
	RK_MUXROUTE_GRF(4, RK_PC3, 3, 0x0300, WRITE_MASK_VAL(2, 2, 1)), /* CAN1 IO mux M1 */
	RK_MUXROUTE_GRF(4, RK_PB5, 3, 0x0300, WRITE_MASK_VAL(4, 4, 0)), /* CAN2 IO mux M0 */
	RK_MUXROUTE_GRF(2, RK_PB2, 4, 0x0300, WRITE_MASK_VAL(4, 4, 1)), /* CAN2 IO mux M1 */
	RK_MUXROUTE_GRF(4, RK_PC4, 1, 0x0300, WRITE_MASK_VAL(6, 6, 0)), /* HPDIN IO mux M0 */
	RK_MUXROUTE_PMU(0, RK_PC2, 2, 0x0300, WRITE_MASK_VAL(6, 6, 1)), /* HPDIN IO mux M1 */
	RK_MUXROUTE_GRF(3, RK_PB1, 3, 0x0300, WRITE_MASK_VAL(8, 8, 0)), /* GMAC1 IO mux M0 */
	RK_MUXROUTE_GRF(4, RK_PA7, 3, 0x0300, WRITE_MASK_VAL(8, 8, 1)), /* GMAC1 IO mux M1 */
	RK_MUXROUTE_GRF(4, RK_PD1, 1, 0x0300, WRITE_MASK_VAL(10, 10, 0)), /* HDMITX IO mux M0 */
	RK_MUXROUTE_PMU(0, RK_PC7, 1, 0x0300, WRITE_MASK_VAL(10, 10, 1)), /* HDMITX IO mux M1 */
	RK_MUXROUTE_PMU(0, RK_PB6, 1, 0x0300, WRITE_MASK_VAL(14, 14, 0)), /* I2C2 IO mux M0 */
	RK_MUXROUTE_GRF(4, RK_PB4, 1, 0x0300, WRITE_MASK_VAL(14, 14, 1)), /* I2C2 IO mux M1 */
	RK_MUXROUTE_GRF(1, RK_PA0, 1, 0x0304, WRITE_MASK_VAL(0, 0, 0)), /* I2C3 IO mux M0 */
	RK_MUXROUTE_GRF(3, RK_PB6, 4, 0x0304, WRITE_MASK_VAL(0, 0, 1)), /* I2C3 IO mux M1 */
	RK_MUXROUTE_GRF(4, RK_PB2, 1, 0x0304, WRITE_MASK_VAL(2, 2, 0)), /* I2C4 IO mux M0 */
	RK_MUXROUTE_GRF(2, RK_PB1, 2, 0x0304, WRITE_MASK_VAL(2, 2, 1)), /* I2C4 IO mux M1 */
	RK_MUXROUTE_GRF(3, RK_PB4, 4, 0x0304, WRITE_MASK_VAL(4, 4, 0)), /* I2C5 IO mux M0 */
	RK_MUXROUTE_GRF(4, RK_PD0, 2, 0x0304, WRITE_MASK_VAL(4, 4, 1)), /* I2C5 IO mux M1 */
	RK_MUXROUTE_GRF(3, RK_PB1, 5, 0x0304, WRITE_MASK_VAL(14, 14, 0)), /* PWM8 IO mux M0 */
	RK_MUXROUTE_GRF(1, RK_PD5, 4, 0x0304, WRITE_MASK_VAL(14, 14, 1)), /* PWM8 IO mux M1 */
	RK_MUXROUTE_GRF(3, RK_PB2, 5, 0x0308, WRITE_MASK_VAL(0, 0, 0)), /* PWM9 IO mux M0 */
	RK_MUXROUTE_GRF(1, RK_PD6, 4, 0x0308, WRITE_MASK_VAL(0, 0, 1)), /* PWM9 IO mux M1 */
	RK_MUXROUTE_GRF(3, RK_PB5, 5, 0x0308, WRITE_MASK_VAL(2, 2, 0)), /* PWM10 IO mux M0 */
	RK_MUXROUTE_GRF(2, RK_PA1, 2, 0x0308, WRITE_MASK_VAL(2, 2, 1)), /* PWM10 IO mux M1 */
	RK_MUXROUTE_GRF(3, RK_PB6, 5, 0x0308, WRITE_MASK_VAL(4, 4, 0)), /* PWM11 IO mux M0 */
	RK_MUXROUTE_GRF(4, RK_PC0, 3, 0x0308, WRITE_MASK_VAL(4, 4, 1)), /* PWM11 IO mux M1 */
	RK_MUXROUTE_GRF(3, RK_PB7, 2, 0x0308, WRITE_MASK_VAL(6, 6, 0)), /* PWM12 IO mux M0 */
	RK_MUXROUTE_GRF(4, RK_PC5, 1, 0x0308, WRITE_MASK_VAL(6, 6, 1)), /* PWM12 IO mux M1 */
	RK_MUXROUTE_GRF(3, RK_PC0, 2, 0x0308, WRITE_MASK_VAL(8, 8, 0)), /* PWM13 IO mux M0 */
	RK_MUXROUTE_GRF(4, RK_PC6, 1, 0x0308, WRITE_MASK_VAL(8, 8, 1)), /* PWM13 IO mux M1 */
	RK_MUXROUTE_GRF(3, RK_PC4, 1, 0x0308, WRITE_MASK_VAL(10, 10, 0)), /* PWM14 IO mux M0 */
	RK_MUXROUTE_GRF(4, RK_PC2, 1, 0x0308, WRITE_MASK_VAL(10, 10, 1)), /* PWM14 IO mux M1 */
	RK_MUXROUTE_GRF(3, RK_PC5, 1, 0x0308, WRITE_MASK_VAL(12, 12, 0)), /* PWM15 IO mux M0 */
	RK_MUXROUTE_GRF(4, RK_PC3, 1, 0x0308, WRITE_MASK_VAL(12, 12, 1)), /* PWM15 IO mux M1 */
	RK_MUXROUTE_GRF(3, RK_PD2, 3, 0x0308, WRITE_MASK_VAL(14, 14, 0)), /* SDMMC2 IO mux M0 */
	RK_MUXROUTE_GRF(3, RK_PA5, 5, 0x0308, WRITE_MASK_VAL(14, 14, 1)), /* SDMMC2 IO mux M1 */
	RK_MUXROUTE_PMU(0, RK_PB5, 2, 0x030c, WRITE_MASK_VAL(0, 0, 0)), /* SPI0 IO mux M0 */
	RK_MUXROUTE_GRF(2, RK_PD3, 3, 0x030c, WRITE_MASK_VAL(0, 0, 1)), /* SPI0 IO mux M1 */
	RK_MUXROUTE_GRF(2, RK_PB5, 3, 0x030c, WRITE_MASK_VAL(2, 2, 0)), /* SPI1 IO mux M0 */
	RK_MUXROUTE_GRF(3, RK_PC3, 3, 0x030c, WRITE_MASK_VAL(2, 2, 1)), /* SPI1 IO mux M1 */
	RK_MUXROUTE_GRF(2, RK_PC1, 4, 0x030c, WRITE_MASK_VAL(4, 4, 0)), /* SPI2 IO mux M0 */
	RK_MUXROUTE_GRF(3, RK_PA0, 3, 0x030c, WRITE_MASK_VAL(4, 4, 1)), /* SPI2 IO mux M1 */
	RK_MUXROUTE_GRF(4, RK_PB3, 4, 0x030c, WRITE_MASK_VAL(6, 6, 0)), /* SPI3 IO mux M0 */
	RK_MUXROUTE_GRF(4, RK_PC2, 2, 0x030c, WRITE_MASK_VAL(6, 6, 1)), /* SPI3 IO mux M1 */
	RK_MUXROUTE_GRF(2, RK_PB4, 2, 0x030c, WRITE_MASK_VAL(8, 8, 0)), /* UART1 IO mux M0 */
	RK_MUXROUTE_PMU(0, RK_PD1, 1, 0x030c, WRITE_MASK_VAL(8, 8, 1)), /* UART1 IO mux M1 */
	RK_MUXROUTE_PMU(0, RK_PD1, 1, 0x030c, WRITE_MASK_VAL(10, 10, 0)), /* UART2 IO mux M0 */
	RK_MUXROUTE_GRF(1, RK_PD5, 2, 0x030c, WRITE_MASK_VAL(10, 10, 1)), /* UART2 IO mux M1 */
	RK_MUXROUTE_GRF(1, RK_PA1, 2, 0x030c, WRITE_MASK_VAL(12, 12, 0)), /* UART3 IO mux M0 */
	RK_MUXROUTE_GRF(3, RK_PB7, 4, 0x030c, WRITE_MASK_VAL(12, 12, 1)), /* UART3 IO mux M1 */
	RK_MUXROUTE_GRF(1, RK_PA6, 2, 0x030c, WRITE_MASK_VAL(14, 14, 0)), /* UART4 IO mux M0 */
	RK_MUXROUTE_GRF(3, RK_PB2, 4, 0x030c, WRITE_MASK_VAL(14, 14, 1)), /* UART4 IO mux M1 */
	RK_MUXROUTE_GRF(2, RK_PA2, 3, 0x0310, WRITE_MASK_VAL(0, 0, 0)), /* UART5 IO mux M0 */
	RK_MUXROUTE_GRF(3, RK_PC2, 4, 0x0310, WRITE_MASK_VAL(0, 0, 1)), /* UART5 IO mux M1 */
	RK_MUXROUTE_GRF(2, RK_PA4, 3, 0x0310, WRITE_MASK_VAL(2, 2, 0)), /* UART6 IO mux M0 */
	RK_MUXROUTE_GRF(1, RK_PD5, 3, 0x0310, WRITE_MASK_VAL(2, 2, 1)), /* UART6 IO mux M1 */
	RK_MUXROUTE_GRF(2, RK_PA6, 3, 0x0310, WRITE_MASK_VAL(5, 4, 0)), /* UART7 IO mux M0 */
	RK_MUXROUTE_GRF(3, RK_PC4, 4, 0x0310, WRITE_MASK_VAL(5, 4, 1)), /* UART7 IO mux M1 */
	RK_MUXROUTE_GRF(4, RK_PA2, 4, 0x0310, WRITE_MASK_VAL(5, 4, 2)), /* UART7 IO mux M2 */
	RK_MUXROUTE_GRF(2, RK_PC5, 3, 0x0310, WRITE_MASK_VAL(6, 6, 0)), /* UART8 IO mux M0 */
	RK_MUXROUTE_GRF(2, RK_PD7, 4, 0x0310, WRITE_MASK_VAL(6, 6, 1)), /* UART8 IO mux M1 */
	RK_MUXROUTE_GRF(2, RK_PB0, 3, 0x0310, WRITE_MASK_VAL(9, 8, 0)), /* UART9 IO mux M0 */
	RK_MUXROUTE_GRF(4, RK_PC5, 4, 0x0310, WRITE_MASK_VAL(9, 8, 1)), /* UART9 IO mux M1 */
	RK_MUXROUTE_GRF(4, RK_PA4, 4, 0x0310, WRITE_MASK_VAL(9, 8, 2)), /* UART9 IO mux M2 */
	RK_MUXROUTE_GRF(1, RK_PA2, 1, 0x0310, WRITE_MASK_VAL(11, 10, 0)), /* I2S1 IO mux M0 */
	RK_MUXROUTE_GRF(3, RK_PC6, 4, 0x0310, WRITE_MASK_VAL(11, 10, 1)), /* I2S1 IO mux M1 */
	RK_MUXROUTE_GRF(2, RK_PD0, 5, 0x0310, WRITE_MASK_VAL(11, 10, 2)), /* I2S1 IO mux M2 */
	RK_MUXROUTE_GRF(2, RK_PC1, 1, 0x0310, WRITE_MASK_VAL(12, 12, 0)), /* I2S2 IO mux M0 */
	RK_MUXROUTE_GRF(4, RK_PB6, 5, 0x0310, WRITE_MASK_VAL(12, 12, 1)), /* I2S2 IO mux M1 */
	RK_MUXROUTE_GRF(3, RK_PA2, 4, 0x0310, WRITE_MASK_VAL(14, 14, 0)), /* I2S3 IO mux M0 */
	RK_MUXROUTE_GRF(4, RK_PC2, 5, 0x0310, WRITE_MASK_VAL(14, 14, 1)), /* I2S3 IO mux M1 */
	RK_MUXROUTE_GRF(1, RK_PA4, 3, 0x0314, WRITE_MASK_VAL(1, 0, 0)), /* PDM IO mux M0 */
	RK_MUXROUTE_GRF(1, RK_PA6, 3, 0x0314, WRITE_MASK_VAL(1, 0, 0)), /* PDM IO mux M0 */
	RK_MUXROUTE_GRF(3, RK_PD6, 5, 0x0314, WRITE_MASK_VAL(1, 0, 1)), /* PDM IO mux M1 */
	RK_MUXROUTE_GRF(4, RK_PA0, 4, 0x0314, WRITE_MASK_VAL(1, 0, 1)), /* PDM IO mux M1 */
	RK_MUXROUTE_GRF(3, RK_PC4, 5, 0x0314, WRITE_MASK_VAL(1, 0, 2)), /* PDM IO mux M2 */
	RK_MUXROUTE_PMU(0, RK_PA5, 3, 0x0314, WRITE_MASK_VAL(3, 2, 0)), /* PCIE20 IO mux M0 */
	RK_MUXROUTE_GRF(2, RK_PD0, 4, 0x0314, WRITE_MASK_VAL(3, 2, 1)), /* PCIE20 IO mux M1 */
	RK_MUXROUTE_GRF(1, RK_PB0, 4, 0x0314, WRITE_MASK_VAL(3, 2, 2)), /* PCIE20 IO mux M2 */
	RK_MUXROUTE_PMU(0, RK_PA4, 3, 0x0314, WRITE_MASK_VAL(5, 4, 0)), /* PCIE30X1 IO mux M0 */
	RK_MUXROUTE_GRF(2, RK_PD2, 4, 0x0314, WRITE_MASK_VAL(5, 4, 1)), /* PCIE30X1 IO mux M1 */
	RK_MUXROUTE_GRF(1, RK_PA5, 4, 0x0314, WRITE_MASK_VAL(5, 4, 2)), /* PCIE30X1 IO mux M2 */
	RK_MUXROUTE_PMU(0, RK_PA6, 2, 0x0314, WRITE_MASK_VAL(7, 6, 0)), /* PCIE30X2 IO mux M0 */
	RK_MUXROUTE_GRF(2, RK_PD4, 4, 0x0314, WRITE_MASK_VAL(7, 6, 1)), /* PCIE30X2 IO mux M1 */
	RK_MUXROUTE_GRF(4, RK_PC2, 4, 0x0314, WRITE_MASK_VAL(7, 6, 2)), /* PCIE30X2 IO mux M2 */
};

static struct rockchip_pin_bank rk3568_pin_banks[] = {
	PIN_BANK_IOMUX_FLAGS(0, 32, "gpio0", IOMUX_SOURCE_PMU | IOMUX_WIDTH_4BIT,
					     IOMUX_SOURCE_PMU | IOMUX_WIDTH_4BIT,
					     IOMUX_SOURCE_PMU | IOMUX_WIDTH_4BIT,
					     IOMUX_SOURCE_PMU | IOMUX_WIDTH_4BIT),
	PIN_BANK_IOMUX_FLAGS(1, 32, "gpio1", IOMUX_WIDTH_4BIT,
					     IOMUX_WIDTH_4BIT,
					     IOMUX_WIDTH_4BIT,
					     IOMUX_WIDTH_4BIT),
	PIN_BANK_IOMUX_FLAGS(2, 32, "gpio2", IOMUX_WIDTH_4BIT,
					     IOMUX_WIDTH_4BIT,
					     IOMUX_WIDTH_4BIT,
					     IOMUX_WIDTH_4BIT),
	PIN_BANK_IOMUX_FLAGS(3, 32, "gpio3", IOMUX_WIDTH_4BIT,
					     IOMUX_WIDTH_4BIT,
					     IOMUX_WIDTH_4BIT,
					     IOMUX_WIDTH_4BIT),
	PIN_BANK_IOMUX_FLAGS(4, 32, "gpio4", IOMUX_WIDTH_4BIT,
					     IOMUX_WIDTH_4BIT,
					     IOMUX_WIDTH_4BIT,
					     IOMUX_WIDTH_4BIT),
};

static struct rockchip_pin_ctrl rk3568_pin_ctrl = {
	.pin_banks		= rk3568_pin_banks,
	.nr_banks		= ARRAY_SIZE(rk3568_pin_banks),
	.label			= "RK3568-GPIO",
	.type			= RK3568,
	.grf_mux_offset		= 0x0,
	.pmu_mux_offset		= 0x0,
	.grf_drv_offset		= 0x0200,
	.pmu_drv_offset		= 0x0070,
	.iomux_routes		= rk3568_mux_route_data,
	.niomux_routes		= ARRAY_SIZE(rk3568_mux_route_data),
	.pull_calc_reg		= rk3568_calc_pull_reg_and_bit,
	.drv_calc_reg		= rk3568_calc_drv_reg_and_bit,
};

static struct of_device_id rockchip_pinctrl_dt_match[] = {
	{
		.compatible = "rockchip,rk2928-pinctrl",
		.data = &rk2928_pin_ctrl,
	},
	{
		.compatible = "rockchip,rk3066a-pinctrl",
		.data = &rk3066a_pin_ctrl,
	},
	{
		.compatible = "rockchip,rk3066b-pinctrl",
		.data = &rk3066b_pin_ctrl,
	},
	{
		.compatible = "rockchip,rk3188-pinctrl",
		.data = &rk3188_pin_ctrl,
	},
	{
		.compatible = "rockchip,rk3568-pinctrl",
		.data = &rk3568_pin_ctrl
	},
	{
		/* sentinel */
	}
};

static struct driver_d rockchip_pinctrl_driver = {
	.name = "rockchip-pinctrl",
	.probe = rockchip_pinctrl_probe,
	.of_compatible = DRV_OF_COMPAT(rockchip_pinctrl_dt_match),
};

core_platform_driver(rockchip_pinctrl_driver);

static struct of_device_id rockchip_gpio_dt_match[] = {
	{
		.compatible = "rockchip,gpio-bank",
		.data = &rk2928_pin_ctrl,
	}, {
		/* sentinel */
	}
};

static struct driver_d rockchip_gpio_driver = {
	.name = "rockchip-gpio",
	.probe = rockchip_gpio_probe,
	.of_compatible = DRV_OF_COMPAT(rockchip_gpio_dt_match),
};

core_platform_driver(rockchip_gpio_driver);
