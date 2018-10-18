/*
 * Copyright (C) 2014 Lucas Stach <l.stach@pengutronix.de>
 *
 * Partly based on code
 * Copyright (C) 2011-2012, NVIDIA CORPORATION.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <common.h>
#include <init.h>
#include <io.h>
#include <malloc.h>
#include <pinctrl.h>
#include <linux/err.h>

struct pinctrl_tegra30_drvdata;

struct pinctrl_tegra30 {
	struct {
		u32 __iomem *ctrl;
		u32 __iomem *mux;
	} regs;
	struct pinctrl_device pinctrl;
	struct pinctrl_tegra30_drvdata *drvdata;
};

struct tegra_pingroup {
	const char *name;
	const char *funcs[4];
	u16 reg;
};

struct tegra_drive_pingroup {
	const char *name;
	u16 reg;
	u32 hsm_bit:5;
	u32 schmitt_bit:5;
	u32 lpmd_bit:5;
	u32 drvdn_bit:5;
	u32 drvup_bit:5;
	u32 slwr_bit:5;
	u32 slwf_bit:5;
	u32 drvtype_bit:5;
	u32 drvdn_width:6;
	u32 drvup_width:6;
	u32 slwr_width:6;
	u32 slwf_width:6;
};

struct pinctrl_tegra30_drvdata {
	const struct tegra_pingroup *pingrps;
	const unsigned int num_pingrps;
	const struct tegra_drive_pingroup *drvgrps;
	const unsigned int num_drvgrps;
};

#define PG(pg_name, f0, f1, f2, f3, offset)		\
	{						\
		.name = #pg_name,			\
		.funcs = { #f0, #f1, #f2, #f3, },	\
		.reg = offset				\
	}

#define DRV_PG(pg_name, r, hsm_b, schmitt_b, lpmd_b,	\
               drvdn_b, drvdn_w, drvup_b, drvup_w,	\
               slwr_b, slwr_w, slwf_b, slwf_w)		\
	{						\
		.name = "drive_" #pg_name,		\
		.reg = r - 0x868,			\
		.hsm_bit = hsm_b,			\
		.schmitt_bit = schmitt_b,		\
		.lpmd_bit = lpmd_b,			\
		.drvdn_bit = drvdn_b,			\
		.drvdn_width = drvdn_w,			\
		.drvup_bit = drvup_b,			\
		.drvup_width = drvup_w,			\
		.slwr_bit = slwr_b,			\
		.slwr_width = slwr_w,			\
		.slwf_bit = slwf_b,			\
		.slwf_width = slwf_w,			\
	}

static const struct tegra_pingroup tegra30_pin_groups[] = {
	/* name,                 f0,        f1,        f2,        f3,           reg  */
	PG(clk_32k_out_pa0,      blink,     rsvd2,     rsvd3,     rsvd4,        0x31c),
	PG(uart3_cts_n_pa1,      uartc,     rsvd2,     gmi,       rsvd4,        0x17c),
	PG(dap2_fs_pa2,          i2s1,      hda,       rsvd3,     gmi,          0x358),
	PG(dap2_sclk_pa3,        i2s1,      hda,       rsvd3,     gmi,          0x364),
	PG(dap2_din_pa4,         i2s1,      hda,       rsvd3,     gmi,          0x35c),
	PG(dap2_dout_pa5,        i2s1,      hda,       rsvd3,     gmi,          0x360),
	PG(sdmmc3_clk_pa6,       uarta,     pwm2,      sdmmc3,    spi3,         0x390),
	PG(sdmmc3_cmd_pa7,       uarta,     pwm3,      sdmmc3,    spi2,         0x394),
	PG(gmi_a17_pb0,          uartd,     spi4,      gmi,       dtv,          0x234),
	PG(gmi_a18_pb1,          uartd,     spi4,      gmi,       dtv,          0x238),
	PG(lcd_pwr0_pb2,         displaya,  displayb,  spi5,      hdcp,         0x090),
	PG(lcd_pclk_pb3,         displaya,  displayb,  rsvd3,     rsvd4,        0x094),
	PG(sdmmc3_dat3_pb4,      rsvd1,     pwm0,      sdmmc3,    spi3,         0x3a4),
	PG(sdmmc3_dat2_pb5,      rsvd1,     pwm1,      sdmmc3,    spi3,         0x3a0),
	PG(sdmmc3_dat1_pb6,      rsvd1,     rsvd2,     sdmmc3,    spi3,         0x39c),
	PG(sdmmc3_dat0_pb7,      rsvd1,     rsvd2,     sdmmc3,    spi3,         0x398),
	PG(uart3_rts_n_pc0,      uartc,     pwm0,      gmi,       rsvd4,        0x180),
	PG(lcd_pwr1_pc1,         displaya,  displayb,  rsvd3,     rsvd4,        0x070),
	PG(uart2_txd_pc2,        uartb,     spdif,     uarta,     spi4,         0x168),
	PG(uart2_rxd_pc3,        uartb,     spdif,     uarta,     spi4,         0x164),
	PG(gen1_i2c_scl_pc4,     i2c1,      rsvd2,     rsvd3,     rsvd4,        0x1a4),
	PG(gen1_i2c_sda_pc5,     i2c1,      rsvd2,     rsvd3,     rsvd4,        0x1a0),
	PG(lcd_pwr2_pc6,         displaya,  displayb,  spi5,      hdcp,         0x074),
	PG(gmi_wp_n_pc7,         rsvd1,     nand,      gmi,       gmi_alt,      0x1c0),
	PG(sdmmc3_dat5_pd0,      pwm0,      spi4,      sdmmc3,    spi2,         0x3ac),
	PG(sdmmc3_dat4_pd1,      pwm1,      spi4,      sdmmc3,    spi2,         0x3a8),
	PG(lcd_dc1_pd2,          displaya,  displayb,  rsvd3,     rsvd4,        0x10c),
	PG(sdmmc3_dat6_pd3,      spdif,     spi4,      sdmmc3,    spi2,         0x3b0),
	PG(sdmmc3_dat7_pd4,      spdif,     spi4,      sdmmc3,    spi2,         0x3b4),
	PG(vi_d1_pd5,            ddr,       sdmmc2,    vi,        rsvd4,        0x128),
	PG(vi_vsync_pd6,         ddr,       rsvd2,     vi,        rsvd4,        0x15c),
	PG(vi_hsync_pd7,         ddr,       rsvd2,     vi,        rsvd4,        0x160),
	PG(lcd_d0_pe0,           displaya,  displayb,  rsvd3,     rsvd4,        0x0a4),
	PG(lcd_d1_pe1,           displaya,  displayb,  rsvd3,     rsvd4,        0x0a8),
	PG(lcd_d2_pe2,           displaya,  displayb,  rsvd3,     rsvd4,        0x0ac),
	PG(lcd_d3_pe3,           displaya,  displayb,  rsvd3,     rsvd4,        0x0b0),
	PG(lcd_d4_pe4,           displaya,  displayb,  rsvd3,     rsvd4,        0x0b4),
	PG(lcd_d5_pe5,           displaya,  displayb,  rsvd3,     rsvd4,        0x0b8),
	PG(lcd_d6_pe6,           displaya,  displayb,  rsvd3,     rsvd4,        0x0bc),
	PG(lcd_d7_pe7,           displaya,  displayb,  rsvd3,     rsvd4,        0x0c0),
	PG(lcd_d8_pf0,           displaya,  displayb,  rsvd3,     rsvd4,        0x0c4),
	PG(lcd_d9_pf1,           displaya,  displayb,  rsvd3,     rsvd4,        0x0c8),
	PG(lcd_d10_pf2,          displaya,  displayb,  rsvd3,     rsvd4,        0x0cc),
	PG(lcd_d11_pf3,          displaya,  displayb,  rsvd3,     rsvd4,        0x0d0),
	PG(lcd_d12_pf4,          displaya,  displayb,  rsvd3,     rsvd4,        0x0d4),
	PG(lcd_d13_pf5,          displaya,  displayb,  rsvd3,     rsvd4,        0x0d8),
	PG(lcd_d14_pf6,          displaya,  displayb,  rsvd3,     rsvd4,        0x0dc),
	PG(lcd_d15_pf7,          displaya,  displayb,  rsvd3,     rsvd4,        0x0e0),
	PG(gmi_ad0_pg0,          rsvd1,     nand,      gmi,       rsvd4,        0x1f0),
	PG(gmi_ad1_pg1,          rsvd1,     nand,      gmi,       rsvd4,        0x1f4),
	PG(gmi_ad2_pg2,          rsvd1,     nand,      gmi,       rsvd4,        0x1f8),
	PG(gmi_ad3_pg3,          rsvd1,     nand,      gmi,       rsvd4,        0x1fc),
	PG(gmi_ad4_pg4,          rsvd1,     nand,      gmi,       rsvd4,        0x200),
	PG(gmi_ad5_pg5,          rsvd1,     nand,      gmi,       rsvd4,        0x204),
	PG(gmi_ad6_pg6,          rsvd1,     nand,      gmi,       rsvd4,        0x208),
	PG(gmi_ad7_pg7,          rsvd1,     nand,      gmi,       rsvd4,        0x20c),
	PG(gmi_ad8_ph0,          pwm0,      nand,      gmi,       rsvd4,        0x210),
	PG(gmi_ad9_ph1,          pwm1,      nand,      gmi,       rsvd4,        0x214),
	PG(gmi_ad10_ph2,         pwm2,      nand,      gmi,       rsvd4,        0x218),
	PG(gmi_ad11_ph3,         pwm3,      nand,      gmi,       rsvd4,        0x21c),
	PG(gmi_ad12_ph4,         rsvd1,     nand,      gmi,       rsvd4,        0x220),
	PG(gmi_ad13_ph5,         rsvd1,     nand,      gmi,       rsvd4,        0x224),
	PG(gmi_ad14_ph6,         rsvd1,     nand,      gmi,       rsvd4,        0x228),
	PG(gmi_ad15_ph7,         rsvd1,     nand,      gmi,       rsvd4,        0x22c),
	PG(gmi_wr_n_pi0,         rsvd1,     nand,      gmi,       rsvd4,        0x240),
	PG(gmi_oe_n_pi1,         rsvd1,     nand,      gmi,       rsvd4,        0x244),
	PG(gmi_dqs_pi2,          rsvd1,     nand,      gmi,       rsvd4,        0x248),
	PG(gmi_cs6_n_pi3,        nand,      nand_alt,  gmi,       sata,         0x1e8),
	PG(gmi_rst_n_pi4,        nand,      nand_alt,  gmi,       rsvd4,        0x24c),
	PG(gmi_iordy_pi5,        rsvd1,     nand,      gmi,       rsvd4,        0x1c4),
	PG(gmi_cs7_n_pi6,        nand,      nand_alt,  gmi,       gmi_alt,      0x1ec),
	PG(gmi_wait_pi7,         rsvd1,     nand,      gmi,       rsvd4,        0x1c8),
	PG(gmi_cs0_n_pj0,        rsvd1,     nand,      gmi,       dtv,          0x1d4),
	PG(lcd_de_pj1,           displaya,  displayb,  rsvd3,     rsvd4,        0x098),
	PG(gmi_cs1_n_pj2,        rsvd1,     nand,      gmi,       dtv,          0x1d8),
	PG(lcd_hsync_pj3,        displaya,  displayb,  rsvd3,     rsvd4,        0x09c),
	PG(lcd_vsync_pj4,        displaya,  displayb,  rsvd3,     rsvd4,        0x0a0),
	PG(uart2_cts_n_pj5,      uarta,     uartb,     gmi,       spi4,         0x170),
	PG(uart2_rts_n_pj6,      uarta,     uartb,     gmi,       spi4,         0x16c),
	PG(gmi_a16_pj7,          uartd,     spi4,      gmi,       gmi_alt,      0x230),
	PG(gmi_adv_n_pk0,        rsvd1,     nand,      gmi,       rsvd4,        0x1cc),
	PG(gmi_clk_pk1,          rsvd1,     nand,      gmi,       rsvd4,        0x1d0),
	PG(gmi_cs4_n_pk2,        rsvd1,     nand,      gmi,       rsvd4,        0x1e4),
	PG(gmi_cs2_n_pk3,        rsvd1,     nand,      gmi,       rsvd4,        0x1dc),
	PG(gmi_cs3_n_pk4,        rsvd1,     nand,      gmi,       gmi_alt,      0x1e0),
	PG(spdif_out_pk5,        spdif,     rsvd2,     i2c1,      sdmmc2,       0x354),
	PG(spdif_in_pk6,         spdif,     hda,       i2c1,      sdmmc2,       0x350),
	PG(gmi_a19_pk7,          uartd,     spi4,      gmi,       rsvd4,        0x23c),
	PG(vi_d2_pl0,            ddr,       sdmmc2,    vi,        rsvd4,        0x12c),
	PG(vi_d3_pl1,            ddr,       sdmmc2,    vi,        rsvd4,        0x130),
	PG(vi_d4_pl2,            ddr,       sdmmc2,    vi,        rsvd4,        0x134),
	PG(vi_d5_pl3,            ddr,       sdmmc2,    vi,        rsvd4,        0x138),
	PG(vi_d6_pl4,            ddr,       sdmmc2,    vi,        rsvd4,        0x13c),
	PG(vi_d7_pl5,            ddr,       sdmmc2,    vi,        rsvd4,        0x140),
	PG(vi_d8_pl6,            ddr,       sdmmc2,    vi,        rsvd4,        0x144),
	PG(vi_d9_pl7,            ddr,       sdmmc2,    vi,        rsvd4,        0x148),
	PG(lcd_d16_pm0,          displaya,  displayb,  rsvd3,     rsvd4,        0x0e4),
	PG(lcd_d17_pm1,          displaya,  displayb,  rsvd3,     rsvd4,        0x0e8),
	PG(lcd_d18_pm2,          displaya,  displayb,  rsvd3,     rsvd4,        0x0ec),
	PG(lcd_d19_pm3,          displaya,  displayb,  rsvd3,     rsvd4,        0x0f0),
	PG(lcd_d20_pm4,          displaya,  displayb,  rsvd3,     rsvd4,        0x0f4),
	PG(lcd_d21_pm5,          displaya,  displayb,  rsvd3,     rsvd4,        0x0f8),
	PG(lcd_d22_pm6,          displaya,  displayb,  rsvd3,     rsvd4,        0x0fc),
	PG(lcd_d23_pm7,          displaya,  displayb,  rsvd3,     rsvd4,        0x100),
	PG(dap1_fs_pn0,          i2s0,      hda,       gmi,       sdmmc2,       0x338),
	PG(dap1_din_pn1,         i2s0,      hda,       gmi,       sdmmc2,       0x33c),
	PG(dap1_dout_pn2,        i2s0,      hda,       gmi,       sdmmc2,       0x340),
	PG(dap1_sclk_pn3,        i2s0,      hda,       gmi,       sdmmc2,       0x344),
	PG(lcd_cs0_n_pn4,        displaya,  displayb,  spi5,      rsvd4,        0x084),
	PG(lcd_sdout_pn5,        displaya,  displayb,  spi5,      hdcp,         0x07c),
	PG(lcd_dc0_pn6,          displaya,  displayb,  rsvd3,     rsvd4,        0x088),
	PG(hdmi_int_pn7,         hdmi,      rsvd2,     rsvd3,     rsvd4,        0x110),
	PG(ulpi_data7_po0,       spi2,      hsi,       uarta,     ulpi,         0x01c),
	PG(ulpi_data0_po1,       spi3,      hsi,       uarta,     ulpi,         0x000),
	PG(ulpi_data1_po2,       spi3,      hsi,       uarta,     ulpi,         0x004),
	PG(ulpi_data2_po3,       spi3,      hsi,       uarta,     ulpi,         0x008),
	PG(ulpi_data3_po4,       spi3,      hsi,       uarta,     ulpi,         0x00c),
	PG(ulpi_data4_po5,       spi2,      hsi,       uarta,     ulpi,         0x010),
	PG(ulpi_data5_po6,       spi2,      hsi,       uarta,     ulpi,         0x014),
	PG(ulpi_data6_po7,       spi2,      hsi,       uarta,     ulpi,         0x018),
	PG(dap3_fs_pp0,          i2s2,      rsvd2,     displaya,  displayb,     0x030),
	PG(dap3_din_pp1,         i2s2,      rsvd2,     displaya,  displayb,     0x034),
	PG(dap3_dout_pp2,        i2s2,      rsvd2,     displaya,  displayb,     0x038),
	PG(dap3_sclk_pp3,        i2s2,      rsvd2,     displaya,  displayb,     0x03c),
	PG(dap4_fs_pp4,          i2s3,      rsvd2,     gmi,       rsvd4,        0x1a8),
	PG(dap4_din_pp5,         i2s3,      rsvd2,     gmi,       rsvd4,        0x1ac),
	PG(dap4_dout_pp6,        i2s3,      rsvd2,     gmi,       rsvd4,        0x1b0),
	PG(dap4_sclk_pp7,        i2s3,      rsvd2,     gmi,       rsvd4,        0x1b4),
	PG(kb_col0_pq0,          kbc,       nand,      trace,     test,         0x2fc),
	PG(kb_col1_pq1,          kbc,       nand,      trace,     test,         0x300),
	PG(kb_col2_pq2,          kbc,       nand,      trace,     rsvd4,        0x304),
	PG(kb_col3_pq3,          kbc,       nand,      trace,     rsvd4,        0x308),
	PG(kb_col4_pq4,          kbc,       nand,      trace,     rsvd4,        0x30c),
	PG(kb_col5_pq5,          kbc,       nand,      trace,     rsvd4,        0x310),
	PG(kb_col6_pq6,          kbc,       nand,      trace,     mio,          0x314),
	PG(kb_col7_pq7,          kbc,       nand,      trace,     mio,          0x318),
	PG(kb_row0_pr0,          kbc,       nand,      rsvd3,     rsvd4,        0x2bc),
	PG(kb_row1_pr1,          kbc,       nand,      rsvd3,     rsvd4,        0x2c0),
	PG(kb_row2_pr2,          kbc,       nand,      rsvd3,     rsvd4,        0x2c4),
	PG(kb_row3_pr3,          kbc,       nand,      rsvd3,     invalid,      0x2c8),
	PG(kb_row4_pr4,          kbc,       nand,      trace,     rsvd4,        0x2cc),
	PG(kb_row5_pr5,          kbc,       nand,      trace,     owr,          0x2d0),
	PG(kb_row6_pr6,          kbc,       nand,      sdmmc2,    mio,          0x2d4),
	PG(kb_row7_pr7,          kbc,       nand,      sdmmc2,    mio,          0x2d8),
	PG(kb_row8_ps0,          kbc,       nand,      sdmmc2,    mio,          0x2dc),
	PG(kb_row9_ps1,          kbc,       nand,      sdmmc2,    mio,          0x2e0),
	PG(kb_row10_ps2,         kbc,       nand,      sdmmc2,    mio,          0x2e4),
	PG(kb_row11_ps3,         kbc,       nand,      sdmmc2,    mio,          0x2e8),
	PG(kb_row12_ps4,         kbc,       nand,      sdmmc2,    mio,          0x2ec),
	PG(kb_row13_ps5,         kbc,       nand,      sdmmc2,    mio,          0x2f0),
	PG(kb_row14_ps6,         kbc,       nand,      sdmmc2,    mio,          0x2f4),
	PG(kb_row15_ps7,         kbc,       nand,      sdmmc2,    mio,          0x2f8),
	PG(vi_pclk_pt0,          rsvd1,     sdmmc2,    vi,        rsvd4,        0x154),
	PG(vi_mclk_pt1,          vi,        vi_alt1,   vi_alt2,   vi_alt3,      0x158),
	PG(vi_d10_pt2,           ddr,       rsvd2,     vi,        rsvd4,        0x14c),
	PG(vi_d11_pt3,           ddr,       rsvd2,     vi,        rsvd4,        0x150),
	PG(vi_d0_pt4,            ddr,       rsvd2,     vi,        rsvd4,        0x124),
	PG(gen2_i2c_scl_pt5,     i2c2,      hdcp,      gmi,       rsvd4,        0x250),
	PG(gen2_i2c_sda_pt6,     i2c2,      hdcp,      gmi,       rsvd4,        0x254),
	PG(sdmmc4_cmd_pt7,       i2c3,      nand,      gmi,       sdmmc4,       0x25c),
	PG(pu0,                  owr,       uarta,     gmi,       rsvd4,        0x184),
	PG(pu1,                  rsvd1,     uarta,     gmi,       rsvd4,        0x188),
	PG(pu2,                  rsvd1,     uarta,     gmi,       rsvd4,        0x18c),
	PG(pu3,                  pwm0,      uarta,     gmi,       rsvd4,        0x190),
	PG(pu4,                  pwm1,      uarta,     gmi,       rsvd4,        0x194),
	PG(pu5,                  pwm2,      uarta,     gmi,       rsvd4,        0x198),
	PG(pu6,                  pwm3,      uarta,     gmi,       rsvd4,        0x19c),
	PG(jtag_rtck_pu7,        rtck,      rsvd2,     rsvd3,     rsvd4,        0x2b0),
	PG(pv0,                  rsvd1,     rsvd2,     rsvd3,     rsvd4,        0x040),
	PG(pv1,                  rsvd1,     rsvd2,     rsvd3,     rsvd4,        0x044),
	PG(pv2,                  owr,       rsvd2,     rsvd3,     rsvd4,        0x060),
	PG(pv3,                  clk_12m_out, rsvd2,   rsvd3,     rsvd4,        0x064),
	PG(ddc_scl_pv4,          i2c4,      rsvd2,     rsvd3,     rsvd4,        0x114),
	PG(ddc_sda_pv5,          i2c4,      rsvd2,     rsvd3,     rsvd4,        0x118),
	PG(crt_hsync_pv6,        crt,       rsvd2,     rsvd3,     rsvd4,        0x11c),
	PG(crt_vsync_pv7,        crt,       rsvd2,     rsvd3,     rsvd4,        0x120),
	PG(lcd_cs1_n_pw0,        displaya,  displayb,  spi5,      rsvd4,        0x104),
	PG(lcd_m1_pw1,           displaya,  displayb,  rsvd3,     rsvd4,        0x108),
	PG(spi2_cs1_n_pw2,       spi3,      spi2,      spi2_alt,  i2c1,         0x388),
	PG(spi2_cs2_n_pw3,       spi3,      spi2,      spi2_alt,  i2c1,         0x38c),
	PG(clk1_out_pw4,         extperiph1, rsvd2,    rsvd3,     rsvd4,        0x34c),
	PG(clk2_out_pw5,         extperiph2, rsvd2,    rsvd3,     rsvd4,        0x068),
	PG(uart3_txd_pw6,        uartc,     rsvd2,     gmi,       rsvd4,        0x174),
	PG(uart3_rxd_pw7,        uartc,     rsvd2,     gmi,       rsvd4,        0x178),
	PG(spi2_mosi_px0,        spi6,      spi2,      spi3,      gmi,          0x368),
	PG(spi2_miso_px1,        spi6,      spi2,      spi3,      gmi,          0x36c),
	PG(spi2_sck_px2,         spi6,      spi2,      spi3,      gmi,          0x374),
	PG(spi2_cs0_n_px3,       spi6,      spi2,      spi3,      gmi,          0x370),
	PG(spi1_mosi_px4,        spi2,      spi1,      spi2_alt,  gmi,          0x378),
	PG(spi1_sck_px5,         spi2,      spi1,      spi2_alt,  gmi,          0x37c),
	PG(spi1_cs0_n_px6,       spi2,      spi1,      spi2_alt,  gmi,          0x380),
	PG(spi1_miso_px7,        spi3,      spi1,      spi2_alt,  rsvd4,        0x384),
	PG(ulpi_clk_py0,         spi1,      rsvd2,     uartd,     ulpi,         0x020),
	PG(ulpi_dir_py1,         spi1,      rsvd2,     uartd,     ulpi,         0x024),
	PG(ulpi_nxt_py2,         spi1,      rsvd2,     uartd,     ulpi,         0x028),
	PG(ulpi_stp_py3,         spi1,      rsvd2,     uartd,     ulpi,         0x02c),
	PG(sdmmc1_dat3_py4,      sdmmc1,    rsvd2,     uarte,     uarta,        0x050),
	PG(sdmmc1_dat2_py5,      sdmmc1,    rsvd2,     uarte,     uarta,        0x054),
	PG(sdmmc1_dat1_py6,      sdmmc1,    rsvd2,     uarte,     uarta,        0x058),
	PG(sdmmc1_dat0_py7,      sdmmc1,    rsvd2,     uarte,     uarta,        0x05c),
	PG(sdmmc1_clk_pz0,       sdmmc1,    rsvd2,     rsvd3,     uarta,        0x048),
	PG(sdmmc1_cmd_pz1,       sdmmc1,    rsvd2,     rsvd3,     uarta,        0x04c),
	PG(lcd_sdin_pz2,         displaya,  displayb,  spi5,      rsvd4,        0x078),
	PG(lcd_wr_n_pz3,         displaya,  displayb,  spi5,      hdcp,         0x080),
	PG(lcd_sck_pz4,          displaya,  displayb,  spi5,      hdcp,         0x08c),
	PG(sys_clk_req_pz5,      sysclk,    rsvd2,     rsvd3,     rsvd4,        0x320),
	PG(pwr_i2c_scl_pz6,      i2cpwr,    rsvd2,     rsvd3,     rsvd4,        0x2b4),
	PG(pwr_i2c_sda_pz7,      i2cpwr,    rsvd2,     rsvd3,     rsvd4,        0x2b8),
	PG(sdmmc4_dat0_paa0,     uarte,     spi3,      gmi,       sdmmc4,       0x260),
	PG(sdmmc4_dat1_paa1,     uarte,     spi3,      gmi,       sdmmc4,       0x264),
	PG(sdmmc4_dat2_paa2,     uarte,     spi3,      gmi,       sdmmc4,       0x268),
	PG(sdmmc4_dat3_paa3,     uarte,     spi3,      gmi,       sdmmc4,       0x26c),
	PG(sdmmc4_dat4_paa4,     i2c3,      i2s4,      gmi,       sdmmc4,       0x270),
	PG(sdmmc4_dat5_paa5,     vgp3,      i2s4,      gmi,       sdmmc4,       0x274),
	PG(sdmmc4_dat6_paa6,     vgp4,      i2s4,      gmi,       sdmmc4,       0x278),
	PG(sdmmc4_dat7_paa7,     vgp5,      i2s4,      gmi,       sdmmc4,       0x27c),
	PG(pbb0,                 i2s4,      rsvd2,     rsvd3,     sdmmc4,       0x28c),
	PG(cam_i2c_scl_pbb1,     vgp1,      i2c3,      rsvd3,     sdmmc4,       0x290),
	PG(cam_i2c_sda_pbb2,     vgp2,      i2c3,      rsvd3,     sdmmc4,       0x294),
	PG(pbb3,                 vgp3,      displaya,  displayb,  sdmmc4,       0x298),
	PG(pbb4,                 vgp4,      displaya,  displayb,  sdmmc4,       0x29c),
	PG(pbb5,                 vgp5,      displaya,  displayb,  sdmmc4,       0x2a0),
	PG(pbb6,                 vgp6,      displaya,  displayb,  sdmmc4,       0x2a4),
	PG(pbb7,                 i2s4,      rsvd2,     rsvd3,     sdmmc4,       0x2a8),
	PG(cam_mclk_pcc0,        vi,        vi_alt1,   vi_alt3,   sdmmc4,       0x284),
	PG(pcc1,                 i2s4,      rsvd2,     rsvd3,     sdmmc4,       0x288),
	PG(pcc2,                 i2s4,      rsvd2,     rsvd3,     rsvd4,        0x2ac),
	PG(sdmmc4_rst_n_pcc3,    vgp6,      rsvd2,     rsvd3,     sdmmc4,       0x280),
	PG(sdmmc4_clk_pcc4,      invalid,   nand,      gmi,       sdmmc4,       0x258),
	PG(clk2_req_pcc5,        dap,       rsvd2,     rsvd3,     rsvd4,        0x06c),
	PG(pex_l2_rst_n_pcc6,    pcie,      hda,       rsvd3,     rsvd4,        0x3d8),
	PG(pex_l2_clkreq_n_pcc7, pcie,      hda,       rsvd3,     rsvd4,        0x3dc),
	PG(pex_l0_prsnt_n_pdd0,  pcie,      hda,       rsvd3,     rsvd4,        0x3b8),
	PG(pex_l0_rst_n_pdd1,    pcie,      hda,       rsvd3,     rsvd4,        0x3bc),
	PG(pex_l0_clkreq_n_pdd2, pcie,      hda,       rsvd3,     rsvd4,        0x3c0),
	PG(pex_wake_n_pdd3,      pcie,      hda,       rsvd3,     rsvd4,        0x3c4),
	PG(pex_l1_prsnt_n_pdd4,  pcie,      hda,       rsvd3,     rsvd4,        0x3c8),
	PG(pex_l1_rst_n_pdd5,    pcie,      hda,       rsvd3,     rsvd4,        0x3cc),
	PG(pex_l1_clkreq_n_pdd6, pcie,      hda,       rsvd3,     rsvd4,        0x3d0),
	PG(pex_l2_prsnt_n_pdd7,  pcie,      hda,       rsvd3,     rsvd4,        0x3d4),
	PG(clk3_out_pee0,        extperiph3, rsvd2,    rsvd3,     rsvd4,        0x1b8),
	PG(clk3_req_pee1,        dev3,      rsvd2,     rsvd3,     rsvd4,        0x1bc),
	PG(clk1_req_pee2,        dap,       hda,       rsvd3,     rsvd4,        0x348),
	PG(hdmi_cec_pee3,        cec,       rsvd2,     rsvd3,     rsvd4,        0x3e0),
	PG(clk_32k_in,           clk_32k_in, rsvd2,    rsvd3,     rsvd4,        0x330),
	PG(core_pwr_req,         core_pwr_req, rsvd2,  rsvd3,     rsvd4,        0x324),
	PG(cpu_pwr_req,          cpu_pwr_req,  rsvd2,  rsvd3,     rsvd4,        0x328),
	PG(owr,                  owr,       cec,       rsvd3,     rsvd4,        0x334),
	PG(pwr_int_n,            pwr_int_n, rsvd2,     rsvd3,     rsvd4,        0x32c),
};

static const struct tegra_drive_pingroup tegra30_drive_groups[] = {
	DRV_PG(ao1,   0x868,  2,  3,  4,  12,  5,  20,  5,  28,  2,  30,  2),
	DRV_PG(ao2,   0x86c,  2,  3,  4,  12,  5,  20,  5,  28,  2,  30,  2),
	DRV_PG(at1,   0x870,  2,  3,  4,  14,  5,  19,  5,  24,  2,  28,  2),
	DRV_PG(at2,   0x874,  2,  3,  4,  14,  5,  19,  5,  24,  2,  28,  2),
	DRV_PG(at3,   0x878,  2,  3,  4,  14,  5,  19,  5,  28,  2,  30,  2),
	DRV_PG(at4,   0x87c,  2,  3,  4,  14,  5,  19,  5,  28,  2,  30,  2),
	DRV_PG(at5,   0x880,  2,  3,  4,  14,  5,  19,  5,  28,  2,  30,  2),
	DRV_PG(cdev1, 0x884,  2,  3,  4,  12,  5,  20,  5,  28,  2,  30,  2),
	DRV_PG(cdev2, 0x888,  2,  3,  4,  12,  5,  20,  5,  28,  2,  30,  2),
	DRV_PG(cec,   0x938,  2,  3,  4,  12,  5,  20,  5,  28,  2,  30,  2),
	DRV_PG(crt,   0x8f8,  2,  3,  4,  12,  5,  20,  5,  28,  2,  30,  2),
	DRV_PG(csus,  0x88c, -1, -1, -1,  12,  5,  19,  5,  24,  4,  28,  4),
	DRV_PG(dap1,  0x890,  2,  3,  4,  12,  5,  20,  5,  28,  2,  30,  2),
	DRV_PG(dap2,  0x894,  2,  3,  4,  12,  5,  20,  5,  28,  2,  30,  2),
	DRV_PG(dap3,  0x898,  2,  3,  4,  12,  5,  20,  5,  28,  2,  30,  2),
	DRV_PG(dap4,  0x89c,  2,  3,  4,  12,  5,  20,  5,  28,  2,  30,  2),
	DRV_PG(dbg,   0x8a0,  2,  3,  4,  12,  5,  20,  5,  28,  2,  30,  2),
	DRV_PG(ddc,   0x8fc,  2,  3,  4,  12,  5,  20,  5,  28,  2,  30,  2),
	DRV_PG(dev3,  0x92c,  2,  3,  4,  12,  5,  20,  5,  28,  2,  30,  2),
	DRV_PG(gma,   0x900, -1, -1, -1,  14,  5,  19,  5,  24,  4,  28,  4),
	DRV_PG(gmb,   0x904, -1, -1, -1,  14,  5,  19,  5,  24,  4,  28,  4),
	DRV_PG(gmc,   0x908, -1, -1, -1,  14,  5,  19,  5,  24,  4,  28,  4),
	DRV_PG(gmd,   0x90c, -1, -1, -1,  14,  5,  19,  5,  24,  4,  28,  4),
	DRV_PG(gme,   0x910,  2,  3,  4,  14,  5,  19,  5,  28,  2,  30,  2),
	DRV_PG(gmf,   0x914,  2,  3,  4,  14,  5,  19,  5,  28,  2,  30,  2),
	DRV_PG(gmg,   0x918,  2,  3,  4,  14,  5,  19,  5,  28,  2,  30,  2),
	DRV_PG(gmh,   0x91c,  2,  3,  4,  14,  5,  19,  5,  28,  2,  30,  2),
	DRV_PG(gpv,   0x928,  2,  3,  4,  12,  5,  20,  5,  28,  2,  30,  2),
	DRV_PG(lcd1,  0x8a4,  2,  3,  4,  12,  5,  20,  5,  28,  2,  30,  2),
	DRV_PG(lcd2,  0x8a8,  2,  3,  4,  12,  5,  20,  5,  28,  2,  30,  2),
	DRV_PG(owr,   0x920,  2,  3,  4,  12,  5,  20,  5,  28,  2,  30,  2),
	DRV_PG(sdio1, 0x8ec,  2,  3, -1,  12,  7,  20,  7,  28,  2,  30,  2),
	DRV_PG(sdio2, 0x8ac,  2,  3, -1,  12,  7,  20,  7,  28,  2,  30,  2),
	DRV_PG(sdio3, 0x8b0,  2,  3, -1,  12,  7,  20,  7,  28,  2,  30,  2),
	DRV_PG(spi,   0x8b4,  2,  3,  4,  12,  5,  20,  5,  28,  2,  30,  2),
	DRV_PG(uaa,   0x8b8,  2,  3,  4,  12,  5,  20,  5,  28,  2,  30,  2),
	DRV_PG(uab,   0x8bc,  2,  3,  4,  12,  5,  20,  5,  28,  2,  30,  2),
	DRV_PG(uart2, 0x8c0,  2,  3,  4,  12,  5,  20,  5,  28,  2,  30,  2),
	DRV_PG(uart3, 0x8c4,  2,  3,  4,  12,  5,  20,  5,  28,  2,  30,  2),
	DRV_PG(uda,   0x924,  2,  3,  4,  12,  5,  20,  5,  28,  2,  30,  2),
	DRV_PG(vi1,   0x8c8, -1, -1, -1,  14,  5,  19,  5,  24,  4,  28,  4),
};

static const struct pinctrl_tegra30_drvdata tegra30_drvdata = {
	.pingrps = tegra30_pin_groups,
	.num_pingrps = ARRAY_SIZE(tegra30_pin_groups),
	.drvgrps = tegra30_drive_groups,
	.num_drvgrps = ARRAY_SIZE(tegra30_drive_groups),
};

static const struct tegra_pingroup tegra124_pin_groups[] = {
	PG(ulpi_data0_po1,         spi3,       hsi,        uarta,        ulpi,       0x000),
	PG(ulpi_data1_po2,         spi3,       hsi,        uarta,        ulpi,       0x004),
	PG(ulpi_data2_po3,         spi3,       hsi,        uarta,        ulpi,       0x008),
	PG(ulpi_data3_po4,         spi3,       hsi,        uarta,        ulpi,       0x00c),
	PG(ulpi_data4_po5,         spi2,       hsi,        uarta,        ulpi,       0x010),
	PG(ulpi_data5_po6,         spi2,       hsi,        uarta,        ulpi,       0x014),
	PG(ulpi_data6_po7,         spi2,       hsi,        uarta,        ulpi,       0x018),
	PG(ulpi_data7_po0,         spi2,       hsi,        uarta,        ulpi,       0x01c),
	PG(ulpi_clk_py0,           spi1,       spi5,       uartd,        ulpi,       0x020),
	PG(ulpi_dir_py1,           spi1,       spi5,       uartd,        ulpi,       0x024),
	PG(ulpi_nxt_py2,           spi1,       spi5,       uartd,        ulpi,       0x028),
	PG(ulpi_stp_py3,           spi1,       spi5,       uartd,        ulpi,       0x02c),
	PG(dap3_fs_pp0,            i2s2,       spi5,       displaya,     displayb,   0x030),
	PG(dap3_din_pp1,           i2s2,       spi5,       displaya,     displayb,   0x034),
	PG(dap3_dout_pp2,          i2s2,       spi5,       displaya,     rsvd4,      0x038),
	PG(dap3_sclk_pp3,          i2s2,       spi5,       rsvd3,        displayb,   0x03c),
	PG(pv0,                    rsvd1,      rsvd2,      rsvd3,        rsvd4,      0x040),
	PG(pv1,                    rsvd1,      rsvd2,      rsvd3,        rsvd4,      0x044),
	PG(sdmmc1_clk_pz0,         sdmmc1,     clk12,      rsvd3,        rsvd4,      0x048),
	PG(sdmmc1_cmd_pz1,         sdmmc1,     spdif,      spi4,         uarta,      0x04c),
	PG(sdmmc1_dat3_py4,        sdmmc1,     spdif,      spi4,         uarta,      0x050),
	PG(sdmmc1_dat2_py5,        sdmmc1,     pwm0,       spi4,         uarta,      0x054),
	PG(sdmmc1_dat1_py6,        sdmmc1,     pwm1,       spi4,         uarta,      0x058),
	PG(sdmmc1_dat0_py7,        sdmmc1,     rsvd2,      spi4,         uarta,      0x05c),
	PG(clk2_out_pw5,           extperiph2, rsvd2,      rsvd3,        rsvd4,      0x068),
	PG(clk2_req_pcc5,          dap,        rsvd2,      rsvd3,        rsvd4,      0x06c),
	PG(hdmi_int_pn7,           rsvd1,      rsvd2,      rsvd3,        rsvd4,      0x110),
	PG(ddc_scl_pv4,            i2c4,       rsvd2,      rsvd3,        rsvd4,      0x114),
	PG(ddc_sda_pv5,            i2c4,       rsvd2,      rsvd3,        rsvd4,      0x118),
	PG(uart2_rxd_pc3,          irda,       spdif,      uarta,        spi4,       0x164),
	PG(uart2_txd_pc2,          irda,       spdif,      uarta,        spi4,       0x168),
	PG(uart2_rts_n_pj6,        uarta,      uartb,      gmi,          spi4,       0x16c),
	PG(uart2_cts_n_pj5,        uarta,      uartb,      gmi,          spi4,       0x170),
	PG(uart3_txd_pw6,          uartc,      rsvd2,      gmi,          spi4,       0x174),
	PG(uart3_rxd_pw7,          uartc,      rsvd2,      gmi,          spi4,       0x178),
	PG(uart3_cts_n_pa1,        uartc,      sdmmc1,     dtv,          gmi,        0x17c),
	PG(uart3_rts_n_pc0,        uartc,      pwm0,       dtv,          gmi,        0x180),
	PG(pu0,                    owr,        uarta,      gmi,          rsvd4,      0x184),
	PG(pu1,                    rsvd1,      uarta,      gmi,          rsvd4,      0x188),
	PG(pu2,                    rsvd1,      uarta,      gmi,          rsvd4,      0x18c),
	PG(pu3,                    pwm0,       uarta,      gmi,          displayb,   0x190),
	PG(pu4,                    pwm1,       uarta,      gmi,          displayb,   0x194),
	PG(pu5,                    pwm2,       uarta,      gmi,          displayb,   0x198),
	PG(pu6,                    pwm3,       uarta,      rsvd3,        gmi,        0x19c),
	PG(gen1_i2c_sda_pc5,       i2c1,       rsvd2,      rsvd3,        rsvd4,      0x1a0),
	PG(gen1_i2c_scl_pc4,       i2c1,       rsvd2,      rsvd3,        rsvd4,      0x1a4),
	PG(dap4_fs_pp4,            i2s3,       gmi,        dtv,          rsvd4,      0x1a8),
	PG(dap4_din_pp5,           i2s3,       gmi,        rsvd3,        rsvd4,      0x1ac),
	PG(dap4_dout_pp6,          i2s3,       gmi,        dtv,          rsvd4,      0x1b0),
	PG(dap4_sclk_pp7,          i2s3,       gmi,        rsvd3,        rsvd4,      0x1b4),
	PG(clk3_out_pee0,          extperiph3, rsvd2,      rsvd3,        rsvd4,      0x1b8),
	PG(clk3_req_pee1,          dev3,       rsvd2,      rsvd3,        rsvd4,      0x1bc),
	PG(pc7,                    rsvd1,      rsvd2,      gmi,          gmi_alt,    0x1c0),
	PG(pi5,                    sdmmc2,     rsvd2,      gmi,          rsvd4,      0x1c4),
	PG(pi7,                    rsvd1,      trace,      gmi,          dtv,        0x1c8),
	PG(pk0,                    rsvd1,      sdmmc3,     gmi,          soc,        0x1cc),
	PG(pk1,                    sdmmc2,     trace,      gmi,          rsvd4,      0x1d0),
	PG(pj0,                    rsvd1,      rsvd2,      gmi,          usb,        0x1d4),
	PG(pj2,                    rsvd1,      rsvd2,      gmi,          soc,        0x1d8),
	PG(pk3,                    sdmmc2,     trace,      gmi,          ccla,       0x1dc),
	PG(pk4,                    sdmmc2,     rsvd2,      gmi,          gmi_alt,    0x1e0),
	PG(pk2,                    rsvd1,      rsvd2,      gmi,          rsvd4,      0x1e4),
	PG(pi3,                    rsvd1,      rsvd2,      gmi,          spi4,       0x1e8),
	PG(pi6,                    rsvd1,      rsvd2,      gmi,          sdmmc2,     0x1ec),
	PG(pg0,                    rsvd1,      rsvd2,      gmi,          rsvd4,      0x1f0),
	PG(pg1,                    rsvd1,      rsvd2,      gmi,          rsvd4,      0x1f4),
	PG(pg2,                    rsvd1,      trace,      gmi,          rsvd4,      0x1f8),
	PG(pg3,                    rsvd1,      trace,      gmi,          rsvd4,      0x1fc),
	PG(pg4,                    rsvd1,      tmds,       gmi,          spi4,       0x200),
	PG(pg5,                    rsvd1,      rsvd2,      gmi,          spi4,       0x204),
	PG(pg6,                    rsvd1,      rsvd2,      gmi,          spi4,       0x208),
	PG(pg7,                    rsvd1,      rsvd2,      gmi,          spi4,       0x20c),
	PG(ph0,                    pwm0,       trace,      gmi,          dtv,        0x210),
	PG(ph1,                    pwm1,       tmds,       gmi,          displaya,   0x214),
	PG(ph2,                    pwm2,       tmds,       gmi,          cldvfs,     0x218),
	PG(ph3,                    pwm3,       spi4,       gmi,          cldvfs,     0x21c),
	PG(ph4,                    sdmmc2,     rsvd2,      gmi,          rsvd4,      0x220),
	PG(ph5,                    sdmmc2,     rsvd2,      gmi,          rsvd4,      0x224),
	PG(ph6,                    sdmmc2,     trace,      gmi,          dtv,        0x228),
	PG(ph7,                    sdmmc2,     trace,      gmi,          dtv,        0x22c),
	PG(pj7,                    uartd,      rsvd2,      gmi,          gmi_alt,    0x230),
	PG(pb0,                    uartd,      rsvd2,      gmi,          rsvd4,      0x234),
	PG(pb1,                    uartd,      rsvd2,      gmi,          rsvd4,      0x238),
	PG(pk7,                    uartd,      rsvd2,      gmi,          rsvd4,      0x23c),
	PG(pi0,                    rsvd1,      rsvd2,      gmi,          rsvd4,      0x240),
	PG(pi1,                    rsvd1,      rsvd2,      gmi,          rsvd4,      0x244),
	PG(pi2,                    sdmmc2,     trace,      gmi,          rsvd4,      0x248),
	PG(pi4,                    spi4,       trace,      gmi,          displaya,   0x24c),
	PG(gen2_i2c_scl_pt5,       i2c2,       rsvd2,      gmi,          rsvd4,      0x250),
	PG(gen2_i2c_sda_pt6,       i2c2,       rsvd2,      gmi,          rsvd4,      0x254),
	PG(sdmmc4_clk_pcc4,        sdmmc4,     rsvd2,      gmi,          rsvd4,      0x258),
	PG(sdmmc4_cmd_pt7,         sdmmc4,     rsvd2,      gmi,          rsvd4,      0x25c),
	PG(sdmmc4_dat0_paa0,       sdmmc4,     spi3,       gmi,          rsvd4,      0x260),
	PG(sdmmc4_dat1_paa1,       sdmmc4,     spi3,       gmi,          rsvd4,      0x264),
	PG(sdmmc4_dat2_paa2,       sdmmc4,     spi3,       gmi,          rsvd4,      0x268),
	PG(sdmmc4_dat3_paa3,       sdmmc4,     spi3,       gmi,          rsvd4,      0x26c),
	PG(sdmmc4_dat4_paa4,       sdmmc4,     spi3,       gmi,          rsvd4,      0x270),
	PG(sdmmc4_dat5_paa5,       sdmmc4,     spi3,       rsvd3,        rsvd4,      0x274),
	PG(sdmmc4_dat6_paa6,       sdmmc4,     spi3,       gmi,          rsvd4,      0x278),
	PG(sdmmc4_dat7_paa7,       sdmmc4,     rsvd2,      gmi,          rsvd4,      0x27c),
	PG(cam_mclk_pcc0,          vi,         vi_alt1,    vi_alt3,      sdmmc2,     0x284),
	PG(pcc1,                   i2s4,       rsvd2,      rsvd3,        sdmmc2,     0x288),
	PG(pbb0,                   vgp6,       vimclk2,    sdmmc2,       vimclk2_alt,0x28c),
	PG(cam_i2c_scl_pbb1,       vgp1,       i2c3,       rsvd3,        sdmmc2,     0x290),
	PG(cam_i2c_sda_pbb2,       vgp2,       i2c3,       rsvd3,        sdmmc2,     0x294),
	PG(pbb3,                   vgp3,       displaya,   displayb,     sdmmc2,     0x298),
	PG(pbb4,                   vgp4,       displaya,   displayb,     sdmmc2,     0x29c),
	PG(pbb5,                   vgp5,       displaya,   rsvd3,        sdmmc2,     0x2a0),
	PG(pbb6,                   i2s4,       rsvd2,      displayb,     sdmmc2,     0x2a4),
	PG(pbb7,                   i2s4,       rsvd2,      rsvd3,        sdmmc2,     0x2a8),
	PG(pcc2,                   i2s4,       rsvd2,      sdmmc3,       sdmmc2,     0x2ac),
	PG(jtag_rtck,              rtck,       rsvd2,      rsvd3,        rsvd4,      0x2b0),
	PG(pwr_i2c_scl_pz6,        i2cpwr,     rsvd2,      rsvd3,        rsvd4,      0x2b4),
	PG(pwr_i2c_sda_pz7,        i2cpwr,     rsvd2,      rsvd3,        rsvd4,      0x2b8),
	PG(kb_row0_pr0,            kbc,        rsvd2,      rsvd3,        rsvd4,      0x2bc),
	PG(kb_row1_pr1,            kbc,        rsvd2,      rsvd3,        rsvd4,      0x2c0),
	PG(kb_row2_pr2,            kbc,        rsvd2,      rsvd3,        rsvd4,      0x2c4),
	PG(kb_row3_pr3,            kbc,        displaya,   sys,          displayb,   0x2c8),
	PG(kb_row4_pr4,            kbc,        displaya,   rsvd3,        displayb,   0x2cc),
	PG(kb_row5_pr5,            kbc,        displaya,   rsvd3,        displayb,   0x2d0),
	PG(kb_row6_pr6,            kbc,        displaya,   displaya_alt, displayb,   0x2d4),
	PG(kb_row7_pr7,            kbc,        rsvd2,      cldvfs,       uarta,      0x2d8),
	PG(kb_row8_ps0,            kbc,        rsvd2,      cldvfs,       uarta,      0x2dc),
	PG(kb_row9_ps1,            kbc,        rsvd2,      rsvd3,        uarta,      0x2e0),
	PG(kb_row10_ps2,           kbc,        rsvd2,      rsvd3,        uarta,      0x2e4),
	PG(kb_row11_ps3,           kbc,        rsvd2,      rsvd3,        irda,       0x2e8),
	PG(kb_row12_ps4,           kbc,        rsvd2,      rsvd3,        irda,       0x2ec),
	PG(kb_row13_ps5,           kbc,        rsvd2,      spi2,         rsvd4,      0x2f0),
	PG(kb_row14_ps6,           kbc,        rsvd2,      spi2,         rsvd4,      0x2f4),
	PG(kb_row15_ps7,           kbc,        soc,        rsvd3,        rsvd4,      0x2f8),
	PG(kb_col0_pq0,            kbc,        rsvd2,      spi2,         rsvd4,      0x2fc),
	PG(kb_col1_pq1,            kbc,        rsvd2,      spi2,         rsvd4,      0x300),
	PG(kb_col2_pq2,            kbc,        rsvd2,      spi2,         rsvd4,      0x304),
	PG(kb_col3_pq3,            kbc,        displaya,   pwm2,         uarta,      0x308),
	PG(kb_col4_pq4,            kbc,        owr,        sdmmc3,       uarta,      0x30c),
	PG(kb_col5_pq5,            kbc,        rsvd2,      sdmmc3,       rsvd4,      0x310),
	PG(kb_col6_pq6,            kbc,        rsvd2,      spi2,         uartd,      0x314),
	PG(kb_col7_pq7,            kbc,        rsvd2,      spi2,         uartd,      0x318),
	PG(clk_32k_out_pa0,        blink,      soc,        rsvd3,        rsvd4,      0x31c),
	PG(core_pwr_req,           pwron,      rsvd2,      rsvd3,        rsvd4,      0x324),
	PG(cpu_pwr_req,            cpu,        rsvd2,      rsvd3,        rsvd4,      0x328),
	PG(pwr_int_n,              pmi,        rsvd2,      rsvd3,        rsvd4,      0x32c),
	PG(clk_32k_in,             clk,        rsvd2,      rsvd3,        rsvd4,      0x330),
	PG(owr,                    owr,        rsvd2,      rsvd3,        rsvd4,      0x334),
	PG(dap1_fs_pn0,            i2s0,       hda,        gmi,          rsvd4,      0x338),
	PG(dap1_din_pn1,           i2s0,       hda,        gmi,          rsvd4,      0x33c),
	PG(dap1_dout_pn2,          i2s0,       hda,        gmi,          sata,       0x340),
	PG(dap1_sclk_pn3,          i2s0,       hda,        gmi,          rsvd4,      0x344),
	PG(dap_mclk1_req_pee2,     dap,        dap1,       sata,         rsvd4,      0x348),
	PG(dap_mclk1_pw4,          extperiph1, dap2,       rsvd3,        rsvd4,      0x34c),
	PG(spdif_in_pk6,           spdif,      rsvd2,      rsvd3,        i2c3,       0x350),
	PG(spdif_out_pk5,          spdif,      rsvd2,      rsvd3,        i2c3,       0x354),
	PG(dap2_fs_pa2,            i2s1,       hda,        gmi,          rsvd4,      0x358),
	PG(dap2_din_pa4,           i2s1,       hda,        gmi,          rsvd4,      0x35c),
	PG(dap2_dout_pa5,          i2s1,       hda,        gmi,          rsvd4,      0x360),
	PG(dap2_sclk_pa3,          i2s1,       hda,        gmi,          rsvd4,      0x364),
	PG(dvfs_pwm_px0,           spi6,       cldvfs,     gmi,          rsvd4,      0x368),
	PG(gpio_x1_aud_px1,        spi6,       rsvd2,      gmi,          rsvd4,      0x36c),
	PG(gpio_x3_aud_px3,        spi6,       spi1,       gmi,          rsvd4,      0x370),
	PG(dvfs_clk_px2,           spi6,       cldvfs,     gmi,          rsvd4,      0x374),
	PG(gpio_x4_aud_px4,        gmi,        spi1,       spi2,         dap2,       0x378),
	PG(gpio_x5_aud_px5,        gmi,        spi1,       spi2,         rsvd4,      0x37c),
	PG(gpio_x6_aud_px6,        spi6,       spi1,       spi2,         gmi,        0x380),
	PG(gpio_x7_aud_px7,        rsvd1,      spi1,       spi2,         rsvd4,      0x384),
	PG(sdmmc3_clk_pa6,         sdmmc3,     rsvd2,      rsvd3,        spi3,       0x390),
	PG(sdmmc3_cmd_pa7,         sdmmc3,     pwm3,       uarta,        spi3,       0x394),
	PG(sdmmc3_dat0_pb7,        sdmmc3,     rsvd2,      rsvd3,        spi3,       0x398),
	PG(sdmmc3_dat1_pb6,        sdmmc3,     pwm2,       uarta,        spi3,       0x39c),
	PG(sdmmc3_dat2_pb5,        sdmmc3,     pwm1,       displaya,     spi3,       0x3a0),
	PG(sdmmc3_dat3_pb4,        sdmmc3,     pwm0,       displayb,     spi3,       0x3a4),
	PG(pex_l0_rst_n_pdd1,      pe0,        rsvd2,      rsvd3,        rsvd4,      0x3bc),
	PG(pex_l0_clkreq_n_pdd2,   pe0,        rsvd2,      rsvd3,        rsvd4,      0x3c0),
	PG(pex_wake_n_pdd3,        pe,         rsvd2,      rsvd3,        rsvd4,      0x3c4),
	PG(pex_l1_rst_n_pdd5,      pe1,        rsvd2,      rsvd3,        rsvd4,      0x3cc),
	PG(pex_l1_clkreq_n_pdd6,   pe1,        rsvd2,      rsvd3,        rsvd4,      0x3d0),
	PG(hdmi_cec_pee3,          cec,        rsvd2,      rsvd3,        rsvd4,      0x3e0),
	PG(sdmmc1_wp_n_pv3,        sdmmc1,     clk12,      spi4,         uarta,      0x3e4),
	PG(sdmmc3_cd_n_pv2,        sdmmc3,     owr,        rsvd3,        rsvd4,      0x3e8),
	PG(gpio_w2_aud_pw2,        spi6,       rsvd2,      spi2,         i2c1,       0x3ec),
	PG(gpio_w3_aud_pw3,        spi6,       spi1,       spi2,         i2c1,       0x3f0),
	PG(usb_vbus_en0_pn4,       usb,        rsvd2,      rsvd3,        rsvd4,      0x3f4),
	PG(usb_vbus_en1_pn5,       usb,        rsvd2,      rsvd3,        rsvd4,      0x3f8),
	PG(sdmmc3_clk_lb_in_pee5,  sdmmc3,     rsvd2,      rsvd3,        rsvd4,      0x3fc),
	PG(sdmmc3_clk_lb_out_pee4, sdmmc3,     rsvd2,      rsvd3,        rsvd4,      0x400),
	PG(gmi_clk_lb,             sdmmc2,     rsvd2,      gmi,          rsvd4,      0x404),
	PG(reset_out_n,            rsvd1,      rsvd2,      rsvd3,        reset_out_n,0x408),
	PG(kb_row16_pt0,           kbc,        rsvd2,      rsvd3,        uartc,      0x40c),
	PG(kb_row17_pt1,           kbc,        rsvd2,      rsvd3,        uartc,      0x410),
	PG(usb_vbus_en2_pff1,      usb,        rsvd2,      rsvd3,        rsvd4,      0x414),
	PG(pff2,                   sata,       rsvd2,      rsvd3,        rsvd4,      0x418),
	PG(dp_hpd_pff0,            dp,         rsvd2,      rsvd3,        rsvd4,      0x430),
};

static const struct tegra_drive_pingroup tegra124_drive_groups[] = {
	DRV_PG(ao1,   0x868,  2,  3,  4,  12,  5,  20,  5,  28,  2,  30,  2),
	DRV_PG(ao2,   0x86c,  2,  3,  4,  12,  5,  20,  5,  28,  2,  30,  2),
	DRV_PG(at1,   0x870,  2,  3,  4,  12,  7,  20,  7,  28,  2,  30,  2),
	DRV_PG(at2,   0x874,  2,  3,  4,  12,  7,  20,  7,  28,  2,  30,  2),
	DRV_PG(at3,   0x878,  2,  3,  4,  12,  7,  20,  7,  28,  2,  30,  2),
	DRV_PG(at4,   0x87c,  2,  3,  4,  12,  7,  20,  7,  28,  2,  30,  2),
	DRV_PG(at5,   0x880,  2,  3,  4,  14,  5,  19,  5,  28,  2,  30,  2),
	DRV_PG(cdev1, 0x884,  2,  3,  4,  12,  5,  20,  5,  28,  2,  30,  2),
	DRV_PG(cdev2, 0x888,  2,  3,  4,  12,  5,  20,  5,  28,  2,  30,  2),
	DRV_PG(dap1,  0x890,  2,  3,  4,  12,  5,  20,  5,  28,  2,  30,  2),
	DRV_PG(dap2,  0x894,  2,  3,  4,  12,  5,  20,  5,  28,  2,  30,  2),
	DRV_PG(dap3,  0x898,  2,  3,  4,  12,  5,  20,  5,  28,  2,  30,  2),
	DRV_PG(dap4,  0x89c,  2,  3,  4,  12,  5,  20,  5,  28,  2,  30,  2),
	DRV_PG(dbg,   0x8a0,  2,  3,  4,  12,  5,  20,  5,  28,  2,  30,  2),
	DRV_PG(sdio3, 0x8b0,  2,  3, -1,  12,  7,  20,  7,  28,  2,  30,  2),
	DRV_PG(spi,   0x8b4,  2,  3,  4,  12,  5,  20,  5,  28,  2,  30,  2),
	DRV_PG(uaa,   0x8b8,  2,  3,  4,  12,  5,  20,  5,  28,  2,  30,  2),
	DRV_PG(uab,   0x8bc,  2,  3,  4,  12,  5,  20,  5,  28,  2,  30,  2),
	DRV_PG(uart2, 0x8c0,  2,  3,  4,  12,  5,  20,  5,  28,  2,  30,  2),
	DRV_PG(uart3, 0x8c4,  2,  3,  4,  12,  5,  20,  5,  28,  2,  30,  2),
	DRV_PG(sdio1, 0x8ec,  2,  3, -1,  12,  7,  20,  7,  28,  2,  30,  2),
	DRV_PG(ddc,   0x8fc,  2,  3,  4,  12,  5,  20,  5,  28,  2,  30,  2),
	DRV_PG(gma,   0x900,  2,  3,  4,  14,  5,  20,  5,  28,  2,  30,  2),
	DRV_PG(gme,   0x910,  2,  3,  4,  14,  5,  19,  5,  28,  2,  30,  2),
	DRV_PG(gmf,   0x914,  2,  3,  4,  14,  5,  19,  5,  28,  2,  30,  2),
	DRV_PG(gmg,   0x918,  2,  3,  4,  14,  5,  19,  5,  28,  2,  30,  2),
	DRV_PG(gmh,   0x91c,  2,  3,  4,  14,  5,  19,  5,  28,  2,  30,  2),
	DRV_PG(owr,   0x920,  2,  3,  4,  12,  5,  20,  5,  28,  2,  30,  2),
	DRV_PG(uda,   0x924,  2,  3,  4,  12,  5,  20,  5,  28,  2,  30,  2),
	DRV_PG(gpv,   0x928,  2,  3,  4,  12,  5,  20,  5,  28,  2,  30,  2),
	DRV_PG(dev3,  0x92c,  2,  3,  4,  12,  5,  20,  5,  28,  2,  30,  2),
	DRV_PG(cec,   0x938,  2,  3,  4,  12,  5,  20,  5,  28,  2,  30,  2),
	DRV_PG(at6,   0x994,  2,  3,  4,  12,  7,  20,  7,  28,  2,  30,  2),
	DRV_PG(dap5,  0x998,  2,  3,  4,  12,  5,  20,  5,  28,  2,  30,  2),
	DRV_PG(usb_vbus_en, 0x99c,  2,  3,  4,  12,  5,  20,  5,  28,  2,  30,  2),
	DRV_PG(ao3,   0x9a8,  2,  3,  4,  12,  5,  -1, -1,  28,  2,  -1, -1),
	DRV_PG(ao0,   0x9b0,  2,  3,  4,  12,  5,  20,  5,  28,  2,  30,  2),
	DRV_PG(hv0,   0x9b4,  2,  3,  4,  12,  5,  -1, -1,  28,  2,  -1, -1),
	DRV_PG(sdio4, 0x9c4,  2,  3,  4,  12,  5,  20,  5,  28,  2,  30,  2),
	DRV_PG(ao4,   0x9c8,  2,  3,  4,  12,  7,  20,  7,  28,  2,  30,  2),
};

static const struct pinctrl_tegra30_drvdata tegra124_drvdata = {
	.pingrps = tegra124_pin_groups,
	.num_pingrps = ARRAY_SIZE(tegra124_pin_groups),
	.drvgrps = tegra124_drive_groups,
	.num_drvgrps = ARRAY_SIZE(tegra124_drive_groups),
};

static int pinctrl_tegra30_set_drvstate(struct pinctrl_tegra30 *ctrl,
                                        struct device_node *np)
{
	const char *pins = NULL;
	const struct tegra_drive_pingroup *group = NULL;
	int hsm = -1, schmitt = -1, pds = -1, pus = -1, srr = -1, srf = -1;
	int i;
	u32 __iomem *regaddr;
	u32 val;

	if (of_property_read_string(np, "nvidia,pins", &pins))
		return 0;

	for (i = 0; i < ctrl->drvdata->num_drvgrps; i++) {
		if (!strcmp(pins, ctrl->drvdata->drvgrps[i].name)) {
			group = &ctrl->drvdata->drvgrps[i];
			break;
		}
	}
	/* if no matching drivegroup is found */
	if (i == ctrl->drvdata->num_drvgrps)
		return 0;

	regaddr = ctrl->regs.ctrl + (group->reg >> 2);

	of_property_read_u32_array(np, "nvidia,high-speed-mode", &hsm, 1);
	of_property_read_u32_array(np, "nvidia,schmitt", &schmitt, 1);
	of_property_read_u32_array(np, "nvidia,pull-down-strength", &pds, 1);
	of_property_read_u32_array(np, "nvidia,pull-up-strength", &pus, 1);
	of_property_read_u32_array(np, "nvidia,slew-rate-rising", &srr, 1);
	of_property_read_u32_array(np, "nvidia,slew-rate-falling", &srf, 1);

	if (hsm >= 0) {
		val = readl(regaddr);
		val &= ~(0x1 << group->hsm_bit);
		val |= hsm << group->hsm_bit;
		writel(val, regaddr);
	}
	if (schmitt >= 0) {
		val = readl(regaddr);
		val &= ~(0x1 << group->schmitt_bit);
		val |= hsm << group->schmitt_bit;
		writel(val, regaddr);
	}
	if (pds >= 0) {
		val = readl(regaddr);
		val &= ~(((1 << group->drvdn_width) - 1) << group->drvdn_bit);
		val |= hsm << group->drvdn_bit;
		writel(val, regaddr);
	}
	if (pus >= 0) {
		val = readl(regaddr);
		val &= ~(((1 << group->drvup_width) - 1) << group->drvup_bit);
		val |= hsm << group->drvup_bit;
		writel(val, regaddr);
	}
	if (srr >= 0) {
		val = readl(regaddr);
		val &= ~(((1 << group->slwr_width) - 1) << group->slwr_bit);
		val |= hsm << group->slwr_bit;
		writel(val, regaddr);
	}
	if (srf >= 0) {
		val = readl(regaddr);
		val &= ~(((1 << group->slwf_width) - 1) << group->slwf_bit);
		val |= hsm << group->slwf_bit;
		writel(val, regaddr);
	}

	return 1;
}

static void pinctrl_tegra30_set_func(struct pinctrl_tegra30 *ctrl,
				     u32 reg, int func)
{
	u32 __iomem *regaddr = ctrl->regs.mux + (reg >> 2);
	u32 val;

	val = readl(regaddr);
	val &= ~(0x3);
	val |= func;
	writel(val, regaddr);
}

static void pinctrl_tegra30_set_pull(struct pinctrl_tegra30 *ctrl,
				     u32 reg, int pull)
{
	u32 __iomem *regaddr = ctrl->regs.mux + (reg >> 2);
	u32 val;

	val = readl(regaddr);
	val &= ~(0x3 << 2);
	val |= pull << 2;
	writel(val, regaddr);
}

static void pinctrl_tegra30_set_input(struct pinctrl_tegra30 *ctrl,
				      u32 reg, int input)
{
	u32 __iomem *regaddr = ctrl->regs.mux + (reg >> 2);
	u32 val;

	val = readl(regaddr);
	val &= ~(1 << 5);
	val |= input << 5;
	writel(val, regaddr);
}

static void pinctrl_tegra30_set_tristate(struct pinctrl_tegra30 *ctrl,
					 u32 reg, int tristate)
{
	u32 __iomem *regaddr = ctrl->regs.mux + (reg >> 2);
	u32 val;

	val = readl(regaddr);
	val &= ~(1 << 4);
	val |= tristate << 4;
	writel(val, regaddr);
}

static void pinctrl_tegra30_set_opendrain(struct pinctrl_tegra30 *ctrl,
					  u32 reg, int opendrain)
{
	u32 __iomem *regaddr = ctrl->regs.mux + (reg >> 2);
	u32 val;

	val = readl(regaddr);
	val &= ~(1 << 6);
	val |= opendrain << 6;
	writel(val, regaddr);
}

static void pinctrl_tegra30_set_ioreset(struct pinctrl_tegra30 *ctrl,
					u32 reg, int ioreset)
{
	u32 __iomem *regaddr = ctrl->regs.mux + (reg >> 2);
	u32 val;

	val = readl(regaddr);
	val &= ~(1 << 8);
	val |= ioreset << 8;
	writel(val, regaddr);
}

static int pinctrl_tegra30_set_state(struct pinctrl_device *pdev,
				     struct device_node *np)
{
	struct pinctrl_tegra30 *ctrl =
			container_of(pdev, struct pinctrl_tegra30, pinctrl);
	struct device_node *childnode;
	int pull = -1, tri = -1, in = -1, od = -1, ior = -1, i, j, k;
	const char *pins, *func = NULL;
	const struct tegra_pingroup *group = NULL;

	/*
	 * At first look if the node we are pointed at has children,
	 * which we may want to visit.
	 */
	list_for_each_entry(childnode, &np->children, parent_list)
		pinctrl_tegra30_set_state(pdev, childnode);

	/* read relevant state from devicetree */
	of_property_read_string(np, "nvidia,function", &func);
	of_property_read_u32_array(np, "nvidia,pull", &pull, 1);
	of_property_read_u32_array(np, "nvidia,tristate", &tri, 1);
	of_property_read_u32_array(np, "nvidia,enable-input", &in, 1);
	of_property_read_u32_array(np, "nvidia,open-drain", &od, 1);
	of_property_read_u32_array(np, "nvidia,io-reset", &ior, 1);

	/* iterate over all pingroups referenced in the dt node */
	for (i = 0; ; i++) {
		if (of_property_read_string_index(np, "nvidia,pins", i, &pins))
			break;

		for (j = 0; j < ctrl->drvdata->num_pingrps; j++) {
			if (!strcmp(pins, ctrl->drvdata->pingrps[j].name)) {
				group = &ctrl->drvdata->pingrps[j];
				break;
			}
		}
		/* if no matching pingroup is found */
		if (j == ctrl->drvdata->num_pingrps) {
			/* see if we can find a drivegroup */
			if (pinctrl_tegra30_set_drvstate(ctrl, np))
				continue;

			/* nothing matching found, warn and bail out */
			dev_warn(ctrl->pinctrl.dev,
				 "invalid pingroup %s referenced in node %s\n",
				 pins, np->name);
			continue;
		}

		if (func) {
			for (k = 0; k < 4; k++) {
				if (!strcmp(func, group->funcs[k]))
					break;
			}
			if (k < 4)
				pinctrl_tegra30_set_func(ctrl, group->reg, k);
			else
				dev_warn(ctrl->pinctrl.dev,
					 "invalid function %s for pingroup %s in node %s\n",
					 func, group->name, np->name);
		}

		if (pull >= 0)
			pinctrl_tegra30_set_pull(ctrl, group->reg, pull);

		if (in >= 0)
			pinctrl_tegra30_set_input(ctrl, group->reg, in);

		if (tri >= 0)
			pinctrl_tegra30_set_tristate(ctrl, group->reg, tri);

		if (od >= 0)
			pinctrl_tegra30_set_opendrain(ctrl, group->reg, od);

		if (ior >= 0)
			pinctrl_tegra30_set_ioreset(ctrl, group->reg, ior);
	}

	return 0;
}

static struct pinctrl_ops pinctrl_tegra30_ops = {
	.set_state = pinctrl_tegra30_set_state,
};

static int pinctrl_tegra30_probe(struct device_d *dev)
{
	struct resource *iores;
	struct pinctrl_tegra30 *ctrl;
	int i, ret;
	u32 **regs;

	ctrl = xzalloc(sizeof(*ctrl));

	/*
	 * Tegra pincontrol is split out into four independent memory ranges:
	 * tristate control, function mux, pullup/down control, pad control
	 * (from lowest to highest hardware address).
	 * We are only interested in the first three for now.
	 */
	regs = (u32 **)&ctrl->regs;
	for (i = 0; i <= 1; i++) {
		iores = dev_request_mem_resource(dev, i);
		if (IS_ERR(iores)) {
			dev_err(dev, "Could not get iomem region %d\n", i);
			return PTR_ERR(iores);
		}
		regs[i] = IOMEM(iores->start);
	}

	dev_get_drvdata(dev, (const void **)&ctrl->drvdata);

	ctrl->pinctrl.dev = dev;
	ctrl->pinctrl.ops = &pinctrl_tegra30_ops;

	ret = pinctrl_register(&ctrl->pinctrl);
	if (ret) {
		free(ctrl);
		return ret;
	}

	of_pinctrl_select_state(dev->device_node, "boot");

	return 0;
}

static __maybe_unused struct of_device_id pinctrl_tegra30_dt_ids[] = {
	{
#ifdef CONFIG_ARCH_TEGRA_3x_SOC
		.compatible = "nvidia,tegra30-pinmux",
		.data = &tegra30_drvdata,
	}, {
#endif
#ifdef CONFIG_ARCH_TEGRA_124_SOC
		.compatible = "nvidia,tegra124-pinmux",
		.data = &tegra124_drvdata,
	}, {
#endif
		/* sentinel */
	}
};

static struct driver_d pinctrl_tegra30_driver = {
	.name		= "pinctrl-tegra30",
	.probe		= pinctrl_tegra30_probe,
	.of_compatible	= DRV_OF_COMPAT(pinctrl_tegra30_dt_ids),
};

static int pinctrl_tegra30_init(void)
{
	return platform_driver_register(&pinctrl_tegra30_driver);
}
core_initcall(pinctrl_tegra30_init);
