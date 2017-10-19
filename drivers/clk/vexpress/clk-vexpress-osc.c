/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <common.h>
#include <malloc.h>
#include <linux/clk.h>
#include <linux/err.h>

/*
 * This represents the vexpress-osc as a fixed clock, which isn't really
 * accurate, as this clock allows rate changes in real implementations. As those
 * would need access to the config bus, a whole lot more infrastructure would be
 * needed. We skip this complication for now, as we don't have a use-case, yet.
 */
static int vexpress_osc_setup(struct device_node *node)
{
	struct clk *clk;
	u32 range[2];
	const char *name;

	if (of_property_read_u32_array(node, "freq-range", range,
				       ARRAY_SIZE(range)))
		return -EINVAL;

	if (of_property_read_string(node, "clock-output-names", &name))
		return -EINVAL;

	clk = clk_fixed(name, range[0]);
	if (IS_ERR(clk))
		return PTR_ERR(clk);

	return of_clk_add_provider(node, of_clk_src_simple_get, clk);
}
CLK_OF_DECLARE(vexpress_osc, "arm,vexpress-osc", vexpress_osc_setup);
