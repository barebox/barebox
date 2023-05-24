/*
 * Copyright (C) 2013 Jean-Christophe PLAGNIOL-VILLARD <plagnio@jcrosoft.com>
 *
 * Under GPLv2 only
 */

#ifndef __SMC91111_H__
#define __SMC91111_H__

struct smc91c111_pdata {
	int addr_shift;
	int bus_width;
	bool word_aligned_short_writes;
	int config_setup;
	int control_setup;
};

#endif /* __SMC91111_H__ */
