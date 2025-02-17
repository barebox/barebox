// SPDX-License-Identifier: GPL-2.0-only

#define pr_fmt(fmt) "pinctrl-sunxi: " fmt

#include <common.h>
#include <io.h>
#include <of.h>
#include <of_address.h>
#include <malloc.h>
#include <linux/clk.h>

#include "pinctrl-sunxi.h"

/* This driver assumes the gpio function mux value will not change */
#define FUNC_GPIO_IN	0
#define FUNC_GPIO_OUT	1

static struct sunxi_pinctrl *to_sunxi_pinctrl(struct pinctrl_device *pdev)
{
	return container_of(pdev, struct sunxi_pinctrl, pdev);
}

static void sunxi_pinctrl_set_pull(struct sunxi_pinctrl *pinctrl,
				   u16 pin, u32 pull)
{
	u32 reg = sunxi_pull_reg(pin);
	u32 off = sunxi_pull_offset(pin);
	u32 msk = MUX_PINS_MASK << off;
	u32 val = readl(pinctrl->base + reg);

	val &= ~msk;
	val |= (pull << off) & msk;
	writel(val, pinctrl->base + reg);
}

static void sunxi_pinctrl_set_dlevel(struct sunxi_pinctrl *pinctrl,
				   u16 pin, u32 lvl)
{
	u32 reg = sunxi_dlevel_reg(pin);
	u32 off = sunxi_dlevel_offset(pin);
	u32 msk = MUX_PINS_MASK << off;
	u32 val = readl(pinctrl->base + reg);

	val &= ~msk;
	val |= (lvl << off) & msk;
	writel(val, pinctrl->base + reg);
}

static void sunxi_pinctrl_set_mux(struct sunxi_pinctrl *pinctrl,
				  u16 pin, u8 mux)
{
	u32 reg = sunxi_mux_reg(pin);
	u32 off = sunxi_mux_offset(pin);
	u32 msk = MUX_PINS_MASK << off;
	u32 val = readl(pinctrl->base + reg);

	val &= ~msk;
	val |= (mux << off) & msk;
	writel(val, pinctrl->base + reg);
}

static u8 sunxi_pinctrl_get_mux(struct sunxi_pinctrl *pinctrl, u16 pin)
{
	u32 reg = sunxi_mux_reg(pin);
	u32 off = sunxi_mux_offset(pin);
	u32 val = readl(pinctrl->base + reg);

	return (val >> off) & MUX_PINS_MASK;
}

static void sunxi_pinctrl_set_conf(struct sunxi_pinctrl *pinctrl,
				  u16 pin, struct device_node *node)
{
	u32 val;

	if (of_find_property(node, "bias-pull-up", NULL))
		sunxi_pinctrl_set_pull(pinctrl, pin, 1);
	if (of_find_property(node, "bias-pull-down", NULL))
		sunxi_pinctrl_set_pull(pinctrl, pin, 2);
	if (of_find_property(node, "bias-disable", NULL))
		sunxi_pinctrl_set_pull(pinctrl, pin, 0);

	if (!of_property_read_u32(node, "drive-strength", &val)) {
		val = rounddown(val, 10) / 10 - 1;
		sunxi_pinctrl_set_dlevel(pinctrl, pin, val);
	}
}

static const char *sunxi_pinctrl_parse_function_prop(struct device_node *node)
{
	const char *function;
	int ret;

	/* Try the generic binding */
	ret = of_property_read_string(node, "function", &function);
	if (!ret)
		return function;

	/* And fall back to our legacy one */
	ret = of_property_read_string(node, "allwinner,function", &function);
	if (!ret)
		return function;

	return NULL;
}

static struct property *sunxi_pinctrl_find_pins_prop(struct device_node *node)
{
	struct property *prop;

	/* Try the generic binding */
	prop = of_find_property(node, "pins", NULL);
	if (prop)
		return prop;

	/* And fall back to our legacy one */
	prop = of_find_property(node, "allwinner,pins", NULL);
	if (prop)
		return prop;

	return NULL;
}

#define sunxi_pinctrl_of_pins_for_each_string(np, prop, s)	\
	for (prop = sunxi_pinctrl_find_pins_prop(np),		\
		s = of_prop_next_string(prop, NULL);		\
		s;						\
		s = of_prop_next_string(prop, s))


static const struct sunxi_desc_pin *
sunxi_pinctrl_find_pin(struct sunxi_pinctrl *pinctrl, const char *pin_name)
{
	const struct sunxi_desc_pin *pin;
	int i;

	for (i = 0; i < pinctrl->desc->npins; i++) {
		pin = &pinctrl->desc->pins[i];
		if (!strcmp(pin->pin.name, pin_name))
			return pin;
	}

	return NULL;
}

static const struct sunxi_desc_function *
sunxi_pinctrl_find_func(struct sunxi_pinctrl *pinctrl,
			const char *pin_name, const char *func_name)
{
	const struct sunxi_desc_pin *pin;
	const struct sunxi_desc_function *func;

	pin = sunxi_pinctrl_find_pin(pinctrl, pin_name);
	if (!pin)
		return NULL;

	for (func = pin->functions; func->name; func++)
		if (!strcmp(func->name, func_name))
			return func;

	return NULL;
}

static int sunxi_pinctrl_set_func(struct sunxi_pinctrl *pinctrl,
				  struct device_node *np,
				  const char *pin_name, const char *func_name)
{
	struct device *dev = pinctrl->pdev.dev;
	const struct sunxi_desc_pin *pin;
	const struct sunxi_desc_function *func;

	dev_dbg(dev, "setfunc %s @ %s\n", func_name, pin_name);

	pin = sunxi_pinctrl_find_pin(pinctrl, pin_name);
	if (!pin) {
		dev_err(dev, "pin %s not found\n", pin_name);
		return -EINVAL;
	}

	func = sunxi_pinctrl_find_func(pinctrl, pin_name, func_name);
	if (!func) {
		dev_err(dev, "func %s not found\n", func_name);
		return -EINVAL;
	}

	sunxi_pinctrl_set_mux(pinctrl, pin->pin.number, func->muxval);
	sunxi_pinctrl_set_conf(pinctrl, pin->pin.number, np);

	return 0;
}

static int sunxi_pinctrl_set_state(struct pinctrl_device *pdev, struct device_node *np)
{
	struct sunxi_pinctrl *pinctrl = to_sunxi_pinctrl(pdev);
	struct device *dev = pinctrl->pdev.dev;
	struct property *prop;
	const char *func_name;
	const char *pin_name;

	func_name = sunxi_pinctrl_parse_function_prop(np);
	if (!func_name) {
		dev_err(dev, "%pOF: missing 'function' property\n", np);
		return -EINVAL;
	}

	sunxi_pinctrl_of_pins_for_each_string(np, prop, pin_name) {
		sunxi_pinctrl_set_func(pinctrl, np, pin_name, func_name);
	}

	return 0;
}

static int sunxi_pinctrl_set_direction(struct pinctrl_device *pdev, unsigned int gpio, bool in)
{
	struct sunxi_pinctrl *pinctrl = to_sunxi_pinctrl(pdev);
	u32 func = in ? FUNC_GPIO_IN : FUNC_GPIO_OUT;

	sunxi_pinctrl_set_mux(pinctrl, gpio, func);

	return 0;
}

static int sunxi_gpio_get(struct gpio_chip *chip, unsigned gpio)
{
	struct sunxi_pinctrl *pinctrl = chip->dev->priv;
	u32 reg = sunxi_data_reg(gpio);
	u32 bit = sunxi_data_offset(gpio);
	u32 val = readl(pinctrl->base + reg);

	return val & BIT(bit);
}

static void sunxi_gpio_set(struct gpio_chip *chip, unsigned gpio, int value)
{
	struct sunxi_pinctrl *pinctrl = chip->dev->priv;
	u32 reg = sunxi_data_reg(gpio);
	u32 bit = sunxi_data_offset(gpio);
	u32 val = readl(pinctrl->base + reg);

	if (value)
		val |= BIT(bit);
	else
		val &= ~BIT(bit);
	writel(val, pinctrl->base + reg);
}

static int sunxi_gpio_direction_output(struct gpio_chip *chip,
				       unsigned gpio, int value)
{
	struct sunxi_pinctrl *pinctrl = chip->dev->priv;

	sunxi_gpio_set(chip, gpio, value);
	sunxi_pinctrl_set_mux(pinctrl, gpio, FUNC_GPIO_OUT);

	return 0;
}

static int sunxi_gpio_direction_input(struct gpio_chip *chip,
					unsigned gpio)
{
	struct sunxi_pinctrl *pinctrl = chip->dev->priv;

	sunxi_pinctrl_set_mux(pinctrl, gpio, FUNC_GPIO_IN);

	return 0;
}

static int sunxi_gpio_get_direction(struct gpio_chip *chip, unsigned gpio)
{
	struct sunxi_pinctrl *pinctrl = chip->dev->priv;
	u32 func = sunxi_pinctrl_get_mux(pinctrl, gpio);

	if (func == FUNC_GPIO_IN)
		return GPIOF_DIR_IN;
	if (func == FUNC_GPIO_OUT)
		return GPIOF_DIR_OUT;
	return -EINVAL;
}

static int sunxi_gpio_of_xlate(struct gpio_chip *chip,
			       const struct of_phandle_args *gpiospec,
			       u32 *flags)
{
	int pin, base;

	if (gpiospec->args_count != 3)
		return -EINVAL;

	base = PINS_PER_BANK * gpiospec->args[0];
	pin = base + gpiospec->args[1];

	if (pin > chip->ngpio)
		return -EINVAL;

	if (flags)
		*flags = gpiospec->args[2];

	return pin;
}

static struct pinctrl_ops sunxi_pinctrl_ops = {
	.set_state = sunxi_pinctrl_set_state,
	.set_direction = sunxi_pinctrl_set_direction,
};

static struct gpio_ops sunxi_gpio_ops = {
	.request = sunxi_gpio_direction_input, /* switch to input function */
	.direction_input = sunxi_gpio_direction_input,
	.direction_output = sunxi_gpio_direction_output,
	.get_direction = sunxi_gpio_get_direction,
	.get = sunxi_gpio_get,
	.set = sunxi_gpio_set,
	.of_xlate = sunxi_gpio_of_xlate,
};

int sunxi_pinctrl_probe(struct device *dev)
{
	const struct sunxi_pinctrl_desc *desc;
	struct sunxi_pinctrl *pinctrl;
	struct resource *iores;
	int ret;

	desc = device_get_match_data(dev);
	if (!desc)
                return -EINVAL;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);

	pinctrl = xzalloc(sizeof(*pinctrl));
	dev->priv = pinctrl;
	pinctrl->base = IOMEM(iores->start);

	pinctrl->desc = desc;
	pinctrl->pdev.dev = dev;
	pinctrl->pdev.ops = &sunxi_pinctrl_ops;

	ret = pinctrl_register(&pinctrl->pdev);
	if (ret) {
		dev_err(dev, "couldn't register %s driver\n", "pinctrl");
		goto err;
	}
	dev_dbg(dev, "sunxi %s registered\n", "pinctrl");

	pinctrl->chip.dev = dev;
	pinctrl->chip.ops = &sunxi_gpio_ops;
	/* only the first 8 bank are supported */
	pinctrl->chip.base = 0;
	pinctrl->chip.ngpio = 8 * PINS_PER_BANK;
	pinctrl->chip.of_gpio_n_cells = 3;

	if (of_property_read_bool(dev->of_node, "gpio-controller")) {
		ret = gpiochip_add(&pinctrl->chip);
		if (ret) {
			dev_err(dev, "couldn't register %s driver\n", "gpio-chip");
			goto pinctrl_unregister;
		}
		dev_dbg(dev, "sunxi %s registered\n", "gpio-chip");
	}
	return 0;

pinctrl_unregister:
	pinctrl_unregister(&pinctrl->pdev);
err:
	release_region(iores);
	free(pinctrl);
	return ret;
}
