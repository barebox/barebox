/*
 * Copyright 2010-2011 Freescale Semiconductor, Inc.
 * Author: Dipen Dudhat <dipen.dudhat@freescale.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __FSL_IFC_H
#define __FSL_IFC_H

#include <config.h>
#include <common.h>

/* Big-Endian */
#define ifc_in32(a)			in_be32(a)
#define ifc_out32(a, v)			out_be32(a, v)
#define ifc_in16(a)			in_be16(a)

/*
 * CSPR - Chip Select Property Register
 */
#define CSPR_BA				0xFFFF0000
#define CSPR_BA_SHIFT			16
#define CSPR_PORT_SIZE			0x00000180
#define CSPR_PORT_SIZE_SHIFT		7
#define CSPR_PORT_SIZE_8		0x00000080
#define CSPR_PORT_SIZE_16		0x00000100
#define CSPR_PORT_SIZE_32		0x00000180
/* Write Protect */
#define CSPR_WP				0x00000040
#define CSPR_WP_SHIFT			6
#define CSPR_MSEL			0x00000006
#define CSPR_MSEL_SHIFT			1
#define CSPR_MSEL_NOR			0x00000000
#define CSPR_MSEL_NAND			0x00000002
#define CSPR_MSEL_GPCM			0x00000004
#define CSPR_V				0x00000001
#define CSPR_V_SHIFT			0

/* Convert an address into the right format for the CSPR Registers */
#define CSPR_PHYS_ADDR(x)		(((uint64_t)x) & 0xffff0000)

/*
 * Address Mask Register
 */
#define IFC_AMASK_MASK			0xFFFF0000
#define IFC_AMASK_SHIFT			16
#define IFC_AMASK(n)			(IFC_AMASK_MASK << \
					(__ilog2(n) - IFC_AMASK_SHIFT))

/*
 * Chip Select Option Register IFC_NAND Machine
 */
#define CSOR_NAND_ECC_ENC_EN		0x80000000
#define CSOR_NAND_ECC_MODE_MASK		0x30000000
/* 4 bit correction per 520 Byte sector */
#define CSOR_NAND_ECC_MODE_4		0x00000000
/* 8 bit correction per 528 Byte sector */
#define CSOR_NAND_ECC_MODE_8		0x10000000
#define CSOR_NAND_ECC_DEC_EN		0x04000000
/* Row Address Length */
#define CSOR_NAND_RAL_MASK		0x01800000
#define CSOR_NAND_RAL_SHIFT		20
#define CSOR_NAND_RAL_1			0x00000000
#define CSOR_NAND_RAL_2			0x00800000
#define CSOR_NAND_RAL_3			0x01000000
#define CSOR_NAND_RAL_4			0x01800000
/* Page Size 512b, 2k, 4k */
#define CSOR_NAND_PGS_MASK		0x00180000
#define CSOR_NAND_PGS_SHIFT		16
#define CSOR_NAND_PGS_512		0x00000000
#define CSOR_NAND_PGS_2K		0x00080000
#define CSOR_NAND_PGS_4K		0x00100000
#define CSOR_NAND_PGS_8K		0x00180000
/* Spare region Size */
#define CSOR_NAND_SPRZ_MASK		0x0000E000
#define CSOR_NAND_SPRZ_SHIFT		13
#define CSOR_NAND_SPRZ_16		0x00000000
#define CSOR_NAND_SPRZ_64		0x00002000
#define CSOR_NAND_SPRZ_128		0x00004000
#define CSOR_NAND_SPRZ_210		0x00006000
#define CSOR_NAND_SPRZ_218		0x00008000
#define CSOR_NAND_SPRZ_224		0x0000A000
#define CSOR_NAND_SPRZ_CSOR_EXT	0x0000C000
/* Pages Per Block */
#define CSOR_NAND_PB_MASK		0x00000700
#define CSOR_NAND_PB_SHIFT		8
#define CSOR_NAND_PB(n)			((__ilog2(n) - 5) << CSOR_NAND_PB_SHIFT)
/* Time for Read Enable High to Output High Impedance */
#define CSOR_NAND_TRHZ_MASK		0x0000001C
#define CSOR_NAND_TRHZ_SHIFT		2
#define CSOR_NAND_TRHZ_20		0x00000000
#define CSOR_NAND_TRHZ_40		0x00000004
#define CSOR_NAND_TRHZ_60		0x00000008
#define CSOR_NAND_TRHZ_80		0x0000000C
#define CSOR_NAND_TRHZ_100		0x00000010
/* Buffer control disable */
#define CSOR_NAND_BCTLD			0x00000001

/*
 * Chip Select Option Register - NOR Flash Mode
 */
/* Enable Address shift Mode */
#define CSOR_NOR_ADM_SHFT_MODE_EN	0x80000000
/* Page Read Enable from NOR device */
#define CSOR_NOR_PGRD_EN		0x10000000
/* AVD Toggle Enable during Burst Program */
#define CSOR_NOR_AVD_TGL_PGM_EN		0x01000000
/* Address Data Multiplexing Shift */
#define CSOR_NOR_ADM_MASK		0x0003E000
#define CSOR_NOR_ADM_SHIFT_SHIFT	13
#define CSOR_NOR_ADM_SHIFT(n)	((n) << CSOR_NOR_ADM_SHIFT_SHIFT)
/* Type of the NOR device hooked */
#define CSOR_NOR_NOR_MODE_AYSNC_NOR	0x00000000
#define CSOR_NOR_NOR_MODE_AVD_NOR	0x00000020
/* Time for Read Enable High to Output High Impedance */
#define CSOR_NOR_TRHZ_MASK		0x0000001C
#define CSOR_NOR_TRHZ_SHIFT		2
#define CSOR_NOR_TRHZ_20		0x00000000
#define CSOR_NOR_TRHZ_40		0x00000004
#define CSOR_NOR_TRHZ_60		0x00000008
#define CSOR_NOR_TRHZ_80		0x0000000C
#define CSOR_NOR_TRHZ_100		0x00000010
/* Buffer control disable */
#define CSOR_NOR_BCTLD			0x00000001

/*
 * Flash Timing Registers (FTIM0 - FTIM2_CSn)
 */

/*
 * FTIM0 - NOR Flash Mode
 */
#define FTIM0_NOR			0xF03F3F3F
#define FTIM0_NOR_TACSE_SHIFT		28
#define FTIM0_NOR_TACSE(n)		((n) << FTIM0_NOR_TACSE_SHIFT)
#define FTIM0_NOR_TEADC_SHIFT		16
#define FTIM0_NOR_TEADC(n)		((n) << FTIM0_NOR_TEADC_SHIFT)
#define FTIM0_NOR_TAVDS_SHIFT		8
#define FTIM0_NOR_TAVDS(n)		((n) << FTIM0_NOR_TAVDS_SHIFT)
#define FTIM0_NOR_TEAHC_SHIFT		0
#define FTIM0_NOR_TEAHC(n)		((n) << FTIM0_NOR_TEAHC_SHIFT)
/*
 * FTIM1 - NOR Flash Mode
 */
#define FTIM1_NOR			0xFF003F3F
#define FTIM1_NOR_TACO_SHIFT		24
#define FTIM1_NOR_TACO(n)		((n) << FTIM1_NOR_TACO_SHIFT)
#define FTIM1_NOR_TRAD_NOR_SHIFT	8
#define FTIM1_NOR_TRAD_NOR(n)		((n) << FTIM1_NOR_TRAD_NOR_SHIFT)
#define FTIM1_NOR_TSEQRAD_NOR_SHIFT	0
#define FTIM1_NOR_TSEQRAD_NOR(n)	((n) << FTIM1_NOR_TSEQRAD_NOR_SHIFT)
/*
 * FTIM2 - NOR Flash Mode
 */
#define FTIM2_NOR			0x0F3CFCFF
#define FTIM2_NOR_TCS_SHIFT		24
#define FTIM2_NOR_TCS(n)		((n) << FTIM2_NOR_TCS_SHIFT)
#define FTIM2_NOR_TCH_SHIFT		18
#define FTIM2_NOR_TCH(n)		((n) << FTIM2_NOR_TCH_SHIFT)
#define FTIM2_NOR_TWPH_SHIFT		10
#define FTIM2_NOR_TWPH(n)		((n) << FTIM2_NOR_TWPH_SHIFT)
#define FTIM2_NOR_TWP_SHIFT		0
#define FTIM2_NOR_TWP(n)		((n) << FTIM2_NOR_TWP_SHIFT)

/*
 * FTIM0 - Normal GPCM Mode
 */
#define FTIM0_GPCM			0xF03F3F3F
#define FTIM0_GPCM_TACSE_SHIFT		28
#define FTIM0_GPCM_TACSE(n)		((n) << FTIM0_GPCM_TACSE_SHIFT)
#define FTIM0_GPCM_TEADC_SHIFT		16
#define FTIM0_GPCM_TEADC(n)		((n) << FTIM0_GPCM_TEADC_SHIFT)
#define FTIM0_GPCM_TAVDS_SHIFT		8
#define FTIM0_GPCM_TAVDS(n)		((n) << FTIM0_GPCM_TAVDS_SHIFT)
#define FTIM0_GPCM_TEAHC_SHIFT		0
#define FTIM0_GPCM_TEAHC(n)		((n) << FTIM0_GPCM_TEAHC_SHIFT)
/*
 * FTIM1 - Normal GPCM Mode
 */
#define FTIM1_GPCM			0xFF003F00
#define FTIM1_GPCM_TACO_SHIFT		24
#define FTIM1_GPCM_TACO(n)		((n) << FTIM1_GPCM_TACO_SHIFT)
#define FTIM1_GPCM_TRAD_SHIFT		8
#define FTIM1_GPCM_TRAD(n)		((n) << FTIM1_GPCM_TRAD_SHIFT)
/*
 * FTIM2 - Normal GPCM Mode
 */
#define FTIM2_GPCM			0x0F3C00FF
#define FTIM2_GPCM_TCS_SHIFT		24
#define FTIM2_GPCM_TCS(n)		((n) << FTIM2_GPCM_TCS_SHIFT)
#define FTIM2_GPCM_TCH_SHIFT		18
#define FTIM2_GPCM_TCH(n)		((n) << FTIM2_GPCM_TCH_SHIFT)
#define FTIM2_GPCM_TWP_SHIFT		0
#define FTIM2_GPCM_TWP(n)		((n) << FTIM2_GPCM_TWP_SHIFT)

/*
 * General Control Register (GCR)
 */
#define IFC_GCR_MASK			0x8000F800
/* reset all IFC hardware */
#define IFC_GCR_SOFT_RST_ALL		0x80000000
/* Turnaroud Time of external buffer */
#define IFC_GCR_TBCTL_TRN_TIME		0x0000F800
#define IFC_GCR_TBCTL_TRN_TIME_SHIFT	11

/*
 * Clock Control Register (CCR)
 */
#define IFC_CCR_MASK			0x0F0F8800
/* Clock division ratio */
#define IFC_CCR_CLK_DIV_MASK		0x0F000000
#define IFC_CCR_CLK_DIV_SHIFT		24
#define IFC_CCR_CLK_DIV(n)		((n-1) << IFC_CCR_CLK_DIV_SHIFT)
/* IFC Clock Delay */
#define IFC_CCR_CLK_DLY_MASK		0x000F0000
#define IFC_CCR_CLK_DLY_SHIFT		16
#define IFC_CCR_CLK_DLY(n)		((n) << IFC_CCR_CLK_DLY_SHIFT)

#ifndef __ASSEMBLY__
#include <asm/io.h>

#define IFC_BASE_ADDR		((void __iomem *)IFC_ADDR)
#define FSL_IFC_CSPRX(i)	(0x10 + ((i) * 0xc))
#define FSL_IFC_CSORX(i)	(0x130 + ((i) * 0xc))
#define FSL_IFC_AMASKX(i)	(0xa0 + ((i) * 0xc))
#define FSL_IFC_CSX_FTIMY(i, j)	((0x1c0 + ((i) * 0x30)) + ((j) * 4))

#define get_ifc_cspr(i) (ifc_in32(IFC_BASE_ADDR + FSL_IFC_CSPRX(i)))
#define get_ifc_csor(i) (ifc_in32(IFC_BASE_ADDR + FSL_IFC_CSORX(i))
#define get_ifc_amask(i) (ifc_in32(IFC_BASE_ADDR + FSL_IFC_AMASKX(i)))
#define get_ifc_ftim(i, j) (ifc_in32(IFC_BASE_ADDR + FSL_IFC_CSX_FTIMY(i, j)))

#define set_ifc_cspr(i, v) (ifc_out32(IFC_BASE_ADDR + FSL_IFC_CSPRX(i), v))
#define set_ifc_csor(i, v) (ifc_out32(IFC_BASE_ADDR + FSL_IFC_CSORX(i), v))
#define set_ifc_amask(i, v) (ifc_out32(IFC_BASE_ADDR + FSL_IFC_AMASKX(i), v))
#define set_ifc_ftim(i, j, v) \
			(ifc_out32(IFC_BASE_ADDR + FSL_IFC_CSX_FTIMY(i, j), v))

#define FSL_IFC_GCR_OFFSET	0x40c
#define FSL_IFC_CCR_OFFSET	0x44c

enum ifc_chip_sel {
	IFC_CS0,
	IFC_CS1,
	IFC_CS2,
	IFC_CS3,
	IFC_CS4,
	IFC_CS5,
	IFC_CS6,
	IFC_CS7,
};

enum ifc_ftims {
	IFC_FTIM0,
	IFC_FTIM1,
	IFC_FTIM2,
	IFC_FTIM3,
};

#ifdef CONFIG_FSL_ERRATUM_IFC_A002769
#undef CSPR_MSEL_NOR
#define CSPR_MSEL_NOR	CSPR_MSEL_GPCM
#endif

#endif /* __ASSEMBLY__ */
#endif /* __FSL_IFC_H */
