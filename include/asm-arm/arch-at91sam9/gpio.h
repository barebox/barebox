#ifndef __ASM_ARCH_AT91SAM9_GPIO_H
#define __ASM_ARCH_AT91SAM9_GPIO_H

struct at91_gpio_bank {
	unsigned chipbase;		/* bank's first GPIO number */
	void __iomem *regbase;		/* base of register bank */
	struct at91_gpio_bank *next;	/* bank sharing same IRQ/clock/... */
	unsigned short id;		/* peripheral ID */
	unsigned long offset;		/* offset from system peripheral base */
};

extern int at91_gpio_init(struct at91_gpio_bank *data, int nr_banks);

static inline int gpio_request(unsigned gpio, const char *label)
{
	return 0;
}

static inline void gpio_free(unsigned gpio)
{
}

extern int gpio_direction_input(unsigned gpio);
extern int gpio_direction_output(unsigned gpio, int value);
extern int gpio_get_value(unsigned gpio);
extern void gpio_set_value(unsigned gpio, int value);

#endif /* __ASM_ARCH_AT91SAM9_GPIO_H */
