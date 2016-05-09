/*
 * (C) Copyright 2012 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * Under GPLv2
 */

#ifndef __SMC911X_PLATFORM_H_
#define __SMC911X_PLATFORM_H_

/**
 * @brief Platform dependent feature:
 * Pass pointer to this structure as part of device_d -> platform_data
 */
struct smc911x_plat {
	int shift;
	unsigned int flags;
	unsigned int phy_mask;	/* external PHY only: mask out PHYs,
				   e.g. ~(1 << 5) to use PHY addr 5 */
};

#define SMC911X_FORCE_INTERNAL_PHY	0x01
#define SMC911X_FORCE_EXTERNAL_PHY	0x02

#endif /* __SMC911X_PLATFORM_H_ */
