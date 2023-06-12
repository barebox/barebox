// SPDX-License-Identifier: GPL-2.0-only
/*
 * COPYRIGHT 2020 Shanghai StarFive Technology Co., Ltd.
 */

#include <linux/basic_mmio_gpio.h>
#include <linux/reset.h>
#include <linux/clk.h>
#include <linux/printk.h>
#include <driver.h>
#include <errno.h>
#include <pinctrl.h>

#define GPIO_EN		0x0
#define GPIO_IS_LOW	0x10
#define GPIO_IS_HIGH	0x14
#define GPIO_IBE_LOW	0x18
#define GPIO_IBE_HIGH	0x1c
#define GPIO_IEV_LOW	0x20
#define GPIO_IEV_HIGH	0x24
#define GPIO_IE_LOW	0x28
#define GPIO_IE_HIGH	0x2c
#define GPIO_IC_LOW	0x30
#define GPIO_IC_HIGH	0x34
//read only
#define GPIO_RIS_LOW	0x38
#define GPIO_RIS_HIGH	0x3c
#define GPIO_MIS_LOW	0x40
#define GPIO_MIS_HIGH	0x44
#define GPIO_DIN_LOW	0x48
#define GPIO_DIN_HIGH	0x4c

#define GPIO_DOUT_X_REG	0x50
#define GPIO_DOEN_X_REG	0x54

#define MAX_GPIO	 64

struct starfive_gpio {
	void __iomem		*base;
	struct gpio_chip	gc;
};

#define to_starfive_gpio(gc) container_of(gc, struct starfive_gpio, gc)

static int starfive_direction_input(struct gpio_chip *gc, unsigned offset)
{
	struct starfive_gpio *chip = to_starfive_gpio(gc);

	if (offset >= gc->ngpio)
		return -EINVAL;

	writel(0x1, chip->base + GPIO_DOEN_X_REG + offset * 8);

	return 0;
}

static int starfive_direction_output(struct gpio_chip *gc, unsigned offset, int value)
{
	struct starfive_gpio *chip = to_starfive_gpio(gc);

	if (offset >= gc->ngpio)
		return -EINVAL;
	writel(0x0, chip->base + GPIO_DOEN_X_REG + offset * 8);
	writel(value, chip->base + GPIO_DOUT_X_REG + offset * 8);

	return 0;
}

static int starfive_get_direction(struct gpio_chip *gc, unsigned offset)
{
	struct starfive_gpio *chip = to_starfive_gpio(gc);

	if (offset >= gc->ngpio)
		return -EINVAL;

	return readl(chip->base + GPIO_DOEN_X_REG + offset * 8) & 0x1;
}

static int starfive_get_value(struct gpio_chip *gc, unsigned offset)
{
	struct starfive_gpio *chip = to_starfive_gpio(gc);
	int value;

	if (offset >= gc->ngpio)
		return -EINVAL;

	if(offset < 32){
		value = readl(chip->base + GPIO_DIN_LOW);
		return (value >> offset) & 0x1;
	} else {
		value = readl(chip->base + GPIO_DIN_HIGH);
		return (value >> (offset - 32)) & 0x1;
	}
}

static void starfive_set_value(struct gpio_chip *gc, unsigned offset, int value)
{
	struct starfive_gpio *chip = to_starfive_gpio(gc);

	if (offset >= gc->ngpio)
		return;

	writel(value, chip->base + GPIO_DOUT_X_REG + offset * 8);
}

static struct gpio_ops starfive_gpio_ops = {
	.direction_input = starfive_direction_input,
	.direction_output = starfive_direction_output,
	.get_direction = starfive_get_direction,
	.get = starfive_get_value,
	.set = starfive_set_value,
};

static int starfive_gpio_probe(struct device *dev)
{
	struct starfive_gpio *chip;
	struct resource *res;
	struct clk *clk;
	int ret;

	clk = clk_get(dev, NULL);
	if (IS_ERR(clk))
		return PTR_ERR(clk);

	clk_enable(clk);

	ret = device_reset(dev);
	if (ret)
		return ret;

	ret = pinctrl_single_probe(dev);
	if (ret)
		return ret;

	res = dev_get_resource(dev, IORESOURCE_MEM, 0);
	if (IS_ERR(res))
		return PTR_ERR(res);

	chip = xzalloc(sizeof(*chip));
	chip->base = IOMEM(res->start);

	chip->gc.base = -1;
	chip->gc.ngpio = MAX_GPIO;
	chip->gc.dev = dev;
	chip->gc.ops = &starfive_gpio_ops;

	/* Disable all GPIO interrupts */
	iowrite32(0, chip->base + GPIO_IE_HIGH);
	iowrite32(0, chip->base + GPIO_IE_LOW);

	ret = gpiochip_add(&chip->gc);
	if (ret) {
		dev_err(dev, "could not add gpiochip\n");
		gpiochip_remove(&chip->gc);
		return ret;
	}

	writel(1, chip->base + GPIO_EN);

	return 0;
}

static const struct of_device_id starfive_gpio_match[] = {
	{ .compatible = "starfive,gpio0", },
	{ },
};
MODULE_DEVICE_TABLE(of, starfive_gpio_match);

static struct driver starfive_gpio_driver = {
	.probe	= starfive_gpio_probe,
	.name		= "starfive_gpio",
	.of_compatible	= starfive_gpio_match,
};
postcore_platform_driver(starfive_gpio_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Huan Feng <huan.feng@starfivetech.com>");
MODULE_DESCRIPTION("Starfive VIC GPIO generator driver");
