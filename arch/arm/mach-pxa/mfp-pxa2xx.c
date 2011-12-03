/*
 *  linux/arch/arm/mach-pxa/mfp-pxa2xx.c
 *
 *  PXA2xx pin mux configuration support
 *
 *  The GPIOs on PXA2xx can be configured as one of many alternate
 *  functions, this is by concept samilar to the MFP configuration
 *  on PXA3xx,  what's more important, the low power pin state and
 *  wakeup detection are also supported by the same framework.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */

#include <common.h>
#include <errno.h>
#include <init.h>

#include <mach/gpio.h>
#include <mach/hardware.h>
#include <mach/mfp-pxa2xx.h>
#include <mach/pxa-regs.h>

#define PGSR(x)		__REG2(0x40F00020, (x) << 2)
#define __GAFR(u, x)	__REG2((u) ? 0x40E00058 : 0x40E00054, (x) << 3)
#define GAFR_L(x)	__GAFR(0, x)
#define GAFR_U(x)	__GAFR(1, x)

struct gpio_desc {
	unsigned	valid:1;
	unsigned	dir_inverted:1;
	unsigned long	config;
};

static struct gpio_desc gpio_desc[MFP_PIN_GPIO127 + 1];

static unsigned long gpdr_lpm[4];

static int __mfp_config_gpio(unsigned gpio, unsigned long c)
{
	unsigned long gafr, mask = GPIO_bit(gpio);
	int bank = gpio_to_bank(gpio);
	int uorl = !!(gpio & 0x10); /* GAFRx_U or GAFRx_L ? */
	int shft = (gpio & 0xf) << 1;
	int fn = MFP_AF(c);
	int is_out = (c & MFP_DIR_OUT) ? 1 : 0;

	if (fn > 3)
		return -EINVAL;

	/* alternate function and direction at run-time */
	gafr = (uorl == 0) ? GAFR_L(bank) : GAFR_U(bank);
	gafr = (gafr & ~(0x3 << shft)) | (fn << shft);

	if (uorl == 0)
		GAFR_L(bank) = gafr;
	else
		GAFR_U(bank) = gafr;

	if (is_out ^ gpio_desc[gpio].dir_inverted)
		GPDR(gpio) |= mask;
	else
		GPDR(gpio) &= ~mask;

	/* alternate function and direction at low power mode */
	switch (c & MFP_LPM_STATE_MASK) {
	case MFP_LPM_DRIVE_HIGH:
		PGSR(bank) |= mask;
		is_out = 1;
		break;
	case MFP_LPM_DRIVE_LOW:
		PGSR(bank) &= ~mask;
		is_out = 1;
		break;
	case MFP_LPM_DEFAULT:
		break;
	default:
		/* warning and fall through, treat as MFP_LPM_DEFAULT */
		pr_warning("%s: GPIO%d: unsupported low power mode\n",
				__func__, gpio);
		break;
	}

	if (is_out ^ gpio_desc[gpio].dir_inverted)
		gpdr_lpm[bank] |= mask;
	else
		gpdr_lpm[bank] &= ~mask;

	return 0;
}

static inline int __mfp_validate(int mfp)
{
	int gpio = mfp_to_gpio(mfp);

	if ((mfp > MFP_PIN_GPIO127) || !gpio_desc[gpio].valid) {
		pr_warning("%s: GPIO%d is invalid pin\n", __func__, gpio);
		return -1;
	}

	return gpio;
}

void pxa2xx_mfp_config(unsigned long *mfp_cfgs, int num)
{
	unsigned long *c;
	int i, gpio;

	for (i = 0, c = mfp_cfgs; i < num; i++, c++) {

		gpio = __mfp_validate(MFP_PIN(*c));
		if (gpio < 0)
			continue;

		gpio_desc[gpio].config = *c;
		__mfp_config_gpio(gpio, *c);
	}
}

void pxa2xx_mfp_set_lpm(int mfp, unsigned long lpm)
{
	unsigned long c;
	int gpio;

	gpio = __mfp_validate(mfp);
	if (gpio < 0)
		return;

	c = gpio_desc[gpio].config;
	c = (c & ~MFP_LPM_STATE_MASK) | lpm;
	__mfp_config_gpio(gpio, c);
}

static void __init pxa25x_mfp_init(void)
{
	int i;

	for (i = 0; i <= pxa_last_gpio; i++)
		gpio_desc[i].valid = 1;

	/* PXA26x has additional 4 GPIOs (86/87/88/89) which has the
	 * direction bit inverted in GPDR2. See PXA26x DM 4.1.1.
	 */
	for (i = 86; i <= pxa_last_gpio; i++)
		gpio_desc[i].dir_inverted = 1;
}

static void __init pxa27x_mfp_init(void)
{
	int i;

	for (i = 0; i <= pxa_last_gpio; i++) {
		/*
		 * skip GPIO2, 5, 6, 7, 8, they are not
		 * valid pins allow configuration
		 */
		if (i == 2 || i == 5 || i == 6 || i == 7 || i == 8)
			continue;

		gpio_desc[i].valid = 1;
	}
}

static int __init pxa2xx_mfp_init(void)
{
	int i;

	if (!cpu_is_pxa2xx())
		return 0;

	if (cpu_is_pxa25x())
		pxa25x_mfp_init();

	if (cpu_is_pxa27x()) {
		pxa_init_gpio(2, 120);
		pxa27x_mfp_init();
	}

	/* clear RDH bit to enable GPIO receivers after reset/sleep exit */
	PSSR = PSSR_RDH;

	/* initialize gafr_run[], pgsr_lpm[] from existing values */
	for (i = 0; i <= gpio_to_bank(pxa_last_gpio); i++)
		gpdr_lpm[i] = GPDR(i * 32);

	return 0;
}
postcore_initcall(pxa2xx_mfp_init);
