// SPDX-License-Identifier: GPL-2.0
/*
 * sama5d2 pin control and GPIO chip driver
 * Copyright (c) 2019 Ahmad Fatoum, Pengutronix
 */

#include <common.h>
#include <init.h>
#include <io.h>
#include <of.h>
#include <pinctrl.h>
#include <malloc.h>
#include <gpio.h>
#include <mach/gpio.h>
#include <linux/clk.h>

#include <dt-bindings/pinctrl/at91.h>

#define ATMEL_GET_PIN_NO(pinfunc)	((pinfunc) & 0xff)
#define ATMEL_GET_PIN_FUNC(pinfunc)	((pinfunc >> 16) & 0xf)
#define ATMEL_GET_PIN_IOSET(pinfunc)	((pinfunc >> 20) & 0xf)

struct pinctrl_at91_pio4 {
	void __iomem *base;
	struct pinctrl_device pinctrl;
	struct gpio_chip gpiochip;
};

struct at91_pinctrl_data {
        unsigned nbanks;
};

static inline void __iomem *pin_to_pio4(struct pinctrl_device *pdev,
					unsigned int *pin)
{
	void __iomem *pio;
	struct pinctrl_at91_pio4 *pinctrl =
		container_of(pdev, struct pinctrl_at91_pio4, pinctrl);

	pio = pinctrl->base + (*pin / 32) * 0x40;
	*pin %= 32;

	return pio;
}

static int __pinctrl_at91_pio4_set_state(struct pinctrl_device *pdev,
					 struct device_node *np)
{
	u32 drive_strength, enable = 0, disable = ~0;
	int output = -1;

	int npins, i;
	int ret;

	ret = of_property_read_u32(np, "drive-strength", &drive_strength);
	if (!ret && ATMEL_PIO_DRVSTR_LO <= drive_strength
	    && drive_strength <= ATMEL_PIO_DRVSTR_HI) {
		disable &= ~PIO4_DRVSTR_MASK;
		enable |= drive_strength << PIO4_DRVSTR_OFFSET;
	}

	if (of_get_property(np, "bias-disable", NULL)) {
		disable &= ~PIO4_PUEN_MASK;
		disable &= ~PIO4_PDEN_MASK;
	}

	if (of_get_property(np, "bias-pull-up", NULL)) {
		enable |= PIO4_PUEN_MASK;
		disable &= ~PIO4_PDEN_MASK;
	}

	if (of_get_property(np, "bias-pull-down", NULL)) {
		enable |= PIO4_PDEN_MASK;
		disable &= ~PIO4_PUEN_MASK;
	}

	if (of_get_property(np, "drive-open-drain", NULL))
		enable |= PIO4_OPD_MASK;

	if (of_get_property(np, "input-schmitt-enable", NULL))
		enable |= PIO4_SCHMITT_MASK;

	if (of_get_property(np, "input-enable", NULL))
		disable &= ~PIO4_DIR_MASK;

	if (of_get_property(np, "output-enable", NULL))
		enable |= PIO4_DIR_MASK;

	if (of_get_property(np, "output-low", NULL))
		output = GPIOF_OUT_INIT_LOW;

	if (of_get_property(np, "output-high", NULL))
		output = GPIOF_OUT_INIT_HIGH;

	of_get_property(np, "pinmux", &npins);
	npins /= sizeof(__be32);

	if (!npins) {
		dev_err(pdev->dev, "Invalid pinmux property in %s\n",
			np->full_name);
		return -EINVAL;
	}

	for (i = 0; i < npins; i++) {
		void __iomem *pio;
		u32 conf, no, func, cfgr;

		ret = of_property_read_u32_index(np, "pinmux", i, &conf);
		if (ret)
			return ret;

		no    = ATMEL_GET_PIN_NO(conf);
		func  = ATMEL_GET_PIN_FUNC(conf);

		pio = pin_to_pio4(pdev, &no);

		if (output == GPIOF_OUT_INIT_HIGH)
			at91_mux_gpio4_set(pio, BIT(no), true);

		if (output == GPIOF_OUT_INIT_LOW)
			at91_mux_gpio4_set(pio, BIT(no), false);

		writel(BIT(no), pio + PIO4_MSKR);
		cfgr = readl(pio + PIO4_CFGR);
		cfgr &= disable;
		cfgr |= enable;
		writel(func | cfgr, pio + PIO4_CFGR);
	}

	return 0;
}

static int pinctrl_at91_pio4_set_state(struct pinctrl_device *pdev,
				       struct device_node *np)
{
	struct device_node *child;
	void *prop;
	int ret;

	prop = of_find_property(np, "pinmux", NULL);
	if (prop)
		return __pinctrl_at91_pio4_set_state(pdev, np);

	for_each_child_of_node(np, child) {
		ret = __pinctrl_at91_pio4_set_state(pdev, child);
		if (ret)
			return ret;
	}

	return 0;
}

static inline void __iomem *pin_to_gpio4(struct gpio_chip *chip, unsigned int *pin)
{
	void __iomem *gpio;
	struct pinctrl_at91_pio4 *pinctrl =
		container_of(chip, struct pinctrl_at91_pio4, gpiochip);

	gpio = pinctrl->base + (*pin / 32) * 0x40;
	*pin %= 32;

	return gpio;
}

static int at91_gpio4_direction_input(struct gpio_chip *chip, unsigned offset)
{
	void __iomem *gpio = pin_to_gpio4(chip, &offset);

	at91_mux_gpio4_input(gpio, BIT(offset), true);

	return 0;
}

static int at91_gpio4_direction_output(struct gpio_chip *chip, unsigned offset,
				      int value)
{
	void __iomem *gpio = pin_to_gpio4(chip, &offset);

	at91_mux_gpio4_set(gpio, BIT(offset), value);
	at91_mux_gpio4_input(gpio, BIT(offset), false);

	return 0;
}

static int at91_gpio4_request(struct gpio_chip *chip, unsigned offset)
{
	void __iomem *gpio = pin_to_gpio4(chip, &offset);

	at91_mux_gpio4_enable(gpio, BIT(offset));

	return 0;
}

static int at91_gpio4_get_direction(struct gpio_chip *chip,
				   unsigned int offset)
{
	u32 cfgr;
	void __iomem *gpio = pin_to_gpio4(chip, &offset);

	writel(BIT(offset), gpio + PIO4_MSKR);
	cfgr = readl(gpio + PIO4_CFGR);

	return cfgr & PIO4_DIR_MASK ? GPIOF_DIR_OUT : GPIOF_DIR_IN;
}

static void at91_gpio4_set(struct gpio_chip *chip, unsigned offset, int value)
{
	void __iomem *gpio = pin_to_gpio4(chip, &offset);

	at91_mux_gpio4_set(gpio, BIT(offset), value);
}

static int at91_gpio4_get(struct gpio_chip *chip, unsigned offset)
{
	void __iomem *gpio = pin_to_gpio4(chip, &offset);

	return at91_mux_gpio4_get(gpio, BIT(offset));
}

static struct gpio_ops at91_gpio4_ops = {
	.request = at91_gpio4_request,
	.direction_input = at91_gpio4_direction_input,
	.direction_output = at91_gpio4_direction_output,
	.get_direction = at91_gpio4_get_direction,
	.get = at91_gpio4_get,
	.set = at91_gpio4_set,
};

static int pinctrl_at91_pio4_gpiochip_add(struct device_d *dev,
					  struct pinctrl_at91_pio4 *pinctrl)
{
	struct at91_pinctrl_data *drvdata;
	struct clk *clk;
	int ret;

	clk = clk_get(dev, NULL);
	if (IS_ERR(clk)) {
		ret = PTR_ERR(clk);
		dev_err(dev, "clock not found: %d\n", ret);
		return ret;
	}

	ret = clk_enable(clk);
	if (ret < 0) {
		dev_err(dev, "clock failed to enable: %d\n", ret);
		clk_put(clk);
		return ret;
	}

	dev_get_drvdata(dev, (const void **)&drvdata);

	pinctrl->gpiochip.ops = &at91_gpio4_ops;
	pinctrl->gpiochip.base = 0;
	pinctrl->gpiochip.ngpio = drvdata->nbanks * MAX_NB_GPIO_PER_BANK;
	pinctrl->gpiochip.dev = dev;

	ret = gpiochip_add(&pinctrl->gpiochip);
	if (ret) {
		dev_err(dev, "couldn't add gpiochip, ret = %d\n", ret);
		return ret;
	}

	dev_info(dev, "gpio driver registered\n");

	return 0;
}

static struct pinctrl_ops pinctrl_at91_pio4_ops = {
	.set_state = pinctrl_at91_pio4_set_state,
};

static int pinctrl_at91_pio4_probe(struct device_d *dev)
{
	struct device_node *np = dev->device_node;
	struct pinctrl_at91_pio4 *pinctrl;
	struct resource *io;
	int ret;

	pinctrl = xzalloc(sizeof(*pinctrl));

	io = dev_request_mem_resource(dev, 0);
	if (IS_ERR(io))
		return PTR_ERR(io);

	pinctrl->base = IOMEM(io->start);
	pinctrl->pinctrl.dev = dev;
	pinctrl->pinctrl.ops = &pinctrl_at91_pio4_ops;

	ret = pinctrl_register(&pinctrl->pinctrl);
	if (ret)
		return ret;

	dev_info(dev, "pinctrl driver registered\n");

	if (of_get_property(np, "gpio-controller", NULL))
		return pinctrl_at91_pio4_gpiochip_add(dev, pinctrl);

	return 0;
}

static const struct at91_pinctrl_data sama5d2_pinctrl_data = {
	.nbanks = 4,
};

static __maybe_unused struct of_device_id pinctrl_at91_pio4_dt_ids[] = {
	{ .compatible = "atmel,sama5d2-pinctrl", .data = &sama5d2_pinctrl_data },
	{ /* sentinel */ }
};

static struct driver_d pinctrl_at91_pio4_driver = {
	.name		= "pinctrl-at91-pio4",
	.probe		= pinctrl_at91_pio4_probe,
	.of_compatible	= DRV_OF_COMPAT(pinctrl_at91_pio4_dt_ids),
};

static int pinctrl_at91_pio4_init(void)
{
	return platform_driver_register(&pinctrl_at91_pio4_driver);
}
core_initcall(pinctrl_at91_pio4_init);
