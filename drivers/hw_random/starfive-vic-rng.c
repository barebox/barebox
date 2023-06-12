// SPDX-License-Identifier: GPL-2.0-only
/*
 * COPYRIGHT 2020 Shanghai StarFive Technology Co., Ltd.
 */
#include <common.h>
#include <linux/err.h>
#include <io.h>
#include <of.h>
#include <driver.h>
#include <linux/hw_random.h>
#include <linux/reset.h>
#include <linux/clk.h>

#define VIC_RAND_LEN 4

#define VIC_CTRL		0x00
#define VIC_MODE		0x04
#define VIC_SMODE		0x08
#define VIC_STAT		0x0C
#define VIC_IE			0x10
#define VIC_ISTAT		0x14
#define VIC_ALARM		0x18
#define VIC_BUILD_ID		0x1C
#define VIC_FEATURES		0x20
#define VIC_RAND0		0x24
#define VIC_NPA_DATA0		0x34
#define VIC_SEED0		0x74
#define VIC_IA_RDATA		0xA4
#define VIC_IA_WDATA		0xA8
#define VIC_IA_ADDR		0xAC
#define VIC_IA_CMD		0xB0

/* CTRL */
#define VIC_CTRL_CMD_NOP		0
#define VIC_CTRL_CMD_GEN_NOISE		1
#define VIC_CTRL_CMD_GEN_NONCE		2
#define VIC_CTRL_CMD_CREATE_STATE	3
#define VIC_CTRL_CMD_RENEW_STATE	4
#define VIC_CTRL_CMD_REFRESH_ADDIN	5
#define VIC_CTRL_CMD_GEN_RANDOM		6
#define VIC_CTRL_CMD_ADVANCE_STATE	7
#define VIC_CTRL_CMD_KAT		8
#define VIC_CTRL_CMD_ZEROIZE		15

/* SMODE */
#define _VIC_SMODE_SECURE_EN	1

#define VIC_SMODE_SECURE_EN(x)		((x) << _VIC_SMODE_SECURE_EN)

/* STAT */
#define _VIC_STAT_BUSY		31

#define VIC_STAT_BUSY		(1UL << _VIC_STAT_BUSY)

/* IE */
#define _VIC_IE_GLBL		31
#define _VIC_IE_DONE		4
#define _VIC_IE_ALARMS		3
#define _VIC_IE_NOISE_RDY	2
#define _VIC_IE_KAT_COMPLETE	1
#define _VIC_IE_ZEROIZE		0

#define VIC_IE_GLBL		(1UL << _VIC_IE_GLBL)
#define VIC_IE_DONE		(1UL << _VIC_IE_DONE)
#define VIC_IE_ALARMS		(1UL << _VIC_IE_ALARMS)
#define VIC_IE_NOISE_RDY	(1UL << _VIC_IE_NOISE_RDY)
#define VIC_IE_KAT_COMPLETE	(1UL << _VIC_IE_KAT_COMPLETE)
#define VIC_IE_ZEROIZE		(1UL << _VIC_IE_ZEROIZE)
#define VIC_IE_ALL		(VIC_IE_GLBL | VIC_IE_DONE | VIC_IE_ALARMS | \
				 VIC_IE_NOISE_RDY | VIC_IE_KAT_COMPLETE | VIC_IE_ZEROIZE)

#define to_vic_rng(p)	container_of(p, struct vic_rng, rng)

struct vic_rng {
	struct device	*dev;
	void __iomem	*base;
	struct hwrng	rng;
};

static inline void vic_wait_till_idle(struct vic_rng *hrng)
{
	while(readl(hrng->base + VIC_STAT) & VIC_STAT_BUSY)
		;
}

static inline void vic_rng_irq_mask_clear(struct vic_rng *hrng)
{
	u32 data = readl(hrng->base + VIC_ISTAT);
	writel(data, hrng->base + VIC_ISTAT);
	writel(0, hrng->base + VIC_ALARM);
}

static int vic_trng_cmd(struct vic_rng *hrng, u32 cmd)
{
	vic_wait_till_idle(hrng);

	switch (cmd) {
	case VIC_CTRL_CMD_NOP:
	case VIC_CTRL_CMD_GEN_NOISE:
	case VIC_CTRL_CMD_GEN_NONCE:
	case VIC_CTRL_CMD_CREATE_STATE:
	case VIC_CTRL_CMD_RENEW_STATE:
	case VIC_CTRL_CMD_REFRESH_ADDIN:
	case VIC_CTRL_CMD_GEN_RANDOM:
	case VIC_CTRL_CMD_ADVANCE_STATE:
	case VIC_CTRL_CMD_KAT:
	case VIC_CTRL_CMD_ZEROIZE:
		writel(cmd, hrng->base + VIC_CTRL);
		return 0;
	default:
		return -EINVAL;
	}
}

static int vic_rng_init(struct hwrng *rng)
{
	struct vic_rng *hrng = to_vic_rng(rng);
	struct clk *clk;
	int ret;

	clk = clk_get(rng->dev, NULL);
	if (IS_ERR(clk))
		return PTR_ERR(clk);

	clk_enable(clk);

	ret = device_reset(rng->dev);
	if (ret)
		return ret;

	// clear register: ISTAT
	vic_rng_irq_mask_clear(hrng);

	// set mission mode
	writel(VIC_SMODE_SECURE_EN(1), hrng->base + VIC_SMODE);

	vic_trng_cmd(hrng, VIC_CTRL_CMD_GEN_NOISE);
	vic_wait_till_idle(hrng);

	// set interrupt
	writel(VIC_IE_ALL, hrng->base + VIC_IE);

	// zeroize
	vic_trng_cmd(hrng, VIC_CTRL_CMD_ZEROIZE);

	vic_wait_till_idle(hrng);

	return 0;
}

static int vic_rng_read(struct hwrng *rng, void *buf, size_t max, bool wait)
{
	struct vic_rng *hrng = to_vic_rng(rng);

	vic_trng_cmd(hrng, VIC_CTRL_CMD_ZEROIZE);
	vic_trng_cmd(hrng, VIC_CTRL_CMD_GEN_NOISE);
	vic_trng_cmd(hrng, VIC_CTRL_CMD_CREATE_STATE);

	vic_wait_till_idle(hrng);
	max = min_t(size_t, max, VIC_RAND_LEN * 4);

	writel(0x0, hrng->base + VIC_MODE);
	vic_trng_cmd(hrng, VIC_CTRL_CMD_GEN_RANDOM);

	vic_wait_till_idle(hrng);
	memcpy_fromio(buf, hrng->base + VIC_RAND0, max);
	vic_trng_cmd(hrng, VIC_CTRL_CMD_ZEROIZE);

	vic_wait_till_idle(hrng);
	return max;
}

static int vic_rng_probe(struct device *dev)
{
	struct vic_rng *hrng;
	struct resource *res;

	hrng = xzalloc(sizeof(*hrng));

	res = dev_request_mem_resource(dev, 0);
	if (IS_ERR(res))
		return PTR_ERR(res);

	hrng->base = IOMEM(res->start);
	hrng->dev = dev;

	hrng->rng.name = dev_name(dev);
	hrng->rng.init = vic_rng_init;
	hrng->rng.read = vic_rng_read;

	return hwrng_register(dev, &hrng->rng);
}

static const struct of_device_id vic_rng_dt_ids[] = {
	{ .compatible = "starfive,vic-rng" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, vic_rng_dt_ids);

static struct driver vic_rng_driver = {
	.name		= "vic-rng",
	.probe		= vic_rng_probe,
	.of_compatible	= vic_rng_dt_ids,
};
device_platform_driver(vic_rng_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Huan Feng <huan.feng@starfivetech.com>");
MODULE_DESCRIPTION("Starfive VIC random number generator driver");
