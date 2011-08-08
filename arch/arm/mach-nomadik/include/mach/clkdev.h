#ifndef __ASM_MACH_CLKDEV_H
#define __ASM_MACH_CLKDEV_H

#define __clk_get(clk) ({ 1; })
#define __clk_put(clk) do { } while (0)

#define CLKDEV_CON_ID(_id, _clk)			\
	{						\
		.con_id = _id,				\
		.clk = _clk,				\
	}

#endif
