/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef __GPIO_INTEL_H
#define __GPIO_INTEL_H

#include <linux/types.h>

struct gpio_intel_platform_data {
	resource_size_t community_base;
	resource_size_t community_size;
	unsigned int	ngpios;
};

static inline struct device *add_intel_gpio_device(
	struct gpio_intel_platform_data *pdata
)
{
	return add_generic_device("intel-gpio", DEVICE_ID_DYNAMIC, NULL,
		pdata->community_base, pdata->community_size,
		IORESOURCE_MEM, pdata);
}

#endif /* __GPIO_INTEL_H */
