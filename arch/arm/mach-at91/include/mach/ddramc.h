// SPDX-License-Identifier: BSD-1-Clause
/*
 * Copyright (c) 2006, Atmel Corporation
 */
#ifndef __DDRAMC_H__
#define __DDRAMC_H__

/* Note: reserved bits must always be zeroed */
struct at91_ddramc_register {
	unsigned long mdr;
	unsigned long cr;
	unsigned long rtr;
	unsigned long t0pr;
	unsigned long t1pr;
	unsigned long t2pr;
	unsigned long lpr;
	unsigned long lpddr2_lpr;
	unsigned long tim_calr;
	unsigned long cal_mr4r;
};

void at91_ddram_initialize(void __iomem *base_address,
			   void __iomem *ram_address,
			   const struct at91_ddramc_register *ddramc_config);

void at91_lpddr2_sdram_initialize(void __iomem *base_address,
				  void __iomem *ram_address,
				  const struct at91_ddramc_register *ddramc_config);


void at91_lpddr1_sdram_initialize(void __iomem *base_address,
				  void __iomem *ram_address,
				  const struct at91_ddramc_register *ddramc_config);

void __noreturn sama5d2_barebox_entry(unsigned int r4, void *boarddata);
void __noreturn sama5d3_barebox_entry(unsigned int r4, void *boarddata);

#endif /* #ifndef __DDRAMC_H__ */
