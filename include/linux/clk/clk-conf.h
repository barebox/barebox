/*
 * Copyright (C) 2014 Samsung Electronics Co., Ltd.
 * Sylwester Nawrocki <s.nawrocki@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef __CLK_CONF_H
#define __CLK_CONF_H

#include <linux/types.h>

#if defined(CONFIG_OFTREE) && defined(CONFIG_COMMON_CLK_OF_PROVIDER)

struct device_node;
int of_clk_set_defaults(struct device_node *node, bool clk_supplier);

#else

static inline int of_clk_set_defaults(struct device_node *node,
				      bool clk_supplier)
{
	return 0;
}

#endif
#endif /* __CLK_CONF_H */
