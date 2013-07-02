/*
 * OF helpers for the GPIO API
 *
 * based on Linux OF_GPIO API
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef __OF_GPIO_H
#define __OF_GPIO_H

/*
 * This is Linux-specific flags. By default controllers' and Linux' mapping
 * match, but GPIO controllers are free to translate their own flags to
 * Linux-specific in their .xlate callback. Though, 1:1 mapping is recommended.
 */
enum of_gpio_flags {
	OF_GPIO_ACTIVE_LOW = 0x1,
};

#ifdef CONFIG_OFTREE
extern int of_get_named_gpio_flags(struct device_node *np,
		const char *list_name, int index, enum of_gpio_flags *flags);

#else /* CONFIG_OFTREE */

static inline int of_get_named_gpio_flags(struct device_node *np,
		const char *list_name, int index, enum of_gpio_flags *flags)
{
	return -ENOSYS;
}

#endif /* CONFIG_OFTREE */

static inline int of_get_named_gpio(struct device_node *np,
				const char *list_name, int index)
{
	return of_get_named_gpio_flags(np, list_name, index, NULL);
}

#endif
