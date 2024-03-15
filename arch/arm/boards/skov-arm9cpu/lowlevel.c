// SPDX-License-Identifier: GPL-2.0
// SPDX-FileCopyrightText: 2022 Sam Ravnborg <sam@ravnborg.org>

#include <mach/at91/at91sam926x_board_init.h>
#include <mach/at91/at91sam9263_matrix.h>
#include <mach/at91/sam92_ll.h>
#include <mach/at91/xload.h>
#include <mach/at91/barebox-arm.h>
#include <linux/build_bug.h>

/* MCK = 20 MHz */
#define MAIN_CLOCK	200000000
#define MASTER_CLOCK	(MAIN_CLOCK / 2)	/* PMC_MCKR divides by 2 */

#define PLLA_SETTINGS	(AT91_PMC_PLLA_WR_ERRATA | AT91_PMC_MUL_(49) | AT91_PMC_OUT_2 | \
			 AT91_PMC_PLLCOUNT_(48) | AT91_PMC_DIV_(4))
static_assert(PLLA_SETTINGS == 0x2031B004);

#define PLLB_SETTINGS	(AT91_PMC_USBDIV_2 | AT91_PMC_MUL_(5) | AT91_PMC_OUT_0 | \
			 AT91_PMC_PLLCOUNT_(48) | AT91_PMC_DIV_BYPASS)
static_assert(PLLB_SETTINGS == 0x10053001);

/*
 * Check if target is 64 or 128 MB and adjust AT91_SDRAMC_CR
 * accordingly.
 * Size      Start      Size(hex)
 * 64 MB  => 0x20000000 0x4000000
 * 128 MB => 0x20000000 0x8000000
 *
 * If 64 MiB RAM with NC_10 set, then we see holes in the memory, which
 * is how we detect if memory is 64 or 128 MiB
 */
static int check_if_128mb(void)
{
	unsigned int *test_adr = (unsigned int *)AT91_CHIPSELECT_1;
	unsigned int test_val = 0xdeadbee0;
	unsigned int *p;
	int i;

	/* Fill up memory with a known pattern */
	p = test_adr;
	for (i = 0; i < 0xb00; i++)
		*p++ = test_val + i;

	/*
	 * Check that we can read back the values just written
	 * If one or more fails, we have only 64 MB
	 */
	p = test_adr;
	for (i = 0; i < 0xb00; i++)
		if (*p++ != (test_val + i))
			return false;

	return true;
}

static void sam9263_sdramc_init(void)
{
	void __iomem *piod = IOMEM(AT91SAM9263_BASE_PIOD);
	static struct at91sam9_sdramc_config config = {
		.sdramc = IOMEM(AT91SAM9263_BASE_SDRAMC0),
		.mr = 0,
		.tr = (MASTER_CLOCK * 7) / 1000000, // TODO 140 versus 0x13c (316)?
		.cr = AT91_SDRAMC_NC_10 | AT91_SDRAMC_NR_13 | AT91_SDRAMC_CAS_2
		      | AT91_SDRAMC_NB_4 | AT91_SDRAMC_DBW_32
		      | AT91_SDRAMC_TWR_2 | AT91_SDRAMC_TRC_7
		      | AT91_SDRAMC_TRP_2 | AT91_SDRAMC_TRCD_2
		      | AT91_SDRAMC_TRAS_5 | AT91_SDRAMC_TXSR_8,
		.lpr = 0,
		.mdr = AT91_SDRAMC_MD_SDRAM,
	};

	/* Define PD[31:16] as DATA[31:16] */
	at91_mux_gpio_disable(piod, GENMASK(31, 16));
	/* No pull-up for D[31:16] */
	at91_mux_set_pullup(piod, GENMASK(31, 16), false);
	/* PD16 to PD31 are pheripheral A */
	at91_mux_set_A_periph(piod, GENMASK(31, 16));

	/* EBI0_CSA, CS1 SDRAM, 3.3V memories */
	setbits_le32(IOMEM(AT91SAM9263_BASE_MATRIX + AT91SAM9263_MATRIX_EBI0CSA),
	       AT91SAM9263_MATRIX_EBI0_VDDIOMSEL_3_3V | AT91SAM9263_MATRIX_EBI0_CS1A_SDRAMC);

	at91sam9_sdramc_initialize(&config, AT91SAM9263_BASE_EBI0_CS1);

	if (!check_if_128mb()) {
		/* Change number of columns to 9 for 64MB ram. */
		/* Other parameters does not need to be changed due to chip size. */

		pr_debug("64M variant detected\n");

		/* Clear NC bits */
		config.cr &= ~AT91_SDRAMC_NC;
		config.cr |= AT91_SDRAMC_NC_9;
		at91sam9_sdramc_initialize(&config, AT91SAM9263_BASE_EBI0_CS1);
	}
}

static noinline void continue_skov_arm9cpu_xload_mmc(void)
{
	sam9263_lowlevel_init(PLLA_SETTINGS, PLLB_SETTINGS);
	sam92_dbgu_setup_ll(MASTER_CLOCK);

	sam92_udelay_init(MASTER_CLOCK);
	sam9263_sdramc_init();
	sam9263_atmci_start_image(1, MASTER_CLOCK, 0);
}

SAM9_ENTRY_FUNCTION(start_skov_arm9cpu_xload_mmc)
{
	/* Configure system so we are less constrained */
	arm_cpu_lowlevel_init();
	relocate_to_current_adr();
	setup_c();

	continue_skov_arm9cpu_xload_mmc();
}

extern char __dtb_at91_skov_arm9cpu_start[];

AT91_ENTRY_FUNCTION(start_skov_arm9cpu, r0, r1, r2)
{
	void *fdt;

	/*
	 * We may be running after at91bootstrap, so redo the initialization to
	 * be sure, everything is as we expect it.
	 */
	arm_cpu_lowlevel_init();

	fdt = __dtb_at91_skov_arm9cpu_start + get_runtime_offset();
	barebox_arm_entry(AT91_CHIPSELECT_1, at91sam9263_get_sdram_size(0), fdt);
}
