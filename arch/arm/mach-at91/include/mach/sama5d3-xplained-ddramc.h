/* SPDX-License-Identifier: BSD-1-Clause
 *
 * Copyright (C) 2014, Atmel Corporation
 *
 * SAMA5D27 System-in-Package DDRAMC configuration
 */

#include <mach/at91_ddrsdrc.h>
#include <mach/ddramc.h>
#include <mach/sama5d3_ll.h>

static inline void sama5d3_xplained_ddrconf(void)
{
	const struct at91_ddramc_register conf = {
		.mdr = AT91_DDRC2_DBW_32_BITS | AT91_DDRC2_MD_DDR2_SDRAM,

		.cr = AT91_DDRC2_NC_DDR10_SDR9
			| AT91_DDRC2_NR_13
			| AT91_DDRC2_CAS_3
			| AT91_DDRC2_DISABLE_RESET_DLL
			| AT91_DDRC2_ENABLE_DLL
			| AT91_DDRC2_ENRDM_ENABLE
			| AT91_DDRC2_NB_BANKS_8
			| AT91_DDRC2_NDQS_DISABLED
			| AT91_DDRC2_DECOD_INTERLEAVED
			| AT91_DDRC2_UNAL_SUPPORTED,

		/*
		 * The DDR2-SDRAM device requires a refresh every 15.625 us or 7.81 us.
		 * With a 133 MHz frequency, the refresh timer count register must to be
		 * set with (15.625 x 133 MHz) ~ 2084 i.e. 0x824
		 * or (7.81 x 133 MHz) ~ 1039 i.e. 0x40F.
		 */
		.rtr = 0x40F,     /* Refresh timer: 7.812us */

		/* One clock cycle @ 133 MHz = 7.5 ns */
		.t0pr = AT91_DDRC2_TRAS_(6)	/* 6 * 7.5 = 45 ns */
			| AT91_DDRC2_TRCD_(2)	/* 2 * 7.5 = 22.5 ns */
			| AT91_DDRC2_TWR_(2)	/* 2 * 7.5 = 15   ns */
			| AT91_DDRC2_TRC_(8)	/* 8 * 7.5 = 75   ns */
			| AT91_DDRC2_TRP_(2)	/* 2 * 7.5 = 15   ns */
			| AT91_DDRC2_TRRD_(2)	/* 2 * 7.5 = 15   ns */
			| AT91_DDRC2_TWTR_(2)	/* 2 clock cycles min */
			| AT91_DDRC2_TMRD_(2),	/* 2 clock cycles */

		.t1pr = AT91_DDRC2_TXP_(2)		/* 2 clock cycles */
			| AT91_DDRC2_TXSRD_(200)	/* 200 clock cycles */
			| AT91_DDRC2_TXSNR_(19)	/* 19 * 7.5 = 142.5 ns */
			| AT91_DDRC2_TRFC_(17),	/* 17 * 7.5 = 127.5 ns */

		.t2pr = AT91_DDRC2_TFAW_(6)	/* 6 * 7.5 = 45 ns */
			| AT91_DDRC2_TRTP_(2)		/* 2 clock cycles min */
			| AT91_DDRC2_TRPA_(2)		/* 2 * 7.5 = 15 ns */
			| AT91_DDRC2_TXARDS_(8)	/* = TXARD */
			| AT91_DDRC2_TXARD_(8),	/* MR12 = 1 */
	};
	u32 reg;

	/* enable ddr2 clock */
	sama5d3_pmc_enable_periph_clock(SAMA5D3_ID_MPDDRC);
	at91_pmc_enable_system_clock(IOMEM(SAMA5D3_BASE_PMC), AT91CAP9_PMC_DDR);


	/* Init the special register for sama5d3x */
	/* MPDDRC DLL Slave Offset Register: DDR2 configuration */
	reg = AT91_MPDDRC_S0OFF_1
		| AT91_MPDDRC_S2OFF_1
		| AT91_MPDDRC_S3OFF_1;
	writel(reg, SAMA5D3_BASE_MPDDRC + AT91_MPDDRC_DLL_SOR);

	/* MPDDRC DLL Master Offset Register */
	/* write master + clk90 offset */
	reg = AT91_MPDDRC_MOFF_7
		| AT91_MPDDRC_CLK90OFF_31
		| AT91_MPDDRC_SELOFF_ENABLED | AT91_MPDDRC_KEY;
	writel(reg, SAMA5D3_BASE_MPDDRC + AT91_MPDDRC_DLL_MOR);

	/* MPDDRC I/O Calibration Register */
	/* DDR2 RZQ = 50 Ohm */
	/* TZQIO = 4 */
	reg = AT91_MPDDRC_RDIV_DDR2_RZQ_50
		| AT91_MPDDRC_TZQIO_4;
	writel(reg, SAMA5D3_BASE_MPDDRC + AT91_MPDDRC_IO_CALIBR);

	/* DDRAM2 Controller initialize */
	at91_ddram_initialize(IOMEM(SAMA5D3_BASE_MPDDRC), IOMEM(SAMA5_DDRCS),
			      &conf);
}
