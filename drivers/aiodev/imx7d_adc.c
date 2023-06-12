// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Freescale i.MX7D ADC driver
 *
 * Copyright (C) 2015 Freescale Semiconductor, Inc.
 */

#include <linux/clk.h>
#include <linux/err.h>
#include <io.h>
#include <linux/printk.h>
#include <driver.h>
#include <init.h>
#include <linux/kernel.h>
#include <linux/bitops.h>
#include <regulator.h>
#include <linux/barebox-wrapper.h>

#include <linux/iopoll.h>
#include <aiodev.h>

/* ADC register */
#define IMX7D_REG_ADC_TIMER_UNIT		0x90
#define IMX7D_REG_ADC_INT_STATUS		0xe0
#define IMX7D_REG_ADC_CHA_B_CNV_RSLT		0xf0
#define IMX7D_REG_ADC_CHC_D_CNV_RSLT		0x100
#define IMX7D_REG_ADC_ADC_CFG			0x130

#define IMX7D_REG_ADC_CFG1(ch)			((ch) * 0x20)
#define IMX7D_REG_ADC_CFG2(ch)			((ch) * 0x20 + 0x10)

#define IMX7D_REG_ADC_CH_CFG1_CHANNEL_EN			(0x1 << 31)
#define IMX7D_REG_ADC_CH_CFG1_CHANNEL_SINGLE			BIT(30)
#define IMX7D_REG_ADC_CH_CFG1_CHANNEL_AVG_EN			BIT(29)
#define IMX7D_REG_ADC_CH_CFG1_CHANNEL_SEL(x)			((x) << 24)

#define IMX7D_REG_ADC_CH_CFG2_AVG_NUM_4				(0x0 << 12)
#define IMX7D_REG_ADC_CH_CFG2_AVG_NUM_8				(0x1 << 12)
#define IMX7D_REG_ADC_CH_CFG2_AVG_NUM_16			(0x2 << 12)
#define IMX7D_REG_ADC_CH_CFG2_AVG_NUM_32			(0x3 << 12)

#define IMX7D_REG_ADC_TIMER_UNIT_PRE_DIV_4			(0x0 << 29)
#define IMX7D_REG_ADC_TIMER_UNIT_PRE_DIV_8			(0x1 << 29)
#define IMX7D_REG_ADC_TIMER_UNIT_PRE_DIV_16			(0x2 << 29)
#define IMX7D_REG_ADC_TIMER_UNIT_PRE_DIV_32			(0x3 << 29)
#define IMX7D_REG_ADC_TIMER_UNIT_PRE_DIV_64			(0x4 << 29)
#define IMX7D_REG_ADC_TIMER_UNIT_PRE_DIV_128			(0x5 << 29)

#define IMX7D_REG_ADC_ADC_CFG_ADC_CLK_DOWN			BIT(31)
#define IMX7D_REG_ADC_ADC_CFG_ADC_POWER_DOWN			BIT(1)
#define IMX7D_REG_ADC_ADC_CFG_ADC_EN				BIT(0)

#define IMX7D_REG_ADC_INT_STATUS_CHANNEL_INT_STATUS		0xf00
#define IMX7D_REG_ADC_INT_STATUS_CHANNEL_CONV_TIME_OUT		0xf0000

#define IMX7D_ADC_TIMEOUT_NSEC		(100 * NSEC_PER_MSEC)
#define IMX7D_ADC_INPUT_CLK		24000000

enum imx7d_adc_clk_pre_div {
	IMX7D_ADC_ANALOG_CLK_PRE_DIV_4,
	IMX7D_ADC_ANALOG_CLK_PRE_DIV_8,
	IMX7D_ADC_ANALOG_CLK_PRE_DIV_16,
	IMX7D_ADC_ANALOG_CLK_PRE_DIV_32,
	IMX7D_ADC_ANALOG_CLK_PRE_DIV_64,
	IMX7D_ADC_ANALOG_CLK_PRE_DIV_128,
};

enum imx7d_adc_average_num {
	IMX7D_ADC_AVERAGE_NUM_4,
	IMX7D_ADC_AVERAGE_NUM_8,
	IMX7D_ADC_AVERAGE_NUM_16,
	IMX7D_ADC_AVERAGE_NUM_32,
};

struct imx7d_adc_feature {
	enum imx7d_adc_clk_pre_div clk_pre_div;
	enum imx7d_adc_average_num avg_num;

	u32 core_time_unit;	/* impact the sample rate */
};

struct imx7d_adc {
	struct device *dev;
	void __iomem *regs;
	struct clk *clk;
	struct aiodevice aiodev;
	void (*aiodev_info)(struct device *);

	u32 vref_uv;
	u32 pre_div_num;

	struct regulator *vref;
	struct imx7d_adc_feature adc_feature;

	struct aiochannel aiochan[16];
};

struct imx7d_adc_analogue_core_clk {
	u32 pre_div;
	u32 reg_config;
};

#define IMX7D_ADC_ANALOGUE_CLK_CONFIG(_pre_div, _reg_conf) {	\
	.pre_div = (_pre_div),					\
	.reg_config = (_reg_conf),				\
}

static const struct imx7d_adc_analogue_core_clk imx7d_adc_analogue_clk[] = {
	IMX7D_ADC_ANALOGUE_CLK_CONFIG(4, IMX7D_REG_ADC_TIMER_UNIT_PRE_DIV_4),
	IMX7D_ADC_ANALOGUE_CLK_CONFIG(8, IMX7D_REG_ADC_TIMER_UNIT_PRE_DIV_8),
	IMX7D_ADC_ANALOGUE_CLK_CONFIG(16, IMX7D_REG_ADC_TIMER_UNIT_PRE_DIV_16),
	IMX7D_ADC_ANALOGUE_CLK_CONFIG(32, IMX7D_REG_ADC_TIMER_UNIT_PRE_DIV_32),
	IMX7D_ADC_ANALOGUE_CLK_CONFIG(64, IMX7D_REG_ADC_TIMER_UNIT_PRE_DIV_64),
	IMX7D_ADC_ANALOGUE_CLK_CONFIG(128, IMX7D_REG_ADC_TIMER_UNIT_PRE_DIV_128),
};

static const u32 imx7d_adc_average_num[] = {
	IMX7D_REG_ADC_CH_CFG2_AVG_NUM_4,
	IMX7D_REG_ADC_CH_CFG2_AVG_NUM_8,
	IMX7D_REG_ADC_CH_CFG2_AVG_NUM_16,
	IMX7D_REG_ADC_CH_CFG2_AVG_NUM_32,
};

static void imx7d_adc_feature_config(struct imx7d_adc *info)
{
	info->adc_feature.clk_pre_div = IMX7D_ADC_ANALOG_CLK_PRE_DIV_4;
	info->adc_feature.avg_num = IMX7D_ADC_AVERAGE_NUM_32;
	info->adc_feature.core_time_unit = 1;
}

static void imx7d_adc_sample_rate_set(struct imx7d_adc *info)
{
	struct imx7d_adc_feature *adc_feature = &info->adc_feature;
	struct imx7d_adc_analogue_core_clk adc_analogue_clk;
	unsigned i;
	u32 tmp_cfg1;
	u32 sample_rate = 0;

	/*
	 * Before sample set, disable channel A,B,C,D. Here we
	 * clear the bit 31 of register REG_ADC_CH_A\B\C\D_CFG1.
	 */
	for (i = 0; i < 4; i++) {
		tmp_cfg1 = readl(info->regs + IMX7D_REG_ADC_CFG1(i));
		tmp_cfg1 &= ~IMX7D_REG_ADC_CH_CFG1_CHANNEL_EN;
		writel(tmp_cfg1, info->regs + IMX7D_REG_ADC_CFG1(i));
	}

	adc_analogue_clk = imx7d_adc_analogue_clk[adc_feature->clk_pre_div];
	sample_rate |= adc_analogue_clk.reg_config;
	info->pre_div_num = adc_analogue_clk.pre_div;

	sample_rate |= adc_feature->core_time_unit;
	writel(sample_rate, info->regs + IMX7D_REG_ADC_TIMER_UNIT);
}

static void imx7d_adc_hw_init(struct imx7d_adc *info)
{
	u32 cfg;

	/* power up and enable adc analogue core */
	cfg = readl(info->regs + IMX7D_REG_ADC_ADC_CFG);
	cfg &= ~(IMX7D_REG_ADC_ADC_CFG_ADC_CLK_DOWN |
		 IMX7D_REG_ADC_ADC_CFG_ADC_POWER_DOWN);
	cfg |= IMX7D_REG_ADC_ADC_CFG_ADC_EN;
	writel(cfg, info->regs + IMX7D_REG_ADC_ADC_CFG);

	imx7d_adc_sample_rate_set(info);
}

static void imx7d_adc_channel_set(struct imx7d_adc *info, u32 channel)
{
	u32 cfg1 = 0;
	u32 cfg2;

	/* the channel choose single conversion, and enable average mode */
	cfg1 |= (IMX7D_REG_ADC_CH_CFG1_CHANNEL_EN |
		 IMX7D_REG_ADC_CH_CFG1_CHANNEL_SINGLE |
		 IMX7D_REG_ADC_CH_CFG1_CHANNEL_AVG_EN);

	/*
	 * physical channel 0 chose logical channel A
	 * physical channel 1 chose logical channel B
	 * physical channel 2 chose logical channel C
	 * physical channel 3 chose logical channel D
	 */
	cfg1 |= IMX7D_REG_ADC_CH_CFG1_CHANNEL_SEL(channel);

	/*
	 * read register REG_ADC_CH_A\B\C\D_CFG2, according to the
	 * channel chosen
	 */
	cfg2 = readl(info->regs + IMX7D_REG_ADC_CFG2(channel));

	cfg2 |= imx7d_adc_average_num[info->adc_feature.avg_num];

	/*
	 * write the register REG_ADC_CH_A\B\C\D_CFG2, according to
	 * the channel chosen
	 */
	writel(cfg2, info->regs + IMX7D_REG_ADC_CFG2(channel));
	writel(cfg1, info->regs + IMX7D_REG_ADC_CFG1(channel));
}

static u16 __imx7d_adc_read_data(struct imx7d_adc *info, u32 channel)
{
	u32 value;

	/*
	 * channel A and B conversion result share one register,
	 * bit[27~16] is the channel B conversion result,
	 * bit[11~0] is the channel A conversion result.
	 * channel C and D is the same.
	 */
	if (channel < 2)
		value = readl(info->regs + IMX7D_REG_ADC_CHA_B_CNV_RSLT);
	else
		value = readl(info->regs + IMX7D_REG_ADC_CHC_D_CNV_RSLT);
	if (channel & 0x1)	/* channel B or D */
		value = (value >> 16) & 0xFFF;
	else			/* channel A or C */
		value &= 0xFFF;

	return value;
}

static int imx7d_adc_read_data(struct imx7d_adc *info, u32 channel)
{
	int ret = -EAGAIN;
	int status;

	status = readl(info->regs + IMX7D_REG_ADC_INT_STATUS);
	if (status & IMX7D_REG_ADC_INT_STATUS_CHANNEL_INT_STATUS) {
		ret = __imx7d_adc_read_data(info, channel);

		/*
		 * The register IMX7D_REG_ADC_INT_STATUS can't clear
		 * itself after read operation, need software to write
		 * 0 to the related bit. Here we clear the channel A/B/C/D
		 * conversion finished flag.
		 */
		status &= ~IMX7D_REG_ADC_INT_STATUS_CHANNEL_INT_STATUS;
		writel(status, info->regs + IMX7D_REG_ADC_INT_STATUS);
	}

	/*
	 * If the channel A/B/C/D conversion timeout, report it and clear these
	 * timeout flags.
	 */
	if (status & IMX7D_REG_ADC_INT_STATUS_CHANNEL_CONV_TIME_OUT) {
		dev_err(info->dev,
			"ADC got conversion time out interrupt: 0x%08x\n",
			status);
		status &= ~IMX7D_REG_ADC_INT_STATUS_CHANNEL_CONV_TIME_OUT;
		writel(status, info->regs + IMX7D_REG_ADC_INT_STATUS);
		ret = -ETIMEDOUT;
	}

	return ret;
}


static int imx7d_adc_read_raw(struct aiochannel *chan, int *data)
{
	struct imx7d_adc *info = container_of(chan->aiodev, struct imx7d_adc, aiodev);
	u64 raw64, start;
	u32 channel;
	int ret;

	channel = chan->index & 0x03;
	imx7d_adc_channel_set(info, channel);

	start = get_time_ns();
	do {
		if (is_timeout(start, IMX7D_ADC_TIMEOUT_NSEC)) {
			ret = -ETIMEDOUT;
			break;
		}

		ret = imx7d_adc_read_data(info, channel);
	} while (ret == -EAGAIN);

	if (ret < 0)
		return ret;

	raw64 = ret;
	raw64 *= info->vref_uv;
	raw64 = div_u64(raw64, 1000);
	*data = div_u64(raw64, (1 << 12));

	return 0;
}

static const struct of_device_id imx7d_adc_match[] = {
	{ .compatible = "fsl,imx7d-adc", },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, imx7d_adc_match);

static void imx7d_adc_power_down(struct imx7d_adc *info)
{
	u32 adc_cfg;

	adc_cfg = readl(info->regs + IMX7D_REG_ADC_ADC_CFG);
	adc_cfg |= IMX7D_REG_ADC_ADC_CFG_ADC_CLK_DOWN |
		   IMX7D_REG_ADC_ADC_CFG_ADC_POWER_DOWN;
	adc_cfg &= ~IMX7D_REG_ADC_ADC_CFG_ADC_EN;
	writel(adc_cfg, info->regs + IMX7D_REG_ADC_ADC_CFG);
}

static int imx7d_adc_enable(struct imx7d_adc *info)
{
	struct device *dev = info->dev;
	int ret;

	ret = regulator_enable(info->vref);
	if (ret)
		return dev_err_probe(dev, ret,
				     "Can't enable adc reference top voltage\n");

	ret = clk_enable(info->clk);
	if (ret) {
		regulator_disable(info->vref);
		return dev_err_probe(dev, ret, "Could not enable clock.\n");
	}

	imx7d_adc_hw_init(info);

	ret = regulator_get_voltage(info->vref);
	if (ret < 0)
		return dev_err_probe(dev, ret, "can't get vref-supply value\n");

	info->vref_uv = ret;
	return 0;
}

static u32 imx7d_adc_get_sample_rate(struct imx7d_adc *info)
{
	u32 analogue_core_clk;
	u32 core_time_unit = info->adc_feature.core_time_unit;
	u32 tmp;

	analogue_core_clk = IMX7D_ADC_INPUT_CLK / info->pre_div_num;
	tmp = (core_time_unit + 1) * 6;

	return analogue_core_clk / tmp;
}

static void imx7d_adc_devinfo(struct device *dev)
{
	struct imx7d_adc *info = dev->parent->priv;

	if (info->aiodev_info)
		info->aiodev_info(dev);

	printf("Sample Rate: %u\n", imx7d_adc_get_sample_rate(info));
}

static int imx7d_adc_probe(struct device *dev)
{
	struct aiodevice *aiodev;
	struct imx7d_adc *info;
	int ret, i;

	info = xzalloc(sizeof(*info));

	info->dev = dev;

	info->clk = clk_get(dev, "adc");
	if (IS_ERR(info->clk))
		return dev_err_probe(dev, PTR_ERR(info->clk), "Failed getting clock\n");

	info->vref = regulator_get(dev, "vref");
	if (IS_ERR(info->vref))
		return dev_err_probe(dev, PTR_ERR(info->vref),
				     "Failed getting reference voltage\n");

	info->regs = dev_request_mem_region(dev, 0);
	if (IS_ERR(info->regs))
		return dev_err_probe(dev, PTR_ERR(info->regs),
				     "Failed to get memory region\n");

	dev->priv = info;
	aiodev = &info->aiodev;

	aiodev->num_channels = 4;
	aiodev->hwdev = dev;
	aiodev->read = imx7d_adc_read_raw;
	aiodev->channels = xzalloc(aiodev->num_channels * sizeof(aiodev->channels[0]));

	for (i = 0; i < aiodev->num_channels; i++) {
		aiodev->channels[i] = &info->aiochan[i];
		info->aiochan[i].unit = "mV";
	}

	imx7d_adc_feature_config(info);

	ret = imx7d_adc_enable(info);
	if (ret)
		return ret;

	ret = aiodevice_register(aiodev);
	if (ret < 0)
		return dev_err_probe(dev, ret, "Failed to register aiodev\n");

	info->aiodev_info = aiodev->dev.info;
	aiodev->dev.info = imx7d_adc_devinfo;

	return 0;
}

static void imx7d_adc_disable(struct device *dev)
{
	struct imx7d_adc *info = dev->priv;

	imx7d_adc_power_down(info);

	clk_disable(info->clk);
	regulator_disable(info->vref);
}

static struct driver imx7d_adc_driver = {
	.probe		= imx7d_adc_probe,
	.name		= "imx7d_adc",
	.of_compatible	= imx7d_adc_match,
	.remove		= imx7d_adc_disable,
};
device_platform_driver(imx7d_adc_driver);

MODULE_AUTHOR("Haibo Chen <haibo.chen@freescale.com>");
MODULE_DESCRIPTION("Freescale IMX7D ADC driver");
MODULE_LICENSE("GPL v2");
