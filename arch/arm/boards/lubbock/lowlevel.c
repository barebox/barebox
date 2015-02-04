#include <common.h>
#include <init.h>
#include <io.h>

#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <linux/sizes.h>
#include <mach/pxa-regs.h>
#include <mach/regs-ost.h>

/*
 * Memory settings
 */
#define DEFAULT_MSC0_VAL	0x23d223d2
#define DEFAULT_MSC1_VAL	0x3ff1a441
#define DEFAULT_MSC2_VAL	0x7ff17ff1
#define DEFAULT_MDCNFG_VAL	0x00001ac9
#define DEFAULT_MDREFR_VAL	0x00018018
#define DEFAULT_MDMRS_VAL	0x00000000

#define DEFAULT_FLYCNFG_VAL	0x00000000
#define DEFAULT_SXCNFG_VAL	0x00000000

/*
 * PCMCIA and CF Interfaces
 */
#define DEFAULT_MECR_VAL	0x00000000
#define DEFAULT_MCMEM0_VAL	0x00010504
#define DEFAULT_MCMEM1_VAL	0x00010504
#define DEFAULT_MCATT0_VAL	0x00010504
#define DEFAULT_MCATT1_VAL	0x00010504
#define DEFAULT_MCIO0_VAL	0x00004715
#define DEFAULT_MCIO1_VAL	0x00004715

static inline void writelrb(uint32_t val, volatile u32 __iomem *addr)
{
	writel(val, addr);
	barrier();
	readl(addr);
	barrier();
}

static inline void pxa_wait_ticks(int ticks)
{
	writel(0, &OSCR);
	while (readl(&OSCR) < ticks)
		barrier();
}

static inline void pxa2xx_dram_init(void)
{
	uint32_t tmp;
	int i;
	/*
	 * 1) Initialize Asynchronous static memory controller
	 */

	writelrb(DEFAULT_MSC0_VAL, &MSC0);
	writelrb(DEFAULT_MSC1_VAL, &MSC1);
	writelrb(DEFAULT_MSC2_VAL, &MSC2);
	/*
	 * 2) Initialize Card Interface
	 */

	/* MECR: Memory Expansion Card Register */
	writelrb(DEFAULT_MECR_VAL, &MECR);
	/* MCMEM0: Card Interface slot 0 timing */
	writelrb(DEFAULT_MCMEM0_VAL, &MCMEM0);
	/* MCMEM1: Card Interface slot 1 timing */
	writelrb(DEFAULT_MCMEM1_VAL, &MCMEM1);
	/* MCATT0: Card Interface Attribute Space Timing, slot 0 */
	writelrb(DEFAULT_MCATT0_VAL, &MCATT0);
	/* MCATT1: Card Interface Attribute Space Timing, slot 1 */
	writelrb(DEFAULT_MCATT1_VAL, &MCATT1);
	/* MCIO0: Card Interface I/O Space Timing, slot 0 */
	writelrb(DEFAULT_MCIO0_VAL, &MCIO0);
	/* MCIO1: Card Interface I/O Space Timing, slot 1 */
	writelrb(DEFAULT_MCIO1_VAL, &MCIO1);

	/*
	 * 3) Configure Fly-By DMA register
	 */

	writelrb(DEFAULT_FLYCNFG_VAL, &FLYCNFG);

	/*
	 * 4) Initialize Timing for Sync Memory (SDCLK0)
	 */

	/*
	 * Before accessing MDREFR we need a valid DRI field, so we set
	 * this to power on defaults + DRI field.
	 */

	/* Read current MDREFR config and zero out DRI */
	tmp = readl(&MDREFR) & ~0xfff;
	/* Add user-specified DRI */
	tmp |= DEFAULT_MDREFR_VAL & 0xfff;
	/* Configure important bits */
	tmp |= MDREFR_K0RUN | MDREFR_SLFRSH;
	tmp &= ~(MDREFR_APD | MDREFR_E1PIN);

	/* Write MDREFR back */
	writelrb(tmp, &MDREFR);

	/*
	 * 5) Initialize Synchronous Static Memory (Flash/Peripherals)
	 */

	/* Initialize SXCNFG register. Assert the enable bits.
	 *
	 * Write SXMRS to cause an MRS command to all enabled banks of
	 * synchronous static memory. Note that SXLCR need not be written
	 * at this time.
	 */
	writelrb(DEFAULT_SXCNFG_VAL, &SXCNFG);

	/*
	 * 6) Initialize SDRAM
	 */

	writelrb(DEFAULT_MDREFR_VAL & ~MDREFR_SLFRSH, &MDREFR);
	writelrb(DEFAULT_MDREFR_VAL | MDREFR_E1PIN, &MDREFR);

	/*
	 * 7) Write MDCNFG with MDCNFG:DEx deasserted (set to 0), to configure
	 *    but not enable each SDRAM partition pair.
	 */

	writelrb(DEFAULT_MDCNFG_VAL &
		 ~(MDCNFG_DE0 | MDCNFG_DE1 | MDCNFG_DE2 | MDCNFG_DE3), &MDCNFG);
	/* Wait for the clock to the SDRAMs to stabilize, 100..200 usec. */
	pxa_wait_ticks(0x300);

	/*
	 * 8) Trigger a number (usually 8) refresh cycles by attempting
	 *    non-burst read or write accesses to disabled SDRAM, as commonly
	 *    specified in the power up sequence documented in SDRAM data
	 *    sheets. The address(es) used for this purpose must not be
	 *    cacheable.
	 */
	for (i = 9; i >= 0; i--) {
		writel(i, 0xa0000000);
		barrier();
	}
	/*
	 * 9) Write MDCNFG with enable bits asserted (MDCNFG:DEx set to 1).
	 */

	tmp = DEFAULT_MDCNFG_VAL &
		(MDCNFG_DE0 | MDCNFG_DE1 | MDCNFG_DE2 | MDCNFG_DE3);
	tmp |= readl(&MDCNFG);
	writelrb(tmp, &MDCNFG);

	/*
	 * 10) Write MDMRS.
	 */

	writelrb(DEFAULT_MDMRS_VAL, &MDMRS);

	/*
	 * 11) Enable APD
	 */

	if (DEFAULT_MDREFR_VAL & MDREFR_APD) {
		tmp = readl(&MDREFR);
		tmp |= MDREFR_APD;
		writelrb(tmp, &MDREFR);
	}
}

void __bare_init __naked barebox_arm_reset_vector(void)
{
	unsigned long pssr = PSSR;
	unsigned long pc = get_pc();

	arm_cpu_lowlevel_init();
	CKEN |= CKEN_OSTIMER | CKEN_MEMC | CKEN_FFUART;

	/*
	 * When not running from SDRAM, get it out of self refresh, and/or
	 * initialize it.
	 */
	if (!(pc >= 0xa0000000 && pc < 0xb0000000))
		pxa2xx_dram_init();

	if ((pssr >= 0xa0000000 && pssr < 0xb0000000) ||
	    (pssr >= 0x04000000 && pssr < 0x10000000))
		asm("mov pc, %0" : : "r"(pssr) : );

	barebox_arm_entry(0xa0000000, SZ_64M, 0);
}
