// SPDX-License-Identifier: GPL-2.0-only

/*
 * simple driver for PWM (Pulse Width Modulator) controller
 *
 * 2008-02-13	initial version eric miao <eric.miao@marvell.com>
 * 2012         Robert Jarzmik <robert.jarzmik@free.fr>
 */

#include <common.h>
#include <init.h>
#include <errno.h>
#include <io.h>
#include <pwm.h>

#include <mach/pxa/hardware.h>
#include <mach/pxa/clock.h>
#include <mach/pxa/pxa-regs.h>
#include <mach/pxa/regs-pwm.h>
#include <linux/math64.h>
#include <linux/compiler.h>

/* PWM registers and bits definitions */
#define PWMCR		(0x00)
#define PWMDCR		(0x04)
#define PWMPCR		(0x08)

#define PWMCR_SD	(1 << 6)
#define PWMDCR_FD	(1 << 10)

struct pxa_pwm_chip {
	struct pwm_chip chip;
	void __iomem *iobase;
	int id;
};

static struct pxa_pwm_chip *to_pxa_pwm_chip(struct pwm_chip *chip)
{
	return container_of(chip, struct pxa_pwm_chip, chip);
}

static int pxa_pwm_enable(struct pxa_pwm_chip *pxa_pwm)
{
	switch (pxa_pwm->id) {
	case 0:
	case 2:
		CKEN |= CKEN_PWM0;
		break;
	case 1:
	case 3:
		CKEN |= CKEN_PWM1;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static void pxa_pwm_disable(struct pxa_pwm_chip *pxa_pwm)
{
	switch (pxa_pwm->id) {
	case 0:
	case 2:
		CKEN &= ~CKEN_PWM0;
		break;
	case 1:
	case 3:
		CKEN &= ~CKEN_PWM1;
		break;
	default:
		break;
	}
}

/*
 * period_ns    = 10^9 * (PRESCALE + 1) * (PV + 1) / PWM_CLK_RATE
 * duty_ns      = 10^9 * (PRESCALE + 1) * DC / PWM_CLK_RATE
 * PWM_CLK_RATE = 13 MHz
 */
static int pxa_pwm_apply(struct pwm_chip *chip, const struct pwm_state *state)
{
	unsigned long long c;
	unsigned long period_cycles, prescale, pv, dc;
	struct pxa_pwm_chip *pxa_pwm = to_pxa_pwm_chip(chip);
	bool enabled;

	enabled = chip->state.p_enable;

	if (enabled && !state->p_enable) {
		pxa_pwm_disable(pxa_pwm);
		return 0;
	}

	c = pxa_get_pwmclk();
	c = c * state->period_ns;
	do_div(c, 1000000000);
	period_cycles = c;

	if (period_cycles < 1)
		period_cycles = 1;
	prescale = (period_cycles - 1) / 1024;
	pv = period_cycles / (prescale + 1) - 1;

	if (prescale > 63)
		return -EINVAL;

	if (state->duty_ns == state->period_ns)
		dc = PWMDCR_FD;
	else
		dc = (pv + 1) * state->duty_ns / state->period_ns;

	/* NOTE: the clock to PWM has to be enabled first
	 * before writing to the registers
	 */
	writel(prescale, pxa_pwm->iobase + PWMCR);
	writel(dc, pxa_pwm->iobase + PWMDCR);
	writel(pv, pxa_pwm->iobase + PWMPCR);

	if (!enabled && state->p_enable) {
		pxa_pwm_enable(pxa_pwm);
		return 0;
	}

	return 0;
}

static struct pwm_ops pxa_pwm_ops = {
	.apply = pxa_pwm_apply,
};

static int pxa_pwm_probe(struct device *dev)
{
	struct resource *iores;
	struct pxa_pwm_chip *chip;

	chip = xzalloc(sizeof(*chip));
	chip->chip.devname = basprintf("pwm%d", dev->id);
	chip->chip.ops = &pxa_pwm_ops;
	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	chip->iobase = IOMEM(iores->start);
	chip->id = dev->id;
	chip->chip.dev = dev;
	dev->priv = chip;

	return pwmchip_add(&chip->chip);
}

static struct driver pxa_pwm_driver = {
	.name  = "pxa_pwm",
	.probe = pxa_pwm_probe,
};

static int __init pxa_pwm_init_driver(void)
{
	CKEN &= ~CKEN_PWM0 & ~CKEN_PWM1;
	platform_driver_register(&pxa_pwm_driver);
	return 0;
}

device_initcall(pxa_pwm_init_driver);
