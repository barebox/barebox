#include <init.h>
#include <io.h>
#include <sizes.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <mach/control.h>
#include <mach/generic.h>
#include <mach/omap3-silicon.h>
#include <mach/omap3-generic.h>
#include <mach/omap3-mux.h>
#include <mach/sdrc.h>
#include <mach/syslib.h>
#include <mach/sys_info.h>

/**
 * @brief Do the pin muxing required for Board operation.
 * We enable ONLY the pins we require to set. OMAP provides pins which do not
 * have alternate modes. Such pins done need to be set.
 *
 * See @ref MUX_VAL for description of the muxing mode.
 *
 * @return void
 */
static void mux_config(void)
{
	/* SDRC_D0 - SDRC_D31 default mux mode is mode0 */

	/* GPMC */
	MUX_VAL(CP(GPMC_A1), (IDIS | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_A2), (IDIS | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_A3), (IDIS | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_A4), (IDIS | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_A5), (IDIS | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_A6), (IDIS | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_A7), (IDIS | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_A8), (IDIS | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_A9), (IDIS | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_A10), (IDIS | PTD | DIS | M0));

	/* D0-D7 default mux mode is mode0 */
	MUX_VAL(CP(GPMC_D8), (IEN | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_D9), (IEN | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_D10), (IEN | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_D11), (IEN | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_D12), (IEN | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_D13), (IEN | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_D14), (IEN | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_D15), (IEN | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_CLK), (IDIS | PTD | DIS | M0));
	/* GPMC_NADV_ALE default mux mode is mode0 */
	/* GPMC_NOE default mux mode is mode0 */
	/* GPMC_NWE default mux mode is mode0 */
	/* GPMC_NBE0_CLE default mux mode is mode0 */
	MUX_VAL(CP(GPMC_NBE0_CLE), (IDIS | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_NBE1), (IEN | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_NWP), (IEN | PTD | DIS | M0));
	/* GPMC_WAIT0 default mux mode is mode0 */
	MUX_VAL(CP(GPMC_WAIT1), (IEN | PTU | EN | M0));

	/* SERIAL INTERFACE */
	MUX_VAL(CP(UART3_CTS_RCTX), (IEN | PTD | EN | M0));
	MUX_VAL(CP(UART3_RTS_SD), (IDIS | PTD | DIS | M0));
	MUX_VAL(CP(UART3_RX_IRRX), (IEN | PTD | DIS | M0));
	MUX_VAL(CP(UART3_TX_IRTX), (IDIS | PTD | DIS | M0));
	MUX_VAL(CP(HSUSB0_CLK), (IEN | PTD | DIS | M0));
	MUX_VAL(CP(HSUSB0_STP), (IDIS | PTU | EN | M0));
	MUX_VAL(CP(HSUSB0_DIR), (IEN | PTD | DIS | M0));
	MUX_VAL(CP(HSUSB0_NXT), (IEN | PTD | DIS | M0));
	MUX_VAL(CP(HSUSB0_DATA0), (IEN | PTD | DIS | M0));
	MUX_VAL(CP(HSUSB0_DATA1), (IEN | PTD | DIS | M0));
	MUX_VAL(CP(HSUSB0_DATA2), (IEN | PTD | DIS | M0));
	MUX_VAL(CP(HSUSB0_DATA3), (IEN | PTD | DIS | M0));
	MUX_VAL(CP(HSUSB0_DATA4), (IEN | PTD | DIS | M0));
	MUX_VAL(CP(HSUSB0_DATA5), (IEN | PTD | DIS | M0));
	MUX_VAL(CP(HSUSB0_DATA6), (IEN | PTD | DIS | M0));
	MUX_VAL(CP(HSUSB0_DATA7), (IEN | PTD | DIS | M0));
	/* I2C1_SCL default mux mode is mode0 */
	/* I2C1_SDA default mux mode is mode0 */
	/* USB EHCI (port 2) */
	MUX_VAL(CP(MCSPI1_CS3),		(IEN  | PTU | DIS | M3));
	MUX_VAL(CP(MCSPI2_CLK),		(IEN  | PTU | DIS | M3));
	MUX_VAL(CP(MCSPI2_SIMO),	(IEN  | PTU | DIS | M3));
	MUX_VAL(CP(MCSPI2_SOMI),	(IEN  | PTU | DIS | M3));
	MUX_VAL(CP(MCSPI2_CS0),		(IEN  | PTU | DIS | M3));
	MUX_VAL(CP(MCSPI2_CS1),		(IEN  | PTU | DIS | M3));
	MUX_VAL(CP(ETK_D10_ES2),	(IDIS | PTU | DIS | M3));
	MUX_VAL(CP(ETK_D11_ES2),	(IDIS | PTU | DIS | M3));
	MUX_VAL(CP(ETK_D12_ES2),	(IEN  | PTU | DIS | M3));
	MUX_VAL(CP(ETK_D13_ES2),	(IEN  | PTU | DIS | M3));
	MUX_VAL(CP(ETK_D14_ES2),	(IEN  | PTU | DIS | M3));
	MUX_VAL(CP(ETK_D15_ES2),	(IEN  | PTU | DIS | M3));
	MUX_VAL(CP(UART2_RX),		(IEN  | PTD | DIS | M4)) /*GPIO_147*/;
	/* Expansion card */
	MUX_VAL(CP(MMC1_CLK),		(IDIS | PTU | EN  | M0)); /* MMC1_CLK */
	MUX_VAL(CP(MMC1_CMD),		(IEN  | PTU | EN  | M0)); /* MMC1_CMD */
	MUX_VAL(CP(MMC1_DAT0),		(IEN  | PTU | EN  | M0)); /* MMC1_DAT0 */
	MUX_VAL(CP(MMC1_DAT1),		(IEN  | PTU | EN  | M0)); /* MMC1_DAT1 */
	MUX_VAL(CP(MMC1_DAT2),		(IEN  | PTU | EN  | M0)); /* MMC1_DAT2 */
	MUX_VAL(CP(MMC1_DAT3),		(IEN  | PTU | EN  | M0)); /* MMC1_DAT3 */
	MUX_VAL(CP(MMC1_DAT4),		(IEN  | PTU | EN  | M0)); /* MMC1_DAT4 */
	MUX_VAL(CP(MMC1_DAT5),		(IEN  | PTU | EN  | M0)); /* MMC1_DAT5 */
	MUX_VAL(CP(MMC1_DAT6),		(IEN  | PTU | EN  | M0)); /* MMC1_DAT6 */
	MUX_VAL(CP(MMC1_DAT7),		(IEN  | PTU | EN  | M0)); /* MMC1_DAT7 */
}

/**
 * @brief Do the SDRC initialization for 128Meg Micron DDR for CS0
 *
 * @return void
 */
static void sdrc_init(void)
{
	/* SDRAM software reset */
	/* No idle ack and RESET enable */
	writel(0x1A, OMAP3_SDRC_REG(SYSCONFIG));
	sdelay(100);
	/* No idle ack and RESET disable */
	writel(0x18, OMAP3_SDRC_REG(SYSCONFIG));

	/* SDRC Sharing register */
	/* 32-bit SDRAM on data lane [31:0] - CS0 */
	/* pin tri-stated = 1 */
	writel(0x00000100, OMAP3_SDRC_REG(SHARING));

	/* ----- SDRC Registers Configuration --------- */
	/* SDRC_MCFG0 register */
	writel(0x02584099, OMAP3_SDRC_REG(MCFG_0));

	/* SDRC_RFR_CTRL0 register */
	writel(0x54601, OMAP3_SDRC_REG(RFR_CTRL_0));

	/* SDRC_ACTIM_CTRLA0 register */
	writel(0xA29DB4C6, OMAP3_SDRC_REG(ACTIM_CTRLA_0));

	/* SDRC_ACTIM_CTRLB0 register */
	writel(0x12214, OMAP3_SDRC_REG(ACTIM_CTRLB_0));

	/* Disble Power Down of CKE due to 1 CKE on combo part */
	writel(0x00000081, OMAP3_SDRC_REG(POWER));

	/* SDRC_MANUAL command register */
	/* NOP command */
	writel(0x00000000, OMAP3_SDRC_REG(MANUAL_0));
	/* Precharge command */
	writel(0x00000001, OMAP3_SDRC_REG(MANUAL_0));
	/* Auto-refresh command */
	writel(0x00000002, OMAP3_SDRC_REG(MANUAL_0));
	/* Auto-refresh command */
	writel(0x00000002, OMAP3_SDRC_REG(MANUAL_0));

	/* SDRC MR0 register Burst length=4 */
	writel(0x00000032, OMAP3_SDRC_REG(MR_0));

	/* SDRC DLLA control register */
	writel(0x0000000A, OMAP3_SDRC_REG(DLLA_CTRL));

	return;
}

/**
 * @brief The basic entry point for board initialization.
 *
 * This is called as part of machine init (after arch init).
 * This is again called with stack in SRAM, so not too many
 * constructs possible here.
 *
 * @return void
 */
static int beagle_board_init(void)
{
	int in_sdram = running_in_sdram();

	if (!in_sdram)
		omap3_core_init();

	mux_config();
	/* Dont reconfigure SDRAM while running in SDRAM! */
	if (!in_sdram)
		sdrc_init();

	return 0;
}

void __naked  __bare_init barebox_arm_reset_vector(uint32_t *data)
{
	omap3_save_bootinfo(data);

	arm_cpu_lowlevel_init();

	beagle_board_init();

	barebox_arm_entry(0x80000000, SZ_128M, 0);
}
