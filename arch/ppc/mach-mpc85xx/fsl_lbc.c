/*
 * Copyright 2010-2011 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * Version 2 as published by the Free Software Foundation.
 */

#include <common.h>
#include <asm/fsl_lbc.h>
#include <mach/immap_85xx.h>

void fsl_init_early_memctl_regs(void)
{
	fsl_set_lbc_br(0, CFG_BR0_PRELIM);
	fsl_set_lbc_or(0, CFG_OR0_PRELIM);
}
