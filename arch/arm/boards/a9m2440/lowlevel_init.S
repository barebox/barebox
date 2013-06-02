/*
 *
 */

#include <config.h>
#include <sizes.h>
#include <mach/s3c-iomap.h>
#include <mach/s3c24xx-gpio.h>
#include <asm/barebox-arm-head.h>

	.section ".text_bare_init.barebox_arm_reset_vector","ax"

/*
 * To be able to setup the SDRAM interface correctly, we need some
 * external information about the connected SDRAM devices.
 *
 * When we set GPH8, we can read at GPB:
 * Bit 0..1: Memory device size -> 00=16M, 01=64M, 10=32M, 11=128M
 * Bit 2: CL setting
 *
 * Some remarks: The CL setting seems useless. It always signals a CL3
 * requirement, but the SDRAM types I found on the cards are supporting
 * CL2 @ 100 MHz. But also these SDRAM types are only support 105 MHz max.
 * So, we never need CL3 because we can't run the CPU at 533 MHz (which
 * implies an 133 MHz SDRAM clock).
 * All devices are connected via 32 bit databus
 *
 * Note: I was able to check the 32 MiB and 64 MiB configuration only. I didn't
 * had access to a 16 MiB nor 128 MiB config.
 *
 */

sdram_init:
	/*
	 * Read the configuration. After reset until any GPIO port is
	 * configured yet, these pins show external settings, to detect
	 * the SDRAM size.
	 */
	ldr r1, =S3C_GPBDAT
	ldr r4, [r1]
	and r4, r4, #0x3

	ldr r1, =S3C_MEMCTL_BASE
	/* configure both SDRAM areas with 32 bit data bus width */
	ldr r0, =((0x2 << 24) + (0x2 << 28))
	str r0, [r1], #0x1c	/* post add register offset for bank6 */

	/*
	 * With the configuration we simply need to calculate an offset into
	 * our table with the predefined SDRAM settings
	 */
	adr r0, SDRAMDATA
	mov r2, #6*4		/* # of bytes per table entry */
	mul r3, r4, r2
	add r0, r0, r3		/* start address of the entry */

	/*
	 * store the table entry data into the registers
	 */
1:
	ldr     r3, [r0], #4
	str     r3, [r1], #4
	subs    r2, r2, #4
	bne     1b

/* TODO: Check if the second bank is populated, and switch it off if not */

	mov pc, lr

/*
 * we need 4 sets of memory settings per main CPU clock speed
 *
 * 400MHz main speed:
 * - 16 MiB in the first bank, maybe 16 MiB in the second bank (untested!)
 * - 32 MiB in the first bank, maybe 32 MiB in the second bank (CL=2)
 * - 64 MiB in the first bank, maybe 64 MiB in the second bank (CL=2)
 * - 128 MiB in the first bank, maybe 128 MiB in the second bank (untested!)
 *
 * Note: SDRAM clock runs at 100MHz
 */

SDRAMDATA:
/* --------------------------- 16 MiB @ 100MHz --------------------------- */
	/*
	 *  - MT = 11 (= sync dram type)
	 *  - Trcd = 01 (= CL3)
	 *  - SCAN = 00 (= 8 bit collumns)
	 */
	.word ((0x3 << 15) + (0x1 << 2) + (0x0))
	.word ((0x3 << 15) + (0x1 << 2) + (0x0))
	/*
	 * SDRAM refresh settings
	 *  - REFEN = 1 (= refresh enabled)
	 *  - TREFMD = 0 (= auto refresh)
	 *  - Trp = 00 (= 2 RAS precharge clocks)
	 *  - Tsrc = 11 (= 7 clocks -> row cycle time @100MHz 2+5=7 -> 70ns)
	 *  - Refrsh = 2^11 + 1 - 100 * 15.6 = 2049 - 1560 = FIXME
	 */
	.word ((0x1 << 23) + (0x0 << 22) + (0x0 << 20) + (0x3 << 18) + 468)
	/*
	 * SDRAM banksize
	 *  - BURST_EN = 0 (= burst mode disabled)
	 *  - SCKE_EN = 1 (= SDRAM SCKE enabled)
	 *  - SCLK_EN = 1 (= clock active only during accesses)
	 *  - BK67MAP = 010 (= 128MiB) FIXME?????
	 */
	.word ((0 << 7) + (1 << 5) + (1 << 4) + 2)
	/*
	 * SDRAM mode register
	 * CL = 010 (= 2 clocks)
	 */
	.word (0x2 << 4)
	.word (0x2 << 4)

/* ------------- one or two banks with 64 MiB @ 100MHz -------------------- */

	/*
	 *  - MT = 11 (= sync dram type)
	 *  - Trcd = 00 (= CL2)
	 *  - SCAN = 01 (= 9 bit collumns)
	 */
	.word ((0x3 << 15) + (0x0 << 2) + (0x1))
	.word ((0x3 << 15) + (0x0 << 2) + (0x1))
	/*
	 * SDRAM refresh settings
	 *  - REFEN = 1 (= refresh enabled)
	 *  - TREFMD = 0 (= auto refresh)
	 *  - Trp = 00 (= 2 RAS precharge clocks)
	 *  - Tsrc = 01 (= 5 clocks -> row cycle time @100MHz 2+5=7 -> 70ns)
	 *  - Refrsh = 2^11 + 1 - 100 * 15.6 = 2049 - 1560 = 489
	 */
	.word ((0x1 << 23) + (0x0 << 22) + (0x0 << 20) + (0x1 << 18) + 489)
	/*
	 * SDRAM banksize
	 *  - BURST_EN = 1 (= burst mode enabled)
	 *  - SCKE_EN = 1 (= SDRAM SCKE enabled)
	 *  - SCLK_EN = 1 (= clock active only during accesses)
	 *  - BK67MAP = 001 (= 64 MiB)
	 */
	.word ((1 << 7) + (1 << 5) + (1 << 4) + 1)
	/*
	 * SDRAM mode register
	 * CL = 010 (= 2 clocks)
	 */
	.word (0x2 << 4)
	.word (0x2 << 4)

/* ------------- one or two banks with 32 MiB @ 100MHz -------------------- */

	/*
	 *  - MT = 11 (= sync dram type)
	 *  - Trcd = 00 (= CL2)
	 *  - SCAN = 01 (= 9 bit collumns)
	 */
	.word ((0x3 << 15) + (0x0 << 2) + (0x1))
	.word ((0x3 << 15) + (0x0 << 2) + (0x1))
	/*
	 * SDRAM refresh settings
	 *  - REFEN = 1 (= refresh enabled)
	 *  - TREFMD = 0 (= auto refresh)
	 *  - Trp = 00 (= 2 RAS precharge clocks)
	 *  - Tsrc = 01 (= 5 clocks -> row cycle time @100MHz 2+5=7 -> 70ns)
	 *  - Refrsh = 2^11 + 1 - 100 * 15.6 = 2049 - 1560 = 489
	 */
	.word ((0x1 << 23) + (0x0 << 22) + (0x0 << 20) + (0x1 << 18) + 489)
	/*
	 * SDRAM banksize
	 *  - BURST_EN = 1 (= burst mode enabled)
	 *  - SCKE_EN = 1 (= SDRAM SCKE enabled)
	 *  - SCLK_EN = 1 (= clock active only during accesses)
	 *  - BK67MAP = 000 (= 32 MiB)
	 */
	.word ((1 << 7) + (1 << 5) + (1 << 4) + 0)
	/*
	 * SDRAM mode register
	 * CL = 010 (= 2 clocks)
	 */
	.word (0x2 << 4)
	.word (0x2 << 4)

/* ------------ one or two banks with 128 MiB @ 100MHz -------------------- */

	/*
	 *  - MT = 11 (= sync dram type)
	 *  - Trcd = 00 (= CL2)
	 *  - SCAN = 01 (= 9 bit collumns)
	 */
	.word ((0x3 << 15) + (0x0 << 2) + (0x1))
	.word ((0x3 << 15) + (0x0 << 2) + (0x1))
	/*
	 * SDRAM refresh settings
	 *  - REFEN = 1 (= refresh enabled)
	 *  - TREFMD = 0 (= auto refresh)
	 *  - Trp = 00 (= 2 RAS precharge clocks)
	 *  - Tsrc = 01 (= 5 clocks -> row cycle time @100MHz 2+5=7 -> 70ns)
	 *  - Refrsh = 2^11 + 1 - 100 * 7.5 = 2049 - FIXME = 1259
	 */
	.word ((0x1 << 23) + (0x0 << 22) + (0x1 << 20) + (0x3 << 18) + 1259)
	/*
	 * SDRAM banksize
	 *  - BURST_EN = 0 (= burst mode disabled)
	 *  - SCKE_EN = 1 (= SDRAM SCKE enabled)
	 *  - SCLK_EN = 1 (= clock active only during accesses)
	 *  - BK67MAP = 010 (= 128MiB)
	 */
	.word (0x32)
	/*
	 * SDRAM mode register
	 * CL = 010 (= 2 clocks)
	 */
	.word (0x2 << 4)
	.word (0x2 << 4)

/* ------------------------------------------------------------------------ */

.globl barebox_arm_reset_vector
barebox_arm_reset_vector:

	bl arm_cpu_lowlevel_init

	bl s3c24x0_disable_wd

	/* skip everything here if we are already running from SDRAM */
	cmp pc, #S3C_SDRAM_BASE
	blo 1f
	cmp pc, #S3C_SDRAM_END
	bhs 1f

	b out

/* we are running from NOR or NAND/SRAM memory. Do further initialisation */
1:
	bl s3c24x0_pll_init

	bl sdram_init

#ifdef CONFIG_S3C_NAND_BOOT
/* up to here we are running from the internal SRAM area */
	bl s3c24x0_nand_boot
#endif
out:
	mov r0, #S3C_SDRAM_BASE
	mov r1, #SZ_32M
	mov r2, #0
	b barebox_arm_entry
