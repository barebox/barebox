// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright 2018-2019 NXP.
 *
 * Limitations:
 * - The TPM counter and period counter are shared between
 *   multiple channels, so all channels should use same period
 *   settings.
 * - Changes to polarity cannot be latched at the time of the
 *   next period start.
 * - Changing period and duty cycle together isn't atomic,
 *   with the wrong timing it might happen that a period is
 *   produced with old duty cycle but new period settings.
 */

#include <common.h>
#include <clock.h>
#include <of.h>
#include <pwm.h>
#include <init.h>
#include <linux/bitfield.h>
#include <linux/bitops.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/time.h>
#include <linux/log2.h>
#include <linux/module.h>
#include <linux/barebox-wrapper.h>
#include <linux/pinctrl/consumer.h>
#include <linux/slab.h>
#include <linux/device.h>

#define PWM_IMX_TPM_PARAM	0x4
#define PWM_IMX_TPM_GLOBAL	0x8
#define PWM_IMX_TPM_SC		0x10
#define PWM_IMX_TPM_CNT		0x14
#define PWM_IMX_TPM_MOD		0x18
#define PWM_IMX_TPM_CnSC(n)	(0x20 + (n) * 0x8)
#define PWM_IMX_TPM_CnV(n)	(0x24 + (n) * 0x8)

#define PWM_IMX_TPM_PARAM_CHAN			GENMASK(7, 0)

#define PWM_IMX_TPM_SC_PS			GENMASK(2, 0)
#define PWM_IMX_TPM_SC_CMOD			GENMASK(4, 3)
#define PWM_IMX_TPM_SC_CMOD_INC_EVERY_CLK	FIELD_PREP(PWM_IMX_TPM_SC_CMOD, 1)
#define PWM_IMX_TPM_SC_CPWMS			BIT(5)

#define PWM_IMX_TPM_CnSC_CHF	BIT(7)
#define PWM_IMX_TPM_CnSC_MSB	BIT(5)
#define PWM_IMX_TPM_CnSC_MSA	BIT(4)

/*
 * The reference manual describes this field as two separate bits. The
 * semantic of the two bits isn't orthogonal though, so they are treated
 * together as a 2-bit field here.
 */
#define PWM_IMX_TPM_CnSC_ELS	GENMASK(3, 2)
#define PWM_IMX_TPM_CnSC_ELS_INVERSED	FIELD_PREP(PWM_IMX_TPM_CnSC_ELS, 1)
#define PWM_IMX_TPM_CnSC_ELS_NORMAL	FIELD_PREP(PWM_IMX_TPM_CnSC_ELS, 2)


#define PWM_IMX_TPM_MOD_WIDTH	16
#define PWM_IMX_TPM_MOD_MOD	GENMASK(PWM_IMX_TPM_MOD_WIDTH - 1, 0)

struct imx_tpm_pwm;

struct imx_tpm_pwm_chip {
	struct pwm_chip chip;
	struct imx_tpm_pwm *tpm;
	u32 user_count;
	u32 enable_count;
	u32 real_period;
};

struct imx_tpm_pwm {
	struct imx_tpm_pwm_chip pwm[8];
	struct clk *clk;
	void __iomem *base;
};

struct imx_tpm_pwm_param {
	u8 prescale;
	u32 mod;
	u32 val;
};

#define to_imx_tpm_pwm_chip(_chip) container_of(_chip, struct imx_tpm_pwm_chip, chip)

/*
 * This function determines for a given pwm_state *state that a consumer
 * might request the pwm_state *real_state that eventually is implemented
 * by the hardware and the necessary register values (in *p) to achieve
 * this.
 */
static int pwm_imx_tpm_round_state(struct pwm_chip *chip,
				   struct imx_tpm_pwm_param *p,
				   struct pwm_state *real_state,
				   const struct pwm_state *state)
{
	struct imx_tpm_pwm_chip *tpm = to_imx_tpm_pwm_chip(chip);
	u32 rate, prescale, period_count, clock_unit;
	u64 tmp;

	rate = clk_get_rate(tpm->tpm->clk);
	tmp = (u64)state->period * rate;
	clock_unit = DIV_ROUND_CLOSEST_ULL(tmp, NSEC_PER_SEC);
	if (clock_unit <= PWM_IMX_TPM_MOD_MOD)
		prescale = 0;
	else
		prescale = ilog2(clock_unit) + 1 - PWM_IMX_TPM_MOD_WIDTH;

	if ((!FIELD_FIT(PWM_IMX_TPM_SC_PS, prescale)))
		return -ERANGE;
	p->prescale = prescale;

	period_count = (clock_unit + ((1 << prescale) >> 1)) >> prescale;
	if (period_count == 0)
		return -EINVAL;
	p->mod = period_count - 1;

	/* calculate real period HW can support */
	tmp = (u64)period_count << prescale;
	tmp *= NSEC_PER_SEC;
	real_state->period = DIV_ROUND_CLOSEST_ULL(tmp, rate);

	/*
	 * if eventually the PWM output is inactive, either
	 * duty cycle is 0 or status is disabled, need to
	 * make sure the output pin is inactive.
	 */
	if (!state->enabled)
		real_state->duty_cycle = 0;
	else
		real_state->duty_cycle = state->duty_cycle;

	tmp = (u64)p->mod * real_state->duty_cycle;
	p->val = DIV64_U64_ROUND_CLOSEST(tmp, real_state->period);

	real_state->polarity = state->polarity;
	real_state->enabled = state->enabled;

	return 0;
}

static int pwm_imx_tpm_get_state(struct pwm_chip *chip,
				 struct pwm_device *pwm,
				 struct pwm_state *state)
{
	struct imx_tpm_pwm_chip *tpm = to_imx_tpm_pwm_chip(chip);
	u32 rate, val, prescale;
	u64 tmp;

	/* get period */
	state->period = tpm->real_period;

	/* get duty cycle */
	rate = clk_get_rate(tpm->tpm->clk);
	val = readl(tpm->tpm->base + PWM_IMX_TPM_SC);
	prescale = FIELD_GET(PWM_IMX_TPM_SC_PS, val);
	tmp = readl(tpm->tpm->base + PWM_IMX_TPM_CnV(chip->id));
	tmp = (tmp << prescale) * NSEC_PER_SEC;
	state->duty_cycle = DIV_ROUND_CLOSEST_ULL(tmp, rate);

	/* get polarity */
	val = readl(tpm->tpm->base + PWM_IMX_TPM_CnSC(chip->id));
	if ((val & PWM_IMX_TPM_CnSC_ELS) == PWM_IMX_TPM_CnSC_ELS_INVERSED)
		state->polarity = PWM_POLARITY_INVERSED;
	else
		/*
		 * Assume reserved values (2b00 and 2b11) to yield
		 * normal polarity.
		 */
		state->polarity = PWM_POLARITY_NORMAL;

	/* get channel status */
	state->enabled = FIELD_GET(PWM_IMX_TPM_CnSC_ELS, val) ? true : false;

	return 0;
}

/* this function is supposed to be called with mutex hold */
static int pwm_imx_tpm_apply_hw(struct pwm_chip *chip,
				struct imx_tpm_pwm_param *p,
				struct pwm_state *state,
				struct pwm_device *pwm)
{
	struct imx_tpm_pwm_chip *tpm = to_imx_tpm_pwm_chip(chip);
	bool period_update = false;
	bool duty_update = false;
	u32 val, cmod, cur_prescale;
	unsigned long timeout_ms;
	struct pwm_state c;
	uint64_t start;

	if (state->period != tpm->real_period) {
		/*
		 * TPM counter is shared by multiple channels, so
		 * prescale and period can NOT be modified when
		 * there are multiple channels in use with different
		 * period settings.
		 */
		if (tpm->user_count > 1)
			return -EBUSY;

		val = readl(tpm->tpm->base + PWM_IMX_TPM_SC);
		cmod = FIELD_GET(PWM_IMX_TPM_SC_CMOD, val);
		cur_prescale = FIELD_GET(PWM_IMX_TPM_SC_PS, val);
		if (cmod && cur_prescale != p->prescale)
			return -EBUSY;

		/* set TPM counter prescale */
		val &= ~PWM_IMX_TPM_SC_PS;
		val |= FIELD_PREP(PWM_IMX_TPM_SC_PS, p->prescale);
		writel(val, tpm->tpm->base + PWM_IMX_TPM_SC);

		/*
		 * set period count:
		 * if the PWM is disabled (CMOD[1:0] = 2b00), then MOD register
		 * is updated when MOD register is written.
		 *
		 * if the PWM is enabled (CMOD[1:0] ≠ 2b00), the period length
		 * is latched into hardware when the next period starts.
		 */
		writel(p->mod, tpm->tpm->base + PWM_IMX_TPM_MOD);
		tpm->real_period = state->period;
		period_update = true;
	}

	pwm_imx_tpm_get_state(chip, pwm, &c);

	/* polarity is NOT allowed to be changed if PWM is active */
	if (c.enabled && c.polarity != state->polarity)
		return -EBUSY;

	if (state->duty_cycle != c.duty_cycle) {
		/*
		 * set channel value:
		 * if the PWM is disabled (CMOD[1:0] = 2b00), then CnV register
		 * is updated when CnV register is written.
		 *
		 * if the PWM is enabled (CMOD[1:0] ≠ 2b00), the duty length
		 * is latched into hardware when the next period starts.
		 */
		writel(p->val, tpm->tpm->base + PWM_IMX_TPM_CnV(chip->id));
		duty_update = true;
	}

	/* make sure MOD & CnV registers are updated */
	if (period_update || duty_update) {
		start = get_time_ns();
		timeout_ms = (tpm->real_period / NSEC_PER_MSEC + 1);
		while (readl(tpm->tpm->base + PWM_IMX_TPM_MOD) != p->mod
		       || readl(tpm->tpm->base + PWM_IMX_TPM_CnV(chip->id))
		       != p->val) {
			if (is_timeout(start, timeout_ms))
				return -ETIME;
			cpu_relax();
		}		udelay(10);
	}

	/*
	 * polarity settings will enabled/disable output status
	 * immediately, so if the channel is disabled, need to
	 * make sure MSA/MSB/ELS are set to 0 which means channel
	 * disabled.
	 */
	val = readl(tpm->tpm->base + PWM_IMX_TPM_CnSC(chip->id));
	val &= ~(PWM_IMX_TPM_CnSC_ELS | PWM_IMX_TPM_CnSC_MSA |
		 PWM_IMX_TPM_CnSC_MSB);
	if (state->enabled) {
		/*
		 * set polarity (for edge-aligned PWM modes)
		 *
		 * ELS[1:0] = 2b10 yields normal polarity behaviour,
		 * ELS[1:0] = 2b01 yields inversed polarity.
		 * The other values are reserved.
		 */
		val |= PWM_IMX_TPM_CnSC_MSB;
		val |= (state->polarity == PWM_POLARITY_NORMAL) ?
			PWM_IMX_TPM_CnSC_ELS_NORMAL :
			PWM_IMX_TPM_CnSC_ELS_INVERSED;
	}
	writel(val, tpm->tpm->base + PWM_IMX_TPM_CnSC(chip->id));

	/* control the counter status */
	if (state->enabled != c.enabled) {
		val = readl(tpm->tpm->base + PWM_IMX_TPM_SC);
		if (state->enabled) {
			if (++tpm->enable_count == 1)
				val |= PWM_IMX_TPM_SC_CMOD_INC_EVERY_CLK;
		} else {
			if (--tpm->enable_count == 0)
				val &= ~PWM_IMX_TPM_SC_CMOD;
		}
		writel(val, tpm->tpm->base + PWM_IMX_TPM_SC);
	}

	return 0;
}

static int pwm_imx_tpm_apply(struct pwm_chip *chip,
			     struct pwm_device *pwm,
			     const struct pwm_state *state)
{
	struct imx_tpm_pwm_param param;
	struct pwm_state real_state;
	int ret;

	ret = pwm_imx_tpm_round_state(chip, &param, &real_state, state);
	if (ret)
		return ret;

	ret = pwm_imx_tpm_apply_hw(chip, &param, &real_state, pwm);

	return ret;
}

static int pwm_imx_tpm_request(struct pwm_chip *chip)
{
	struct imx_tpm_pwm_chip *tpm = to_imx_tpm_pwm_chip(chip);

	tpm->user_count++;

	return 0;
}

static void pwm_imx_tpm_free(struct pwm_chip *chip)
{
	struct imx_tpm_pwm_chip *tpm = to_imx_tpm_pwm_chip(chip);

	tpm->user_count--;
}

static const struct pwm_ops imx_tpm_pwm_ops = {
	.request = pwm_imx_tpm_request,
	.free = pwm_imx_tpm_free,
	.apply = pwm_imx_tpm_apply,
};

static int id = -1;

static int pwm_imx_tpm_probe(struct device *dev)
{
	struct pwm_chip *chip;
	struct imx_tpm_pwm *tpm;
	struct clk *clk;
	void __iomem *base;
	const char *alias;
	int ret;
	unsigned int npwm;
	u32 val;
	int i;

	base = dev_platform_ioremap_resource(dev, 0);
	if (IS_ERR(base))
		return PTR_ERR(base);

	clk = clk_get_enabled(dev, NULL);
	if (IS_ERR(clk))
		return dev_err_probe(dev, PTR_ERR(clk),
				     "failed to get PWM clock\n");

	/* get number of channels */
	val = readl(base + PWM_IMX_TPM_PARAM);
	npwm = FIELD_GET(PWM_IMX_TPM_PARAM_CHAN, val);

	tpm = xzalloc(sizeof(*tpm));

	if (npwm > ARRAY_SIZE(tpm->pwm))
		return dev_err_probe(dev, PTR_ERR(clk),
				     "hardware has more channels then supported\n");

	alias = of_alias_get(dev->of_node);
	if (!alias)
		id++;

	for (i = 0; i < npwm; i++) {
		struct imx_tpm_pwm_chip *tpmc = &tpm->pwm[i];

		chip = &tpmc->chip;

		tpmc->tpm = tpm;
		tpmc->tpm->base = base;
		tpmc->tpm->clk = clk;

		if (alias)
			chip->devname = xasprintf("%sch%u", alias, i + 1);
		else
			chip->devname = xasprintf("pwm%uch%u", id, i + 1);

		chip->ops = &imx_tpm_pwm_ops;
		chip->id = i;
		chip->dev = dev;

		ret = pwmchip_add(chip);
		if (ret)
			return dev_err_probe(dev, ret, "failed to add PWM chip\n");
	}

	return 0;
}

static const struct of_device_id imx_tpm_pwm_dt_ids[] = {
	{ .compatible = "fsl,imx7ulp-pwm", },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, imx_tpm_pwm_dt_ids);

static struct driver imx_tpm_pwm_driver = {
	.name = "imx7ulp-tpm-pwm",
	.probe	= pwm_imx_tpm_probe,
	.of_compatible = imx_tpm_pwm_dt_ids,
};
coredevice_platform_driver(imx_tpm_pwm_driver);

MODULE_AUTHOR("Anson Huang <Anson.Huang@nxp.com>");
MODULE_DESCRIPTION("i.MX TPM PWM Driver");
MODULE_LICENSE("GPL v2");
