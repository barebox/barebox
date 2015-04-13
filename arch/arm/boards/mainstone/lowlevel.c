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
#define DEFAULT_MSC0_VAL	0x23F2B8F2
#define DEFAULT_MSC1_VAL	0x7ff0fff1
/*
 * MSC2: static partitions 4 and 5
 *
 * [31]      0   - RBUFF5
 * [30:28]   111 - RRR5
 * [27:24]   1111- RDN5
 * [23:20]   1111- RDF5
 * [19]      0   - RBW5
 * [18:16]   000 - RT5
 * [15]      0   - RBUFF4: Slow device (don't wait for data return)
 * [14:12]   111 - RRR4: Toff=(2*RRR + 1)*CLK_MEM (from nCS=1 to next nCS=0)
 * [11:8]    1111- RDN4: T=2*RDN*CLK_MEM (from nOE=1 to addr hold)
 * [7:4]     1111- RDF4: T=RDF*CLK_MEM of hold nOE/nPWE for read/write
 * [3]       0   - RBW4: Bus width is 32 bits
 * [2:0]     000 - RT4: Partition is VLIO
 */
#define DEFAULT_MSC2_VAL	0x7ff0fff4

/*
 * MDCNFG: SDRAM Configuration Register
 *
 * [31]      0	 - Memory map 0/1 uses normal 256 MBytes
 * [30]      0   - dcacx2: no extra column addressing
 * [29]      0   - reserved
 * [28]      0	 - SA1111 compatiblity mode
 * [27]      0   - latch return data with return clock
 * [26]      0   - alternate addressing for pair 2/3
 * [25:24]   00  - timings
 * [23]      0   - internal banks in lower partition 2/3 (not used)
 * [22:21]   00  - row address bits for partition 2/3 (not used)
 * [20:19]   00  - column address bits for partition 2/3 (not used)
 * [18]      0   - SDRAM partition 2/3 width is 32 bit
 * [17]      0   - SDRAM partition 3 disabled
 * [16]      0   - SDRAM partition 2 disabled
 * [15]      0	 - Stack1 : see stack0
 * [14]      0   - dcacx0 : no extra column addressing
 * [13]      0   - stack0 : stack = 0b00 => SDRAM address placed on MA<24:10>
 * [12]      0	 - SA1110 compatiblity mode
 * [11]      1   - always 1
 * [10]      0   - no alternate addressing for pair 0/1
 * [09:08]   10  - tRP=2*MemClk CL=2 tRCD=2*MemClk tRAS=7*MemClk tRC=11*MemClk
 * [7]       1   - 4 internal banks in partitions 0/1
 * [06:05]   10  - drac0: 13 row address bits for partition 0/1
 * [04:03]   01  - dcac0: 9 column address bits for partition 0/1
 * [02]      0   - SDRAM partition 0/1 width is 32 bit
 * [01]      1   - enable SDRAM partition 1
 * [00]      1   - enable SDRAM partition 0
 *
 * Configuration is for 1 bank of 64MBytes (13 rows * 9 cols)
 * in bank0, of width 32 bits, with 4 internal banks.
 * Timings (in times of SDCLK<1>): tRP = 3clk, CL=3, rRCD=3clk,
 *                                 tRAS=7clk, tRC=11clk
 */
#define DEFAULT_MDCNFG_VAL	0x00000acb

/*
 * MDREFR: SDRAM Configuration Register
 *
 * [25]      0   - K2FREE=0
 * [24]      0   - K1FREE=0
 * [23]      0   - K0FREE=0
 * [22]      0   - SLFRSH=0
 * [21]
 * [20]      0   - APD
 * [19]      0   - K2DB2=0
 * [18]      0   - K2RUN=0
 * [17]      1   - K1DB2=1
 * [16]      1   - K1RUN=1
 * [15]      0   - EP1IN
 * [14]      1   - K0DB2=1
 * [13]      1   - K0RUN=1
 * [12]
 * [11..0]  17   - DRI=17
 */
#define DEFAULT_MDREFR_VAL	0x00036017
#define DEFAULT_MDMRS_VAL	0x00320032

#define DEFAULT_FLYCNFG_VAL	0x00000000
#define DEFAULT_SXCNFG_VAL	0x40044004

/*
 * PCMCIA and CF Interfaces
 */
#define DEFAULT_MECR_VAL	0x00000001
#define DEFAULT_MCMEM0_VAL	0x00014307
#define DEFAULT_MCMEM1_VAL	0x00014307
#define DEFAULT_MCATT0_VAL	0x0001c787
#define DEFAULT_MCATT1_VAL	0x0001c787
#define DEFAULT_MCIO0_VAL	0x0001430f
#define DEFAULT_MCIO1_VAL	0x0001430f

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
	uint32_t tmp, mask;
	int i;
	/*
	 * 1) Initialize Asynchronous static memory controller
	 */

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
	tmp |= DEFAULT_MDREFR_VAL & 0xfff;
	writelrb(tmp, &MDREFR);

	/* clear the free-running clock bits (clear K0Free, K1Free, K2Free) */
	mask = MDREFR_K0FREE | MDREFR_K1FREE | MDREFR_K2FREE |
		MDREFR_K0DB2 | MDREFR_K0DB4 | MDREFR_K1DB2 | MDREFR_K2DB2 |
		MDREFR_K0RUN | MDREFR_K1RUN | MDREFR_K2RUN;
	tmp &= ~mask;
	tmp |= (DEFAULT_MDREFR_VAL & mask);
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

	tmp &= ~MDREFR_SLFRSH;
	writelrb(tmp, &MDREFR);
	tmp |= MDREFR_E1PIN;
	writelrb(tmp, &MDREFR);

	/*
	 * 7) Write MDCNFG with MDCNFG:DEx deasserted (set to 0), to configure
	 *    but not enable each SDRAM partition pair.
	 */

	mask = MDCNFG_DE0 | MDCNFG_DE1 | MDCNFG_DE2 | MDCNFG_DE3;
	writelrb(DEFAULT_MDCNFG_VAL & ~mask, &MDCNFG);

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
		readl(0xa0000000);
		barrier();
	}
	/*
	 * 9) Write MDCNFG with enable bits asserted (MDCNFG:DEx set to 1).
	 */

	tmp = (readl(&MDCNFG) & ~mask) | (DEFAULT_MDCNFG_VAL & mask);
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
