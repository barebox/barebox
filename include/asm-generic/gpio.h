#ifndef __ASM_GENERIC_GPIO_H
#define __ASM_GENERIC_GPIO_H

#define ARCH_NR_GPIOS 256

static inline int gpio_is_valid(int gpio)
{
	if (gpio < 0)
		return 0;
	if (gpio < ARCH_NR_GPIOS)
		return 1;
	return 0;
}

void gpio_set_value(unsigned gpio, int value);
int gpio_get_value(unsigned gpio);
int gpio_direction_output(unsigned gpio, int value);
int gpio_direction_input(unsigned gpio);

#endif /* __ASM_GENERIC_GPIO_H */
