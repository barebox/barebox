/* *
 * Copyright (C) 2010 Erik Gilling <konkers@google.com>, Google, Inc
 * Copyright (C) 2013 Lucas Stach <l.stach@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <common.h>
#include <gpio.h>
#include <init.h>
#include <io.h>
#include <linux/err.h>

#define GPIO_BANK(x)		((x) >> 5)
#define GPIO_PORT(x)		(((x) >> 3) & 0x3)
#define GPIO_BIT(x)		((x) & 0x7)

#define GPIO_REG(x)		(GPIO_BANK(x) * config->bank_stride + \
					GPIO_PORT(x) * 4)

#define GPIO_CNF(x)		(GPIO_REG(x) + 0x00)
#define GPIO_OE(x)		(GPIO_REG(x) + 0x10)
#define GPIO_OUT(x)		(GPIO_REG(x) + 0x20)
#define GPIO_IN(x)		(GPIO_REG(x) + 0x30)
#define GPIO_INT_ENB(x)		(GPIO_REG(x) + 0x50)

#define GPIO_MSK_CNF(x)		(GPIO_REG(x) + config->upper_offset + 0x00)
#define GPIO_MSK_OE(x)		(GPIO_REG(x) + config->upper_offset + 0x10)
#define GPIO_MSK_OUT(x)		(GPIO_REG(x) + config->upper_offset + 0X20)

struct tegra_gpio_bank {
	int bank;
	int irq;
};

struct tegra_gpio_soc_config {
	u32 bank_stride;
	u32 upper_offset;
	u32 bank_count;
};

static void __iomem *gpio_base;
static struct tegra_gpio_soc_config *config;

static inline void tegra_gpio_writel(u32 val, u32 reg)
{
	writel(val, gpio_base + reg);
}

static inline u32 tegra_gpio_readl(u32 reg)
{
	return readl(gpio_base + reg);
}

static int tegra_gpio_compose(int bank, int port, int bit)
{
	return (bank << 5) | ((port & 0x3) << 3) | (bit & 0x7);
}

static void tegra_gpio_mask_write(u32 reg, int gpio, int value)
{
	u32 val;

	val = 0x100 << GPIO_BIT(gpio);
	if (value)
		val |= 1 << GPIO_BIT(gpio);
	tegra_gpio_writel(val, reg);
}

static void tegra_gpio_enable(int gpio)
{
	tegra_gpio_mask_write(GPIO_MSK_CNF(gpio), gpio, 1);
}

static void tegra_gpio_disable(int gpio)
{
	tegra_gpio_mask_write(GPIO_MSK_CNF(gpio), gpio, 0);
}

static int tegra_gpio_request(struct gpio_chip *chip, unsigned offset)
{
	return 0;
}

static void tegra_gpio_free(struct gpio_chip *chip, unsigned offset)
{
	tegra_gpio_disable(offset);
}

static void tegra_gpio_set(struct gpio_chip *chip, unsigned offset, int value)
{
	tegra_gpio_mask_write(GPIO_MSK_OUT(offset), offset, value);
}

static int tegra_gpio_get(struct gpio_chip *chip, unsigned offset)
{
	/* If gpio is in output mode then read from the out value */
	if ((tegra_gpio_readl(GPIO_OE(offset)) >> GPIO_BIT(offset)) & 1) {
		printf("GPIO output mode\n");
		return (tegra_gpio_readl(GPIO_OUT(offset)) >>
				GPIO_BIT(offset)) & 0x1;
	}

	printf("GPIO input mode\n");
	return (tegra_gpio_readl(GPIO_IN(offset)) >> GPIO_BIT(offset)) & 0x1;
}

static int tegra_gpio_direction_input(struct gpio_chip *chip, unsigned offset)
{
	tegra_gpio_mask_write(GPIO_MSK_OE(offset), offset, 0);
	tegra_gpio_enable(offset);
	return 0;
}

static int tegra_gpio_direction_output(struct gpio_chip *chip, unsigned offset,
					int value)
{
	tegra_gpio_set(chip, offset, value);
	tegra_gpio_mask_write(GPIO_MSK_OE(offset), offset, 1);
	tegra_gpio_enable(offset);
	return 0;
}

static struct gpio_ops tegra_gpio_ops = {
	.request		= tegra_gpio_request,
	.free			= tegra_gpio_free,
	.direction_input	= tegra_gpio_direction_input,
	.direction_output	= tegra_gpio_direction_output,
	.get			= tegra_gpio_get,
	.set			= tegra_gpio_set,
};

static struct gpio_chip tegra_gpio_chip = {
	.ops	= &tegra_gpio_ops,
	.base	= 0,
};

static int tegra_gpio_probe(struct device_d *dev)
{
	int i, j, ret;

	ret = dev_get_drvdata(dev, (unsigned long *)&config);
	if (ret) {
		dev_err(dev, "dev_get_drvdata failed: %d\n", ret);
		return ret;
	}

	gpio_base = dev_request_mem_region(dev, 0);
	if (!gpio_base) {
		dev_err(dev, "could not get memory region\n");
		return -ENODEV;
	}

	for (i = 0; i < config->bank_count; i++) {
		for (j = 0; j < 4; j++) {
			int gpio = tegra_gpio_compose(i, j, 0);
			tegra_gpio_writel(0x00, GPIO_INT_ENB(gpio));
		}
	}

	tegra_gpio_chip.ngpio = config->bank_count * 32;
	tegra_gpio_chip.dev = dev;

	gpiochip_add(&tegra_gpio_chip);

	return 0;
}

static struct tegra_gpio_soc_config tegra20_gpio_config = {
	.bank_stride = 0x80,
	.upper_offset = 0x800,
	.bank_count = 7,
};

static struct platform_device_id tegra_gpio_ids[] = {
	{
		.name = "tegra20-gpio",
		.driver_data = (unsigned long)&tegra20_gpio_config,
	}, {
		/* sentinel */
	},
};

static __maybe_unused struct of_device_id tegra_gpio_dt_ids[] = {
	{
		.compatible = "nvidia,tegra20-gpio",
		.data = (unsigned long)&tegra20_gpio_config
	}, {
		/* sentinel */
	},
};

static struct driver_d tegra_gpio_driver = {
	.name		= "tegra-gpio",
	.id_table	= tegra_gpio_ids,
	.of_compatible	= DRV_OF_COMPAT(tegra_gpio_dt_ids),
	.probe		= tegra_gpio_probe,
};

static int __init tegra_gpio_init(void)
{
	return platform_driver_register(&tegra_gpio_driver);
}
coredevice_initcall(tegra_gpio_init);
