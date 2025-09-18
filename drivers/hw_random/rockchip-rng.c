// SPDX-License-Identifier: GPL-2.0
/*
 * rockchip-rng.c Random Number Generator driver for the Rockchip
 *
 * Copyright (c) 2018, Fuzhou Rockchip Electronics Co., Ltd.
 * Author: Lin Jinhan <troy.lin@rock-chips.com>
 *
 */
#include <linux/clk.h>
#include <linux/hw_random.h>
#include <linux/iopoll.h>
#include <linux/reset.h>
#include <linux/bitfield.h>
#include <linux/mod_devicetable.h>
#include <of.h>
#include <of_address.h>
#include <linux/device.h>

#define HIWORD_UPDATE(val, mask, shift) \
			((val) << (shift) | (mask) << ((shift) + 16))

#define ROCKCHIP_AUTOSUSPEND_DELAY		100
#define ROCKCHIP_POLL_PERIOD_US			100
#define ROCKCHIP_POLL_TIMEOUT_US		10000
#define RK_MAX_RNG_BYTE				(32)

#define CRYPTO_V1_CTRL				0x0008
#define CRYPTO_V1_RNG_START			BIT(8)
#define CRYPTO_V1_RNG_FLUSH			BIT(9)
#define CRYPTO_V1_TRNG_CTRL			0x0200
#define CRYPTO_V1_OSC_ENABLE			BIT(16)
#define CRYPTO_V1_TRNG_SAMPLE_PERIOD(x)		(x)
#define CRYPTO_V1_TRNG_DOUT_0			0x0204

#define CRYPTO_V2_RNG_CTL			0x0400
#define CRYPTO_V2_RNG_BIT_LEN			GENMASK(5, 4)
#define CRYPTO_V2_RNG_64_BIT_LEN		FIELD_PREP(CRYPTO_V2_RNG_BIT_LEN, 0)
#define CRYPTO_V2_RNG_128_BIT_LEN		FIELD_PREP(CRYPTO_V2_RNG_BIT_LEN, 1)
#define CRYPTO_V2_RNG_192_BIT_LEN		FIELD_PREP(CRYPTO_V2_RNG_BIT_LEN, 2)
#define CRYPTO_V2_RNG_256_BIT_LEN		FIELD_PREP(CRYPTO_V2_RNG_BIT_LEN, 3)
#define CRYPTO_V2_RNG_SOC_RING			GENMASK(3, 2)
#define CRYPTO_V2_RNG_FASTEST_SOC_RING		FIELD_PREP(CRYPTO_V2_RNG_SOC_RING, 0)
#define CRYPTO_V2_RNG_SLOWER_SOC_RING_0		FIELD_PREP(CRYPTO_V2_RNG_SOC_RING, 1)
#define CRYPTO_V2_RNG_SLOWER_SOC_RING_1		FIELD_PREP(CRYPTO_V2_RNG_SOC_RING, 2)
#define CRYPTO_V2_RNG_SLOWEST_SOC_RING		FIELD_PREP(CRYPTO_V2_RNG_SOC_RING, 3)
#define CRYPTO_V2_RNG_ENABLE			BIT(1)
#define CRYPTO_V2_RNG_START			BIT(0)
#define CRYPTO_V2_RNG_SAMPLE_CNT		0x0404
#define CRYPTO_V2_RNG_DOUT_0			0x0410

struct rk_rng_soc_data {
	const char * const *clks;
	int clks_num;
	int (*rk_rng_read)(struct hwrng *rng, void *buf, size_t max, bool wait);
};

struct rk_rng {
	struct device		*dev;
	struct hwrng		rng;
	void __iomem		*mem;
	struct rk_rng_soc_data	*soc_data;
	u32			clk_num;
	struct clk_bulk_data	*clk_bulks;
};

static void rk_rng_writel(struct rk_rng *rng, u32 val, u32 offset)
{
	__raw_writel(val, rng->mem + offset);
}

static u32 rk_rng_readl(struct rk_rng *rng, u32 offset)
{
	return __raw_readl(rng->mem + offset);
}

static int rk_rng_init(struct hwrng *rng)
{
	int ret;
	struct rk_rng *rk_rng = container_of(rng, struct rk_rng, rng);

	ret = clk_bulk_prepare_enable(rk_rng->clk_num, rk_rng->clk_bulks);
	if (ret < 0) {
		dev_err(rk_rng->dev, "failed to enable clks %d\n", ret);
		return ret;
	}

	return 0;
}

static void rk_rng_cleanup(struct rk_rng *rk_rng)
{
	clk_bulk_disable_unprepare(rk_rng->clk_num, rk_rng->clk_bulks);
}

static void rk_rng_read_regs(struct rk_rng *rng, u32 offset, void *buf,
			     size_t size)
{
	u32 i, sample;

	for (i = 0; i < size; i += 4) {
		sample = rk_rng_readl(rng, offset + i);
		memcpy(buf + i, &sample, sizeof(sample));
	}
}

static int rk_rng_v1_read(struct hwrng *rng, void *buf, size_t max, bool wait)
{
	int ret = 0;
	u32 reg_ctrl = 0;
	struct rk_rng *rk_rng = container_of(rng, struct rk_rng, rng);

	/* enable osc_ring to get entropy, sample period is set as 100 */
	reg_ctrl = CRYPTO_V1_OSC_ENABLE | CRYPTO_V1_TRNG_SAMPLE_PERIOD(100);
	rk_rng_writel(rk_rng, reg_ctrl, CRYPTO_V1_TRNG_CTRL);

	reg_ctrl = HIWORD_UPDATE(CRYPTO_V1_RNG_START, CRYPTO_V1_RNG_START, 0);

	rk_rng_writel(rk_rng, reg_ctrl, CRYPTO_V1_CTRL);

	ret = readl_poll_timeout(rk_rng->mem + CRYPTO_V1_CTRL, reg_ctrl,
				 !(reg_ctrl & CRYPTO_V1_RNG_START),
				 ROCKCHIP_POLL_TIMEOUT_US);
	if (ret < 0)
		goto out;

	ret = min_t(size_t, max, RK_MAX_RNG_BYTE);

	rk_rng_read_regs(rk_rng, CRYPTO_V1_TRNG_DOUT_0, buf, ret);

out:
	/* close TRNG */
	rk_rng_writel(rk_rng, HIWORD_UPDATE(0, CRYPTO_V1_RNG_START, 0),
		      CRYPTO_V1_CTRL);

	return ret;
}

static int rk_rng_v2_read(struct hwrng *rng, void *buf, size_t max, bool wait)
{
	int ret = 0;
	u32 reg_ctrl = 0;
	struct rk_rng *rk_rng = container_of(rng, struct rk_rng, rng);

	/* enable osc_ring to get entropy, sample period is set as 100 */
	rk_rng_writel(rk_rng, 100, CRYPTO_V2_RNG_SAMPLE_CNT);

	reg_ctrl |= CRYPTO_V2_RNG_256_BIT_LEN;
	reg_ctrl |= CRYPTO_V2_RNG_SLOWER_SOC_RING_0;
	reg_ctrl |= CRYPTO_V2_RNG_ENABLE;
	reg_ctrl |= CRYPTO_V2_RNG_START;

	rk_rng_writel(rk_rng, HIWORD_UPDATE(reg_ctrl, 0xffff, 0),
			CRYPTO_V2_RNG_CTL);

	ret = readl_poll_timeout(rk_rng->mem + CRYPTO_V2_RNG_CTL, reg_ctrl,
				 !(reg_ctrl & CRYPTO_V2_RNG_START),
				 ROCKCHIP_POLL_TIMEOUT_US);
	if (ret < 0)
		goto out;

	ret = min_t(size_t, max, RK_MAX_RNG_BYTE);

	rk_rng_read_regs(rk_rng, CRYPTO_V2_RNG_DOUT_0, buf, ret);

out:
	/* close TRNG */
	rk_rng_writel(rk_rng, HIWORD_UPDATE(0, 0xffff, 0), CRYPTO_V2_RNG_CTL);

	return ret;
}

static const struct rk_rng_soc_data rk_rng_rk3399_soc_data = {
	.clks_num = 3,
	.rk_rng_read = rk_rng_v1_read,
};

static const struct rk_rng_soc_data rk_rng_v1_soc_data = {
	.clks_num = 2,
	.rk_rng_read = rk_rng_v1_read,
};

static const struct rk_rng_soc_data rk_rng_v2_soc_data = {
	.clks_num = 2,
	.rk_rng_read = rk_rng_v2_read,
};

static const struct of_device_id rk_rng_dt_match[] = {
	{
		.compatible = "rockchip,rk3399-crypto",
		.data = &rk_rng_rk3399_soc_data,
	},
	{
		.compatible = "rockchip,cryptov1-rng",
		.data = &rk_rng_v1_soc_data,
	},
	{
		.compatible = "rockchip,rk3568-rng",
		.data = &rk_rng_v2_soc_data,
	},
	{ },
};

MODULE_DEVICE_TABLE(of, rk_rng_dt_match);

static int rk_rng_probe(struct device *dev)
{
	int ret;
	struct rk_rng *rk_rng;
	struct device_node *np = dev->of_node;
	const struct of_device_id *match;

	rk_rng = devm_kzalloc(dev, sizeof(struct rk_rng), GFP_KERNEL);
	if (!rk_rng)
		return -ENOMEM;

	match = of_match_node(rk_rng_dt_match, np);
	rk_rng->soc_data = (struct rk_rng_soc_data *)match->data;

	rk_rng->dev = dev;
	rk_rng->rng.name    = "rockchip";
	rk_rng->rng.init    = rk_rng_init;
	rk_rng->rng.read    = rk_rng->soc_data->rk_rng_read;

	rk_rng->clk_num = clk_bulk_get_all(dev, &rk_rng->clk_bulks);
	if (rk_rng->clk_num < rk_rng->soc_data->clks_num)
		return dev_err_probe(dev, -EINVAL,
				     "Missing clocks, got %d instead of %d\n",
				     rk_rng->clk_num, rk_rng->soc_data->clks_num);

	ret = device_reset_us(dev, 2);
	if (ret)
		return ret;

	rk_rng->mem = of_iomap(dev->device_node, 0);
	if (IS_ERR(rk_rng->mem))
		return PTR_ERR(rk_rng->mem);

	dev->priv = rk_rng;

	return hwrng_register(dev, &rk_rng->rng);
}

static void rk_rng_remove(struct device *dev)
{
	rk_rng_cleanup(dev->priv);
}

static struct driver rk_rng_driver = {
	.name	= "rockchip-rng",
	.of_match_table = rk_rng_dt_match,
	.probe	= rk_rng_probe,
	.remove	= rk_rng_remove,
};

device_platform_driver(rk_rng_driver);

MODULE_DESCRIPTION("ROCKCHIP H/W Random Number Generator driver");
MODULE_AUTHOR("Lin Jinhan <troy.lin@rock-chips.com>");
MODULE_LICENSE("GPL v2");
