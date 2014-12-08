#ifndef __ASM_GENERIC_GPIO_H
#define __ASM_GENERIC_GPIO_H

void gpio_set_value(unsigned gpio, int value);
int gpio_get_value(unsigned gpio);
int gpio_direction_output(unsigned gpio, int value);
int gpio_direction_input(unsigned gpio);

#endif /* __ASM_GENERIC_GPIO_H */
