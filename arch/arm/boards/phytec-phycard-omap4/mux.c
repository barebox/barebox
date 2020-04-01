#include <common.h>
#include <init.h>
#include <io.h>
#include <mach/omap4-silicon.h>
#include <mach/omap4-mux.h>
#include <mach/omap4-clock.h>

#include "mux.h"

static const struct pad_conf_entry core_padconf_array[] = {
	{GPMC_AD0, (IEN | PTD | DIS | M0)},				/* gpmc_ad0 */
	{GPMC_AD1, (IEN | PTD | DIS | M0)},			 	/* gpmc_ad1 */
	{GPMC_AD2, (IEN | PTD | DIS | M0)},				/* gpmc_ad2 */
	{GPMC_AD3, (IEN | PTD | DIS | M0)},				/* gpmc_ad3 */
	{GPMC_AD4, (IEN | PTD | DIS | M0)},				/* gpmc_ad4 */
	{GPMC_AD5, (IEN | PTD | DIS | M0)},				/* gpmc_ad5 */
	{GPMC_AD6, (IEN | PTD | DIS | M0)},				/* gpmc_ad6 */
	{GPMC_AD7, (IEN | PTD | DIS | M0)},				/* gpmc_ad7 */
	{GPMC_AD8, (IEN | PTD | DIS | M0)},				/* gpmc_ad8 */
	{GPMC_AD9, (IEN | PTD | DIS | M0)},				/* gpmc_ad9 */
	{GPMC_AD10, (IEN | PTD | DIS | M0)},				/* gpmc_ad10 */
	{GPMC_AD11, (IEN | PTD | DIS | M0)},				/* gpmc_ad11 */
	{GPMC_AD12, (IEN | PTD | DIS | M0)},				/* gpmc_ad12 */
	{GPMC_AD13, (IEN | PTD | DIS | M0)},				/* gpmc_ad13 */
	{GPMC_AD14, (IEN | PTD | DIS | M0)},				/* gpmc_ad14 */
	{GPMC_AD15, (IEN | PTD | DIS | M0)},				/* gpmc_ad15 */
	{GPMC_A16, (IEN | PTD | DIS | M0)},				/* gpmc_a16 */
	{GPMC_A17, (SAFE_MODE)},					/* nc */
	{GPMC_A18, (SAFE_MODE)},					/* nc */
	{GPMC_A19, (SAFE_MODE)},					/* nc */
	{GPMC_A20, (SAFE_MODE)},					/* nc */
	{GPMC_A21, (SAFE_MODE)},					/* nc */
	{GPMC_A22, (SAFE_MODE)},					/* nc */
	{GPMC_A23, (SAFE_MODE)},					/* nc */
	{GPMC_A24, (SAFE_MODE)},					/* nc */
	{GPMC_A25, (SAFE_MODE)},					/* nc */
	{GPMC_NCS0, (IDIS | PTU | EN | M0)},				/* gpmc_nsc0 */
	{GPMC_NCS1, (IDIS | PTU | EN | M0)},				/* gpmc_nsc1 */
	{GPMC_NCS2, (SAFE_MODE)},					/* nc */
	{GPMC_NCS3, (SAFE_MODE)},					/* nc */
	{GPMC_NWP, (IEN | PTD | DIS | M0)},				/* gpmc_nwp */
	{GPMC_CLK, (PTU | IEN | M3)},					/* gpio_55 */
	{GPMC_NADV_ALE, (IDIS | PTD | DIS | M0)},			/* gpmc_ndav_ale */
	{GPMC_NOE, (IDIS | PTD | DIS | M0)},				/* gpmc_noe */
	{GPMC_NWE, (IDIS | PTD | DIS | M0)},				/* gpmc_nwe */
	{GPMC_NBE0_CLE, (IDIS | PTD | DIS | M0)},			/* gpmc_nbe0_cle */
	{GPMC_NBE1, (SAFE_MODE)},					/* nc */
	{GPMC_WAIT0, (IEN | PTU | EN | M0)},				/* gpmc_wait0 */
	{GPMC_WAIT1, (IEN | PTU | EN | M0)},				/* gpmc_wait1 */
	{C2C_DATA11, (SAFE_MODE)},					/* nc */
	{C2C_DATA12, (SAFE_MODE)},					/* nc */
	{C2C_DATA13, (IDIS | PTU | EN | M0)},				/* gpmc_nsc5 */
	{C2C_DATA14, (SAFE_MODE)},					/* nc */
	{C2C_DATA15, (SAFE_MODE)},					/* nc */
	{HDMI_HPD, (SAFE_MODE)},					/* nc */
	{HDMI_CEC, (SAFE_MODE)},					/* nc */
	{HDMI_DDC_SCL, (SAFE_MODE)},					/* nc */
	{HDMI_DDC_SDA, (SAFE_MODE)},					/* nc */
	{CSI21_DX0, (SAFE_MODE)},					/* nc */
	{CSI21_DY0, (SAFE_MODE)},					/* nc */
	{CSI21_DX1, (SAFE_MODE)},					/* nc */
	{CSI21_DY1, (SAFE_MODE)},					/* nc */
	{CSI21_DX2, (SAFE_MODE)},					/* nc */
	{CSI21_DY2, (SAFE_MODE)},					/* nc */
	{CSI21_DX3, (SAFE_MODE)},					/* nc */
	{CSI21_DY3, (SAFE_MODE)},					/* nc */
	{CSI21_DX4, (SAFE_MODE)},					/* nc */
	{CSI21_DY4, (SAFE_MODE)},					/* nc */
	{CSI22_DX0, (SAFE_MODE)},					/* nc */
	{CSI22_DY0, (SAFE_MODE)},					/* nc */
	{CSI22_DX1, (SAFE_MODE)},					/* nc */
	{CSI22_DY1, (SAFE_MODE)},					/* nc */
	{CAM_SHUTTER, (SAFE_MODE)},					/* unused */
	{CAM_STROBE, (SAFE_MODE)},					/* unused */
	{CAM_GLOBALRESET, (SAFE_MODE)},					/* unused */
	{USBB1_ULPITLL_CLK, (PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M4)},/* usbb1_ulpiphy_clk */
	{USBB1_ULPITLL_STP, (OFF_EN | OFF_OUT_PTD | M4)},		/* usbb1_ulpiphy_stp */
	{USBB1_ULPITLL_DIR, (IEN | OFF_EN | OFF_PD | OFF_IN | M4)},	/* usbb1_ulpiphy_dir */
	{USBB1_ULPITLL_NXT, (IEN | OFF_EN | OFF_PD | OFF_IN | M4)},	/* usbb1_ulpiphy_nxt */
	{USBB1_ULPITLL_DAT0, (IEN | OFF_EN | OFF_PD | OFF_IN | M4)},	/* usbb1_ulpiphy_dat0 */
	{USBB1_ULPITLL_DAT1, (IEN | OFF_EN | OFF_PD | OFF_IN | M4)},	/* usbb1_ulpiphy_dat1 */
	{USBB1_ULPITLL_DAT2, (IEN | OFF_EN | OFF_PD | OFF_IN | M4)},	/* usbb1_ulpiphy_dat2 */
	{USBB1_ULPITLL_DAT3, (IEN | OFF_EN | OFF_PD | OFF_IN | M4)},	/* usbb1_ulpiphy_dat3 */
	{USBB1_ULPITLL_DAT4, (IEN | OFF_EN | OFF_PD | OFF_IN | M4)},	/* usbb1_ulpiphy_dat4 */
	{USBB1_ULPITLL_DAT5, (IEN | OFF_EN | OFF_PD | OFF_IN | M4)},	/* usbb1_ulpiphy_dat5 */
	{USBB1_ULPITLL_DAT6, (IEN | OFF_EN | OFF_PD | OFF_IN | M4)},	/* usbb1_ulpiphy_dat6 */
	{USBB1_ULPITLL_DAT7, (IEN | OFF_EN | OFF_PD | OFF_IN | M4)},	/* usbb1_ulpiphy_dat7 */
	{USBB1_HSIC_DATA, (SAFE_MODE)},					/* nc */
	{USBB1_HSIC_STROBE, (SAFE_MODE)},				/* nc */
	{USBC1_ICUSB_DP, (SAFE_MODE)},					/* nc */
	{USBC1_ICUSB_DM, (SAFE_MODE)},					/* nc */
	{SDMMC1_CLK, (PTU | OFF_EN | OFF_OUT_PTD | M0)},		/* sdmmc1_clk */
	{SDMMC1_CMD, (PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M0)},	/* sdmmc1_cmd */
	{SDMMC1_DAT0, (PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M0)},	/* sdmmc1_dat0 */
	{SDMMC1_DAT1, (PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M0)},	/* sdmmc1_dat1 */
	{SDMMC1_DAT2, (PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M0)},	/* sdmmc1_dat2 */
	{SDMMC1_DAT3, (PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M0)},	/* sdmmc1_dat3 */
	{SDMMC1_DAT4, (SAFE_MODE)},					/* nc */
	{SDMMC1_DAT5, (SAFE_MODE)},					/* nc */
	{SDMMC1_DAT6, (SAFE_MODE)},					/* nc */
	{SDMMC1_DAT7, (SAFE_MODE)},					/* nc */
	{ABE_MCBSP2_CLKX, (SAFE_MODE)},					/* nc */
	{ABE_MCBSP2_DR, (SAFE_MODE)},					/* nc */
	{ABE_MCBSP2_DX, (SAFE_MODE)},					/* nc */
	{ABE_MCBSP2_FSX, (SAFE_MODE)},					/* nc */
	{ABE_MCBSP1_CLKX, (SAFE_MODE)},					/* unused */
	{ABE_MCBSP1_DR, (SAFE_MODE)},					/* unused */
	{ABE_MCBSP1_DX, (SAFE_MODE)},					/* unused */
	{ABE_MCBSP1_FSX, (SAFE_MODE)},					/* nc */
	{ABE_PDM_UL_DATA, (SAFE_MODE)},					/* unused */
	{ABE_PDM_DL_DATA, (SAFE_MODE)},					/* unused */
	{ABE_PDM_FRAME, (SAFE_MODE)},					/* unused */
	{ABE_PDM_LB_CLK, (SAFE_MODE)},					/* unused */
	{ABE_CLKS, (SAFE_MODE)},					/* unused */
	{ABE_DMIC_CLK1, (SAFE_MODE)},					/* nc */
	{ABE_DMIC_DIN1, (SAFE_MODE)},					/* unused */
	{ABE_DMIC_DIN2, (SAFE_MODE)},					/* nc */
	{ABE_DMIC_DIN3, (SAFE_MODE)},					/* unused */
	{UART2_CTS, (SAFE_MODE)},					/* nc */
	{UART2_RTS, (SAFE_MODE)},					/* nc */
	{UART2_RX, (SAFE_MODE)},					/* nc */
	{UART2_TX, (SAFE_MODE)},					/* nc */
	{HDQ_SIO, (SAFE_MODE)},						/* unused */
	{I2C1_SCL, (PTU | IEN | M0)},					/* i2c1_scl */
	{I2C1_SDA, (PTU | IEN | M0)},					/* i2c1_sda */
	{I2C2_SCL, (SAFE_MODE)},					/* unused */
	{I2C2_SDA, (SAFE_MODE)},					/* unused */
	{I2C3_SCL, (PTU | IEN | M0)},					/* i2c3_scl */
	{I2C3_SDA, (PTU | IEN | M0)},					/* i2c3_sda */
	{I2C4_SCL, (SAFE_MODE)},					/* nc */
	{I2C4_SDA, (SAFE_MODE)},					/* nc */
	{MCSPI1_CLK, (SAFE_MODE)},					/* unused */
	{MCSPI1_SOMI, (SAFE_MODE)},					/* unused */
	{MCSPI1_SIMO, (SAFE_MODE)},					/* unused */
	{MCSPI1_CS0, (SAFE_MODE)},					/* unused */
	{MCSPI1_CS1, (SAFE_MODE)},					/* unused */
	{MCSPI1_CS2, (SAFE_MODE)},					/* nc */
	{MCSPI1_CS3, (SAFE_MODE)},					/* nc */
	{UART3_CTS_RCTX, (PTU | IEN | M0)},				/* uart3_tx */
	{UART3_RTS_SD, (M0)},						/* uart3_rts_sd */
	{UART3_RX_IRRX, (IEN | M0)},					/* uart3_rx */
	{UART3_TX_IRTX, (M0)},						/* uart3_tx */
	{SDMMC5_CLK, (PTU | IEN | M3)},					/* goio_145 */
	{SDMMC5_CMD, (PTU | IEN | M3)},					/* goio_146 */
	{SDMMC5_DAT0, (SAFE_MODE)},					/* nc */
	{SDMMC5_DAT1, (SAFE_MODE)},					/* nc */
	{SDMMC5_DAT2, (SAFE_MODE)},					/* nc */
	{SDMMC5_DAT3, (SAFE_MODE)},					/* nc */
	{MCSPI4_CLK, (PTU | IEN | M3)},					/* gpio_151 */
	{MCSPI4_SIMO, (PTU | IEN | M3)},				/* gpio_152 */
	{MCSPI4_SOMI, (PTU | IEN | M3)},				/* gpio_153 */
	{MCSPI4_CS0, (SAFE_MODE)},					/* nc */
	{UART4_RX, (SAFE_MODE)},					/* nc */
	{UART4_TX, (SAFE_MODE)},					/* nc */
	{USBB2_ULPITLL_CLK, (SAFE_MODE)},				/* nc */
	{USBB2_ULPITLL_STP, (SAFE_MODE)},				/* unused */
	{USBB2_ULPITLL_DIR, (SAFE_MODE)},				/* unused */
	{USBB2_ULPITLL_NXT, (SAFE_MODE)},				/* unused */
	{USBB2_ULPITLL_DAT0, (SAFE_MODE)},				/* unused */
	{USBB2_ULPITLL_DAT1, (SAFE_MODE)},				/* unused */
	{USBB2_ULPITLL_DAT2, (SAFE_MODE)},				/* unused */
	{USBB2_ULPITLL_DAT3, (SAFE_MODE)},				/* unused */
	{USBB2_ULPITLL_DAT4, (SAFE_MODE)},				/* unused */
	{USBB2_ULPITLL_DAT5, (SAFE_MODE)},				/* unused */
	{USBB2_ULPITLL_DAT6, (SAFE_MODE)},				/* unused */
	{USBB2_ULPITLL_DAT7, (SAFE_MODE)},				/* unused */
	{USBB2_HSIC_DATA, (SAFE_MODE)},					/* unused */
	{USBB2_HSIC_STROBE, (SAFE_MODE)},				/* nc */
	{UNIPRO_TX0, (SAFE_MODE)},					/* nc */
	{UNIPRO_TY0, (SAFE_MODE)},					/* nc */
	{UNIPRO_TX1, (SAFE_MODE)},					/* nc */
	{UNIPRO_TY1, (SAFE_MODE)},					/* nc */
	{UNIPRO_TX2, (SAFE_MODE)},					/* unused */
	{UNIPRO_TY2, (SAFE_MODE)},					/* unused */
	{UNIPRO_RX0, (SAFE_MODE)},					/* unused */
	{UNIPRO_RY0, (SAFE_MODE)},					/* unused */
	{UNIPRO_RX1, (SAFE_MODE)},					/* unused */
	{UNIPRO_RY1, (SAFE_MODE)},					/* unused */
	{UNIPRO_RX2, (SAFE_MODE)},					/* unused */
	{UNIPRO_RY2, (SAFE_MODE)},					/* unused */
	{USBA0_OTG_CE, (PTD | OFF_EN | OFF_PD | OFF_OUT_PTD | M0)},	/* usba0_otg_ce */
	{USBA0_OTG_DP, (IEN | OFF_EN | OFF_PD | OFF_IN | M0)},		/* usba0_otg_dp */
	{USBA0_OTG_DM, (IEN | OFF_EN | OFF_PD | OFF_IN | M0)},		/* usba0_otg_dm */
	{FREF_CLK1_OUT, (SAFE_MODE)},					/* nc */
	{FREF_CLK2_OUT, (SAFE_MODE)},					/* nc */
	{SYS_NIRQ1, (PTU | IEN | M0)},					/* sys_nirq1 */
	{SYS_NIRQ2, (SAFE_MODE)},					/* nc */
	{SYS_BOOT0, (M0)},						/* sys_boot */
	{SYS_BOOT1, (M0)},						/* sys_boot */
	{SYS_BOOT2, (M0)},						/* sys_boot */
	{SYS_BOOT3, (M0)},						/* sys_boot */
	{SYS_BOOT4, (M0)},						/* sys_boot */
	{SYS_BOOT5, (M0)},						/* sys_boot */
	{DPM_EMU0, (IEN | M0)},						/* dpm_emu0 */
	{DPM_EMU1, (IEN | M0)},						/* dpm_emu1 */
	{DPM_EMU2, (SAFE_MODE)},					/* unused */
	{DPM_EMU3, (SAFE_MODE)},					/* unused */
	{DPM_EMU4, (SAFE_MODE)},					/* unused */
	{DPM_EMU5, (SAFE_MODE)},					/* unused */
	{DPM_EMU6, (SAFE_MODE)},					/* unused */
	{DPM_EMU7, (SAFE_MODE)},					/* unused */
	{DPM_EMU8, (SAFE_MODE)},					/* unused */
	{DPM_EMU9, (SAFE_MODE)},					/* unused */
	{DPM_EMU10, (SAFE_MODE)},					/* unused */
	{DPM_EMU11, (SAFE_MODE)},					/* unused */
	{DPM_EMU12, (SAFE_MODE)},					/* unused */
	{DPM_EMU13, (SAFE_MODE)},					/* unused */
	{DPM_EMU14, (SAFE_MODE)},					/* unused */
	{DPM_EMU15, (SAFE_MODE)},					/* unused */
	{DPM_EMU16, (SAFE_MODE)},					/* unused */
	{DPM_EMU17, (SAFE_MODE)},					/* unused */
	{DPM_EMU18, (SAFE_MODE)},					/* unused */
	{DPM_EMU19, (SAFE_MODE)},					/* unused */
};

static const struct pad_conf_entry wkup_padconf_array[] = {
	{GPIO_WK0, (SAFE_MODE)},		/* tbd */
	{GPIO_WK1, (SAFE_MODE)},		/* nc */
	{GPIO_WK2, (SAFE_MODE)},		/* nc */
	{GPIO_WK3, (SAFE_MODE)},		/* nc */
	{GPIO_WK4, (SAFE_MODE)},		/* nc */
	{SR_SCL, (PTU | IEN | M0)},		/* sr_scl */
	{SR_SDA, (PTU | IEN | M0)},		/* sr_sda */
	{FREF_XTAL_IN, (M0)},			/* # */
	{FREF_SLICER_IN, (SAFE_MODE)},		/* nc */
	{FREF_CLK_IOREQ, (SAFE_MODE)},		/* nc */
	{FREF_CLK0_OUT, (M2)},			/* sys_drm_msecure */
	{FREF_CLK3_REQ, (SAFE_MODE)},		/* nc */
	{FREF_CLK3_OUT, (M0)},			/* fref_clk3_out */
	{FREF_CLK4_REQ, (IEN | M3)},		/* gpio_wk7 */
	{FREF_CLK4_OUT, (M0)},			/* fref_clk4_out */
	{SYS_32K, (IEN | M0)},			/* sys_32k */
	{SYS_NRESPWRON, (M0)},			/* sys_nrespwron */
	{SYS_NRESWARM, (M0)},			/* sys_nreswarm */
	{SYS_PWR_REQ, (PTU | M0)},		/* sys_pwr_req */
	{SYS_PWRON_RESET_OUT, (M0)},		/* sys_pwron_reset_out */
	{SYS_BOOT6, (M0)},			/* sys_boot6 */
	{SYS_BOOT7, (M0)},			/* sys_boot7 */
};

void phycard_omap4_set_muxconf_regs(void)
{
	omap4_do_set_mux(OMAP44XX_CONTROL_PADCONF_CORE, core_padconf_array,
			ARRAY_SIZE(core_padconf_array));

	omap4_do_set_mux(OMAP44XX_CONTROL_PADCONF_WKUP, wkup_padconf_array,
			ARRAY_SIZE(wkup_padconf_array));

	/* gpio_wk7 is used for controlling TPS on 4460 */
	if (omap4_revision() >= OMAP4460_ES1_0) {
		writew(M3, OMAP44XX_CONTROL_PADCONF_WKUP + FREF_CLK4_REQ);
		/* Enable GPIO-1 clocks before TPS initialization */
		omap4_enable_gpio1_wup_clocks();
	}
}
