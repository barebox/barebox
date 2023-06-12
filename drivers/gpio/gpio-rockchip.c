// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <errno.h>
#include <io.h>
#include <of.h>
#include <gpio.h>
#include <init.h>
#include <linux/clk.h>
#include <linux/basic_mmio_gpio.h>
#include <mach/rockchip/rockchip.h>

struct rockchip_gpiochip {
	struct device			*dev;
	void __iomem			*reg_base;
	struct clk			*clk;
	struct bgpio_chip		bgpio_chip;
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
	RK_GPIOV2_EXT_PORT = 0x70,
};

static struct rockchip_gpiochip *gc_to_rockchip_pinctrl(struct gpio_chip *gc)
{
	struct bgpio_chip *bgc = to_bgpio_chip(gc);

	return container_of(bgc, struct rockchip_gpiochip, bgpio_chip);
}

static int rockchip_gpiov2_direction_input(struct gpio_chip *gc, unsigned int gpio)
{
	struct rockchip_gpiochip *rgc = gc_to_rockchip_pinctrl(gc);
	u32 mask;

	mask = 1 << (16 + (gpio % 16));

	if (gpio < 16)
		writel(mask, rgc->reg_base + RK_GPIOV2_DDR_L);
	else
		writel(mask, rgc->reg_base + RK_GPIOV2_DDR_H);

	return 0;
}

static int rockchip_gpiov2_get_direction(struct gpio_chip *gc, unsigned int gpio)
{
	struct rockchip_gpiochip *rgc = gc_to_rockchip_pinctrl(gc);
	u32 r;

	if (gpio < 16)
		r = readl(rgc->reg_base + RK_GPIOV2_DDR_L);
	else
		r = readl(rgc->reg_base + RK_GPIOV2_DDR_H);

	return r & BIT(gpio % 16) ? GPIOF_DIR_OUT : GPIOF_DIR_IN;
}

static void rockchip_gpiov2_set_value(struct gpio_chip *gc, unsigned int gpio,
				      int val)
{
	struct rockchip_gpiochip *rgc = gc_to_rockchip_pinctrl(gc);
	u32 mask, vval = 0;

	mask = 1 << (16 + (gpio % 16));
	if (val)
		vval = 1 << (gpio % 16);

	if (gpio < 16)
		writel(mask | vval, rgc->reg_base + RK_GPIOV2_DR_L);
	else
		writel(mask | vval, rgc->reg_base + RK_GPIOV2_DR_H);
}

static int rockchip_gpiov2_direction_output(struct gpio_chip *gc,
					    unsigned int gpio, int val)
{
	struct rockchip_gpiochip *rgc = gc_to_rockchip_pinctrl(gc);
	u32 mask, out, vval = 0;

	mask = 1 << (16 + (gpio % 16));
	out = 1 << (gpio % 16);
	if (val)
		vval = 1 << (gpio % 16);

	if (gpio < 16) {
		writel(mask | vval, rgc->reg_base + RK_GPIOV2_DR_L);
		writel(mask | out, rgc->reg_base + RK_GPIOV2_DDR_L);
	} else {
		writel(mask | vval, rgc->reg_base + RK_GPIOV2_DR_H);
		writel(mask | out, rgc->reg_base + RK_GPIOV2_DDR_H);
	}

	return 0;
}

static int rockchip_gpiov2_get_value(struct gpio_chip *gc, unsigned int gpio)
{
	struct rockchip_gpiochip *rgc = gc_to_rockchip_pinctrl(gc);
	u32 mask, r;

	mask = 1 << (gpio % 32);
	r = readl(rgc->reg_base + RK_GPIOV2_EXT_PORT);

	return r & mask ? 1 : 0;
}

static struct gpio_ops rockchip_gpio_ops = {
	.direction_input = rockchip_gpiov2_direction_input,
	.direction_output = rockchip_gpiov2_direction_output,
	.get = rockchip_gpiov2_get_value,
	.set = rockchip_gpiov2_set_value,
	.get_direction = rockchip_gpiov2_get_direction,
};

static int rockchip_gpio_probe(struct device *dev)
{
	struct rockchip_gpiochip *rgc;
	struct gpio_chip *gpio;
	struct resource *res;
	void __iomem *reg_base;
	int ret;

	rgc = xzalloc(sizeof(*rgc));
	gpio = &rgc->bgpio_chip.gc;

	res = dev_request_mem_resource(dev, 0);
	if (IS_ERR(res))
		return PTR_ERR(res);

	rgc->reg_base = IOMEM(res->start);

	rgc->clk = clk_get(dev, NULL);

	if (IS_ERR(rgc->clk))
		return PTR_ERR(rgc->clk);

	ret = clk_enable(rgc->clk);
	if (ret)
		return ret;

	reg_base = rgc->reg_base;

	if (rockchip_soc() == 3568) {
		gpio->ngpio = 32;
		gpio->dev = dev;
		gpio->ops = &rockchip_gpio_ops;
		gpio->base = of_alias_get_id(dev->of_node, "gpio");
		if (gpio->base < 0)
			return -EINVAL;
		gpio->base *= 32;
	} else {
		ret = bgpio_init(&rgc->bgpio_chip, dev, 4,
				 reg_base + RK_GPIO_EXT_PORT,
				 reg_base + RK_GPIO_SWPORT_DR, NULL,
				 reg_base + RK_GPIO_SWPORT_DDR, NULL, 0);
		if (ret)
			return ret;
	}

	ret = gpiochip_add(&rgc->bgpio_chip.gc);
	if (ret) {
		dev_err(dev, "failed to register gpio_chip:: %d\n", ret);
		return ret;
	}

	return 0;
}

static struct of_device_id rockchip_gpio_dt_match[] = {
	{
		.compatible = "rockchip,gpio-bank",
	}, {
		/* sentinel */
	}
};
MODULE_DEVICE_TABLE(of, rockchip_gpio_dt_match);

static struct driver rockchip_gpio_driver = {
	.name = "rockchip-gpio",
	.probe = rockchip_gpio_probe,
	.of_compatible = DRV_OF_COMPAT(rockchip_gpio_dt_match),
};

core_platform_driver(rockchip_gpio_driver);
