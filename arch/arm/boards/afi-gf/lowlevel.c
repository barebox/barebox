#include <init.h>
#include <linux/sizes.h>
#include <io.h>
#include <asm/armlinux.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <linux/bitops.h>
#include <mach/am33xx-generic.h>
#include <mach/am33xx-silicon.h>
#include <mach/am33xx-clock.h>
#include <mach/sdrc.h>
#include <mach/sys_info.h>
#include <mach/syslib.h>
#include <mach/am33xx-mux.h>
#include <mach/wdt.h>
#include <debug_ll.h>

/* AM335X EMIF Register values */
#define	VTP_CTRL_READY		(0x1 << 5)
#define VTP_CTRL_ENABLE		(0x1 << 6)
#define VTP_CTRL_LOCK_EN	(0x1 << 4)
#define VTP_CTRL_START_EN	(0x1)
#define DDR2_RATIO		0x80	/* for mDDR */		//
#define CMD_FORCE		0x00	/* common #def */
#define CMD_DELAY		0x00

#define EMIF_READ_LATENCY	0x05				// (CL + 2 - 1)
#define EMIF_TIM1		0x04447289			//
#define EMIF_TIM2		0x10160580			//
#define	EMIF_TIM3		0x000000E7			// check tREFI
#define EMIF_SDCFG		0x0 \
	| (1 << 29) /* reg_sdram_type = 1 (LPDDR1) */ \
	| (0 << 27) /* reg_ibank_pos = 0 */ \
	| (0 << 24) /* reg_ddr_term = 0? */ \
	| (0 << 23) /* reg_ddr2_ddqs = 0 (single ended DQS for LPDDR) */ \
	| (0 << 21) /* reg_dyn_odt = 0 (only used for DDR3) */ \
	| (0 << 20) /* reg_ddr_disable_dll = 0 */ \
	| (1 << 18) /* reg_sdram_drive = 1? (1/2 for LPDDR) */ \
	| (0 << 16) /* reg_cwl = 0 (only for DDR3) */ \
	| (1 << 14) /* reg_narrow_mode = 1 (AM335x is always 16 bit) */ \
	| (3 << 10) /* reg_cl = 3 */ \
	| (5 << 7) /* reg_rowsize = 5 (unused?, 14 row bits)  */ \
	| (2 << 4) /* reg_ibank = 2 (4 banks) */ \
	| (0 << 3) /* reg_ebank = 0 */ \
	| (3 << 0) /* reg_pagesize = 3? (11 column bits = 2048 words) */
#define EMIF_SDREF		0x00000618			//
#define DDR2_DLL_LOCK_DIFF	0x0
#define DDR2_RD_DQS		0x40				//
#define	DDR2_WR_DQS		0x2				//
#define DDR2_PHY_WRLVL          0x00
#define DDR2_PHY_GATELVL        0x00
#define DDR2_PHY_FIFO_WE	0x110				//
#define	DDR2_PHY_WR_DATA	0x40				//

#define	DDR2_INVERT_CLKOUT	0x0				//
#define	PHY_RANK0_DELAY		0x1				//
#define PHY_DLL_LOCK_DIFF	0x0
#define DDR_IOCTRL_VALUE	0x18B				//

static void board_data_macro_config(int dataMacroNum)
{
	u32 BaseAddrOffset = 0x00;

	if (dataMacroNum == 1)
		BaseAddrOffset = 0xA4;

	__raw_writel(DDR2_RD_DQS,
		(AM33XX_DATA0_RD_DQS_SLAVE_RATIO_0 + BaseAddrOffset));
	__raw_writel(DDR2_RD_DQS>>2,
		(AM33XX_DATA0_RD_DQS_SLAVE_RATIO_1 + BaseAddrOffset));
	__raw_writel(DDR2_WR_DQS,
		(AM33XX_DATA0_WR_DQS_SLAVE_RATIO_0 + BaseAddrOffset));
	__raw_writel(DDR2_WR_DQS>>2,
		(AM33XX_DATA0_WR_DQS_SLAVE_RATIO_1 + BaseAddrOffset));
	__raw_writel(DDR2_PHY_WRLVL,
		(AM33XX_DATA0_WRLVL_INIT_RATIO_0 + BaseAddrOffset));
	__raw_writel(DDR2_PHY_WRLVL>>2,
		(AM33XX_DATA0_WRLVL_INIT_RATIO_1 + BaseAddrOffset));
	__raw_writel(DDR2_PHY_GATELVL,
		(AM33XX_DATA0_GATELVL_INIT_RATIO_0 + BaseAddrOffset));
	__raw_writel(DDR2_PHY_GATELVL>>2,
		(AM33XX_DATA0_GATELVL_INIT_RATIO_1 + BaseAddrOffset));
	__raw_writel(DDR2_PHY_FIFO_WE,
		(AM33XX_DATA0_FIFO_WE_SLAVE_RATIO_0 + BaseAddrOffset));
	__raw_writel(DDR2_PHY_FIFO_WE>>2,
		(AM33XX_DATA0_FIFO_WE_SLAVE_RATIO_1 + BaseAddrOffset));
	__raw_writel(DDR2_PHY_WR_DATA,
		(AM33XX_DATA0_WR_DATA_SLAVE_RATIO_0 + BaseAddrOffset));
	__raw_writel(DDR2_PHY_WR_DATA>>2,
		(AM33XX_DATA0_WR_DATA_SLAVE_RATIO_1 + BaseAddrOffset));
	__raw_writel(PHY_DLL_LOCK_DIFF,
		(AM33XX_DATA0_DLL_LOCK_DIFF_0 + BaseAddrOffset));
}

static void board_cmd_macro_config(void)
{
	__raw_writel(DDR2_RATIO, AM33XX_CMD0_CTRL_SLAVE_RATIO_0);
	__raw_writel(CMD_FORCE, AM33XX_CMD0_CTRL_SLAVE_FORCE_0);
	__raw_writel(CMD_DELAY, AM33XX_CMD0_CTRL_SLAVE_DELAY_0);
	__raw_writel(DDR2_DLL_LOCK_DIFF, AM33XX_CMD0_DLL_LOCK_DIFF_0);
	__raw_writel(DDR2_INVERT_CLKOUT, AM33XX_CMD0_INVERT_CLKOUT_0);

	__raw_writel(DDR2_RATIO, AM33XX_CMD1_CTRL_SLAVE_RATIO_0);
	__raw_writel(CMD_FORCE, AM33XX_CMD1_CTRL_SLAVE_FORCE_0);
	__raw_writel(CMD_DELAY, AM33XX_CMD1_CTRL_SLAVE_DELAY_0);
	__raw_writel(DDR2_DLL_LOCK_DIFF, AM33XX_CMD1_DLL_LOCK_DIFF_0);
	__raw_writel(DDR2_INVERT_CLKOUT, AM33XX_CMD1_INVERT_CLKOUT_0);

	__raw_writel(DDR2_RATIO, AM33XX_CMD2_CTRL_SLAVE_RATIO_0);
	__raw_writel(CMD_FORCE, AM33XX_CMD2_CTRL_SLAVE_FORCE_0);
	__raw_writel(CMD_DELAY, AM33XX_CMD2_CTRL_SLAVE_DELAY_0);
	__raw_writel(DDR2_DLL_LOCK_DIFF, AM33XX_CMD2_DLL_LOCK_DIFF_0);
	__raw_writel(DDR2_INVERT_CLKOUT, AM33XX_CMD2_INVERT_CLKOUT_0);
}

static void board_config_vtp(void)
{
	__raw_writel(__raw_readl(AM33XX_VTP0_CTRL_REG) | VTP_CTRL_ENABLE,
		AM33XX_VTP0_CTRL_REG);
	__raw_writel(__raw_readl(AM33XX_VTP0_CTRL_REG) & (~VTP_CTRL_START_EN),
		AM33XX_VTP0_CTRL_REG);
	__raw_writel(__raw_readl(AM33XX_VTP0_CTRL_REG) | VTP_CTRL_START_EN,
		AM33XX_VTP0_CTRL_REG);

	/* Poll for READY */
	while ((__raw_readl(AM33XX_VTP0_CTRL_REG) & VTP_CTRL_READY) != VTP_CTRL_READY);
}

static void board_config_emif_ddr(void)
{
	u32 i;

	/*Program EMIF0 CFG Registers*/
	__raw_writel(EMIF_READ_LATENCY, AM33XX_EMIF4_0_REG(DDR_PHY_CTRL_1));
	__raw_writel(EMIF_READ_LATENCY, AM33XX_EMIF4_0_REG(DDR_PHY_CTRL_1_SHADOW));
	__raw_writel(EMIF_READ_LATENCY, AM33XX_EMIF4_0_REG(DDR_PHY_CTRL_2));
	__raw_writel(EMIF_TIM1, AM33XX_EMIF4_0_REG(SDRAM_TIM_1));
	__raw_writel(EMIF_TIM1, AM33XX_EMIF4_0_REG(SDRAM_TIM_1_SHADOW));
	__raw_writel(EMIF_TIM2, AM33XX_EMIF4_0_REG(SDRAM_TIM_2));
	__raw_writel(EMIF_TIM2, AM33XX_EMIF4_0_REG(SDRAM_TIM_2_SHADOW));
	__raw_writel(EMIF_TIM3, AM33XX_EMIF4_0_REG(SDRAM_TIM_3));
	__raw_writel(EMIF_TIM3, AM33XX_EMIF4_0_REG(SDRAM_TIM_3_SHADOW));

	__raw_writel(EMIF_SDCFG, AM33XX_EMIF4_0_REG(SDRAM_CONFIG));
	__raw_writel(EMIF_SDCFG, AM33XX_EMIF4_0_REG(SDRAM_CONFIG2));

	__raw_writel(0x00004650, AM33XX_EMIF4_0_REG(SDRAM_REF_CTRL));
	__raw_writel(0x00004650, AM33XX_EMIF4_0_REG(SDRAM_REF_CTRL_SHADOW));

	for (i = 0; i < 5000; i++) {

	}

	__raw_writel(EMIF_SDREF, AM33XX_EMIF4_0_REG(SDRAM_REF_CTRL));
	__raw_writel(EMIF_SDREF, AM33XX_EMIF4_0_REG(SDRAM_REF_CTRL_SHADOW));

	__raw_writel(EMIF_SDCFG, AM33XX_EMIF4_0_REG(SDRAM_CONFIG));
	__raw_writel(EMIF_SDCFG, AM33XX_EMIF4_0_REG(SDRAM_CONFIG2));
}

static void board_config_ddr(void)
{
	am33xx_enable_ddr_clocks();

	board_config_vtp();

	board_cmd_macro_config();
	board_data_macro_config(0);
	board_data_macro_config(1);

	__raw_writel(PHY_RANK0_DELAY, AM33XX_DATA0_RANK0_DELAYS_0);
	__raw_writel(PHY_RANK0_DELAY, AM33XX_DATA1_RANK0_DELAYS_0);

	__raw_writel(DDR_IOCTRL_VALUE, AM33XX_DDR_CMD0_IOCTRL);
	__raw_writel(DDR_IOCTRL_VALUE, AM33XX_DDR_CMD1_IOCTRL);
	__raw_writel(DDR_IOCTRL_VALUE, AM33XX_DDR_CMD2_IOCTRL);
	__raw_writel(DDR_IOCTRL_VALUE, AM33XX_DDR_DATA0_IOCTRL);
	__raw_writel(DDR_IOCTRL_VALUE, AM33XX_DDR_DATA1_IOCTRL);

	__raw_writel(__raw_readl(AM33XX_DDR_IO_CTRL) | BIT(28), AM33XX_DDR_IO_CTRL);
	__raw_writel(__raw_readl(AM33XX_DDR_CKE_CTRL) | BIT(0), AM33XX_DDR_CKE_CTRL);

	board_config_emif_ddr();
}

static const struct module_pin_mux board_can_pin_mux[] = {
	{OFFSET(mii1_txd3), (MODE(1) | PULLUP_EN)}, /* dcan0_tx_mux0 */
	{OFFSET(mii1_txd2), (MODE(1) | RXACTIVE | PULLUP_EN)}, /* dcan0_rx_mux0 */
	{OFFSET(mcasp0_aclkr), (MODE(7) | PULLUP_EN)}, /* gpio3[18] / DCAN0_LBK */
	{-1},
};

extern char __dtb_z_am335x_afi_gf_start[];

/**
 * @brief The basic entry point for board initialization.
 *
 * This is called as part of machine init (after arch init).
 * This is again called with stack in SRAM, so not too many
 * constructs possible here.
 *
 * @return void
 */
static noinline int gf_sram_init(void)
{
	void *fdt;

	fdt = __dtb_z_am335x_afi_gf_start;

	/* WDT1 is already running when the bootloader gets control
	 * Disable it to avoid "random" resets
	 */
	__raw_writel(WDT_DISABLE_CODE1, AM33XX_WDT_REG(WSPR));
	while(__raw_readl(AM33XX_WDT_REG(WWPS)) != 0x0);
	__raw_writel(WDT_DISABLE_CODE2, AM33XX_WDT_REG(WSPR));
	while(__raw_readl(AM33XX_WDT_REG(WWPS)) != 0x0);

	/* Setup the PLLs and the clocks for the peripherals */
	am33xx_pll_init(MPUPLL_M_500, DDRPLL_M_200);

	board_config_ddr();

	/*
	 * FIXME configure CAN pinmux early to avoid driving the bus
	 * with the low by default pins.
	 */
	configure_module_pin_mux(board_can_pin_mux);

	am33xx_uart_soft_reset((void *)AM33XX_UART2_BASE);
	am33xx_enable_uart2_pin_mux();
	omap_uart_lowlevel_init((void *)AM33XX_UART2_BASE);
	putc_ll('>');

	barebox_arm_entry(0x80000000, SZ_256M, fdt);
}

ENTRY_FUNCTION(start_am33xx_afi_gf_sram, bootinfo, r1, r2)
{
	am33xx_save_bootinfo((void *)bootinfo);

	/*
	 * Setup C environment, the board init code uses global variables.
	 * Stackpointer has already been initialized by the ROM code.
	 */
	relocate_to_current_adr();
	setup_c();

	gf_sram_init();
}

ENTRY_FUNCTION(start_am33xx_afi_gf_sdram, r0, r1, r2)
{
	void *fdt;

	fdt = __dtb_z_am335x_afi_gf_start + get_runtime_offset();

	putc_ll('>');

	barebox_arm_entry(0x80000000, SZ_256M, fdt);
}
