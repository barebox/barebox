/*
 * Designware GPIO support functions
 *
 * Copyright (C) 2012 Altera
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <common.h>
#include <errno.h>
#include <io.h>
#include <gpio.h>
#include <init.h>

#define DW_GPIO_DR 		0x0
#define DW_GPIO_DDR		0x4
#define DW_GPIO_EXT 		0x50
#define DW_GPIO_CONFIG2		0x70
#define DW_GPIO_CONFIG1		0x74

#define DW_GPIO_CONFIG2_WIDTH(val, port)	(((val) >> ((port) * 4) & 0x1f) + 1)
#define DW_GPIO_CONFIG1_NPORTS(val)		(((val) >> 2 & 0x3) + 1)

struct dw_gpio_instance {
	struct gpio_chip chip;
	u32 gpio_state;		/* GPIO state shadow register */
	u32 gpio_dir;		/* GPIO direction shadow register */
	void __iomem *regs;
};

static inline struct dw_gpio_instance *to_dw_gpio(struct gpio_chip *gc)
{
	return container_of(gc, struct dw_gpio_instance, chip);
}

static int dw_gpio_get(struct gpio_chip *gc, unsigned offset)
{
	struct dw_gpio_instance *chip = to_dw_gpio(gc);

	return (readl(chip->regs + DW_GPIO_EXT) >> offset) & 1;
}

static void dw_gpio_set(struct gpio_chip *gc, unsigned offset, int value)
{
	struct dw_gpio_instance *chip = to_dw_gpio(gc);
	u32 data_reg;

	data_reg = readl(chip->regs + DW_GPIO_DR);
	data_reg = (data_reg & ~(1<<offset)) | (value << offset);
	writel(data_reg, chip->regs + DW_GPIO_DR);
}

static int dw_gpio_direction_input(struct gpio_chip *gc, unsigned offset)
{
	struct dw_gpio_instance *chip = to_dw_gpio(gc);
	u32 gpio_ddr;

	/* Set pin as input, assumes software controlled IP */
	gpio_ddr = readl(chip->regs + DW_GPIO_DDR);
	gpio_ddr &= ~(1 << offset);
	writel(gpio_ddr, chip->regs + DW_GPIO_DDR);

	return 0;
}

static int dw_gpio_direction_output(struct gpio_chip *gc,
		unsigned offset, int value)
{
	struct dw_gpio_instance *chip = to_dw_gpio(gc);
	u32 gpio_ddr;

	dw_gpio_set(gc, offset, value);

	/* Set pin as output, assumes software controlled IP */
	gpio_ddr = readl(chip->regs + DW_GPIO_DDR);
	gpio_ddr |= (1 << offset);
	writel(gpio_ddr, chip->regs + DW_GPIO_DDR);

	return 0;
}

static struct gpio_ops imx_gpio_ops = {
	.direction_input = dw_gpio_direction_input,
	.direction_output = dw_gpio_direction_output,
	.get = dw_gpio_get,
	.set = dw_gpio_set,
};

static int dw_gpio_probe(struct device_d *dev)
{
	struct dw_gpio_instance *chip;
	uint32_t config1, config2;
	int ngpio, ret;

	chip = xzalloc(sizeof(*chip));
	chip->regs = dev_request_mem_region(dev, 0);
	if (!chip->regs)
		return -EBUSY;

	chip->chip.ops = &imx_gpio_ops;
	if (dev->id < 0) {
		chip->chip.base = of_alias_get_id(dev->device_node, "gpio");
		if (chip->chip.base < 0)
			return chip->chip.base;
		chip->chip.base *= 32;
	} else {
		chip->chip.base = dev->id * 32;
	}

	config2 = readl(chip->regs + DW_GPIO_CONFIG2);
	config1 = readl(chip->regs + DW_GPIO_CONFIG1);
	ngpio = DW_GPIO_CONFIG2_WIDTH(config2, 0);

	if (DW_GPIO_CONFIG1_NPORTS(config1) > 1)
		dev_info(dev, "ignoring ports B-D\n");

	chip->chip.ngpio = ngpio;
	chip->chip.dev = dev;

	ret = gpiochip_add(&chip->chip);
	if (ret)
		return ret;

	dev_dbg(dev, "probed gpiochip with %d gpios, base %d\n",
			chip->chip.ngpio, chip->chip.base);

	return 0;
}

static __maybe_unused struct of_device_id dwgpio_match[] = {
	{
		.compatible = "snps,dw-apb-gpio",
	}, {
		/* sentinel */
	},
};

static struct driver_d dwgpio_driver = {
	.name = "dw-apb-gpio",
	.probe = dw_gpio_probe,
	.of_compatible = DRV_OF_COMPAT(dwgpio_match),
};

static int __init dwgpio_init(void)
{
	return platform_driver_register(&dwgpio_driver);
}
core_initcall(dwgpio_init);
