/*
 * MPC85xx Internal Memory Map
 *
 * Copyright 2007-2011 Freescale Semiconductor, Inc.
 *
 * Copyright(c) 2002,2003 Motorola Inc.
 * Xianghua Xiao (x.xiao@motorola.com)
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#ifndef __IMMAP_85xx__
#define __IMMAP_85xx__

#include <asm/types.h>
#include <asm/fsl_lbc.h>
#include <asm/config.h>

#define MPC85xx_LOCAL_OFFSET	0x0000
#define MPC85xx_ECM_OFFSET	0x1000
#define MPC85xx_DDR_OFFSET	0x2000
#define MPC85xx_LBC_OFFSET	0x5000

#define MPC85xx_GPIO_OFFSET	0xf000
#define MPC85xx_L2_OFFSET	0x20000
#ifdef CONFIG_TSECV2
#define TSEC1_OFFSET		0xB0000
#else
#define TSEC1_OFFSET		0x24000
#endif

#define MPC85xx_PIC_OFFSET	0x40000
#define MPC85xx_GUTS_OFFSET	0xe0000

#define MPC85xx_LOCAL_ADDR	(CFG_IMMR + MPC85xx_LOCAL_OFFSET)
#define MPC85xx_ECM_ADDR	(CFG_IMMR + MPC85xx_ECM_OFFSET)
#define MPC85xx_GUTS_ADDR	(CFG_IMMR + MPC85xx_GUTS_OFFSET)
#define MPC85xx_DDR_ADDR	(CFG_IMMR + MPC85xx_DDR_OFFSET)
#define LBC_ADDR		(CFG_IMMR + MPC85xx_LBC_OFFSET)
#define MPC85xx_GPIO_ADDR	(CFG_IMMR + MPC85xx_GPIO_OFFSET)
#define MPC85xx_L2_ADDR		(CFG_IMMR + MPC85xx_L2_OFFSET)
#define MPC8xxx_PIC_ADDR	(CFG_IMMR + MPC85xx_PIC_OFFSET)

/* Local-Access Registers */
#define MPC85xx_LOCAL_BPTR_OFFSET	0x20 /* Boot Page Translation */

/* ECM Registers */
#define MPC85xx_ECM_EEBPCR_OFFSET	0x00 /* ECM CCB Port Configuration */

/*
 * DDR Memory Controller Register Offsets
 */
/* Chip Select 0, 1,2, 3 Memory Bounds */
#define MPC85xx_DDR_CS0_BNDS_OFFSET		0x000
#define MPC85xx_DDR_CS1_BNDS_OFFSET		0x008
#define MPC85xx_DDR_CS2_BNDS_OFFSET		0x010
#define MPC85xx_DDR_CS3_BNDS_OFFSET		0x018
/* Chip Select 0, 1, 2, 3 Configuration */
#define MPC85xx_DDR_CS0_CONFIG_OFFSET		0x080
#define MPC85xx_DDR_CS1_CONFIG_OFFSET		0x084
#define MPC85xx_DDR_CS2_CONFIG_OFFSET		0x088
#define MPC85xx_DDR_CS3_CONFIG_OFFSET		0x08c
/* SDRAM Timing Configuration 0, 1, 2, 3 */
#define MPC85xx_DDR_TIMING_CFG_3_OFFSET		0x100
#define MPC85xx_DDR_TIMING_CFG_0_OFFSET		0x104
#define MPC85xx_DDR_TIMING_CFG_1_OFFSET		0x108
#define MPC85xx_DDR_TIMING_CFG_2_OFFSET		0x10c
/* SDRAM Control Configuration */
#define MPC85xx_DDR_SDRAM_CFG_OFFSET		0x110
#define MPC85xx_DDR_SDRAM_CFG_2_OFFSET		0x114
/* SDRAM Mode Configuration */
#define MPC85xx_DDR_SDRAM_MODE_OFFSET		0x118
#define MPC85xx_DDR_SDRAM_MODE_2_OFFSET		0x11c
/* SDRAM Mode Control */
#define MPC85xx_DDR_SDRAM_MD_CNTL_OFFSET	0x120
/* SDRAM Interval Configuration */
#define MPC85xx_DDR_SDRAM_INTERVAL_OFFSET	0x124
/* SDRAM Data initialization */
#define MPC85xx_DDR_SDRAM_DATA_INIT_OFFSET	0x128
/* SDRAM Clock Control */
#define MPC85xx_DDR_SDRAM_CLK_CNTL_OFFSET	0x130
/* training init and extended addr */
#define MPC85xx_DDR_SDRAM_INIT_ADDR_OFFSET	0x148
#define MPC85xx_DDR_SDRAM_INIT_ADDR_EXT_OFFSET	0x14c

#define DDR_OFF(REGNAME)	(MPC85xx_DDR_##REGNAME##_OFFSET)

/*
 * GPIO Register Offsets
 */
#define MPC85xx_GPIO_GPDIR	0x00
#define MPC85xx_GPIO_GPDAT	0x08

/*
 * L2 Cache Register Offsets
 */
#define MPC85xx_L2_CTL_OFFSET	0x0		/* L2 configuration 0 */
#define		MPC85xx_L2CTL_L2E	0x80000000

/* PIC registers offsets */
#define MPC85xx_PIC_WHOAMI_OFFSET	0x090
#define MPC85xx_PIC_FRR_OFFSET		0x1000	/* Feature Reporting */
/* PIC registers fields values and masks. */
#define MPC8xxx_PICFRR_NCPU_MASK	0x00001f00
#define MPC8xxx_PICFRR_NCPU_SHIFT	8
#define MPC85xx_PICGCR_RST		0x80000000
#define MPC85xx_PICGCR_M		0x20000000

#define MPC85xx_PIC_IACK0_OFFSET	0x600a0	/* IRQ Acknowledge for
						   Processor 0 */

/* Global Utilities Register Offsets and field values */
#define MPC85xx_GUTS_PORPLLSR_OFFSET	0x0
#define		MPC85xx_PORPLLSR_DDR_RATIO		0x00003e00
#define		MPC85xx_PORPLLSR_DDR_RATIO_SHIFT	9
#define MPC85xx_GUTS_DEVDISR_OFFSET	0x70
#define		MPC85xx_DEVDISR_TB0	0x00004000
#define		MPC85xx_DEVDISR_TB1	0x00001000
#define MPC85xx_GUTS_RSTCR_OFFSET	0xb0

#define GFAR_BASE_ADDR		(CFG_IMMR + TSEC1_OFFSET)
#define MDIO_BASE_ADDR		(CFG_IMMR + 0x24000)

#define I2C1_BASE_ADDR		(CFG_IMMR + 0x3000)
#define I2C2_BASE_ADDR		(CFG_IMMR + 0x3100)

#endif /*__IMMAP_85xx__*/
