#include <common.h>
#include <init.h>
#include <asm/io.h>
#include <mach/omap4-silicon.h>
#include <mach/omap4-mux.h>

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
	{GPMC_A17, (IEN | PTD | DIS | M0)},				/* gpmc_a17 */
	{GPMC_A18, (IEN | PTD | DIS | M0)},				/* gpmc_a18 */
	{GPMC_A19, (IEN | PTD | DIS | M0)},				/* gpmc_a19 */
	{GPMC_A20, (IEN | PTD | DIS | M0)},				/* gpmc_a20 */
	{GPMC_A21, (IEN | PTD | DIS | M0)},				/* gpmc_a21 */
	{GPMC_A22, (IEN | PTD | DIS | M0)},				/* gpmc_a22 */
	{GPMC_A23, (IEN | PTD | DIS | M0)},				/* gpmc_a23 */
	{GPMC_A24, (IEN | PTD | DIS | M0)},				/* gpmc_a24 */
	{GPMC_A25, (IEN | PTD | DIS | M0)},				/* gpmc_a25 */
	{GPMC_NCS0, (IDIS | PTU | EN | M0)},				/* gpmc_nsc0 */
	{GPMC_NCS1, (IDIS | PTU | EN | M0)},				/* gpmc_nsc1 */
	{GPMC_NCS2, (SAFE_MODE)},					/* nc */
	{GPMC_NCS3, (SAFE_MODE)},					/* nc */
	{GPMC_NWP, (IEN | PTD | DIS | M0)},				/* gpmc_nwp */
	{GPMC_CLK, (SAFE_MODE)},					/* nc */
	{GPMC_NADV_ALE, (IDIS | PTD | DIS | M0)},			/* gpmc_ndav_ale */
	{GPMC_NOE, (IDIS | PTD | DIS | M0)},				/* gpmc_noe */
	{GPMC_NWE, (IDIS | PTD | DIS | M0)},				/* gpmc_nwe */
	{GPMC_NBE0_CLE, (IDIS | PTD | DIS | M0)},			/* gpmc_nbe0_cle */
	{GPMC_NBE1, (SAFE_MODE)},					/* nc */
	{GPMC_WAIT0, (IEN | PTU | EN | M0)},				/* gpmc_wait0 */
	{GPMC_WAIT1, (SAFE_MODE)},					/* nc */
	{C2C_DATA11, (SAFE_MODE)},					/* nc */
	{C2C_DATA12, (SAFE_MODE)},					/* nc */
	{C2C_DATA13, (IDIS | PTU | EN | M0)},				/* gpmc_nsc5 */
	{C2C_DATA14, (SAFE_MODE)},					/* nc */
	{C2C_DATA15, (SAFE_MODE)},					/* nc */
	{HDMI_HPD, (M0)},						/* hdmi_hpd */
	{HDMI_CEC, (DIS | IEN | M3)},					/* gpio_64 */
	{HDMI_DDC_SCL, (PTU | M0)},					/* hdmi_ddc_scl */
	{HDMI_DDC_SDA, (PTU | IEN | M0)},				/* hdmi_ddc_sda */
	{CSI21_DX0, (IEN | M0)},					/* csi21_dx0 */
	{CSI21_DY0, (IEN | M0)},					/* csi21_dy0 */
	{CSI21_DX1, (IEN | M0)},					/* csi21_dx1 */
	{CSI21_DY1, (IEN | M0)},					/* csi21_dy1 */
	{CSI21_DX2, (IEN | M0)},					/* csi21_dx2 */
	{CSI21_DY2, (IEN | M0)},					/* csi21_dy2 */
	{CSI21_DX3, (PTD | M7)},					/* csi21_dx3 */
	{CSI21_DY3, (PTD | M7)},					/* csi21_dy3 */
	{CSI21_DX4, (PTD | OFF_EN | OFF_PD | OFF_IN | M7)},		/* csi21_dx4 */
	{CSI21_DY4, (PTD | OFF_EN | OFF_PD | OFF_IN | M7)},		/* csi21_dy4 */
	{CSI22_DX0, (IEN | M0)},					/* csi22_dx0 */
	{CSI22_DY0, (IEN | M0)},					/* csi22_dy0 */
	{CSI22_DX1, (IEN | M0)},					/* csi22_dx1 */
	{CSI22_DY1, (IEN | M0)},					/* csi22_dy1 */
	{CAM_SHUTTER, (OFF_EN | OFF_PD | OFF_OUT_PTD | M0)},		/* cam_shutter */
	{CAM_STROBE, (OFF_EN | OFF_PD | OFF_OUT_PTD | M0)},		/* cam_strobe */
	{CAM_GLOBALRESET, (PTD | OFF_EN | OFF_PD | OFF_OUT_PTD | M3)},	/* gpio_83 */
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
	{USBB1_HSIC_DATA, (DIS | IEN | M3)},				/* gpio_96 */
	{USBB1_HSIC_STROBE, (DIS | IEN | M3)},				/* gpio_97 */
	{USBC1_ICUSB_DP, (IEN | M0)},					/* usbc1_icusb_dp */
	{USBC1_ICUSB_DM, (IEN | M0)},					/* usbc1_icusb_dm */
	{SDMMC1_CLK, (PTU | OFF_EN | OFF_OUT_PTD | M0)},		/* sdmmc1_clk */
	{SDMMC1_CMD, (PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M0)},	/* sdmmc1_cmd */
	{SDMMC1_DAT0, (PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M0)},	/* sdmmc1_dat0 */
	{SDMMC1_DAT1, (PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M0)},	/* sdmmc1_dat1 */
	{SDMMC1_DAT2, (PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M0)},	/* sdmmc1_dat2 */
	{SDMMC1_DAT3, (PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M0)},	/* sdmmc1_dat3 */
	{SDMMC1_DAT4, (PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M0)},	/* sdmmc1_dat4 */
	{SDMMC1_DAT5, (PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M0)},	/* sdmmc1_dat5 */
	{SDMMC1_DAT6, (PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M0)},	/* sdmmc1_dat6 */
	{SDMMC1_DAT7, (PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M0)},	/* sdmmc1_dat7 */
	{ABE_MCBSP2_CLKX, (IEN | OFF_EN | OFF_PD | OFF_IN | M0)},	/* abe_mcbsp2_clkx */
	{ABE_MCBSP2_DR, (IEN | OFF_EN | OFF_OUT_PTD | M0)},		/* abe_mcbsp2_dr */
	{ABE_MCBSP2_DX, (OFF_EN | OFF_OUT_PTD | M0)},			/* abe_mcbsp2_dx */
	{ABE_MCBSP2_FSX, (IEN | OFF_EN | OFF_PD | OFF_IN | M0)},	/* abe_mcbsp2_fsx */
	{ABE_MCBSP1_CLKX, (DIS | IEN | M3)},				/* gpio_114 */
	{ABE_MCBSP1_DR, (DIS | IEN | M3)},				/* gpio_115 */
	{ABE_MCBSP1_DX, (DIS | IEN | M3)},				/* gpio_116 */
	{ABE_MCBSP1_FSX, (DIS | IEN | M2)},				/* abe_mcasp_amutein */
	{ABE_PDM_UL_DATA, (IEN | OFF_EN | OFF_OUT_PTD | M1)},		/* abe_mcbsp3_dr */
	{ABE_PDM_DL_DATA, (OFF_EN | OFF_OUT_PTD | M1)},			/* abe_mcbsp3_dx */
	{ABE_PDM_FRAME, (IEN | OFF_EN | OFF_PD | OFF_IN | M1)},		/* abe_mcbsp3_clkx */
	{ABE_PDM_LB_CLK, (IEN | OFF_EN | OFF_PD | OFF_IN | M1)},	/* abe_mcbsp3_fsx */
	{ABE_CLKS, (DIS | IEN | M3)},					/* gpio_118 */
	{ABE_DMIC_CLK1, (SAFE_MODE)},					/* nc */
	{ABE_DMIC_DIN1, (SAFE_MODE)},					/* nc */
	{ABE_DMIC_DIN2, (SAFE_MODE)},					/* nc */
	{ABE_DMIC_DIN3, (SAFE_MODE)},					/* nc */
	{UART2_CTS, (PTU | IEN | M0)},					/* uart2_cts */
	{UART2_RTS, (M0)},						/* uart2_rts */
	{UART2_RX, (PTU | IEN | M0)},					/* uart2_rx */
	{UART2_TX, (M0)},						/* uart2_tx */
	{HDQ_SIO, (M0)},						/* hdq_sio */
	{I2C1_SCL, (PTU | IEN | M0)},					/* i2c1_scl */
	{I2C1_SDA, (PTU | IEN | M0)},					/* i2c1_sda */
	{I2C2_SCL, (PTU | IEN | M1)},					/* uart1_rx */
	{I2C2_SDA, (M1)},						/* uart1_tx */
	{I2C3_SCL, (PTU | IEN | M0)},					/* i2c3_scl */
	{I2C3_SDA, (PTU | IEN | M0)},					/* i2c3_sda */
	{I2C4_SCL, (PTU | IEN | M0)},					/* i2c4_scl */
	{I2C4_SDA, (PTU | IEN | M0)},					/* i2c4_sda */
	{MCSPI1_CLK, (IEN | OFF_EN | OFF_PD | OFF_IN | M0)},		/* mcspi1_clk */
	{MCSPI1_SOMI, (IEN | OFF_EN | OFF_PD | OFF_IN | M0)},		/* mcspi1_somi */
	{MCSPI1_SIMO, (IEN | OFF_EN | OFF_PD | OFF_IN | M0)},		/* mcspi1_simo */
	{MCSPI1_CS0, (PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M0)},	/* mcspi1_cs0 */
	{MCSPI1_CS1, (PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M0)},	/* mcspi1_cs1 */
	{MCSPI1_CS2, (PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M0)},	/* mcspi1_cs2 */
	{MCSPI1_CS3, (PTD | IEN | OFF_EN | OFF_PD | OFF_IN | M0)},	/* mcspi1_cs3 */
	{UART3_CTS_RCTX, (PTU | IEN | M0)},				/* uart3_tx */
	{UART3_RTS_SD, (M0)},						/* uart3_rts_sd */
	{UART3_RX_IRRX, (IEN | M0)},					/* uart3_rx */
	{UART3_TX_IRTX, (M0)},						/* uart3_tx */
	{SDMMC5_CLK, (PTU | IEN | OFF_EN | OFF_OUT_PTD | M0)},		/* sdmmc5_clk */
	{SDMMC5_CMD, (PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M0)},	/* sdmmc5_cmd */
	{SDMMC5_DAT0, (PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M0)},	/* sdmmc5_dat0 */
	{SDMMC5_DAT1, (PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M0)},	/* sdmmc5_dat1 */
	{SDMMC5_DAT2, (PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M0)},	/* sdmmc5_dat2 */
	{SDMMC5_DAT3, (PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M0)},	/* sdmmc5_dat3 */
	{MCSPI4_CLK, (SAFE_MODE)},					/* nc */
	{MCSPI4_SIMO, (PTU | IEN | M3)},				/* gpio_152 */
	{MCSPI4_SOMI, (PTU | IEN | M3)},				/* gpio_153 */
	{MCSPI4_CS0, (SAFE_MODE)},					/* nc */
	{UART4_RX, (IEN | M0)},						/* uart4_rx */
	{UART4_TX, (M0)},						/* uart4_tx */
	{USBB2_ULPITLL_CLK, (IEN | M3)},				/* gpio_157 */
	{USBB2_ULPITLL_STP, (IEN | M5)},				/* dispc2_data23 */
	{USBB2_ULPITLL_DIR, (IEN | M5)},				/* dispc2_data22 */
	{USBB2_ULPITLL_NXT, (IEN | M5)},				/* dispc2_data21 */
	{USBB2_ULPITLL_DAT0, (IEN | M5)},				/* dispc2_data20 */
	{USBB2_ULPITLL_DAT1, (IEN | M5)},				/* dispc2_data19 */
	{USBB2_ULPITLL_DAT2, (IEN | M5)},				/* dispc2_data18 */
	{USBB2_ULPITLL_DAT3, (IEN | M5)},				/* dispc2_data15 */
	{USBB2_ULPITLL_DAT4, (IEN | M5)},				/* dispc2_data14 */
	{USBB2_ULPITLL_DAT5, (IEN | M5)},				/* dispc2_data13 */
	{USBB2_ULPITLL_DAT6, (IEN | M5)},				/* dispc2_data12 */
	{USBB2_ULPITLL_DAT7, (IEN | M5)},				/* dispc2_data11 */
	{USBB2_HSIC_DATA, (SAFE_MODE)},					/* nc */
	{USBB2_HSIC_STROBE, (SAFE_MODE)},				/* nc */
	{UNIPRO_TX0, (OFF_EN | OFF_PD | OFF_IN | M1)},			/* kpd_col0 */
	{UNIPRO_TY0, (OFF_EN | OFF_PD | OFF_IN | M1)},			/* kpd_col1 */
	{UNIPRO_TX1, (OFF_EN | OFF_PD | OFF_IN | M1)},			/* kpd_col2 */
	{UNIPRO_TY1, (OFF_EN | OFF_PD | OFF_IN | M1)},			/* kpd_col3 */
	{UNIPRO_TX2, (OFF_EN | OFF_PD | OFF_IN | M1)},			/* kpd_col4 */
	{UNIPRO_TY2, (SAFE_MODE)},					/* nc */
	{UNIPRO_RX0, (PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M1)},	/* kpd_row0 */
	{UNIPRO_RY0, (PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M1)},	/* kpd_row1 */
	{UNIPRO_RX1, (PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M1)},	/* kpd_row2 */
	{UNIPRO_RY1, (PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M1)},	/* kpd_row3 */
	{UNIPRO_RX2, (PTU | IEN | OFF_EN | OFF_PD | OFF_IN | M1)},	/* kpd_row4 */
	{UNIPRO_RY2, (DIS | IEN | M3)},					/* gpio_3 */
	{USBA0_OTG_CE, (PTD | OFF_EN | OFF_PD | OFF_OUT_PTD | M0)},	/* usba0_otg_ce */
	{USBA0_OTG_DP, (IEN | OFF_EN | OFF_PD | OFF_IN | M0)},		/* usba0_otg_dp */
	{USBA0_OTG_DM, (IEN | OFF_EN | OFF_PD | OFF_IN | M0)},		/* usba0_otg_dm */
	{FREF_CLK1_OUT, (SAFE_MODE)},					/* nc */
	{FREF_CLK2_OUT, (SAFE_MODE)},					/* nc */
	{SYS_NIRQ1, (PTU | IEN | M0)},					/* sys_nirq1 */
	{SYS_NIRQ2, (DIS | IEN | M3)},					/* gpio_183 */
	{SYS_BOOT0, (PTU | IEN | M3)},					/* gpio_184 */
	{SYS_BOOT1, (M3)},						/* gpio_185 */
	{SYS_BOOT2, (PTD | IEN | M3)},					/* gpio_186 */
	{SYS_BOOT3, (M3)},						/* gpio_187 */
	{SYS_BOOT4, (M3)},						/* gpio_188 */
	{SYS_BOOT5, (PTD | IEN | M3)},					/* gpio_189 */
	{DPM_EMU0, (IEN | M0)},						/* dpm_emu0 */
	{DPM_EMU1, (IEN | M0)},						/* dpm_emu1 */
	{DPM_EMU2, (SAFE_MODE)},					/* nc */
	{DPM_EMU3, (SAFE_MODE)},					/* nc */
	{DPM_EMU4, (SAFE_MODE)},					/* nc */
	{DPM_EMU5, (SAFE_MODE)},					/* nc */
	{DPM_EMU6, (SAFE_MODE)},					/* nc */
	{DPM_EMU7, (SAFE_MODE)},					/* nc */
	{DPM_EMU8, (SAFE_MODE)},					/* nc */
	{DPM_EMU9, (SAFE_MODE)},					/* nc */
	{DPM_EMU10, (SAFE_MODE)},					/* nc */
	{DPM_EMU11, (SAFE_MODE)},					/* nc */
	{DPM_EMU12, (SAFE_MODE)},					/* nc */
	{DPM_EMU13, (SAFE_MODE)},					/* nc */
	{DPM_EMU14, (SAFE_MODE)},					/* nc */
	{DPM_EMU15, (DIS | M3)},					/* gpio_26 */
	{DPM_EMU16, (M1)},						/* dmtimer8_pwm_evt */
	{DPM_EMU17, (M1)},						/* dmtimer9_pwm_evt */
	{DPM_EMU18, (IEN | M3)},					/* gpio_190 */
	{DPM_EMU19, (IEN | M3)},					/* gpio_191 */
};

static const struct pad_conf_entry wkup_padconf_array[] = {
	{PAD0_SIM_IO, (IEN | M3)},					/* gpio_wk0 */
	{PAD1_SIM_CLK, (IEN | M3)},					/* gpio_wk1 */
	{PAD0_SIM_RESET, (IEN | M3)},					/* gpio_wk2 */
	{PAD1_SIM_CD, (SAFE_MODE)},					/* should be gpio_wk3 but muxed with gpio_3*/
	{PAD0_SIM_PWRCTRL, (IEN | M3)},					/* gpio_wk4 */
	{PAD1_SR_SCL, (PTU | IEN | M0)},				/* sr_scl */
	{PAD0_SR_SDA, (PTU | IEN | M0)},				/* sr_sda */
	{PAD1_FREF_XTAL_IN, (M0)},					/* # */
	{PAD0_FREF_SLICER_IN, (SAFE_MODE)},				/* nc */
	{PAD1_FREF_CLK_IOREQ, (SAFE_MODE)},					/* nc */
	{PAD0_FREF_CLK0_OUT, (M2)},					/* sys_drm_msecure */
	{PAD1_FREF_CLK3_REQ, (SAFE_MODE)},				/* nc */
	{PAD0_FREF_CLK3_OUT, (M0)},					/* fref_clk3_out */
	{PAD1_FREF_CLK4_REQ, (M0)},					/* fref_clk4_req */
	{PAD0_FREF_CLK4_OUT, (M0)},					/* fref_clk4_out */
	{PAD1_SYS_32K, (IEN | M0)},					/* sys_32k */
	{PAD0_SYS_NRESPWRON, (M0)},					/* sys_nrespwron */
	{PAD1_SYS_NRESWARM, (M0)},					/* sys_nreswarm */
	{PAD0_SYS_PWR_REQ, (PTU | M0)},					/* sys_pwr_req */
	{PAD1_SYS_PWRON_RESET, (M0)},					/* sys_pwron_reset_out */
	{PAD0_SYS_BOOT6, (IEN | M3)},					/* gpio_wk9 */
	{PAD1_SYS_BOOT7, (IEN | M3)},					/* gpio_wk10 */
};

static void do_set_mux(u32 base, struct pad_conf_entry const *array, int size)
{
	int i;
	struct pad_conf_entry *pad = (struct pad_conf_entry *) array;

	for (i = 0; i < size; i++, pad++)
		writew(pad->val, base + pad->offset);
}

void set_muxconf_regs(void)
{
	do_set_mux(OMAP44XX_CONTROL_PADCONF_CORE, core_padconf_array,
			ARRAY_SIZE(core_padconf_array));

	do_set_mux(OMAP44XX_CONTROL_PADCONF_WKUP, wkup_padconf_array,
			ARRAY_SIZE(wkup_padconf_array));
}
