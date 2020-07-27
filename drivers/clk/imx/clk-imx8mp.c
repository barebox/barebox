// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright 2019 NXP.
 */

#include <dt-bindings/clock/imx8mp-clock.h>
#include <io.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/types.h>
#include <of_address.h>

#include "clk.h"

static const char *pll_ref_sels[] = { "osc_24m", "dummy", "dummy", "dummy", };
static const char *audio_pll1_bypass_sels[] = {"audio_pll1", "audio_pll1_ref_sel", };
static const char *audio_pll2_bypass_sels[] = {"audio_pll2", "audio_pll2_ref_sel", };
static const char *video_pll1_bypass_sels[] = {"video_pll1", "video_pll1_ref_sel", };
static const char *dram_pll_bypass_sels[] = {"dram_pll", "dram_pll_ref_sel", };
static const char *gpu_pll_bypass_sels[] = {"gpu_pll", "gpu_pll_ref_sel", };
static const char *vpu_pll_bypass_sels[] = {"vpu_pll", "vpu_pll_ref_sel", };
static const char *arm_pll_bypass_sels[] = {"arm_pll", "arm_pll_ref_sel", };
static const char *sys_pll1_bypass_sels[] = {"sys_pll1", "sys_pll1_ref_sel", };
static const char *sys_pll2_bypass_sels[] = {"sys_pll2", "sys_pll2_ref_sel", };
static const char *sys_pll3_bypass_sels[] = {"sys_pll3", "sys_pll3_ref_sel", };

static const char *imx8mp_a53_sels[] = {"osc_24m", "arm_pll_out", "sys_pll2_500m",
					       "sys_pll2_1000m", "sys_pll1_800m", "sys_pll1_400m",
					       "audio_pll1_out", "sys_pll3_out", };

static const char *imx8mp_a53_core_sels[] = {"arm_a53_div", "arm_pll_out", };

static const char *imx8mp_m7_sels[] = {"osc_24m", "sys_pll2_200m", "sys_pll2_250m",
					      "vpu_pll_out", "sys_pll1_800m", "audio_pll1_out",
					      "video_pll1_out", "sys_pll3_out", };

static const char *imx8mp_ml_sels[] = {"osc_24m", "gpu_pll_out", "sys_pll1_800m",
					      "sys_pll3_out", "sys_pll2_1000m", "audio_pll1_out",
					      "video_pll1_out", "audio_pll2_out", };

static const char *imx8mp_gpu3d_core_sels[] = {"osc_24m", "gpu_pll_out", "sys_pll1_800m",
						      "sys_pll3_out", "sys_pll2_1000m", "audio_pll1_out",
						      "video_pll1_out", "audio_pll2_out", };

static const char *imx8mp_gpu3d_shader_sels[] = {"osc_24m", "gpu_pll_out", "sys_pll1_800m",
							"sys_pll3_out", "sys_pll2_1000m", "audio_pll1_out",
							"video_pll1_out", "audio_pll2_out", };

static const char *imx8mp_gpu2d_sels[] = {"osc_24m", "gpu_pll_out", "sys_pll1_800m",
						 "sys_pll3_out", "sys_pll2_1000m", "audio_pll1_out",
						 "video_pll1_out", "audio_pll2_out", };

static const char *imx8mp_audio_axi_sels[] = {"osc_24m", "gpu_pll_out", "sys_pll1_800m",
						     "sys_pll3_out", "sys_pll2_1000m", "audio_pll1_out",
						     "video_pll1_out", "audio_pll2_out", };

static const char *imx8mp_hsio_axi_sels[] = {"osc_24m", "sys_pll2_500m", "sys_pll1_800m",
						    "sys_pll2_100m", "sys_pll2_200m", "clk_ext2",
						    "clk_ext4", "audio_pll2_out", };

static const char *imx8mp_media_isp_sels[] = {"osc_24m", "sys_pll2_1000m", "sys_pll1_800m",
						     "sys_pll3_out", "sys_pll1_400m", "audio_pll2_out",
						     "clk_ext1", "sys_pll2_500m", };

static const char *imx8mp_main_axi_sels[] = {"osc_24m", "sys_pll2_333m", "sys_pll1_800m",
						    "sys_pll2_250m", "sys_pll2_1000m", "audio_pll1_out",
						    "video_pll1_out", "sys_pll1_100m",};

static const char *imx8mp_enet_axi_sels[] = {"osc_24m", "sys_pll1_266m", "sys_pll1_800m",
						    "sys_pll2_250m", "sys_pll2_200m", "audio_pll1_out",
						    "video_pll1_out", "sys_pll3_out", };

static const char *imx8mp_nand_usdhc_sels[] = {"osc_24m", "sys_pll1_266m", "sys_pll1_800m",
						      "sys_pll2_200m", "sys_pll1_133m", "sys_pll3_out",
						      "sys_pll2_250m", "audio_pll1_out", };

static const char *imx8mp_vpu_bus_sels[] = {"osc_24m", "sys_pll1_800m", "vpu_pll_out",
						   "audio_pll2_out", "sys_pll3_out", "sys_pll2_1000m",
						   "sys_pll2_200m", "sys_pll1_100m", };

static const char *imx8mp_media_axi_sels[] = {"osc_24m", "sys_pll2_1000m", "sys_pll1_800m",
						     "sys_pll3_out", "sys_pll1_40m", "audio_pll2_out",
						     "clk_ext1", "sys_pll2_500m", };

static const char *imx8mp_media_apb_sels[] = {"osc_24m", "sys_pll2_125m", "sys_pll1_800m",
						     "sys_pll3_out", "sys_pll1_40m", "audio_pll2_out",
						     "clk_ext1", "sys_pll1_133m", };

static const char *imx8mp_gpu_axi_sels[] = {"osc_24m", "sys_pll1_800m", "gpu_pll_out",
						   "sys_pll3_out", "sys_pll2_1000m", "audio_pll1_out",
						   "video_pll1_out", "audio_pll2_out", };

static const char *imx8mp_gpu_ahb_sels[] = {"osc_24m", "sys_pll1_800m", "gpu_pll_out",
						   "sys_pll3_out", "sys_pll2_1000m", "audio_pll1_out",
						   "video_pll1_out", "audio_pll2_out", };

static const char *imx8mp_noc_sels[] = {"osc_24m", "sys_pll1_800m", "sys_pll3_out",
					       "sys_pll2_1000m", "sys_pll2_500m", "audio_pll1_out",
					       "video_pll1_out", "audio_pll2_out", };

static const char *imx8mp_noc_io_sels[] = {"osc_24m", "sys_pll1_800m", "sys_pll3_out",
						  "sys_pll2_1000m", "sys_pll2_500m", "audio_pll1_out",
						  "video_pll1_out", "audio_pll2_out", };

static const char *imx8mp_ml_axi_sels[] = {"osc_24m", "sys_pll1_800m", "gpu_pll_out",
						  "sys_pll3_out", "sys_pll2_1000m", "audio_pll1_out",
						  "video_pll1_out", "audio_pll2_out", };

static const char *imx8mp_ml_ahb_sels[] = {"osc_24m", "sys_pll1_800m", "gpu_pll_out",
						  "sys_pll3_out", "sys_pll2_1000m", "audio_pll1_out",
						  "video_pll1_out", "audio_pll2_out", };

static const char *imx8mp_ahb_sels[] = {"osc_24m", "sys_pll1_133m", "sys_pll1_800m",
					       "sys_pll1_400m", "sys_pll2_125m", "sys_pll3_out",
					       "audio_pll1_out", "video_pll1_out", };

static const char *imx8mp_audio_ahb_sels[] = {"osc_24m", "sys_pll2_500m", "sys_pll1_800m",
						     "sys_pll2_1000m", "sys_pll2_166m", "sys_pll3_out",
						     "audio_pll1_out", "video_pll1_out", };

static const char *imx8mp_mipi_dsi_esc_rx_sels[] = {"osc_24m", "sys_pll2_100m", "sys_pll1_80m",
							   "sys_pll1_800m", "sys_pll2_1000m",
							   "sys_pll3_out", "clk_ext3", "audio_pll2_out", };

static const char *imx8mp_dram_alt_sels[] = {"osc_24m", "sys_pll1_800m", "sys_pll1_100m",
						    "sys_pll2_500m", "sys_pll2_1000m", "sys_pll3_out",
						    "audio_pll1_out", "sys_pll1_266m", };

static const char *imx8mp_dram_apb_sels[] = {"osc_24m", "sys_pll2_200m", "sys_pll1_40m",
						    "sys_pll1_160m", "sys_pll1_800m", "sys_pll3_out",
						    "sys_pll2_250m", "audio_pll2_out", };

static const char *imx8mp_vpu_g1_sels[] = {"osc_24m", "vpu_pll_out", "sys_pll1_800m",
						  "sys_pll2_1000m", "sys_pll1_100m", "sys_pll2_125m",
						  "sys_pll3_out", "audio_pll1_out", };

static const char *imx8mp_vpu_g2_sels[] = {"osc_24m", "vpu_pll_out", "sys_pll1_800m",
						  "sys_pll2_1000m", "sys_pll1_100m", "sys_pll2_125m",
						  "sys_pll3_out", "audio_pll1_out", };

static const char *imx8mp_can1_sels[] = {"osc_24m", "sys_pll2_200m", "sys_pll1_40m",
						"sys_pll1_160m", "sys_pll1_800m", "sys_pll3_out",
						"sys_pll2_250m", "audio_pll2_out", };

static const char *imx8mp_can2_sels[] = {"osc_24m", "sys_pll2_200m", "sys_pll1_40m",
						"sys_pll1_160m", "sys_pll1_800m", "sys_pll3_out",
						"sys_pll2_250m", "audio_pll2_out", };

static const char *imx8mp_memrepair_sels[] = {"osc_24m", "sys_pll1_160m", "sys_pll2_50m",
						     "sys_pll3_out", "audio_pll1_out", "video_pll1_out",
						     "audio_pll2_out", "sys_pll1_133m", };

static const char *imx8mp_pcie_phy_sels[] = {"osc_24m", "sys_pll2_100m", "sys_pll2_500m",
						    "clk_ext1", "clk_ext2", "clk_ext3",
						    "clk_ext4", "sys_pll1_400m", };

static const char *imx8mp_pcie_aux_sels[] = {"osc_24m", "sys_pll2_200m", "sys_pll2_50m",
						    "sys_pll3_out", "sys_pll2_100m", "sys_pll1_80m",
						    "sys_pll1_160m", "sys_pll1_200m", };

static const char *imx8mp_i2c5_sels[] = {"osc_24m", "sys_pll1_160m", "sys_pll2_50m",
						"sys_pll3_out", "audio_pll1_out", "video_pll1_out",
						"audio_pll2_out", "sys_pll1_133m", };

static const char *imx8mp_i2c6_sels[] = {"osc_24m", "sys_pll1_160m", "sys_pll2_50m",
						"sys_pll3_out", "audio_pll1_out", "video_pll1_out",
						"audio_pll2_out", "sys_pll1_133m", };

static const char *imx8mp_sai1_sels[] = {"osc_24m", "audio_pll1_out", "audio_pll2_out",
						"video_pll1_out", "sys_pll1_133m", "osc_hdmi",
						"clk_ext1", "clk_ext2", };

static const char *imx8mp_sai2_sels[] = {"osc_24m", "audio_pll1_out", "audio_pll2_out",
						"video_pll1_out", "sys_pll1_133m", "osc_hdmi",
						"clk_ext2", "clk_ext3", };

static const char *imx8mp_sai3_sels[] = {"osc_24m", "audio_pll1_out", "audio_pll2_out",
						"video_pll1_out", "sys_pll1_133m", "osc_hdmi",
						"clk_ext3", "clk_ext4", };

static const char *imx8mp_sai4_sels[] = {"osc_24m", "audio_pll1_out", "audio_pll2_out",
						"video_pll1_out", "sys_pll1_133m", "osc_hdmi",
						"clk_ext1", "clk_ext2", };

static const char *imx8mp_sai5_sels[] = {"osc_24m", "audio_pll1_out", "audio_pll2_out",
						"video_pll1_out", "sys_pll1_133m", "osc_hdmi",
						"clk_ext2", "clk_ext3", };

static const char *imx8mp_sai6_sels[] = {"osc_24m", "audio_pll1_out", "audio_pll2_out",
						"video_pll1_out", "sys_pll1_133m", "osc_hdmi",
						"clk_ext3", "clk_ext4", };

static const char *imx8mp_enet_qos_sels[] = {"osc_24m", "sys_pll2_125m", "sys_pll2_50m",
						    "sys_pll2_100m", "sys_pll1_160m", "audio_pll1_out",
						    "video_pll1_out", "clk_ext4", };

static const char *imx8mp_enet_qos_timer_sels[] = {"osc_24m", "sys_pll2_100m", "audio_pll1_out",
							  "clk_ext1", "clk_ext2", "clk_ext3",
							  "clk_ext4", "video_pll1_out", };

static const char *imx8mp_enet_ref_sels[] = {"osc_24m", "sys_pll2_125m", "sys_pll2_50m",
						    "sys_pll2_100m", "sys_pll1_160m", "audio_pll1_out",
						    "video_pll1_out", "clk_ext4", };

static const char *imx8mp_enet_timer_sels[] = {"osc_24m", "sys_pll2_100m", "audio_pll1_out",
						      "clk_ext1", "clk_ext2", "clk_ext3",
						      "clk_ext4", "video_pll1_out", };

static const char *imx8mp_enet_phy_ref_sels[] = {"osc_24m", "sys_pll2_50m", "sys_pll2_125m",
							"sys_pll2_200m", "sys_pll2_500m", "audio_pll1_out",
							"video_pll1_out", "audio_pll2_out", };

static const char *imx8mp_nand_sels[] = {"osc_24m", "sys_pll2_500m", "audio_pll1_out",
						"sys_pll1_400m", "audio_pll2_out", "sys_pll3_out",
						"sys_pll2_250m", "video_pll1_out", };

static const char *imx8mp_qspi_sels[] = {"osc_24m", "sys_pll1_400m", "sys_pll2_333m",
						"sys_pll2_500m", "audio_pll2_out", "sys_pll1_266m",
						"sys_pll3_out", "sys_pll1_100m", };

static const char *imx8mp_usdhc1_sels[] = {"osc_24m", "sys_pll1_400m", "sys_pll1_800m",
						  "sys_pll2_500m", "sys_pll3_out", "sys_pll1_266m",
						  "audio_pll2_out", "sys_pll1_100m", };

static const char *imx8mp_usdhc2_sels[] = {"osc_24m", "sys_pll1_400m", "sys_pll1_800m",
						  "sys_pll2_500m", "sys_pll3_out", "sys_pll1_266m",
						  "audio_pll2_out", "sys_pll1_100m", };

static const char *imx8mp_i2c1_sels[] = {"osc_24m", "sys_pll1_160m", "sys_pll2_50m",
						"sys_pll3_out", "audio_pll1_out", "video_pll1_out",
						"audio_pll2_out", "sys_pll1_133m", };

static const char *imx8mp_i2c2_sels[] = {"osc_24m", "sys_pll1_160m", "sys_pll2_50m",
						"sys_pll3_out", "audio_pll1_out", "video_pll1_out",
						"audio_pll2_out", "sys_pll1_133m", };

static const char *imx8mp_i2c3_sels[] = {"osc_24m", "sys_pll1_160m", "sys_pll2_50m",
						"sys_pll3_out", "audio_pll1_out", "video_pll1_out",
						"audio_pll2_out", "sys_pll1_133m", };

static const char *imx8mp_i2c4_sels[] = {"osc_24m", "sys_pll1_160m", "sys_pll2_50m",
						"sys_pll3_out", "audio_pll1_out", "video_pll1_out",
						"audio_pll2_out", "sys_pll1_133m", };

static const char *imx8mp_uart1_sels[] = {"osc_24m", "sys_pll1_80m", "sys_pll2_200m",
						 "sys_pll2_100m", "sys_pll3_out", "clk_ext2",
						 "clk_ext4", "audio_pll2_out", };

static const char *imx8mp_uart2_sels[] = {"osc_24m", "sys_pll1_80m", "sys_pll2_200m",
						 "sys_pll2_100m", "sys_pll3_out", "clk_ext2",
						 "clk_ext3", "audio_pll2_out", };

static const char *imx8mp_uart3_sels[] = {"osc_24m", "sys_pll1_80m", "sys_pll2_200m",
						 "sys_pll2_100m", "sys_pll3_out", "clk_ext2",
						 "clk_ext4", "audio_pll2_out", };

static const char *imx8mp_uart4_sels[] = {"osc_24m", "sys_pll1_80m", "sys_pll2_200m",
						 "sys_pll2_100m", "sys_pll3_out", "clk_ext2",
						 "clk_ext3", "audio_pll2_out", };

static const char *imx8mp_usb_core_ref_sels[] = {"osc_24m", "sys_pll1_100m", "sys_pll1_40m",
							"sys_pll2_100m", "sys_pll2_200m", "clk_ext2",
							"clk_ext3", "audio_pll2_out", };

static const char *imx8mp_usb_phy_ref_sels[] = {"osc_24m", "sys_pll1_100m", "sys_pll1_40m",
						       "sys_pll2_100m", "sys_pll2_200m", "clk_ext2",
						       "clk_ext3", "audio_pll2_out", };

static const char *imx8mp_gic_sels[] = {"osc_24m", "sys_pll2_200m", "sys_pll1_40m",
					       "sys_pll2_100m", "sys_pll1_800m",
					       "sys_pll2_500m", "clk_ext4", "audio_pll2_out" };

static const char *imx8mp_ecspi1_sels[] = {"osc_24m", "sys_pll2_200m", "sys_pll1_40m",
						  "sys_pll1_160m", "sys_pll1_800m", "sys_pll3_out",
						  "sys_pll2_250m", "audio_pll2_out", };

static const char *imx8mp_ecspi2_sels[] = {"osc_24m", "sys_pll2_200m", "sys_pll1_40m",
						  "sys_pll1_160m", "sys_pll1_800m", "sys_pll3_out",
						  "sys_pll2_250m", "audio_pll2_out", };

static const char *imx8mp_pwm1_sels[] = {"osc_24m", "sys_pll2_100m", "sys_pll1_160m",
						"sys_pll1_40m", "sys_pll3_out", "clk_ext1",
						"sys_pll1_80m", "video_pll1_out", };

static const char *imx8mp_pwm2_sels[] = {"osc_24m", "sys_pll2_100m", "sys_pll1_160m",
						"sys_pll1_40m", "sys_pll3_out", "clk_ext1",
						"sys_pll1_80m", "video_pll1_out", };

static const char *imx8mp_pwm3_sels[] = {"osc_24m", "sys_pll2_100m", "sys_pll1_160m",
						"sys_pll1_40m", "sys_pll3_out", "clk_ext2",
						"sys_pll1_80m", "video_pll1_out", };

static const char *imx8mp_pwm4_sels[] = {"osc_24m", "sys_pll2_100m", "sys_pll1_160m",
						"sys_pll1_40m", "sys_pll3_out", "clk_ext2",
						"sys_pll1_80m", "video_pll1_out", };

static const char *imx8mp_gpt1_sels[] = {"osc_24m", "sys_pll2_100m", "sys_pll1_400m",
						"sys_pll1_40m", "video_pll1_out", "sys_pll1_80m",
						"audio_pll1_out", "clk_ext1" };

static const char *imx8mp_gpt2_sels[] = {"osc_24m", "sys_pll2_100m", "sys_pll1_400m",
						"sys_pll1_40m", "video_pll1_out", "sys_pll1_80m",
						"audio_pll1_out", "clk_ext2" };

static const char *imx8mp_gpt3_sels[] = {"osc_24m", "sys_pll2_100m", "sys_pll1_400m",
						"sys_pll1_40m", "video_pll1_out", "sys_pll1_80m",
						"audio_pll1_out", "clk_ext3" };

static const char *imx8mp_gpt4_sels[] = {"osc_24m", "sys_pll2_100m", "sys_pll1_400m",
						"sys_pll1_40m", "video_pll1_out", "sys_pll1_80m",
						"audio_pll1_out", "clk_ext1" };

static const char *imx8mp_gpt5_sels[] = {"osc_24m", "sys_pll2_100m", "sys_pll1_400m",
						"sys_pll1_40m", "video_pll1_out", "sys_pll1_80m",
						"audio_pll1_out", "clk_ext2" };

static const char *imx8mp_gpt6_sels[] = {"osc_24m", "sys_pll2_100m", "sys_pll1_400m",
						"sys_pll1_40m", "video_pll1_out", "sys_pll1_80m",
						"audio_pll1_out", "clk_ext3" };

static const char *imx8mp_wdog_sels[] = {"osc_24m", "sys_pll1_133m", "sys_pll1_160m",
						"vpu_pll_out", "sys_pll2_125m", "sys_pll3_out",
						"sys_pll1_80m", "sys_pll2_166m" };

static const char *imx8mp_wrclk_sels[] = {"osc_24m", "sys_pll1_40m", "vpu_pll_out",
						 "sys_pll3_out", "sys_pll2_200m", "sys_pll1_266m",
						 "sys_pll2_500m", "sys_pll1_100m" };

static const char *imx8mp_ipp_do_clko1_sels[] = {"osc_24m", "sys_pll1_800m", "sys_pll1_133m",
							"sys_pll1_200m", "audio_pll2_out", "sys_pll2_500m",
							"vpu_pll_out", "sys_pll1_80m" };

static const char *imx8mp_ipp_do_clko2_sels[] = {"osc_24m", "sys_pll2_200m", "sys_pll1_400m",
							"sys_pll1_166m", "sys_pll3_out", "audio_pll1_out",
							"video_pll1_out", "osc_32k" };

static const char *imx8mp_hdmi_fdcc_tst_sels[] = {"osc_24m", "sys_pll1_266m", "sys_pll2_250m",
							 "sys_pll1_800m", "sys_pll2_1000m", "sys_pll3_out",
							 "audio_pll2_out", "video_pll1_out", };

static const char *imx8mp_hdmi_24m_sels[] = {"osc_24m", "sys_pll1_160m", "sys_pll2_50m",
						    "sys_pll3_out", "audio_pll1_out", "video_pll1_out",
						    "audio_pll2_out", "sys_pll1_133m", };

static const char *imx8mp_hdmi_ref_266m_sels[] = {"osc_24m", "sys_pll1_400m", "sys_pll3_out",
							 "sys_pll2_333m", "sys_pll1_266m", "sys_pll2_200m",
							 "audio_pll1_out", "video_pll1_out", };

static const char *imx8mp_usdhc3_sels[] = {"osc_24m", "sys_pll1_400m", "sys_pll1_800m",
						  "sys_pll2_500m", "sys_pll3_out", "sys_pll1_266m",
						  "audio_pll2_out", "sys_pll1_100m", };

static const char *imx8mp_media_cam1_pix_sels[] = {"osc_24m", "sys_pll1_266m", "sys_pll2_250m",
							  "sys_pll1_800m", "sys_pll2_1000m",
							  "sys_pll3_out", "audio_pll2_out",
							  "video_pll1_out", };

static const char *imx8mp_media_mipi_phy1_ref_sels[] = {"osc_24m", "sys_pll2_333m", "sys_pll2_100m",
							       "sys_pll1_800m", "sys_pll2_1000m",
							       "clk_ext2", "audio_pll2_out",
							       "video_pll1_out", };

static const char *imx8mp_media_disp1_pix_sels[] = {"osc_24m", "video_pll1_out", "audio_pll2_out",
							   "audio_pll1_out", "sys_pll1_800m",
							   "sys_pll2_1000m", "sys_pll3_out", "clk_ext4", };

static const char *imx8mp_media_cam2_pix_sels[] = {"osc_24m", "sys_pll1_266m", "sys_pll2_250m",
							  "sys_pll1_800m", "sys_pll2_1000m",
							  "sys_pll3_out", "audio_pll2_out",
							  "video_pll1_out", };

static const char *imx8mp_media_mipi_phy2_ref_sels[] = {"osc_24m", "sys_pll2_333m", "sys_pll2_100m",
							       "sys_pll1_800m", "sys_pll2_1000m",
							       "clk_ext2", "audio_pll2_out",
							       "video_pll1_out", };

static const char *imx8mp_media_mipi_csi2_esc_sels[] = {"osc_24m", "sys_pll2_100m", "sys_pll1_80m",
							       "sys_pll1_800m", "sys_pll2_1000m",
							       "sys_pll3_out", "clk_ext3",
							       "audio_pll2_out", };

static const char *imx8mp_pcie2_ctrl_sels[] = {"osc_24m", "sys_pll2_250m", "sys_pll2_200m",
						      "sys_pll1_266m", "sys_pll1_800m", "sys_pll2_500m",
						      "sys_pll2_333m", "sys_pll3_out", };

static const char *imx8mp_pcie2_phy_sels[] = {"osc_24m", "sys_pll2_100m", "sys_pll2_500m",
						     "clk_ext1", "clk_ext2", "clk_ext3",
						     "clk_ext4", "sys_pll1_400m", };

static const char *imx8mp_media_mipi_test_byte_sels[] = {"osc_24m", "sys_pll2_200m", "sys_pll2_50m",
								"sys_pll3_out", "sys_pll2_100m",
								"sys_pll1_80m", "sys_pll1_160m",
								"sys_pll1_200m", };

static const char *imx8mp_ecspi3_sels[] = {"osc_24m", "sys_pll2_200m", "sys_pll1_40m",
						  "sys_pll1_160m", "sys_pll1_800m", "sys_pll3_out",
						  "sys_pll2_250m", "audio_pll2_out", };

static const char *imx8mp_pdm_sels[] = {"osc_24m", "sys_pll2_100m", "audio_pll1_out",
					       "sys_pll1_800m", "sys_pll2_1000m", "sys_pll3_out",
					       "clk_ext3", "audio_pll2_out", };

static const char *imx8mp_vpu_vc8000e_sels[] = {"osc_24m", "vpu_pll_out", "sys_pll1_800m",
						       "sys_pll2_1000m", "audio_pll2_out", "sys_pll2_125m",
						       "sys_pll3_out", "audio_pll1_out", };

static const char *imx8mp_sai7_sels[] = {"osc_24m", "audio_pll1_out", "audio_pll2_out",
						"video_pll1_out", "sys_pll1_133m", "osc_hdmi",
						"clk_ext3", "clk_ext4", };

static const char *imx8mp_dram_core_sels[] = {"dram_pll_out", "dram_alt_root", };

static struct clk_onecell_data clk_data;
static struct clk *clks[IMX8MP_CLK_END];

static int imx8mp_clocks_init(struct device_node *ccm_np)
{
	struct device_node *anatop_np;
	void __iomem *ccm, *ana;
	int ret;

	anatop_np = of_find_compatible_node(NULL, NULL, "fsl,imx8mp-anatop");
	ana = of_iomap(anatop_np, 0);
	if (WARN_ON(!ana))
		return -ENOMEM;

	ccm = of_iomap(ccm_np, 0);
	if (WARN_ON(!ccm))
		return -ENOMEM;

	clks[IMX8MP_CLK_DUMMY] = clk_fixed("dummy", 0);
	clks[IMX8MP_CLK_24M] = of_clk_get_by_name(ccm_np, "osc_24m");
	clks[IMX8MP_CLK_32K] = of_clk_get_by_name(ccm_np, "osc_32k");
	clks[IMX8MP_CLK_EXT1] = of_clk_get_by_name(ccm_np, "clk_ext1");
	clks[IMX8MP_CLK_EXT2] = of_clk_get_by_name(ccm_np, "clk_ext2");
	clks[IMX8MP_CLK_EXT3] = of_clk_get_by_name(ccm_np, "clk_ext3");
	clks[IMX8MP_CLK_EXT4] = of_clk_get_by_name(ccm_np, "clk_ext4");

	clks[IMX8MP_AUDIO_PLL1_REF_SEL] = imx_clk_mux("audio_pll1_ref_sel", ana + 0x0, 0, 2, pll_ref_sels, ARRAY_SIZE(pll_ref_sels));
	clks[IMX8MP_AUDIO_PLL2_REF_SEL] = imx_clk_mux("audio_pll2_ref_sel", ana + 0x14, 0, 2, pll_ref_sels, ARRAY_SIZE(pll_ref_sels));
	clks[IMX8MP_VIDEO_PLL1_REF_SEL] = imx_clk_mux("video_pll1_ref_sel", ana + 0x28, 0, 2, pll_ref_sels, ARRAY_SIZE(pll_ref_sels));
	clks[IMX8MP_DRAM_PLL_REF_SEL] = imx_clk_mux("dram_pll_ref_sel", ana + 0x50, 0, 2, pll_ref_sels, ARRAY_SIZE(pll_ref_sels));
	clks[IMX8MP_GPU_PLL_REF_SEL] = imx_clk_mux("gpu_pll_ref_sel", ana + 0x64, 0, 2, pll_ref_sels, ARRAY_SIZE(pll_ref_sels));
	clks[IMX8MP_VPU_PLL_REF_SEL] = imx_clk_mux("vpu_pll_ref_sel", ana + 0x74, 0, 2, pll_ref_sels, ARRAY_SIZE(pll_ref_sels));
	clks[IMX8MP_ARM_PLL_REF_SEL] = imx_clk_mux("arm_pll_ref_sel", ana + 0x84, 0, 2, pll_ref_sels, ARRAY_SIZE(pll_ref_sels));
	clks[IMX8MP_SYS_PLL1_REF_SEL] = imx_clk_mux("sys_pll1_ref_sel", ana + 0x94, 0, 2, pll_ref_sels, ARRAY_SIZE(pll_ref_sels));
	clks[IMX8MP_SYS_PLL2_REF_SEL] = imx_clk_mux("sys_pll2_ref_sel", ana + 0x104, 0, 2, pll_ref_sels, ARRAY_SIZE(pll_ref_sels));
	clks[IMX8MP_SYS_PLL3_REF_SEL] = imx_clk_mux("sys_pll3_ref_sel", ana + 0x114, 0, 2, pll_ref_sels, ARRAY_SIZE(pll_ref_sels));

	clks[IMX8MP_AUDIO_PLL1] = imx_clk_pll14xx("audio_pll1", "audio_pll1_ref_sel", ana, &imx_1443x_pll);
	clks[IMX8MP_AUDIO_PLL2] = imx_clk_pll14xx("audio_pll2", "audio_pll2_ref_sel", ana + 0x14, &imx_1443x_pll);
	clks[IMX8MP_VIDEO_PLL1] = imx_clk_pll14xx("video_pll1", "video_pll1_ref_sel", ana + 0x28, &imx_1443x_pll);
	clks[IMX8MP_GPU_PLL] = imx_clk_pll14xx("gpu_pll", "gpu_pll_ref_sel", ana + 0x64, &imx_1416x_pll);
	clks[IMX8MP_VPU_PLL] = imx_clk_pll14xx("vpu_pll", "vpu_pll_ref_sel", ana + 0x74, &imx_1416x_pll);
	clks[IMX8MP_ARM_PLL] = imx_clk_pll14xx("arm_pll", "arm_pll_ref_sel", ana + 0x84, &imx_1416x_pll);
	clks[IMX8MP_SYS_PLL1] = imx_clk_pll14xx("sys_pll1", "sys_pll1_ref_sel", ana + 0x94, &imx_1416x_pll);
	clks[IMX8MP_SYS_PLL2] = imx_clk_pll14xx("sys_pll2", "sys_pll2_ref_sel", ana + 0x104, &imx_1416x_pll);
	clks[IMX8MP_SYS_PLL3] = imx_clk_pll14xx("sys_pll3", "sys_pll3_ref_sel", ana + 0x114, &imx_1416x_pll);

	clks[IMX8MP_AUDIO_PLL1_BYPASS] = imx_clk_mux_flags("audio_pll1_bypass", ana, 4, 1, audio_pll1_bypass_sels, ARRAY_SIZE(audio_pll1_bypass_sels), CLK_SET_RATE_PARENT);
	clks[IMX8MP_AUDIO_PLL2_BYPASS] = imx_clk_mux_flags("audio_pll2_bypass", ana + 0x14, 4, 1, audio_pll2_bypass_sels, ARRAY_SIZE(audio_pll2_bypass_sels), CLK_SET_RATE_PARENT);
	clks[IMX8MP_VIDEO_PLL1_BYPASS] = imx_clk_mux_flags("video_pll1_bypass", ana + 0x28, 4, 1, video_pll1_bypass_sels, ARRAY_SIZE(video_pll1_bypass_sels), CLK_SET_RATE_PARENT);
	clks[IMX8MP_DRAM_PLL_BYPASS] = imx_clk_mux_flags("dram_pll_bypass", ana + 0x50, 4, 1, dram_pll_bypass_sels, ARRAY_SIZE(dram_pll_bypass_sels), CLK_SET_RATE_PARENT);
	clks[IMX8MP_GPU_PLL_BYPASS] = imx_clk_mux_flags("gpu_pll_bypass", ana + 0x64, 4, 1, gpu_pll_bypass_sels, ARRAY_SIZE(gpu_pll_bypass_sels), CLK_SET_RATE_PARENT);
	clks[IMX8MP_VPU_PLL_BYPASS] = imx_clk_mux_flags("vpu_pll_bypass", ana + 0x74, 4, 1, vpu_pll_bypass_sels, ARRAY_SIZE(vpu_pll_bypass_sels), CLK_SET_RATE_PARENT);
	clks[IMX8MP_ARM_PLL_BYPASS] = imx_clk_mux_flags("arm_pll_bypass", ana + 0x84, 4, 1, arm_pll_bypass_sels, ARRAY_SIZE(arm_pll_bypass_sels), CLK_SET_RATE_PARENT);
	clks[IMX8MP_SYS_PLL1_BYPASS] = imx_clk_mux_flags("sys_pll1_bypass", ana + 0x94, 4, 1, sys_pll1_bypass_sels, ARRAY_SIZE(sys_pll1_bypass_sels), CLK_SET_RATE_PARENT);
	clks[IMX8MP_SYS_PLL2_BYPASS] = imx_clk_mux_flags("sys_pll2_bypass", ana + 0x104, 4, 1, sys_pll2_bypass_sels, ARRAY_SIZE(sys_pll2_bypass_sels), CLK_SET_RATE_PARENT);
	clks[IMX8MP_SYS_PLL3_BYPASS] = imx_clk_mux_flags("sys_pll3_bypass", ana + 0x114, 4, 1, sys_pll3_bypass_sels, ARRAY_SIZE(sys_pll3_bypass_sels), CLK_SET_RATE_PARENT);

	clks[IMX8MP_AUDIO_PLL1_OUT] = imx_clk_gate("audio_pll1_out", "audio_pll1_bypass", ana, 13);
	clks[IMX8MP_AUDIO_PLL2_OUT] = imx_clk_gate("audio_pll2_out", "audio_pll2_bypass", ana + 0x14, 13);
	clks[IMX8MP_VIDEO_PLL1_OUT] = imx_clk_gate("video_pll1_out", "video_pll1_bypass", ana + 0x28, 13);
	clks[IMX8MP_DRAM_PLL_OUT] = imx_clk_gate("dram_pll_out", "dram_pll_bypass", ana + 0x50, 13);
	clks[IMX8MP_GPU_PLL_OUT] = imx_clk_gate("gpu_pll_out", "gpu_pll_bypass", ana + 0x64, 11);
	clks[IMX8MP_VPU_PLL_OUT] = imx_clk_gate("vpu_pll_out", "vpu_pll_bypass", ana + 0x74, 11);
	clks[IMX8MP_ARM_PLL_OUT] = imx_clk_gate("arm_pll_out", "arm_pll_bypass", ana + 0x84, 11);
	clks[IMX8MP_SYS_PLL1_OUT] = imx_clk_gate("sys_pll1_out", "sys_pll1_bypass", ana + 0x94, 11);
	clks[IMX8MP_SYS_PLL2_OUT] = imx_clk_gate("sys_pll2_out", "sys_pll2_bypass", ana + 0x104, 11);
	clks[IMX8MP_SYS_PLL3_OUT] = imx_clk_gate("sys_pll3_out", "sys_pll3_bypass", ana + 0x114, 11);

	clks[IMX8MP_SYS_PLL1_40M] = imx_clk_fixed_factor("sys_pll1_40m", "sys_pll1_out", 1, 20);
	clks[IMX8MP_SYS_PLL1_80M] = imx_clk_fixed_factor("sys_pll1_80m", "sys_pll1_out", 1, 10);
	clks[IMX8MP_SYS_PLL1_100M] = imx_clk_fixed_factor("sys_pll1_100m", "sys_pll1_out", 1, 8);
	clks[IMX8MP_SYS_PLL1_133M] = imx_clk_fixed_factor("sys_pll1_133m", "sys_pll1_out", 1, 6);
	clks[IMX8MP_SYS_PLL1_160M] = imx_clk_fixed_factor("sys_pll1_160m", "sys_pll1_out", 1, 5);
	clks[IMX8MP_SYS_PLL1_200M] = imx_clk_fixed_factor("sys_pll1_200m", "sys_pll1_out", 1, 4);
	clks[IMX8MP_SYS_PLL1_266M] = imx_clk_fixed_factor("sys_pll1_266m", "sys_pll1_out", 1, 3);
	clks[IMX8MP_SYS_PLL1_400M] = imx_clk_fixed_factor("sys_pll1_400m", "sys_pll1_out", 1, 2);
	clks[IMX8MP_SYS_PLL1_800M] = imx_clk_fixed_factor("sys_pll1_800m", "sys_pll1_out", 1, 1);

	clks[IMX8MP_SYS_PLL2_50M] = imx_clk_fixed_factor("sys_pll2_50m", "sys_pll2_out", 1, 20);
	clks[IMX8MP_SYS_PLL2_100M] = imx_clk_fixed_factor("sys_pll2_100m", "sys_pll2_out", 1, 10);
	clks[IMX8MP_SYS_PLL2_125M] = imx_clk_fixed_factor("sys_pll2_125m", "sys_pll2_out", 1, 8);
	clks[IMX8MP_SYS_PLL2_166M] = imx_clk_fixed_factor("sys_pll2_166m", "sys_pll2_out", 1, 6);
	clks[IMX8MP_SYS_PLL2_200M] = imx_clk_fixed_factor("sys_pll2_200m", "sys_pll2_out", 1, 5);
	clks[IMX8MP_SYS_PLL2_250M] = imx_clk_fixed_factor("sys_pll2_250m", "sys_pll2_out", 1, 4);
	clks[IMX8MP_SYS_PLL2_333M] = imx_clk_fixed_factor("sys_pll2_333m", "sys_pll2_out", 1, 3);
	clks[IMX8MP_SYS_PLL2_500M] = imx_clk_fixed_factor("sys_pll2_500m", "sys_pll2_out", 1, 2);
	clks[IMX8MP_SYS_PLL2_1000M] = imx_clk_fixed_factor("sys_pll2_1000m", "sys_pll2_out", 1, 1);

	clks[IMX8MP_CLK_A53_SRC] = imx_clk_mux2("arm_a53_src", ccm + 0x8000, 24, 3, imx8mp_a53_sels, ARRAY_SIZE(imx8mp_a53_sels));
	clks[IMX8MP_CLK_M7_SRC] = imx_clk_mux2("arm_m7_src", ccm + 0x8080, 24, 3, imx8mp_m7_sels, ARRAY_SIZE(imx8mp_m7_sels));
	clks[IMX8MP_CLK_ML_SRC] = imx_clk_mux2("ml_src", ccm + 0x8100, 24, 3, imx8mp_ml_sels, ARRAY_SIZE(imx8mp_ml_sels));
	clks[IMX8MP_CLK_GPU3D_CORE_SRC] = imx_clk_mux2("gpu3d_core_src", ccm + 0x8180, 24, 3,  imx8mp_gpu3d_core_sels, ARRAY_SIZE(imx8mp_gpu3d_core_sels));
	clks[IMX8MP_CLK_GPU3D_SHADER_SRC] = imx_clk_mux2("gpu3d_shader_src", ccm + 0x8200, 24, 3, imx8mp_gpu3d_shader_sels, ARRAY_SIZE(imx8mp_gpu3d_shader_sels));
	clks[IMX8MP_CLK_GPU2D_SRC] = imx_clk_mux2("gpu2d_src", ccm + 0x8280, 24, 3, imx8mp_gpu2d_sels, ARRAY_SIZE(imx8mp_gpu2d_sels));
	clks[IMX8MP_CLK_AUDIO_AXI_SRC] = imx_clk_mux2("audio_axi_src", ccm + 0x8300, 24, 3, imx8mp_audio_axi_sels, ARRAY_SIZE(imx8mp_audio_axi_sels));
	clks[IMX8MP_CLK_HSIO_AXI_SRC] = imx_clk_mux2("hsio_axi_src", ccm + 0x8380, 24, 3, imx8mp_hsio_axi_sels, ARRAY_SIZE(imx8mp_hsio_axi_sels));
	clks[IMX8MP_CLK_MEDIA_ISP_SRC] = imx_clk_mux2("media_isp_src", ccm + 0x8400, 24, 3, imx8mp_media_isp_sels, ARRAY_SIZE(imx8mp_media_isp_sels));
	clks[IMX8MP_CLK_A53_CG] = imx_clk_gate3("arm_a53_cg", "arm_a53_src", ccm + 0x8000, 28);
	clks[IMX8MP_CLK_M4_CG] = imx_clk_gate3("arm_m7_cg", "arm_m7_src", ccm + 0x8080, 28);
	clks[IMX8MP_CLK_ML_CG] = imx_clk_gate3("ml_cg", "ml_src", ccm + 0x8100, 28);
	clks[IMX8MP_CLK_GPU3D_CORE_CG] = imx_clk_gate3("gpu3d_core_cg", "gpu3d_core_src", ccm + 0x8180, 28);
	clks[IMX8MP_CLK_GPU3D_SHADER_CG] = imx_clk_gate3("gpu3d_shader_cg", "gpu3d_shader_src", ccm + 0x8200, 28);
	clks[IMX8MP_CLK_GPU2D_CG] = imx_clk_gate3("gpu2d_cg", "gpu2d_src", ccm + 0x8280, 28);
	clks[IMX8MP_CLK_AUDIO_AXI_CG] = imx_clk_gate3("audio_axi_cg", "audio_axi_src", ccm + 0x8300, 28);
	clks[IMX8MP_CLK_HSIO_AXI_CG] = imx_clk_gate3("hsio_axi_cg", "hsio_axi_src", ccm + 0x8380, 28);
	clks[IMX8MP_CLK_MEDIA_ISP_CG] = imx_clk_gate3("media_isp_cg", "media_isp_src", ccm + 0x8400, 28);
	clks[IMX8MP_CLK_A53_DIV] = imx_clk_divider2("arm_a53_div", "arm_a53_cg", ccm + 0x8000, 0, 3);
	clks[IMX8MP_CLK_M7_DIV] = imx_clk_divider2("arm_m7_div", "arm_m7_cg", ccm + 0x8080, 0, 3);
	clks[IMX8MP_CLK_ML_DIV] = imx_clk_divider2("ml_div", "ml_cg", ccm + 0x8100, 0, 3);
	clks[IMX8MP_CLK_GPU3D_CORE_DIV] = imx_clk_divider2("gpu3d_core_div", "gpu3d_core_cg", ccm + 0x8180, 0, 3);
	clks[IMX8MP_CLK_GPU3D_SHADER_DIV] = imx_clk_divider2("gpu3d_shader_div", "gpu3d_shader_cg", ccm + 0x8200, 0, 3);
	clks[IMX8MP_CLK_GPU2D_DIV] = imx_clk_divider2("gpu2d_div", "gpu2d_cg", ccm + 0x8280, 0, 3);
	clks[IMX8MP_CLK_AUDIO_AXI_DIV] = imx_clk_divider2("audio_axi_div", "audio_axi_cg", ccm + 0x8300, 0, 3);
	clks[IMX8MP_CLK_HSIO_AXI_DIV] = imx_clk_divider2("hsio_axi_div", "hsio_axi_cg", ccm + 0x8380, 0, 3);
	clks[IMX8MP_CLK_MEDIA_ISP_DIV] = imx_clk_divider2("media_isp_div", "media_isp_cg", ccm + 0x8400, 0, 3);

	/* CORE SEL */
	clks[IMX8MP_CLK_A53_CORE] = imx_clk_mux2("arm_a53_core", ccm + 0x9880, 24, 1, imx8mp_a53_core_sels, ARRAY_SIZE(imx8mp_a53_core_sels));

	clks[IMX8MP_CLK_MAIN_AXI] = imx8m_clk_composite_critical("main_axi", imx8mp_main_axi_sels, ccm + 0x8800);
	clks[IMX8MP_CLK_ENET_AXI] = imx8m_clk_composite("enet_axi", imx8mp_enet_axi_sels, ccm + 0x8880);
	clks[IMX8MP_CLK_NAND_USDHC_BUS] = imx8m_clk_composite_critical("nand_usdhc_bus", imx8mp_nand_usdhc_sels, ccm + 0x8900);
	clks[IMX8MP_CLK_VPU_BUS] = imx8m_clk_composite("vpu_bus", imx8mp_vpu_bus_sels, ccm + 0x8980);
	clks[IMX8MP_CLK_MEDIA_AXI] = imx8m_clk_composite("media_axi", imx8mp_media_axi_sels, ccm + 0x8a00);
	clks[IMX8MP_CLK_MEDIA_APB] = imx8m_clk_composite("media_apb", imx8mp_media_apb_sels, ccm + 0x8a80);
	clks[IMX8MP_CLK_HDMI_APB] = imx8m_clk_composite("hdmi_apb", imx8mp_media_apb_sels, ccm + 0x8b00);
	clks[IMX8MP_CLK_HDMI_AXI] = imx8m_clk_composite("hdmi_axi", imx8mp_media_axi_sels, ccm + 0x8b80);
	clks[IMX8MP_CLK_GPU_AXI] = imx8m_clk_composite("gpu_axi", imx8mp_gpu_axi_sels, ccm + 0x8c00);
	clks[IMX8MP_CLK_GPU_AHB] = imx8m_clk_composite("gpu_ahb", imx8mp_gpu_ahb_sels, ccm + 0x8c80);
	clks[IMX8MP_CLK_NOC] = imx8m_clk_composite_critical("noc", imx8mp_noc_sels, ccm + 0x8d00);
	clks[IMX8MP_CLK_NOC_IO] = imx8m_clk_composite_critical("noc_io", imx8mp_noc_io_sels, ccm + 0x8d80);
	clks[IMX8MP_CLK_ML_AXI] = imx8m_clk_composite("ml_axi", imx8mp_ml_axi_sels, ccm + 0x8e00);
	clks[IMX8MP_CLK_ML_AHB] = imx8m_clk_composite("ml_ahb", imx8mp_ml_ahb_sels, ccm + 0x8e80);

	clks[IMX8MP_CLK_AHB] = imx8m_clk_composite_critical("ahb_root", imx8mp_ahb_sels, ccm + 0x9000);
	clks[IMX8MP_CLK_AUDIO_AHB] = imx8m_clk_composite("audio_ahb", imx8mp_audio_ahb_sels, ccm + 0x9100);
	clks[IMX8MP_CLK_MIPI_DSI_ESC_RX] = imx8m_clk_composite("mipi_dsi_esc_rx", imx8mp_mipi_dsi_esc_rx_sels, ccm + 0x9200);

	clks[IMX8MP_CLK_IPG_ROOT] = imx_clk_divider2("ipg_root", "ahb_root", ccm + 0x9080, 0, 1);
	clks[IMX8MP_CLK_IPG_AUDIO_ROOT] = imx_clk_divider2("ipg_audio_root", "audio_ahb", ccm + 0x9180, 0, 1);

	clks[IMX8MP_CLK_DRAM_ALT] = imx8m_clk_composite("dram_alt", imx8mp_dram_alt_sels, ccm + 0xa000);
	clks[IMX8MP_CLK_DRAM_APB] = imx8m_clk_composite_critical("dram_apb", imx8mp_dram_apb_sels, ccm + 0xa080);
	clks[IMX8MP_CLK_VPU_G1] = imx8m_clk_composite("vpu_g1", imx8mp_vpu_g1_sels, ccm + 0xa100);
	clks[IMX8MP_CLK_VPU_G2] = imx8m_clk_composite("vpu_g2", imx8mp_vpu_g2_sels, ccm + 0xa180);
	clks[IMX8MP_CLK_CAN1] = imx8m_clk_composite("can1", imx8mp_can1_sels, ccm + 0xa200);
	clks[IMX8MP_CLK_CAN2] = imx8m_clk_composite("can2", imx8mp_can2_sels, ccm + 0xa280);
	clks[IMX8MP_CLK_MEMREPAIR] = imx8m_clk_composite("memrepair", imx8mp_memrepair_sels, ccm + 0xa300);
	clks[IMX8MP_CLK_PCIE_PHY] = imx8m_clk_composite("pcie_phy", imx8mp_pcie_phy_sels, ccm + 0xa380);
	clks[IMX8MP_CLK_PCIE_AUX] = imx8m_clk_composite("pcie_aux", imx8mp_pcie_aux_sels, ccm + 0xa400);
	clks[IMX8MP_CLK_I2C5] = imx8m_clk_composite("i2c5", imx8mp_i2c5_sels, ccm + 0xa480);
	clks[IMX8MP_CLK_I2C6] = imx8m_clk_composite("i2c6", imx8mp_i2c6_sels, ccm + 0xa500);
	clks[IMX8MP_CLK_SAI1] = imx8m_clk_composite("sai1", imx8mp_sai1_sels, ccm + 0xa580);
	clks[IMX8MP_CLK_SAI2] = imx8m_clk_composite("sai2", imx8mp_sai2_sels, ccm + 0xa600);
	clks[IMX8MP_CLK_SAI3] = imx8m_clk_composite("sai3", imx8mp_sai3_sels, ccm + 0xa680);
	clks[IMX8MP_CLK_SAI4] = imx8m_clk_composite("sai4", imx8mp_sai4_sels, ccm + 0xa700);
	clks[IMX8MP_CLK_SAI5] = imx8m_clk_composite("sai5", imx8mp_sai5_sels, ccm + 0xa780);
	clks[IMX8MP_CLK_SAI6] = imx8m_clk_composite("sai6", imx8mp_sai6_sels, ccm + 0xa800);
	clks[IMX8MP_CLK_ENET_QOS] = imx8m_clk_composite("enet_qos", imx8mp_enet_qos_sels, ccm + 0xa880);
	clks[IMX8MP_CLK_ENET_QOS_TIMER] = imx8m_clk_composite("enet_qos_timer", imx8mp_enet_qos_timer_sels, ccm + 0xa900);
	clks[IMX8MP_CLK_ENET_REF] = imx8m_clk_composite("enet_ref", imx8mp_enet_ref_sels, ccm + 0xa980);
	clks[IMX8MP_CLK_ENET_TIMER] = imx8m_clk_composite("enet_timer", imx8mp_enet_timer_sels, ccm + 0xaa00);
	clks[IMX8MP_CLK_ENET_PHY_REF] = imx8m_clk_composite("enet_phy_ref", imx8mp_enet_phy_ref_sels, ccm + 0xaa80);
	clks[IMX8MP_CLK_NAND] = imx8m_clk_composite("nand", imx8mp_nand_sels, ccm + 0xab00);
	clks[IMX8MP_CLK_QSPI] = imx8m_clk_composite("qspi", imx8mp_qspi_sels, ccm + 0xab80);
	clks[IMX8MP_CLK_USDHC1] = imx8m_clk_composite("usdhc1", imx8mp_usdhc1_sels, ccm + 0xac00);
	clks[IMX8MP_CLK_USDHC2] = imx8m_clk_composite("usdhc2", imx8mp_usdhc2_sels, ccm + 0xac80);
	clks[IMX8MP_CLK_I2C1] = imx8m_clk_composite("i2c1", imx8mp_i2c1_sels, ccm + 0xad00);
	clks[IMX8MP_CLK_I2C2] = imx8m_clk_composite("i2c2", imx8mp_i2c2_sels, ccm + 0xad80);
	clks[IMX8MP_CLK_I2C3] = imx8m_clk_composite("i2c3", imx8mp_i2c3_sels, ccm + 0xae00);
	clks[IMX8MP_CLK_I2C4] = imx8m_clk_composite("i2c4", imx8mp_i2c4_sels, ccm + 0xae80);

	clks[IMX8MP_CLK_UART1] = imx8m_clk_composite("uart1", imx8mp_uart1_sels, ccm + 0xaf00);
	clks[IMX8MP_CLK_UART2] = imx8m_clk_composite("uart2", imx8mp_uart2_sels, ccm + 0xaf80);
	clks[IMX8MP_CLK_UART3] = imx8m_clk_composite("uart3", imx8mp_uart3_sels, ccm + 0xb000);
	clks[IMX8MP_CLK_UART4] = imx8m_clk_composite("uart4", imx8mp_uart4_sels, ccm + 0xb080);
	clks[IMX8MP_CLK_USB_CORE_REF] = imx8m_clk_composite("usb_core_ref", imx8mp_usb_core_ref_sels, ccm + 0xb100);
	clks[IMX8MP_CLK_USB_PHY_REF] = imx8m_clk_composite("usb_phy_ref", imx8mp_usb_phy_ref_sels, ccm + 0xb180);
	clks[IMX8MP_CLK_GIC] = imx8m_clk_composite_critical("gic", imx8mp_gic_sels, ccm + 0xb200);
	clks[IMX8MP_CLK_ECSPI1] = imx8m_clk_composite("ecspi1", imx8mp_ecspi1_sels, ccm + 0xb280);
	clks[IMX8MP_CLK_ECSPI2] = imx8m_clk_composite("ecspi2", imx8mp_ecspi2_sels, ccm + 0xb300);
	clks[IMX8MP_CLK_PWM1] = imx8m_clk_composite("pwm1", imx8mp_pwm1_sels, ccm + 0xb380);
	clks[IMX8MP_CLK_PWM2] = imx8m_clk_composite("pwm2", imx8mp_pwm2_sels, ccm + 0xb400);
	clks[IMX8MP_CLK_PWM3] = imx8m_clk_composite("pwm3", imx8mp_pwm3_sels, ccm + 0xb480);
	clks[IMX8MP_CLK_PWM4] = imx8m_clk_composite("pwm4", imx8mp_pwm4_sels, ccm + 0xb500);

	clks[IMX8MP_CLK_GPT1] = imx8m_clk_composite("gpt1", imx8mp_gpt1_sels, ccm + 0xb580);
	clks[IMX8MP_CLK_GPT2] = imx8m_clk_composite("gpt2", imx8mp_gpt2_sels, ccm + 0xb600);
	clks[IMX8MP_CLK_GPT3] = imx8m_clk_composite("gpt3", imx8mp_gpt3_sels, ccm + 0xb680);
	clks[IMX8MP_CLK_GPT4] = imx8m_clk_composite("gpt4", imx8mp_gpt4_sels, ccm + 0xb700);
	clks[IMX8MP_CLK_GPT5] = imx8m_clk_composite("gpt5", imx8mp_gpt5_sels, ccm + 0xb780);
	clks[IMX8MP_CLK_GPT6] = imx8m_clk_composite("gpt6", imx8mp_gpt6_sels, ccm + 0xb800);
	clks[IMX8MP_CLK_WDOG] = imx8m_clk_composite("wdog", imx8mp_wdog_sels, ccm + 0xb900);
	clks[IMX8MP_CLK_WRCLK] = imx8m_clk_composite("wrclk", imx8mp_wrclk_sels, ccm + 0xb980);
	clks[IMX8MP_CLK_IPP_DO_CLKO1] = imx8m_clk_composite("ipp_do_clko1", imx8mp_ipp_do_clko1_sels, ccm + 0xba00);
	clks[IMX8MP_CLK_IPP_DO_CLKO2] = imx8m_clk_composite("ipp_do_clko2", imx8mp_ipp_do_clko2_sels, ccm + 0xba80);
	clks[IMX8MP_CLK_HDMI_FDCC_TST] = imx8m_clk_composite("hdmi_fdcc_tst", imx8mp_hdmi_fdcc_tst_sels, ccm + 0xbb00);
	clks[IMX8MP_CLK_HDMI_24M] = imx8m_clk_composite("hdmi_24m", imx8mp_hdmi_24m_sels, ccm + 0xbb80);
	clks[IMX8MP_CLK_HDMI_REF_266M] = imx8m_clk_composite("hdmi_ref_266m", imx8mp_hdmi_ref_266m_sels, ccm + 0xbc00);
	clks[IMX8MP_CLK_USDHC3] = imx8m_clk_composite("usdhc3", imx8mp_usdhc3_sels, ccm + 0xbc80);
	clks[IMX8MP_CLK_MEDIA_CAM1_PIX] = imx8m_clk_composite("media_cam1_pix", imx8mp_media_cam1_pix_sels, ccm + 0xbd00);
	clks[IMX8MP_CLK_MEDIA_MIPI_PHY1_REF] = imx8m_clk_composite("media_mipi_phy1_ref", imx8mp_media_mipi_phy1_ref_sels, ccm + 0xbd80);
	clks[IMX8MP_CLK_MEDIA_DISP1_PIX] = imx8m_clk_composite("media_disp1_pix", imx8mp_media_disp1_pix_sels, ccm + 0xbe00);
	clks[IMX8MP_CLK_MEDIA_CAM2_PIX] = imx8m_clk_composite("media_cam2_pix", imx8mp_media_cam2_pix_sels, ccm + 0xbe80);
	clks[IMX8MP_CLK_MEDIA_MIPI_PHY2_REF] = imx8m_clk_composite("media_mipi_phy2_ref", imx8mp_media_mipi_phy2_ref_sels, ccm + 0xbf00);
	clks[IMX8MP_CLK_MEDIA_MIPI_CSI2_ESC] = imx8m_clk_composite("media_mipi_csi2_esc", imx8mp_media_mipi_csi2_esc_sels, ccm + 0xbf80);
	clks[IMX8MP_CLK_PCIE2_CTRL] = imx8m_clk_composite("pcie2_ctrl", imx8mp_pcie2_ctrl_sels, ccm + 0xc000);
	clks[IMX8MP_CLK_PCIE2_PHY] = imx8m_clk_composite("pcie2_phy", imx8mp_pcie2_phy_sels, ccm + 0xc080);
	clks[IMX8MP_CLK_MEDIA_MIPI_TEST_BYTE] = imx8m_clk_composite("media_mipi_test_byte", imx8mp_media_mipi_test_byte_sels, ccm + 0xc100);
	clks[IMX8MP_CLK_ECSPI3] = imx8m_clk_composite("ecspi3", imx8mp_ecspi3_sels, ccm + 0xc180);
	clks[IMX8MP_CLK_PDM] = imx8m_clk_composite("pdm", imx8mp_pdm_sels, ccm + 0xc200);
	clks[IMX8MP_CLK_VPU_VC8000E] = imx8m_clk_composite("vpu_vc8000e", imx8mp_vpu_vc8000e_sels, ccm + 0xc280);
	clks[IMX8MP_CLK_SAI7] = imx8m_clk_composite("sai7", imx8mp_sai7_sels, ccm + 0xc300);

	clks[IMX8MP_CLK_DRAM_ALT_ROOT] = imx_clk_fixed_factor("dram_alt_root", "dram_alt", 1, 4);
	clks[IMX8MP_CLK_DRAM_CORE] = imx_clk_mux2_flags("dram_core_clk", ccm + 0x9800, 24, 1, imx8mp_dram_core_sels, ARRAY_SIZE(imx8mp_dram_core_sels), CLK_IS_CRITICAL);

	clks[IMX8MP_CLK_ECSPI1_ROOT] = imx_clk_gate4("ecspi1_root_clk", "ecspi1", ccm + 0x4070, 0);
	clks[IMX8MP_CLK_ECSPI2_ROOT] = imx_clk_gate4("ecspi2_root_clk", "ecspi2", ccm + 0x4080, 0);
	clks[IMX8MP_CLK_ECSPI3_ROOT] = imx_clk_gate4("ecspi3_root_clk", "ecspi3", ccm + 0x4090, 0);
	clks[IMX8MP_CLK_ENET1_ROOT] = imx_clk_gate4("enet1_root_clk", "enet_axi", ccm + 0x40a0, 0);
	clks[IMX8MP_CLK_GPIO1_ROOT] = imx_clk_gate4("gpio1_root_clk", "ipg_root", ccm + 0x40b0, 0);
	clks[IMX8MP_CLK_GPIO2_ROOT] = imx_clk_gate4("gpio2_root_clk", "ipg_root", ccm + 0x40c0, 0);
	clks[IMX8MP_CLK_GPIO3_ROOT] = imx_clk_gate4("gpio3_root_clk", "ipg_root", ccm + 0x40d0, 0);
	clks[IMX8MP_CLK_GPIO4_ROOT] = imx_clk_gate4("gpio4_root_clk", "ipg_root", ccm + 0x40e0, 0);
	clks[IMX8MP_CLK_GPIO5_ROOT] = imx_clk_gate4("gpio5_root_clk", "ipg_root", ccm + 0x40f0, 0);
	clks[IMX8MP_CLK_GPT1_ROOT] = imx_clk_gate4("gpt1_root_clk", "gpt1", ccm + 0x4100, 0);
	clks[IMX8MP_CLK_GPT2_ROOT] = imx_clk_gate4("gpt2_root_clk", "gpt2", ccm + 0x4110, 0);
	clks[IMX8MP_CLK_GPT3_ROOT] = imx_clk_gate4("gpt3_root_clk", "gpt3", ccm + 0x4120, 0);
	clks[IMX8MP_CLK_GPT4_ROOT] = imx_clk_gate4("gpt4_root_clk", "gpt4", ccm + 0x4130, 0);
	clks[IMX8MP_CLK_GPT5_ROOT] = imx_clk_gate4("gpt5_root_clk", "gpt5", ccm + 0x4140, 0);
	clks[IMX8MP_CLK_GPT6_ROOT] = imx_clk_gate4("gpt6_root_clk", "gpt6", ccm + 0x4150, 0);
	clks[IMX8MP_CLK_I2C1_ROOT] = imx_clk_gate4("i2c1_root_clk", "i2c1", ccm + 0x4170, 0);
	clks[IMX8MP_CLK_I2C2_ROOT] = imx_clk_gate4("i2c2_root_clk", "i2c2", ccm + 0x4180, 0);
	clks[IMX8MP_CLK_I2C3_ROOT] = imx_clk_gate4("i2c3_root_clk", "i2c3", ccm + 0x4190, 0);
	clks[IMX8MP_CLK_I2C4_ROOT] = imx_clk_gate4("i2c4_root_clk", "i2c4", ccm + 0x41a0, 0);
	clks[IMX8MP_CLK_OCOTP_ROOT] = imx_clk_gate4("ocotp_root_clk", "ipg_root", ccm + 0x4220, 0);
	clks[IMX8MP_CLK_PCIE_ROOT] = imx_clk_gate4("pcie_root_clk", "pcie_aux", ccm + 0x4250, 0);
	clks[IMX8MP_CLK_PWM1_ROOT] = imx_clk_gate4("pwm1_root_clk", "pwm1", ccm + 0x4280, 0);
	clks[IMX8MP_CLK_PWM2_ROOT] = imx_clk_gate4("pwm2_root_clk", "pwm2", ccm + 0x4290, 0);
	clks[IMX8MP_CLK_PWM3_ROOT] = imx_clk_gate4("pwm3_root_clk", "pwm3", ccm + 0x42a0, 0);
	clks[IMX8MP_CLK_PWM4_ROOT] = imx_clk_gate4("pwm4_root_clk", "pwm4", ccm + 0x42b0, 0);
	clks[IMX8MP_CLK_QOS_ROOT] = imx_clk_gate4("qos_root_clk", "ipg_root", ccm + 0x42c0, 0);
	clks[IMX8MP_CLK_QOS_ENET_ROOT] = imx_clk_gate4("qos_enet_root_clk", "ipg_root", ccm + 0x42e0, 0);
	clks[IMX8MP_CLK_QSPI_ROOT] = imx_clk_gate4("qspi_root_clk", "qspi", ccm + 0x42f0, 0);
	clks[IMX8MP_CLK_NAND_ROOT] = imx_clk_gate2_shared2("nand_root_clk", "nand", ccm + 0x4300, 0);
	clks[IMX8MP_CLK_NAND_USDHC_BUS_RAWNAND_CLK] = imx_clk_gate2_shared2("nand_usdhc_rawnand_clk", "nand_usdhc_bus", ccm + 0x4300, 0);
	clks[IMX8MP_CLK_I2C5_ROOT] = imx_clk_gate2("i2c5_root_clk", "i2c5", ccm + 0x4330, 0);
	clks[IMX8MP_CLK_I2C6_ROOT] = imx_clk_gate2("i2c6_root_clk", "i2c6", ccm + 0x4340, 0);
	clks[IMX8MP_CLK_CAN1_ROOT] = imx_clk_gate2("can1_root_clk", "can1", ccm + 0x4350, 0);
	clks[IMX8MP_CLK_CAN2_ROOT] = imx_clk_gate2("can2_root_clk", "can2", ccm + 0x4360, 0);
	clks[IMX8MP_CLK_SDMA1_ROOT] = imx_clk_gate4("sdma1_root_clk", "ipg_root", ccm + 0x43a0, 0);
	clks[IMX8MP_CLK_ENET_QOS_ROOT] = imx_clk_gate4("enet_qos_root_clk", "sim_enet_root_clk", ccm + 0x43b0, 0);
	clks[IMX8MP_CLK_SIM_ENET_ROOT] = imx_clk_gate4("sim_enet_root_clk", "enet_axi", ccm + 0x4400, 0);
	clks[IMX8MP_CLK_GPU2D_ROOT] = imx_clk_gate4("gpu2d_root_clk", "gpu2d_div", ccm + 0x4450, 0);
	clks[IMX8MP_CLK_GPU3D_ROOT] = imx_clk_gate4("gpu3d_root_clk", "gpu3d_core_div", ccm + 0x4460, 0);
	clks[IMX8MP_CLK_SNVS_ROOT] = imx_clk_gate4("snvs_root_clk", "ipg_root", ccm + 0x4470, 0);
	clks[IMX8MP_CLK_UART1_ROOT] = imx_clk_gate4("uart1_root_clk", "uart1", ccm + 0x4490, 0);
	clks[IMX8MP_CLK_UART2_ROOT] = imx_clk_gate4("uart2_root_clk", "uart2", ccm + 0x44a0, 0);
	clks[IMX8MP_CLK_UART3_ROOT] = imx_clk_gate4("uart3_root_clk", "uart3", ccm + 0x44b0, 0);
	clks[IMX8MP_CLK_UART4_ROOT] = imx_clk_gate4("uart4_root_clk", "uart4", ccm + 0x44c0, 0);
	clks[IMX8MP_CLK_USB_ROOT] = imx_clk_gate4("usb_root_clk", "osc_32k", ccm + 0x44d0, 0);
	clks[IMX8MP_CLK_USB_PHY_ROOT] = imx_clk_gate4("usb_phy_root_clk", "usb_phy_ref", ccm + 0x44f0, 0);
	clks[IMX8MP_CLK_USDHC1_ROOT] = imx_clk_gate4("usdhc1_root_clk", "usdhc1", ccm + 0x4510, 0);
	clks[IMX8MP_CLK_USDHC2_ROOT] = imx_clk_gate4("usdhc2_root_clk", "usdhc2", ccm + 0x4520, 0);
	clks[IMX8MP_CLK_WDOG1_ROOT] = imx_clk_gate4("wdog1_root_clk", "wdog", ccm + 0x4530, 0);
	clks[IMX8MP_CLK_WDOG2_ROOT] = imx_clk_gate4("wdog2_root_clk", "wdog", ccm + 0x4540, 0);
	clks[IMX8MP_CLK_WDOG3_ROOT] = imx_clk_gate4("wdog3_root_clk", "wdog", ccm + 0x4550, 0);
	clks[IMX8MP_CLK_VPU_G1_ROOT] = imx_clk_gate4("vpu_g1_root_clk", "vpu_g1", ccm + 0x4560, 0);
	clks[IMX8MP_CLK_GPU_ROOT] = imx_clk_gate4("gpu_root_clk", "gpu_axi", ccm + 0x4570, 0);
	clks[IMX8MP_CLK_VPU_VC8KE_ROOT] = imx_clk_gate4("vpu_vc8ke_root_clk", "vpu_vc8000e", ccm + 0x4590, 0);
	clks[IMX8MP_CLK_VPU_G2_ROOT] = imx_clk_gate4("vpu_g2_root_clk", "vpu_g2", ccm + 0x45a0, 0);
	clks[IMX8MP_CLK_NPU_ROOT] = imx_clk_gate4("npu_root_clk", "ml_div", ccm + 0x45b0, 0);
	clks[IMX8MP_CLK_HSIO_ROOT] = imx_clk_gate4("hsio_root_clk", "ipg_root", ccm + 0x45c0, 0);
	clks[IMX8MP_CLK_MEDIA_APB_ROOT] = imx_clk_gate2_shared2("media_apb_root_clk", "media_apb", ccm + 0x45d0, 0);
	clks[IMX8MP_CLK_MEDIA_AXI_ROOT] = imx_clk_gate2_shared2("media_axi_root_clk", "media_axi", ccm + 0x45d0, 0);
	clks[IMX8MP_CLK_MEDIA_CAM1_PIX_ROOT] = imx_clk_gate2_shared2("media_cam1_pix_root_clk", "media_cam1_pix", ccm + 0x45d0, 0);
	clks[IMX8MP_CLK_MEDIA_CAM2_PIX_ROOT] = imx_clk_gate2_shared2("media_cam2_pix_root_clk", "media_cam2_pix", ccm + 0x45d0, 0);
	clks[IMX8MP_CLK_MEDIA_DISP1_PIX_ROOT] = imx_clk_gate2_shared2("media_disp1_pix_root_clk", "media_disp1_pix", ccm + 0x45d0, 0);
	clks[IMX8MP_CLK_MEDIA_DISP2_PIX_ROOT] = imx_clk_gate2_shared2("media_disp2_pix_root_clk", "media_disp2_pix", ccm + 0x45d0, 0);
	clks[IMX8MP_CLK_MEDIA_ISP_ROOT] = imx_clk_gate2_shared2("media_isp_root_clk", "media_isp_div", ccm + 0x45d0, 0);

	clks[IMX8MP_CLK_USDHC3_ROOT] = imx_clk_gate4("usdhc3_root_clk", "usdhc3", ccm + 0x45e0, 0);
	clks[IMX8MP_CLK_HDMI_ROOT] = imx_clk_gate4("hdmi_root_clk", "hdmi_axi", ccm + 0x45f0, 0);
	clks[IMX8MP_CLK_TSENSOR_ROOT] = imx_clk_gate4("tsensor_root_clk", "ipg_root", ccm + 0x4620, 0);
	clks[IMX8MP_CLK_VPU_ROOT] = imx_clk_gate4("vpu_root_clk", "vpu_bus", ccm + 0x4630, 0);
	clks[IMX8MP_CLK_AUDIO_ROOT] = imx_clk_gate4("audio_root_clk", "ipg_root", ccm + 0x4650, 0);

	clks[IMX8MP_CLK_ARM] = imx_clk_cpu("arm", "arm_a53_div",
					     clks[IMX8MP_CLK_A53_CORE],
					     clks[IMX8MP_CLK_A53_CORE],
					     clks[IMX8MP_ARM_PLL_OUT],
					     clks[IMX8MP_CLK_A53_DIV]);

	imx_check_clocks(clks, ARRAY_SIZE(clks));

	clk_enable(clks[IMX8MP_SYS_PLL3_OUT]);
	clk_data.clks = clks;
	clk_data.clk_num = ARRAY_SIZE(clks);

	ret = of_clk_add_provider(ccm_np, of_clk_src_onecell_get, &clk_data);
	if (ret < 0)
		pr_err("failed to register clks for i.MX8MM\n");

	return ret;
}
CLK_OF_DECLARE(imx8mp, "fsl,imx8mp-ccm", imx8mp_clocks_init);
