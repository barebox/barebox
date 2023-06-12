// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2018, STMicroelectronics - All Rights Reserved
 * Author: Fabrice Gasnier <fabrice.gasnier@st.com>
 *
 * Originally based on the Linux kernel v4.18 drivers/iio/adc/stm32-adc.c.
 */

#include <common.h>
#include <linux/bitops.h>
#include <linux/iopoll.h>
#include <aiodev.h>
#include <regulator.h>
#include <linux/math64.h>
#include "stm32-adc-core.h"

/* STM32H7 - Registers for each ADC instance */
#define STM32H7_ADC_ISR			0x00
#define STM32H7_ADC_CR			0x08
#define STM32H7_ADC_CFGR		0x0C
#define STM32H7_ADC_SMPR1		0x14
#define STM32H7_ADC_SMPR2		0x18
#define STM32H7_ADC_PCSEL		0x1C
#define STM32H7_ADC_SQR1		0x30
#define STM32H7_ADC_DR			0x40
#define STM32H7_ADC_DIFSEL		0xC0

/* STM32H7_ADC_ISR - bit fields */
#define STM32MP1_VREGREADY		BIT(12)
#define STM32H7_EOC			BIT(2)
#define STM32H7_ADRDY			BIT(0)

/* STM32H7_ADC_CR - bit fields */
#define STM32H7_DEEPPWD			BIT(29)
#define STM32H7_ADVREGEN		BIT(28)
#define STM32H7_BOOST			BIT(8)
#define STM32H7_ADSTART			BIT(2)
#define STM32H7_ADDIS			BIT(1)
#define STM32H7_ADEN			BIT(0)

/* STM32H7_ADC_CFGR bit fields */
#define STM32H7_EXTEN			GENMASK(11, 10)
#define STM32H7_DMNGT			GENMASK(1, 0)

/* STM32H7_ADC_SQR1 - bit fields */
#define STM32H7_SQ1_SHIFT		6

/* BOOST bit must be set on STM32H7 when ADC clock is above 20MHz */
#define STM32H7_BOOST_CLKRATE		20000000UL

#define STM32_ADC_CH_MAX		20	/* max number of channels */
#define STM32_ADC_MAX_SMP		7	/* SMPx range is [0..7] */
#define STM32_ADC_TIMEOUT_US		100000

struct stm32_adc_regs {
	int reg;
	int mask;
	int shift;
};

struct stm32_adc_cfg {
	unsigned int max_channels;
	unsigned int num_bits;
	bool has_vregready;
	const struct stm32_adc_regs *smp_bits;
	const unsigned int *smp_cycles;
};

struct stm32_adc {
	struct stm32_adc_common *common;
	void __iomem *regs;
	const struct stm32_adc_cfg *cfg;
	u32 channel_mask;
	u32 data_mask;
	struct aiodevice aiodev;
	void __iomem *base;
	struct aiochannel *channels;
	u32 *channel_map;
	u32 smpr_val[2];
};

static void stm32_adc_stop(struct stm32_adc *adc)
{
	setbits_le32(adc->regs + STM32H7_ADC_CR, STM32H7_ADDIS);
	clrbits_le32(adc->regs + STM32H7_ADC_CR, STM32H7_BOOST);
	/* Setting DEEPPWD disables ADC vreg and clears ADVREGEN */
	setbits_le32(adc->regs + STM32H7_ADC_CR, STM32H7_DEEPPWD);

	regulator_disable(adc->common->vref);
}

static int stm32_adc_start_channel(struct stm32_adc *adc, int channel)
{
	struct device *dev = adc->aiodev.hwdev;
	struct stm32_adc_common *common = adc->common;
	int ret;
	u32 val;

	ret = regulator_enable(common->vref);
	if (ret)
		return ret;

	/* Exit deep power down, then enable ADC voltage regulator */
	clrbits_le32(adc->regs + STM32H7_ADC_CR, STM32H7_DEEPPWD);
	setbits_le32(adc->regs + STM32H7_ADC_CR, STM32H7_ADVREGEN);
	if (common->rate > STM32H7_BOOST_CLKRATE)
		setbits_le32(adc->regs + STM32H7_ADC_CR, STM32H7_BOOST);

	/* Wait for startup time */
	if (!adc->cfg->has_vregready) {
		udelay(20);
	} else {
		ret = readl_poll_timeout(adc->regs + STM32H7_ADC_ISR, val,
					 val & STM32MP1_VREGREADY,
					 STM32_ADC_TIMEOUT_US);
		if (ret < 0) {
			stm32_adc_stop(adc);
			dev_err(dev, "Failed to enable vreg: %d\n", ret);
			return ret;
		}
	}

	/* Only use single ended channels */
	writel(0, adc->regs + STM32H7_ADC_DIFSEL);

	/* Enable ADC, Poll for ADRDY to be set (after adc startup time) */
	setbits_le32(adc->regs + STM32H7_ADC_CR, STM32H7_ADEN);
	ret = readl_poll_timeout(adc->regs + STM32H7_ADC_ISR, val,
				 val & STM32H7_ADRDY, STM32_ADC_TIMEOUT_US);
	if (ret < 0) {
		stm32_adc_stop(adc);
		dev_err(dev, "Failed to enable ADC: %d\n", ret);
		return ret;
	}

	/* Preselect channels */
	writel(adc->channel_mask, adc->regs + STM32H7_ADC_PCSEL);

	/* Apply sampling time settings */
	writel(adc->smpr_val[0], adc->regs + STM32H7_ADC_SMPR1);
	writel(adc->smpr_val[1], adc->regs + STM32H7_ADC_SMPR2);

	/* Program regular sequence: chan in SQ1 & len = 0 for one channel */
	writel(channel << STM32H7_SQ1_SHIFT, adc->regs + STM32H7_ADC_SQR1);

	/* Trigger detection disabled (conversion can be launched in SW) */
	clrbits_le32(adc->regs + STM32H7_ADC_CFGR, STM32H7_EXTEN |
		     STM32H7_DMNGT);

	return 0;
}

static int stm32_adc_channel_data(struct stm32_adc *adc, int channel,
				  int *data)
{
	struct device *dev = &adc->aiodev.dev;
	int ret;
	u32 val;

	setbits_le32(adc->regs + STM32H7_ADC_CR, STM32H7_ADSTART);
	ret = readl_poll_timeout(adc->regs + STM32H7_ADC_ISR, val,
				 val & STM32H7_EOC, STM32_ADC_TIMEOUT_US);
	if (ret < 0) {
		dev_err(dev, "conversion timed out: %d\n", ret);
		return ret;
	}

	*data = readl(adc->regs + STM32H7_ADC_DR);

	return 0;
}

static int stm32_adc_channel_single_shot(struct aiochannel *chan, int *data)
{
	struct stm32_adc *adc = container_of(chan->aiodev, struct stm32_adc, aiodev);
	int ret, index;
	s64 raw64;

	index = adc->channel_map[chan->index];

	ret = stm32_adc_start_channel(adc, index);
	if (ret)
		return ret;

	ret = stm32_adc_channel_data(adc, index, data);
	if (ret)
		return ret;

	raw64 = *data;
	raw64 *= adc->common->vref_uv;
	*data = div_s64(raw64, adc->data_mask);

	return 0;
}

static void stm32_adc_smpr_init(struct stm32_adc *adc, int channel, u32 smp_ns)
{
	const struct stm32_adc_regs *smpr = &adc->cfg->smp_bits[channel];
	u32 period_ns, shift = smpr->shift, mask = smpr->mask;
	unsigned int smp, r = smpr->reg;

	/* Determine sampling time (ADC clock cycles) */
	period_ns = NSEC_PER_SEC / adc->common->rate;
	for (smp = 0; smp <= STM32_ADC_MAX_SMP; smp++)
		if ((period_ns * adc->cfg->smp_cycles[smp]) >= smp_ns)
			break;
	if (smp > STM32_ADC_MAX_SMP)
		smp = STM32_ADC_MAX_SMP;

	/* pre-build sampling time registers (e.g. smpr1, smpr2) */
	adc->smpr_val[r] = (adc->smpr_val[r] & ~mask) | (smp << shift);
}

static int stm32_adc_chan_of_init(struct device *dev, struct stm32_adc *adc)
{
	unsigned int i;
	int num_channels = 0, num_times = 0;
	u32 smp = 0xffffffff; /* Set sampling time to max value by default */
	int ret;

	/* Retrieve single ended channels listed in device tree */
	of_get_property(dev->of_node, "st,adc-channels", &num_channels);
	num_channels /= sizeof(__be32);

	if (num_channels > adc->cfg->max_channels) {
		dev_err(dev, "too many st,adc-channels: %d\n", num_channels);
		return -EINVAL;
	}

	/* Optional sample time is provided either for each, or all channels */
	of_get_property(dev->of_node, "st,min-sample-time-nsecs", &num_times);
	num_times /= sizeof(__be32);
	if (num_times > 1 && num_times != num_channels) {
		dev_err(dev, "Invalid st,min-sample-time-nsecs\n");
		return -EINVAL;
	}

	adc->channels = calloc(sizeof(*adc->channels), num_channels);
	if (!adc->channels)
		return -ENOMEM;

	adc->aiodev.channels = calloc(sizeof(*adc->aiodev.channels), num_channels);
	if (!adc->aiodev.channels)
		return -ENOMEM;

	adc->channel_map = calloc(sizeof(u32), num_channels);

	adc->aiodev.num_channels = num_channels;
	adc->aiodev.hwdev = dev;
	adc->aiodev.read = stm32_adc_channel_single_shot;

	for (i = 0; i < num_channels; i++) {
		u32 chan;

		ret = of_property_read_u32_index(dev->of_node,
						 "st,adc-channels", i, &chan);
		if (ret)
			return ret;

		if (chan >= adc->cfg->max_channels) {
			dev_err(dev, "bad channel %u\n", chan);
			return -EINVAL;
		}

		adc->channel_mask |= 1 << chan;

		adc->aiodev.channels[i] = &adc->channels[i];
		adc->channels[i].unit = "uV";
		adc->channel_map[i] = chan;

		/*
		 * Using of_property_read_u32_index(), smp value will only be
		 * modified if valid u32 value can be decoded. This allows to
		 * get either no value, 1 shared value for all indexes, or one
		 * value per channel.
		 */
		of_property_read_u32_index(dev->of_node,
					   "st,min-sample-time-nsecs",
					   i, &smp);
		/* Prepare sampling time settings */
		stm32_adc_smpr_init(adc, chan, smp);
	}

	adc->data_mask = (1 << adc->cfg->num_bits) - 1;

	ret = aiodevice_register(&adc->aiodev);
	if (ret < 0)
		dev_err(dev, "Failed to register aiodev\n");

	return ret;
}

static int stm32_adc_probe(struct device *dev)
{
	struct stm32_adc_common *common = dev->parent->priv;
	struct stm32_adc *adc;
	u32 offset;
	int ret;

	ret = of_property_read_u32(dev->of_node, "reg", &offset);
	if (ret) {
		dev_err(dev, "Can't read reg property\n");
		return ret;
	}

	adc = xzalloc(sizeof(*adc));

	adc->regs = common->base + offset;
	adc->cfg = device_get_match_data(dev);
	adc->common = common;

	return stm32_adc_chan_of_init(dev, adc);
}

/*
 * stm32h7_smp_bits - describe sampling time register index & bit fields
 * Sorted so it can be indexed by channel number.
 */
static const struct stm32_adc_regs stm32h7_smp_bits[] = {
	/* STM32H7_ADC_SMPR1, smpr[] index, mask, shift for SMP0 to SMP9 */
	{ 0, GENMASK(2, 0), 0 },
	{ 0, GENMASK(5, 3), 3 },
	{ 0, GENMASK(8, 6), 6 },
	{ 0, GENMASK(11, 9), 9 },
	{ 0, GENMASK(14, 12), 12 },
	{ 0, GENMASK(17, 15), 15 },
	{ 0, GENMASK(20, 18), 18 },
	{ 0, GENMASK(23, 21), 21 },
	{ 0, GENMASK(26, 24), 24 },
	{ 0, GENMASK(29, 27), 27 },
	/* STM32H7_ADC_SMPR2, smpr[] index, mask, shift for SMP10 to SMP19 */
	{ 1, GENMASK(2, 0), 0 },
	{ 1, GENMASK(5, 3), 3 },
	{ 1, GENMASK(8, 6), 6 },
	{ 1, GENMASK(11, 9), 9 },
	{ 1, GENMASK(14, 12), 12 },
	{ 1, GENMASK(17, 15), 15 },
	{ 1, GENMASK(20, 18), 18 },
	{ 1, GENMASK(23, 21), 21 },
	{ 1, GENMASK(26, 24), 24 },
	{ 1, GENMASK(29, 27), 27 },
};

/* STM32H7 programmable sampling time (ADC clock cycles, rounded down) */
static const unsigned int stm32h7_adc_smp_cycles[STM32_ADC_MAX_SMP + 1] = {
	1, 2, 8, 16, 32, 64, 387, 810,
};

static const struct stm32_adc_cfg stm32h7_adc_cfg = {
	.num_bits = 16,
	.max_channels = STM32_ADC_CH_MAX,
	.smp_bits = stm32h7_smp_bits,
	.smp_cycles = stm32h7_adc_smp_cycles,
};


static const struct stm32_adc_cfg stm32mp1_adc_cfg = {
	.num_bits = 16,
	.max_channels = STM32_ADC_CH_MAX,
	.smp_bits = stm32h7_smp_bits,
	.smp_cycles = stm32h7_adc_smp_cycles,
	.has_vregready = true,
};

static const struct of_device_id stm32_adc_match[] = {
	{ .compatible = "st,stm32h7-adc", .data = &stm32h7_adc_cfg },
	{ .compatible = "st,stm32mp1-adc", .data = &stm32mp1_adc_cfg },
	{}
};
MODULE_DEVICE_TABLE(of, stm32_adc_match);

static struct driver stm32_adc_driver = {
	.name		= "stm32-adc",
	.probe		= stm32_adc_probe,
	.of_compatible	= DRV_OF_COMPAT(stm32_adc_match),
};
device_platform_driver(stm32_adc_driver);
