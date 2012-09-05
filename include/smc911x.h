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
};

#endif /* __SMC911X_PLATFORM_H_ */
