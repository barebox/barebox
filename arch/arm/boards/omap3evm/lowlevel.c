#include <io.h>
#include <init.h>
#include <sizes.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <mach/generic.h>
#include <mach/omap3-mux.h>
#include <mach/sdrc.h>
#include <mach/control.h>
#include <mach/syslib.h>
#include <mach/omap3-silicon.h>
#include <mach/omap3-generic.h>
#include <mach/sys_info.h>


/*
 * Boot-time initialization(s)
 */

/**
 * @brief Initialize the SDRC module
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
 * @brief Do the necessary pin muxing required for OMAP3EVM. Some pins in OMAP3
 * do not have alternate modes. We don't program these pins.
 *
 * See @ref MUX_VAL for description of the muxing mode.
 *
 * @return void
 */
static void mux_config(void)
{
	/*
	 * SDRC
	 * - SDRC_D0-SDRC_D31: Default MUX mode is mode0.
	 */

	/*
	 * GPMC
	 * - GPMC_D0-GPMC_D7: Default MUX mode is mode0.
	 * - GPMC_NADV_ALE: Default MUX mode is mode0.
	 * - GPMC_NOE: Default MUX mode is mode0.
	 * - GPMC_NWE: Default MUX mode is mode0.
	 * - GPMC_WAIT0: Default MUX mode is mode0.
	 */
	MUX_VAL(CP(GPMC_A1),		(IDIS | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_A2),		(IDIS | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_A3),		(IDIS | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_A4),		(IDIS | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_A5),		(IDIS | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_A6),		(IDIS | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_A7),		(IDIS | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_A8),		(IDIS | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_A9),		(IDIS | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_A10),		(IDIS | PTD | DIS | M0));

	MUX_VAL(CP(GPMC_D8),		(IEN  | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_D9),		(IEN  | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_D10),		(IEN  | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_D11),		(IEN  | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_D12),		(IEN  | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_D13),		(IEN  | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_D14),		(IEN  | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_D15),		(IEN  | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_CLK),		(IDIS | PTD | DIS | M0));

	MUX_VAL(CP(GPMC_NBE0_CLE),	(IDIS | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_NBE1),		(IEN  | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_NWP),		(IEN  | PTD | DIS | M0));

	MUX_VAL(CP(GPMC_WAIT1),		(IEN  | PTU | EN  | M0));

	/*
	 * Serial Interface
	 */
#if defined(CONFIG_OMAP_UART1)
	MUX_VAL(CP(UART1_TX),		(IDIS | PTD | DIS | M0));
	MUX_VAL(CP(UART1_RTS),		(IDIS | PTD | DIS | M0));
	MUX_VAL(CP(UART1_CTS),		(IEN  | PTU | DIS | M0));
	MUX_VAL(CP(UART1_RX),		(IEN  | PTD | DIS | M0));
#elif defined(CONFIG_OMAP_UART3)
	MUX_VAL(CP(UART3_CTS_RCTX),	(IEN  | PTD | EN  | M0));
	MUX_VAL(CP(UART3_RTS_SD),	(IDIS | PTD | DIS | M0));
	MUX_VAL(CP(UART3_RX_IRRX),	(IEN  | PTD | DIS | M0));
	MUX_VAL(CP(UART3_TX_IRTX),	(IDIS | PTD | DIS | M0));
#endif
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
static int omap3_evm_board_init(void)
{
	int in_sdram = running_in_sdram();

	omap3_core_init();

	mux_config();

	/* Dont reconfigure SDRAM while running in SDRAM! */
	if (!in_sdram)
		sdrc_init();

	return 0;
}

void __naked __bare_init barebox_arm_reset_vector(uint32_t *data)
{
	omap3_save_bootinfo(data);

	arm_cpu_lowlevel_init();

	omap3_evm_board_init();

	barebox_arm_entry(0x80000000, SZ_128M, 0);
}
