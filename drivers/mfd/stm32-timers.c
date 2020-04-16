// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) STMicroelectronics 2016
 * Author: Benjamin Gaignard <benjamin.gaignard@st.com>
 */

#include <common.h>
#include <linux/clk.h>
#include <driver.h>
#include <init.h>
#include <io.h>
#include <linux/bitfield.h>
#include <linux/mfd/stm32-timers.h>
#include <of.h>
#include <linux/reset.h>

#define STM32_TIMERS_MAX_REGISTERS	0x3fc

static const struct regmap_config stm32_timers_regmap_cfg = {
	.reg_bits = 32,
	.val_bits = 32,
	.reg_stride = sizeof(u32),
	.max_register = STM32_TIMERS_MAX_REGISTERS,
};

static void stm32_timers_get_arr_size(struct stm32_timers *ddata)
{
	/*
	 * Only the available bits will be written so when readback
	 * we get the maximum value of auto reload register
	 */
	regmap_write(ddata->regmap, TIM_ARR, ~0L);
	regmap_read(ddata->regmap, TIM_ARR, &ddata->max_arr);
	regmap_write(ddata->regmap, TIM_ARR, 0x0);
}

static int stm32_timers_probe(struct device_d *dev)
{
	struct stm32_timers *ddata;
	struct resource *res;

	ddata = xzalloc(sizeof(*ddata));

	res = dev_request_mem_resource(dev, 0);
	if (IS_ERR(res))
		return PTR_ERR(res);

	ddata->regmap = regmap_init_mmio_clk(dev, "int", IOMEM(res->start),
					     &stm32_timers_regmap_cfg);
	if (IS_ERR(ddata->regmap))
		return PTR_ERR(ddata->regmap);

	ddata->clk = clk_get(dev, NULL);
	if (IS_ERR(ddata->clk))
		return PTR_ERR(ddata->clk);

	stm32_timers_get_arr_size(ddata);

	dev->priv = ddata;

	return of_platform_populate(dev->device_node, NULL, dev);
}

static const struct of_device_id stm32_timers_of_match[] = {
	{ .compatible = "st,stm32-timers", },
	{ /* sentinel */ },
};

static struct driver_d stm32_timers_driver = {
	.name = "stm32-timers",
	.probe = stm32_timers_probe,
	.of_compatible = stm32_timers_of_match,
};
coredevice_platform_driver(stm32_timers_driver);
