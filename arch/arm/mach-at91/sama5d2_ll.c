// SPDX-License-Identifier: BSD-1-Clause
/*
 * Copyright (c) 2017, Microchip Corporation
 *
 * Microchip's name may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 */

#include <mach/sama5d2_ll.h>
#include <mach/at91_ddrsdrc.h>
#include <mach/ddramc.h>
#include <mach/early_udelay.h>
#include <mach/tz_matrix.h>
#include <mach/matrix.h>
#include <mach/at91_rstc.h>
#include <asm/barebox-arm.h>

#define sama5d2_pmc_write(off, val) writel(val, SAMA5D2_BASE_PMC + off)
#define sama5d2_pmc_read(off) readl(SAMA5D2_BASE_PMC + off)

void sama5d2_ddr2_init(struct at91_ddramc_register *ddramc_reg_config)
{
	unsigned int reg;

	/* enable ddr2 clock */
	sama5d2_pmc_enable_periph_clock(SAMA5D2_ID_MPDDRC);
	sama5d2_pmc_write(AT91_PMC_SCER, AT91CAP9_PMC_DDR);

	reg = AT91_MPDDRC_RD_DATA_PATH_ONE_CYCLES;
	writel(reg, SAMA5D2_BASE_MPDDRC + AT91_MPDDRC_RD_DATA_PATH);

	reg = readl(SAMA5D2_BASE_MPDDRC + AT91_MPDDRC_IO_CALIBR);
	reg &= ~AT91_MPDDRC_RDIV;
	reg &= ~AT91_MPDDRC_TZQIO;
	reg |= AT91_MPDDRC_RDIV_DDR2_RZQ_50;
	reg |= AT91_MPDDRC_TZQIO_(101);
	writel(reg, SAMA5D2_BASE_MPDDRC + AT91_MPDDRC_IO_CALIBR);

	/* DDRAM2 Controller initialize */
	at91_ddram_initialize(SAMA5D2_BASE_MPDDRC, IOMEM(SAMA5_DDRCS),
			      ddramc_reg_config);
}

static void sama5d2_pmc_init(void)
{
	at91_pmc_init(SAMA5D2_BASE_PMC, AT91_PMC_LL_SAMA5D2);

	/* Configure PLLA = MOSC * (PLL_MULA + 1) / PLL_DIVA */
	sama5d2_pmc_write(AT91_CKGR_PLLAR, AT91_PMC_PLLA_WR_ERRATA);
	sama5d2_pmc_write(AT91_CKGR_PLLAR, AT91_PMC_PLLA_WR_ERRATA
		       | AT91_PMC3_MUL_(40) | AT91_PMC_OUT_0
		       | AT91_PMC_PLLCOUNT
		       | AT91_PMC_DIV_BYPASS);

	while (!(sama5d2_pmc_read(AT91_PMC_SR) & AT91_PMC_LOCKA))
		;

	/* Initialize PLLA charge pump */
	/* No need: we keep what is set in ROM code */
	//sama5d2_pmc_write(AT91_PMC_PLLICPR, AT91_PMC_IPLLA_3);

	/* Switch PCK/MCK on PLLA output */
	at91_pmc_cfg_mck(SAMA5D2_BASE_PMC,
			AT91_PMC_H32MXDIV
			| AT91_PMC_PLLADIV2_ON
			| AT91SAM9_PMC_MDIV_3
			| AT91_PMC_CSS_PLLA,
			AT91_PMC_LL_SAMA5D2);
}

static void matrix_configure_slave(void)
{
	u32 ddr_port;
	u32 ssr_setting, sasplit_setting, srtop_setting;

	/*
	 * Matrix 0 (H64MX)
	 */

	/*
	 * 0: Bridge from H64MX to AXIMX
	 * (Internal ROM, Crypto Library, PKCC RAM): Always Secured
	 */

	/* 1: H64MX Peripheral Bridge */

	/* 2 ~ 9 DDR2 Port0 ~ 7: Non-Secure */
	srtop_setting = MATRIX_SRTOP(0, MATRIX_SRTOP_VALUE_128M);
	sasplit_setting = MATRIX_SASPLIT(0, MATRIX_SASPLIT_VALUE_128M);
	ssr_setting = MATRIX_LANSECH_NS(0) |
		      MATRIX_RDNSECH_NS(0) |
		      MATRIX_WRNSECH_NS(0);
	for (ddr_port = 0; ddr_port < 8; ddr_port++) {
		at91_matrix_configure_slave_security(SAMA5D2_BASE_MATRIX64,
					SAMA5D2_H64MX_SLAVE_DDR2_PORT_0 + ddr_port,
					srtop_setting,
					sasplit_setting,
					ssr_setting);
	}

	/*
	 * 10: Internal SRAM 128K
	 * TOP0 is set to 128K
	 * SPLIT0 is set to 64K
	 * LANSECH0 is set to 0, the low area of region 0 is the Securable one
	 * RDNSECH0 is set to 0, region 0 Securable area is secured for reads.
	 * WRNSECH0 is set to 0, region 0 Securable area is secured for writes
	 */
	srtop_setting = MATRIX_SRTOP(0, MATRIX_SRTOP_VALUE_128K);
	sasplit_setting = MATRIX_SASPLIT(0, MATRIX_SASPLIT_VALUE_64K);
	ssr_setting = MATRIX_LANSECH_S(0) |
		      MATRIX_RDNSECH_S(0) |
		      MATRIX_WRNSECH_S(0);
	at91_matrix_configure_slave_security(SAMA5D2_BASE_MATRIX64,
					SAMA5D2_H64MX_SLAVE_INTERNAL_SRAM,
					srtop_setting,
					sasplit_setting,
					ssr_setting);

	/* 11:  Internal SRAM 128K (Cache L2) */
	/* 12:  QSPI0 */
	/* 13:  QSPI1 */
	/* 14:  AESB */

	/*
	 * Matrix 1 (H32MX)
	 */

	/* 0: Bridge from H32MX to H64MX: Not Secured */

	/* 1: H32MX Peripheral Bridge 0: Not Secured */

	/* 2: H32MX Peripheral Bridge 1: Not Secured */

	/*
	 * 3: External Bus Interface
	 * EBI CS0 Memory(256M) ----> Slave Region 0, 1
	 * EBI CS1 Memory(256M) ----> Slave Region 2, 3
	 * EBI CS2 Memory(256M) ----> Slave Region 4, 5
	 * EBI CS3 Memory(128M) ----> Slave Region 6
	 * NFC Command Registers(128M) -->Slave Region 7
	 *
	 * NANDFlash(EBI CS3) --> Slave Region 6: Non-Secure
	 */
	srtop_setting =	MATRIX_SRTOP(6, MATRIX_SRTOP_VALUE_128M) |
			MATRIX_SRTOP(7, MATRIX_SRTOP_VALUE_128M);
	sasplit_setting = MATRIX_SASPLIT(6, MATRIX_SASPLIT_VALUE_128M) |
			  MATRIX_SASPLIT(7, MATRIX_SASPLIT_VALUE_128M);
	ssr_setting = MATRIX_LANSECH_NS(6) |
		      MATRIX_RDNSECH_NS(6) |
		      MATRIX_WRNSECH_NS(6) |
		      MATRIX_LANSECH_NS(7) |
		      MATRIX_RDNSECH_NS(7) |
		      MATRIX_WRNSECH_NS(7);
	at91_matrix_configure_slave_security(SAMA5D2_BASE_MATRIX32,
					SAMA5D2_H32MX_EXTERNAL_EBI,
					srtop_setting,
					sasplit_setting,
					ssr_setting);

	/* 4: NFC SRAM (4K): Non-Secure */
	srtop_setting = MATRIX_SRTOP(0, MATRIX_SRTOP_VALUE_8K);
	sasplit_setting = MATRIX_SASPLIT(0, MATRIX_SASPLIT_VALUE_8K);
	ssr_setting = MATRIX_LANSECH_NS(0) |
		      MATRIX_RDNSECH_NS(0) |
		      MATRIX_WRNSECH_NS(0);
	at91_matrix_configure_slave_security(SAMA5D2_BASE_MATRIX32,
					SAMA5D2_H32MX_NFC_SRAM,
					srtop_setting,
					sasplit_setting,
					ssr_setting);

	/* 5:
	 * USB Device High Speed Dual Port RAM (DPR): 1M
	 * USB Host OHCI registers: 1M
	 * USB Host EHCI registers: 1M
	 */
	srtop_setting = MATRIX_SRTOP(0, MATRIX_SRTOP_VALUE_1M) |
			MATRIX_SRTOP(1, MATRIX_SRTOP_VALUE_1M) |
			MATRIX_SRTOP(2, MATRIX_SRTOP_VALUE_1M);
	sasplit_setting = MATRIX_SASPLIT(0, MATRIX_SASPLIT_VALUE_1M) |
			  MATRIX_SASPLIT(1, MATRIX_SASPLIT_VALUE_1M) |
			  MATRIX_SASPLIT(2, MATRIX_SASPLIT_VALUE_1M);
	ssr_setting = MATRIX_LANSECH_NS(0) |
		      MATRIX_LANSECH_NS(1) |
		      MATRIX_LANSECH_NS(2) |
		      MATRIX_RDNSECH_NS(0) |
		      MATRIX_RDNSECH_NS(1) |
		      MATRIX_RDNSECH_NS(2) |
		      MATRIX_WRNSECH_NS(0) |
		      MATRIX_WRNSECH_NS(1) |
		      MATRIX_WRNSECH_NS(2);
	at91_matrix_configure_slave_security(SAMA5D2_BASE_MATRIX32,
					SAMA5D2_H32MX_USB,
					srtop_setting,
					sasplit_setting,
					ssr_setting);
}

static void sama5d2_matrix_init(void)
{
	at91_matrix_write_protect_disable(SAMA5D2_BASE_MATRIX64);
	at91_matrix_write_protect_disable(SAMA5D2_BASE_MATRIX32);

	matrix_configure_slave();
}

static void sama5d2_rstc_init(void)
{
	writel(AT91_RSTC_KEY | AT91_RSTC_URSTEN,
	       SAMA5D2_BASE_RSTC + AT91_RSTC_MR);
}

void sama5d2_lowlevel_init(void)
{
	arm_cpu_lowlevel_init();
	sama5d2_pmc_init();
	sama5d2_matrix_init();
	sama5d2_rstc_init();
}
