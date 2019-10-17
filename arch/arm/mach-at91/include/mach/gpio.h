/*
 * Copyright (C) 2011-2012 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * Under GPLv2 only
 */

#ifndef __AT91_GPIO_H__
#define __AT91_GPIO_H__

#include <dt-bindings/gpio/gpio.h>
#include <asm/io.h>
#include <mach/at91_pio.h>

#define MAX_NB_GPIO_PER_BANK	32

enum at91_mux {
	AT91_MUX_GPIO = 0,
	AT91_MUX_PERIPH_A = 1,
	AT91_MUX_PERIPH_B = 2,
	AT91_MUX_PERIPH_C = 3,
	AT91_MUX_PERIPH_D = 4,
	AT91_MUX_PERIPH_E = 5,
	AT91_MUX_PERIPH_F = 6,
	AT91_MUX_PERIPH_G = 7,
};

static inline unsigned pin_to_bank(unsigned pin)
{
	return pin / MAX_NB_GPIO_PER_BANK;
}

static inline unsigned pin_to_bank_offset(unsigned pin)
{
	return pin % MAX_NB_GPIO_PER_BANK;
}

static inline unsigned pin_to_mask(unsigned pin)
{
	return 1 << pin_to_bank_offset(pin);
}

static inline void at91_mux_disable_interrupt(void __iomem *pio, unsigned mask)
{
	writel(mask, pio + PIO_IDR);
}

static inline void at91_mux_set_pullup(void __iomem *pio, unsigned mask, bool on)
{
	writel(mask, pio + (on ? PIO_PUER : PIO_PUDR));
}

static inline void at91_mux_set_multidrive(void __iomem *pio, unsigned mask, bool on)
{
	writel(mask, pio + (on ? PIO_MDER : PIO_MDDR));
}

static inline void at91_mux_set_A_periph(void __iomem *pio, unsigned mask)
{
	writel(mask, pio + PIO_ASR);
}

static inline void at91_mux_set_B_periph(void __iomem *pio, unsigned mask)
{
	writel(mask, pio + PIO_BSR);
}

static inline void at91_mux_pio3_set_A_periph(void __iomem *pio, unsigned mask)
{
	writel(readl(pio + PIO_ABCDSR1) & ~mask, pio + PIO_ABCDSR1);
	writel(readl(pio + PIO_ABCDSR2) & ~mask, pio + PIO_ABCDSR2);
}

static inline void at91_mux_pio3_set_B_periph(void __iomem *pio, unsigned mask)
{
	writel(readl(pio + PIO_ABCDSR1) |  mask, pio + PIO_ABCDSR1);
	writel(readl(pio + PIO_ABCDSR2) & ~mask, pio + PIO_ABCDSR2);
}

static inline void at91_mux_pio3_set_C_periph(void __iomem *pio, unsigned mask)
{
	writel(readl(pio + PIO_ABCDSR1) & ~mask, pio + PIO_ABCDSR1);
	writel(readl(pio + PIO_ABCDSR2) |  mask, pio + PIO_ABCDSR2);
}

static inline void at91_mux_pio3_set_D_periph(void __iomem *pio, unsigned mask)
{
	writel(readl(pio + PIO_ABCDSR1) | mask, pio + PIO_ABCDSR1);
	writel(readl(pio + PIO_ABCDSR2) | mask, pio + PIO_ABCDSR2);
}

static inline void at91_mux_set_deglitch(void __iomem *pio, unsigned mask, bool is_on)
{
	writel(mask, pio + (is_on ? PIO_IFER : PIO_IFDR));
}

static inline void at91_mux_pio3_set_deglitch(void __iomem *pio, unsigned mask, bool is_on)
{
	if (is_on)
		writel(mask, pio + PIO_IFSCDR);
	at91_mux_set_deglitch(pio, mask, is_on);
}

static inline void at91_mux_pio3_set_debounce(void __iomem *pio, unsigned mask,
				bool is_on, u32 div)
{
	if (is_on) {
		writel(mask, pio + PIO_IFSCER);
		writel(div & PIO_SCDR_DIV, pio + PIO_SCDR);
		writel(mask, pio + PIO_IFER);
	} else {
		writel(mask, pio + PIO_IFDR);
	}
}

static inline void at91_mux_pio3_set_pulldown(void __iomem *pio, unsigned mask, bool is_on)
{
	writel(mask, pio + (is_on ? PIO_PPDER : PIO_PPDDR));
}

static inline void at91_mux_pio3_disable_schmitt_trig(void __iomem *pio, unsigned mask)
{
	writel(readl(pio + PIO_SCHMITT) | mask, pio + PIO_SCHMITT);
}

static inline void at91_mux_gpio_disable(void __iomem *pio, unsigned mask)
{
	writel(mask, pio + PIO_PDR);
}

static inline void at91_mux_gpio_enable(void __iomem *pio, unsigned mask)
{
	writel(mask, pio + PIO_PER);
}

static inline void at91_mux_gpio_input(void __iomem *pio, unsigned mask, bool input)
{
	writel(mask, pio + (input ? PIO_ODR : PIO_OER));
}

static inline void at91_mux_gpio_set(void __iomem *pio, unsigned mask,
int value)
{
	writel(mask, pio + (value ? PIO_SODR : PIO_CODR));
}

static inline int at91_mux_gpio_get(void __iomem *pio, unsigned mask)
{
	u32 pdsr;

	pdsr = readl(pio + PIO_PDSR);
	return (pdsr & mask) != 0;
}

static inline void at91_mux_pio3_pin(void __iomem *pio, unsigned mask,
				     enum at91_mux mux, int gpio_state)
{
	at91_mux_disable_interrupt(pio, mask);

	switch(mux) {
	case AT91_MUX_GPIO:
		at91_mux_gpio_enable(pio, mask);
		break;
	case AT91_MUX_PERIPH_A:
		at91_mux_pio3_set_A_periph(pio, mask);
		break;
	case AT91_MUX_PERIPH_B:
		at91_mux_pio3_set_B_periph(pio, mask);
		break;
	case AT91_MUX_PERIPH_C:
		at91_mux_pio3_set_C_periph(pio, mask);
		break;
	case AT91_MUX_PERIPH_D:
		at91_mux_pio3_set_D_periph(pio, mask);
		break;
	default:
		/* ignore everything else */
		break;
	}
	if (mux != AT91_MUX_GPIO)
		at91_mux_gpio_disable(pio, mask);

	at91_mux_set_pullup(pio, mask, gpio_state & GPIO_PULL_UP);
	at91_mux_pio3_set_pulldown(pio, mask, gpio_state & GPIO_PULL_DOWN);
}

/* helpers for PIO4 pinctrl (>= sama5d2) */

static inline void at91_mux_pio4_set_periph(void __iomem *pio, unsigned mask, u32 func)
{
	writel(mask, pio + PIO4_MSKR);
	writel(func, pio + PIO4_CFGR);
}

static inline void at91_mux_pio4_set_A_periph(void __iomem *pio, unsigned mask)
{
	at91_mux_pio4_set_periph(pio, mask, AT91_MUX_PERIPH_A);
}

static inline void at91_mux_pio4_set_B_periph(void __iomem *pio, unsigned mask)
{
	at91_mux_pio4_set_periph(pio, mask, AT91_MUX_PERIPH_B);
}

static inline void at91_mux_pio4_set_C_periph(void __iomem *pio, unsigned mask)
{
	at91_mux_pio4_set_periph(pio, mask, AT91_MUX_PERIPH_C);
}

static inline void at91_mux_pio4_set_D_periph(void __iomem *pio, unsigned mask)
{
	at91_mux_pio4_set_periph(pio, mask, AT91_MUX_PERIPH_D);
}

static inline void at91_mux_pio4_set_E_periph(void __iomem *pio, unsigned mask)
{
	at91_mux_pio4_set_periph(pio, mask, AT91_MUX_PERIPH_E);
}

static inline void at91_mux_pio4_set_F_periph(void __iomem *pio, unsigned mask)
{
	at91_mux_pio4_set_periph(pio, mask, AT91_MUX_PERIPH_F);
}

static inline void at91_mux_pio4_set_G_periph(void __iomem *pio, unsigned mask)
{
	at91_mux_pio4_set_periph(pio, mask, AT91_MUX_PERIPH_G);
}

static inline void at91_mux_pio4_set_func(void __iomem *pio,
					  unsigned pin_mask,
					  unsigned cfgr_and_mask,
					  unsigned cfgr_or_mask)
{
	u32 reg;
	writel(pin_mask, pio + PIO4_MSKR);
	reg = readl(pio + PIO4_CFGR);
	reg &= cfgr_and_mask;
	reg |= cfgr_or_mask;
	writel(reg, pio + PIO4_CFGR);
}

static inline void at91_mux_pio4_set_bistate(void __iomem *pio,
					     unsigned pin_mask,
					     unsigned func_mask,
					     bool is_on)
{
	at91_mux_pio4_set_func(pio, pin_mask, ~func_mask,
			       is_on ? func_mask : 0);
}

static inline void at91_mux_pio4_set_deglitch(void __iomem *pio, unsigned mask, bool is_on)
{
	at91_mux_pio4_set_bistate(pio, mask, PIO4_IFEN_MASK, is_on);
}

static inline void at91_mux_pio4_set_debounce(void __iomem *pio, unsigned mask,
					      bool is_on, u32 div)
{
	at91_mux_pio4_set_bistate(pio, mask, PIO4_IFEN_MASK, is_on);
	at91_mux_pio4_set_bistate(pio, mask, PIO4_IFSCEN_MASK, is_on);
}

static inline void at91_mux_pio4_set_pulldown(void __iomem *pio, unsigned mask, bool is_on)
{
	at91_mux_pio4_set_bistate(pio, mask, PIO4_PDEN_MASK, is_on);
}

static inline void at91_mux_pio4_disable_schmitt_trig(void __iomem *pio, unsigned mask)
{
	at91_mux_pio4_set_bistate(pio, mask, PIO4_SCHMITT_MASK, false);
}

static inline void at91_mux_gpio4_enable(void __iomem *pio, unsigned mask)
{
	at91_mux_pio4_set_func(pio, mask, ~PIO4_CFGR_FUNC_MASK, AT91_MUX_GPIO);
}

static inline void at91_mux_gpio4_input(void __iomem *pio, unsigned mask, bool input)
{
	u32 cfgr;

	writel(mask, pio + PIO4_MSKR);

	cfgr = readl(pio + PIO4_CFGR);
	if (input)
		cfgr &= ~PIO4_DIR_MASK;
	else
		cfgr |= PIO4_DIR_MASK;
	writel(cfgr, pio + PIO4_CFGR);
}

static inline void at91_mux_gpio4_set(void __iomem *pio, unsigned mask,
				      int value)
{
	writel(mask, pio + (value ? PIO4_SODR : PIO4_CODR));
}

static inline int at91_mux_gpio4_get(void __iomem *pio, unsigned mask)
{
	u32 pdsr;

	pdsr = readl(pio + PIO4_PDSR);
	return (pdsr & mask) != 0;
}

#endif /* __AT91_GPIO_H__ */
