// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2021, WolfVision GmbH
 * Author: Michael Riesch <michael.riesch@wolfvision.net>
 *
 * Originally based on the Linux kernel v5.12 drivers/iio/adc/rockchip-saradc.c.
 */

#include <common.h>
#include <aiodev.h>
#include <regulator.h>
#include <linux/bitfield.h>
#include <linux/clk.h>
#include <linux/reset.h>

/* v1 registers */
#define SARADC_DATA	    		0x00
#define SARADC_CTRL			0x08
#	define SARADC_CTRL_IRQ_STATUS	(1 << 6)
#	define SARADC_CTRL_IRQ_ENABLE	(1 << 5)
#	define SARADC_CTRL_POWER_CTRL	(1 << 3)
#define SARADC_CTRL_CHN_MASK		0x07
#define SARADC_DLY_PU_SOC		0x0c

/* v2 registers */
#define SARADC2_CONV_CON		0x000
#	define SARADC2_CONV_CHANNELS	GENMASK(3, 0)
#	define SARADC2_START		BIT(4)
#	define SARADC2_SINGLE_MODE	BIT(5)
#define SARADC_T_PD_SOC			0x004
#define SARADC_T_DAS_SOC		0x00c
#define SARADC2_END_INT_EN		0x104
#	define SARADC2_EN_END_INT	BIT(0)
#define SARADC2_END_INT_ST		0x110
#define SARADC2_DATA_BASE		0x120

#define SARADC_TIMEOUT_NS      (100 * MSECOND)

struct rockchip_saradc_data;

struct rockchip_saradc_cfg {
	unsigned int num_bits;
	unsigned int num_channels;
	void (*init)(struct rockchip_saradc_data *data);
	int (*read)(struct aiochannel *chan, int *val);
};

struct rockchip_saradc_data {
	const struct rockchip_saradc_cfg *config;
	void __iomem *base;
	struct regulator *vref;
	unsigned int ref_voltage_mv;
	struct clk *cclk;
	struct clk *pclk;
	struct aiodevice aiodev;
	struct aiochannel *channels;
	struct reset_control *reset;
};

static inline void rockchip_saradc_reg_wr(struct rockchip_saradc_data *data,
					  u32 value, u32 reg)
{
	writel(value, data->base + reg);
}

static inline u32 rockchip_saradc_reg_rd(struct rockchip_saradc_data *data,
					 u32 reg)
{
	return readl(data->base + reg);
}

static void rockchip_saradc_reset_controller(struct reset_control *reset)
{
	reset_control_assert(reset);
	udelay(20);
	reset_control_deassert(reset);
}

static void rockchip_saradc_init_v1(struct rockchip_saradc_data *data)
{
	rockchip_saradc_reg_wr(data, 0, SARADC_CTRL);
};

static int rockchip_saradc_read_v1(struct aiochannel *chan, int *val)
{
	struct rockchip_saradc_data *data;
	u32 value, control, mask;
	u64 start;

	data = container_of(chan->aiodev, struct rockchip_saradc_data, aiodev);

	rockchip_saradc_reg_wr(data, 8, SARADC_DLY_PU_SOC);
	rockchip_saradc_reg_wr(data,
			       (chan->index & SARADC_CTRL_CHN_MASK) |
				       SARADC_CTRL_IRQ_ENABLE |
				       SARADC_CTRL_POWER_CTRL,
			       SARADC_CTRL);

	start = get_time_ns();
	do {
		control = rockchip_saradc_reg_rd(data, SARADC_CTRL);

		if (is_timeout(start, SARADC_TIMEOUT_NS))
			return -ETIMEDOUT;
	} while (!(control & SARADC_CTRL_IRQ_STATUS));

	mask = (1 << data->config->num_bits) - 1;
	value = rockchip_saradc_reg_rd(data, SARADC_DATA) & mask;
	rockchip_saradc_reg_wr(data, 0, SARADC_CTRL);

	*val = (value * data->ref_voltage_mv) / mask;

	return 0;
}

static int rockchip_saradc_read_v2(struct aiochannel *chan, int *val)
{
	struct rockchip_saradc_data *data;
	u32 value, status, mask;
	u64 start;

	data = container_of(chan->aiodev, struct rockchip_saradc_data, aiodev);

	rockchip_saradc_reset_controller(data->reset);

	rockchip_saradc_reg_wr(data, 0xc, SARADC_T_DAS_SOC);
	rockchip_saradc_reg_wr(data, 0x20, SARADC_T_PD_SOC);
	value = FIELD_PREP(SARADC2_EN_END_INT, 1);
	value |= SARADC2_EN_END_INT << 16;
	rockchip_saradc_reg_wr(data, value, SARADC2_END_INT_EN);
	value = FIELD_PREP(SARADC2_START, 1) |
		FIELD_PREP(SARADC2_SINGLE_MODE, 1) |
		FIELD_PREP(SARADC2_CONV_CHANNELS, chan->index);
	value |= (SARADC2_START | SARADC2_SINGLE_MODE | SARADC2_CONV_CHANNELS) << 16;
	rockchip_saradc_reg_wr(data, value, SARADC2_CONV_CON);

	start = get_time_ns();
	do {
		status = rockchip_saradc_reg_rd(data, SARADC2_END_INT_ST);

		if (is_timeout(start, SARADC_TIMEOUT_NS))
			return -ETIMEDOUT;
	} while (!(status & SARADC2_EN_END_INT));

	mask = (1 << data->config->num_bits) - 1;
	value = rockchip_saradc_reg_rd(data, SARADC2_DATA_BASE + chan->index * 4);
	value &= mask;

	rockchip_saradc_reg_wr(data, SARADC2_EN_END_INT, SARADC2_END_INT_ST);

	*val = (value * data->ref_voltage_mv) / mask;

	return 0;
}

static int rockchip_saradc_probe(struct device *dev)
{
	struct rockchip_saradc_data *data;
	int i, ret;

	data = xzalloc(sizeof(struct rockchip_saradc_data));

	data->config = device_get_match_data(dev);
	data->aiodev.hwdev = dev;
	data->aiodev.read = data->config->read;

	data->base = dev_request_mem_region(dev, 0);
	if (IS_ERR(data->base)) {
		ret = PTR_ERR(data->base);
		goto fail_data;
	}

	data->vref = regulator_get(dev, "vref");
	if (IS_ERR(data->vref)) {
		dev_err(dev, "can't get vref-supply: %pe\n", data->vref);
		ret = PTR_ERR(data->vref);
		goto fail_data;
	}

	ret = regulator_enable(data->vref);
	if (ret < 0) {
		dev_err(dev, "can't enable vref-supply value: %d\n", ret);
		goto fail_data;
	}

	ret = regulator_get_voltage(data->vref);
	if (ret < 0) {
		dev_warn(dev, "can't get vref-supply value: %d\n", ret);
		/* use default value as fallback */
		ret = 1800000;
	}
	data->ref_voltage_mv = ret / 1000;

	data->cclk = clk_get(dev, "saradc");
	if (IS_ERR(data->cclk)) {
		dev_err(dev, "can't get converter clock: %pe\n", data->cclk);
		ret = PTR_ERR(data->cclk);
		goto fail_data;
	}

	ret = clk_enable(data->cclk);
	if (ret < 0) {
		dev_err(dev, "can't enable converter clock: %pe\n",
			ERR_PTR(ret));
		goto fail_data;
	}

	data->pclk = clk_get(dev, "apb_pclk");
	if (IS_ERR(data->pclk)) {
		dev_err(dev, "can't get peripheral clock: %pe\n", data->pclk);
		ret = PTR_ERR(data->pclk);
		goto fail_data;
	}

	ret = clk_enable(data->pclk);
	if (ret < 0) {
		dev_err(dev, "can't enable peripheral clk: %pe\n",
			ERR_PTR(ret));
		goto fail_data;
	}

	data->aiodev.num_channels = data->config->num_channels;
	data->channels =
		xzalloc(sizeof(*data->channels) * data->aiodev.num_channels);
	data->aiodev.channels = xmalloc(sizeof(*data->aiodev.channels) *
					data->aiodev.num_channels);
	for (i = 0; i < data->aiodev.num_channels; i++) {
		data->aiodev.channels[i] = &data->channels[i];
		data->channels[i].unit = "mV";
	}

	if (data->config->init)
		 data->config->init(data);

	data->reset = reset_control_get(dev, "saradc-apb");
	if (IS_ERR(data->reset))
		data->reset = NULL;

	ret = aiodevice_register(&data->aiodev);
	if (ret)
		goto fail_channels;

	rockchip_saradc_reset_controller(data->reset);

	dev_info(dev, "registered as %s\n", dev_name(&data->aiodev.dev));

	return 0;

fail_channels:
	reset_control_put(data->reset);
	kfree(data->channels);
	kfree(data->aiodev.channels);

fail_data:
	kfree(data);
	return ret;
}

static const struct rockchip_saradc_cfg rk3568_saradc_cfg = {
	.num_bits = 10,
	.num_channels = 8,
	.init = rockchip_saradc_init_v1,
	.read = rockchip_saradc_read_v1,
};

static const struct rockchip_saradc_cfg rk3576_saradc_cfg = {
	.num_bits = 12,
	.num_channels = 8,
	.read = rockchip_saradc_read_v2,
};

static const struct rockchip_saradc_cfg rk3588_saradc_cfg = {
	.num_bits = 12,
	.num_channels = 8,
	.read = rockchip_saradc_read_v2,
};

static const struct of_device_id of_rockchip_saradc_match[] = {
	{ .compatible = "rockchip,rk3568-saradc", .data = &rk3568_saradc_cfg },
	{ .compatible = "rockchip,rk3576-saradc", .data = &rk3576_saradc_cfg },
	{ .compatible = "rockchip,rk3588-saradc", .data = &rk3588_saradc_cfg },
	{ /* end */ }
};
MODULE_DEVICE_TABLE(of, of_rockchip_saradc_match);

static struct driver rockchip_saradc_driver = {
	.name = "rockchip_saradc",
	.probe = rockchip_saradc_probe,
	.of_compatible = DRV_OF_COMPAT(of_rockchip_saradc_match),
};
device_platform_driver(rockchip_saradc_driver);
