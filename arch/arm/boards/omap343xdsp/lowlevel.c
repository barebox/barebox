#include <common.h>
#include <init.h>
#include <io.h>
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

/**
 * @brief Do the SDRC initialization for 128Meg Infenion DDR for CS0
 *
 * @return void
 */
static void sdrc_init(void)
{
	/* Issue SDRC Soft reset  */
	writel(0x12, OMAP3_SDRC_REG(SYSCONFIG));

	/* Wait until Reset complete */
	while ((readl(OMAP3_SDRC_REG(STATUS)) & 0x1) == 0);
	/* SDRC to normal mode */
	writel(0x10, OMAP3_SDRC_REG(SYSCONFIG));
	/* SDRC Sharing register */
	/* 32-bit SDRAM on data lane [31:0] - CS0 */
	/* pin tri-stated = 1 */
	writel(0x00000100, OMAP3_SDRC_REG(SHARING));

	/* ----- SDRC_REG(CS0 Configuration --------- */
	/* SDRC_REG(MCFG0 register */
	writel(0x02584019, OMAP3_SDRC_REG(MCFG_0));

	/* SDRC_REG(RFR_CTRL0 register */
	writel(0x0003DE01, OMAP3_SDRC_REG(RFR_CTRL_0));

	/* SDRC_REG(ACTIM_CTRLA0 register */
	writel(0X5A9A4486, OMAP3_SDRC_REG(ACTIM_CTRLA_0));

	/* SDRC_REG(ACTIM_CTRLB0 register */
	writel(0x00000010, OMAP3_SDRC_REG(ACTIM_CTRLB_0));

	/* Disble Power Down of CKE cuz of 1 CKE on combo part */
	writel(0x00000081, OMAP3_SDRC_REG(POWER));

	/* SDRC_REG(Manual command register */
	/* NOP command */
	writel(0x00000000, OMAP3_SDRC_REG(MANUAL_0));
	/* Precharge command */
	writel(0x00000001, OMAP3_SDRC_REG(MANUAL_0));
	/* Auto-refresh command */
	writel(0x00000002, OMAP3_SDRC_REG(MANUAL_0));
	/* Auto-refresh command */
	writel(0x00000002, OMAP3_SDRC_REG(MANUAL_0));

	/* SDRC MR0 register */
	/* CAS latency = 3 */
	/* Write Burst = Read Burst */
	/* Serial Mode */
	writel(0x00000032, OMAP3_SDRC_REG(MR_0));	/* Burst length =4 */

	/* SDRC DLLA control register */
	/* Enable DLL A */
	writel(0x0000000A, OMAP3_SDRC_REG(DLLA_CTRL));

	/* wait until DLL is locked  */
	while ((readl(OMAP3_SDRC_REG(DLLA_STATUS)) & 0x4) == 0);
}

/**
 * @brief Do the pin muxing required for Board operation.
 *
 * See @ref MUX_VAL for description of the muxing mode. Since some versions
 * of Linux depend on all pin muxing being done at barebox level, we may need to
 * enable CONFIG_MACH_OMAP_ADVANCED_MUX to enable the full fledged pin muxing.
 *
 * @return void
 */
static void mux_config(void)
{
	/* Essential MUX Settings */
	MUX_VAL(CP(SDRC_D0), (IEN | PTD | DIS | M0));	/* SDRC_D0 */
	MUX_VAL(CP(SDRC_D1), (IEN | PTD | DIS | M0));	/* SDRC_D1 */
	MUX_VAL(CP(SDRC_D2), (IEN | PTD | DIS | M0));	/* SDRC_D2 */
	MUX_VAL(CP(SDRC_D3), (IEN | PTD | DIS | M0));	/* SDRC_D3 */
	MUX_VAL(CP(SDRC_D4), (IEN | PTD | DIS | M0));	/* SDRC_D4 */
	MUX_VAL(CP(SDRC_D5), (IEN | PTD | DIS | M0));	/* SDRC_D5 */
	MUX_VAL(CP(SDRC_D6), (IEN | PTD | DIS | M0));	/* SDRC_D6 */
	MUX_VAL(CP(SDRC_D7), (IEN | PTD | DIS | M0));	/* SDRC_D7 */
	MUX_VAL(CP(SDRC_D8), (IEN | PTD | DIS | M0));	/* SDRC_D8 */
	MUX_VAL(CP(SDRC_D9), (IEN | PTD | DIS | M0));	/* SDRC_D9 */
	MUX_VAL(CP(SDRC_D10), (IEN | PTD | DIS | M0));	/* SDRC_D10 */
	MUX_VAL(CP(SDRC_D11), (IEN | PTD | DIS | M0));	/* SDRC_D11 */
	MUX_VAL(CP(SDRC_D12), (IEN | PTD | DIS | M0));	/* SDRC_D12 */
	MUX_VAL(CP(SDRC_D13), (IEN | PTD | DIS | M0));	/* SDRC_D13 */
	MUX_VAL(CP(SDRC_D14), (IEN | PTD | DIS | M0));	/* SDRC_D14 */
	MUX_VAL(CP(SDRC_D15), (IEN | PTD | DIS | M0));	/* SDRC_D15 */
	MUX_VAL(CP(SDRC_D16), (IEN | PTD | DIS | M0));	/* SDRC_D16 */
	MUX_VAL(CP(SDRC_D17), (IEN | PTD | DIS | M0));	/* SDRC_D17 */
	MUX_VAL(CP(SDRC_D18), (IEN | PTD | DIS | M0));	/* SDRC_D18 */
	MUX_VAL(CP(SDRC_D19), (IEN | PTD | DIS | M0));	/* SDRC_D19 */
	MUX_VAL(CP(SDRC_D20), (IEN | PTD | DIS | M0));	/* SDRC_D20 */
	MUX_VAL(CP(SDRC_D21), (IEN | PTD | DIS | M0));	/* SDRC_D21 */
	MUX_VAL(CP(SDRC_D22), (IEN | PTD | DIS | M0));	/* SDRC_D22 */
	MUX_VAL(CP(SDRC_D23), (IEN | PTD | DIS | M0));	/* SDRC_D23 */
	MUX_VAL(CP(SDRC_D24), (IEN | PTD | DIS | M0));	/* SDRC_D24 */
	MUX_VAL(CP(SDRC_D25), (IEN | PTD | DIS | M0));	/* SDRC_D25 */
	MUX_VAL(CP(SDRC_D26), (IEN | PTD | DIS | M0));	/* SDRC_D26 */
	MUX_VAL(CP(SDRC_D27), (IEN | PTD | DIS | M0));	/* SDRC_D27 */
	MUX_VAL(CP(SDRC_D28), (IEN | PTD | DIS | M0));	/* SDRC_D28 */
	MUX_VAL(CP(SDRC_D29), (IEN | PTD | DIS | M0));	/* SDRC_D29 */
	MUX_VAL(CP(SDRC_D30), (IEN | PTD | DIS | M0));	/* SDRC_D30 */
	MUX_VAL(CP(SDRC_D31), (IEN | PTD | DIS | M0));	/* SDRC_D31 */
	MUX_VAL(CP(SDRC_CLK), (IEN | PTD | DIS | M0));	/* SDRC_CLK */
	MUX_VAL(CP(SDRC_DQS0), (IEN | PTD | DIS | M0));	/* SDRC_DQS0 */
	MUX_VAL(CP(SDRC_DQS1), (IEN | PTD | DIS | M0));	/* SDRC_DQS1 */
	MUX_VAL(CP(SDRC_DQS2), (IEN | PTD | DIS | M0));	/* SDRC_DQS2 */
	MUX_VAL(CP(SDRC_DQS3), (IEN | PTD | DIS | M0));	/* SDRC_DQS3 */
	/* GPMC */
	MUX_VAL(CP(GPMC_A1), (IDIS | PTD | DIS | M0));	/* GPMC_A1 */
	MUX_VAL(CP(GPMC_A2), (IDIS | PTD | DIS | M0));	/* GPMC_A2 */
	MUX_VAL(CP(GPMC_A3), (IDIS | PTD | DIS | M0));	/* GPMC_A3 */
	MUX_VAL(CP(GPMC_A4), (IDIS | PTD | DIS | M0));	/* GPMC_A4 */
	MUX_VAL(CP(GPMC_A5), (IDIS | PTD | DIS | M0));	/* GPMC_A5 */
	MUX_VAL(CP(GPMC_A6), (IDIS | PTD | DIS | M0));	/* GPMC_A6 */
	MUX_VAL(CP(GPMC_A7), (IDIS | PTD | DIS | M0));	/* GPMC_A7 */
	MUX_VAL(CP(GPMC_A8), (IDIS | PTD | DIS | M0));	/* GPMC_A8 */
	MUX_VAL(CP(GPMC_A9), (IDIS | PTD | DIS | M0));	/* GPMC_A9 */
	MUX_VAL(CP(GPMC_A10), (IDIS | PTD | DIS | M0));	/* GPMC_A10 */
	MUX_VAL(CP(GPMC_D0), (IEN | PTD | DIS | M0));	/* GPMC_D0 */
	MUX_VAL(CP(GPMC_D1), (IEN | PTD | DIS | M0));	/* GPMC_D1 */
	MUX_VAL(CP(GPMC_D2), (IEN | PTD | DIS | M0));	/* GPMC_D2 */
	MUX_VAL(CP(GPMC_D3), (IEN | PTD | DIS | M0));	/* GPMC_D3 */
	MUX_VAL(CP(GPMC_D4), (IEN | PTD | DIS | M0));	/* GPMC_D4 */
	MUX_VAL(CP(GPMC_D5), (IEN | PTD | DIS | M0));	/* GPMC_D5 */
	MUX_VAL(CP(GPMC_D6), (IEN | PTD | DIS | M0));	/* GPMC_D6 */
	MUX_VAL(CP(GPMC_D7), (IEN | PTD | DIS | M0));	/* GPMC_D7 */
	MUX_VAL(CP(GPMC_D8), (IEN | PTD | DIS | M0));	/* GPMC_D8 */
	MUX_VAL(CP(GPMC_D9), (IEN | PTD | DIS | M0));	/* GPMC_D9 */
	MUX_VAL(CP(GPMC_D10), (IEN | PTD | DIS | M0));	/* GPMC_D10 */
	MUX_VAL(CP(GPMC_D11), (IEN | PTD | DIS | M0));	/* GPMC_D11 */
	MUX_VAL(CP(GPMC_D12), (IEN | PTD | DIS | M0));	/* GPMC_D12 */
	MUX_VAL(CP(GPMC_D13), (IEN | PTD | DIS | M0));	/* GPMC_D13 */
	MUX_VAL(CP(GPMC_D14), (IEN | PTD | DIS | M0));	/* GPMC_D14 */
	MUX_VAL(CP(GPMC_D15), (IEN | PTD | DIS | M0));	/* GPMC_D15 */
	MUX_VAL(CP(GPMC_NCS0), (IDIS | PTU | EN | M0));	/* GPMC_NCS0 */
	MUX_VAL(CP(GPMC_NCS1), (IDIS | PTU | EN | M0));	/* GPMC_NCS1 */
	MUX_VAL(CP(GPMC_NCS2), (IDIS | PTU | EN | M0));	/* GPMC_NCS2 */
	MUX_VAL(CP(GPMC_NCS3), (IDIS | PTU | EN | M0));	/* GPMC_NCS3 */
	/* GPIO_55 - FLASH_DIS */
	MUX_VAL(CP(GPMC_NCS4), (IEN | PTU | EN | M4));
	/* GPIO_56 - TORCH_EN */
	MUX_VAL(CP(GPMC_NCS5), (IDIS | PTD | DIS | M4));
	/* GPIO_57 - AGPS SLP */
	MUX_VAL(CP(GPMC_NCS6), (IEN | PTD | DIS | M4));
	/* GPMC_58 - WLAN_IRQ */
	MUX_VAL(CP(GPMC_NCS7), (IEN | PTU | EN | M4));
	MUX_VAL(CP(GPMC_CLK), (IDIS | PTD | DIS | M0));	/* GPMC_CLK */
	/* GPMC_NADV_ALE */
	MUX_VAL(CP(GPMC_NADV_ALE), (IDIS | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_NOE), (IDIS | PTD | DIS | M0));	/* GPMC_NOE */
	MUX_VAL(CP(GPMC_NWE), (IDIS | PTD | DIS | M0));	/* GPMC_NWE */
	/* GPMC_NBE0_CLE */
	MUX_VAL(CP(GPMC_NBE0_CLE), (IDIS | PTD | DIS | M0));
	MUX_VAL(CP(GPMC_NBE1), (IEN | PTD | DIS | M4));	/* GPIO_61 -BT_SHUTDN */
	MUX_VAL(CP(GPMC_NWP), (IEN | PTD | DIS | M0));	/* GPMC_NWP */
	MUX_VAL(CP(GPMC_WAIT0), (IEN | PTU | EN | M0));	/* GPMC_WAIT0 */
	MUX_VAL(CP(GPMC_WAIT1), (IEN | PTU | EN | M0));	/* GPMC_WAIT1 */
	MUX_VAL(CP(GPMC_WAIT2), (IEN | PTU | EN | M4));	/* GPIO_64 */
	MUX_VAL(CP(GPMC_WAIT3), (IEN | PTU | EN | M4));	/* GPIO_65 */

	/* SERIAL INTERFACE */
	/* UART3_CTS_RCTX */
	MUX_VAL(CP(UART3_CTS_RCTX), (IEN | PTD | EN | M0));
	/* UART3_RTS_SD */
	MUX_VAL(CP(UART3_RTS_SD), (IDIS | PTD | DIS | M0));
	/* UART3_RX_IRRX */
	MUX_VAL(CP(UART3_RX_IRRX), (IEN | PTD | DIS | M0));
	/* UART3_TX_IRTX */
	MUX_VAL(CP(UART3_TX_IRTX), (IDIS | PTD | DIS | M0));
	/* HSUSB0_CLK */
	MUX_VAL(CP(HSUSB0_CLK), (IEN | PTD | DIS | M0));
	/* HSUSB0_STP */
	MUX_VAL(CP(HSUSB0_STP), (IDIS | PTU | EN | M0));
	/* HSUSB0_DIR */
	MUX_VAL(CP(HSUSB0_DIR), (IEN | PTD | DIS | M0));
	/* HSUSB0_NXT */
	MUX_VAL(CP(HSUSB0_NXT), (IEN | PTD | DIS | M0));
	/* HSUSB0_DATA0 */
	MUX_VAL(CP(HSUSB0_DATA0), (IEN | PTD | DIS | M0));
	/* HSUSB0_DATA1 */
	MUX_VAL(CP(HSUSB0_DATA1), (IEN | PTD | DIS | M0));
	/* HSUSB0_DATA2 */
	MUX_VAL(CP(HSUSB0_DATA2), (IEN | PTD | DIS | M0));
	/* HSUSB0_DATA3 */
	MUX_VAL(CP(HSUSB0_DATA3), (IEN | PTD | DIS | M0));
	/* HSUSB0_DATA4 */
	MUX_VAL(CP(HSUSB0_DATA4), (IEN | PTD | DIS | M0));
	/* HSUSB0_DATA5 */
	MUX_VAL(CP(HSUSB0_DATA5), (IEN | PTD | DIS | M0));
	/* HSUSB0_DATA6 */
	MUX_VAL(CP(HSUSB0_DATA6), (IEN | PTD | DIS | M0));
	/* HSUSB0_DATA7 */
	MUX_VAL(CP(HSUSB0_DATA7), (IEN | PTD | DIS | M0));
	MUX_VAL(CP(I2C1_SCL), (IEN | PTU | EN | M0));	/* I2C1_SCL */
	MUX_VAL(CP(I2C1_SDA), (IEN | PTU | EN | M0));	/* I2C1_SDA */
#ifdef CONFIG_MACH_OMAP_ADVANCED_MUX
	/* DSS */
	MUX_VAL(CP(DSS_PCLK), (IDIS | PTD | DIS | M0));	/* DSS_PCLK */
	MUX_VAL(CP(DSS_HSYNC), (IDIS | PTD | DIS | M0));	/* DSS_HSYNC */
	MUX_VAL(CP(DSS_VSYNC), (IDIS | PTD | DIS | M0));	/* DSS_VSYNC */
	MUX_VAL(CP(DSS_ACBIAS), (IDIS | PTD | DIS | M0));	/* DSS_ACBIAS */
	MUX_VAL(CP(DSS_DATA0), (IDIS | PTD | DIS | M0));	/* DSS_DATA0 */
	MUX_VAL(CP(DSS_DATA1), (IDIS | PTD | DIS | M0));	/* DSS_DATA1 */
	MUX_VAL(CP(DSS_DATA2), (IDIS | PTD | DIS | M0));	/* DSS_DATA2 */
	MUX_VAL(CP(DSS_DATA3), (IDIS | PTD | DIS | M0));	/* DSS_DATA3 */
	MUX_VAL(CP(DSS_DATA4), (IDIS | PTD | DIS | M0));	/* DSS_DATA4 */
	MUX_VAL(CP(DSS_DATA5), (IDIS | PTD | DIS | M0));	/* DSS_DATA5 */
	MUX_VAL(CP(DSS_DATA6), (IDIS | PTD | DIS | M0));	/* DSS_DATA6 */
	MUX_VAL(CP(DSS_DATA7), (IDIS | PTD | DIS | M0));	/* DSS_DATA7 */
	MUX_VAL(CP(DSS_DATA8), (IDIS | PTD | DIS | M0));	/* DSS_DATA8 */
	MUX_VAL(CP(DSS_DATA9), (IDIS | PTD | DIS | M0));	/* DSS_DATA9 */
	MUX_VAL(CP(DSS_DATA10), (IDIS | PTD | DIS | M0));	/* DSS_DATA10 */
	MUX_VAL(CP(DSS_DATA11), (IDIS | PTD | DIS | M0));	/* DSS_DATA11 */
	MUX_VAL(CP(DSS_DATA12), (IDIS | PTD | DIS | M0));	/* DSS_DATA12 */
	MUX_VAL(CP(DSS_DATA13), (IDIS | PTD | DIS | M0));	/* DSS_DATA13 */
	MUX_VAL(CP(DSS_DATA14), (IDIS | PTD | DIS | M0));	/* DSS_DATA14 */
	MUX_VAL(CP(DSS_DATA15), (IDIS | PTD | DIS | M0));	/* DSS_DATA15 */
	MUX_VAL(CP(DSS_DATA16), (IDIS | PTD | DIS | M0));	/* DSS_DATA16 */
	MUX_VAL(CP(DSS_DATA17), (IDIS | PTD | DIS | M0));	/* DSS_DATA17 */
	MUX_VAL(CP(DSS_DATA18), (IDIS | PTD | DIS | M0));	/* DSS_DATA18 */
	MUX_VAL(CP(DSS_DATA19), (IDIS | PTD | DIS | M0));	/* DSS_DATA19 */
	MUX_VAL(CP(DSS_DATA20), (IDIS | PTD | DIS | M0));	/* DSS_DATA20 */
	MUX_VAL(CP(DSS_DATA21), (IDIS | PTD | DIS | M0));	/* DSS_DATA21 */
	MUX_VAL(CP(DSS_DATA22), (IDIS | PTD | DIS | M0));	/* DSS_DATA22 */
	MUX_VAL(CP(DSS_DATA23), (IDIS | PTD | DIS | M0));	/* DSS_DATA23 */
	/* CAMERA */
	MUX_VAL(CP(CAM_HS), (IEN | PTU | EN | M0));	/* CAM_HS */
	MUX_VAL(CP(CAM_VS), (IEN | PTU | EN | M0));	/* CAM_VS */
	MUX_VAL(CP(CAM_XCLKA), (IDIS | PTD | DIS | M0));	/* CAM_XCLKA */
	MUX_VAL(CP(CAM_PCLK), (IEN | PTU | EN | M0));	/* CAM_PCLK */
	/* GPIO_98 - CAM_RESET */
	MUX_VAL(CP(CAM_FLD), (IDIS | PTD | DIS | M4));
	MUX_VAL(CP(CAM_D0), (IEN | PTD | DIS | M0));	/* CAM_D0 */
	MUX_VAL(CP(CAM_D1), (IEN | PTD | DIS | M0));	/* CAM_D1 */
	MUX_VAL(CP(CAM_D2), (IEN | PTD | DIS | M0));	/* CAM_D2 */
	MUX_VAL(CP(CAM_D3), (IEN | PTD | DIS | M0));	/* CAM_D3 */
	MUX_VAL(CP(CAM_D4), (IEN | PTD | DIS | M0));	/* CAM_D4 */
	MUX_VAL(CP(CAM_D5), (IEN | PTD | DIS | M0));	/* CAM_D5 */
	MUX_VAL(CP(CAM_D6), (IEN | PTD | DIS | M0));	/* CAM_D6 */
	MUX_VAL(CP(CAM_D7), (IEN | PTD | DIS | M0));	/* CAM_D7 */
	MUX_VAL(CP(CAM_D8), (IEN | PTD | DIS | M0));	/* CAM_D8 */
	MUX_VAL(CP(CAM_D9), (IEN | PTD | DIS | M0));	/* CAM_D9 */
	MUX_VAL(CP(CAM_D10), (IEN | PTD | DIS | M0));	/* CAM_D10 */
	MUX_VAL(CP(CAM_D11), (IEN | PTD | DIS | M0));	/* CAM_D11 */
	MUX_VAL(CP(CAM_XCLKB), (IDIS | PTD | DIS | M0));	/* CAM_XCLKB */
	MUX_VAL(CP(CAM_WEN), (IEN | PTD | DIS | M4));	/* GPIO_167 */
	MUX_VAL(CP(CAM_STROBE), (IDIS | PTD | DIS | M0));	/* CAM_STROBE */
	MUX_VAL(CP(CSI2_DX0), (IEN | PTD | DIS | M0));	/* CSI2_DX0 */
	MUX_VAL(CP(CSI2_DY0), (IEN | PTD | DIS | M0));	/* CSI2_DY0 */
	MUX_VAL(CP(CSI2_DX1), (IEN | PTD | DIS | M0));	/* CSI2_DX1 */
	MUX_VAL(CP(CSI2_DY1), (IEN | PTD | DIS | M0));	/* CSI2_DY1 */
	/* AUDIO INTERFACE */
	MUX_VAL(CP(MCBSP2_FSX), (IEN | PTD | DIS | M0));	/* MCBSP2_FSX */
	/* MCBSP2_CLKX */
	MUX_VAL(CP(MCBSP2_CLKX), (IEN | PTD | DIS | M0));
	MUX_VAL(CP(MCBSP2_DR), (IEN | PTD | DIS | M0));	/* MCBSP2_DR */
	MUX_VAL(CP(MCBSP2_DX), (IDIS | PTD | DIS | M0));	/* MCBSP2_DX */
	/* EXPANSION CARD  */
	MUX_VAL(CP(MMC1_CLK), (IDIS | PTU | EN | M0));	/* MMC1_CLK */
	MUX_VAL(CP(MMC1_CMD), (IEN | PTU | EN | M0));	/* MMC1_CMD */
	MUX_VAL(CP(MMC1_DAT0), (IEN | PTU | EN | M0));	/* MMC1_DAT0 */
	MUX_VAL(CP(MMC1_DAT1), (IEN | PTU | EN | M0));	/* MMC1_DAT1 */
	MUX_VAL(CP(MMC1_DAT2), (IEN | PTU | EN | M0));	/* MMC1_DAT2 */
	MUX_VAL(CP(MMC1_DAT3), (IEN | PTU | EN | M0));	/* MMC1_DAT3 */
	MUX_VAL(CP(MMC1_DAT4), (IEN | PTU | EN | M0));	/* MMC1_DAT4 */
	MUX_VAL(CP(MMC1_DAT5), (IEN | PTU | EN | M0));	/* MMC1_DAT5 */
	MUX_VAL(CP(MMC1_DAT6), (IEN | PTU | EN | M0));	/* MMC1_DAT6 */
	MUX_VAL(CP(MMC1_DAT7), (IEN | PTU | EN | M0));	/* MMC1_DAT7 */
	/* WIRELESS LAN */
	MUX_VAL(CP(MMC2_CLK), (IEN | PTD | DIS | M0));	/* MMC2_CLK */
	MUX_VAL(CP(MMC2_CMD), (IEN | PTU | EN | M0));	/* MMC2_CMD */
	MUX_VAL(CP(MMC2_DAT0), (IEN | PTU | EN | M0));	/* MMC2_DAT0 */
	MUX_VAL(CP(MMC2_DAT1), (IEN | PTU | EN | M0));	/* MMC2_DAT1 */
	MUX_VAL(CP(MMC2_DAT2), (IEN | PTU | EN | M0));	/* MMC2_DAT2 */
	MUX_VAL(CP(MMC2_DAT3), (IEN | PTU | EN | M0));	/* MMC2_DAT3 */
	/* MMC2_DIR_DAT0 */
	MUX_VAL(CP(MMC2_DAT4), (IDIS | PTD | DIS | M1));
	/* MMC2_DIR_DAT1 */
	MUX_VAL(CP(MMC2_DAT5), (IDIS | PTD | DIS | M1));
	/* MMC2_DIR_CMD */
	MUX_VAL(CP(MMC2_DAT6), (IDIS | PTD | DIS | M1));
	/* MMC2_CLKIN */
	MUX_VAL(CP(MMC2_DAT7), (IEN | PTU | EN | M1));
	/* BLUETOOTH */
	/* MCBSP3_DX */
	MUX_VAL(CP(MCBSP3_DX), (IDIS | PTD | DIS | M0));
	/* MCBSP3_DR */
	MUX_VAL(CP(MCBSP3_DR), (IEN | PTD | DIS | M0));
	/* MCBSP3_CLKX */
	MUX_VAL(CP(MCBSP3_CLKX), (IEN | PTD | DIS | M0));
	/* MCBSP3_FSX */
	MUX_VAL(CP(MCBSP3_FSX), (IEN | PTD | DIS | M0));
	MUX_VAL(CP(UART2_CTS), (IEN | PTU | EN | M0));	/* UART2_CTS */
	MUX_VAL(CP(UART2_RTS), (IDIS | PTD | DIS | M0));	/* UART2_RTS */
	MUX_VAL(CP(UART2_TX), (IDIS | PTD | DIS | M0));	/* UART2_TX */
	MUX_VAL(CP(UART2_RX), (IEN | PTD | DIS | M0));	/* UART2_RX */
	/* MODEM INTERFACE */
	MUX_VAL(CP(UART1_TX), (IDIS | PTD | DIS | M0));	/* UART1_TX */
	MUX_VAL(CP(UART1_RTS), (IDIS | PTD | DIS | M0));	/* UART1_RTS */
	MUX_VAL(CP(UART1_CTS), (IEN | PTU | DIS | M0));	/* UART1_CTS */
	MUX_VAL(CP(UART1_RX), (IEN | PTD | DIS | M0));	/* UART1_RX */
	/* SSI1_DAT_RX */
	MUX_VAL(CP(MCBSP4_CLKX), (IEN | PTD | DIS | M1));
	MUX_VAL(CP(MCBSP4_DR), (IEN | PTD | DIS | M1));	/* SSI1_FLAG_RX */
	MUX_VAL(CP(MCBSP4_DX), (IEN | PTD | DIS | M1));	/* SSI1_RDY_RX  */
	MUX_VAL(CP(MCBSP4_FSX), (IEN | PTD | DIS | M1));	/* SSI1_WAKE */
	/* MCBSP1_CLKR  */
	MUX_VAL(CP(MCBSP1_CLKR), (IEN | PTD | DIS | M0));
	/* GPIO_157 - BT_WKUP */
	MUX_VAL(CP(MCBSP1_FSR), (IDIS | PTU | EN | M4));
	/* MCBSP1_DX */
	MUX_VAL(CP(MCBSP1_DX), (IDIS | PTD | DIS | M0));
	MUX_VAL(CP(MCBSP1_DR), (IEN | PTD | DIS | M0));	/* MCBSP1_DR */
	/* MCBSP_CLKS  */
	MUX_VAL(CP(MCBSP_CLKS), (IEN | PTU | DIS | M0));
	/* MCBSP1_FSX */
	MUX_VAL(CP(MCBSP1_FSX), (IEN | PTD | DIS | M0));
	/* MCBSP1_CLKX  */
	MUX_VAL(CP(MCBSP1_CLKX), (IEN | PTD | DIS | M0));
	/* SERIAL INTERFACE */
	MUX_VAL(CP(I2C2_SCL), (IEN | PTU | EN | M0));	/* I2C2_SCL */
	MUX_VAL(CP(I2C2_SDA), (IEN | PTU | EN | M0));	/* I2C2_SDA */
	MUX_VAL(CP(I2C3_SCL), (IEN | PTU | EN | M0));	/* I2C3_SCL */
	MUX_VAL(CP(I2C3_SDA), (IEN | PTU | EN | M0));	/* I2C3_SDA */
	MUX_VAL(CP(I2C4_SCL), (IEN | PTU | EN | M0));	/* I2C4_SCL */
	MUX_VAL(CP(I2C4_SDA), (IEN | PTU | EN | M0));	/* I2C4_SDA */
	MUX_VAL(CP(HDQ_SIO), (IEN | PTU | EN | M0));	/* HDQ_SIO */
	/* MCSPI1_CLK */
	MUX_VAL(CP(MCSPI1_CLK), (IEN | PTD | DIS | M0));
	/* MCSPI1_SIMO */
	MUX_VAL(CP(MCSPI1_SIMO), (IEN | PTD | DIS | M0));
	/* MCSPI1_SOMI */
	MUX_VAL(CP(MCSPI1_SOMI), (IEN | PTD | DIS | M0));
	/* MCSPI1_CS0 */
	MUX_VAL(CP(MCSPI1_CS0), (IEN | PTD | EN | M0));
	/* MCSPI1_CS1 */
	MUX_VAL(CP(MCSPI1_CS1), (IDIS | PTD | EN | M0));
	/* GPIO_176-NOR_DPD */
	MUX_VAL(CP(MCSPI1_CS2), (IDIS | PTD | DIS | M4));
	/* MCSPI1_CS3 */
	MUX_VAL(CP(MCSPI1_CS3), (IEN | PTD | EN | M0));
	/* MCSPI2_CLK */
	MUX_VAL(CP(MCSPI2_CLK), (IEN | PTD | DIS | M0));
	/* MCSPI2_SIMO */
	MUX_VAL(CP(MCSPI2_SIMO), (IEN | PTD | DIS | M0));
	/* MCSPI2_SOMI */
	MUX_VAL(CP(MCSPI2_SOMI), (IEN | PTD | DIS | M0));
	/* MCSPI2_CS0 */
	MUX_VAL(CP(MCSPI2_CS0), (IEN | PTD | EN | M0));
	/* MCSPI2_CS1 */
	MUX_VAL(CP(MCSPI2_CS1), (IEN | PTD | EN | M0));

	/* CONTROL AND DEBUG */
	MUX_VAL(CP(SYS_32K), (IEN | PTD | DIS | M0));	/* SYS_32K */
	MUX_VAL(CP(SYS_CLKREQ), (IEN | PTD | DIS | M0));	/* SYS_CLKREQ */
	MUX_VAL(CP(SYS_NIRQ), (IEN | PTU | EN | M0));	/* SYS_NIRQ */
	MUX_VAL(CP(SYS_BOOT0), (IEN | PTD | DIS | M4));	/* GPIO_2 - PEN_IRQ */
	MUX_VAL(CP(SYS_BOOT1), (IEN | PTD | DIS | M4));	/* GPIO_3 */
	MUX_VAL(CP(SYS_BOOT2), (IEN | PTD | DIS | M4));	/* GPIO_4 - MMC1_WP */
	MUX_VAL(CP(SYS_BOOT3), (IEN | PTD | DIS | M4));	/* GPIO_5 - LCD_ENVDD */
	MUX_VAL(CP(SYS_BOOT4), (IEN | PTD | DIS | M4));	/* GPIO_6 - LAN_INTR0 */
	MUX_VAL(CP(SYS_BOOT5), (IEN | PTD | DIS | M4));	/* GPIO_7 - MMC2_WP */
	/* GPIO_8-LCD_ENBKL */
	MUX_VAL(CP(SYS_BOOT6), (IDIS | PTD | DIS | M4));
	/* SYS_OFF_MODE */
	MUX_VAL(CP(SYS_OFF_MODE), (IEN | PTD | DIS | M0));
	/* SYS_CLKOUT1  */
	MUX_VAL(CP(SYS_CLKOUT1), (IEN | PTD | DIS | M0));
	MUX_VAL(CP(SYS_CLKOUT2), (IEN | PTU | EN | M4));	/* GPIO_186 */
	MUX_VAL(CP(JTAG_NTRST), (IEN | PTD | DIS | M0));	/* JTAG_NTRST */
	MUX_VAL(CP(JTAG_TCK), (IEN | PTD | DIS | M0));	/* JTAG_TCK */
	MUX_VAL(CP(JTAG_TMS), (IEN | PTD | DIS | M0));	/* JTAG_TMS */
	MUX_VAL(CP(JTAG_TDI), (IEN | PTD | DIS | M0));	/* JTAG_TDI */
	MUX_VAL(CP(JTAG_EMU0), (IEN | PTD | DIS | M0));	/* JTAG_EMU0 */
	MUX_VAL(CP(JTAG_EMU1), (IEN | PTD | DIS | M0));	/* JTAG_EMU1 */
	/* HSUSB1_TLL_STP */
	MUX_VAL(CP(ETK_CLK_ES2), (IDIS | PTU | EN | M0));
	/* HSUSB1_TLL_CLK */
	MUX_VAL(CP(ETK_CTL_ES2), (IDIS | PTD | DIS | M0));
	/* HSUSB1_TLL_DATA0 */
	MUX_VAL(CP(ETK_D0_ES2), (IEN | PTD | DIS | M1));
	/* MCSPI3_CS0 */
	MUX_VAL(CP(ETK_D1_ES2), (IEN | PTD | DIS | M1));
	/* HSUSB1_TLL_DATA2 */
	MUX_VAL(CP(ETK_D2_ES2), (IEN | PTD | EN | M1));
	/* HSUSB1_TLL_DATA7 */
	MUX_VAL(CP(ETK_D3_ES2), (IEN | PTD | DIS | M1));
	/* HSUSB1_TLL_DATA4 */
	MUX_VAL(CP(ETK_D4_ES2), (IEN | PTD | DIS | M0));
	/* HSUSB1_TLL_DATA5 */
	MUX_VAL(CP(ETK_D5_ES2), (IEN | PTD | DIS | M0));
	/* HSUSB1_TLL_DATA6 */
	MUX_VAL(CP(ETK_D6_ES2), (IEN | PTD | DIS | M0));
	/* HSUSB1_TLL_DATA3 */
	MUX_VAL(CP(ETK_D7_ES2), (IEN | PTD | DIS | M0));
	/* HSUSB1_TLL_DIR */
	MUX_VAL(CP(ETK_D8_ES2), (IEN | PTD | DIS | M0));
	/* HSUSB1_TLL_NXT */
	MUX_VAL(CP(ETK_D9_ES2), (IEN | PTD | DIS | M0));
	/* HSUSB2_TLL_CLK */
	MUX_VAL(CP(ETK_D10_ES2), (IEN | PTD | DIS | M0));
	/* HSUSB2_TLL_STP */
	MUX_VAL(CP(ETK_D11_ES2), (IEN | PTD | DIS | M0));
	/* HSUSB2_TLL_DIR */
	MUX_VAL(CP(ETK_D12_ES2), (IEN | PTD | DIS | M0));
	/* HSUSB2_TLL_NXT */
	MUX_VAL(CP(ETK_D13_ES2), (IEN | PTD | DIS | M0));
	/* HSUSB2_TLL_DATA0 */
	MUX_VAL(CP(ETK_D14_ES2), (IEN | PTD | DIS | M0));
	/* HSUSB2_TLL_DATA1 */
	MUX_VAL(CP(ETK_D15_ES2), (IEN | PTD | DIS | M0));

	/* DIE TO DIE */
	MUX_VAL(CP(D2D_MCAD0), (IEN | PTD | EN | M0));	/* D2D_MCAD0 */
	MUX_VAL(CP(D2D_MCAD1), (IEN | PTD | EN | M0));	/* D2D_MCAD1 */
	MUX_VAL(CP(D2D_MCAD2), (IEN | PTD | EN | M0));	/* D2D_MCAD2 */
	MUX_VAL(CP(D2D_MCAD3), (IEN | PTD | EN | M0));	/* D2D_MCAD3 */
	MUX_VAL(CP(D2D_MCAD4), (IEN | PTD | EN | M0));	/* D2D_MCAD4 */
	MUX_VAL(CP(D2D_MCAD5), (IEN | PTD | EN | M0));	/* D2D_MCAD5 */
	MUX_VAL(CP(D2D_MCAD6), (IEN | PTD | EN | M0));	/* D2D_MCAD6 */
	MUX_VAL(CP(D2D_MCAD7), (IEN | PTD | EN | M0));	/* D2D_MCAD7 */
	MUX_VAL(CP(D2D_MCAD8), (IEN | PTD | EN | M0));	/* D2D_MCAD8 */
	MUX_VAL(CP(D2D_MCAD9), (IEN | PTD | EN | M0));	/* D2D_MCAD9 */
	MUX_VAL(CP(D2D_MCAD10), (IEN | PTD | EN | M0));	/* D2D_MCAD10 */
	MUX_VAL(CP(D2D_MCAD11), (IEN | PTD | EN | M0));	/* D2D_MCAD11 */
	MUX_VAL(CP(D2D_MCAD12), (IEN | PTD | EN | M0));	/* D2D_MCAD12 */
	MUX_VAL(CP(D2D_MCAD13), (IEN | PTD | EN | M0));	/* D2D_MCAD13 */
	MUX_VAL(CP(D2D_MCAD14), (IEN | PTD | EN | M0));	/* D2D_MCAD14 */
	MUX_VAL(CP(D2D_MCAD15), (IEN | PTD | EN | M0));	/* D2D_MCAD15 */
	MUX_VAL(CP(D2D_MCAD16), (IEN | PTD | EN | M0));	/* D2D_MCAD16 */
	MUX_VAL(CP(D2D_MCAD17), (IEN | PTD | EN | M0));	/* D2D_MCAD17 */
	MUX_VAL(CP(D2D_MCAD18), (IEN | PTD | EN | M0));	/* D2D_MCAD18 */
	MUX_VAL(CP(D2D_MCAD19), (IEN | PTD | EN | M0));	/* D2D_MCAD19 */
	MUX_VAL(CP(D2D_MCAD20), (IEN | PTD | EN | M0));	/* D2D_MCAD20 */
	MUX_VAL(CP(D2D_MCAD21), (IEN | PTD | EN | M0));	/* D2D_MCAD21 */
	MUX_VAL(CP(D2D_MCAD22), (IEN | PTD | EN | M0));	/* D2D_MCAD22 */
	MUX_VAL(CP(D2D_MCAD23), (IEN | PTD | EN | M0));	/* D2D_MCAD23 */
	MUX_VAL(CP(D2D_MCAD24), (IEN | PTD | EN | M0));	/* D2D_MCAD24 */
	MUX_VAL(CP(D2D_MCAD25), (IEN | PTD | EN | M0));	/* D2D_MCAD25 */
	MUX_VAL(CP(D2D_MCAD26), (IEN | PTD | EN | M0));	/* D2D_MCAD26 */
	MUX_VAL(CP(D2D_MCAD27), (IEN | PTD | EN | M0));	/* D2D_MCAD27 */
	MUX_VAL(CP(D2D_MCAD28), (IEN | PTD | EN | M0));	/* D2D_MCAD28 */
	MUX_VAL(CP(D2D_MCAD29), (IEN | PTD | EN | M0));	/* D2D_MCAD29 */
	MUX_VAL(CP(D2D_MCAD30), (IEN | PTD | EN | M0));	/* D2D_MCAD30 */
	MUX_VAL(CP(D2D_MCAD31), (IEN | PTD | EN | M0));	/* D2D_MCAD31 */
	MUX_VAL(CP(D2D_MCAD32), (IEN | PTD | EN | M0));	/* D2D_MCAD32 */
	MUX_VAL(CP(D2D_MCAD33), (IEN | PTD | EN | M0));	/* D2D_MCAD33 */
	MUX_VAL(CP(D2D_MCAD34), (IEN | PTD | EN | M0));	/* D2D_MCAD34 */
	MUX_VAL(CP(D2D_MCAD35), (IEN | PTD | EN | M0));	/* D2D_MCAD35 */
	MUX_VAL(CP(D2D_MCAD36), (IEN | PTD | EN | M0));	/* D2D_MCAD36 */
	/* D2D_CLK26MI  */
	MUX_VAL(CP(D2D_CLK26MI), (IEN | PTD | DIS | M0));
	/* D2D_NRESPWRON */
	MUX_VAL(CP(D2D_NRESPWRON), (IEN | PTD | EN | M0));
	/* D2D_NRESWARM */
	MUX_VAL(CP(D2D_NRESWARM), (IEN | PTU | EN | M0));
	/* D2D_ARM9NIRQ */
	MUX_VAL(CP(D2D_ARM9NIRQ), (IEN | PTD | DIS | M0));
	/* D2D_UMA2P6FIQ */
	MUX_VAL(CP(D2D_UMA2P6FIQ), (IEN | PTD | DIS | M0));
	/* D2D_SPINT */
	MUX_VAL(CP(D2D_SPINT), (IEN | PTD | EN | M0));
	/* D2D_FRINT */
	MUX_VAL(CP(D2D_FRINT), (IEN | PTD | EN | M0));
	/* D2D_DMAREQ0  */
	MUX_VAL(CP(D2D_DMAREQ0), (IEN | PTD | DIS | M0));
	/* D2D_DMAREQ1  */
	MUX_VAL(CP(D2D_DMAREQ1), (IEN | PTD | DIS | M0));
	/* D2D_DMAREQ2  */
	MUX_VAL(CP(D2D_DMAREQ2), (IEN | PTD | DIS | M0));
	/* D2D_DMAREQ3  */
	MUX_VAL(CP(D2D_DMAREQ3), (IEN | PTD | DIS | M0));
	/* D2D_N3GTRST  */
	MUX_VAL(CP(D2D_N3GTRST), (IEN | PTD | DIS | M0));
	/* D2D_N3GTDI */
	MUX_VAL(CP(D2D_N3GTDI), (IEN | PTD | DIS | M0));
	/* D2D_N3GTDO */
	MUX_VAL(CP(D2D_N3GTDO), (IEN | PTD | DIS | M0));
	/* D2D_N3GTMS */
	MUX_VAL(CP(D2D_N3GTMS), (IEN | PTD | DIS | M0));
	/* D2D_N3GTCK */
	MUX_VAL(CP(D2D_N3GTCK), (IEN | PTD | DIS | M0));
	/* D2D_N3GRTCK  */
	MUX_VAL(CP(D2D_N3GRTCK), (IEN | PTD | DIS | M0));
	/* D2D_MSTDBY */
	MUX_VAL(CP(D2D_MSTDBY), (IEN | PTU | EN | M0));
	/* D2D_SWAKEUP */
	MUX_VAL(CP(D2D_SWAKEUP), (IEN | PTD | EN | M0));
	/* D2D_IDLEREQ */
	MUX_VAL(CP(D2D_IDLEREQ), (IEN | PTD | DIS | M0));
	/* D2D_IDLEACK */
	MUX_VAL(CP(D2D_IDLEACK), (IEN | PTU | EN | M0));
	/* D2D_MWRITE */
	MUX_VAL(CP(D2D_MWRITE), (IEN | PTD | DIS | M0));
	/* D2D_SWRITE */
	MUX_VAL(CP(D2D_SWRITE), (IEN | PTD | DIS | M0));
	/* D2D_MREAD */
	MUX_VAL(CP(D2D_MREAD), (IEN | PTD | DIS | M0));
	/* D2D_SREAD */
	MUX_VAL(CP(D2D_SREAD), (IEN | PTD | DIS | M0));
	/* D2D_MBUSFLAG */
	MUX_VAL(CP(D2D_MBUSFLAG), (IEN | PTD | DIS | M0));
	/* D2D_SBUSFLAG */
	MUX_VAL(CP(D2D_SBUSFLAG), (IEN | PTD | DIS | M0));
	/* SDRC_CKE0 */
	MUX_VAL(CP(SDRC_CKE0), (IDIS | PTU | EN | M0));
	/* SDRC_CKE1 NOT USED */
	MUX_VAL(CP(SDRC_CKE1), (IDIS | PTD | DIS | M7));
#endif				/* CONFIG_MACH_OMAP_ADVANCED_MUX */
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
static int sdp343x_board_init(void)
{
	int in_sdram = running_in_sdram();

	if (!in_sdram)
		omap3_core_init();

	mux_config();
	if (!in_sdram)
		sdrc_init();

	return 0;
}

void __naked __bare_init barebox_arm_reset_vector(uint32_t *data)
{
	omap3_save_bootinfo(data);

	arm_cpu_lowlevel_init();

	sdp343x_board_init();

	barebox_arm_entry(0x80000000, SZ_128M, 0);
}
