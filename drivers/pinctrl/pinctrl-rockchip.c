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

struct rockchip_pin_bank {
	void __iomem			*reg_base;
	struct clk			*clk;
	u32				pin_base;
	u8				nr_pins;
	char				*name;
	u8				bank_num;
	enum rockchip_pin_bank_type	bank_type;
	bool				valid;
	struct device_node		*of_node;
	struct rockchip_pinctrl		*drvdata;
	struct bgpio_chip		bgpio_chip;
};

#define PIN_BANK(id, pins, label)			\
	{						\
		.bank_num	= id,			\
		.nr_pins	= pins,			\
		.name		= label,		\
	}

struct rockchip_pin_ctrl {
	struct rockchip_pin_bank	*pin_banks;
	u32				nr_banks;
	u32				nr_pins;
	char				*label;
	enum rockchip_pinctrl_type	type;
	int				mux_offset;
	void	(*pull_calc_reg)(struct rockchip_pin_bank *bank, int pin_num,
				 void __iomem **reg, u8 *bit);
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

static int rockchip_pinctrl_set_func(struct rockchip_pin_bank *bank, int pin,
				     int mux)
{
	struct rockchip_pinctrl *info = bank->drvdata;
	void __iomem *reg = info->reg_base + info->ctrl->mux_offset;
	u8 bit;
	u32 data;

	/* get basic quadruple of mux registers and the correct reg inside */
	reg += bank->bank_num * 0x10;
	reg += (pin / 8) * 4;
	bit = (pin % 8) * 2;

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

static int rockchip_pinctrl_set_state(struct pinctrl_device *pdev,
				      struct device_node *np)
{
	struct rockchip_pinctrl *info = to_rockchip_pinctrl(pdev);
	const __be32 *list;
	int i, size;
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
		dev_err(dev, "cannot request iomem region %08x\n",
			node_res.start);
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
	int i;

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

	bank = ctrl->pin_banks;
	for (i = 0; i < ctrl->nr_banks; ++i, ++bank) {
		bank->drvdata = d;
		bank->pin_base = ctrl->nr_pins;
		ctrl->nr_pins += bank->nr_pins;
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
	.mux_offset	= 0xa8,
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
	.mux_offset	= 0xa8,
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
	.mux_offset	= 0x60,
};

static struct rockchip_pin_bank rk3188_pin_banks[] = {
	PIN_BANK(0, 32, "gpio0"),
	PIN_BANK(1, 32, "gpio1"),
	PIN_BANK(2, 32, "gpio2"),
	PIN_BANK(3, 32, "gpio3"),
};

static struct rockchip_pin_ctrl rk3188_pin_ctrl = {
	.pin_banks	= rk3188_pin_banks,
	.nr_banks	= ARRAY_SIZE(rk3188_pin_banks),
	.type		= RK3188,
	.mux_offset	= 0x60,
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
