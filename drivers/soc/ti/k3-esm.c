// SPDX-License-Identifier: GPL-2.0+
/*
 * Texas Instruments' K3 Error Signalling Module driver
 *
 * Copyright (C) 2019 Texas Instruments Incorporated - https://www.ti.com/
 *      Tero Kristo <t-kristo@ti.com>
 *
 */

#include <driver.h>
#include <malloc.h>
#include <xfuncs.h>
#include <io.h>

#define ESM_SFT_RST			0x0c
#define ESM_SFT_RST_KEY			0x0f
#define ESM_EN				0x08
#define ESM_EN_KEY			0x0f

#define ESM_STS(i)			(0x404 + (i) / 32 * 0x20)
#define ESM_STS_MASK(i)			(1 << ((i) % 32))
#define ESM_PIN_EN_SET_OFFSET(i)	(0x414 + (i) / 32 * 0x20)
#define ESM_PIN_MASK(i)			(1 << ((i) % 32))
#define ESM_INTR_EN_SET_OFFSET(i)	(0x408 + (i) / 32 * 0x20)
#define ESM_INTR_MASK(i)		(1 << ((i) % 32))
#define ESM_INTR_PRIO_SET_OFFSET(i)	(0x410 + (i) / 32 * 0x20)
#define ESM_INTR_PRIO_MASK(i)		(1 << ((i) % 32))

static void esm_pin_enable(void __iomem *base, int pin)
{
	u32 value;

	value = readl(base + ESM_PIN_EN_SET_OFFSET(pin));
	value |= ESM_PIN_MASK(pin);
	/* Enable event */
	writel(value, base + ESM_PIN_EN_SET_OFFSET(pin));
}

static void esm_intr_enable(void __iomem *base, int pin)
{
	u32 value;

	value = readl(base + ESM_INTR_EN_SET_OFFSET(pin));
	value |= ESM_INTR_MASK(pin);
	/* Enable Interrupt event */
	writel(value, base + ESM_INTR_EN_SET_OFFSET(pin));
}

static void esm_intr_prio_set(void __iomem *base, int pin)
{
	u32 value;

	value = readl(base + ESM_INTR_PRIO_SET_OFFSET(pin));
	value |= ESM_INTR_PRIO_MASK(pin);
	/* Set to priority */
	writel(value, base + ESM_INTR_PRIO_SET_OFFSET(pin));
}

static void esm_clear_raw_status(void __iomem *base, int pin)
{
	u32 value;

	value = readl(base + ESM_STS(pin));
	value |= ESM_STS_MASK(pin);
	/* Clear Event status */
	writel(value, base + ESM_STS(pin));
}
/**
 * k3_esm_probe: configures ESM based on DT data
 *
 * Parses ESM info from device tree, and configures the module accordingly.
 */
static int k3_esm_probe(struct device *dev)
{
	int ret;
	void __iomem *base;
	int num_pins;
	u32 *pins;
	int i;
	struct device_node *np = dev->of_node;

	base = dev_request_mem_region(dev, 0);
	if (IS_ERR(base))
		return -ENODEV;

	num_pins = of_property_count_elems_of_size(np, "ti,esm-pins", sizeof(u32));
	if (num_pins < 0) {
		dev_err(dev, "ti,esm-pins property missing or invalid: %d\n",
			num_pins);
		return num_pins;
	}

	pins = xzalloc(num_pins * sizeof(u32));
	if (!pins)
		return -ENOMEM;

	ret = of_property_read_u32_array(np, "ti,esm-pins", pins, num_pins);
	if (ret < 0) {
		dev_err(dev, "failed to read ti,esm-pins property: %d\n",
			ret);
		goto free_pins;
	}

	/* Clear any pending events */
	writel(ESM_SFT_RST_KEY, base + ESM_SFT_RST);

	for (i = 0; i < num_pins; i++) {
		esm_intr_prio_set(base, pins[i]);
		esm_clear_raw_status(base, pins[i]);
		esm_pin_enable(base, pins[i]);
		esm_intr_enable(base, pins[i]);
	}

	/* Enable ESM */
	writel(ESM_EN_KEY, base + ESM_EN);

free_pins:
	free(pins);
	return ret;
}

static const struct of_device_id k3_esm_ids[] = {
	{
		.compatible = "ti,j721e-esm",
	}, {
		/* sentinel */
	},
};

static struct driver k3_esm = {
	.probe  = k3_esm_probe,
	.name   = "k3-esm",
	.of_compatible = k3_esm_ids,
};

core_platform_driver(k3_esm);
