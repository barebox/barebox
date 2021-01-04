/* SPDX-License-Identifier: BSD-1-Clause
 *
 * Copyright (C) 2014, Atmel Corporation
 *
 * SAMA5D27 System-in-Package DDRAMC configuration
 */

#include <mach/at91_ddrsdrc.h>
#include <mach/ddramc.h>
#include <mach/sama5d2_ll.h>

static inline void sama5d2_d1g_ddrconf(void) /* DDR2 1Gbit SDRAM */
{
	const struct at91_ddramc_register conf = {
		.mdr = AT91_DDRC2_DBW_16_BITS | AT91_DDRC2_MD_DDR2_SDRAM,

		.cr = AT91_DDRC2_NC_DDR10_SDR9 | AT91_DDRC2_NR_13 |
			AT91_DDRC2_CAS_3 | AT91_DDRC2_DISABLE_RESET_DLL |
			AT91_DDRC2_WEAK_STRENGTH_RZQ7 | AT91_DDRC2_ENABLE_DLL |
			AT91_DDRC2_NB_BANKS_8 | AT91_DDRC2_NDQS_ENABLED |
			AT91_DDRC2_DECOD_INTERLEAVED | AT91_DDRC2_UNAL_SUPPORTED,

		.rtr = 0x511,

		.t0pr = AT91_DDRC2_TRAS_(7) | AT91_DDRC2_TRCD_(3) |
			AT91_DDRC2_TWR_(3) | AT91_DDRC2_TRC_(9) |
			AT91_DDRC2_TRP_(3) | AT91_DDRC2_TRRD_(2) |
			AT91_DDRC2_TWTR_(2) | AT91_DDRC2_TMRD_(2),

		.t1pr = AT91_DDRC2_TRFC_(22) | AT91_DDRC2_TXSNR_(23) |
			AT91_DDRC2_TXSRD_(200) | AT91_DDRC2_TXP_(2),

		.t2pr = AT91_DDRC2_TXARD_(2) | AT91_DDRC2_TXARDS_(8) |
			AT91_DDRC2_TRPA_(4) | AT91_DDRC2_TRTP_(2) |
			AT91_DDRC2_TFAW_(8),
	};

	sama5d2_ddr2_init(&conf);
}
