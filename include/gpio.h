#ifndef __GPIO_H
#define __GPIO_H

#include <asm/gpio.h>

static inline int gpio_request(unsigned gpio, const char *label)
{
       return 0;
}

static inline void gpio_free(unsigned gpio)
{
}

struct gpio_chip;

struct gpio_ops {
	int (*direction_input)(struct gpio_chip *chip, unsigned offset);
	int (*direction_output)(struct gpio_chip *chip, unsigned offset, int value);
	int (*get)(struct gpio_chip *chip, unsigned offset);
	void (*set)(struct gpio_chip *chip, unsigned offset, int value);
};

struct gpio_chip {
	struct device_d *dev;

	int base;
	int ngpio;

	struct gpio_ops *ops;

	struct list_head list;
};

int gpiochip_add(struct gpio_chip *chip);

int gpio_get_num(struct device_d *dev, int gpio);

#endif /* __GPIO_H */
