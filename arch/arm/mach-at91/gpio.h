/*
 * Copyright (C) 2011-2012 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * Under GPLv2 only
 */

#ifndef __AT91_GPIO_H__
#define __AT91_GPIO_H__

#ifndef __gpio_init
#define __gpio_init
#endif

#define MAX_NB_GPIO_PER_BANK	32

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

static __gpio_init void at91_mux_disable_interrupt(void __iomem *pio, unsigned mask)
{
	__raw_writel(mask, pio + PIO_IDR);
}

static __gpio_init void at91_mux_set_pullup(void __iomem *pio, unsigned mask, bool on)
{
	__raw_writel(mask, pio + (on ? PIO_PUER : PIO_PUDR));
}

static __gpio_init void at91_mux_set_multidrive(void __iomem *pio, unsigned mask, bool on)
{
	__raw_writel(mask, pio + (on ? PIO_MDER : PIO_MDDR));
}

static __gpio_init void at91_mux_set_A_periph(void __iomem *pio, unsigned mask)
{
	__raw_writel(mask, pio + PIO_ASR);
}

static __gpio_init void at91_mux_set_B_periph(void __iomem *pio, unsigned mask)
{
	__raw_writel(mask, pio + PIO_BSR);
}

static __gpio_init void at91_mux_pio3_set_A_periph(void __iomem *pio, unsigned mask)
{

	__raw_writel(__raw_readl(pio + PIO_ABCDSR1) & ~mask,
						pio + PIO_ABCDSR1);
	__raw_writel(__raw_readl(pio + PIO_ABCDSR2) & ~mask,
						pio + PIO_ABCDSR2);
}

static __gpio_init void at91_mux_pio3_set_B_periph(void __iomem *pio, unsigned mask)
{
	__raw_writel(__raw_readl(pio + PIO_ABCDSR1) | mask,
						pio + PIO_ABCDSR1);
	__raw_writel(__raw_readl(pio + PIO_ABCDSR2) & ~mask,
						pio + PIO_ABCDSR2);
}

static __gpio_init void at91_mux_pio3_set_C_periph(void __iomem *pio, unsigned mask)
{
	__raw_writel(__raw_readl(pio + PIO_ABCDSR1) & ~mask, pio + PIO_ABCDSR1);
	__raw_writel(__raw_readl(pio + PIO_ABCDSR2) | mask, pio + PIO_ABCDSR2);
}

static __gpio_init void at91_mux_pio3_set_D_periph(void __iomem *pio, unsigned mask)
{
	__raw_writel(__raw_readl(pio + PIO_ABCDSR1) | mask, pio + PIO_ABCDSR1);
	__raw_writel(__raw_readl(pio + PIO_ABCDSR2) | mask, pio + PIO_ABCDSR2);
}

static __gpio_init void at91_mux_set_deglitch(void __iomem *pio, unsigned mask, bool is_on)
{
	__raw_writel(mask, pio + (is_on ? PIO_IFER : PIO_IFDR));
}

static __gpio_init void at91_mux_pio3_set_deglitch(void __iomem *pio, unsigned mask, bool is_on)
{
	if (is_on)
		__raw_writel(mask, pio + PIO_IFSCDR);
	at91_mux_set_deglitch(pio, mask, is_on);
}

static __gpio_init void at91_mux_pio3_set_debounce(void __iomem *pio, unsigned mask,
				bool is_on, u32 div)
{
	if (is_on) {
		__raw_writel(mask, pio + PIO_IFSCER);
		__raw_writel(div & PIO_SCDR_DIV, pio + PIO_SCDR);
		__raw_writel(mask, pio + PIO_IFER);
	} else {
		__raw_writel(mask, pio + PIO_IFDR);
	}
}

static __gpio_init void at91_mux_pio3_set_pulldown(void __iomem *pio, unsigned mask, bool is_on)
{
	__raw_writel(mask, pio + (is_on ? PIO_PPDER : PIO_PPDDR));
}

static __gpio_init void at91_mux_pio3_disable_schmitt_trig(void __iomem *pio, unsigned mask)
{
	__raw_writel(__raw_readl(pio + PIO_SCHMITT) | mask, pio + PIO_SCHMITT);
}

static __gpio_init void at91_mux_gpio_disable(void __iomem *pio, unsigned mask)
{
	__raw_writel(mask, pio + PIO_PDR);
}

static __gpio_init void at91_mux_gpio_enable(void __iomem *pio, unsigned mask)
{
	__raw_writel(mask, pio + PIO_PER);
}

static __gpio_init void at91_mux_gpio_input(void __iomem *pio, unsigned mask, bool input)
{
	__raw_writel(mask, pio + (input ? PIO_ODR : PIO_OER));
}

static __gpio_init void at91_mux_gpio_set(void __iomem *pio, unsigned mask,
int value)
{
	__raw_writel(mask, pio + (value ? PIO_SODR : PIO_CODR));
}

static __gpio_init int at91_mux_gpio_get(void __iomem *pio, unsigned mask)
{
       u32 pdsr;

       pdsr = __raw_readl(pio + PIO_PDSR);
       return (pdsr & mask) != 0;
}

#endif /* __AT91_GPIO_H__ */
