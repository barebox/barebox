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
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
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
};

enum rockchip_pin_bank_type {
	COMMON_BANK,
	RK3188_BANK0,
};

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

static int rockchip_gpiolib_register(struct device_d *dev,
				     struct rockchip_pinctrl *info)
{
	struct rockchip_pin_ctrl *ctrl = info->ctrl;
	struct rockchip_pin_bank *bank = ctrl->pin_banks;
	void __iomem *reg_base;
	int ret;
	int i;

	for (i = 0; i < ctrl->nr_banks; ++i, ++bank) {
		if (!bank->valid) {
			dev_warn(dev, "bank %s is not valid\n", bank->name);
			continue;
		}

		reg_base = bank->reg_base;

		ret = bgpio_init(&bank->bgpio_chip, dev, 4,
				 reg_base + RK_GPIO_EXT_PORT,
				 reg_base + RK_GPIO_SWPORT_DR, NULL,
				 reg_base + RK_GPIO_SWPORT_DDR, NULL, 0);
		if (ret)
			goto fail;

		bank->bgpio_chip.gc.ngpio = bank->nr_pins;
		ret = gpiochip_add(&bank->bgpio_chip.gc);
		if (ret) {
			dev_err(dev, "failed to register gpio_chip %s, error code: %d\n",
				bank->name, ret);
			goto fail;
		}

	}

	return 0;
fail:
	for (--i, --bank; i >= 0; --i, --bank) {
		if (!bank->valid)
			continue;

		gpiochip_remove(&bank->bgpio_chip.gc);
	}
	return ret;
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
	char *name;
	int grf_offs, pmu_offs, drv_grf_offs, drv_pmu_offs, i, j;

	match = of_match_node(rockchip_pinctrl_dt_match, node);
	ctrl = (struct rockchip_pin_ctrl *)match->data;

	for_each_child_of_node(node, np) {
		if (!of_find_property(np, "gpio-controller", NULL))
			continue;

		bank = ctrl->pin_banks;
		for (i = 0; i < ctrl->nr_banks; ++i, ++bank) {
			name = bank->name;
			if (!strncmp(name, np->name, strlen(name))) {
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

	ret = rockchip_gpiolib_register(dev, info);
	if (ret)
		return ret;

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
	}, {
		/* sentinel */
	}
};

static struct driver_d rockchip_pinctrl_driver = {
	.name = "rockchip-pinctrl",
	.probe = rockchip_pinctrl_probe,
	.of_compatible = DRV_OF_COMPAT(rockchip_pinctrl_dt_match),
};

console_platform_driver(rockchip_pinctrl_driver);
