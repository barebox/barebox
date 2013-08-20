#include <common.h>
#include <sizes.h>
#include <io.h>
#include <init.h>
#include <asm/barebox-arm-head.h>
#include <asm/barebox-arm.h>
#include <mach/am33xx-silicon.h>
#include <mach/am33xx-clock.h>
#include <mach/generic.h>
#include <mach/sdrc.h>
#include <mach/sys_info.h>
#include <mach/syslib.h>
#include <mach/am33xx-mux.h>
#include <mach/wdt.h>

/* UART Defines */
#define UART_SYSCFG_OFFSET  (0x54)
#define UART_SYSSTS_OFFSET  (0x58)

#define UART_RESET      (0x1 << 1)
#define UART_CLK_RUNNING_MASK   0x1
#define UART_SMART_IDLE_EN  (0x1 << 0x3)

/* AM335X EMIF Register values */
#define VTP_CTRL_READY          (0x1 << 5)
#define VTP_CTRL_ENABLE         (0x1 << 6)
#define VTP_CTRL_START_EN       (0x1)
#define CMD_FORCE              0x00    /* common #def */
#define CMD_DELAY              0x00

#define EMIF_READ_LATENCY       0x06
#define EMIF_SDCFG              0x61C04832 /* CL 5, CWL 5 */
#define EMIF_SDREF              0x0000093B

#define EMIF_TIM1               0x0668A39B
#define EMIF_TIM2               0x26337FDA
#define EMIF_TIM3               0x501F830F

#define DLL_LOCK_DIFF           0x0
#define PHY_WR_DATA             0xC1
#define RD_DQS                  0x3B
#define WR_DQS                  0x85
#define PHY_FIFO_WE             0x100
#define INVERT_CLKOUT           0x1
#define PHY_RANK0_DELAY         0x01
#define DDR_IOCTRL_VALUE        0x18B
#define CTRL_SLAVE_RATIO        0x40
#define PHY_LVL_MODE            0x1
#define DDR_ZQ_CFG              0x50074BE4

static void Cmd_Macro_Config(void)
{
	writel(CTRL_SLAVE_RATIO, AM33XX_CMD0_CTRL_SLAVE_RATIO_0);
	writel(CMD_FORCE, AM33XX_CMD0_CTRL_SLAVE_FORCE_0);
	writel(CMD_DELAY, AM33XX_CMD0_CTRL_SLAVE_DELAY_0);
	writel(DLL_LOCK_DIFF, AM33XX_CMD0_DLL_LOCK_DIFF_0);
	writel(INVERT_CLKOUT, AM33XX_CMD0_INVERT_CLKOUT_0);

	writel(CTRL_SLAVE_RATIO, AM33XX_CMD1_CTRL_SLAVE_RATIO_0);
	writel(CMD_FORCE, AM33XX_CMD1_CTRL_SLAVE_FORCE_0);
	writel(CMD_DELAY, AM33XX_CMD1_CTRL_SLAVE_DELAY_0);
	writel(DLL_LOCK_DIFF, AM33XX_CMD1_DLL_LOCK_DIFF_0);
	writel(INVERT_CLKOUT, AM33XX_CMD1_INVERT_CLKOUT_0);

	writel(CTRL_SLAVE_RATIO, AM33XX_CMD2_CTRL_SLAVE_RATIO_0);
	writel(CMD_FORCE, AM33XX_CMD2_CTRL_SLAVE_FORCE_0);
	writel(CMD_DELAY, AM33XX_CMD2_CTRL_SLAVE_DELAY_0);
	writel(DLL_LOCK_DIFF, AM33XX_CMD2_DLL_LOCK_DIFF_0);
	writel(INVERT_CLKOUT, AM33XX_CMD2_INVERT_CLKOUT_0);
}

static void config_vtp(void)
{
	writel(readl(AM33XX_VTP0_CTRL_REG) | VTP_CTRL_ENABLE,
			AM33XX_VTP0_CTRL_REG);
	writel(readl(AM33XX_VTP0_CTRL_REG) & (~VTP_CTRL_START_EN),
			AM33XX_VTP0_CTRL_REG);
	writel(readl(AM33XX_VTP0_CTRL_REG) | VTP_CTRL_START_EN,
			AM33XX_VTP0_CTRL_REG);

	/* Poll for READY */
	while ((readl(AM33XX_VTP0_CTRL_REG) &
			VTP_CTRL_READY) != VTP_CTRL_READY);
}

static void phy_config_data(void)
{
	writel(RD_DQS, AM33XX_DATA0_RD_DQS_SLAVE_RATIO_0);
	writel(WR_DQS, AM33XX_DATA0_WR_DQS_SLAVE_RATIO_0);
	writel(PHY_FIFO_WE, AM33XX_DATA0_FIFO_WE_SLAVE_RATIO_0);
	writel(PHY_WR_DATA, AM33XX_DATA0_WR_DATA_SLAVE_RATIO_0);

	writel(RD_DQS, AM33XX_DATA1_RD_DQS_SLAVE_RATIO_0);
	writel(WR_DQS, AM33XX_DATA1_WR_DQS_SLAVE_RATIO_0);
	writel(PHY_FIFO_WE, AM33XX_DATA1_FIFO_WE_SLAVE_RATIO_0);
	writel(PHY_WR_DATA, AM33XX_DATA1_WR_DATA_SLAVE_RATIO_0);
}

static void config_emif(void)
{
	/*Program EMIF0 CFG Registers*/
	writel(EMIF_READ_LATENCY, AM33XX_EMIF4_0_REG(DDR_PHY_CTRL_1));
	writel(EMIF_READ_LATENCY,
				AM33XX_EMIF4_0_REG(DDR_PHY_CTRL_1_SHADOW));
	writel(EMIF_READ_LATENCY, AM33XX_EMIF4_0_REG(DDR_PHY_CTRL_2));
	writel(EMIF_TIM1, AM33XX_EMIF4_0_REG(SDRAM_TIM_1));
	writel(EMIF_TIM1, AM33XX_EMIF4_0_REG(SDRAM_TIM_1_SHADOW));
	writel(EMIF_TIM2, AM33XX_EMIF4_0_REG(SDRAM_TIM_2));
	writel(EMIF_TIM2, AM33XX_EMIF4_0_REG(SDRAM_TIM_2_SHADOW));
	writel(EMIF_TIM3, AM33XX_EMIF4_0_REG(SDRAM_TIM_3));
	writel(EMIF_TIM3, AM33XX_EMIF4_0_REG(SDRAM_TIM_3_SHADOW));

	writel(EMIF_SDREF, AM33XX_EMIF4_0_REG(SDRAM_REF_CTRL));
	writel(EMIF_SDREF, AM33XX_EMIF4_0_REG(SDRAM_REF_CTRL_SHADOW));

	writel(EMIF_SDCFG, AM33XX_EMIF4_0_REG(SDRAM_CONFIG));
	writel(DDR_ZQ_CFG, AM33XX_EMIF4_0_REG(ZQ_CONFIG));

	while ((readl(AM33XX_EMIF4_0_REG(SDRAM_STATUS)) & 0x4) != 0x4);
}

static void pcm051_config_ddr(void)
{
	enable_ddr_clocks();

	config_vtp();

	/* init mode */
	writel(PHY_LVL_MODE, AM33XX_DATA0_WRLVL_INIT_MODE_0);
	writel(PHY_LVL_MODE, AM33XX_DATA0_GATELVL_INIT_MODE_0);
	writel(PHY_LVL_MODE, AM33XX_DATA1_WRLVL_INIT_MODE_0);
	writel(PHY_LVL_MODE, AM33XX_DATA1_GATELVL_INIT_MODE_0);

	Cmd_Macro_Config();
	phy_config_data();

	writel(PHY_RANK0_DELAY, AM33XX_DATA0_RANK0_DELAYS_0);
	writel(PHY_RANK0_DELAY, AM33XX_DATA1_RANK0_DELAYS_0);

	writel(DDR_IOCTRL_VALUE, AM33XX_DDR_CMD0_IOCTRL);
	writel(DDR_IOCTRL_VALUE, AM33XX_DDR_CMD1_IOCTRL);
	writel(DDR_IOCTRL_VALUE, AM33XX_DDR_CMD2_IOCTRL);
	writel(DDR_IOCTRL_VALUE, AM33XX_DDR_DATA0_IOCTRL);
	writel(DDR_IOCTRL_VALUE, AM33XX_DDR_DATA1_IOCTRL);

	writel(readl(AM33XX_DDR_IO_CTRL) &
				0xefffffff, AM33XX_DDR_IO_CTRL);
	writel(readl(AM33XX_DDR_CKE_CTRL) |
				0x00000001, AM33XX_DDR_CKE_CTRL);

	config_emif();
}

/*
 * early system init of muxing and clocks.
 */
void pcm051_sram_init(void)
{
	u32 regVal, uart_base;

	/* Setup the PLLs and the clocks for the peripherals */
	pll_init(MPUPLL_M_600, 25);

	pcm051_config_ddr();

	/* UART softreset */
	am33xx_enable_uart0_pin_mux();
	uart_base = AM33XX_UART0_BASE;

	regVal = readl(uart_base + UART_SYSCFG_OFFSET);
	regVal |= UART_RESET;
	writel(regVal, (uart_base + UART_SYSCFG_OFFSET));
	while ((readl(uart_base + UART_SYSSTS_OFFSET) &
		UART_CLK_RUNNING_MASK) != UART_CLK_RUNNING_MASK);

	/* Disable smart idle */
	regVal = readl((uart_base + UART_SYSCFG_OFFSET));
	regVal |= UART_SMART_IDLE_EN;
	writel(regVal, (uart_base + UART_SYSCFG_OFFSET));
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
static int pcm051_board_init(void)
{
	int in_sdram = running_in_sdram();

	/* WDT1 is already running when the bootloader gets control
	 * Disable it to avoid "random" resets
	 */
	writel(WDT_DISABLE_CODE1, AM33XX_WDT_REG(WSPR));
	while (readl(AM33XX_WDT_REG(WWPS)) != 0x0);

	writel(WDT_DISABLE_CODE2, AM33XX_WDT_REG(WSPR));
	while (readl(AM33XX_WDT_REG(WWPS)) != 0x0);

	/* Dont reconfigure SDRAM while running in SDRAM! */
	if (!in_sdram)
		pcm051_sram_init();

	return 0;
}

void __naked __bare_init barebox_arm_reset_vector(uint32_t *data)
{
	omap_save_bootinfo();

	arm_cpu_lowlevel_init();

	pcm051_board_init();

	barebox_arm_entry(0x80000000, SZ_512M, 0);
}
