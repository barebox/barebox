// SPDX-License-Identifier: GPL-2.0-only

#include <common.h>
#include <debug_ll.h>
#include <linux/sizes.h>
#include <asm/barebox-arm.h>
#include <asm/barebox-arm-head.h>
#include <asm/io.h>
#include <mach/pxa/debug_ll.h>

#define RAUMFELD_SDRAM_BASE	0xa0000000
#define RAUMFELD_SDRAM_SIZE	SZ_128M

/* Application subsystem clock configuration and its status counterpart */
#define ACCR			0x41340000
#define ACSR			0x41340004

#define ACCR_XSPCLK(x)		(((x) & 0x3) << 16)	/* core clock during the change */
#define ACCR_XSPCLK_MASK	ACCR_XSPCLK(~0)
#define ACCR_XN(x)		(((x) & 0x7) << 8)	/* turbo-mode-to-run-mode ratio */
#define ACCR_XN_MASK		ACCR_XN(~0)
#define ACCR_XL(x)		((x) & 0x1f)		/* run-mode-to-oscillator ratio */
#define ACCR_XL_MASK		ACCR_XL(~0)

#define XSPCLK_NONE		3	/* no core clock while the PLL relocks */
#define XCLKCFG_F		(1 << 1)	/* start a frequency change */
#define XCLKCFG_T		(1 << 0)	/* turbo mode */

extern char __dtb_pxa300_raumfeld_speaker_start[];

/*
 * Switch the core to the 624MHz operating point.
 *
 * The vendor OBM writes ACCR asking for XL=24 and XN=2, i.e. 312MHz run and
 * 624MHz turbo, but writing ACCR does not change the core PLL by itself: the
 * ratio only takes effect once software starts a frequency change by writing
 * XCLKCFG. Nothing in the original boot chain ever did - the U-Boot in NAND
 * contains no such write either - so the core has always run at its 104MHz
 * reset default. Everything else in that ACCR value does apply immediately,
 * which is why ACSR reads back the OBM's HSS, SMCFS, SFLFS and DMCFS next to
 * XL=8 and XN=1.
 *
 * This is done blind, without touching the rails. 624MHz wants 1.375V on
 * VCC_CORE and 1.4V on VCC_SRAM (pxa300_freqs[] in Linux'
 * drivers/cpufreq/pxa3xx-cpufreq.c), and both come from a MAX8660 that is
 * write-only over I2C, so what it supplies cannot be read back. It does not
 * have to be: V3 and V4 power up at 1.4V, which covers every operating point
 * the part has. Linux has always relied on the same thing - pxa3xx-cpufreq
 * carries those voltages and applies none of them, and with the performance
 * governor it takes this board to 624MHz at boot - so this is what the
 * hardware has been doing since the factory, not a new assumption.
 *
 * The cost is that the rails stay at 1.4V at every frequency instead of
 * dropping to 1.0V at the low ones. That needs the PMIC driver and the driver
 * model, which is a long way past lowlevel, and it buys power rather than
 * anything this board notices.
 *
 * FFUART is clocked from the system PLL rather than from the core, so the
 * console baud rate is unaffected.
 */
static void raumfeld_speaker_core_624mhz(void)
{
	u32 want = ACCR_XN(2) | ACCR_XL(24);
	int timeout = 100000;
	u32 accr;

	accr = readl(IOMEM(ACCR));
	accr &= ~(ACCR_XN_MASK | ACCR_XL_MASK | ACCR_XSPCLK_MASK);
	accr |= want | ACCR_XSPCLK(XSPCLK_NONE);
	writel(accr, IOMEM(ACCR));

	/*
	 * XN > 1 is the turbo-mode-to-run-mode ratio, so the turbo bit has to
	 * go out with the frequency change or the core stops at the 312MHz
	 * run frequency.
	 */
	asm volatile("mcr p14, 0, %0, c6, c0, 0"
		     : : "r" (XCLKCFG_F | XCLKCFG_T));

	/* ACSR follows ACCR once the PLL has relocked */
	while (--timeout &&
	       (readl(IOMEM(ACSR)) & (ACCR_XN_MASK | ACCR_XL_MASK)) != want)
		;
}

ENTRY_FUNCTION_WITHSTACK(start_raumfeld_speaker, 0, r0, r1, r2)
{
	void *fdt;

	arm_cpu_lowlevel_init();

	/*
	 * We are the second stage: SDRAM, clocks and pinmuxing are already
	 * set up by the OBM. The stack the previous stage left us is used
	 * until barebox_arm_entry() installs its own.
	 */
	pxa_debug_ll_init();

	putc_ll('>');

	raumfeld_speaker_core_624mhz();

	fdt = __dtb_pxa300_raumfeld_speaker_start + get_runtime_offset();

	barebox_arm_entry(RAUMFELD_SDRAM_BASE, RAUMFELD_SDRAM_SIZE, fdt);
}
