/*
 * Copyright 2012 Freescale Semiconductor, Inc.
 *
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <common.h>
#include <init.h>
#include <errno.h>
#include <io.h>
#include <pwm.h>
#include <stmp-device.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <asm-generic/div64.h>

#define SET	0x4
#define CLR	0x8
#define TOG	0xc

#define PWM_CTRL		0x0
#define PWM_ACTIVE0		0x10
#define PWM_PERIOD0		0x20
#define  PERIOD_PERIOD(p)	((p) & 0xffff)
#define  PERIOD_PERIOD_MAX	0x10000
#define  PERIOD_ACTIVE_HIGH	(3 << 16)
#define  PERIOD_INACTIVE_LOW	(2 << 18)
#define  PERIOD_CDIV(div)	(((div) & 0x7) << 20)
#define  PERIOD_CDIV_MAX	8

static const unsigned int cdiv[PERIOD_CDIV_MAX] = {
	1, 2, 4, 8, 16, 64, 256, 1024
};

struct mxs_pwm;

struct mxs_pwm_chip {
	struct pwm_chip chip;
	struct mxs_pwm *mxs;
};

struct mxs_pwm {
	struct mxs_pwm_chip pwm[8];
	struct clk *clk;
	void __iomem *base;
};

#define to_mxs_pwm_chip(_chip) container_of(_chip, struct mxs_pwm_chip, chip)

static int mxs_pwm_apply(struct pwm_chip *chip, const struct pwm_state *state)
{
	struct mxs_pwm_chip *mxs = to_mxs_pwm_chip(chip);
	int div = 0;
	unsigned int period_cycles, duty_cycles;
	unsigned long rate;
	unsigned long long c;
	bool enabled;

	enabled = chip->state.p_enable;

	if (enabled && !state->p_enable) {
		writel(1 << mxs->chip.id, mxs->mxs->base + PWM_CTRL + CLR);
		return 0;
	}

	rate = clk_get_rate(mxs->mxs->clk);
	while (1) {
		c = rate / cdiv[div];
		c = c * state->period_ns;
		do_div(c, 1000000000);
		if (c < PERIOD_PERIOD_MAX)
			break;
		div++;
		if (div >= PERIOD_CDIV_MAX)
			return -EINVAL;
	}

	period_cycles = c;
	c *= state->duty_ns;
	do_div(c, state->period_ns);
	duty_cycles = c;

	writel(duty_cycles << 16,
			mxs->mxs->base + PWM_ACTIVE0 + mxs->chip.id * 0x20);
	writel(PERIOD_PERIOD(period_cycles) | PERIOD_ACTIVE_HIGH |
	       PERIOD_INACTIVE_LOW | PERIOD_CDIV(div),
			mxs->mxs->base + PWM_PERIOD0 + mxs->chip.id * 0x20);

	if (!enabled && state->p_enable)
		writel(1 << mxs->chip.id, mxs->mxs->base + PWM_CTRL + SET);

	return 0;
}

static struct pwm_ops mxs_pwm_ops = {
	.apply = mxs_pwm_apply,
};

static int mxs_pwm_probe(struct device_d *dev)
{
	struct resource *iores;
	struct device_node *np = dev->device_node;
	struct mxs_pwm *mxs;
	int ret, i;
	uint32_t npwm;

	mxs = xzalloc(sizeof(*mxs));

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	mxs->base = IOMEM(iores->start);

	mxs->clk = clk_get(dev, NULL);
	if (IS_ERR(mxs->clk))
		return PTR_ERR(mxs->clk);

	clk_enable(mxs->clk);

	ret = stmp_reset_block(mxs->base, 0);
	if (ret)
		return ret;

	ret = of_property_read_u32(np, "fsl,pwm-number", &npwm);
	if (ret < 0) {
		dev_err(dev, "failed to get pwm number: %d\n", ret);
		return ret;
	}

	for (i = 0; i < npwm; i++) {
		struct mxs_pwm_chip *mxspwm = &mxs->pwm[i];

		mxspwm->chip.ops = &mxs_pwm_ops;
		mxspwm->chip.devname = basprintf("pwm%d", i);
		mxspwm->chip.id = i;
		mxspwm->mxs = mxs;

		ret = pwmchip_add(&mxspwm->chip, dev);
		if (ret < 0) {
			dev_err(dev, "failed to add pwm chip %d\n", ret);
			return ret;
		}
	}

	return 0;
}

static const struct of_device_id mxs_pwm_dt_ids[] = {
	{ .compatible = "fsl,imx23-pwm", },
	{ /* sentinel */ }
};

static struct driver_d mxs_pwm_driver = {
	.name	= "mxs-pwm",
	.of_compatible	= mxs_pwm_dt_ids,
	.probe		= mxs_pwm_probe,
};

coredevice_platform_driver(mxs_pwm_driver);

MODULE_ALIAS("platform:mxs-pwm");
MODULE_AUTHOR("Shawn Guo <shawn.guo@linaro.org>");
MODULE_DESCRIPTION("Freescale MXS PWM Driver");
MODULE_LICENSE("GPL v2");
