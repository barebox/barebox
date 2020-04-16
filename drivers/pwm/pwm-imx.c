/*
 * simple driver for PWM (Pulse Width Modulator) controller
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Derived from pxa PWM driver by eric miao <eric.miao@marvell.com>
 */

#include <common.h>
#include <init.h>
#include <errno.h>
#include <io.h>
#include <pwm.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <asm-generic/div64.h>

/* i.MX1 and i.MX21 share the same PWM function block: */

#define MX1_PWMC    0x00   /* PWM Control Register */
#define MX1_PWMS    0x04   /* PWM Sample Register */
#define MX1_PWMP    0x08   /* PWM Period Register */

#define MX1_PWMC_EN		(1 << 4)

/* i.MX27, i.MX31, i.MX35 share the same PWM function block: */

#define MX3_PWMCR                 0x00    /* PWM Control Register */
#define MX3_PWMSAR                0x0C    /* PWM Sample Register */
#define MX3_PWMPR                 0x10    /* PWM Period Register */
#define MX3_PWMCR_PRESCALER(x)    (((x - 1) & 0xFFF) << 4)
#define MX3_PWMCR_DOZEEN                (1 << 24)
#define MX3_PWMCR_WAITEN                (1 << 23)
#define MX3_PWMCR_DBGEN			(1 << 22)
#define MX3_PWMCR_CLKSRC_IPG_HIGH (2 << 16)
#define MX3_PWMCR_CLKSRC_IPG      (1 << 16)
#define MX3_PWMCR_EN              (1 << 0)

struct imx_chip {
	struct clk	*clk_per;

	void __iomem	*mmio_base;

	struct pwm_chip	chip;

	int (*config)(struct pwm_chip *chip,
		int duty_ns, int period_ns);
	void (*set_enable)(struct pwm_chip *chip, bool enable);
};

#define to_imx_chip(chip)	container_of(chip, struct imx_chip, chip)

static int imx_pwm_config_v1(struct pwm_chip *chip,
		int duty_ns, int period_ns)
{
	struct imx_chip *imx = to_imx_chip(chip);

	/*
	 * The PWM subsystem allows for exact frequencies. However,
	 * I cannot connect a scope on my device to the PWM line and
	 * thus cannot provide the program the PWM controller
	 * exactly. Instead, I'm relying on the fact that the
	 * Bootloader (u-boot or WinCE+haret) has programmed the PWM
	 * function group already. So I'll just modify the PWM sample
	 * register to follow the ratio of duty_ns vs. period_ns
	 * accordingly.
	 *
	 * This is good enough for programming the brightness of
	 * the LCD backlight.
	 *
	 * The real implementation would divide PERCLK[0] first by
	 * both the prescaler (/1 .. /128) and then by CLKSEL
	 * (/2 .. /16).
	 */
	u32 max = readl(imx->mmio_base + MX1_PWMP);
	u32 p = max * duty_ns / period_ns;
	writel(max - p, imx->mmio_base + MX1_PWMS);

	return 0;
}

static void imx_pwm_set_enable_v1(struct pwm_chip *chip, bool enable)
{
	struct imx_chip *imx = to_imx_chip(chip);
	u32 val;

	val = readl(imx->mmio_base + MX1_PWMC);

	if (enable)
		val |= MX1_PWMC_EN;
	else
		val &= ~MX1_PWMC_EN;

	writel(val, imx->mmio_base + MX1_PWMC);
}

static int imx_pwm_config_v2(struct pwm_chip *chip,
		int duty_ns, int period_ns)
{
	struct imx_chip *imx = to_imx_chip(chip);
	unsigned long long c;
	unsigned long period_cycles, duty_cycles, prescale;
	u32 cr;

	c = clk_get_rate(imx->clk_per);
	c = c * period_ns;
	do_div(c, 1000000000);
	period_cycles = c;

	prescale = period_cycles / 0x10000 + 1;

	period_cycles /= prescale;
	c = (unsigned long long)period_cycles * duty_ns;
	do_div(c, period_ns);
	duty_cycles = c;

	/*
	 * according to imx pwm RM, the real period value should be
	 * PERIOD value in PWMPR plus 2.
	 */
	if (period_cycles > 2)
		period_cycles -= 2;
	else
		period_cycles = 0;

	writel(duty_cycles, imx->mmio_base + MX3_PWMSAR);
	writel(period_cycles, imx->mmio_base + MX3_PWMPR);

	cr = MX3_PWMCR_PRESCALER(prescale) |
		MX3_PWMCR_DOZEEN | MX3_PWMCR_WAITEN |
		MX3_PWMCR_DBGEN | MX3_PWMCR_CLKSRC_IPG_HIGH;

	if (readl(imx->mmio_base + MX3_PWMCR) & MX3_PWMCR_EN)
		cr |= MX3_PWMCR_EN;

	writel(cr, imx->mmio_base + MX3_PWMCR);

	return 0;
}

static void imx_pwm_set_enable_v2(struct pwm_chip *chip, bool enable)
{
	struct imx_chip *imx = to_imx_chip(chip);
	u32 val;

	val = readl(imx->mmio_base + MX3_PWMCR);

	if (enable)
		val |= MX3_PWMCR_EN;
	else
		val &= ~MX3_PWMCR_EN;

	writel(val, imx->mmio_base + MX3_PWMCR);
}

static int imx_pwm_apply(struct pwm_chip *chip, const struct pwm_state *state)
{
	struct imx_chip *imx = to_imx_chip(chip);
	bool enabled;
	int ret;

	enabled = chip->state.p_enable;

	if (enabled && !state->p_enable) {
		imx->set_enable(chip, false);
		return 0;
	}

	ret = imx->config(chip, state->duty_ns, state->period_ns);
	if (ret)
		return ret;

	if (!enabled && state->p_enable)
		imx->set_enable(chip, true);

	return 0;
}

static struct pwm_ops imx_pwm_ops = {
	.apply = imx_pwm_apply,
};

struct imx_pwm_data {
	int (*config)(struct pwm_chip *chip,
		int duty_ns, int period_ns);
	void (*set_enable)(struct pwm_chip *chip, bool enable);
};

static struct imx_pwm_data imx_pwm_data_v1 = {
	.config = imx_pwm_config_v1,
	.set_enable = imx_pwm_set_enable_v1,
};

static struct imx_pwm_data imx_pwm_data_v2 = {
	.config = imx_pwm_config_v2,
	.set_enable = imx_pwm_set_enable_v2,
};

static struct of_device_id imx_pwm_dt_ids[] = {
	{ .compatible = "fsl,imx1-pwm", .data = &imx_pwm_data_v1, },
	{ .compatible = "fsl,imx27-pwm", .data = &imx_pwm_data_v2, },
	{ /* sentinel */ }
};

static int imx_pwm_probe(struct device_d *dev)
{
	struct resource *iores;
	const struct imx_pwm_data *data;
	struct imx_chip *imx;
	int ret = 0;

	ret = dev_get_drvdata(dev, (const void **)&data);
	if (ret)
		return ret;

	imx = xzalloc(sizeof(*imx));

	imx->clk_per = clk_get(dev, "per");
	if (IS_ERR(imx->clk_per))
		return PTR_ERR(imx->clk_per);

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	imx->mmio_base = IOMEM(iores->start);

	imx->chip.ops = &imx_pwm_ops;
	if (dev->device_node) {
		imx->chip.devname = of_alias_get(dev->device_node);
		if (!imx->chip.devname)
			imx->chip.devname = basprintf("pwm_%p",
							imx->mmio_base);
	} else {
		imx->chip.devname = basprintf("pwm%d", dev->id);
	}

	imx->config = data->config;
	imx->set_enable = data->set_enable;

	ret = pwmchip_add(&imx->chip, dev);
	if (ret < 0)
		return ret;

	return 0;
}

static struct driver_d imx_pwm_driver = {
	.name	= "imx-pwm",
	.of_compatible	= imx_pwm_dt_ids,
	.probe		= imx_pwm_probe,
};

coredevice_platform_driver(imx_pwm_driver);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Sascha Hauer <s.hauer@pengutronix.de>");
