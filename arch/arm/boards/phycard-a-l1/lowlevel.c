#include <common.h>
#include <io.h>
#include <init.h>
#include <sizes.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <mach/omap3-mux.h>
#include <mach/generic.h>
#include <mach/sdrc.h>
#include <mach/control.h>
#include <mach/syslib.h>
#include <mach/omap3-silicon.h>
#include <mach/omap3-generic.h>
#include <mach/sys_info.h>

/* Slower full frequency range default timings for x32 operation */
#define SDP_SDRC_SHARING	0x00000100
/* Diabling power down mode using CKE pin */
#define SDP_SDRC_POWER_POP	0x00000081
/* rkw - need to find of 90/72 degree recommendation for speed like before. */
#define SDP_SDRC_DLLAB_CTRL ((DLL_ENADLL << 3) | \
	(DLL_LOCKDLL << 2) | (DLL_DLLPHASE_90 << 1))

/* used to create an array of memory configuartions. */
struct sdrc_config {
	u32	cs_cfg;
	u32	mcfg;
	u32	mr;
	u32	actim_ctrla;
	u32	actim_ctrlb;
	u32	rfr_ctrl;
} const sdrc_config[] = {
/* max cs_size for autodetection, common timing */
/* 2x256MByte, 14 Rows, 10 Columns , RBC (BAL=2) */
{ 0x00000004, 0x03590099, 0x00000032, 0x9A9DB4C6, 0x00011216, 0x0004e201},
/* MT46H32M32LF 2x128MByte, 13 Rows, 10 Columns */
{ 0x00000001, 0x02584099, 0x00000032, 0x9A9DB4C6, 0x00011216, 0x0004e201},
/* MT46H64M32LF 1x256MByte, 14 Rows, 10 Columns */
{ 0x00000002, 0x03588099, 0x00000032, 0x629DB4C6, 0x00011113, 0x0004e201},
/* MT64H128M32L2 2x256MByte, 14 Rows, 10 Columns */
{ 0x00000002, 0x03588099, 0x00000032, 0x629DB4C6, 0x00011113, 0x0004e201},
};

/*
 * Boot-time initialization(s)
 */

/*********************************************************************
 * init_sdram_ddr() - Init DDR controller.
 *********************************************************************/
void init_sdram_ddr(void)
{
	/* reset sdrc controller */
	writel(SOFTRESET, OMAP3_SDRC_REG(SYSCONFIG));
	wait_on_value(1<<0, 1<<0, OMAP3_SDRC_REG(STATUS), 12000000);
	writel(0, OMAP3_SDRC_REG(SYSCONFIG));

	/* setup sdrc to ball mux */
	writel(SDP_SDRC_SHARING, OMAP3_SDRC_REG(SHARING));
	writel(SDP_SDRC_POWER_POP, OMAP3_SDRC_REG(POWER));

	/* set up dll */
	writel(SDP_SDRC_DLLAB_CTRL, OMAP3_SDRC_REG(DLLA_CTRL));
	sdelay(0x2000);	/* give time to lock */

}
/*********************************************************************
 * config_sdram_ddr() - Init DDR on dev board.
 *********************************************************************/
void config_sdram_ddr(u8 cs, u8 cfg)
{

	writel(sdrc_config[cfg].mcfg, OMAP3_SDRC_REG(MCFG_0) + (0x30 * cs));
	writel(sdrc_config[cfg].actim_ctrla, OMAP3_SDRC_REG(ACTIM_CTRLA_0) + (0x28 * cs));
	writel(sdrc_config[cfg].actim_ctrlb, OMAP3_SDRC_REG(ACTIM_CTRLB_0) + (0x28 * cs));
	writel(sdrc_config[cfg].rfr_ctrl, OMAP3_SDRC_REG(RFR_CTRL_0) + (0x30 * cs));

	writel(CMD_NOP, OMAP3_SDRC_REG(MANUAL_0) + (0x30 * cs));

	sdelay(5000);

	writel(CMD_PRECHARGE, OMAP3_SDRC_REG(MANUAL_0) + (0x30 * cs));
	writel(CMD_AUTOREFRESH, OMAP3_SDRC_REG(MANUAL_0) + (0x30 * cs));
	writel(CMD_AUTOREFRESH, OMAP3_SDRC_REG(MANUAL_0) + (0x30 * cs));

	/* set mr0 */
	writel(sdrc_config[cfg].mr, OMAP3_SDRC_REG(MR_0) + (0x30 * cs));

	sdelay(2000);
}

/**
 * @brief Initialize the SDRC module
 * Initialisation for 1x256MByte but normally
 * done by x-loader.
 * @return void
 */
static void pcaal1_sdrc_init(void)
{
	u32 test0, test1;
	char cfg;

	init_sdram_ddr();

	config_sdram_ddr(0, 0); /* 256MByte at CS0 */
	config_sdram_ddr(1, 0); /* 256MByte at CS1 */

	test0 = get_ram_size((long *) 0x80000000, SZ_256M);
	test1 = get_ram_size((long *) 0xA0000000, SZ_256M);

	/* mask out lower nible, its not tested with
	in common/memsize.c */
	test1 &= 0xfffffff0;

	if ((test1 > 0) && (test1 != test0))
		hang();

	cfg = -1; /* illegal configuration found */

	if (test1 == 0) {
		init_sdram_ddr();
		writel((sdrc_config[(uchar) cfg].mcfg & 0xfffc00ff), OMAP3_SDRC_REG(MCFG_1));

		/* 1 x 256MByte */
		if (test0 == SZ_256M)
			cfg = 2;

		if (cfg != -1) {
			config_sdram_ddr(0, cfg);
			writel(sdrc_config[(uchar) cfg].cs_cfg, OMAP3_SDRC_REG(CS_CFG));
		}
		return;
	}

	/* reinit both cs with correct size */
	/* 2 x 128MByte */
	if (test0 == SZ_128M)
		cfg = 1;
	/* 2 x 256MByte */
	if (test0 == SZ_256M)
		cfg = 3;

	if (cfg != -1) {
		init_sdram_ddr();
		writel(sdrc_config[(uchar) cfg].cs_cfg, OMAP3_SDRC_REG(CS_CFG));
		config_sdram_ddr(0, cfg);
		config_sdram_ddr(1, cfg);
	}
}

/**
 * @brief Do the necessary pin muxing required for phyCARD-A-L1.
 * Some pins in OMAP3 do not have alternate modes.
 * We don't program these pins.
 *
 * See @ref MUX_VAL for description of the muxing mode.
 *
 * @return void
 */
static void pcaal1_mux_config(void)
{
	/*
	 * Serial Interface
	 */
	MUX_VAL(CP(UART3_CTS_RCTX),	(IEN  | PTD | EN  | M0));
	MUX_VAL(CP(UART3_RTS_SD),	(IDIS | PTD | DIS | M0));
	MUX_VAL(CP(UART3_RX_IRRX),	(IEN  | PTD | EN  | M0));
	MUX_VAL(CP(UART3_TX_IRTX),	(IDIS | PTD | DIS | M0));

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
	MUX_VAL(CP(GPMC_NADV_ALE),	(IDIS | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_NOE),		(IDIS | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_NWE),		(IDIS | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_NBE0_CLE),	(IDIS | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_NWP),		(IEN  | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_WAIT0),		(IEN  | PTU | EN  | M0));

	/* ETH_PME (GPIO_55) */
	MUX_VAL(CP(GPMC_NCS4),		(IDIS | PTU | EN  | M4));
	/* #CS5 (Ethernet) */
	MUX_VAL(CP(GPMC_NCS5),		(IDIS | PTU | EN  | M0));
	/* ETH_FIFO_SEL (GPIO_57) */
	MUX_VAL(CP(GPMC_NCS6),		(IDIS | PTD | EN  | M4));
	/* ETH_AMDIX_EN (GPIO_58) */
	MUX_VAL(CP(GPMC_NCS7),		(IDIS | PTU | EN  | M4));
	/* ETH_nRST (GPIO_64) */
	MUX_VAL(CP(GPMC_WAIT2),		(IDIS | PTU | EN  | M4));

	/* HSMMC1 */
	MUX_VAL(CP(MMC1_CLK),		(IDIS | PTU | EN  | M0));
	MUX_VAL(CP(MMC1_CMD),		(IEN  | PTU | EN  | M0));
	MUX_VAL(CP(MMC1_DAT0),		(IEN  | PTU | EN  | M0));
	MUX_VAL(CP(MMC1_DAT1),		(IEN  | PTU | EN  | M0));
	MUX_VAL(CP(MMC1_DAT2),		(IEN  | PTU | EN  | M0));
	MUX_VAL(CP(MMC1_DAT3),		(IEN  | PTU | EN  | M0));

	/* USBOTG_nRST (GPIO_63) */
	MUX_VAL(CP(GPMC_WAIT1),		(IDIS | PTU | EN  | M4));

	/* USBH_nRST (GPIO_65) */
	MUX_VAL(CP(GPMC_WAIT3),		(IDIS | PTU | EN  | M4));
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
static int pcaal1_board_init(void)
{
	int in_sdram = running_in_sdram();

	if (!in_sdram)
		omap3_core_init();

	pcaal1_mux_config();
	/* Dont reconfigure SDRAM while running in SDRAM! */
	if (!in_sdram)
		pcaal1_sdrc_init();

	return 0;
}

void __bare_init __naked barebox_arm_reset_vector(uint32_t *data)
{
	omap3_save_bootinfo(data);

	arm_cpu_lowlevel_init();

	pcaal1_board_init();

	barebox_arm_entry(0x80000000, SZ_256M, 0);
}
