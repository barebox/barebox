#ifndef __SOC_K3_CLK_H
#define __SOC_K3_CLK_H

void ti_k3_pll_init(void __iomem *reg);
int ti_k3_pll_set_rate(void __iomem *reg, unsigned long rate, unsigned long parent_rate);

#endif /* __SOC_K3_CLK_H */
