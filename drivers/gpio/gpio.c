#include <common.h>
#include <gpio.h>
#include <errno.h>

static LIST_HEAD(chip_list);

static struct gpio_chip *gpio_desc[ARCH_NR_GPIOS];

void gpio_set_value(unsigned gpio, int value)
{
	struct gpio_chip *chip = gpio_desc[gpio];

	if (!gpio_is_valid(gpio))
		return;
	if (!chip)
		return;
	if (!chip->ops->set)
		return;
	chip->ops->set(chip, gpio - chip->base, value);
}
EXPORT_SYMBOL(gpio_set_value);

int gpio_get_value(unsigned gpio)
{
	struct gpio_chip *chip = gpio_desc[gpio];

	if (!gpio_is_valid(gpio))
		return -EINVAL;
	if (!chip)
		return -ENODEV;
	if (!chip->ops->get)
		return -ENOSYS;
	return chip->ops->get(chip, gpio - chip->base);
}
EXPORT_SYMBOL(gpio_get_value);

int gpio_direction_output(unsigned gpio, int value)
{
	struct gpio_chip *chip = gpio_desc[gpio];

	if (!gpio_is_valid(gpio))
		return -EINVAL;
	if (!chip)
		return -ENODEV;
	if (!chip->ops->direction_output)
		return -ENOSYS;
	return chip->ops->direction_output(chip, gpio - chip->base, value);
}
EXPORT_SYMBOL(gpio_direction_output);

int gpio_direction_input(unsigned gpio)
{
	struct gpio_chip *chip = gpio_desc[gpio];

	if (!gpio_is_valid(gpio))
		return -EINVAL;
	if (!chip)
		return -ENODEV;
	if (!chip->ops->direction_input)
		return -ENOSYS;
	return chip->ops->direction_input(chip, gpio - chip->base);
}
EXPORT_SYMBOL(gpio_direction_input);

static int gpiochip_find_base(int start, int ngpio)
{
	int i;
	int spare = 0;
	int base = -ENOSPC;

	if (start < 0)
		start = 0;

	for (i = start; i < ARCH_NR_GPIOS; i++) {
		struct gpio_chip *chip = gpio_desc[i];

		if (!chip) {
			spare++;
			if (spare == ngpio) {
				base = i + 1 - ngpio;
				break;
			}
		} else {
			spare = 0;
			i += chip->ngpio - 1;
		}
	}

	if (gpio_is_valid(base))
		debug("%s: found new base at %d\n", __func__, base);
	return base;
}

int gpiochip_add(struct gpio_chip *chip)
{
	int base, i;

	base = gpiochip_find_base(chip->base, chip->ngpio);
	if (base < 0)
		return base;

	if (chip->base >= 0 && chip->base != base)
		return -EBUSY;

	chip->base = base;

	list_add_tail(&chip->list, &chip_list);

	for (i = chip->base; i < chip->base + chip->ngpio; i++)
		gpio_desc[i] = chip;

	return 0;
}

int gpio_get_num(struct device_d *dev, int gpio)
{
	struct gpio_chip *chip;

	list_for_each_entry(chip, &chip_list, list) {
		if (chip->dev == dev)
			return chip->base + gpio;
	}

	return -ENODEV;
}
