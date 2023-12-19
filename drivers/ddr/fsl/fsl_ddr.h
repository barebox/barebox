/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright 2008-2016 Freescale Semiconductor, Inc.
 * Copyright 2017-2018 NXP Semiconductor
 */

#ifndef FSL_DDR_H
#define FSL_DDR_H

#include <ddr_spd.h>
#include <soc/fsl/fsl_immap.h>

#define DDR_BL4		4	/* burst length 4 */
#define DDR_BC4		DDR_BL4	/* burst chop for ddr3 */
#define DDR_OTF		6	/* on-the-fly BC4 and BL8 */
#define DDR_BL8		8	/* burst length 8 */

#define DDR3_RTT_OFF		0
#define DDR3_RTT_60_OHM		1 /* RTT_Nom = RZQ/4 */
#define DDR3_RTT_120_OHM	2 /* RTT_Nom = RZQ/2 */
#define DDR3_RTT_40_OHM		3 /* RTT_Nom = RZQ/6 */
#define DDR3_RTT_20_OHM		4 /* RTT_Nom = RZQ/12 */
#define DDR3_RTT_30_OHM		5 /* RTT_Nom = RZQ/8 */

#define DDR4_RTT_OFF		0
#define DDR4_RTT_60_OHM		1	/* RZQ/4 */
#define DDR4_RTT_120_OHM	2	/* RZQ/2 */
#define DDR4_RTT_40_OHM		3	/* RZQ/6 */
#define DDR4_RTT_240_OHM	4	/* RZQ/1 */
#define DDR4_RTT_48_OHM		5	/* RZQ/5 */
#define DDR4_RTT_80_OHM		6	/* RZQ/3 */
#define DDR4_RTT_34_OHM		7	/* RZQ/7 */

#define DDR2_RTT_OFF		0
#define DDR2_RTT_75_OHM		1
#define DDR2_RTT_150_OHM	2
#define DDR2_RTT_50_OHM		3

#define FSL_DDR_MIN_TCKE_PULSE_WIDTH_DDR1	1
#define FSL_DDR_MIN_TCKE_PULSE_WIDTH_DDR2	3

#define FSL_DDR_ODT_NEVER		0x0
#define FSL_DDR_ODT_CS			0x1
#define FSL_DDR_ODT_ALL_OTHER_CS	0x2
#define FSL_DDR_ODT_OTHER_DIMM		0x3
#define FSL_DDR_ODT_ALL			0x4
#define FSL_DDR_ODT_SAME_DIMM		0x5
#define FSL_DDR_ODT_CS_AND_OTHER_DIMM	0x6
#define FSL_DDR_ODT_OTHER_CS_ONSAMEDIMM	0x7

/* define bank(chip select) interleaving mode */
#define FSL_DDR_CS0_CS1			0x40
#define FSL_DDR_CS2_CS3			0x20
#define FSL_DDR_CS0_CS1_AND_CS2_CS3	(FSL_DDR_CS0_CS1 | FSL_DDR_CS2_CS3)
#define FSL_DDR_CS0_CS1_CS2_CS3		(FSL_DDR_CS0_CS1_AND_CS2_CS3 | 0x04)

/* define memory controller interleaving mode */
#define FSL_DDR_CACHE_LINE_INTERLEAVING	0x0
#define FSL_DDR_PAGE_INTERLEAVING	0x1
#define FSL_DDR_BANK_INTERLEAVING	0x2
#define FSL_DDR_SUPERBANK_INTERLEAVING	0x3
#define FSL_DDR_256B_INTERLEAVING	0x8
#define FSL_DDR_3WAY_1KB_INTERLEAVING	0xA
#define FSL_DDR_3WAY_4KB_INTERLEAVING	0xC
#define FSL_DDR_3WAY_8KB_INTERLEAVING	0xD
/* placeholder for 4-way interleaving */
#define FSL_DDR_4WAY_1KB_INTERLEAVING	0x1A
#define FSL_DDR_4WAY_4KB_INTERLEAVING	0x1C
#define FSL_DDR_4WAY_8KB_INTERLEAVING	0x1D

#define SDRAM_CS_CONFIG_EN		0x80000000

/* DDR_SDRAM_CFG - DDR SDRAM Control Configuration
 */
#define SDRAM_CFG_MEM_EN		0x80000000
#define SDRAM_CFG_SREN			0x40000000
#define SDRAM_CFG_ECC_EN		0x20000000
#define SDRAM_CFG_RD_EN			0x10000000
#define SDRAM_CFG_SDRAM_TYPE_DDR1	0x02000000
#define SDRAM_CFG_SDRAM_TYPE_DDR2	0x03000000
#define SDRAM_CFG_SDRAM_TYPE_MASK	0x07000000
#define SDRAM_CFG_SDRAM_TYPE_SHIFT	24
#define SDRAM_CFG_DYN_PWR		0x00200000
#define SDRAM_CFG_DBW_MASK		0x00180000
#define SDRAM_CFG_DBW_SHIFT		19
#define SDRAM_CFG_32_BE			0x00080000
#define SDRAM_CFG_16_BE			0x00100000
#define SDRAM_CFG_8_BE			0x00040000
#define SDRAM_CFG_NCAP			0x00020000
#define SDRAM_CFG_2T_EN			0x00008000
#define SDRAM_CFG_BI			0x00000001

#define SDRAM_CFG2_FRC_SR		0x80000000
#define SDRAM_CFG2_D_INIT		0x00000010
#define SDRAM_CFG2_AP_EN		0x00000020
#define SDRAM_CFG2_ODT_CFG_MASK		0x00600000
#define SDRAM_CFG2_ODT_NEVER		0
#define SDRAM_CFG2_ODT_ONLY_WRITE	1
#define SDRAM_CFG2_ODT_ONLY_READ	2
#define SDRAM_CFG2_ODT_ALWAYS		3

#define SDRAM_INTERVAL_BSTOPRE	0x3FFF
#define TIMING_CFG_2_CPO_MASK	0x0F800000

#define RD_TO_PRE_MASK		0xf
#define RD_TO_PRE_SHIFT		13
#define WR_DATA_DELAY_MASK	0xf
#define WR_DATA_DELAY_SHIFT	9

/* DDR_EOR register */
#define DDR_EOR_RD_REOD_DIS	0x07000000
#define DDR_EOR_WD_REOD_DIS	0x00100000

/* DDR_MD_CNTL */
#define MD_CNTL_MD_EN		0x80000000
#define MD_CNTL_CS_SEL_CS0	0x00000000
#define MD_CNTL_CS_SEL_CS1	0x10000000
#define MD_CNTL_CS_SEL_CS2	0x20000000
#define MD_CNTL_CS_SEL_CS3	0x30000000
#define MD_CNTL_CS_SEL_CS0_CS1	0x40000000
#define MD_CNTL_CS_SEL_CS2_CS3	0x50000000
#define MD_CNTL_MD_SEL_MR	0x00000000
#define MD_CNTL_MD_SEL_EMR	0x01000000
#define MD_CNTL_MD_SEL_EMR2	0x02000000
#define MD_CNTL_MD_SEL_EMR3	0x03000000
#define MD_CNTL_SET_REF		0x00800000
#define MD_CNTL_SET_PRE		0x00400000
#define MD_CNTL_CKE_CNTL_LOW	0x00100000
#define MD_CNTL_CKE_CNTL_HIGH	0x00200000
#define MD_CNTL_WRCW		0x00080000
#define MD_CNTL_MD_VALUE(x)	(x & 0x0000FFFF)
#define MD_CNTL_CS_SEL(x)	(((x) & 0x7) << 28)
#define MD_CNTL_MD_SEL(x)	(((x) & 0xf) << 24)

/* DDR_CDR1 */
#define DDR_CDR1_DHC_EN	0x80000000
#define DDR_CDR1_V0PT9_EN	0x40000000
#define DDR_CDR1_ODT_SHIFT	17
#define DDR_CDR1_ODT_MASK	0x6
#define DDR_CDR2_ODT_MASK	0x1
#define DDR_CDR1_ODT(x) ((x & DDR_CDR1_ODT_MASK) << DDR_CDR1_ODT_SHIFT)
#define DDR_CDR2_ODT(x) (x & DDR_CDR2_ODT_MASK)
#define DDR_CDR2_VREF_OVRD(x)	(0x00008080 | ((((x) - 37) & 0x3F) << 8))
#define DDR_CDR2_VREF_TRAIN_EN	0x00000080
#define DDR_CDR2_VREF_RANGE_2	0x00000040

/* DDR ERR_DISABLE */
#define DDR_ERR_DISABLE_APED	(1 << 8)  /* Address parity error disable */

/* Mode Registers */
#define DDR_MR5_CA_PARITY_LAT_4_CLK	0x1 /* for DDR4-1600/1866/2133 */
#define DDR_MR5_CA_PARITY_LAT_5_CLK	0x2 /* for DDR4-2400 */

/* DEBUG_26 register */
#define DDR_CAS_TO_PRE_SUB_MASK  0x0000f000 /* CAS to preamble subtract value */
#define DDR_CAS_TO_PRE_SUB_SHIFT 12

/* DEBUG_29 register */
#define DDR_TX_BD_DIS	(1 << 10) /* Transmit Bit Deskew Disable */

static inline int is_ddr1(const memctl_options_t *popts)
{
	return IS_ENABLED(CONFIG_DDR_FSL_DDR1) &&
			popts->ddrtype == SDRAM_TYPE_DDR1;
}

static inline int is_ddr2(const memctl_options_t *popts)
{
	return IS_ENABLED(CONFIG_DDR_FSL_DDR2) &&
			popts->ddrtype == SDRAM_TYPE_DDR2;
}

static inline int is_ddr3(const memctl_options_t *popts)
{
	return IS_ENABLED(CONFIG_DDR_FSL_DDR3) &&
			popts->ddrtype == SDRAM_TYPE_DDR3;
}

static inline int is_ddr4(const memctl_options_t *popts)
{
	return IS_ENABLED(CONFIG_DDR_FSL_DDR4) &&
			popts->ddrtype == SDRAM_TYPE_DDR4;
}

static inline int is_ddr3_4(const memctl_options_t *popts)
{
	return is_ddr3(popts) || is_ddr4(popts);
}

struct fsl_ddr_info;

u32 fsl_ddr_get_intl3r(void);

void board_mem_sleep_setup(void);
static inline bool is_warm_boot(void)
{
	return false;
}

int fsl_dp_resume(void);

struct fsl_ddr_controller;

u32 fsl_ddr_get_version(struct fsl_ddr_controller *c);

void fsl_ddr_set_intl3r(const unsigned int granule_size);

unsigned int compute_fsl_memctl_config_regs(struct fsl_ddr_controller *c);
unsigned int compute_lowest_common_dimm_parameters(struct fsl_ddr_controller *c);
unsigned int populate_memctl_options(struct fsl_ddr_controller *c);
void check_interleaving_options(struct fsl_ddr_info *pinfo);

unsigned int mclk_to_picos(struct fsl_ddr_controller *c, unsigned int mclk);
unsigned int get_memory_clk_period_ps(struct fsl_ddr_controller *c);
unsigned int picos_to_mclk(struct fsl_ddr_controller *c, unsigned int picos);

void erratum_a009942_check_cpo(void);

#endif
