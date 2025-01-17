#define pr_fmt(fmt)     "ti-k3-clk: " fmt

#include <linux/clk.h>
#include <of.h>
#include <driver.h>
#include "ti-k3-clk.h"

static const char * const gluelogic_hfosc0_clkout_parents[] = {
	NULL,
	NULL,
	"osc_24_mhz",
	"osc_25_mhz",
	"osc_26_mhz",
	NULL,
};

static const char * const main_emmcsd0_io_clklb_sel_out0_parents[] = {
	"board_0_mmc0_clklb_out",
	"board_0_mmc0_clk_out",
};

static const char * const main_emmcsd1_io_clklb_sel_out0_parents[] = {
	"board_0_mmc1_clklb_out",
	"board_0_mmc1_clk_out",
};

static const char * const main_ospi_loopback_clk_sel_out0_parents[] = {
	"board_0_ospi0_dqs_out",
	"board_0_ospi0_lbclko_out",
};

static const char * const main_usb0_refclk_sel_out0_parents[] = {
	"gluelogic_hfosc0_clkout",
	"postdiv4_16ff_main_0_hsdivout8_clk",
};

static const char * const main_usb1_refclk_sel_out0_parents[] = {
	"gluelogic_hfosc0_clkout",
	"postdiv4_16ff_main_0_hsdivout8_clk",
};

static const char * const sam62_pll_ctrl_wrap_main_0_sysclkout_clk_parents[] = {
	"gluelogic_hfosc0_clkout",
	"hsdiv4_16fft_main_0_hsdivout0_clk",
};

static const char * const sam62_pll_ctrl_wrap_mcu_0_sysclkout_clk_parents[] = {
	"gluelogic_hfosc0_clkout",
	"hsdiv4_16fft_mcu_0_hsdivout0_clk",
};

static const char * const clkout0_ctrl_out0_parents[] = {
	"hsdiv4_16fft_main_2_hsdivout1_clk",
	"hsdiv4_16fft_main_2_hsdivout1_clk10",
};

static const char * const clk_32k_rc_sel_out0_parents[] = {
	"gluelogic_rcosc_clk_1p0v_97p65k",
	"hsdiv0_16fft_mcu_32khz_gen_0_hsdivout0_clk",
	"clk_32k_rc_sel_div_clkout",
	"gluelogic_lfosc0_clkout",
};

static const char * const main_cp_gemac_cpts_clk_sel_out0_parents[] = {
	"postdiv4_16ff_main_2_hsdivout5_clk",
	"postdiv4_16ff_main_0_hsdivout6_clk",
	"board_0_cp_gemac_cpts0_rft_clk_out",
	NULL,
	"board_0_mcu_ext_refclk0_out",
	"board_0_ext_refclk1_out",
	"sam62_pll_ctrl_wrap_mcu_0_chip_div1_clk_clk",
	"sam62_pll_ctrl_wrap_main_0_chip_div1_clk_clk",
};

static const char * const main_emmcsd0_refclk_sel_out0_parents[] = {
	"postdiv4_16ff_main_0_hsdivout5_clk",
	"hsdiv4_16fft_main_2_hsdivout2_clk",
};

static const char * const main_emmcsd1_refclk_sel_out0_parents[] = {
	"postdiv4_16ff_main_0_hsdivout5_clk",
	"hsdiv4_16fft_main_2_hsdivout2_clk",
};

static const char * const main_gtcclk_sel_out0_parents[] = {
	"postdiv4_16ff_main_2_hsdivout5_clk",
	"postdiv4_16ff_main_0_hsdivout6_clk",
	"board_0_cp_gemac_cpts0_rft_clk_out",
	NULL,
	"board_0_mcu_ext_refclk0_out",
	"board_0_ext_refclk1_out",
	"sam62_pll_ctrl_wrap_mcu_0_chip_div1_clk_clk",
	"sam62_pll_ctrl_wrap_main_0_chip_div1_clk_clk",
};

static const char * const main_ospi_ref_clk_sel_out0_parents[] = {
	"hsdiv4_16fft_main_0_hsdivout1_clk",
	"postdiv1_16fft_main_1_hsdivout5_clk",
};

static const char * const wkup_clkout_sel_out0_parents[] = {
	"gluelogic_hfosc0_clkout",
	"gluelogic_lfosc0_clkout",
	"hsdiv4_16fft_main_0_hsdivout2_clk",
	"hsdiv4_16fft_main_1_hsdivout2_clk",
	"postdiv4_16ff_main_2_hsdivout9_clk",
	"clk_32k_rc_sel_out0",
	"gluelogic_rcosc_clkout",
	"gluelogic_hfosc0_clkout",
};

static const char * const wkup_clksel_out0_parents[] = {
	"hsdiv1_16fft_main_15_hsdivout0_clk",
	"hsdiv4_16fft_mcu_0_hsdivout0_clk",
};

static const char * const main_usart0_fclk_sel_out0_parents[] = {
	"usart_programmable_clock_divider_out0",
	"hsdiv4_16fft_main_1_hsdivout1_clk",
};

static inline struct clk *k3_clk_divider(const char *name, const char *parent,
                void __iomem *reg, u8 shift, u8 width, u32 flags, u32 div_flags)
{
        return clk_divider(name, parent, CLK_SET_RATE_PARENT, reg, shift, width, 0);
}

static inline struct clk *k3_clk_pll(const char *name, const char *parent,
                void __iomem *reg, u32 flags)
{
	return clk_register_ti_k3_pll(name, parent, reg);
}

static inline struct clk *k3_clk_mux(const char *name, const char * const *parents,
				     int num_parents, void __iomem *reg, u8 shift,
				     u8 width, u32 flags)
{
	return clk_mux(name, CLK_SET_RATE_PARENT | CLK_SET_RATE_NO_REPARENT | flags, reg,
		       shift, width, parents, num_parents, 0);
}

static inline struct clk *k3_clk_div_devfreq(const char *name, const char *parent,
                void __iomem *reg, u8 shift, u8 width, u32 flags, u32 div_flags, u32 freq)
{
	return NULL;
}

static inline struct clk *k3_clk_mux_pllctrl(const char *name, const char * const *parents,
					     int num_parents, void __iomem *reg, u32 freq)
{
	return NULL;
}

struct clk_lookup_data {
	unsigned int dev_id;
	unsigned int clk_id;
	const char *clk_name;
};

#define DEV_CLK(_dev_id, _clk_id, _clk_name) { .dev_id = _dev_id, \
	.clk_id = _clk_id, .clk_name = _clk_name, }

static struct clk_lookup_data am625_lookup[] = {
	DEV_CLK(13, 0, "sam62_pll_ctrl_wrap_main_0_chip_div1_clk_clk"),
	DEV_CLK(13, 3, "main_cp_gemac_cpts_clk_sel_out0"),
	DEV_CLK(13, 4, "postdiv4_16ff_main_2_hsdivout5_clk"),
	DEV_CLK(13, 5, "postdiv4_16ff_main_0_hsdivout6_clk"),
	DEV_CLK(13, 6, "board_0_cp_gemac_cpts0_rft_clk_out"),
	DEV_CLK(13, 8, "board_0_mcu_ext_refclk0_out"),
	DEV_CLK(13, 9, "board_0_ext_refclk1_out"),
	DEV_CLK(13, 10, "sam62_pll_ctrl_wrap_mcu_0_chip_div1_clk_clk"),
	DEV_CLK(13, 11, "sam62_pll_ctrl_wrap_main_0_chip_div1_clk_clk"),
	DEV_CLK(13, 13, "hsdiv4_16fft_main_2_hsdivout1_clk"),
	DEV_CLK(13, 14, "hsdiv4_16fft_main_2_hsdivout1_clk"),
	DEV_CLK(13, 15, "hsdiv4_16fft_main_2_hsdivout1_clk"),
	DEV_CLK(13, 16, "hsdiv4_16fft_main_2_hsdivout1_clk"),
	DEV_CLK(13, 17, "hsdiv4_16fft_main_2_hsdivout1_clk"),
	DEV_CLK(13, 19, "board_0_rgmii1_rxc_out"),
	DEV_CLK(13, 20, "board_0_rgmii1_txc_out"),
	DEV_CLK(13, 22, "board_0_rgmii2_rxc_out"),
	DEV_CLK(13, 23, "board_0_rgmii2_txc_out"),
	DEV_CLK(13, 25, "hsdiv4_16fft_main_2_hsdivout1_clk"),
	DEV_CLK(13, 26, "hsdiv4_16fft_main_2_hsdivout1_clk"),
	DEV_CLK(13, 27, "hsdiv4_16fft_main_2_hsdivout1_clk"),
	DEV_CLK(13, 28, "board_0_rmii1_ref_clk_out"),
	DEV_CLK(13, 29, "board_0_rmii2_ref_clk_out"),
	DEV_CLK(16, 0, "hsdiv4_16fft_main_0_hsdivout1_clk"),
	DEV_CLK(16, 1, "hsdiv4_16fft_main_0_hsdivout2_clk"),
	DEV_CLK(16, 2, "hsdiv4_16fft_main_0_hsdivout3_clk"),
	DEV_CLK(16, 3, "hsdiv4_16fft_main_0_hsdivout4_clk"),
	DEV_CLK(16, 4, "gluelogic_hfosc0_clkout"),
	DEV_CLK(16, 5, "board_0_ext_refclk1_out"),
	DEV_CLK(16, 6, "sam62_pll_ctrl_wrap_main_0_chip_div1_clk_clk"),
	DEV_CLK(16, 7, "postdiv4_16ff_main_2_hsdivout8_clk"),
	DEV_CLK(16, 8, "gluelogic_hfosc0_clkout"),
	DEV_CLK(16, 9, "board_0_ext_refclk1_out"),
	DEV_CLK(16, 10, "gluelogic_rcosc_clkout"),
	DEV_CLK(16, 11, "sam62_pll_ctrl_wrap_main_0_chip_div1_clk_clk"),
	DEV_CLK(16, 12, "sam62_pll_ctrl_wrap_main_0_chip_div1_clk_clk"),
	DEV_CLK(57, 0, "main_emmcsd0_io_clklb_sel_out0"),
	DEV_CLK(57, 1, "board_0_mmc0_clklb_out"),
	DEV_CLK(57, 2, "board_0_mmc0_clk_out"),
	DEV_CLK(57, 5, "sam62_pll_ctrl_wrap_main_0_chip_div1_clk_clk"),
	DEV_CLK(57, 6, "main_emmcsd0_refclk_sel_out0"),
	DEV_CLK(57, 7, "postdiv4_16ff_main_0_hsdivout5_clk"),
	DEV_CLK(57, 8, "hsdiv4_16fft_main_2_hsdivout2_clk"),
	DEV_CLK(58, 0, "main_emmcsd1_io_clklb_sel_out0"),
	DEV_CLK(58, 1, "board_0_mmc1_clklb_out"),
	DEV_CLK(58, 2, "board_0_mmc1_clk_out"),
	DEV_CLK(58, 5, "sam62_pll_ctrl_wrap_main_0_chip_div1_clk_clk"),
	DEV_CLK(58, 6, "main_emmcsd1_refclk_sel_out0"),
	DEV_CLK(58, 7, "postdiv4_16ff_main_0_hsdivout5_clk"),
	DEV_CLK(58, 8, "hsdiv4_16fft_main_2_hsdivout2_clk"),
	DEV_CLK(61, 0, "main_gtcclk_sel_out0"),
	DEV_CLK(61, 1, "postdiv4_16ff_main_2_hsdivout5_clk"),
	DEV_CLK(61, 2, "postdiv4_16ff_main_0_hsdivout6_clk"),
	DEV_CLK(61, 3, "board_0_cp_gemac_cpts0_rft_clk_out"),
	DEV_CLK(61, 5, "board_0_mcu_ext_refclk0_out"),
	DEV_CLK(61, 6, "board_0_ext_refclk1_out"),
	DEV_CLK(61, 7, "sam62_pll_ctrl_wrap_mcu_0_chip_div1_clk_clk"),
	DEV_CLK(61, 8, "sam62_pll_ctrl_wrap_main_0_chip_div1_clk_clk"),
	DEV_CLK(61, 9, "wkup_clksel_out0"),
	DEV_CLK(61, 10, "hsdiv1_16fft_main_15_hsdivout0_clk"),
	DEV_CLK(61, 11, "hsdiv4_16fft_mcu_0_hsdivout0_clk"),
	DEV_CLK(75, 0, "board_0_ospi0_dqs_out"),
	DEV_CLK(75, 1, "sam62_pll_ctrl_wrap_main_0_chip_div1_clk_clk"),
	DEV_CLK(75, 2, "main_ospi_loopback_clk_sel_out0"),
	DEV_CLK(75, 3, "board_0_ospi0_dqs_out"),
	DEV_CLK(75, 4, "board_0_ospi0_lbclko_out"),
	DEV_CLK(75, 6, "sam62_pll_ctrl_wrap_main_0_chip_div1_clk_clk"),
	DEV_CLK(75, 7, "main_ospi_ref_clk_sel_out0"),
	DEV_CLK(75, 8, "hsdiv4_16fft_main_0_hsdivout1_clk"),
	DEV_CLK(75, 9, "postdiv1_16fft_main_1_hsdivout5_clk"),
	DEV_CLK(77, 0, "sam62_pll_ctrl_wrap_main_0_chip_div1_clk_clk"),
	DEV_CLK(95, 0, "gluelogic_rcosc_clkout"),
	DEV_CLK(95, 1, "gluelogic_hfosc0_clkout"),
	DEV_CLK(95, 2, "wkup_clksel_out0"),
	DEV_CLK(95, 3, "hsdiv1_16fft_main_15_hsdivout0_clk"),
	DEV_CLK(95, 4, "hsdiv4_16fft_mcu_0_hsdivout0_clk"),
	DEV_CLK(102, 0, "sam62_pll_ctrl_wrap_main_0_chip_div1_clk_clk"),
	DEV_CLK(102, 1, "board_0_i2c0_scl_out"),
	DEV_CLK(102, 2, "hsdiv4_16fft_main_1_hsdivout0_clk"),
	DEV_CLK(107, 0, "wkup_clksel_out0"),
	DEV_CLK(107, 1, "hsdiv1_16fft_main_15_hsdivout0_clk"),
	DEV_CLK(107, 2, "hsdiv4_16fft_mcu_0_hsdivout0_clk"),
	DEV_CLK(107, 3, "mshsi2c_wkup_0_porscl"),
	DEV_CLK(107, 4, "hsdiv4_16fft_mcu_0_hsdivout1_clk"),
	DEV_CLK(135, 0, "hsdiv0_16fft_main_8_hsdivout0_clk"),
	DEV_CLK(136, 0, "hsdiv0_16fft_main_8_hsdivout0_clk"),
	DEV_CLK(140, 0, "sam62_pll_ctrl_wrap_mcu_0_chip_div1_clk_clk"),
	DEV_CLK(140, 1, "sam62_pll_ctrl_wrap_mcu_0_chip_div1_clk_clk"),
	DEV_CLK(146, 0, "main_usart0_fclk_sel_out0"),
	DEV_CLK(146, 1, "usart_programmable_clock_divider_out0"),
	DEV_CLK(146, 2, "hsdiv4_16fft_main_1_hsdivout1_clk"),
	DEV_CLK(146, 5, "sam62_pll_ctrl_wrap_main_0_chip_div1_clk_clk"),
	DEV_CLK(157, 20, "clkout0_ctrl_out0"),
	DEV_CLK(157, 21, "hsdiv4_16fft_main_2_hsdivout1_clk"),
	DEV_CLK(157, 22, "hsdiv4_16fft_main_2_hsdivout1_clk10"),
	DEV_CLK(157, 24, "sam62_pll_ctrl_wrap_main_0_chip_div1_clk_clk"),
	DEV_CLK(157, 25, "board_0_ddr0_ck0_out"),
	DEV_CLK(157, 40, "mshsi2c_main_0_porscl"),
	DEV_CLK(157, 77, "sam62_pll_ctrl_wrap_mcu_0_sysclkout_clk"),
	DEV_CLK(157, 82, "cpsw_3guss_main_0_mdio_mdclk_o"),
	DEV_CLK(157, 83, "emmcsd8ss_main_0_emmcsdss_io_clk_o"),
	DEV_CLK(157, 87, "emmcsd4ss_main_0_emmcsdss_io_clk_o"),
	DEV_CLK(157, 89, "emmcsd4ss_main_0_emmcsdss_io_clk_o"),
	DEV_CLK(157, 129, "fss_ul_main_0_ospi_0_ospi_oclk_clk"),
	DEV_CLK(157, 132, "cpsw_3guss_main_0_rgmii1_txc_o"),
	DEV_CLK(157, 135, "cpsw_3guss_main_0_rgmii2_txc_o"),
	DEV_CLK(157, 145, "sam62_pll_ctrl_wrap_main_0_sysclkout_clk"),
	DEV_CLK(157, 158, "wkup_clkout_sel_out0"),
	DEV_CLK(157, 159, "gluelogic_hfosc0_clkout"),
	DEV_CLK(157, 160, "gluelogic_lfosc0_clkout"),
	DEV_CLK(157, 161, "hsdiv4_16fft_main_0_hsdivout2_clk"),
	DEV_CLK(157, 162, "hsdiv4_16fft_main_1_hsdivout2_clk"),
	DEV_CLK(157, 163, "postdiv4_16ff_main_2_hsdivout9_clk"),
	DEV_CLK(157, 164, "clk_32k_rc_sel_out0"),
	DEV_CLK(157, 165, "gluelogic_rcosc_clkout"),
	DEV_CLK(157, 166, "gluelogic_hfosc0_clkout"),
	DEV_CLK(161, 0, "sam62_pll_ctrl_wrap_main_0_chip_div1_clk_clk"),
	DEV_CLK(161, 1, "sam62_pll_ctrl_wrap_main_0_chip_div1_clk_clk"),
	DEV_CLK(161, 2, "sam62_pll_ctrl_wrap_main_0_chip_div1_clk_clk"),
	DEV_CLK(161, 3, "main_usb0_refclk_sel_out0"),
	DEV_CLK(161, 4, "gluelogic_hfosc0_clkout"),
	DEV_CLK(161, 5, "postdiv4_16ff_main_0_hsdivout8_clk"),
	DEV_CLK(161, 10, "board_0_tck_out"),
	DEV_CLK(162, 0, "sam62_pll_ctrl_wrap_main_0_chip_div1_clk_clk"),
	DEV_CLK(162, 1, "sam62_pll_ctrl_wrap_main_0_chip_div1_clk_clk"),
	DEV_CLK(162, 2, "sam62_pll_ctrl_wrap_main_0_chip_div1_clk_clk"),
	DEV_CLK(162, 3, "main_usb1_refclk_sel_out0"),
	DEV_CLK(162, 4, "gluelogic_hfosc0_clkout"),
	DEV_CLK(162, 5, "postdiv4_16ff_main_0_hsdivout8_clk"),
	DEV_CLK(162, 10, "board_0_tck_out"),
	DEV_CLK(166, 3, "hsdiv0_16fft_main_8_hsdivout0_clk"),
	DEV_CLK(166, 5, "sam62_pll_ctrl_wrap_main_0_chip_div1_clk_clk"),
	DEV_CLK(169, 0, "sam62_pll_ctrl_wrap_main_0_chip_div1_clk_clk"),
	DEV_CLK(169, 1, "sam62_pll_ctrl_wrap_main_0_chip_div1_clk_clk"),
	DEV_CLK(170, 0, "hsdiv0_16fft_main_12_hsdivout0_clk"),
	DEV_CLK(170, 1, "board_0_tck_out"),
	DEV_CLK(170, 2, "sam62_pll_ctrl_wrap_main_0_chip_div1_clk_clk"),

	/*
	 * Timer clocks. There is a mux between the timer and the clock core
	 * which we haven't modeled here. The values reflect the reset default
	 * of the mux
	 */
	DEV_CLK(35, 2, "gluelogic_hfosc0_clkout"),
	DEV_CLK(36, 2, "gluelogic_hfosc0_clkout"),
	DEV_CLK(37, 2, "gluelogic_hfosc0_clkout"),
	DEV_CLK(38, 2, "gluelogic_hfosc0_clkout"),
	DEV_CLK(39, 2, "gluelogic_hfosc0_clkout"),
	DEV_CLK(40, 2, "gluelogic_hfosc0_clkout"),
	DEV_CLK(41, 2, "gluelogic_hfosc0_clkout"),
	DEV_CLK(42, 2, "gluelogic_hfosc0_clkout"),
	DEV_CLK(43, 2, "gluelogic_hfosc0_clkout"),
	DEV_CLK(48, 2, "gluelogic_hfosc0_clkout"),
	DEV_CLK(49, 2, "gluelogic_hfosc0_clkout"),
	DEV_CLK(50, 2, "gluelogic_hfosc0_clkout"),

	DEV_CLK(0, 0, NULL),
};

static struct clk *of_clk_ti_k3_get(struct of_phandle_args *clkspec, void *data)
{
	struct clk_lookup_data *clk_data = data;
	struct clk *clk;
	unsigned int dev_id = clkspec->args[0];
	unsigned int clk_id = clkspec->args[1];

	while (1) {
		if (!clk_data->clk_name) {
			pr_err("clk %d/%d not found\n", dev_id, clk_id);
			return ERR_PTR(-ENOENT);
		}

		if (clk_data->dev_id == dev_id && clk_data->clk_id == clk_id)
			goto found;

		clk_data++;
	}
found:
	clk = clk_lookup(clk_data->clk_name);
	if (IS_ERR(clk))
		pr_err("clk %s not found\n", clk_data->clk_name);

	return clk;
}

static int am625_clk_init(struct device *dev)
{
	int ret;

	clk_fixed("osc_26_mhz", 26000000);
	clk_fixed("osc_25_mhz", 25000000);
	clk_fixed("osc_24_mhz", 24000000);
	k3_clk_mux("gluelogic_hfosc0_clkout",
	 gluelogic_hfosc0_clkout_parents, 6, (void *)0x43000030, 0, 3, 0),
	clk_fixed("gluelogic_rcosc_clkout", 12500000);
	clk_fixed("gluelogic_rcosc_clk_1p0v_97p65k", 97656);
	clk_fixed("board_0_cp_gemac_cpts0_rft_clk_out", 0);
	clk_fixed("board_0_ddr0_ck0_out", 0);
	clk_fixed("board_0_ext_refclk1_out", 0);
	clk_fixed("board_0_i2c0_scl_out", 0);
	clk_fixed("board_0_mcu_ext_refclk0_out", 0);
	clk_fixed("board_0_mmc0_clklb_out", 0);
	clk_fixed("board_0_mmc0_clk_out", 0);
	clk_fixed("board_0_mmc1_clklb_out", 0);
	clk_fixed("board_0_mmc1_clk_out", 0);
	clk_fixed("board_0_ospi0_dqs_out", 0);
	clk_fixed("board_0_ospi0_lbclko_out", 0);
	clk_fixed("board_0_rgmii1_rxc_out", 0);
	clk_fixed("board_0_rgmii1_txc_out", 0);
	clk_fixed("board_0_rgmii2_rxc_out", 0);
	clk_fixed("board_0_rgmii2_txc_out", 0);
	clk_fixed("board_0_rmii1_ref_clk_out", 0);
	clk_fixed("board_0_rmii2_ref_clk_out", 0);
	clk_fixed("board_0_tck_out", 0);
	clk_fixed("cpsw_3guss_main_0_mdio_mdclk_o", 0);
	clk_fixed("cpsw_3guss_main_0_rgmii1_txc_o", 0);
	clk_fixed("cpsw_3guss_main_0_rgmii2_txc_o", 0);
	clk_fixed("emmcsd4ss_main_0_emmcsdss_io_clk_o", 0);
	clk_fixed("emmcsd8ss_main_0_emmcsdss_io_clk_o", 0);
	clk_fixed("fss_ul_main_0_ospi_0_ospi_oclk_clk", 0);
	k3_clk_divider("hsdiv0_16fft_mcu_32khz_gen_0_hsdivout0_clk", "gluelogic_hfosc0_clkout", (void *)0x4508030, 0, 7, 0, 0);
	clk_fixed("mshsi2c_main_0_porscl", 0);
	k3_clk_pll("pllfracf_ssmod_16fft_main_0_foutvcop_clk", "gluelogic_hfosc0_clkout", (void *)0x680000, 0);
	k3_clk_divider("pllfracf_ssmod_16fft_main_0_foutpostdiv_clk_subdiv", "pllfracf_ssmod_16fft_main_0_foutvcop_clk", (void *)0x680038, 16, 3, 0, CLK_DIVIDER_ONE_BASED);
	k3_clk_divider("pllfracf_ssmod_16fft_main_0_foutpostdiv_clk", "pllfracf_ssmod_16fft_main_0_foutpostdiv_clk_subdiv", (void *)0x680038, 24, 3, 0, CLK_DIVIDER_ONE_BASED);
	k3_clk_pll("pllfracf_ssmod_16fft_main_1_foutvcop_clk", "gluelogic_hfosc0_clkout", (void *)0x681000, 0);
	k3_clk_divider("pllfracf_ssmod_16fft_main_1_foutpostdiv_clk_subdiv", "pllfracf_ssmod_16fft_main_1_foutvcop_clk", (void *)0x681038, 16, 3, 0, CLK_DIVIDER_ONE_BASED);
	k3_clk_divider("pllfracf_ssmod_16fft_main_1_foutpostdiv_clk", "pllfracf_ssmod_16fft_main_1_foutpostdiv_clk_subdiv", (void *)0x681038, 24, 3, 0, CLK_DIVIDER_ONE_BASED);
	k3_clk_pll("pllfracf_ssmod_16fft_main_12_foutvcop_clk", "gluelogic_hfosc0_clkout", (void *)0x68c000, 0);
	k3_clk_pll("pllfracf_ssmod_16fft_main_15_foutvcop_clk", "gluelogic_hfosc0_clkout", (void *)0x68f000, 0);
	k3_clk_pll("pllfracf_ssmod_16fft_main_2_foutvcop_clk", "gluelogic_hfosc0_clkout", (void *)0x682000, 0);
	k3_clk_divider("pllfracf_ssmod_16fft_main_2_foutpostdiv_clk_subdiv", "pllfracf_ssmod_16fft_main_2_foutvcop_clk", (void *)0x682038, 16, 3, 0, CLK_DIVIDER_ONE_BASED);
	k3_clk_divider("pllfracf_ssmod_16fft_main_2_foutpostdiv_clk", "pllfracf_ssmod_16fft_main_2_foutpostdiv_clk_subdiv", (void *)0x682038, 24, 3, 0, CLK_DIVIDER_ONE_BASED);
	k3_clk_pll("pllfracf_ssmod_16fft_main_8_foutvcop_clk", "gluelogic_hfosc0_clkout", (void *)0x688000, 0);
	k3_clk_pll("pllfracf_ssmod_16fft_mcu_0_foutvcop_clk", "gluelogic_hfosc0_clkout", (void *)0x4040000, 0);
	k3_clk_divider("postdiv1_16fft_main_1_hsdivout5_clk", "pllfracf_ssmod_16fft_main_1_foutpostdiv_clk", (void *)0x681094, 0, 7, 0, 0);
	k3_clk_divider("postdiv4_16ff_main_0_hsdivout5_clk", "pllfracf_ssmod_16fft_main_0_foutpostdiv_clk", (void *)0x680094, 0, 7, 0, 0);
	k3_clk_divider("postdiv4_16ff_main_0_hsdivout6_clk", "pllfracf_ssmod_16fft_main_0_foutpostdiv_clk", (void *)0x680098, 0, 7, 0, 0);
	k3_clk_divider("postdiv4_16ff_main_0_hsdivout8_clk", "pllfracf_ssmod_16fft_main_0_foutpostdiv_clk", (void *)0x6800a0, 0, 7, 0, 0);
	k3_clk_divider("postdiv4_16ff_main_2_hsdivout5_clk", "pllfracf_ssmod_16fft_main_2_foutpostdiv_clk", (void *)0x682094, 0, 7, 0, 0);
	k3_clk_divider("postdiv4_16ff_main_2_hsdivout8_clk", "pllfracf_ssmod_16fft_main_2_foutpostdiv_clk", (void *)0x6820a0, 0, 7, 0, 0);
	k3_clk_divider("postdiv4_16ff_main_2_hsdivout9_clk", "pllfracf_ssmod_16fft_main_2_foutpostdiv_clk", (void *)0x6820a4, 0, 7, 0, 0);
	k3_clk_mux("main_emmcsd0_io_clklb_sel_out0", main_emmcsd0_io_clklb_sel_out0_parents, 2, (void *)0x108160, 16, 1, 0);
	k3_clk_mux("main_emmcsd1_io_clklb_sel_out0", main_emmcsd1_io_clklb_sel_out0_parents, 2, (void *)0x108168, 16, 1, 0);
	k3_clk_mux("main_ospi_loopback_clk_sel_out0", main_ospi_loopback_clk_sel_out0_parents, 2, (void *)0x108500, 4, 1, 0);
	k3_clk_mux("main_usb0_refclk_sel_out0", main_usb0_refclk_sel_out0_parents, 2, (void *)0x43008190, 0, 1, 0);
	k3_clk_mux("main_usb1_refclk_sel_out0", main_usb1_refclk_sel_out0_parents, 2, (void *)0x43008194, 0, 1, 0);
	k3_clk_divider("hsdiv0_16fft_main_12_hsdivout0_clk", "pllfracf_ssmod_16fft_main_12_foutvcop_clk", (void *)0x68c080, 0, 7, 0, 0);
	k3_clk_divider("hsdiv0_16fft_main_8_hsdivout0_clk", "pllfracf_ssmod_16fft_main_8_foutvcop_clk", (void *)0x688080, 0, 7, 0, 0);
	k3_clk_divider("hsdiv1_16fft_main_15_hsdivout0_clk", "pllfracf_ssmod_16fft_main_15_foutvcop_clk", (void *)0x68f080, 0, 7, 0, 0);
	k3_clk_divider("hsdiv4_16fft_main_0_hsdivout0_clk", "pllfracf_ssmod_16fft_main_0_foutvcop_clk", (void *)0x680080, 0, 7, 0, 0);
	k3_clk_divider("hsdiv4_16fft_main_0_hsdivout1_clk", "pllfracf_ssmod_16fft_main_0_foutvcop_clk", (void *)0x680084, 0, 7, 0, 0);
	k3_clk_divider("hsdiv4_16fft_main_0_hsdivout2_clk", "pllfracf_ssmod_16fft_main_0_foutvcop_clk", (void *)0x680088, 0, 7, 0, 0);
	k3_clk_divider("hsdiv4_16fft_main_0_hsdivout3_clk", "pllfracf_ssmod_16fft_main_0_foutvcop_clk", (void *)0x68008c, 0, 7, 0, 0);
	k3_clk_divider("hsdiv4_16fft_main_0_hsdivout4_clk", "pllfracf_ssmod_16fft_main_0_foutvcop_clk", (void *)0x680090, 0, 7, 0, 0);
	k3_clk_div_devfreq("hsdiv4_16fft_main_1_hsdivout0_clk", "pllfracf_ssmod_16fft_main_1_foutvcop_clk", (void *)0x681080, 0, 7, 0, 0, 192000000);
	k3_clk_divider("hsdiv4_16fft_main_1_hsdivout1_clk", "pllfracf_ssmod_16fft_main_1_foutvcop_clk", (void *)0x681084, 0, 7, 0, 0);
	k3_clk_divider("hsdiv4_16fft_main_1_hsdivout2_clk", "pllfracf_ssmod_16fft_main_1_foutvcop_clk", (void *)0x681088, 0, 7, 0, 0);
	k3_clk_divider("hsdiv4_16fft_main_2_hsdivout1_clk", "pllfracf_ssmod_16fft_main_2_foutvcop_clk", (void *)0x682084, 0, 7, 0, 0);
	k3_clk_divider("hsdiv4_16fft_main_2_hsdivout1_clk10", "pllfracf_ssmod_16fft_main_2_foutvcop_clk", (void *)0x682084, 0, 7, 0, 0);
	k3_clk_divider("hsdiv4_16fft_main_2_hsdivout2_clk", "pllfracf_ssmod_16fft_main_2_foutvcop_clk", (void *)0x682088, 0, 7, 0, 0);
	k3_clk_divider("hsdiv4_16fft_mcu_0_hsdivout0_clk", "pllfracf_ssmod_16fft_mcu_0_foutvcop_clk", (void *)0x4040080, 0, 7, 0, 0);
	k3_clk_mux_pllctrl("sam62_pll_ctrl_wrap_main_0_sysclkout_clk", sam62_pll_ctrl_wrap_main_0_sysclkout_clk_parents, 2, (void *)0x410000, 0);
	k3_clk_divider("sam62_pll_ctrl_wrap_main_0_chip_div1_clk_clk", "sam62_pll_ctrl_wrap_main_0_sysclkout_clk", (void *)0x410118, 0, 5, 0, 0);
	k3_clk_mux_pllctrl("sam62_pll_ctrl_wrap_mcu_0_sysclkout_clk", sam62_pll_ctrl_wrap_mcu_0_sysclkout_clk_parents, 2, (void *)0x4020000, 0);
	k3_clk_divider("sam62_pll_ctrl_wrap_mcu_0_chip_div1_clk_clk", "sam62_pll_ctrl_wrap_mcu_0_sysclkout_clk", (void *)0x4020118, 0, 5, 0, 0);
	k3_clk_mux("clkout0_ctrl_out0", clkout0_ctrl_out0_parents, 2, (void *)0x108010, 0, 1, 0);
	k3_clk_mux("clk_32k_rc_sel_out0", clk_32k_rc_sel_out0_parents, 4, (void *)0x4508058, 0, 2, 0);
	k3_clk_mux("main_cp_gemac_cpts_clk_sel_out0", main_cp_gemac_cpts_clk_sel_out0_parents, 8, (void *)0x108140, 0, 3, 0);
	k3_clk_mux("main_emmcsd0_refclk_sel_out0", main_emmcsd0_refclk_sel_out0_parents, 2, (void *)0x108160, 0, 1, 0);
	k3_clk_mux("main_emmcsd1_refclk_sel_out0", main_emmcsd1_refclk_sel_out0_parents, 2, (void *)0x108168, 0, 1, 0);
	k3_clk_mux("main_gtcclk_sel_out0", main_gtcclk_sel_out0_parents, 8, (void *)0x43008030, 0, 3, 0);
	k3_clk_mux("main_ospi_ref_clk_sel_out0", main_ospi_ref_clk_sel_out0_parents, 2, (void *)0x108500, 0, 1, 0);
	k3_clk_div_devfreq("usart_programmable_clock_divider_out0", "hsdiv4_16fft_main_1_hsdivout0_clk", (void *)0x108240, 0, 2, 0, 0, 48000000);
	k3_clk_mux("wkup_clkout_sel_out0", wkup_clkout_sel_out0_parents, 8, (void *)0x43008020, 0, 3, 0);
	k3_clk_mux("wkup_clksel_out0", wkup_clksel_out0_parents, 2, (void *)0x43008010, 0, 1, 0);
	k3_clk_mux("main_usart0_fclk_sel_out0", main_usart0_fclk_sel_out0_parents, 2, (void *)0x108280, 0, 1, 0);
	k3_clk_divider("hsdiv4_16fft_mcu_0_hsdivout1_clk", "pllfracf_ssmod_16fft_mcu_0_foutvcop_clk", (void *)0x4040084, 0, 7, 0, 0);
	clk_fixed("mshsi2c_wkup_0_porscl", 0);
	k3_clk_divider("sam62_pll_ctrl_wrap_main_0_chip_div24_clk_clk", "sam62_pll_ctrl_wrap_main_0_sysclkout_clk", (void *)0x41011c, 0, 5, 0, 0);
	k3_clk_divider("sam62_pll_ctrl_wrap_mcu_0_chip_div24_clk_clk", "sam62_pll_ctrl_wrap_mcu_0_sysclkout_clk", (void *)0x402011c, 0, 5, 0, 0);

	ret = of_clk_add_provider(dev->of_node, of_clk_ti_k3_get, &am625_lookup);
	if (ret < 0)
		pr_err("failed to register clks for i.MX8MM\n");

	return 0;
};

static int ti_k3_clk_probe(struct device *dev)
{
	if (of_machine_is_compatible("ti,am625"))
                return am625_clk_init(dev);

	return dev_err_probe(dev, -EINVAL, "Unknown SoC\n");
}

static __maybe_unused struct of_device_id ti_k3_clk_dt_ids[] = {
	{
		.compatible = "ti,k2g-sci-clk",
	}, {
		/* sentinel */
	}
};

static struct driver ti_k3_clk_driver = {
	.probe = ti_k3_clk_probe,
	.name = "ti-k3-clk",
	.of_compatible = ti_k3_clk_dt_ids,
};

core_platform_driver(ti_k3_clk_driver);
