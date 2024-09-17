/* SPDX-License-Identifier: GPL-2.0-or-later */
/* SPDX-FileCopyrightText: 2014 Lucas Stach, Pengutronix */

/**
 * enable_arm_errata_709718_war() - Workaround ARM erratum 709718
 *
 * Enables workaround for "Load and Store operations on the shared device
 * memory regions may not complete in program order".
 *
 * This is described as part of the i.MX51 errata at
 * https://www.nxp.com/docs/en/errata/IMX51CE.pdf
 *
 * Affects Cortex-A8 as found in i.MX51
 */
static inline void enable_arm_errata_709718_war(void)
{
	__asm__ __volatile__ (
		"mrc	p15, 0, r0, c10, c2, 0\n"
		"bic	r0, #3 << 16\n"
		"mcr	p15, 0, r0, c10, c2, 0\n"
		"mrc	p15, 0, r0, c1, c0, 0\n"
		"orr	r0, r0, #1 << 28\n"
		"mcr	p15, 0, r0, c1, c0, 0\n"
	);
}

/**
 * enable_arm_errata_716044_war() - Workaround ARM erratum 716044
 *
 * Enables workaround for "Under very rare circumstances, an uncacheable
 * load multiple instruction might cause a deadlock"
 *
 * Affects Cortex-A9 revisions before r2p0
 */
static inline void enable_arm_errata_716044_war(void)
{
	__asm__ __volatile__ (
		"mrc	p15, 0, r0, c1, c0, 0\n"
		"orr	r0, r0, #1 << 11\n"
		"mcr	p15, 0, r0, c1, c0, 0\n"
	);
}

/**
 * enable_arm_errata_742230_war() - Workaround ARM erratum 742230
 *
 * Enables workaround for "DMB operation may be faulty"
 *
 * Affects Cortex-A9 revisions before r2p3
 */
static inline void enable_arm_errata_742230_war(void)
{
	__asm__ __volatile__ (
		"mrc	p15, 0, r0, c15, c0, 1\n"
		"orr	r0, r0, #1 << 4\n"
		"mcr	p15, 0, r0, c15, c0, 1\n"
	);
}

/**
 * enable_arm_errata_743622_war() - Workaround for ARM erratum 743622
 *
 * Enables workaround for "Faulty hazard checking in the Store Buffer may
 * lead to data corruption"
 *
 * Affects Cortex-A9 revisions before r3p0
 */
static inline void enable_arm_errata_743622_war(void)
{
	__asm__ __volatile__ (
		"mrc	p15, 0, r0, c15, c0, 1\n"
		"orr	r0, r0, #1 << 6\n"
		"mcr	p15, 0, r0, c15, c0, 1\n"
	);
}

/** enable_arm_errata_751472_war() - Workaround for ARM erratum 751472
 *
 * Workaround for "Interrupted ICIALLUIS may prevent completion of
 * broadcasted operation"
 *
 * Affects Cortex-A9 revisions before r3p0
 *
 * NOTE: Must be first set in secure mode
 */
static inline void enable_arm_errata_751472_war(void)
{
	__asm__ __volatile__ (
		"mrc	p15, 0, r0, c15, c0, 1\n"
		"orr	r0, r0, #1 << 11\n"
		"mcr	p15, 0, r0, c15, c0, 1\n"
	);
}

/** enable_arm_errata_761320_war() - Workaround for ARM erratum 761320
 *
 * Enables workaround for "Full cache line writes to the same memory region
 * from at least two processors might deadlock processor"
 *
 * Affects Cortex-A9 revisions before r4p0
 *
 * NOTE: Must be first set in secure mode
 */
static inline void enable_arm_errata_761320_war(void)
{
	__asm__ __volatile__ (
		"mrc	p15, 0, r0, c15, c0, 1\n"
		"orr	r0, r0, #1 << 21\n"
		"mcr	p15, 0, r0, c15, c0, 1\n"
	);
}

/**
 * enable_arm_errata_794072_war() - Workaround for ARM erratum 794072
 *
 * Enables workaround for "A short loop including a DMB instruction might
 * cause a denial of service"
 *
 * Affects Cortex-A9 all revisions
 *
 * NOTE: Must be first set in secure mode
 */
static inline void enable_arm_errata_794072_war(void)
{
	__asm__ __volatile__ (
		"mrc	p15, 0, r0, c15, c0, 1\n"
		"orr	r0, r0, #1 << 4\n"
		"mcr	p15, 0, r0, c15, c0, 1\n"
	);
}

/**
 * enable_arm_errata_845369_war() - Workaround for ARM erratum 845369
 *
 * Enables workaround for "Under very rare timing circumstances, transitioning
 * into streaming mode might create a data corruption"
 *
 * Affects Cortex-A9 all revisions
 *
 * NOTE: Must be first set in secure mode
 */
static inline void enable_arm_errata_845369_war(void)
{
	__asm__ __volatile__ (
		"mrc	p15, 0, r0, c15, c0, 1\n"
		"orr	r0, r0, #1 << 22\n"
		"mcr	p15, 0, r0, c15, c0, 1\n"
	);
}

/**
 * enable_arm_errata_cortexa8_enable_ibe() - Enable Invalidate BTB Enable bit
 * workaround
 *
 * Enables use of BPIALL for hardening the branch predictor as workaround
 * for CVE 2017-5715 "Spectre v2".
 *
 * Affects Cortex-A8 all revisions.
 *
 * NOTE: Must be first set in secure mode
 */
static inline void enable_arm_errata_cortexa8_enable_ibe(void)
{
	__asm__ __volatile__ (
		"mrc	p15, 0, r0, c1, c0, 1\n"
		"orr	r0, r0, #1 << 6\n"
		"mcr	p15, 0, r0, c1, c0, 1\n"
	);
}
