#ifndef __GPIO_H
#define __GPIO_H

#include <asm/gpio.h>

#define GPIOF_DIR_OUT	(0 << 0)
#define GPIOF_DIR_IN	(1 << 0)

#define GPIOF_INIT_LOW	(0 << 1)
#define GPIOF_INIT_HIGH	(1 << 1)

#define GPIOF_IN		(GPIOF_DIR_IN)
#define GPIOF_OUT_INIT_LOW	(GPIOF_DIR_OUT | GPIOF_INIT_LOW)
#define GPIOF_OUT_INIT_HIGH	(GPIOF_DIR_OUT | GPIOF_INIT_HIGH)

/**
 * struct gpio - a structure describing a GPIO with configuration
 * @gpio:	the GPIO number
 * @flags:	GPIO configuration as specified by GPIOF_*
 * @label:	a literal description string of this GPIO
 */
struct gpio {
	unsigned	gpio;
	unsigned long	flags;
	const char	*label;
};

#ifndef CONFIG_GPIOLIB
static inline int gpio_request(unsigned gpio, const char *label)
{
	return 0;
}

static inline void gpio_free(unsigned gpio)
{
}

static inline int gpio_request_one(unsigned gpio,
					unsigned long flags, const char *label)
{
	return -ENOSYS;
}

static inline int gpio_request_array(const struct gpio *array, size_t num)
{
	return -ENOSYS;
}

static inline void gpio_free_array(const struct gpio *array, size_t num)
{
	/* GPIO can never have been requested */
	WARN_ON(1);
}
#else
int gpio_request(unsigned gpio, const char *label);
void gpio_free(unsigned gpio);
int gpio_request_one(unsigned gpio, unsigned long flags, const char *label);
int gpio_request_array(const struct gpio *array, size_t num);
void gpio_free_array(const struct gpio *array, size_t num);
#endif

struct gpio_chip;

struct gpio_ops {
	int (*request)(struct gpio_chip *chip, unsigned offset);
	void (*free)(struct gpio_chip *chip, unsigned offset);
	int (*direction_input)(struct gpio_chip *chip, unsigned offset);
	int (*direction_output)(struct gpio_chip *chip, unsigned offset, int value);
	int (*get_direction)(struct gpio_chip *chip, unsigned offset);
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
void gpiochip_remove(struct gpio_chip *chip);

int gpio_get_num(struct device_d *dev, int gpio);

#endif /* __GPIO_H */
