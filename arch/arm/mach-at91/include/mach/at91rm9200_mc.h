/*
 * arch/arm/mach-at91/include/mach/at91rm9200_mc.h
 *
 * Copyright (C) 2005 Ivan Kokshaysky
 * Copyright (C) SAN People
 *
 * Memory Controllers (MC, EBI, SMC, SDRAMC, BFC) - System peripherals registers.
 * Based on AT91RM9200 datasheet revision E.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef AT91RM9200_MC_H
#define AT91RM9200_MC_H

/* Memory Controller */
#define AT91RM9200_MC_RCR		(0x00)	/* MC Remap Control Register */
#define		AT91RM9200_MC_RCB		(1 <<  0)		/* Remap Command Bit */

#define AT91RM9200_MC_ASR		(0x04)	/* MC Abort Status Register */
#define		AT91RM9200_MC_UNADD		(1 <<  0)		/* Undefined Address Abort Status */
#define		AT91RM9200_MC_MISADD		(1 <<  1)		/* Misaligned Address Abort Status */
#define		AT91RM9200_MC_ABTSZ		(3 <<  8)		/* Abort Size Status */
#define			AT91RM9200_MC_ABTSZ_BYTE		(0 << 8)
#define			AT91RM9200_MC_ABTSZ_HALFWORD		(1 << 8)
#define			AT91RM9200_MC_ABTSZ_WORD		(2 << 8)
#define		AT91RM9200_MC_ABTTYP		(3 << 10)		/* Abort Type Status */
#define			AT91RM9200_MC_ABTTYP_DATAREAD		(0 << 10)
#define			AT91RM9200_MC_ABTTYP_DATAWRITE	(1 << 10)
#define			AT91RM9200_MC_ABTTYP_FETCH		(2 << 10)
#define		AT91RM9200_MC_MST0		(1 << 16)		/* ARM920T Abort Source */
#define		AT91RM9200_MC_MST1		(1 << 17)		/* PDC Abort Source */
#define		AT91RM9200_MC_MST2		(1 << 18)		/* UHP Abort Source */
#define		AT91RM9200_MC_MST3		(1 << 19)		/* EMAC Abort Source */
#define		AT91RM9200_MC_SVMST0		(1 << 24)		/* Saved ARM920T Abort Source */
#define		AT91RM9200_MC_SVMST1		(1 << 25)		/* Saved PDC Abort Source */
#define		AT91RM9200_MC_SVMST2		(1 << 26)		/* Saved UHP Abort Source */
#define		AT91RM9200_MC_SVMST3		(1 << 27)		/* Saved EMAC Abort Source */

#define AT91RM9200_MC_AASR		(0x08)	/* MC Abort Address Status Register */

#define AT91RM9200_MC_MPR		(0x0c)	/* MC Master Priority Register */
#define		AT91RM9200_MPR_MSTP0		(7 <<  0)		/* ARM920T Priority */
#define		AT91RM9200_MPR_MSTP1		(7 <<  4)		/* PDC Priority */
#define		AT91RM9200_MPR_MSTP2		(7 <<  8)		/* UHP Priority */
#define		AT91RM9200_MPR_MSTP3		(7 << 12)		/* EMAC Priority */

/* External Bus Interface (EBI) registers */
#define AT91RM9200_EBI_CSA		(0x60)	/* Chip Select Assignment Register */
#define		AT91RM9200_EBI_CS0A		(1 << 0)		/* Chip Select 0 Assignment */
#define			AT91RM9200_EBI_CS0A_SMC		(0 << 0)
#define			AT91RM9200_EBI_CS0A_BFC		(1 << 0)
#define		AT91RM9200_EBI_CS1A		(1 << 1)		/* Chip Select 1 Assignment */
#define			AT91RM9200_EBI_CS1A_SMC		(0 << 1)
#define			AT91RM9200_EBI_CS1A_SDRAMC		(1 << 1)
#define		AT91RM9200_EBI_CS3A		(1 << 3)		/* Chip Select 2 Assignment */
#define			AT91RM9200_EBI_CS3A_SMC		(0 << 3)
#define			AT91RM9200_EBI_CS3A_SMC_SMARTMEDIA	(1 << 3)
#define		AT91RM9200_EBI_CS4A		(1 << 4)		/* Chip Select 3 Assignment */
#define			AT91RM9200_EBI_CS4A_SMC		(0 << 4)
#define			AT91RM9200_EBI_CS4A_SMC_COMPACTFLASH	(1 << 4)
#define AT91RM9200_EBI_CFGR		(0x64)	/* Configuration Register */
#define		AT91RM9200_EBI_DBPUC		(1 << 0)		/* Data Bus Pull-Up Configuration */

/* Static Memory Controller (SMC) registers */
#define	AT91RM9200_SMC_CSR(n)		(0x70 + ((n) * 4))/* SMC Chip Select Register */
#define		AT91RM9200_SMC_NWS		(0x7f <<  0)		/* Number of Wait States */
#define			AT91RM9200_SMC_NWS_(x)	((x) << 0)
#define		AT91RM9200_SMC_WSEN		(1    <<  7)		/* Wait State Enable */
#define		AT91RM9200_SMC_TDF		(0xf  <<  8)		/* Data Float Time */
#define			AT91RM9200_SMC_TDF_(x)	((x) << 8)
#define		AT91RM9200_SMC_BAT		(1    << 12)		/* Byte Access Type */
#define		AT91RM9200_SMC_DBW		(3    << 13)		/* Data Bus Width */
#define			AT91RM9200_SMC_DBW_16		(1 << 13)
#define			AT91RM9200_SMC_DBW_8		(2 << 13)
#define		AT91RM9200_SMC_DPR		(1 << 15)		/* Data Read Protocol */
#define		AT91RM9200_SMC_ACSS		(3 << 16)		/* Address to Chip Select Setup */
#define			AT91RM9200_SMC_ACSS_STD	(0 << 16)
#define			AT91RM9200_SMC_ACSS_1		(1 << 16)
#define			AT91RM9200_SMC_ACSS_2		(2 << 16)
#define			AT91RM9200_SMC_ACSS_3		(3 << 16)
#define		AT91RM9200_SMC_RWSETUP	(7 << 24)		/* Read & Write Signal Time Setup */
#define			AT91RM9200_SMC_RWSETUP_(x)	((x) << 24)
#define		AT91RM9200_SMC_RWHOLD		(7 << 28)		/* Read & Write Signal Hold Time */
#define			AT91RM9200_SMC_RWHOLD_(x)	((x) << 28)

/* SDRAM Controller registers */
#define AT91RM9200_SDRAMC_MR		(0x90)	/* Mode Register */
#define		AT91RM9200_SDRAMC_MODE	(0xf << 0)		/* Command Mode */
#define			AT91RM9200_SDRAMC_MODE_NORMAL		(0 << 0)
#define			AT91RM9200_SDRAMC_MODE_NOP		(1 << 0)
#define			AT91RM9200_SDRAMC_MODE_PRECHARGE	(2 << 0)
#define			AT91RM9200_SDRAMC_MODE_LMR		(3 << 0)
#define			AT91RM9200_SDRAMC_MODE_REFRESH	(4 << 0)
#define		AT91RM9200_SDRAMC_DBW		(1   << 4)		/* Data Bus Width */
#define			AT91RM9200_SDRAMC_DBW_32	(0 << 4)
#define			AT91RM9200_SDRAMC_DBW_16	(1 << 4)

#define AT91RM9200_SDRAMC_TR		(0x94)	/* Refresh Timer Register */
#define		AT91RM9200_SDRAMC_COUNT	(0xfff << 0)		/* Refresh Timer Count */

#define AT91RM9200_SDRAMC_CR		(0x98)	/* Configuration Register */
#define		AT91RM9200_SDRAMC_NC		(3   <<  0)		/* Number of Column Bits */
#define			AT91RM9200_SDRAMC_NC_8	(0 << 0)
#define			AT91RM9200_SDRAMC_NC_9	(1 << 0)
#define			AT91RM9200_SDRAMC_NC_10	(2 << 0)
#define			AT91RM9200_SDRAMC_NC_11	(3 << 0)
#define		AT91RM9200_SDRAMC_NR		(3   <<  2)		/* Number of Row Bits */
#define			AT91RM9200_SDRAMC_NR_11	(0 << 2)
#define			AT91RM9200_SDRAMC_NR_12	(1 << 2)
#define			AT91RM9200_SDRAMC_NR_13	(2 << 2)
#define		AT91RM9200_SDRAMC_NB		(1   <<  4)		/* Number of Banks */
#define			AT91RM9200_SDRAMC_NB_2	(0 << 4)
#define			AT91RM9200_SDRAMC_NB_4	(1 << 4)
#define		AT91RM9200_SDRAMC_CAS		(3   <<  5)		/* CAS Latency */
#define			AT91RM9200_SDRAMC_CAS_2	(2 << 5)
#define		AT91RM9200_SDRAMC_TWR		(0xf <<  7)		/* Write Recovery Delay */
#define		AT91RM9200_SDRAMC_TRC		(0xf << 11)		/* Row Cycle Delay */
#define		AT91RM9200_SDRAMC_TRP		(0xf << 15)		/* Row Precharge Delay */
#define		AT91RM9200_SDRAMC_TRCD	(0xf << 19)		/* Row to Column Delay */
#define		AT91RM9200_SDRAMC_TRAS	(0xf << 23)		/* Active to Precharge Delay */
#define		AT91RM9200_SDRAMC_TXSR	(0xf << 27)		/* Exit Self Refresh to Active Delay */

#define AT91RM9200_SDRAMC_SRR		(0x9c)	/* Self Refresh Register */
#define AT91RM9200_SDRAMC_LPR		(0xa0)	/* Low Power Register */
#define AT91RM9200_SDRAMC_IER		(0xa4)	/* Interrupt Enable Register */
#define AT91RM9200_SDRAMC_IDR		(0xa8)	/* Interrupt Disable Register */
#define AT91RM9200_SDRAMC_IMR		(0xac)	/* Interrupt Mask Register */
#define AT91RM9200_SDRAMC_ISR		(0xb0)	/* Interrupt Status Register */

/* Burst Flash Controller register */
#define AT91RM9200_BFC_MR		(0xc0)	/* Mode Register */
#define		AT91RM9200_BFC_BFCOM		(3   <<  0)		/* Burst Flash Controller Operating Mode */
#define			AT91RM9200_BFC_BFCOM_DISABLED	(0 << 0)
#define			AT91RM9200_BFC_BFCOM_ASYNC	(1 << 0)
#define			AT91RM9200_BFC_BFCOM_BURST	(2 << 0)
#define		AT91RM9200_BFC_BFCC		(3   <<  2)		/* Burst Flash Controller Clock */
#define			AT91RM9200_BFC_BFCC_MCK	(1 << 2)
#define			AT91RM9200_BFC_BFCC_DIV2	(2 << 2)
#define			AT91RM9200_BFC_BFCC_DIV4	(3 << 2)
#define		AT91RM9200_BFC_AVL		(0xf <<  4)		/* Address Valid Latency */
#define		AT91RM9200_BFC_PAGES		(7   <<  8)		/* Page Size */
#define			AT91RM9200_BFC_PAGES_NO_PAGE	(0 << 8)
#define			AT91RM9200_BFC_PAGES_16	(1 << 8)
#define			AT91RM9200_BFC_PAGES_32	(2 << 8)
#define			AT91RM9200_BFC_PAGES_64	(3 << 8)
#define			AT91RM9200_BFC_PAGES_128	(4 << 8)
#define			AT91RM9200_BFC_PAGES_256	(5 << 8)
#define			AT91RM9200_BFC_PAGES_512	(6 << 8)
#define			AT91RM9200_BFC_PAGES_1024	(7 << 8)
#define		AT91RM9200_BFC_OEL		(3   << 12)		/* Output Enable Latency */
#define		AT91RM9200_BFC_BAAEN		(1   << 16)		/* Burst Address Advance Enable */
#define		AT91RM9200_BFC_BFOEH		(1   << 17)		/* Burst Flash Output Enable Handling */
#define		AT91RM9200_BFC_MUXEN		(1   << 18)		/* Multiplexed Bus Enable */
#define		AT91RM9200_BFC_RDYEN		(1   << 19)		/* Ready Enable Mode */

#ifndef __ASSEMBLY__
#include <io.h>
#include <mach/at91rm9200.h>
static inline u32 at91rm9200_get_sdram_size(void)
{
	u32 cr, mr;
	u32 size;

	cr = readl(AT91RM9200_BASE_MC + AT91RM9200_SDRAMC_CR);
	mr = readl(AT91RM9200_BASE_MC + AT91RM9200_SDRAMC_MR);

	/* Formula:
	 * size = bank << (col + row + 1);
	 * if (bandwidth == 32 bits)
	 *	size <<= 1;
	 */
	size = 1;
	/* COL */
	size += (cr & AT91RM9200_SDRAMC_NC) + 8;
	/* ROW */
	size += ((cr & AT91RM9200_SDRAMC_NR) >> 2) + 11;
	/* BANK */
	size = ((cr & AT91RM9200_SDRAMC_NB) ? 4 : 2) << size;
	/* bandwidth */
	if (!(mr & AT91RM9200_SDRAMC_DBW))
		size <<= 1;

	return size;
}
#endif

#endif
