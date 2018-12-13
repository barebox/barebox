// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2014 WAGO Kontakttechnik GmbH & Co. KG <http://global.wago.com>
 * Author: Heinrich Toews <heinrich.toews@wago.com>
 */

#include <common.h>
#include <init.h>
#include <linux/sizes.h>
#include <io.h>
#include <linux/string.h>
#include <debug_ll.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <mach/generic.h>
#include <mach/sdrc.h>
#include <mach/sys_info.h>
#include <mach/syslib.h>
#include <mach/wdt.h>
#include <mach/omap3-mux.h>
#include <mach/omap3-silicon.h>
#include <mach/omap3-generic.h>
#include <mach/omap3-clock.h>
#include <mach/control.h>
#include <asm/common.h>
#include <asm-generic/memory_layout.h>

#include <mach/emif4.h>

static void mux_config(void)
{
	/* SDRC */
	MUX_VAL(CP(SDRC_D0),		(IEN  | PTD | DIS | M0));
	MUX_VAL(CP(SDRC_D1),		(IEN  | PTD | DIS | M0));
	MUX_VAL(CP(SDRC_D2),		(IEN  | PTD | DIS | M0));
	MUX_VAL(CP(SDRC_D3),		(IEN  | PTD | DIS | M0));
	MUX_VAL(CP(SDRC_D4),		(IEN  | PTD | DIS | M0));
	MUX_VAL(CP(SDRC_D5),		(IEN  | PTD | DIS | M0));
	MUX_VAL(CP(SDRC_D6),		(IEN  | PTD | DIS | M0));
	MUX_VAL(CP(SDRC_D7),		(IEN  | PTD | DIS | M0));
	MUX_VAL(CP(SDRC_D8),		(IEN  | PTD | DIS | M0));
	MUX_VAL(CP(SDRC_D9),		(IEN  | PTD | DIS | M0));
	MUX_VAL(CP(SDRC_D10),		(IEN  | PTD | DIS | M0));
	MUX_VAL(CP(SDRC_D11),		(IEN  | PTD | DIS | M0));
	MUX_VAL(CP(SDRC_D12),		(IEN  | PTD | DIS | M0));
	MUX_VAL(CP(SDRC_D13),		(IEN  | PTD | DIS | M0));
	MUX_VAL(CP(SDRC_D14),		(IEN  | PTD | DIS | M0));
	MUX_VAL(CP(SDRC_D15),		(IEN  | PTD | DIS | M0));
	MUX_VAL(CP(SDRC_D16),		(IEN  | PTD | DIS | M0));
	MUX_VAL(CP(SDRC_D17),		(IEN  | PTD | DIS | M0));
	MUX_VAL(CP(SDRC_D18),		(IEN  | PTD | DIS | M0));
	MUX_VAL(CP(SDRC_D19),		(IEN  | PTD | DIS | M0));
	MUX_VAL(CP(SDRC_D20),		(IEN  | PTD | DIS | M0));
	MUX_VAL(CP(SDRC_D21),		(IEN  | PTD | DIS | M0));
	MUX_VAL(CP(SDRC_D22),		(IEN  | PTD | DIS | M0));
	MUX_VAL(CP(SDRC_D23),		(IEN  | PTD | DIS | M0));
	MUX_VAL(CP(SDRC_D24),		(IEN  | PTD | DIS | M0));
	MUX_VAL(CP(SDRC_D25),		(IEN  | PTD | DIS | M0));
	MUX_VAL(CP(SDRC_D26),		(IEN  | PTD | DIS | M0));
	MUX_VAL(CP(SDRC_D27),		(IEN  | PTD | DIS | M0));
	MUX_VAL(CP(SDRC_D28),		(IEN  | PTD | DIS | M0));
	MUX_VAL(CP(SDRC_D29),		(IEN  | PTD | DIS | M0));
	MUX_VAL(CP(SDRC_D30),		(IEN  | PTD | DIS | M0));
	MUX_VAL(CP(SDRC_D31),		(IEN  | PTD | DIS | M0));
	MUX_VAL(CP(SDRC_CLK),		(IEN  | PTD | DIS | M0));
	MUX_VAL(CP(SDRC_DQS0),		(IEN  | PTD | DIS | M0));
	MUX_VAL(CP(SDRC_DQS1),		(IEN  | PTD | DIS | M0));
	MUX_VAL(CP(SDRC_DQS2),		(IEN  | PTD | DIS | M0));
	MUX_VAL(CP(SDRC_DQS3),		(IEN  | PTD | DIS | M0));
	MUX_VAL(CP(SDRC_DQS0N),		(IEN  | PTD | EN  | M0));
	MUX_VAL(CP(SDRC_DQS1N),		(IEN  | PTD | EN  | M0));
	MUX_VAL(CP(SDRC_DQS2N),		(IEN  | PTD | EN  | M0));
	MUX_VAL(CP(SDRC_DQS3N),		(IEN  | PTD | EN  | M0));
	MUX_VAL(CP(SDRC_CKE0),		(M0));
	MUX_VAL(CP(SDRC_CKE1),		(M0));
	/* sdrc_strben_dly0 */
	MUX_VAL(CP(STRBEN_DLY0),	(IEN  | PTD | EN  | M0));
	 /*sdrc_strben_dly1*/
	MUX_VAL(CP(STRBEN_DLY1),	(IEN  | PTD | EN  | M0));

	/* GPMC */
	MUX_VAL(CP(GPMC_A1),		(IDIS | PTU | EN  | M0));
	MUX_VAL(CP(GPMC_A2),		(IDIS | PTU | EN  | M0));
	MUX_VAL(CP(GPMC_A3),		(IDIS | PTU | EN  | M0));
	MUX_VAL(CP(GPMC_A4),		(IDIS | PTU | EN  | M0));
	MUX_VAL(CP(GPMC_A5),		(IDIS | PTU | EN  | M0));
	MUX_VAL(CP(GPMC_A6),		(IDIS | PTU | EN  | M0));
	MUX_VAL(CP(GPMC_A7),		(IDIS | PTU | EN  | M0));
	MUX_VAL(CP(GPMC_A8),		(IDIS | PTU | EN  | M0));
	MUX_VAL(CP(GPMC_A9),		(IDIS | PTU | EN  | M0));
	MUX_VAL(CP(GPMC_A10),		(IDIS | PTU | EN  | M0));
	MUX_VAL(CP(GPMC_D0),		(IEN  | PTU | EN  | M0));
	MUX_VAL(CP(GPMC_D1),		(IEN  | PTU | EN  | M0));
	MUX_VAL(CP(GPMC_D2),		(IEN  | PTU | EN  | M0));
	MUX_VAL(CP(GPMC_D3),		(IEN  | PTU | EN  | M0));
	MUX_VAL(CP(GPMC_D4),		(IEN  | PTU | EN  | M0));
	MUX_VAL(CP(GPMC_D5),		(IEN  | PTU | EN  | M0));
	MUX_VAL(CP(GPMC_D6),		(IEN  | PTU | EN  | M0));
	MUX_VAL(CP(GPMC_D7),		(IEN  | PTU | EN  | M0));
	MUX_VAL(CP(GPMC_D8),		(IEN  | PTU | EN  | M0));
	MUX_VAL(CP(GPMC_D9),		(IEN  | PTU | EN  | M0));
	MUX_VAL(CP(GPMC_D10),		(IEN  | PTU | EN  | M0));
	MUX_VAL(CP(GPMC_D11),		(IEN  | PTU | EN  | M0));
	MUX_VAL(CP(GPMC_D12),		(IEN  | PTU | EN  | M0));
	MUX_VAL(CP(GPMC_D13),		(IEN  | PTU | EN  | M0));
	MUX_VAL(CP(GPMC_D14),		(IEN  | PTU | EN  | M0));
	MUX_VAL(CP(GPMC_D15),		(IEN  | PTU | EN  | M0));
	MUX_VAL(CP(GPMC_NCS0),		(IDIS | PTU | EN  | M0));
	MUX_VAL(CP(GPMC_NCS1),		(IDIS | PTU | EN  | M0));
	MUX_VAL(CP(GPMC_NCS2),		(IDIS | PTU | EN  | M0));
	MUX_VAL(CP(GPMC_NCS3),		(IDIS | PTU | EN  | M0));
	MUX_VAL(CP(GPMC_NCS4),		(IEN  | PTU | EN  | M0));
	MUX_VAL(CP(GPMC_NCS5),		(IDIS | PTU | EN  | M0));
	MUX_VAL(CP(GPMC_NCS6),		(IEN  | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_NCS7),		(IEN  | PTU | EN  | M0));
	MUX_VAL(CP(GPMC_CLK),		(IDIS | PTU | EN  | M0));
	MUX_VAL(CP(GPMC_NADV_ALE),	(IDIS | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_NOE),		(IDIS | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_NWE),		(IDIS | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_NBE0_CLE),	(IDIS | PTU | EN  | M0));
	MUX_VAL(CP(GPMC_NBE1),		(IEN  | PTU | EN  | M0));
	MUX_VAL(CP(GPMC_NWP),		(IEN  | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_WAIT0),		(IEN  | PTU | EN  | M0));
	MUX_VAL(CP(GPMC_WAIT1),		(IEN  | PTU | EN  | M0));
	MUX_VAL(CP(GPMC_WAIT2),		(IEN  | PTU | EN  | M4)); /*GPIO_64*/
							 /* - ETH_nRESET*/
	MUX_VAL(CP(GPMC_WAIT3),		(IEN  | PTU | EN  | M0));

	/* MMC */
	MUX_VAL(CP(MMC1_CLK),		(IEN  | PTU | EN  | M0));
	MUX_VAL(CP(MMC1_CMD),		(IEN  | PTU | DIS | M0));
	MUX_VAL(CP(MMC1_DAT0),		(IEN  | PTU | DIS | M0));
	MUX_VAL(CP(MMC1_DAT1),		(IEN  | PTU | DIS | M0));
	MUX_VAL(CP(MMC1_DAT2),		(IEN  | PTU | DIS | M0));
	MUX_VAL(CP(MMC1_DAT3),		(IEN  | PTU | DIS | M0));

	/* MMC GPIOs */
	MUX_VAL(CP(MCBSP2_FSX),		(IEN  | PTD | DIS | M4)); /* McBSP2_FSX    -> SD-MMC1-CD (GPIO_116) */
	MUX_VAL(CP(MCBSP2_CLKX),	(IDIS | PTU | DIS | M4)); /* McBSP2_CLKX   -> SD-MMC1-EN (GPIO_117) */
	MUX_VAL(CP(MCBSP2_DR),		(IEN  | PTD | DIS | M4)); /* McBSP2_DR     -> SD-MMC1-WP (GPIO_118) */
	MUX_VAL(CP(MMC2_DAT7),		(IDIS | PTU | DIS | M4)); /* MMC2_DAT7     -> SD-MMC1-RW (GPIO_139) */

	/* UART1 */
	MUX_VAL(CP(UART1_TX),		(IDIS | PTD | DIS | M0));
	MUX_VAL(CP(UART1_RTS),		(IDIS | PTD | DIS | M0)); /* M4, GPIO_149 */
	MUX_VAL(CP(UART1_CTS),		(IEN  | PTU | DIS | M0));
	MUX_VAL(CP(UART1_RX),		(IEN  | PTD | DIS | M0));

	MUX_VAL(CP(MCSPI1_CS2),		(IDIS | PTU | DIS | M4)); /* MCSPI1_CS2    -> SEL_RS232/485_GPIO176 (GPIO_176) */

	/* UART2 */
	MUX_VAL(CP(UART2_TX),		(IDIS | PTD | DIS | M0));
	MUX_VAL(CP(UART2_RX),		(IEN  | PTD | DIS | M0));

	/* WATCHDOG */
	MUX_VAL(CP(UART3_RTS_SD),	(IDIS | PTD | DIS | M4)); /* Trigger Event <1,6s */
	MUX_VAL(CP(UART3_CTS_RCTX),	(IDIS | PTD | DIS | M4)); /* Enable */

	/* I2C1: PMIC */
	MUX_VAL(CP(I2C1_SCL),		(IEN  | PTU | EN  | M0));
	MUX_VAL(CP(I2C1_SDA),		(IEN  | PTU | EN  | M0));
	MUX_VAL(CP(SYS_NIRQ),		(IEN  | PTU | EN  | M4)); /* SYS_nIRQ      -> PMIC_nINT1      (GPIO_0) */

	/* I2C2: RTC, EEPROM */
	MUX_VAL(CP(I2C2_SCL),		(IEN  | PTU | EN  | M0)); /* RTC_EEPROM_SCL2 */
	MUX_VAL(CP(I2C2_SDA),		(IEN  | PTU | EN  | M0)); /* RTC_EEPROM_SDA2 */
	MUX_VAL(CP(HDQ_SIO),		(IDIS | PTD | DIS  | M4)); /* HDQ_SIO       -> WD_nWP   GPIO_170 */
	MUX_VAL(CP(SYS_CLKREQ),		(IEN  | PTD | DIS | M4));  /* SYS_CLKREQ    -> RTC_nINT (GPIO_1) */

	/* GPIO_BANK3: operating mode switch and reset all pushbutton */
	MUX_VAL(CP(DSS_DATA20),  (IEN  | PTU | EN  | M4));  //    DSS_DATA20    -> BAS_RUN      /* GPIO 90 */ (sync, changed)
	MUX_VAL(CP(DSS_DATA21),  (IEN  | PTU | EN  | M4));  //    DSS_DATA21    -> BAS_STOP     /* GPIO 91 */ (sync, changed)
	MUX_VAL(CP(DSS_DATA22),  (IEN  | PTU | EN  | M4));  //    DSS_DATA22    -> BAS_RESET    /* GPIO 92 */ (sync, changed)
	MUX_VAL(CP(DSS_DATA23),  (IEN  | PTU | EN  | M4));  //    DSS_DATA23    -> RESET_ALL    /* GPIO 93 */ (sync, changed)
	MUX_VAL(CP(CCDC_PCLK) , (IEN  | PTU | EN  | M4));   //    CCDC_PCLK     -> System Reset /* GPIO 94 */ (sync, changed) Reserved for later use!

	/* *********** ADDED FOR JTAG DEBUGGING ************* */
	MUX_VAL(CP(SYS_NRESWARM),     	(IDIS | PTU | DIS | M4));
}

static noinline void pfc200_board_init(void)
{
	int in_sdram = omap3_running_in_sdram();
	u32 r0;

	if (IS_ENABLED(CONFIG_DEBUG_LL)) {
		am33xx_uart_soft_reset(IOMEM(OMAP3_UART3_BASE));
		omap_uart_lowlevel_init(IOMEM(OMAP3_UART3_BASE));
		putc_ll('>');
	}

	omap3_core_init();

	mux_config();

#define CONTROL_DEVCONF3   0x48002584
	/* activate DDR2 CPU Termination */
	r0 = readl(CONTROL_DEVCONF3);
	writel(r0 | 0x2, CONTROL_DEVCONF3);

	/* Dont reconfigure SDRAM while running in SDRAM */
	if (!in_sdram)
		am35xx_emif4_init();

	barebox_arm_entry(0x80000000, SZ_256M, NULL);
}

ENTRY_FUNCTION(start_am35xx_pfc_750_820x_sram, bootinfo, r1, r2)
{
	omap3_save_bootinfo((void *)bootinfo);

	arm_cpu_lowlevel_init();

	omap3_gp_romcode_call(OMAP3_GP_ROMCODE_API_L2_INVAL, 0);

	relocate_to_current_adr();
	setup_c();

	pfc200_board_init();
}

extern char __dtb_am35xx_pfc_750_820x_start[];

ENTRY_FUNCTION(start_am35xx_pfc_750_820x_sdram, r0, r1, r2)
{
	void *fdt = __dtb_am35xx_pfc_750_820x_start;

	fdt += get_runtime_offset();

	barebox_arm_entry(0x80000000, SZ_256M, fdt);
}
