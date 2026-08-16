// SPDX-License-Identifier: GPL-2.0-only
/*
 * First stage entry for the Raumfeld speakers.
 *
 * Unlike the second stage entry in lowlevel.c, nothing has run before this:
 * the Boot ROM has copied us from NAND into internal SRAM and jumped here, so
 * pinmuxing, clocks, the UART and DRAM all have to be set up before barebox
 * can be loaded into DRAM and started.
 *
 * The register values are the ones the vendor OBM in the device's NAND uses.
 */

#include <common.h>
#include <debug_ll.h>
#include <mach/pxa/debug_ll.h>
#include <asm/barebox-arm.h>
#include <asm/barebox-arm-head.h>
#include <asm/io.h>
#include <mach/pxa/xload.h>

/*
 * We run from internal SRAM, which ends at 0x5c040000 on this part. The Boot
 * ROM's own scratch area is well below the OBM load address of 0x5c020000.
 */
#define RAUMFELD_OBM_STACK_TOP	0x5c040000

/* OS timer, used as the time base for the delays during DRAM setup */
#define OSCR			0x40a00010

/* Application subsystem clock configuration */
#define ACCR			0x41340000
#define ACCR_INIT		0x028cf298

/* Dynamic memory controller */
#define DMC_MDCNFG		0x48100000
#define DMC_MDREFR		0x48100004
#define DMC_DDR_HCAL		0x48100060
#define DMC_DDR_SCAL		0x48100040
#define DMC_DDR_WCAL		0x48100100

#define MDCNFG_INIT		0x8000044d
#define MDCNFG_DMCEN		0x40000000
#define MDREFR_INIT		0x00000006

struct reg_value {
	u32 reg;
	u32 val;
};

/*
 * Pin multiplexing. The 0x1c01 block is the DDR address and data bus, the
 * rest is the static memory controller and the NAND interface.
 */
static const struct reg_value mfp_init[] = {
	{ 0x40e1020c, 0x00001501 }, { 0x40e10240, 0x00001500 },
	{ 0x40e100cc, 0x0000d481 }, { 0x40e10200, 0x0000d481 },
	{ 0x40e10248, 0x00000001 }, { 0x40e100c8, 0x0000d5c0 },
	{ 0x40e10220, 0x00001c01 }, { 0x40e10228, 0x00001c01 },
	{ 0x40e10230, 0x00001c01 }, { 0x40e10238, 0x00001c01 },
	{ 0x40e10258, 0x00001c01 }, { 0x40e10260, 0x00001c01 },
	{ 0x40e10268, 0x00001c01 }, { 0x40e10270, 0x00001c01 },
	{ 0x40e10224, 0x00001c01 }, { 0x40e1022c, 0x00001c01 },
	{ 0x40e10234, 0x00001c01 }, { 0x40e1023c, 0x00001c01 },
	{ 0x40e1025c, 0x00001c01 }, { 0x40e10264, 0x00001c01 },
	{ 0x40e1026c, 0x00001c01 }, { 0x40e10274, 0x00001c01 },
	{ 0x40e100b8, 0x00001401 },
};

/*
 * FFUART, the console: GPIO77 RXD, 78 TXD, 79 CTS, 81 DSR, 83 DTR, 84 RTS,
 * all alternate function 1.
 *
 * This is not part of the vendor OBM's table. It did not need it - the U-Boot
 * it loaded muxed these pins before using them, and the OBM itself is silent.
 * Nothing does it in this boot path: pxa_debug_ll_init() only programs the
 * UART registers, and no pinctrl driver binds to the pinconf-single node
 * further along either. Without this everything from here to the barebox
 * shell runs correctly and says nothing at all.
 *
 * Values as written by that U-Boot.
 */
static const struct reg_value uart_mfp_init[] = {
	{ 0x40e104c8, 0x0000c801 }, { 0x40e104cc, 0x00000801 },
	{ 0x40e104d0, 0x0000a801 }, { 0x40e104d8, 0x00000801 },
	{ 0x40e104e0, 0x00000801 }, { 0x40e104e4, 0x00000801 },
};

/*
 * Static memory controller. The SMSC ethernet sits on nCS2, and nothing has
 * configured the controller for it: the vendor OBM does not touch it either,
 * because the U-Boot it loaded did so itself before probing the chip. Without
 * this the chip select is dead and the driver gives up with "Device not READY
 * in 100ms aborting".
 *
 * Values read out of that U-Boot (mtd0, literal pool at 0x40744).
 */
static const struct reg_value smc_init[] = {
	{ 0x4a000088, 0x00320809 },	/* CSADRCFG2, the ethernet chip select */
	{ 0x4a000008, 0x00000779 },	/* MSC0 */
	{ 0x4a00000c, 0x7ff07ff0 },	/* MSC1 */
	{ 0x4a00001c, 0x00880008 },	/* SXCNFG */
};

/* Pins and GPIO state the vendor OBM sets up after DRAM is running. */
static const struct reg_value late_init[] = {
	{ 0x40e10524, 0x00000000 }, { 0x40e10528, 0x00000000 },
	{ 0x40e0010c, 0x00000006 }, { 0x40e00118, 0x00000004 },
	{ 0x40e00124, 0x00000004 }, { 0x40e10000, 0x00001901 },
};

static void write_regs(const struct reg_value *r, unsigned int n)
{
	unsigned int i;

	for (i = 0; i < n; i++)
		writel(r[i].val, IOMEM(r[i].reg));
}

/* Busy wait on the OS timer, which is running out of reset. */
static void obm_delay(u32 ticks)
{
	writel(0, IOMEM(OSCR));

	while (readl(IOMEM(OSCR)) <= ticks)
		;
}

/*
 * DRAM setup. The readbacks after the stores are the vendor's write flushes,
 * the delays are what the DDR device needs between the steps; both are kept
 * as they are.
 */
static void obm_ddr_init(void)
{
	writel(MDCNFG_INIT, IOMEM(DMC_MDCNFG));
	readl(IOMEM(DMC_MDCNFG));
	obm_delay(300);

	writel(0x80000000, IOMEM(DMC_DDR_WCAL));
	readl(IOMEM(DMC_DDR_WCAL));

	writel(0x88000000, IOMEM(DMC_DDR_HCAL));
	obm_delay(5);
	readl(IOMEM(DMC_DDR_HCAL));

	writel(0x60000033, IOMEM(DMC_DDR_SCAL));
	obm_delay(300);

	writel(0x60010000, IOMEM(DMC_DDR_SCAL));
	obm_delay(300);

	writel(MDREFR_INIT, IOMEM(DMC_MDREFR));
	readl(IOMEM(DMC_MDREFR));

	writel(readl(IOMEM(DMC_MDCNFG)) | MDCNFG_DMCEN, IOMEM(DMC_MDCNFG));
}

static noinline void __noreturn raumfeld_obm_start(void)
{
	void *barebox;

	write_regs(mfp_init, ARRAY_SIZE(mfp_init));
	write_regs(uart_mfp_init, ARRAY_SIZE(uart_mfp_init));

	/* Written twice, as the vendor OBM does. */
	writel(ACCR_INIT, IOMEM(ACCR));
	writel(ACCR_INIT, IOMEM(ACCR));

	pxa_debug_ll_init();
	putc_ll('>');

	obm_ddr_init();
	write_regs(late_init, ARRAY_SIZE(late_init));
	write_regs(smc_init, ARRAY_SIZE(smc_init));

	/*
	 * The NTIM in NAND says where barebox lives and how big it is, so
	 * unlike the vendor OBM this does not copy a fixed amount.
	 */
	barebox = pxa_nand_load_image(NTIM_ID_BOOT);
	if (!barebox) {
		putc_ll('!');
		while (1)
			;
	}

	putc_ll('>');

	/*
	 * barebox is a separate image with its own entry point, loaded to the
	 * address its NTIM entry gives, so enter it rather than continuing
	 * into a barebox linked into this one.
	 */
	((void (*)(void))barebox)();

	while (1)
		;
}

ENTRY_FUNCTION_WITHSTACK(start_raumfeld_speaker_obm, RAUMFELD_OBM_STACK_TOP,
			 r0, r1, r2)
{
	arm_cpu_lowlevel_init();

	raumfeld_obm_start();
}
