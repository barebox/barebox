#include <common.h>
#include <init.h>
#include <driver.h>
#include <linux/clk.h>
#include <io.h>
#include <of.h>
#include <linux/clkdev.h>
#include <linux/err.h>

#include "clk.h"

void __init imx_check_clocks(struct clk *clks[], unsigned int count)
{
	unsigned i;

	for (i = 0; i < count; i++)
		if (IS_ERR(clks[i]))
			pr_err("i.MX clk %u: register failed with %ld\n",
			       i, PTR_ERR(clks[i]));
}

