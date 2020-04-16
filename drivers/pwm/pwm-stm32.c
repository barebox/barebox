// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2016 STMicroelectronics 2016
 * Copyright (C) 2020 Pengutronix
 *
 * Author: Gerald Baeza <gerald.baeza@st.com>
 *	   Ahmad Fatoum <a.fatoum@pengutronix.de>
 */

#include <common.h>
#include <linux/clk.h>
#include <driver.h>
#include <init.h>
#include <io.h>
#include <linux/bitfield.h>
#include <linux/mfd/stm32-timers.h>
#include <linux/math64.h>
#include <of.h>
#include <pwm.h>

#define CCMR_CHANNEL_SHIFT 8
#define CCMR_CHANNEL_MASK  0xFF
#define MAX_BREAKINPUT 2

struct stm32_pwm {
	struct clk *clk;
	struct regmap *regmap;
	u32 max_arr;
	bool have_complementary_output;
	struct pwm_chip pwms[4];
};

struct stm32_breakinput {
	u32 index;
	u32 level;
	u32 filter;
};

#define for_each_stm32_pwm(i, chip, pwm) \
	for (chip[i = 0] = pwm->pwms[0]; i < 4 && chip->ops; chip = chip[++i])

static inline struct stm32_pwm *to_stm32_pwm_dev(struct pwm_chip *chip)
{
	struct pwm_chip (*pwms)[4] = (void *)&chip[-chip->id];
	return container_of(pwms, struct stm32_pwm, pwms);
}

static u32 active_channels(struct stm32_pwm *dev)
{
	u32 ccer;

	regmap_read(dev->regmap, TIM_CCER, &ccer);

	return ccer & TIM_CCER_CCXE;
}

static int write_ccrx(struct stm32_pwm *dev, unsigned ch, u32 value)
{
	switch (ch) {
	case 0:
		return regmap_write(dev->regmap, TIM_CCR1, value);
	case 1:
		return regmap_write(dev->regmap, TIM_CCR2, value);
	case 2:
		return regmap_write(dev->regmap, TIM_CCR3, value);
	case 3:
		return regmap_write(dev->regmap, TIM_CCR4, value);
	}
	return -EINVAL;
}

#define TIM_CCER_CC12P (TIM_CCER_CC1P | TIM_CCER_CC2P)
#define TIM_CCER_CC12E (TIM_CCER_CC1E | TIM_CCER_CC2E)
#define TIM_CCER_CC34P (TIM_CCER_CC3P | TIM_CCER_CC4P)
#define TIM_CCER_CC34E (TIM_CCER_CC3E | TIM_CCER_CC4E)

static int stm32_pwm_set_polarity(struct stm32_pwm *priv, unsigned ch,
				  unsigned polarity)
{
	u32 mask;

	mask = TIM_CCER_CC1P << (ch * 4);
	if (priv->have_complementary_output)
		mask |= TIM_CCER_CC1NP << (ch * 4);

	regmap_update_bits(priv->regmap, TIM_CCER, mask,
			   polarity == PWM_POLARITY_NORMAL ? 0 : mask);

	return 0;
}

static int stm32_pwm_config(struct stm32_pwm *priv, unsigned ch,
			    int duty_ns, int period_ns)
{
	unsigned long long prd, div, dty;
	unsigned int prescaler = 0;
	u32 ccmr, mask, shift;

	/* Period and prescaler values depends on clock rate */
	div = (unsigned long long)clk_get_rate(priv->clk) * period_ns;

	do_div(div, NSEC_PER_SEC);
	prd = div;

	while (div > priv->max_arr) {
		prescaler++;
		div = prd;
		do_div(div, prescaler + 1);
	}

	prd = div;

	if (prescaler > MAX_TIM_PSC)
		return -EINVAL;

	/*
	 * All channels share the same prescaler and counter so when two
	 * channels are active at the same time we can't change them
	 */
	if (active_channels(priv) & ~(1 << ch * 4)) {
		u32 psc, arr;

		regmap_read(priv->regmap, TIM_PSC, &psc);
		regmap_read(priv->regmap, TIM_ARR, &arr);

		if ((psc != prescaler) || (arr != prd - 1))
			return -EBUSY;
	}

	regmap_write(priv->regmap, TIM_PSC, prescaler);
	regmap_write(priv->regmap, TIM_ARR, prd - 1);
	regmap_update_bits(priv->regmap, TIM_CR1, TIM_CR1_ARPE, TIM_CR1_ARPE);

	/* Calculate the duty cycles */
	dty = prd * duty_ns;
	do_div(dty, period_ns);

	write_ccrx(priv, ch, dty);

	/* Configure output mode */
	shift = (ch & 0x1) * CCMR_CHANNEL_SHIFT;
	ccmr = (TIM_CCMR_PE | TIM_CCMR_M1) << shift;
	mask = CCMR_CHANNEL_MASK << shift;

	if (ch < 2)
		regmap_update_bits(priv->regmap, TIM_CCMR1, mask, ccmr);
	else
		regmap_update_bits(priv->regmap, TIM_CCMR2, mask, ccmr);

	regmap_update_bits(priv->regmap, TIM_BDTR,
			   TIM_BDTR_MOE | TIM_BDTR_AOE,
			   TIM_BDTR_MOE | TIM_BDTR_AOE);

	return 0;
}

static int stm32_pwm_enable(struct stm32_pwm *priv, unsigned ch)
{
	u32 mask;
	int ret;

	ret = clk_enable(priv->clk);
	if (ret)
		return ret;

	/* Enable channel */
	mask = TIM_CCER_CC1E << (ch * 4);
	if (priv->have_complementary_output)
		mask |= TIM_CCER_CC1NE << (ch * 4);

	regmap_update_bits(priv->regmap, TIM_CCER, mask, mask);

	/* Make sure that registers are updated */
	regmap_update_bits(priv->regmap, TIM_EGR, TIM_EGR_UG, TIM_EGR_UG);

	/* Enable controller */
	regmap_update_bits(priv->regmap, TIM_CR1, TIM_CR1_CEN, TIM_CR1_CEN);

	return 0;
}

static void stm32_pwm_disable(struct stm32_pwm *priv, unsigned ch)
{
	u32 mask;

	/* Disable channel */
	mask = TIM_CCER_CC1E << (ch * 4);
	if (priv->have_complementary_output)
		mask |= TIM_CCER_CC1NE << (ch * 4);

	regmap_update_bits(priv->regmap, TIM_CCER, mask, 0);

	/* When all channels are disabled, we can disable the controller */
	if (!active_channels(priv))
		regmap_update_bits(priv->regmap, TIM_CR1, TIM_CR1_CEN, 0);

	clk_disable(priv->clk);
}

static int stm32_pwm_apply(struct pwm_chip *chip, const struct pwm_state *state)
{
	bool enabled;
	struct stm32_pwm *priv = to_stm32_pwm_dev(chip);
	int ret;

	enabled = chip->state.p_enable;

	if (enabled && !state->p_enable) {
		stm32_pwm_disable(priv, chip->id);
		return 0;
	}

	if (state->polarity != chip->state.polarity)
		stm32_pwm_set_polarity(priv, chip->id, state->polarity);

	ret = stm32_pwm_config(priv, chip->id,
			       state->duty_ns, state->period_ns);
	if (ret)
		return ret;

	if (!enabled && state->p_enable)
		ret = stm32_pwm_enable(priv, chip->id);

	return ret;
}

static const struct pwm_ops stm32pwm_ops = {
	.apply = stm32_pwm_apply,
};

static int stm32_pwm_set_breakinput(struct stm32_pwm *priv,
				    int index, int level, int filter)
{
	u32 bke = (index == 0) ? TIM_BDTR_BKE : TIM_BDTR_BK2E;
	int shift = (index == 0) ? TIM_BDTR_BKF_SHIFT : TIM_BDTR_BK2F_SHIFT;
	u32 mask = (index == 0) ? TIM_BDTR_BKE | TIM_BDTR_BKP | TIM_BDTR_BKF
				: TIM_BDTR_BK2E | TIM_BDTR_BK2P | TIM_BDTR_BK2F;
	u32 bdtr = bke;

	/*
	 * The both bits could be set since only one will be wrote
	 * due to mask value.
	 */
	if (level)
		bdtr |= TIM_BDTR_BKP | TIM_BDTR_BK2P;

	bdtr |= (filter & TIM_BDTR_BKF_MASK) << shift;

	regmap_update_bits(priv->regmap, TIM_BDTR, mask, bdtr);

	regmap_read(priv->regmap, TIM_BDTR, &bdtr);

	return (bdtr & bke) ? 0 : -EINVAL;
}

static int stm32_pwm_apply_breakinputs(struct stm32_pwm *priv,
				       struct device_node *np)
{
	struct stm32_breakinput breakinput[MAX_BREAKINPUT];
	int nb, ret, i, array_size;

	nb = of_property_count_elems_of_size(np, "st,breakinput",
					     sizeof(struct stm32_breakinput));

	/*
	 * Because "st,breakinput" parameter is optional do not make probe
	 * failed if it doesn't exist.
	 */
	if (nb <= 0)
		return 0;

	if (nb > MAX_BREAKINPUT)
		return -EINVAL;

	array_size = nb * sizeof(struct stm32_breakinput) / sizeof(u32);
	ret = of_property_read_u32_array(np, "st,breakinput",
					 (u32 *)breakinput, array_size);
	if (ret)
		return ret;

	for (i = 0; i < nb && !ret; i++) {
		ret = stm32_pwm_set_breakinput(priv,
					       breakinput[i].index,
					       breakinput[i].level,
					       breakinput[i].filter);
	}

	return ret;
}

static void stm32_pwm_detect_complementary(struct stm32_pwm *priv)
{
	u32 ccer;

	/*
	 * If complementary bit doesn't exist writing 1 will have no
	 * effect so we can detect it.
	 */
	regmap_update_bits(priv->regmap,
			   TIM_CCER, TIM_CCER_CC1NE, TIM_CCER_CC1NE);
	regmap_read(priv->regmap, TIM_CCER, &ccer);
	regmap_update_bits(priv->regmap, TIM_CCER, TIM_CCER_CC1NE, 0);

	priv->have_complementary_output = (ccer != 0);
}

static int stm32_pwm_detect_channels(struct stm32_pwm *priv)
{
	u32 ccer;
	int npwm = 0;

	/*
	 * If channels enable bits don't exist writing 1 will have no
	 * effect so we can detect and count them.
	 */
	regmap_update_bits(priv->regmap,
			   TIM_CCER, TIM_CCER_CCXE, TIM_CCER_CCXE);
	regmap_read(priv->regmap, TIM_CCER, &ccer);
	regmap_update_bits(priv->regmap, TIM_CCER, TIM_CCER_CCXE, 0);

	if (ccer & TIM_CCER_CC1E)
		npwm++;

	if (ccer & TIM_CCER_CC2E)
		npwm++;

	if (ccer & TIM_CCER_CC3E)
		npwm++;

	if (ccer & TIM_CCER_CC4E)
		npwm++;

	return npwm;
}

static int id = -1;

static int stm32_pwm_probe(struct device_d *dev)
{
	struct device_node *np = dev->device_node;
	struct stm32_timers *ddata = dev->parent->priv;
	struct stm32_pwm *priv;
	const char *alias;
	int ret, i;
	int npwms;

	priv = xzalloc(sizeof(*priv));
	dev->priv = priv;

	priv->regmap = ddata->regmap;
	priv->clk = ddata->clk;
	priv->max_arr = ddata->max_arr;

	if (!priv->regmap || !priv->clk)
		return -EINVAL;

	ret = stm32_pwm_apply_breakinputs(priv, np);
	if (ret)
		return ret;

	stm32_pwm_detect_complementary(priv);

	npwms = stm32_pwm_detect_channels(priv);

	alias = of_alias_get(dev->device_node);
	if (!alias)
		id++;

	for (i = 0; i < npwms; i++) {
		struct pwm_chip *chip = &priv->pwms[i];

		if (alias)
			chip->devname = basprintf("%sch%u", alias, i + 1);
		else
			chip->devname = basprintf("pwm%uch%u", id, i + 1);

		chip->ops = &stm32pwm_ops;
		chip->id = i;

		ret = pwmchip_add(chip, dev);
		if (ret < 0) {
			dev_err(dev, "failed to add pwm chip %d\n", ret);
			return ret;
		}
	}

	return 0;
}

static const struct of_device_id stm32_pwm_of_match[] = {
	{ .compatible = "st,stm32-pwm",	},
	{ /* sentinel */ },
};

static struct driver_d stm32_pwm_driver = {
	.name = "stm32-pwm",
	.probe	= stm32_pwm_probe,
	.of_compatible = stm32_pwm_of_match,
};
coredevice_platform_driver(stm32_pwm_driver);
