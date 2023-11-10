// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2019 Zodiac Inflight Innovation
 * Author: Andrey Smirnov <andrew.smirnov@gmail.com>
 */

#include <common.h>
#include <firmware.h>
#include <linux/sizes.h>
#include <mach/imx/generic.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <mach/imx/imx8m-ccm-regs.h>
#include <mach/imx/iomux-mx8mq.h>
#include <soc/imx8m/ddr.h>
#include <mach/imx/xload.h>
#include <io.h>
#include <debug_ll.h>
#include <mach/imx/debug_ll.h>
#include <asm/cache.h>
#include <asm/sections.h>
#include <asm/mmu.h>
#include <mach/imx/atf.h>
#include <mach/imx/esdctl.h>

#include "ddr.h"

#define UART_PAD_CTRL	MUX_PAD_CTRL(PAD_CTL_DSE_3P3V_45_OHM)

static void setup_uart(void)
{
	void __iomem *uart = IOMEM(MX8M_UART1_BASE_ADDR);

	imx8m_early_setup_uart_clock();

	imx8mq_setup_pad(IMX8MQ_PAD_UART1_TXD__UART1_TX | UART_PAD_CTRL);
	imx8m_uart_setup(uart);

	pbl_set_putc(imx_uart_putc, uart);

	putc_ll('>');
}

/*
 * Two functions below are used when Barebox is used as a DDR
 * initializing payload for OpenOCD
 */
#define RDU3_TCM_MAGIC_LOCATION	0x007e1028
#define RDU3_TCM_MAGIC_REQUEST	0xdeadbeef
#define RDU3_TCM_MAGIC_REPLY	0xbaadf00d

static bool running_as_ddr_helper(void)
{
	return readl(RDU3_TCM_MAGIC_LOCATION) == RDU3_TCM_MAGIC_REQUEST;
}

static __noreturn void ddr_helper_halt(void)
{
	writel(RDU3_TCM_MAGIC_REPLY, RDU3_TCM_MAGIC_LOCATION);
	asm volatile("hlt 0");
	BUG(); 	/* To prevent noreturn warnings */
}

static void zii_imx8mq_dev_sram_setup(void)
{
	ddr_init();

	if (running_as_ddr_helper())
		ddr_helper_halt();
}

enum zii_platform_imx8mq_type {
	ZII_PLATFORM_IMX8MQ_ULTRA_RMB3 = 0b0000,
	ZII_PLATFORM_IMX8MQ_ULTRA_ZEST = 0b1000,
};

static unsigned int get_system_type(void)
{
#define GPIO_DR		0x000
#define GPIO_GDIR	0x004
#define SYSTEM_TYPE	GENMASK(24, 21)

	u32 gdir, dr;
	void __iomem *gpio3 = IOMEM(MX8MQ_GPIO3_BASE_ADDR);
	void __iomem *iomuxbase = IOMEM(MX8MQ_IOMUXC_BASE_ADDR);

	/*
	 * System type is encoded as a 4-bit number specified by the
	 * following pins (pulled up or down with resistors on the
	 * board).
	*/
	imx_setup_pad(iomuxbase, IMX8MQ_PAD_SAI5_RXD0__GPIO3_IO21);
	imx_setup_pad(iomuxbase, IMX8MQ_PAD_SAI5_RXD1__GPIO3_IO22);
	imx_setup_pad(iomuxbase, IMX8MQ_PAD_SAI5_RXD2__GPIO3_IO23);
	imx_setup_pad(iomuxbase, IMX8MQ_PAD_SAI5_RXD3__GPIO3_IO24);

	gdir = readl(gpio3 + GPIO_GDIR);
	gdir &= ~SYSTEM_TYPE;
	writel(gdir, gpio3 + GPIO_GDIR);

	dr = readl(gpio3 + GPIO_DR);

	return FIELD_GET(SYSTEM_TYPE, dr);
}

extern char __dtb_z_imx8mq_zii_ultra_rmb3_start[];
extern char __dtb_z_imx8mq_zii_ultra_zest_start[];

static __noreturn noinline void zii_imx8mq_dev_start(void)
{
	unsigned int system_type;
	void *fdt;

	setup_uart();

	if (get_pc() < MX8MQ_DDR_CSD1_BASE_ADDR) {
		/*
		 * We assume that we were just loaded by MaskROM into
		 * SRAM if we are not running from DDR. We also assume
		 * that means DDR needs to be initialized for the
		 * first time.
		 */
		zii_imx8mq_dev_sram_setup();
	}
	/*
	 * Straight from the power-on we are at EL3, so the following
	 * code _will_ load and jump to ATF.
	 *
	 * However when we are re-executed upon exit from ATF's
	 * initialization routine, it is EL2 which means we'll skip
	 * loadting ATF blob again
	 */
	if (current_el() == 3)
		imx8mq_load_and_start_image_via_tfa();

	system_type = get_system_type();

	switch (system_type) {
	default:
		if (IS_ENABLED(CONFIG_DEBUG_LL)) {
			puts_ll("\n*********************************\n");
			puts_ll("* Unknown system type: ");
			puthex_ll(system_type);
			puts_ll("\n* Assuming Ultra RMB3\n");
			puts_ll("*********************************\n");
		}
		/* FALLTHROUGH */
	case ZII_PLATFORM_IMX8MQ_ULTRA_RMB3:
		fdt = __dtb_z_imx8mq_zii_ultra_rmb3_start;
		break;
	case ZII_PLATFORM_IMX8MQ_ULTRA_ZEST:
		fdt = __dtb_z_imx8mq_zii_ultra_zest_start;
		break;
	}

	/*
	 * Standard entry we hit once we initialized both DDR and ATF
	 */
	imx8mq_barebox_entry(fdt);
}

/*
 * Power-on execution flow of start_zii_imx8mq_dev() might not be
 * obvious for a very frist read, so here's, hopefully helpful,
 * summary:
 *
 * 1. MaskROM uploads PBL into OCRAM and that's where this function is
 *    executed for the first time
 *
 * 2. DDR is initialized and full i.MX image is loaded to the
 *    beginning of RAM
 *
 * 3. start_nxp_imx8mq_evk, now in RAM, is executed again
 *
 * 4. BL31 blob is uploaded to OCRAM and the control is transfer to it
 *
 * 5. BL31 exits EL3 into EL2 at address MX8MQ_ATF_BL33_BASE_ADDR,
 *    executing start_nxp_imx8mq_evk() the third time
 *
 * 6. Standard barebox boot flow continues
 */
ENTRY_FUNCTION(start_zii_imx8mq_dev, r0, r1, r2)
{
	imx8mq_cpu_lowlevel_init();
	relocate_to_current_adr();
	setup_c();

	zii_imx8mq_dev_start();
}
