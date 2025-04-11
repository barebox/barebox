/* SPDX-License-Identifier: BSD-1-Clause */
/*
 * Copyright (c) 2006, Atmel Corporation
 *
 * Atmel's name may not be used to endorse or promote products derived from
 * this software without specific prior written permission.
 */
#ifndef __AT91_DDRSDRC_H__
#define __AT91_DDRSDRC_H__

/**** Register offset in AT91S_HDDRSDRC2 structure ***/
#define AT91_HDDRSDRC2_MR		0x00	/* Mode Register */
#define AT91_HDDRSDRC2_RTR		0x04	/* Refresh Timer Register */
#define AT91_HDDRSDRC2_CR		0x08	/* Configuration Register */
#define AT91_HDDRSDRC2_T0PR		0x0C	/* Timing Parameter 0 Register */
#define AT91_HDDRSDRC2_T1PR		0x10	/* Timing Parameter 1 Register */
#define AT91_HDDRSDRC2_T2PR		0x14	/* Timing Parameter 2 Register */
#define AT91_HDDRSDRC2_T3PR		0x18	/* Timing Parameter 3 Register */
#define AT91_HDDRSDRC2_LPR		0x1C	/* Low-power Register */
#define AT91_HDDRSDRC2_MDR		0x20	/* Memory Device Register */
#define AT91_HDDRSDRC2_DLL		0x24	/* DLL Information Register */
#define AT91_HDDRSDRC2_HS		0x2C	/* High Speed Register */

/* below items defined for sama5d3x */
#define	AT91_MPDDRC_LPDDR2_HS		0x24	/* MPDDRC LPDDR2 High Speed Register */
#define	AT91_MPDDRC_LPDDR2_LPR		0x28	/* MPDDRC LPDDR2 Low-power Register */
#define	AT91_MPDDRC_LPDDR2_CAL_MR4	0x2C	/* MPDDRC LPDDR2 Calibration and MR4 Register */
#define	AT91_MPDDRC_LPDDR2_TIM_CAL	0x30	/* MPDDRC LPDDR2 Timing Calibration Register */
#define	AT91_MPDDRC_IO_CALIBR		0x34	/* MPDDRC IO Calibration */
#define	AT91_MPDDRC_OCMS		0x38	/* MPDDRC OCMS Register */
#define	AT91_MPDDRC_OCMS_KEY1		0x3C	/* MPDDRC OCMS KEY1 Register */
#define	AT91_MPDDRC_OCMS_KEY2		0x40	/* MPDDRC OCMS KEY2 Register */
/* 0x54 ~ 0x70 Reserved */
#define	AT91_MPDDRC_DLL_MOR		0x74	/* MPDDRC DLL Master Offset Register */
#define	AT91_MPDDRC_DLL_SOR		0x78	/* MPDDRC DLL Slave Offset Register */
#define	AT91_MPDDRC_DLL_MSR		0x7C	/* MPDDRC DLL Master Status Register */
#define	AT91_MPDDRC_DLL_S0SR		0x80	/* MPDDRC DLL Slave 0 Status Register */
#define	AT91_MPDDRC_DLL_S1SR		0x84	/* MPDDRC DLL Slave 1 Status Register */

#define AT91_MPDDRC_RD_DATA_PATH	0x5C	/* MPDDRC Read Data Path */

/* 0x94 ~ 0xE0 Reserved */
#define AT91_HDDRSDRC2_WPCR		0xE4	/* Write Protect Mode Register */
#define AT91_HDDRSDRC2_WPSR		0xE8	/* Write Protect Status Register */

/* -------- HDDRSDRC2_MR : (HDDRSDRC2 Offset: 0x0) Mode Register --------*/
#define AT91_DDRC2_MODE	(0x7UL << 0)
#define 	AT91_DDRC2_MODE_NORMAL_CMD		(0x0UL)
#define 	AT91_DDRC2_MODE_NOP_CMD			(0x1UL)
#define 	AT91_DDRC2_MODE_PRCGALL_CMD		(0x2UL)
#define 	AT91_DDRC2_MODE_LMR_CMD			(0x3UL)
#define 	AT91_DDRC2_MODE_RFSH_CMD		(0x4UL)
#define 	AT91_DDRC2_MODE_EXT_LMR_CMD		(0x5UL)
#define 	AT91_DDRC2_MODE_DEEP_CMD		(0x6UL)
#define		AT91_DDRC2_MODE_LPDDR2_CMD		(0x7UL)
#define AT91_DDRC2_MRS(value)	(value << 8)

/* -------- HDDRSDRC2_RTR : (HDDRSDRC2 Offset: 0x4) Refresh Timer Register -------- */
#define AT91_DDRC2_COUNT	(0xFFFUL << 0)
#define AT91_DDRC2_ADJ_REF	(0x1UL << 16)
#define 	AT91_DDRC2_DISABLE_ADJ_REF	(0x0UL << 16)
#define 	AT91_DDRC2_ENABLE_ADJ_REF	(0x1UL << 16)

/* -------- HDDRSDRC2_CR : (HDDRSDRC2 Offset: 0x8) Configuration Register --------*/
#define AT91_DDRC2_NC		(0x3UL <<  0)
#define 	AT91_DDRC2_NC_DDR9_SDR8	(0x0UL)
#define 	AT91_DDRC2_NC_DDR10_SDR9	(0x1UL)
#define 	AT91_DDRC2_NC_DDR11_SDR10	(0x2UL)
#define 	AT91_DDRC2_NC_DDR12_SDR11	(0x3UL)
#define AT91_DDRC2_NR		(0x3UL << 2)
#define 	AT91_DDRC2_NR_11		(0x0UL << 2)
#define 	AT91_DDRC2_NR_12		(0x1UL << 2)
#define 	AT91_DDRC2_NR_13		(0x2UL << 2)
#define 	AT91_DDRC2_NR_14		(0x3UL << 2)
#define AT91_DDRC2_CAS		(0x7UL << 4)
#define 	AT91_DDRC2_CAS_2		(0x2UL << 4)
#define 	AT91_DDRC2_CAS_3		(0x3UL << 4)
#define 	AT91_DDRC2_CAS_4		(0x4UL << 4)
#define 	AT91_DDRC2_CAS_5		(0x5UL << 4)
#define 	AT91_DDRC2_CAS_6		(0x6UL << 4)
#define AT91_DDRC2_RESET_DLL	(0x1UL << 7)
#define 	AT91_DDRC2_DISABLE_RESET_DLL	(0x0UL << 7)
#define 	AT91_DDRC2_ENABLE_RESET_DLL	(0x1UL << 7)
#define AT91_DDRC2_DIC_DS	(0x1UL << 8)
#define		AT91_DDRC2_NORMAL_STRENGTH_RZQ6	(0x0UL << 8)
#define		AT91_DDRC2_WEAK_STRENGTH_RZQ7		(0x1UL << 8)
#define AT91_DDRC2_DLL	(0x1UL << 9)
#define 	AT91_DDRC2_ENABLE_DLL		(0x0UL << 9)
#define 	AT91_DDRC2_DISABLE_DLL		(0x1UL << 9)
#define AT91_DDRC2_ZQ		(0x03 << 10)
#define		AT91_DDRC2_ZQ_INIT		(0x0 << 10)
#define		AT91_DDRC2_ZQ_LONG		(0x1 << 10)
#define		AT91_DDRC2_ZQ_SHORT		(0x2 << 10)
#define		AT91_DDRC2_ZQ_RESET		(0x3 << 10)
#define AT91_DDRC2_OCD		(0x7UL << 12)
#define 	AT91_DDRC2_OCD_EXIT		(0x0UL << 12)
#define 	AT91_DDRC2_OCD_DEFAULT		(0x7UL << 12)
#define AT91_DDRC2_EBISHARE	(0x1UL << 16)
#define AT91_DDRC2_DQMS	(0x1UL << 16)
#define		AT91_DDRC2_DQMS_NOT_SHARED	(0x0UL << 16)
#define		AT91_DDRC2_DQMS_SHARED		(0x1UL << 16)
#define AT91_DDRC2_ENRDM	(0x1UL << 17)
#define 	AT91_DDRC2_ENRDM_DISABLE	(0x0UL << 17)
#define 	AT91_DDRC2_ENRDM_ENABLE	(0x1UL << 17)
#define AT91_DDRC2_ACTBST	(0x1UL << 18)
#define AT91_DDRC2_NB_BANKS	(0x1UL << 20)
#define 	AT91_DDRC2_NB_BANKS_4		(0x0UL << 20)
#define 	AT91_DDRC2_NB_BANKS_8		(0x1UL << 20)
#define AT91_DDRC2_NDQS	(0x1UL << 21)	/* Not DQS(sama5d3x only) */
#define 	AT91_DDRC2_NDQS_ENABLED	(0x0UL << 21)
#define 	AT91_DDRC2_NDQS_DISABLED	(0x1UL << 21)
#define AT91_DDRC2_DECOD	(0x1UL << 22)
#define 	AT91_DDRC2_DECOD_SEQUENTIAL	(0x0UL << 22)
#define 	AT91_DDRC2_DECOD_INTERLEAVED	(0x1UL << 22)
#define AT91_DDRC2_UNAL		(0x1UL << 23)	/* Support Unaligned Access(sama5d3x only) */
#define 	AT91_DDRC2_UNAL_UNSUPPORTED		(0x0UL << 23)
#define 	AT91_DDRC2_UNAL_SUPPORTED		(0x1UL << 23)

/* -------- HDDRSDRC2_T0PR : (HDDRSDRC2 Offset: 0xc) Timing0 Register --------*/
#define AT91_DDRC2_TRAS	(0xFUL <<  0)
#define		AT91_DDRC2_TRAS_(x)		(x & 0x0f)
#define	AT91_DDRC2_TRCD	(0xFUL <<  4)
#define		AT91_DDRC2_TRCD_(x)		((x & 0x0f) << 4)
#define	AT91_DDRC2_TWR		(0xFUL << 8)
#define		AT91_DDRC2_TWR_(x)		((x & 0x0f) << 8)
#define	AT91_DDRC2_TRC		(0xFUL << 12)
#define		AT91_DDRC2_TRC_(x)		((x & 0x0f) << 12)
#define	AT91_DDRC2_TRP		(0xFUL << 16)
#define		AT91_DDRC2_TRP_(x)		((x & 0x0f) << 16)
#define	AT91_DDRC2_TRRD	(0xFUL << 20)
#define		AT91_DDRC2_TRRD_(x)		((x & 0x0f) << 20)
#define	AT91_DDRC2_TWTR	(0xFUL << 24)
#define		AT91_DDRC2_TWTR_(x)		((x & 0x0f) << 24)
#define	AT91_DDRC2_TMRD	(0xFUL << 28)
#define		AT91_DDRC2_TMRD_(x)		((x & 0x0f) << 28)

/* -------- HDDRSDRC2_T1PR : (HDDRSDRC2 Offset: 0x10) Timing1 Register -------- */
#define	AT91_DDRC2_TRFC	(0x7FUL <<  0)
#define		AT91_DDRC2_TRFC_(x)		(x & 0x7f)
#define	AT91_DDRC2_TXSNR	(0xFFUL << 8)
#define		AT91_DDRC2_TXSNR_(x)		((x & 0xff) << 8)
#define AT91_DDRC2_TXSRD	(0xFFUL << 16)
#define		AT91_DDRC2_TXSRD_(x)		((x & 0xff) << 16)
#define	AT91_DDRC2_TXP		(0xFUL << 24)
#define		AT91_DDRC2_TXP_(x)		((x & 0x0f) << 24)

/* -------- HDDRSDRC2_T2PR : (HDDRSDRC2 Offset: 0x14) Timing2 Register --------*/
#define	AT91_DDRC2_TXARD	(0xFUL << 0)
#define		AT91_DDRC2_TXARD_(x)		(x & 0x0f)
#define	AT91_DDRC2_TXARDS	(0xFUL << 4)
#define		AT91_DDRC2_TXARDS_(x)		((x & 0x0f) << 4)
#define	AT91_DDRC2_TRPA	(0xFUL << 8)
#define		AT91_DDRC2_TRPA_(x)		((x & 0x0f) << 8)
#define	AT91_DDRC2_TRT		(0xFUL << 12)
#define		AT91_DDRC2_TRTP_(x)		((x & 0x0f) << 12)
#define	AT91_DDRC2_TFA		(0xFUL << 16)
#define		AT91_DDRC2_TFAW_(x)		((x & 0x0f) << 16)

/* -------- HDDRSDRC2_LPR : (HDDRSDRC2 Offset: 0x1c) --------*/
#define AT91_DDRC2_LPCB	(0x3UL << 0)
#define 	AT91_DDRC2_LPCB_DISABLED	(0x0UL)
#define 	AT91_DDRC2_LPCB_SELFREFRESH	(0x1UL)
#define 	AT91_DDRC2_LPCB_POWERDOWN	(0x2UL)
#define 	AT91_DDRC2_LPCB_DEEP_PWD	(0x3UL)
#define AT91_DDRC2_CLK_FR	(0x1UL << 2)
#define AT91_DDRC2_PASR	(0x7UL << 4)
#define		AT91_DDRC2_PASR_(x)		((x & 0x7) << 4)
#define AT91_DDRC2_DS		(0x7UL << 8)
#define		AT91_DDRC2_DS_(x)		((x & 0x7) << 8)
#define AT91_DDRC2_TIMEOUT	(0x3UL << 12)
#define 	AT91_DDRC2_TIMEOUT_0		(0x0UL << 12)
#define 	AT91_DDRC2_TIMEOUT_64		(0x1UL << 12)
#define 	AT91_DDRC2_TIMEOUT_128		(0x2UL << 12)
#define 	AT91_DDRC2_TIMEOUT_Reserved	(0x3UL << 12)
#define AT91_DDRC2_ADPE	(0x1UL << 16)
#define 	AT91_DDRC2_ADPE_FAST		(0x0UL << 16)
#define 	AT91_DDRC2_ADPE_SLOW		(0x1UL << 16)
#define AT91_DDRC2_UPD_MR	(0x3UL << 20)
#define		AT91_DDRC2_UPD_MR_NO_UPDATE		(0x0UL << 20)
#define		AT91_DDRC2_UPD_MR_SHARED_BUS		(0x1UL << 20)
#define		AT91_DDRC2_UPD_MR_NO_SHARED_BUS	(0x2UL << 20)
#define AT91_DDRC2_SELF_DONE	(0x1UL << 25)

/* -------- HDDRSDRC2_MDR : (HDDRSDRC2 Offset: 0x20) Memory Device Register -------- */
#define AT91_DDRC2_MD		(0x7UL << 0)
#define 	AT91_DDRC2_MD_SDR_SDRAM	(0x0UL)
#define 	AT91_DDRC2_MD_LP_SDR_SDRAM	(0x1UL)
#define 	AT91_DDRC2_MD_DDR_SDRAM	(0x2UL)
#define 	AT91_DDRC2_MD_LP_DDR_SDRAM	(0x3UL)
#define 	AT91_DDRC2_MD_DDR3_SDRAM	(0x4UL)
#define 	AT91_DDRC2_MD_LPDDR3_SDRAM	(0x5UL)
#define 	AT91_DDRC2_MD_DDR2_SDRAM	(0x6UL)
#define		AT91_DDRC2_MD_LPDDR2_SDRAM	(0x7UL)
#define AT91_DDRC2_DBW		(0x1UL << 4)
#define 	AT91_DDRC2_DBW_32_BITS		(0x0UL << 4)
#define 	AT91_DDRC2_DBW_16_BITS		(0x1UL << 4)

/* -------- HDDRSDRC2_DLL : (HDDRSDRC2 Offset: 0x24) DLL Information Register --------*/
#define AT91_DDRC2_MDINC	(0x1UL << 0)
#define AT91_DDRC2_MDDEC	(0x1UL << 1)
#define AT91_DDRC2_MDOVF	(0x1UL << 2)
#define AT91_DDRC2_MDVAL	(0xFFUL << 8)

/* ------- MPDDRC_LPDDR2_LPR (offset: 0x28) */
#define AT91_LPDDRC2_BK_MASK_PASR(value)	(value << 0)
#define AT91_LPDDRC2_SEG_MASK(value)		(value << 8)
#define AT91_LPDDRC2_DS(value)			(value << 24)

/* -------- HDDRSDRC2_HS : (HDDRSDRC2 Offset: 0x2c) High Speed Register --------*/
#define AT91_DDRC2_NO_ANT	(0x1UL << 2)

/* -------- MPDDRC_LPDDR2_CAL_MR4: (MPDDRC Offset: 0x2c) Calibration and MR4 Register --------*/
#define AT91_DDRC2_COUNT_CAL_MASK	(0xFFFFUL)
#define AT91_DDRC2_COUNT_CAL(value)	(((value) & AT91_DDRC2_COUNT_CAL_MASK) << 0)
#define AT91_DDRC2_MR4R(value)		(((value) & 0xFFFFUL) << 16)

/* -------- MPDDRC_LPDDR2_TIM_CAL : (MPDDRC Offset: 0x30) */
#define AT91_DDRC2_ZQCS(value)	(value << 0)

/* -------- MPDDRC_IO_CALIBR : (MPDDRC Offset: 0x34) IO Calibration --------*/
#define AT91_MPDDRC_RDIV	(0x7UL << 0)
#define 	AT91_MPDDRC_RDIV_LPDDR2_RZQ_34		(0x1UL << 0)
#define 	AT91_MPDDRC_RDIV_LPDDR2_RZQ_48		(0x3UL << 0)
#define 	AT91_MPDDRC_RDIV_LPDDR2_RZQ_60		(0x4UL << 0)
#define 	AT91_MPDDRC_RDIV_LPDDR2_RZQ_120	(0x7UL << 0)

#define 	AT91_MPDDRC_RDIV_DDR2_RZQ_33_3		(0x2UL << 0)
#define 	AT91_MPDDRC_RDIV_DDR2_RZQ_50		(0x4UL << 0)
#define		AT91_MPDDRC_RDIV_DDR2_RZQ_66_7		(0x6UL << 0)
#define 	AT91_MPDDRC_RDIV_DDR2_RZQ_100		(0x7UL << 0)

#define		AT91_MPDDRC_RDIV_LPDDR3_RZQ_38		(0x02UL << 0)
#define		AT91_MPDDRC_RDIV_LPDDR3_RZQ_46		(0x03UL << 0)
#define		AT91_MPDDRC_RDIV_LPDDR3_RZQ_57		(0x04UL << 0)
#define		AT91_MPDDRC_RDIV_LPDDR3_RZQ_77		(0x06UL << 0)
#define		AT91_MPDDRC_RDIV_LPDDR3_RZQ_115	(0x07UL << 0)

#define	AT91_MPDDRC_ENABLE_CALIB	(0x01 << 4)
#define		AT91_MPDDRC_DISABLE_CALIB		(0x00 << 4)
#define		AT91_MPDDRC_EN_CALIB		(0x01 << 4)

#define	AT91_MPDDRC_TZQIO	(0x7FUL << 8)
#define	AT91_MPDDRC_TZQIO_(x)		((x) << 8)
#define		AT91_MPDDRC_TZQIO_0	(0x0UL << 8)
#define		AT91_MPDDRC_TZQIO_1	(0x1UL << 8)
#define		AT91_MPDDRC_TZQIO_3	(0x3UL << 8)
#define		AT91_MPDDRC_TZQIO_4	(0x4UL << 8)
#define		AT91_MPDDRC_TZQIO_5	(0x5UL << 8)
#define		AT91_MPDDRC_TZQIO_31	(0x1FUL << 8)

#define	AT91_MPDDRC_CALCODEP	(0xFUL << 16)
#define		AT91_MPDDRC_CALCODEP_(x)	((x) << 16)

#define	AT91_MPDDRC_CALCODEN	(0xFUL << 20)
#define		AT91_MPDDRC_CALCODEN_(x)	((x) << 20)

/* ---- MPDDRC_RD_DATA_PATH : (MPDDRC Offset: 0x5c) MPDDRC Read Data Path */
#define AT91_MPDDRC_SHIFT_SAMPLING	(0x03 << 0)
#define		AT91_MPDDRC_RD_DATA_PATH_NO_SHIFT	(0x00 << 0)
#define		AT91_MPDDRC_RD_DATA_PATH_ONE_CYCLES	(0x01 << 0)
#define		AT91_MPDDRC_RD_DATA_PATH_TWO_CYCLES	(0x02 << 0)
#define		AT91_MPDDRC_RD_DATA_PATH_THREE_CYCLES	(0x03 << 0)

/* -------- MPDDRC_DLL_MOR : (MPDDRC Offset: 0x74) DLL Master Offset Register --------*/
#define AT91_MPDDRC_MOFF(value)	(value << 0)
#define 	AT91_MPDDRC_MOFF_1	(0x1UL << 0)
#define 	AT91_MPDDRC_MOFF_7	(0x7UL << 0)
#define AT91_MPDDRC_CLK90OFF(value)		(value << 8)
#define 	AT91_MPDDRC_CLK90OFF_1		(0x1UL << 8)
#define 	AT91_MPDDRC_CLK90OFF_31	(0x1FUL << 8)
#define AT91_MPDDRC_SELOFF	(0x1UL << 16)
#define 	AT91_MPDDRC_SELOFF_DISABLED	(0x0UL << 16)
#define 	AT91_MPDDRC_SELOFF_ENABLED	(0x1UL << 16)
#define AT91_MPDDRC_KEY	(0xC5UL << 24)

/* -------- MPDDRC_DLL_SOR : (MPDDRC Offset: 0x78) DLL Slave Offset Register --------*/
#define AT91_MPDDRC_S0OFF_1	(0x1UL << 0)
#define AT91_MPDDRC_S1OFF_1	(0x1UL << 8)
#define AT91_MPDDRC_S2OFF_1	(0x1UL << 16)
#define AT91_MPDDRC_S3OFF_1	(0x1UL << 24)

#define AT91_MPDDRC_S0OFF(value)	(value << 0)
#define AT91_MPDDRC_S1OFF(value)	(value << 8)
#define AT91_MPDDRC_S2OFF(value)	(value << 16)
#define AT91_MPDDRC_S3OFF(value)	(value << 24)

/* -------- HDDRSDRC2_WPCR : (HDDRSDRC2 Offset: 0xe4) Write Protect Control Register --------*/
#define AT91_DDRC2_WPEN	(0x1UL << 0)
#define AT91_DDRC2_WPKEY	(0xFFFFFFUL << 8)

/* -------- HDDRSDRC2_WPSR : (HDDRSDRC2 Offset: 0xe8) Write Protect Status Register --------*/
#define AT91_DDRC2_WPVS	(0x1UL << 0)
#define AT91_DDRC2_WPSRC	(0xFFFFUL << 8)

#ifndef __ASSEMBLY__
#include <common.h>
#include <io.h>
#include <mach/at91/hardware.h>

static inline u32 at91_get_ddram_size(void __iomem *base, bool is_nb)
{
	u32 cr;
	u32 mdr;
	u32 size;
	bool is_sdram;

	cr = readl(base + AT91_HDDRSDRC2_CR);
	mdr = readl(base + AT91_HDDRSDRC2_MDR);

	/* will always be false for sama5d2, sama5d3 or sama5d4 */
	is_sdram = (mdr & AT91_DDRC2_MD) <= AT91_DDRC2_MD_LP_SDR_SDRAM;

	/* Formula:
	 * size = bank << (col + row + 1);
	 * if (bandwidth == 32 bits)
	 *	size <<= 1;
	 */
	size = 1;
	/* COL */
	size += (cr & AT91_DDRC2_NC) + 8;
	if (!is_sdram)
		size ++;
	/* ROW */
	size += ((cr & AT91_DDRC2_NR) >> 2) + 11;
	/* BANK */
	if (is_nb)
		size = ((cr & AT91_DDRC2_NB_BANKS) ? 8 : 4) << size;
	else
		size = 4 << size;

	/* bandwidth */
	if (!(mdr & AT91_DDRC2_DBW))
		size <<= 1;

	return size;
}

static inline u32 at91sam9g45_get_ddram_size(int bank)
{
	switch (bank) {
	case 0:
		return at91_get_ddram_size(IOMEM(AT91SAM9G45_BASE_DDRSDRC0), false);
	case 1:
		return at91_get_ddram_size(IOMEM(AT91SAM9G45_BASE_DDRSDRC1), false);
	default:
		return 0;
	}
}

static inline u32 at91sam9x5_get_ddram_size(void)
{
	return at91_get_ddram_size(IOMEM(AT91SAM9X5_BASE_DDRSDRC0), true);
}

static inline u32 at91sam9n12_get_ddram_size(void)
{
	return at91_get_ddram_size(IOMEM(AT91SAM9N12_BASE_DDRSDRC0), true);
}

static inline u32 at91sama5d4_get_ddram_size(void)
{
	return at91_get_ddram_size(IOMEM(SAMA5D4_BASE_MPDDRC), true);
}


#endif /* __ASSEMBLY__ */

#endif	/* #ifndef __AT91_DDRSDRC_H__ */
