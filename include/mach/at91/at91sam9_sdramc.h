/* SPDX-License-Identifier: GPL-2.0-or-later */
/* SPDX-FileCopyrightText: 2009 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com> */
/* SPDX-FileCopyrightText: 2007 Andrew Victor */
/* SPDX-FileCopyrightText: 2007 Atmel Corporation */

/*
 * [origin: Linux kernel arch/arm/mach-at91/include/mach/at91_wdt.h]
 *
 * SDRAM Controllers (SDRAMC) - System peripherals registers.
 * Based on AT91SAM9261 datasheet revision D.
 */

#ifndef AT91SAM9_SDRAMC_H
#define AT91SAM9_SDRAMC_H

#include <linux/compiler.h>

/* SDRAM Controller (SDRAMC) registers */
#define AT91_SDRAMC_MR		0x00	/* SDRAM Controller Mode Register */
#define		AT91_SDRAMC_MODE	(0xf << 0)		/* Command Mode */
#define			AT91_SDRAMC_MODE_NORMAL		0
#define			AT91_SDRAMC_MODE_NOP		1
#define			AT91_SDRAMC_MODE_PRECHARGE	2
#define			AT91_SDRAMC_MODE_LMR		3
#define			AT91_SDRAMC_MODE_REFRESH	4
#define			AT91_SDRAMC_MODE_EXT_LMR	5
#define			AT91_SDRAMC_MODE_DEEP		6

#define AT91_SDRAMC_TR		0x04	/* SDRAM Controller Refresh Timer Register */
#define		AT91_SDRAMC_COUNT	(0xfff << 0)		/* Refresh Timer Counter */

#define AT91_SDRAMC_CR		0x08	/* SDRAM Controller Configuration Register */
#define		AT91_SDRAMC_NC		(3 << 0)		/* Number of Column Bits */
#define			AT91_SDRAMC_NC_8	(0 << 0)
#define			AT91_SDRAMC_NC_9	(1 << 0)
#define			AT91_SDRAMC_NC_10	(2 << 0)
#define			AT91_SDRAMC_NC_11	(3 << 0)
#define		AT91_SDRAMC_NR		(3 << 2)		/* Number of Row Bits */
#define			AT91_SDRAMC_NR_11	(0 << 2)
#define			AT91_SDRAMC_NR_12	(1 << 2)
#define			AT91_SDRAMC_NR_13	(2 << 2)
#define		AT91_SDRAMC_NB		(1 << 4)		/* Number of Banks */
#define			AT91_SDRAMC_NB_2	(0 << 4)
#define			AT91_SDRAMC_NB_4	(1 << 4)
#define		AT91_SDRAMC_CAS		(3 << 5)		/* CAS Latency */
#define			AT91_SDRAMC_CAS_1	(1 << 5)
#define			AT91_SDRAMC_CAS_2	(2 << 5)
#define			AT91_SDRAMC_CAS_3	(3 << 5)
#define		AT91_SDRAMC_DBW		(1 << 7)		/* Data Bus Width */
#define			AT91_SDRAMC_DBW_32	(0 << 7)
#define			AT91_SDRAMC_DBW_16	(1 << 7)
#define AT91_SDRAMC_TWR	(0xF << 8)	/* Number of Write Recovery Time Cycles */
#define 	AT91_SDRAMC_TWR_0		(0x0 << 8)
#define 	AT91_SDRAMC_TWR_1		(0x1 << 8)
#define 	AT91_SDRAMC_TWR_2		(0x2 << 8)
#define 	AT91_SDRAMC_TWR_3		(0x3 << 8)
#define 	AT91_SDRAMC_TWR_4		(0x4 << 8)
#define 	AT91_SDRAMC_TWR_5		(0x5 << 8)
#define 	AT91_SDRAMC_TWR_6		(0x6 << 8)
#define 	AT91_SDRAMC_TWR_7		(0x7 << 8)
#define 	AT91_SDRAMC_TWR_8		(0x8 << 8)
#define 	AT91_SDRAMC_TWR_9		(0x9 << 8)
#define 	AT91_SDRAMC_TWR_10		(0xA << 8)
#define 	AT91_SDRAMC_TWR_11		(0xB << 8)
#define 	AT91_SDRAMC_TWR_12		(0xC << 8)
#define 	AT91_SDRAMC_TWR_13		(0xD << 8)
#define 	AT91_SDRAMC_TWR_14		(0xE << 8)
#define 	AT91_SDRAMC_TWR_15		(0xF << 8)
#define AT91_SDRAMC_TRC	(0xF << 12)	/* Number of Row Cycle Delay Time Cycles */
#define 	AT91_SDRAMC_TRC_0		(0x0 << 12)
#define 	AT91_SDRAMC_TRC_1		(0x1 << 12)
#define 	AT91_SDRAMC_TRC_2		(0x2 << 12)
#define 	AT91_SDRAMC_TRC_3		(0x3 << 12)
#define 	AT91_SDRAMC_TRC_4		(0x4 << 12)
#define 	AT91_SDRAMC_TRC_5		(0x5 << 12)
#define 	AT91_SDRAMC_TRC_6		(0x6 << 12)
#define 	AT91_SDRAMC_TRC_7		(0x7 << 12)
#define 	AT91_SDRAMC_TRC_8		(0x8 << 12)
#define 	AT91_SDRAMC_TRC_9		(0x9 << 12)
#define 	AT91_SDRAMC_TRC_10		(0xA << 12)
#define 	AT91_SDRAMC_TRC_11		(0xB << 12)
#define 	AT91_SDRAMC_TRC_12		(0xC << 12)
#define 	AT91_SDRAMC_TRC_13		(0xD << 12)
#define 	AT91_SDRAMC_TRC_14		(0xE << 12)
#define 	AT91_SDRAMC_TRC_15		(0xF << 12)
#define AT91_SDRAMC_TRP	(0xF << 16)	/* Number of Row Precharge Delay Time Cycles */
#define 	AT91_SDRAMC_TRP_0		(0x0 << 16)
#define 	AT91_SDRAMC_TRP_1		(0x1 << 16)
#define 	AT91_SDRAMC_TRP_2		(0x2 << 16)
#define 	AT91_SDRAMC_TRP_3		(0x3 << 16)
#define 	AT91_SDRAMC_TRP_4		(0x4 << 16)
#define 	AT91_SDRAMC_TRP_5		(0x5 << 16)
#define 	AT91_SDRAMC_TRP_6		(0x6 << 16)
#define 	AT91_SDRAMC_TRP_7		(0x7 << 16)
#define 	AT91_SDRAMC_TRP_8		(0x8 << 16)
#define 	AT91_SDRAMC_TRP_9		(0x9 << 16)
#define 	AT91_SDRAMC_TRP_10		(0xA << 16)
#define 	AT91_SDRAMC_TRP_11		(0xB << 16)
#define 	AT91_SDRAMC_TRP_12		(0xC << 16)
#define 	AT91_SDRAMC_TRP_13		(0xD << 16)
#define 	AT91_SDRAMC_TRP_14		(0xE << 16)
#define 	AT91_SDRAMC_TRP_15		(0xF << 16)
#define AT91_SDRAMC_TRCD	(0xF << 20)	/* Number of Row to Column Delay Time Cycles */
#define 	AT91_SDRAMC_TRCD_0		(0x0 << 20)
#define 	AT91_SDRAMC_TRCD_1		(0x1 << 20)
#define 	AT91_SDRAMC_TRCD_2		(0x2 << 20)
#define 	AT91_SDRAMC_TRCD_3		(0x3 << 20)
#define 	AT91_SDRAMC_TRCD_4		(0x4 << 20)
#define 	AT91_SDRAMC_TRCD_5		(0x5 << 20)
#define 	AT91_SDRAMC_TRCD_6		(0x6 << 20)
#define 	AT91_SDRAMC_TRCD_7		(0x7 << 20)
#define 	AT91_SDRAMC_TRCD_8		(0x8 << 20)
#define 	AT91_SDRAMC_TRCD_9		(0x9 << 20)
#define 	AT91_SDRAMC_TRCD_10		(0xA << 20)
#define 	AT91_SDRAMC_TRCD_11		(0xB << 20)
#define 	AT91_SDRAMC_TRCD_12		(0xC << 20)
#define 	AT91_SDRAMC_TRCD_13		(0xD << 20)
#define 	AT91_SDRAMC_TRCD_14		(0xE << 20)
#define 	AT91_SDRAMC_TRCD_15		(0xF << 20)
#define AT91_SDRAMC_TRAS	(0xF << 24)	/* Number of Active to Precharge Delay Time Cycles */
#define 	AT91_SDRAMC_TRAS_0		(0x0 << 24)
#define 	AT91_SDRAMC_TRAS_1		(0x1 << 24)
#define 	AT91_SDRAMC_TRAS_2		(0x2 << 24)
#define 	AT91_SDRAMC_TRAS_3		(0x3 << 24)
#define 	AT91_SDRAMC_TRAS_4		(0x4 << 24)
#define 	AT91_SDRAMC_TRAS_5		(0x5 << 24)
#define 	AT91_SDRAMC_TRAS_6		(0x6 << 24)
#define 	AT91_SDRAMC_TRAS_7		(0x7 << 24)
#define 	AT91_SDRAMC_TRAS_8		(0x8 << 24)
#define 	AT91_SDRAMC_TRAS_9		(0x9 << 24)
#define 	AT91_SDRAMC_TRAS_10		(0xA << 24)
#define 	AT91_SDRAMC_TRAS_11		(0xB << 24)
#define 	AT91_SDRAMC_TRAS_12		(0xC << 24)
#define 	AT91_SDRAMC_TRAS_13		(0xD << 24)
#define 	AT91_SDRAMC_TRAS_14		(0xE << 24)
#define 	AT91_SDRAMC_TRAS_15		(0xF << 24)
#define AT91_SDRAMC_TXS	(0xF << 28)	/* Number of Exit Self Refresh to Active Delay Time Cycles */
#define 	AT91_SDRAMC_TXSR_0		(0x0 << 28)
#define 	AT91_SDRAMC_TXSR_1		(0x1 << 28)
#define 	AT91_SDRAMC_TXSR_2		(0x2 << 28)
#define 	AT91_SDRAMC_TXSR_3		(0x3 << 28)
#define 	AT91_SDRAMC_TXSR_4		(0x4 << 28)
#define 	AT91_SDRAMC_TXSR_5		(0x5 << 28)
#define 	AT91_SDRAMC_TXSR_6		(0x6 << 28)
#define 	AT91_SDRAMC_TXSR_7		(0x7 << 28)
#define 	AT91_SDRAMC_TXSR_8		(0x8 << 28)
#define 	AT91_SDRAMC_TXSR_9		(0x9 << 28)
#define 	AT91_SDRAMC_TXSR_10		(0xA << 28)
#define 	AT91_SDRAMC_TXSR_11		(0xB << 28)
#define 	AT91_SDRAMC_TXSR_12		(0xC << 28)
#define 	AT91_SDRAMC_TXSR_13		(0xD << 28)
#define 	AT91_SDRAMC_TXSR_14		(0xE << 28)
#define 	AT91_SDRAMC_TXSR_15		(0xF << 28)

#define AT91_SDRAMC_LPR		0x10	/* SDRAM Controller Low Power Register */
#define		AT91_SDRAMC_LPCB		(3 << 0)	/* Low-power Configurations */
#define			AT91_SDRAMC_LPCB_DISABLE		0
#define			AT91_SDRAMC_LPCB_SELF_REFRESH		1
#define			AT91_SDRAMC_LPCB_POWER_DOWN		2
#define			AT91_SDRAMC_LPCB_DEEP_POWER_DOWN	3
#define		AT91_SDRAMC_PASR		(7 << 4)	/* Partial Array Self Refresh */
#define		AT91_SDRAMC_TCSR		(3 << 8)	/* Temperature Compensated Self Refresh */
#define		AT91_SDRAMC_DS			(3 << 10)	/* Drive Strength */
#define		AT91_SDRAMC_TIMEOUT		(3 << 12)	/* Time to define when Low Power Mode is enabled */
#define			AT91_SDRAMC_TIMEOUT_0_CLK_CYCLES	(0 << 12)
#define			AT91_SDRAMC_TIMEOUT_64_CLK_CYCLES	(1 << 12)
#define			AT91_SDRAMC_TIMEOUT_128_CLK_CYCLES	(2 << 12)

#define AT91_SDRAMC_IER		0x14	/* SDRAM Controller Interrupt Enable Register */
#define AT91_SDRAMC_IDR		0x18	/* SDRAM Controller Interrupt Disable Register */
#define AT91_SDRAMC_IMR		0x1C	/* SDRAM Controller Interrupt Mask Register */
#define AT91_SDRAMC_ISR		0x20	/* SDRAM Controller Interrupt Status Register */
#define		AT91_SDRAMC_RES		(1 << 0)		/* Refresh Error Status */

#define AT91_SDRAMC_MDR		0x24	/* SDRAM Memory Device Register */
#define		AT91_SDRAMC_MD		(3 << 0)		/* Memory Device Type */
#define			AT91_SDRAMC_MD_SDRAM		0
#define			AT91_SDRAMC_MD_LOW_POWER_SDRAM	1

#ifndef __ASSEMBLY__
#include <io.h>
#include <mach/at91/at91sam9260.h>
#include <mach/at91/at91sam9261.h>
#include <mach/at91/at91sam9263.h>

struct at91sam9_sdramc_config {
	void __iomem *sdramc;
	unsigned int mr;
	unsigned int tr;
	unsigned int cr;
	unsigned int lpr;
	unsigned int mdr;
};

int at91sam9_sdramc_initialize(const struct at91sam9_sdramc_config *config,
			       unsigned int sdram_address);

static inline u32 at91_get_sdram_size(void *base)
{
	u32 val;
	u32 size;

	val = readl(base + AT91_SDRAMC_CR);

	/* Formula:
	 * size = bank << (col + row + 1);
	 * if (bandwidth == 32 bits)
	 *	size <<= 1;
	 */
	size = 1;
	/* COL */
	size += (val & AT91_SDRAMC_NC) + 8;
	/* ROW */
	size += ((val & AT91_SDRAMC_NR) >> 2) + 11;
	/* BANK */
	size = ((val & AT91_SDRAMC_NB) ? 4 : 2) << size;
	/* bandwidth */
	if (!(val & AT91_SDRAMC_DBW))
		size <<= 1;

	return size;
}

static inline bool at91_is_low_power_sdram(void *base)
{
	return readl(base + AT91_SDRAMC_MDR) & AT91_SDRAMC_MD_LOW_POWER_SDRAM;
}

static inline u32 at91sam9260_get_sdram_size(void)
{
	return at91_get_sdram_size(IOMEM(AT91SAM9260_BASE_SDRAMC));
}

static inline bool at91sam9260_is_low_power_sdram(void)
{
	return at91_is_low_power_sdram(IOMEM(AT91SAM9260_BASE_SDRAMC));
}

static inline u32 at91sam9261_get_sdram_size(void)
{
	return at91_get_sdram_size(IOMEM(AT91SAM9261_BASE_SDRAMC));
}

static inline bool at91sam9261_is_low_power_sdram(void)
{
	return at91_is_low_power_sdram(IOMEM(AT91SAM9261_BASE_SDRAMC));
}

static inline u32 at91sam9263_get_sdram_size(int bank)
{
	switch (bank) {
	case 0:
		return at91_get_sdram_size(IOMEM(AT91SAM9263_BASE_SDRAMC0));
	case 1:
		return at91_get_sdram_size(IOMEM(AT91SAM9263_BASE_SDRAMC1));
	default:
		return 0;
	}
}

static inline bool at91sam9263_is_low_power_sdram(int bank)
{
	switch (bank) {
	case 0:
		return at91_is_low_power_sdram(IOMEM(AT91SAM9263_BASE_SDRAMC0));
	case 1:
		return at91_is_low_power_sdram(IOMEM(AT91SAM9263_BASE_SDRAMC1));
	default:
		return false;
	}
}

void __noreturn at91sam9260_barebox_entry(void *boarddata);

#endif
#endif
