/*
 * Copyright 2012 GE Intelligent Platforms, Inc.
 * Copyright (C) 2004-2008,2010-2011 Freescale Semiconductor, Inc.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef __ASM_PPC_FSL_LBC_H
#define __ASM_PPC_FSL_LBC_H

#include <config.h>
#include <common.h>

/*
 * BR - Base Registers
 */
#define BR_PS				0x00001800
#define BR_PS_SHIFT			11
#define BR_PS_8				0x00000800	/* Port Size 8 bit */
#define BR_PS_16			0x00001000	/* Port Size 16 bit */
#define BR_PS_32			0x00001800	/* Port Size 32 bit */
#define BR_V				0x00000001
#define BR_V_SHIFT			0

/* Convert an address into the right format for the BR registers */
#define BR_PHYS_ADDR(x) ((x) & 0xffff8000)

/*
 * CLKDIV is five bits only on 8536, 8572, and 8610, so far, but the fifth bit
 * should always be zero on older parts that have a four bit CLKDIV.
 */
#define LCRR_CLKDIV			0x0000001f
#define LCRR_CLKDIV_SHIFT		0
#define LCRR_CLKDIV_4			0x00000002
#define LCRR_CLKDIV_8			0x00000004
#define LCRR_CLKDIV_16			0x00000008

#ifndef __ASSEMBLY__
#include <asm/io.h>

/* LBC register offsets. */
#define FSL_LBC_BRX(x)	((x) * 8)	/* bank register offsets.  */
#define FSL_LBC_ORX(x)	(4 + ((x) * 8)) /* option register offset. */
#define FSL_LBC_LCCR	0x0d4		/* Clock ration register. */

#define LBC_BASE_ADDR ((void __iomem *)LBC_ADDR)
#define fsl_get_lbc_br(x) (in_be32((LBC_BASE_ADDR + FSL_LBC_BRX(x))))
#define fsl_get_lbc_or(x) (in_be32((LBC_BASE_ADDR + FSL_LBC_ORX(x))))
#define fsl_set_lbc_br(x, v) (out_be32((LBC_BASE_ADDR + FSL_LBC_BRX(x)), v))
#define fsl_set_lbc_or(x, v) (out_be32((LBC_BASE_ADDR + FSL_LBC_ORX(x)), v))

#endif /* __ASSEMBLY__ */
#endif /* __ASM_PPC_FSL_LBC_H */
