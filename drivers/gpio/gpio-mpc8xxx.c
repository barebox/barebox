/*
 * GPIOs on MPC512x/8349/8572/8610/QorIQ and compatible
 *
 * Copyright (C) 2008 Peter Korsgaard <jacmet@sunsite.dk>
 * Copyright (C) 2016 Freescale Semiconductor Inc.
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2.  This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 */

#include <common.h>
#include <init.h>
#include <gpio.h>
#include <linux/basic_mmio_gpio.h>
#include <linux/clk.h>
#include <malloc.h>
#include <of.h>
#include <of_address.h>
#include <of_device.h>

#define MPC8XXX_GPIO_PINS	32

#define GPIO_DIR		0x00
#define GPIO_ODR		0x04
#define GPIO_DAT		0x08
#define GPIO_IER		0x0c
#define GPIO_IMR		0x10

struct mpc8xxx_gpio_chip {
	struct bgpio_chip	bgc;
	void __iomem		*regs;
};

struct mpc8xxx_gpio_devtype {
	int (*gpio_dir_out)(struct bgpio_chip *, unsigned int, int);
	int (*gpio_get)(struct bgpio_chip *, unsigned int);
};

static int mpc8xxx_probe(struct device_d *dev)
{
	struct device_node *np;
	struct resource *iores;
	struct mpc8xxx_gpio_chip *mpc8xxx_gc;
	struct bgpio_chip *bgc;
	int ret;

	mpc8xxx_gc = xzalloc(sizeof(*mpc8xxx_gc));

	if (dev->device_node) {
		np = dev->device_node;
	} else {
		dev_err(dev, "no device_node\n");
		return -ENODEV;
        }

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);

	mpc8xxx_gc->regs = IOMEM(iores->start);
	if (!mpc8xxx_gc->regs)
		return -ENOMEM;

        bgc = &mpc8xxx_gc->bgc;

	if (of_property_read_bool(np, "little-endian")) {
		ret = bgpio_init(bgc, dev, 4,
				 mpc8xxx_gc->regs + GPIO_DAT,
				 NULL, NULL,
				 mpc8xxx_gc->regs + GPIO_DIR, NULL, 0);
		if (ret)
			goto err;
		dev_dbg(dev, "GPIO registers are LITTLE endian\n");
	} else {
		ret = bgpio_init(bgc, dev, 4,
				 mpc8xxx_gc->regs + GPIO_DAT,
				 NULL, NULL,
				 mpc8xxx_gc->regs + GPIO_DIR, NULL,
				 BGPIOF_BIG_ENDIAN);
		if (ret)
			goto err;
		dev_dbg(dev, "GPIO registers are BIG endian\n");
	}

	ret = gpiochip_add(&mpc8xxx_gc->bgc.gc);
	if (ret) {
		pr_err("%pOF: GPIO chip registration failed with status %d\n",
		       np, ret);
		goto err;
	}

	/* ack and mask all irqs */
	bgc->write_reg(mpc8xxx_gc->regs + GPIO_IER, 0xffffffff);
	bgc->write_reg(mpc8xxx_gc->regs + GPIO_IMR, 0);

	return 0;

err:
	return ret;
}

static __maybe_unused struct of_device_id mpc8xxx_gpio_ids[] = {
	{
		.compatible = "fsl,qoriq-gpio",
	},
	{
		/* sentinel */
	},
};

static struct driver_d mpc8xxx_driver = {
	.name		= "mpc8xxx-gpio",
	.probe		= mpc8xxx_probe,
	.of_compatible  = DRV_OF_COMPAT(mpc8xxx_gpio_ids),
};

static int __init mpc8xxx_init(void)
{
	return platform_driver_register(&mpc8xxx_driver);
}
postcore_initcall(mpc8xxx_init);
