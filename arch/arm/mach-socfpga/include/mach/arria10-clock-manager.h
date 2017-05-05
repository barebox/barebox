/*
 * Copyright (C) 2014 Altera Corporation <www.altera.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef	_ARRIA10_CLOCK_MANAGER_H_
#define	_ARRIA10_CLOCK_MANAGER_H_

struct arria10_clock_manager {
	/* clkmgr */
	volatile uint32_t  ctrl;
	volatile uint32_t  intr;
	volatile uint32_t  intrs;
	volatile uint32_t  intrr;
	volatile uint32_t  intren;
	volatile uint32_t  intrens;
	volatile uint32_t  intrenr;
	volatile uint32_t  stat;
	volatile uint32_t  testioctrl;
	volatile uint32_t  _pad_0x24_0x40[7];

	/* mainpllgrp*/
	volatile uint32_t  main_pll_vco0;
	volatile uint32_t  main_pll_vco1;
	volatile uint32_t  main_pll_en;
	volatile uint32_t  main_pll_ens;
	volatile uint32_t  main_pll_enr;
	volatile uint32_t  main_pll_bypass;
	volatile uint32_t  main_pll_bypasss;
	volatile uint32_t  main_pll_bypassr;
	volatile uint32_t  main_pll_mpuclk;
	volatile uint32_t  main_pll_nocclk;
	volatile uint32_t  main_pll_cntr2clk;
	volatile uint32_t  main_pll_cntr3clk;
	volatile uint32_t  main_pll_cntr4clk;
	volatile uint32_t  main_pll_cntr5clk;
	volatile uint32_t  main_pll_cntr6clk;
	volatile uint32_t  main_pll_cntr7clk;
	volatile uint32_t  main_pll_cntr8clk;
	volatile uint32_t  main_pll_cntr9clk;
	volatile uint32_t  main_pll__pad_0x48_0x5b[5];
	volatile uint32_t  main_pll_cntr15clk;
	volatile uint32_t  main_pll_outrst;
	volatile uint32_t  main_pll_outrststat;
	volatile uint32_t  main_pll_nocdiv;
	volatile uint32_t  main_pll__pad_0x6c_0x80[5];

	/* perpllgrp*/
	volatile uint32_t  per_pll_vco0;
	volatile uint32_t  per_pll_vco1;
	volatile uint32_t  per_pll_en;
	volatile uint32_t  per_pll_ens;
	volatile uint32_t  per_pll_enr;
	volatile uint32_t  per_pll_bypass;
	volatile uint32_t  per_pll_bypasss;
	volatile uint32_t  per_pll_bypassr;
	volatile uint32_t  per_pll__pad_0x20_0x27[2];
	volatile uint32_t  per_pll_cntr2clk;
	volatile uint32_t  per_pll_cntr3clk;
	volatile uint32_t  per_pll_cntr4clk;
	volatile uint32_t  per_pll_cntr5clk;
	volatile uint32_t  per_pll_cntr6clk;
	volatile uint32_t  per_pll_cntr7clk;
	volatile uint32_t  per_pll_cntr8clk;
	volatile uint32_t  per_pll_cntr9clk;
	volatile uint32_t  per_pll__pad_0x48_0x5f[6];
	volatile uint32_t  per_pll_outrst;
	volatile uint32_t  per_pll_outrststat;
	volatile uint32_t  per_pll_emacctl;
	volatile uint32_t  per_pll_gpiodiv;
	volatile uint32_t  per_pll__pad_0x70_0x80[4];
};

struct arria10_mainpll_cfg {
	uint32_t vco0_psrc;
	uint32_t vco1_denom;
	uint32_t vco1_numer;
	uint32_t mpuclk;
	uint32_t mpuclk_cnt;
	uint32_t mpuclk_src;
	uint32_t nocclk;
	uint32_t nocclk_cnt;
	uint32_t nocclk_src;
	uint32_t cntr2clk_cnt;
	uint32_t cntr3clk_cnt;
	uint32_t cntr4clk_cnt;
	uint32_t cntr5clk_cnt;
	uint32_t cntr6clk_cnt;
	uint32_t cntr7clk_cnt;
	uint32_t cntr7clk_src;
	uint32_t cntr8clk_cnt;
	uint32_t cntr9clk_cnt;
	uint32_t cntr9clk_src;
	uint32_t cntr15clk_cnt;
	uint32_t nocdiv_l4mainclk;
	uint32_t nocdiv_l4mpclk;
	uint32_t nocdiv_l4spclk;
	uint32_t nocdiv_csatclk;
	uint32_t nocdiv_cstraceclk;
	uint32_t nocdiv_cspdbgclk;
};

struct arria10_perpll_cfg {
	uint32_t vco0_psrc;
	uint32_t vco1_denom;
	uint32_t vco1_numer;
	uint32_t cntr2clk_cnt;
	uint32_t cntr2clk_src;
	uint32_t cntr3clk_cnt;
	uint32_t cntr3clk_src;
	uint32_t cntr4clk_cnt;
	uint32_t cntr4clk_src;
	uint32_t cntr5clk_cnt;
	uint32_t cntr5clk_src;
	uint32_t cntr6clk_cnt;
	uint32_t cntr6clk_src;
	uint32_t cntr7clk_cnt;
	uint32_t cntr8clk_cnt;
	uint32_t cntr8clk_src;
	uint32_t cntr9clk_cnt;
	uint32_t cntr9clk_src;
	uint32_t emacctl_emac0sel;
	uint32_t emacctl_emac1sel;
	uint32_t emacctl_emac2sel;
	uint32_t gpiodiv_gpiodbclk;
};

extern int arria10_cm_basic_init(struct arria10_mainpll_cfg *mainpll_cfg,
				 struct arria10_perpll_cfg *perpll_cfg);
extern unsigned int cm_get_mmc_controller_clk_hz(void);
extern void arria10_cm_use_intosc(void);
extern uint32_t cm_l4_main_clk_hz;
extern uint32_t cm_l4_sp_clk_hz;
extern uint32_t cm_l4_mp_clk_hz;
extern uint32_t cm_l4_sys_free_clk_hz;

#define ARRIA10_CLKMGR_ALTERAGRP_MPU_CLK_OFFSET			0x140
#define ARRIA10_CLKMGR_MAINPLL_NOC_CLK_OFFSET			0x144

/* value */
#define ARRIA10_CLKMGR_MAINPLL_BYPASS_RESET			0x0000003f
#define ARRIA10_CLKMGR_MAINPLL_VCO0_RESET			0x00010053
#define ARRIA10_CLKMGR_MAINPLL_VCO1_RESET			0x00010001
#define ARRIA10_CLKMGR_MAINPLL_VCO0_PSRC_EOSC			0x0
#define ARRIA10_CLKMGR_MAINPLL_VCO0_PSRC_E_INTOSC		0x1
#define ARRIA10_CLKMGR_MAINPLL_VCO0_PSRC_F2S			0x2
#define ARRIA10_CLKMGR_PERPLL_BYPASS_RESET			0x000000ff
#define ARRIA10_CLKMGR_PERPLL_VCO0_RESET			0x00010053
#define ARRIA10_CLKMGR_PERPLL_VCO1_RESET			0x00010001
#define ARRIA10_CLKMGR_PERPLL_VCO0_PSRC_EOSC			0x0
#define ARRIA10_CLKMGR_PERPLL_VCO0_PSRC_E_INTOSC		0x1
#define ARRIA10_CLKMGR_PERPLL_VCO0_PSRC_F2S			0x2
#define ARRIA10_CLKMGR_PERPLL_VCO0_PSRC_MAIN			0x3

/* mask */
#define ARRIA10_CLKMGR_MAINPLL_EN_S2FUSER0CLKEN_SET_MSK		0x00000040
#define ARRIA10_CLKMGR_MAINPLL_EN_HMCPLLREFCLKEN_SET_MSK	0x00000080
#define ARRIA10_CLKMGR_MAINPLL_VCO0_BGPWRDN_SET_MSK		0x00000001
#define ARRIA10_CLKMGR_MAINPLL_VCO0_PWRDN_SET_MSK		0x00000002
#define ARRIA10_CLKMGR_MAINPLL_VCO0_EN_SET_MSK			0x00000004
#define ARRIA10_CLKMGR_MAINPLL_VCO0_OUTRSTALL_SET_MSK		0x00000008
#define ARRIA10_CLKMGR_MAINPLL_VCO0_REGEXTSEL_SET_MSK		0x00000010
#define ARRIA10_CLKMGR_MAINPLL_VCO0_PSRC_MSK			0x00000003
#define ARRIA10_CLKMGR_MAINPLL_VCO1_NUMER_MSK			0x00001fff
#define ARRIA10_CLKMGR_MAINPLL_VCO1_DENOM_MSK			0x0000003f
#define ARRIA10_CLKMGR_MAINPLL_CNTRCLK_MSK			0x000003ff
#define ARRIA10_CLKMGR_MAINPLL_MPUCLK_CNT_MSK			0x000003ff
#define ARRIA10_CLKMGR_MAINPLL_MPUCLK_SRC_MAIN			0
#define ARRIA10_CLKMGR_MAINPLL_MPUCLK_SRC_PERI			1
#define ARRIA10_CLKMGR_MAINPLL_MPUCLK_SRC_OSC1			2
#define ARRIA10_CLKMGR_MAINPLL_MPUCLK_SRC_INTOSC		3
#define ARRIA10_CLKMGR_MAINPLL_MPUCLK_SRC_FPGA			4
#define ARRIA10_CLKMGR_MAINPLL_NOCDIV_MSK			0x00000003
#define ARRIA10_CLKMGR_MAINPLL_NOCCLK_CNT_MSK			0x000003ff
#define ARRIA10_CLKMGR_MAINPLL_NOCCLK_SRC_MSK			0x00000007
#define ARRIA10_CLKMGR_MAINPLL_NOCCLK_SRC_MAIN			0
#define ARRIA10_CLKMGR_MAINPLL_NOCCLK_SRC_PERI			1
#define ARRIA10_CLKMGR_MAINPLL_NOCCLK_SRC_OSC1			2
#define ARRIA10_CLKMGR_MAINPLL_NOCCLK_SRC_INTOSC		3
#define ARRIA10_CLKMGR_MAINPLL_NOCCLK_SRC_FPGA			4
#define ARRIA10_CLKMGR_CLKMGR_STAT_BUSY_SET_MSK			0x00000001
#define ARRIA10_CLKMGR_CLKMGR_STAT_MAINPLLLOCKED_SET_MSK	0x00000100
#define ARRIA10_CLKMGR_CLKMGR_STAT_PERPLLLOCKED_SET_MSK		0x00000200
#define ARRIA10_CLKMGR_CLKMGR_STAT_BOOTCLKSRC_SET_MSK		0x00020000
#define ARRIA10_CLKMGR_CLKMGR_INTR_PERPLLFBSLIP_SET_MSK		0x00000800
#define ARRIA10_CLKMGR_CLKMGR_INTR_MAINPLLFBSLIP_SET_MSK	0x00000400
#define ARRIA10_CLKMGR_CLKMGR_INTR_PERPLLRFSLIP_SET_MSK		0x00000200
#define ARRIA10_CLKMGR_CLKMGR_INTR_MAINPLLRFSLIP_SET_MSK	0x00000100
#define ARRIA10_CLKMGR_CLKMGR_INTR_PERPLLLOST_SET_MSK		0x00000008
#define ARRIA10_CLKMGR_CLKMGR_INTR_MAINPLLLOST_SET_MSK		0x00000004
#define ARRIA10_CLKMGR_CLKMGR_INTR_MAINPLLACHIEVED_SET_MSK	0x00000001
#define ARRIA10_CLKMGR_CLKMGR_INTR_PERPLLACHIEVED_SET_MSK	0x00000002
#define ARRIA10_CLKMGR_CLKMGR_CTL_BOOTMOD_SET_MSK		0x00000001
#define ARRIA10_CLKMGR_CLKMGR_CTL_BOOTCLK_INTOSC_SET_MSK	0x00000300
#define ARRIA10_CLKMGR_PERPLL_VCO0_BGPWRDN_SET_MSK		0x00000001
#define ARRIA10_CLKMGR_PERPLL_VCO0_PWRDN_SET_MSK		0x00000002
#define ARRIA10_CLKMGR_PERPLL_VCO0_EN_SET_MSK			0x00000004
#define ARRIA10_CLKMGR_PERPLL_VCO0_OUTRSTALL_SET_MSK		0x00000008
#define ARRIA10_CLKMGR_PERPLL_VCO0_REGEXTSEL_SET_MSK		0x00000010
#define ARRIA10_CLKMGR_PERPLL_EN_RESET				0x00000f7f
#define ARRIA10_CLKMGR_PERPLL_VCO0_PSRC_MSK			0x00000003
#define ARRIA10_CLKMGR_PERPLL_VCO1_NUMER_MSK			0x00001fff
#define ARRIA10_CLKMGR_PERPLL_VCO1_DENOM_MSK			0x0000003f
#define ARRIA10_CLKMGR_PERPLL_CNTRCLK_MSK			0x000003ff

#define ARRIA10_CLKMGR_PERPLLGRP_EN_SDMMCCLK_MASK		0x00000020
#define ARRIA10_CLKMGR_PERPLLGRP_SRC_MSK			0x00000007
#define ARRIA10_CLKMGR_PERPLLGRP_SRC_MAIN			0
#define ARRIA10_CLKMGR_PERPLLGRP_SRC_PERI			1
#define ARRIA10_CLKMGR_PERPLLGRP_SRC_OSC1			2
#define ARRIA10_CLKMGR_PERPLLGRP_SRC_INTOSC			3
#define ARRIA10_CLKMGR_PERPLLGRP_SRC_FPGA			4

/* bit shifting macro */
#define ARRIA10_CLKMGR_MAINPLL_VCO0_PSRC_LSB			8
#define ARRIA10_CLKMGR_MAINPLL_VCO1_DENOM_LSB			16
#define ARRIA10_CLKMGR_MAINPLL_NOCCLK_PERICNT_LSB		16
#define ARRIA10_CLKMGR_MAINPLL_NOCCLK_SRC_LSB			16
#define ARRIA10_CLKMGR_MAINPLL_NOCDIV_L4MAINCLK_LSB		0
#define ARRIA10_CLKMGR_MAINPLL_NOCDIV_L4MPCLK_LSB		8
#define ARRIA10_CLKMGR_MAINPLL_NOCDIV_L4SPCLK_LSB		16
#define ARRIA10_CLKMGR_MAINPLL_NOCDIV_CSATCLK_LSB		24
#define ARRIA10_CLKMGR_MAINPLL_NOCDIV_CSTRACECLK_LSB		26
#define ARRIA10_CLKMGR_MAINPLL_NOCDIV_CSPDBGCLK_LSB		28
#define ARRIA10_CLKMGR_MAINPLL_MPUCLK_SRC_LSB			16
#define ARRIA10_CLKMGR_MAINPLL_MPUCLK_PERICNT_LSB		16
#define ARRIA10_CLKMGR_MAINPLL_NOCCLK_SRC_LSB			16
#define ARRIA10_CLKMGR_MAINPLL_CNTR7CLK_SRC_LSB			16
#define ARRIA10_CLKMGR_MAINPLL_CNTR9CLK_SRC_LSB			16
#define ARRIA10_CLKMGR_PERPLL_VCO1_DENOM_LSB			16
#define ARRIA10_CLKMGR_PERPLL_VCO0_PSRC_LSB			8
#define ARRIA10_CLKMGR_PERPLL_CNTR2CLK_SRC_LSB			16
#define ARRIA10_CLKMGR_PERPLL_CNTR3CLK_SRC_LSB			16
#define ARRIA10_CLKMGR_PERPLL_CNTR4CLK_SRC_LSB			16
#define ARRIA10_CLKMGR_PERPLL_CNTR5CLK_SRC_LSB			16
#define ARRIA10_CLKMGR_PERPLL_CNTR6CLK_SRC_LSB			16
#define ARRIA10_CLKMGR_PERPLL_CNTR8CLK_SRC_LSB			16
#define ARRIA10_CLKMGR_PERPLL_EMACCTL_EMAC0SEL_LSB		26
#define ARRIA10_CLKMGR_PERPLL_EMACCTL_EMAC1SEL_LSB		27
#define ARRIA10_CLKMGR_PERPLL_EMACCTL_EMAC2SEL_LSB		28

/* PLL ramping work around */
#define ARRIA10_CLKMGR_PLL_RAMP_MPUCLK_THRESHOLD_HZ		900000000
#define ARRIA10_CLKMGR_PLL_RAMP_NOCCLK_THRESHOLD_HZ		300000000
#define ARRIA10_CLKMGR_PLL_RAMP_MPUCLK_INCREMENT_HZ		100000000
#define ARRIA10_CLKMGR_PLL_RAMP_NOCCLK_INCREMENT_HZ		 33000000

#endif /* _ARRIA10_CLOCK_MANAGER_H_ */
