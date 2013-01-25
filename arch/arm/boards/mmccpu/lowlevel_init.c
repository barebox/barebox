/*
 * Copyright (C) 2009-2011 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * Under GPLv2
 */

#include <common.h>
#include <init.h>
#include <mach/hardware.h>
#include <mach/at91_rstc.h>
#include <mach/at91_wdt.h>
#include <mach/at91_pmc.h>
#include <mach/at91sam9_smc.h>
#include <mach/at91sam9_sdramc.h>
#include <mach/at91sam9_matrix.h>
#include <mach/at91_lowlevel_init.h>

#define MASTER_PLL_MUL		54
#define MASTER_PLL_DIV		4

void __bare_init at91sam926x_lowlevel_board_config(struct at91sam926x_lowlevel_cfg *cfg)
{
	/* Disable Watchdog */
	cfg->wdt_mr =
		AT91_WDT_WDIDLEHLT | AT91_WDT_WDDBGHLT |
		AT91_WDT_WDV |
		AT91_WDT_WDDIS |
		AT91_WDT_WDD;

	/* define PDC[31:16] as DATA[31:16] */
	cfg->ebi_pio_pdr = 0xFFFF0000;
	/* no pull-up for D[31:16] */
	cfg->ebi_pio_ppudr = 0xFFFF0000;
	/* EBI0_CSA, CS1 SDRAM, CS3 NAND Flash, 3.3V memories */
	cfg->ebi_csa =
		AT91_MATRIX_EBI0_DBPUC | AT91_MATRIX_EBI0_VDDIOMSEL_1_8V |
		AT91_MATRIX_EBI0_CS1A_SDRAMC |
		AT91_MATRIX_EBI0_CS3A_SMC_SMARTMEDIA;

	cfg->smc_cs = 0;
#if 1
	cfg->smc_mode =
		AT91_SMC_READMODE | AT91_SMC_WRITEMODE |
		AT91_SMC_DBW_16 |
		AT91_SMC_TDFMODE |
		AT91_SMC_TDF_(6);
	cfg->smc_cycle =
		AT91_SMC_NWECYCLE_(16) | AT91_SMC_NRDCYCLE_(16);
	cfg->smc_pulse =
		AT91_SMC_NWEPULSE_(5) | AT91_SMC_NCS_WRPULSE_(7) |
		AT91_SMC_NRDPULSE_(5) | AT91_SMC_NCS_RDPULSE_(13);
	cfg->smc_setup =
		AT91_SMC_NWESETUP_(3) | AT91_SMC_NCS_WRSETUP_(2) |
		AT91_SMC_NRDSETUP_(8) | AT91_SMC_NCS_RDSETUP_(0);
#elif 0	/* slow setup */
	cfg->smc_mode =
		AT91_SMC_READMODE | AT91_SMC_WRITEMODE |
		AT91_SMC_DBW_16 |
		AT91_SMC_TDFMODE |
		AT91_SMC_TDF_(1);
	cfg->smc_cycle =
		AT91_SMC_NWECYCLE_(0xd00) | AT91_SMC_NRDCYCLE_(0xd00);
	cfg->smc_pulse =
		AT91_SMC_NWEPULSE_(5) | AT91_SMC_NCS_WRPULSE_(7) |
		AT91_SMC_NRDPULSE_(5) | AT91_SMC_NCS_RDPULSE_(13);
	cfg->smc_setup =
		AT91_SMC_NWESETUP_(3) | AT91_SMC_NCS_WRSETUP_(2) |
		AT91_SMC_NRDSETUP_(8) | AT91_SMC_NCS_RDSETUP_(0);
#else	/* RONETIX' original values */
	cfg->smc_mode =
		AT91_SMC_READMODE | AT91_SMC_WRITEMODE |
		AT91_SMC_DBW_16 |
		AT91_SMC_TDFMODE |
		AT91_SMC_TDF_(6);
	cfg->smc_cycle =
		AT91_SMC_NWECYCLE_(22) | AT91_SMC_NRDCYCLE_(22);
	cfg->smc_pulse =
		AT91_SMC_NWEPULSE_(11) | AT91_SMC_NCS_WRPULSE_(11) |
		AT91_SMC_NRDPULSE_(11) | AT91_SMC_NCS_RDPULSE_(11);
	cfg->smc_setup =
		AT91_SMC_NWESETUP_(10) | AT91_SMC_NCS_WRSETUP_(10) |
		AT91_SMC_NRDSETUP_(10) | AT91_SMC_NCS_RDSETUP_(10);
#endif

	cfg->pmc_mor =
		AT91_PMC_MOSCEN |
		(255 << 8);		/* Main Oscillator Start-up Time */
	cfg->pmc_pllar =
		AT91_PMC_PLLA_WR_ERRATA | /* Bit 29 must be 1 when prog */
		AT91_PMC_OUT |
		AT91_PMC_PLLCOUNT |	/* PLL Counter */
		(2 << 28) |		/* PLL Clock Frequency Range */
		((MASTER_PLL_MUL - 1) << 16) | (MASTER_PLL_DIV);
	/* PCK/2 = MCK Master Clock from PLLA */
	cfg->pmc_mckr1 =
		AT91_PMC_CSS_SLOW |
		AT91_PMC_PRES_1 |
		AT91SAM9_PMC_MDIV_2 |
		AT91_PMC_PDIV_1;
	/* PCK/2 = MCK Master Clock from PLLA */
	cfg->pmc_mckr2 =
		AT91_PMC_CSS_PLLA |
		AT91_PMC_PRES_1 |
		AT91SAM9_PMC_MDIV_2 |
		AT91_PMC_PDIV_1;

	/* SDRAM */
	/* SDRAMC_TR - Refresh Timer register */
	cfg->sdrc_tr1 = 0x13C;
	/* SDRAMC_CR - Configuration register*/
	cfg->sdrc_cr =
		AT91_SDRAMC_NC_9 |
		AT91_SDRAMC_NR_13 |
		AT91_SDRAMC_NB_4 |
		AT91_SDRAMC_CAS_3 |
		AT91_SDRAMC_DBW_32 |
		(2 <<  8) |	/* tWR -  Write Recovery Delay */
		(8 << 12) |	/* tRC -  Row Cycle Delay */
		(2 << 16) |	/* tRP -  Row Precharge Delay */
		(2 << 20) |	/* tRCD - Row to Column Delay */
		(5 << 24) |	/* tRAS - Active to Precharge Delay */
		(12 << 28);	/* tXSR - Exit Self Refresh to Active Delay */

	/* Memory Device Register -> SDRAM */
	cfg->sdrc_mdr = AT91_SDRAMC_MD_SDRAM;
	/* SDRAM_TR */
	cfg->sdrc_tr2 = 780;

	/* user reset enable */
	cfg->rstc_rmr =
		AT91_RSTC_KEY |
		AT91_RSTC_PROCRST |
		AT91_RSTC_RSTTYP_WAKEUP |
		AT91_RSTC_RSTTYP_WATCHDOG;
}
