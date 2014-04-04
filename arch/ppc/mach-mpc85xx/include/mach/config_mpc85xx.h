/*
 * Copyright 2012 GE Intelligent Platforms, Inc.
 * Copyright 2011 Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _ASM_MPC85xx_CONFIG_H_
#define _ASM_MPC85xx_CONFIG_H_

#define RESET_VECTOR    0xfffffffc

/* Number of TLB CAM entries we have on FSL Book-E chips */
#if defined(CONFIG_E500)
#define NUM_TLBCAMS	16
#endif

#if defined(CONFIG_P2020)
#define MAX_CPUS	2
#define FSL_NUM_LAWS	12
#define FSL_SEC_COMPAT	2
#define FSL_NUM_TSEC	3
#define FSL_ERRATUM_A005125
#define PPC_E500_DEBUG_TLB 2

#elif defined(CONFIG_MPC8544)
#define MAX_CPUS	1
#define FSL_NUM_LAWS	10
#define FSL_NUM_TSEC	2
#define FSL_ERRATUM_A005125
#define PPC_E500_DEBUG_TLB 0

#elif defined(CONFIG_P1022)
#define MAX_CPUS		2
#define FSL_NUM_LAWS		12
#define FSL_NUM_TSEC		2
#define FSL_SEC_COMPAT		2
#define PPC_E500_DEBUG_TLB	2
#define FSL_TSECV2
#define FSL_ERRATUM_A005125

#else
#error Processor type not defined for this platform
#endif

#endif /* _ASM_MPC85xx_CONFIG_H_ */
