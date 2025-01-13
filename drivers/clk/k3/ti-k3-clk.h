#ifndef __TI_K3_CLK_H
#define __TI_K3_CLK_H

struct clk *clk_register_ti_k3_pll(const char *name, const char *parent_name,
				   void __iomem *reg);

#endif

