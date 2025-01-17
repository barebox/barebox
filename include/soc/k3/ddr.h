#ifndef __SOC_K3_DDR_H
#define __SOC_K3_DDR_H

#include <linux/types.h>

#define LPDDR4_INTR_CTL_REG_COUNT (423U)
#define LPDDR4_INTR_PHY_INDEP_REG_COUNT (345U)
#define LPDDR4_INTR_PHY_REG_COUNT (1406U)

struct reginitdata {
	u32 *regs;
	u16 *offset;
	u32 num;
};

struct k3_ddr_initdata {
	struct reginitdata *ctl_regs;
	struct reginitdata *pi_regs;
	struct reginitdata *phy_regs;
	unsigned long freq0, freq1, freq2;
	unsigned int fhs_cnt;
};

int k3_ddrss_init(struct k3_ddr_initdata *);

#endif /* __SOC_K3_DDR_H */
