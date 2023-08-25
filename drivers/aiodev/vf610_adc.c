// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Freescale Vybrid vf610 ADC driver
 *
 * Copyright 2013 Freescale Semiconductor, Inc.
 */

#include <clock.h>
#include <io.h>
#include <linux/printk.h>
#include <driver.h>
#include <init.h>

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <regulator.h>
#include <linux/err.h>

#include <aiodev.h>

/* This will be the driver name the kernel reports */
#define DRIVER_NAME "vf610-adc"

/* Vybrid/IMX ADC registers */
#define VF610_REG_ADC_HC0		0x00
#define VF610_REG_ADC_HS		0x08
#define VF610_REG_ADC_R0		0x0c
#define VF610_REG_ADC_CFG		0x14
#define VF610_REG_ADC_GC		0x18
#define VF610_REG_ADC_GS		0x1c

/* Configuration register field define */
#define VF610_ADC_MODE_BIT8		0x00
#define VF610_ADC_MODE_BIT10		0x04
#define VF610_ADC_MODE_BIT12		0x08
#define VF610_ADC_MODE_MASK		0x0c
#define VF610_ADC_BUSCLK2_SEL		0x01
#define VF610_ADC_ALTCLK_SEL		0x02
#define VF610_ADC_ADACK_SEL		0x03
#define VF610_ADC_ADCCLK_MASK		0x03
#define VF610_ADC_CLK_DIV2		0x20
#define VF610_ADC_CLK_DIV4		0x40
#define VF610_ADC_CLK_DIV8		0x60
#define VF610_ADC_CLK_MASK		0x60
#define VF610_ADC_ADLSMP_LONG		0x10
#define VF610_ADC_ADSTS_SHORT   0x100
#define VF610_ADC_ADSTS_NORMAL  0x200
#define VF610_ADC_ADSTS_LONG    0x300
#define VF610_ADC_ADSTS_MASK		0x300
#define VF610_ADC_ADLPC_EN		0x80
#define VF610_ADC_ADHSC_EN		0x400
#define VF610_ADC_REFSEL_VALT		0x800
#define VF610_ADC_REFSEL_VBG		0x1000
#define VF610_ADC_AVGS_8		0x4000
#define VF610_ADC_AVGS_16		0x8000
#define VF610_ADC_AVGS_32		0xC000
#define VF610_ADC_AVGS_MASK		0xC000
#define VF610_ADC_OVWREN		0x10000

/* General control register field define */
#define VF610_ADC_AVGEN			0x20
#define VF610_ADC_CAL			0x80

/* Other field define */
#define VF610_ADC_ADCHC(x)		((x) & 0x1F)
#define VF610_ADC_CONV_DISABLE		0x1F
#define VF610_ADC_HS_COCO0		0x1
#define VF610_ADC_CALF			0x2
#define VF610_ADC_TIMEOUT_NSEC	(100 * NSEC_PER_MSEC)

#define DEFAULT_SAMPLE_TIME		1000

enum clk_sel {
	VF610_ADCIOC_BUSCLK_SET,
	VF610_ADCIOC_ALTCLK_SET,
	VF610_ADCIOC_ADACK_SET,
};

enum vol_ref {
	VF610_ADCIOC_VR_VREF_SET,
	VF610_ADCIOC_VR_VALT_SET,
	VF610_ADCIOC_VR_VBG_SET,
};

enum average_sel {
	VF610_ADC_SAMPLE_1,
	VF610_ADC_SAMPLE_4,
	VF610_ADC_SAMPLE_8,
	VF610_ADC_SAMPLE_16,
	VF610_ADC_SAMPLE_32,
};

enum conversion_mode_sel {
	VF610_ADC_CONV_NORMAL,
	VF610_ADC_CONV_HIGH_SPEED,
	VF610_ADC_CONV_LOW_POWER,
};

enum lst_adder_sel {
	VF610_ADCK_CYCLES_3,
	VF610_ADCK_CYCLES_5,
	VF610_ADCK_CYCLES_7,
	VF610_ADCK_CYCLES_9,
	VF610_ADCK_CYCLES_13,
	VF610_ADCK_CYCLES_17,
	VF610_ADCK_CYCLES_21,
	VF610_ADCK_CYCLES_25,
};

struct vf610_adc_feature {
	enum clk_sel	clk_sel;
	enum vol_ref	vol_ref;
	enum conversion_mode_sel conv_mode;

	int	clk_div;
	int     sample_rate;
	int	res_mode;
	u32 lst_adder_index;
	u32 default_sample_time;

	bool	calibration;
	bool	ovwren;
};

struct vf610_adc {
	struct device *dev;
	void __iomem *regs;
	struct clk *clk;
	struct aiodevice aiodev;
	void (*aiodev_info)(struct device *);

	u32 vref_uv;
	u32 value;
	struct regulator *vref;

	u32 max_adck_rate[3];
	struct vf610_adc_feature adc_feature;

	u32 sample_freq_avail[5];

	struct aiochannel aiochan[16];
};

static const u32 vf610_hw_avgs[] = { 1, 4, 8, 16, 32 };
static const u32 vf610_lst_adder[] = { 3, 5, 7, 9, 13, 17, 21, 25 };

static inline void vf610_adc_calculate_rates(struct vf610_adc *info)
{
	struct vf610_adc_feature *adc_feature = &info->adc_feature;
	unsigned long adck_rate, ipg_rate = clk_get_rate(info->clk);
	u32 adck_period, lst_addr_min;
	int divisor, i;

	adck_rate = info->max_adck_rate[adc_feature->conv_mode];

	if (adck_rate) {
		/* calculate clk divider which is within specification */
		divisor = ipg_rate / adck_rate;
		adc_feature->clk_div = 1 << fls(divisor + 1);
	} else {
		/* fall-back value using a safe divisor */
		adc_feature->clk_div = 8;
	}

	adck_rate = ipg_rate / adc_feature->clk_div;

	/*
	 * Determine the long sample time adder value to be used based
	 * on the default minimum sample time provided.
	 */
	adck_period = NSEC_PER_SEC / adck_rate;
	lst_addr_min = adc_feature->default_sample_time / adck_period;
	for (i = 0; i < ARRAY_SIZE(vf610_lst_adder); i++) {
		if (vf610_lst_adder[i] > lst_addr_min) {
			adc_feature->lst_adder_index = i;
			break;
		}
	}

	/*
	 * Calculate ADC sample frequencies
	 * Sample time unit is ADCK cycles. ADCK clk source is ipg clock,
	 * which is the same as bus clock.
	 *
	 * ADC conversion time = SFCAdder + AverageNum x (BCT + LSTAdder)
	 * SFCAdder: fixed to 6 ADCK cycles
	 * AverageNum: 1, 4, 8, 16, 32 samples for hardware average.
	 * BCT (Base Conversion Time): fixed to 25 ADCK cycles for 12 bit mode
	 * LSTAdder(Long Sample Time): 3, 5, 7, 9, 13, 17, 21, 25 ADCK cycles
	 */
	for (i = 0; i < ARRAY_SIZE(vf610_hw_avgs); i++)
		info->sample_freq_avail[i] =
			adck_rate / (6 + vf610_hw_avgs[i] *
			 (25 + vf610_lst_adder[adc_feature->lst_adder_index]));
}

static inline void vf610_adc_cfg_init(struct vf610_adc *info)
{
	struct vf610_adc_feature *adc_feature = &info->adc_feature;

	/* set default Configuration for ADC controller */
	adc_feature->clk_sel = VF610_ADCIOC_BUSCLK_SET;
	adc_feature->vol_ref = VF610_ADCIOC_VR_VREF_SET;

	adc_feature->calibration = true;
	adc_feature->ovwren = true;

	adc_feature->res_mode = 12;
	adc_feature->sample_rate = 1;

	adc_feature->conv_mode = VF610_ADC_CONV_LOW_POWER;

	vf610_adc_calculate_rates(info);
}

static void vf610_adc_cfg_post_set(struct vf610_adc *info)
{
	struct vf610_adc_feature *adc_feature = &info->adc_feature;
	int cfg_data = 0;
	int gc_data = 0;

	switch (adc_feature->clk_sel) {
	case VF610_ADCIOC_ALTCLK_SET:
		cfg_data |= VF610_ADC_ALTCLK_SEL;
		break;
	case VF610_ADCIOC_ADACK_SET:
		cfg_data |= VF610_ADC_ADACK_SEL;
		break;
	default:
		break;
	}

	/* low power set for calibration */
	cfg_data |= VF610_ADC_ADLPC_EN;

	/* enable high speed for calibration */
	cfg_data |= VF610_ADC_ADHSC_EN;

	/* voltage reference */
	switch (adc_feature->vol_ref) {
	case VF610_ADCIOC_VR_VREF_SET:
		break;
	case VF610_ADCIOC_VR_VALT_SET:
		cfg_data |= VF610_ADC_REFSEL_VALT;
		break;
	case VF610_ADCIOC_VR_VBG_SET:
		cfg_data |= VF610_ADC_REFSEL_VBG;
		break;
	default:
		dev_err(info->dev, "error voltage reference\n");
	}

	/* data overwrite enable */
	if (adc_feature->ovwren)
		cfg_data |= VF610_ADC_OVWREN;

	writel(cfg_data, info->regs + VF610_REG_ADC_CFG);
	writel(gc_data, info->regs + VF610_REG_ADC_GC);
}

static void vf610_adc_calibration(struct vf610_adc *info)
{
	int adc_gc, hc_cfg;
	u64 start;
	int coco;

	if (!info->adc_feature.calibration)
		return;

	hc_cfg = VF610_ADC_CONV_DISABLE;
	writel(hc_cfg, info->regs + VF610_REG_ADC_HC0);

	adc_gc = readl(info->regs + VF610_REG_ADC_GC);
	writel(adc_gc | VF610_ADC_CAL, info->regs + VF610_REG_ADC_GC);

	start = get_time_ns();
	do {
		if (is_timeout(start, VF610_ADC_TIMEOUT_NSEC)) {
			dev_err(info->dev, "Timeout for adc calibration\n");
			break;
		}

		coco = readl(info->regs + VF610_REG_ADC_HS);
	} while (!(coco & VF610_ADC_HS_COCO0));

	adc_gc = readl(info->regs + VF610_REG_ADC_GS);
	if (adc_gc & VF610_ADC_CALF)
		dev_err(info->dev, "ADC calibration failed\n");

	info->adc_feature.calibration = false;
}

static void vf610_adc_cfg_set(struct vf610_adc *info)
{
	struct vf610_adc_feature *adc_feature = &(info->adc_feature);
	int cfg_data;

	cfg_data = readl(info->regs + VF610_REG_ADC_CFG);

	cfg_data &= ~VF610_ADC_ADLPC_EN;
	if (adc_feature->conv_mode == VF610_ADC_CONV_LOW_POWER)
		cfg_data |= VF610_ADC_ADLPC_EN;

	cfg_data &= ~VF610_ADC_ADHSC_EN;
	if (adc_feature->conv_mode == VF610_ADC_CONV_HIGH_SPEED)
		cfg_data |= VF610_ADC_ADHSC_EN;

	writel(cfg_data, info->regs + VF610_REG_ADC_CFG);
}

static void vf610_adc_sample_set(struct vf610_adc *info)
{
	struct vf610_adc_feature *adc_feature = &(info->adc_feature);
	int cfg_data, gc_data;

	cfg_data = readl(info->regs + VF610_REG_ADC_CFG);
	gc_data = readl(info->regs + VF610_REG_ADC_GC);

	/* resolution mode */
	cfg_data &= ~VF610_ADC_MODE_MASK;
	switch (adc_feature->res_mode) {
	case 8:
		cfg_data |= VF610_ADC_MODE_BIT8;
		break;
	case 10:
		cfg_data |= VF610_ADC_MODE_BIT10;
		break;
	case 12:
		cfg_data |= VF610_ADC_MODE_BIT12;
		break;
	default:
		dev_err(info->dev, "error resolution mode\n");
		break;
	}

	/* clock select and clock divider */
	cfg_data &= ~(VF610_ADC_CLK_MASK | VF610_ADC_ADCCLK_MASK);
	switch (adc_feature->clk_div) {
	case 1:
		break;
	case 2:
		cfg_data |= VF610_ADC_CLK_DIV2;
		break;
	case 4:
		cfg_data |= VF610_ADC_CLK_DIV4;
		break;
	case 8:
		cfg_data |= VF610_ADC_CLK_DIV8;
		break;
	case 16:
		switch (adc_feature->clk_sel) {
		case VF610_ADCIOC_BUSCLK_SET:
			cfg_data |= VF610_ADC_BUSCLK2_SEL | VF610_ADC_CLK_DIV8;
			break;
		default:
			dev_err(info->dev, "error clk divider\n");
			break;
		}
		break;
	}

	/*
	 * Set ADLSMP and ADSTS based on the Long Sample Time Adder value
	 * determined.
	 */
	switch (adc_feature->lst_adder_index) {
	case VF610_ADCK_CYCLES_3:
		break;
	case VF610_ADCK_CYCLES_5:
		cfg_data |= VF610_ADC_ADSTS_SHORT;
		break;
	case VF610_ADCK_CYCLES_7:
		cfg_data |= VF610_ADC_ADSTS_NORMAL;
		break;
	case VF610_ADCK_CYCLES_9:
		cfg_data |= VF610_ADC_ADSTS_LONG;
		break;
	case VF610_ADCK_CYCLES_13:
		cfg_data |= VF610_ADC_ADLSMP_LONG;
		break;
	case VF610_ADCK_CYCLES_17:
		cfg_data |= VF610_ADC_ADLSMP_LONG;
		cfg_data |= VF610_ADC_ADSTS_SHORT;
		break;
	case VF610_ADCK_CYCLES_21:
		cfg_data |= VF610_ADC_ADLSMP_LONG;
		cfg_data |= VF610_ADC_ADSTS_NORMAL;
		break;
	case VF610_ADCK_CYCLES_25:
		cfg_data |= VF610_ADC_ADLSMP_LONG;
		cfg_data |= VF610_ADC_ADSTS_NORMAL;
		break;
	default:
		dev_err(info->dev, "error in sample time select\n");
	}

	/* update hardware average selection */
	cfg_data &= ~VF610_ADC_AVGS_MASK;
	gc_data &= ~VF610_ADC_AVGEN;
	switch (adc_feature->sample_rate) {
	case VF610_ADC_SAMPLE_1:
		break;
	case VF610_ADC_SAMPLE_4:
		gc_data |= VF610_ADC_AVGEN;
		break;
	case VF610_ADC_SAMPLE_8:
		gc_data |= VF610_ADC_AVGEN;
		cfg_data |= VF610_ADC_AVGS_8;
		break;
	case VF610_ADC_SAMPLE_16:
		gc_data |= VF610_ADC_AVGEN;
		cfg_data |= VF610_ADC_AVGS_16;
		break;
	case VF610_ADC_SAMPLE_32:
		gc_data |= VF610_ADC_AVGEN;
		cfg_data |= VF610_ADC_AVGS_32;
		break;
	default:
		dev_err(info->dev,
			"error hardware sample average select\n");
	}

	writel(cfg_data, info->regs + VF610_REG_ADC_CFG);
	writel(gc_data, info->regs + VF610_REG_ADC_GC);
}

static void vf610_adc_hw_init(struct vf610_adc *info)
{
	/* CFG: Feature set */
	vf610_adc_cfg_post_set(info);
	vf610_adc_sample_set(info);

	/* adc calibration */
	vf610_adc_calibration(info);

	/* CFG: power and speed set */
	vf610_adc_cfg_set(info);
}

static int __vf610_adc_read_data(struct vf610_adc *info)
{
	int result;

	result = readl(info->regs + VF610_REG_ADC_R0);

	switch (info->adc_feature.res_mode) {
	case 8:
		result &= 0xFF;
		break;
	case 10:
		result &= 0x3FF;
		break;
	case 12:
		result &= 0xFFF;
		break;
	default:
		break;
	}

	return result;
}

static int vf610_adc_read_data(struct vf610_adc *info)
{
	int coco;
	int ret = -EAGAIN;

	coco = readl(info->regs + VF610_REG_ADC_HS);
	if (coco & VF610_ADC_HS_COCO0) {
		ret = __vf610_adc_read_data(info);
	}

	return ret;
}

static int vf610_read_sample(struct aiochannel *chan, int *val)
{
	struct vf610_adc *info = container_of(chan->aiodev, struct vf610_adc, aiodev);
	unsigned int hc_cfg;
	u64 raw64, start;
	int ret;

	hc_cfg = VF610_ADC_ADCHC(chan->index & 0x0f);
	writel(hc_cfg, info->regs + VF610_REG_ADC_HC0);

	start = get_time_ns();
	do {
		if (is_timeout(start, VF610_ADC_TIMEOUT_NSEC)) {
			ret = -ETIMEDOUT;
			break;
		}

		ret = vf610_adc_read_data(info);
	} while (ret == -EAGAIN);

	if (ret < 0)
		return ret;

	raw64 = ret;
	raw64 *= info->vref_uv;
	raw64 = div_u64(raw64, 1000);
	*val = div_u64(raw64, (1 << 12));

	return 0;
}


static const struct of_device_id vf610_adc_match[] = {
	{ .compatible = "fsl,vf610-adc", },
	{ /* sentinel */ }
};

static void vf610_adc_devinfo(struct device *dev)
{
	struct vf610_adc *info = dev->parent->priv;

	if (info->aiodev_info)
		info->aiodev_info(dev);

	pr_info("Sample Rate: %u\n", info->sample_freq_avail[info->adc_feature.sample_rate]);
}

static int vf610_adc_probe(struct device *dev)
{
	struct aiodevice *aiodev;
	struct vf610_adc *info;
	int ret, i;

	info = xzalloc(sizeof(*info));
	info->dev = dev;

	info->regs = dev_request_mem_region(dev, 0);
	if (IS_ERR(info->regs))
		return PTR_ERR(info->regs);

	info->clk = clk_get(dev, "adc");
	if (IS_ERR(info->clk)) {
		dev_err(dev, "failed getting clock, err = %ld\n",
						PTR_ERR(info->clk));
		return PTR_ERR(info->clk);
	}

	info->vref = regulator_get(dev, "vref");
	if (IS_ERR(info->vref))
		return PTR_ERR(info->vref);

	of_property_read_u32_array(dev->device_node, "fsl,adck-max-frequency", info->max_adck_rate, 3);

	info->adc_feature.default_sample_time = DEFAULT_SAMPLE_TIME;
	of_property_read_u32(dev->device_node, "min-sample-time", &info->adc_feature.default_sample_time);

	dev->priv = info;
	aiodev = &info->aiodev;

	aiodev->num_channels = 16;
	aiodev->hwdev = dev;
	aiodev->read = vf610_read_sample;
	aiodev->channels = xzalloc(aiodev->num_channels * sizeof(aiodev->channels[0]));

	for (i = 0; i < aiodev->num_channels; i++) {
		aiodev->channels[i] = &info->aiochan[i];
		info->aiochan[i].unit = "mV";
	}

	ret = regulator_enable(info->vref);
	if (ret)
		return ret;

	info->vref_uv = regulator_get_voltage(info->vref);

	ret = clk_enable(info->clk);
	if (ret) {
		dev_err(dev,
			"Could not prepare or enable the clock.\n");
		goto error_adc_clk_enable;
	}

	vf610_adc_cfg_init(info);
	vf610_adc_hw_init(info);

	ret = aiodevice_register(aiodev);
	if (ret < 0) {
		dev_err(dev, "Couldn't register the device.\n");
		goto error_adc_buffer_init;
	}

	info->aiodev_info = aiodev->dev.info;
	aiodev->dev.info = vf610_adc_devinfo;

	return 0;

error_adc_buffer_init:
	clk_disable(info->clk);
error_adc_clk_enable:
	regulator_disable(info->vref);

	return ret;
}

static void vf610_adc_remove(struct device *dev)
{
	struct vf610_adc *info = dev->priv;

	regulator_disable(info->vref);
	clk_disable(info->clk);
}

static struct driver vf610_adc_driver = {
	.probe          = vf610_adc_probe,
	.remove         = vf610_adc_remove,
	.name           = DRIVER_NAME,
	.of_compatible  = vf610_adc_match,
};

device_platform_driver(vf610_adc_driver);

MODULE_AUTHOR("Fugang Duan <B38611@freescale.com>");
MODULE_DESCRIPTION("Freescale VF610 ADC driver");
MODULE_LICENSE("GPL v2");
