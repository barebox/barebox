// SPDX-License-Identifier: GPL-2.0-only
/*
 * Driver for Atmel Pulse Width Modulation Controller
 *
 * Copyright (C) 2013 Atmel Corporation
 *		 Bo Shen <voice.shen@atmel.com>
 * Copyright (C) 2018 Sam Ravnborg <sam@ravnborg.org>
 */

#include <of_device.h>
#include <common.h>
#include <driver.h>
#include <module.h>
#include <linux/printk.h>
#include <stdio.h>
#include <init.h>
#include <pwm.h>
#include <io.h>
#include <of.h>

#include <asm-generic/div64.h>

#include <linux/time.h>
#include <linux/clk.h>


#define PWM_CHANNELS		4

/* The following is global registers for PWM controller */
#define PWM_ENA			0x04
#define PWM_DIS			0x08
#define PWM_SR			0x0C
#define PWM_ISR			0x1C
/* Bit field in SR */
#define PWM_SR_ALL_CH_ON	0x0F

/* The following register is PWM channel related registers */
#define PWM_CH_REG_OFFSET	0x200
#define PWM_CH_REG_SIZE		0x20

#define PWM_CMR			0x0
/* Bit field in CMR */
#define PWM_CMR_CPOL		(1 << 9)
#define PWM_CMR_UPD_CDTY	(1 << 10)
#define PWM_CMR_CPRE_MSK	0xF

/* The following registers for PWM v1 */
#define PWMV1_CDTY		0x04
#define PWMV1_CPRD		0x08
#define PWMV1_CUPD		0x10

/* The following registers for PWM v2 */
#define PWMV2_CDTY		0x04
#define PWMV2_CDTYUPD		0x08
#define PWMV2_CPRD		0x0C
#define PWMV2_CPRDUPD		0x10

/*
 * Max value for duty and period
 *
 * Although the duty and period register is 32 bit,
 * however only the LSB 16 bits are significant.
 */
#define PWM_MAX_DTY		0xFFFF
#define PWM_MAX_PRD		0xFFFF
#define PRD_MAX_PRES		10

struct atmel_pwm_registers {
	u8 period;
	u8 period_upd;
	u8 duty;
	u8 duty_upd;
};

struct atmel_pwm;

struct atmel_pwm_chip {
	struct pwm_chip chip;
	struct atmel_pwm *atmel;
};

struct atmel_pwm {
	struct atmel_pwm_chip atmel_pwm_chip[PWM_CHANNELS];
	const struct atmel_pwm_registers *regs;
	struct clk *clk;
	void __iomem *base;
	struct device_d *dev;
};

static inline struct atmel_pwm_chip *to_atmel_pwm_chip(struct pwm_chip *chip)
{
	return container_of(chip, struct atmel_pwm_chip, chip);
}

static inline void atmel_pwm_writel(struct atmel_pwm_chip *chip,
				    unsigned long offset, unsigned long val)
{
	writel(val, chip->atmel->base + offset);
}

static inline u32 atmel_pwm_ch_readl(struct atmel_pwm_chip *chip,
				     unsigned int ch, unsigned long offset)
{
	unsigned long base = PWM_CH_REG_OFFSET + ch * PWM_CH_REG_SIZE;

	return readl(chip->atmel->base + base + offset);
}

static inline void atmel_pwm_ch_writel(struct atmel_pwm_chip *chip,
				       unsigned int ch, unsigned long offset,
				       unsigned long val)
{
	unsigned long base = PWM_CH_REG_OFFSET + ch * PWM_CH_REG_SIZE;

	writel(val, chip->atmel->base + base + offset);
}

static int atmel_pwm_calculate_cprd_and_pres(struct atmel_pwm_chip *atmel_pwm,
					     int period,
					     unsigned long *cprd, u32 *pres)
{
	unsigned long long cycles = period;
	/* Calculate the period cycles and prescale value */
	cycles *= clk_get_rate(atmel_pwm->atmel->clk);
	do_div(cycles, NSEC_PER_SEC);

	for (*pres = 0; cycles > PWM_MAX_PRD; cycles >>= 1)
		(*pres)++;

	if (*pres > PRD_MAX_PRES) {
		dev_err(atmel_pwm->atmel->dev, "pres exceeds the maximum value\n");
		return -EINVAL;
	}

	*cprd = cycles;

	return 0;
}

static void atmel_pwm_calculate_cdty(int duty, int period,
				     unsigned long cprd, unsigned long *cdty)
{
	unsigned long long cycles = duty;

	cycles *= cprd;
	do_div(cycles, period);
	*cdty = cprd - cycles;
}

static void atmel_pwm_set_cprd_cdty(struct atmel_pwm_chip *atmel_pwm, int ch,
				    unsigned long cprd, unsigned long cdty)
{
	const struct atmel_pwm_registers *regs = atmel_pwm->atmel->regs;

	atmel_pwm_ch_writel(atmel_pwm, ch, regs->duty, cdty);
	atmel_pwm_ch_writel(atmel_pwm, ch, regs->period, cprd);
}

static int atmel_pwm_config(struct pwm_chip *chip, int duty_ns, int period_ns)
{
	struct atmel_pwm_chip *atmel_pwm = to_atmel_pwm_chip(chip);
	unsigned long cprd, cdty;
	u32 pres, val;
	int ret;
	int ch;

	ch = atmel_pwm->chip.id;
	ret = atmel_pwm_calculate_cprd_and_pres(atmel_pwm, period_ns, &cprd, &pres);
	if (ret)
		return ret;

	atmel_pwm_calculate_cdty(duty_ns, period_ns, cprd, &cdty);

	/* It is necessary to preserve CPOL, inside CMR */
	val = atmel_pwm_ch_readl(atmel_pwm, ch, PWM_CMR);
	val = (val & ~PWM_CMR_CPRE_MSK) | (pres & PWM_CMR_CPRE_MSK);
	/* Assuming normal polarity */
	val &= ~PWM_CMR_CPOL;

	atmel_pwm_ch_writel(atmel_pwm, ch, PWM_CMR, val);
	atmel_pwm_set_cprd_cdty(atmel_pwm, ch, cprd, cdty);

	return 0;
}

static int atmel_pwm_enable(struct pwm_chip *chip)
{
	struct atmel_pwm_chip *atmel_pwm = to_atmel_pwm_chip(chip);

	atmel_pwm_writel(atmel_pwm, PWM_ENA, 1 << atmel_pwm->chip.id);
	return 0;
}

static void atmel_pwm_disable(struct pwm_chip *chip)
{
	struct atmel_pwm_chip *atmel_pwm = to_atmel_pwm_chip(chip);

	atmel_pwm_writel(atmel_pwm, PWM_DIS, 1 << atmel_pwm->chip.id);
}

static struct pwm_ops atmel_pwm_ops = {
	.config = atmel_pwm_config,
	.enable = atmel_pwm_enable,
	.disable = atmel_pwm_disable,
};

static const struct atmel_pwm_registers atmel_pwm_regs_v1 = {
	.period		= PWMV1_CPRD,
	.period_upd	= PWMV1_CUPD,
	.duty		= PWMV1_CDTY,
	.duty_upd	= PWMV1_CUPD,
};

static const struct atmel_pwm_registers atmel_pwm_regs_v2 = {
	.period		= PWMV2_CPRD,
	.period_upd	= PWMV2_CPRDUPD,
	.duty		= PWMV2_CDTY,
	.duty_upd	= PWMV2_CDTYUPD,
};

static const struct of_device_id atmel_pwm_dt_ids[] = {
	{
		.compatible = "atmel,at91sam9rl-pwm",
		.data = &atmel_pwm_regs_v1,
	}, {
		.compatible = "atmel,sama5d3-pwm",
		.data = &atmel_pwm_regs_v2,
	}, {
		.compatible = "atmel,sama5d2-pwm",
		.data = &atmel_pwm_regs_v2,
	}, {
		/* sentinel */
	},
};

static int atmel_pwm_probe(struct device_d *dev)
{
	const struct atmel_pwm_registers *regs;
	struct atmel_pwm *atmel_pwm;
	struct resource *res;
	int ret;
	int i;

	ret = dev_get_drvdata(dev, (const void **)&regs);
	if (ret)
		return ret;

	atmel_pwm = xzalloc(sizeof(*atmel_pwm));
	atmel_pwm->regs = regs;
	atmel_pwm->dev = dev;
	atmel_pwm->clk = clk_get(dev, "pwm_clk");
	if (IS_ERR(atmel_pwm->clk))
		return PTR_ERR(atmel_pwm->clk);

	res = dev_request_mem_resource(dev, 0);
	if (IS_ERR(res))
		return PTR_ERR(res);

	atmel_pwm->base = IOMEM(res->start);

	for (i = 0; i < PWM_CHANNELS; i++) {
		struct atmel_pwm_chip *chip = &atmel_pwm->atmel_pwm_chip[i];
		chip->chip.ops = &atmel_pwm_ops;
		chip->chip.devname = basprintf("pwm%d", i);
		chip->chip.id = i;
		chip->atmel = atmel_pwm;

		ret = pwmchip_add(&chip->chip, dev);
		if (ret < 0) {
			dev_err(dev, "failed to add pwm chip[%d] %d\n", i, ret);
			return ret;
		}
	}

	return 0;
}

static struct driver_d atmel_pwm_driver = {
	.name		= "atmel-pwm",
	.of_compatible	= atmel_pwm_dt_ids,
	.probe		= atmel_pwm_probe,
};

coredevice_platform_driver(atmel_pwm_driver);

MODULE_AUTHOR("Bo Shen <voice.shen@atmel.com>");
MODULE_DESCRIPTION("Atmel PWM driver");
MODULE_LICENSE("GPL v2");
