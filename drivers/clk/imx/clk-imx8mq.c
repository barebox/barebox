/*
 * Copyright 2017 NXP.
 *
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <common.h>
#include <init.h>
#include <driver.h>
#include <linux/clk.h>
#include <io.h>
#include <of.h>
#include <of_address.h>
#include <linux/clkdev.h>
#include <linux/err.h>
#include <mach/revision.h>
#include <dt-bindings/clock/imx8mq-clock.h>

#include "clk.h"

static struct clk *clks[IMX8MQ_CLK_END];

static const char *pll_ref_sels[] = { "osc_25m", "osc_27m", "dummy", "dummy", };
static const char *arm_pll_bypass_sels[] = {"arm_pll", "arm_pll_ref_sel", };
static const char *gpu_pll_bypass_sels[] = {"gpu_pll", "gpu_pll_ref_sel", };
static const char *vpu_pll_bypass_sels[] = {"vpu_pll", "vpu_pll_ref_sel", };
static const char *audio_pll1_bypass_sels[] = {"audio_pll1", "audio_pll1_ref_sel", };
static const char *audio_pll2_bypass_sels[] = {"audio_pll2", "audio_pll2_ref_sel", };
static const char *video_pll1_bypass_sels[] = {"video_pll1", "video_pll1_ref_sel", };

static const char *sys1_pll1_out_sels[] = {"sys1_pll1", "sys1_pll1_ref_sel", };
static const char *sys2_pll1_out_sels[] = {"sys2_pll1", "sys1_pll1_ref_sel", };
static const char *sys3_pll1_out_sels[] = {"sys3_pll1", "sys3_pll1_ref_sel", };
static const char *dram_pll1_out_sels[] = {"dram_pll1", "dram_pll1_ref_sel", };
static const char *video2_pll1_out_sels[] = {"video2_pll1", "video2_pll1_ref_sel", };

static const char *sys1_pll2_out_sels[] = {"sys1_pll2_div", "sys1_pll1_ref_sel", };
static const char *sys2_pll2_out_sels[] = {"sys2_pll2_div", "sys2_pll1_ref_sel", };
static const char *sys3_pll2_out_sels[] = {"sys3_pll2_div", "sys2_pll1_ref_sel", };
static const char *dram_pll2_out_sels[] = {"dram_pll2_div", "dram_pll1_ref_sel", };
static const char *video2_pll2_out_sels[] = {"video2_pll2_div", "video2_pll1_ref_sel", };

/* CCM ROOT */
static const char *imx8mq_a53_sels[] = {"osc_25m", "arm_pll_out", "sys2_pll_500m", "sys2_pll_1000m",
					"sys1_pll_800m", "sys1_pll_400m", "audio_pll1_out", "sys3_pll2_out", };

static const char *imx8mq_main_axi_sels[] = {"osc_25m", "sys2_pll_333m", "sys1_pll_800m", "sys2_pll_250m",
					     "sys2_pll_1000m", "audio_pll1_out", "video_pll1_out", "sys1_pll_100m",};

static const char *imx8mq_enet_axi_sels[] = {"osc_25m", "sys1_pll_266m", "sys1_pll_800m", "sys2_pll_250m",
					     "sys2_pll_200m", "audio_pll1_out", "video_pll1_out", "sys3_pll2_out", };

static const char *imx8mq_nand_usdhc_sels[] = {"osc_25m", "sys1_pll_266m", "sys1_pll_800m", "sys2_pll_200m",
					       "sys1_pll_133m", "sys3_pll2_out", "sys2_pll_250m", "audio_pll1_out", };

static const char *imx8mq_usb_bus_sels[] = {"osc_25m", "sys2_pll_500m", "sys1_pll_800m", "sys2_pll_100m",
					    "sys2_pll_200m", "clk_ext2", "clk_ext4", "audio_pll2_out", };

static const char *imx8mq_noc_sels[] = {"osc_25m", "sys1_pll_800m", "sys3_pll2_out", "sys2_pll_1000m", "sys2_pll_500m",
					"audio_pll1_out", "video_pll1_out", "audio_pll2_out", };

static const char *imx8mq_noc_apb_sels[] = {"osc_25m", "sys1_pll_400m", "sys3_pll2_out", "sys2_pll_333m", "sys2_pll_200m",
					    "sys1_pll_800m", "audio_pll1_out", "video_pll1_out", };

static const char *imx8mq_ahb_sels[] = {"osc_25m", "sys1_pll_133m", "sys1_pll_800m", "sys1_pll_400m",
					"sys2_pll_125m", "sys3_pll2_out", "audio_pll1_out", "video_pll1_out", };

static const char *imx8mq_dram_alt_sels[] = {"osc_25m", "sys1_pll_800m", "sys1_pll_100m", "sys2_pll_500m",
					     "sys2_pll_250m", "sys1_pll_400m", "audio_pll1_out", "sys1_pll_266m", };

static const char *imx8mq_dram_apb_sels[] = {"osc_25m", "sys2_pll_200m", "sys1_pll_40m", "sys1_pll_160m",
					     "sys1_pll_800m", "sys3_pll2_out", "sys2_pll_250m", "audio_pll2_out", };

static const char *imx8mq_pcie1_ctrl_sels[] = {"osc_25m", "sys2_pll_250m", "sys2_pll_200m", "sys1_pll_266m",
					       "sys1_pll_800m", "sys2_pll_500m", "sys2_pll_250m", "sys3_pll2_out", };

static const char *imx8mq_pcie1_phy_sels[] = {"osc_25m", "sys2_pll_100m", "sys2_pll_500m", "clk_ext1", "clk_ext2",
					      "clk_ext3", "clk_ext4", };

static const char *imx8mq_pcie1_aux_sels[] = {"osc_25m", "sys2_pll_200m", "sys2_pll_500m", "sys3_pll2_out",
					      "sys2_pll_100m", "sys1_pll_80m", "sys1_pll_160m", "sys1_pll_200m", };

static const char *imx8mq_dc_pixel_sels[] = {"osc_25m", "video_pll1_out", "audio_pll2_out", "audio_pll1_out", "sys1_pll_800m", "sys2_pll_1000m", "sys3_pll2_out", "clk_ext4", };

static const char *imx8mq_lcdif_pixel_sels[] = {"osc_25m", "video_pll1_out", "audio_pll2_out", "audio_pll1_out", "sys1_pll_800m", "sys2_pll_1000m", "sys3_pll2_out", "clk_ext4", };

static const char *imx8mq_enet_ref_sels[] = {"osc_25m", "sys2_pll_125m", "sys2_pll_500m", "sys2_pll_100m",
					     "sys1_pll_160m", "audio_pll1_out", "video_pll1_out", "clk_ext4", };

static const char *imx8mq_enet_timer_sels[] = {"osc_25m", "sys2_pll_100m", "audio_pll1_out", "clk_ext1", "clk_ext2",
					       "clk_ext3", "clk_ext4", "video_pll1_out", };

static const char *imx8mq_enet_phy_sels[] = {"osc_25m", "sys2_pll_50m", "sys2_pll_125m", "sys2_pll_500m",
					     "audio_pll1_out", "video_pll1_out", "audio_pll2_out", };

static const char *imx8mq_nand_sels[] = {"osc_25m", "sys2_pll_500m", "audio_pll1_out", "sys1_pll_400m",
					 "audio_pll2_out", "sys3_pll2_out", "sys2_pll_250m", "video_pll1_out", };

static const char *imx8mq_qspi_sels[] = {"osc_25m", "sys1_pll_400m", "sys1_pll_800m", "sys2_pll_500m",
					 "audio_pll2_out", "sys1_pll_266m", "sys3_pll2_out", "sys1_pll_100m", };

static const char *imx8mq_usdhc1_sels[] = {"osc_25m", "sys1_pll_400m", "sys1_pll_800m", "sys2_pll_500m",
					 "audio_pll2_out", "sys1_pll_266m", "sys3_pll2_out", "sys1_pll_100m", };

static const char *imx8mq_usdhc2_sels[] = {"osc_25m", "sys1_pll_400m", "sys1_pll_800m", "sys2_pll_500m",
					 "audio_pll2_out", "sys1_pll_266m", "sys3_pll2_out", "sys1_pll_100m", };

static const char *imx8mq_i2c1_sels[] = {"osc_25m", "sys1_pll_160m", "sys2_pll_50m", "sys3_pll2_out", "audio_pll1_out",
					 "video_pll1_out", "audio_pll2_out", "sys1_pll_133m", };

static const char *imx8mq_i2c2_sels[] = {"osc_25m", "sys1_pll_160m", "sys2_pll_50m", "sys3_pll2_out", "audio_pll1_out",
					 "video_pll1_out", "audio_pll2_out", "sys1_pll_133m", };

static const char *imx8mq_i2c3_sels[] = {"osc_25m", "sys1_pll_160m", "sys2_pll_50m", "sys3_pll2_out", "audio_pll1_out",
					 "video_pll1_out", "audio_pll2_out", "sys1_pll_133m", };

static const char *imx8mq_i2c4_sels[] = {"osc_25m", "sys1_pll_160m", "sys2_pll_50m", "sys3_pll2_out", "audio_pll1_out",
					 "video_pll1_out", "audio_pll2_out", "sys1_pll_133m", };

static const char *imx8mq_uart1_sels[] = {"osc_25m", "sys1_pll_80m", "sys2_pll_200m", "sys2_pll_100m",
					  "sys3_pll2_out", "clk_ext2", "clk_ext4", "audio_pll2_out", };

static const char *imx8mq_uart2_sels[] = {"osc_25m", "sys1_pll_80m", "sys2_pll_200m", "sys2_pll_100m",
					  "sys3_pll2_out", "clk_ext2", "clk_ext3", "audio_pll2_out", };

static const char *imx8mq_uart3_sels[] = {"osc_25m", "sys1_pll_80m", "sys2_pll_200m", "sys2_pll_100m",
					  "sys3_pll2_out", "clk_ext2", "clk_ext4", "audio_pll2_out", };

static const char *imx8mq_uart4_sels[] = {"osc_25m", "sys1_pll_80m", "sys2_pll_200m", "sys2_pll_100m",
					  "sys3_pll2_out", "clk_ext2", "clk_ext3", "audio_pll2_out", };

static const char *imx8mq_usb_core_sels[] = {"osc_25m", "sys1_pll_100m", "sys1_pll_40m", "sys2_pll_100m",
					     "sys2_pll_200m", "clk_ext2", "clk_ext3", "audio_pll2_out", };

static const char *imx8mq_usb_phy_sels[] = {"osc_25m", "sys1_pll_100m", "sys1_pll_40m", "sys2_pll_100m",
					     "sys2_pll_200m", "clk_ext2", "clk_ext3", "audio_pll2_out", };

static const char *imx8mq_ecspi1_sels[] = {"osc_25m", "sys2_pll_200m", "sys1_pll_40m", "sys1_pll_160m",
					   "sys1_pll_800m", "sys3_pll2_out", "sys2_pll_250m", "audio_pll2_out", };

static const char *imx8mq_ecspi2_sels[] = {"osc_25m", "sys2_pll_200m", "sys1_pll_40m", "sys1_pll_160m",
					   "sys1_pll_800m", "sys3_pll2_out", "sys2_pll_250m", "audio_pll2_out", };

static const char *imx8mq_pwm1_sels[] = {"osc_25m", "sys2_pll_100m", "sys1_pll_160m", "sys1_pll_40m",
					 "sys3_pll2_out", "clk_ext1", "sys1_pll_80m", "video_pll1_out", };

static const char *imx8mq_pwm2_sels[] = {"osc_25m", "sys2_pll_100m", "sys1_pll_160m", "sys1_pll_40m",
					 "sys3_pll2_out", "clk_ext1", "sys1_pll_80m", "video_pll1_out", };

static const char *imx8mq_pwm3_sels[] = {"osc_25m", "sys2_pll_100m", "sys1_pll_160m", "sys1_pll_40m",
					 "sys3_pll2_out", "clk_ext2", "sys1_pll_80m", "video_pll1_out", };

static const char *imx8mq_pwm4_sels[] = {"osc_25m", "sys2_pll_100m", "sys1_pll_160m", "sys1_pll_40m",
					 "sys3_pll2_out", "clk_ext2", "sys1_pll_80m", "video_pll1_out", };

static const char *imx8mq_gpt1_sels[] = {"osc_25m", "sys2_pll_100m", "sys1_pll_400m", "sys1_pll_40m",
					 "sys1_pll_80m", "audio_pll1_out", "clk_ext1", };

static const char *imx8mq_wdog_sels[] = {"osc_25m", "sys1_pll_133m", "sys1_pll_160m", "vpu_pll_out",
					 "sys2_pll_125m", "sys3_pll2_out", "sys1_pll_80m", "sys2_pll_166m", };

static const char *imx8mq_wrclk_sels[] = {"osc_25m", "sys1_pll_40m", "vpu_pll_out", "sys3_pll2_out", "sys2_pll_200m",
					  "sys1_pll_266m", "sys2_pll_500m", "sys1_pll_100m", };

static const char *imx8mq_pcie2_ctrl_sels[] = {"osc_25m", "sys2_pll_250m", "sys2_pll_200m", "sys1_pll_266m",
					       "sys1_pll_800m", "sys2_pll_500m", "sys2_pll_333m", "sys3_pll2_out", };

static const char *imx8mq_pcie2_phy_sels[] = {"osc_25m", "sys2_pll_100m", "sys2_pll_500m", "clk_ext1",
					      "clk_ext2", "clk_ext3", "clk_ext4", "sys1_pll_400m", };

static const char *imx8mq_pcie2_aux_sels[] = {"osc_25m", "sys2_pll_200m", "sys2_pll_50m", "sys3_pll2_out",
					      "sys2_pll_100m", "sys1_pll_80m", "sys1_pll_160m", "sys1_pll_200m", };

static const char *imx8mq_ecspi3_sels[] = {"osc_25m", "sys2_pll_200m", "sys1_pll_40m", "sys1_pll_160m",
					   "sys1_pll_800m", "sys3_pll2_out", "sys2_pll_250m", "audio_pll2_out", };
static const char *imx8mq_dram_core_sels[] = {"dram_pll_out", "dram_alt_root", };

static const char *imx8mq_clko2_sels[] = {"osc_25m", "sys2_pll_200m", "sys1_pll_400m", "sys2_pll_166m", "audio_pll1_out",
					 "video_pll1_out", "ckil", };

static struct clk_onecell_data clk_data;

static void __init imx8mq_clocks_init(struct device_node *ccm_node)
{
	struct device_node *np;
	void __iomem *base;
	int i;

	clks[IMX8MQ_CLK_DUMMY] = clk_fixed("dummy", 0);
	clks[IMX8MQ_CLK_32K] = of_clk_get_by_name(ccm_node, "ckil");
	clks[IMX8MQ_CLK_25M] = of_clk_get_by_name(ccm_node, "osc_25m");
	clks[IMX8MQ_CLK_27M] = of_clk_get_by_name(ccm_node, "osc_27m");
	clks[IMX8MQ_CLK_EXT1] = of_clk_get_by_name(ccm_node, "clk_ext1");
	clks[IMX8MQ_CLK_EXT2] = of_clk_get_by_name(ccm_node, "clk_ext2");
	clks[IMX8MQ_CLK_EXT3] = of_clk_get_by_name(ccm_node, "clk_ext3");
	clks[IMX8MQ_CLK_EXT4] = of_clk_get_by_name(ccm_node, "clk_ext4");

	np = of_find_compatible_node(NULL, NULL, "fsl,imx8mq-anatop");
	base = of_iomap(np, 0);
	WARN_ON(!base);

	clks[IMX8MQ_ARM_PLL_REF_SEL] = imx_clk_mux("arm_pll_ref_sel", base + 0x28, 16, 2, pll_ref_sels, ARRAY_SIZE(pll_ref_sels));
	clks[IMX8MQ_GPU_PLL_REF_SEL] = imx_clk_mux("gpu_pll_ref_sel", base + 0x18, 16, 2, pll_ref_sels, ARRAY_SIZE(pll_ref_sels));
	clks[IMX8MQ_VPU_PLL_REF_SEL] = imx_clk_mux("vpu_pll_ref_sel", base + 0x20, 16, 2, pll_ref_sels, ARRAY_SIZE(pll_ref_sels));
	clks[IMX8MQ_AUDIO_PLL1_REF_SEL] = imx_clk_mux("audio_pll1_ref_sel", base + 0x0, 16, 2, pll_ref_sels, ARRAY_SIZE(pll_ref_sels));
	clks[IMX8MQ_AUDIO_PLL2_REF_SEL] = imx_clk_mux("audio_pll2_ref_sel", base + 0x8, 16, 2, pll_ref_sels, ARRAY_SIZE(pll_ref_sels));
	clks[IMX8MQ_VIDEO_PLL1_REF_SEL] = imx_clk_mux("video_pll1_ref_sel", base + 0x10, 16, 2, pll_ref_sels, ARRAY_SIZE(pll_ref_sels));
	clks[IMX8MQ_SYS1_PLL1_REF_SEL]	= imx_clk_mux("sys1_pll1_ref_sel", base + 0x30, 0, 2, pll_ref_sels, ARRAY_SIZE(pll_ref_sels));
	clks[IMX8MQ_SYS2_PLL1_REF_SEL]	= imx_clk_mux("sys2_pll1_ref_sel", base + 0x3c, 0, 2, pll_ref_sels, ARRAY_SIZE(pll_ref_sels));
	clks[IMX8MQ_SYS3_PLL1_REF_SEL]	= imx_clk_mux("sys3_pll1_ref_sel", base + 0x48, 0, 2, pll_ref_sels, ARRAY_SIZE(pll_ref_sels));
	clks[IMX8MQ_DRAM_PLL1_REF_SEL]	= imx_clk_mux("dram_pll1_ref_sel", base + 0x60, 0, 2, pll_ref_sels, ARRAY_SIZE(pll_ref_sels));
	clks[IMX8MQ_VIDEO2_PLL1_REF_SEL] = imx_clk_mux("video2_pll1_ref_sel", base + 0x54, 0, 2, pll_ref_sels, ARRAY_SIZE(pll_ref_sels));

	clks[IMX8MQ_ARM_PLL_REF_DIV]	= imx_clk_divider("arm_pll_ref_div", "arm_pll_ref_sel", base + 0x28, 5, 6);
	clks[IMX8MQ_GPU_PLL_REF_DIV]	= imx_clk_divider("gpu_pll_ref_div", "gpu_pll_ref_sel", base + 0x18, 5, 6);
	clks[IMX8MQ_VPU_PLL_REF_DIV]	= imx_clk_divider("vpu_pll_ref_div", "vpu_pll_ref_sel", base + 0x20, 5, 6);
	clks[IMX8MQ_AUDIO_PLL1_REF_DIV] = imx_clk_divider("audio_pll1_ref_div", "audio_pll1_ref_sel", base + 0x0, 5, 6);
	clks[IMX8MQ_AUDIO_PLL2_REF_DIV] = imx_clk_divider("audio_pll2_ref_div", "audio_pll2_ref_sel", base + 0x8, 5, 6);
	clks[IMX8MQ_VIDEO_PLL1_REF_DIV] = imx_clk_divider("video_pll1_ref_div", "video_pll1_ref_sel", base + 0x10, 5, 6);
	clks[IMX8MQ_SYS1_PLL1_REF_DIV]	= imx_clk_divider("sys1_pll1_ref_div", "sys1_pll1_ref_sel", base + 0x38, 25, 3);
	clks[IMX8MQ_SYS2_PLL1_REF_DIV]	= imx_clk_divider("sys2_pll1_ref_div", "sys2_pll1_ref_sel", base + 0x44, 25, 3);
	clks[IMX8MQ_SYS3_PLL1_REF_DIV]	= imx_clk_divider("sys3_pll1_ref_div", "sys3_pll1_ref_sel", base + 0x50, 25, 3);
	clks[IMX8MQ_DRAM_PLL1_REF_DIV]	= imx_clk_divider("dram_pll1_ref_div", "dram_pll1_ref_sel", base + 0x68, 25, 3);
	clks[IMX8MQ_VIDEO2_PLL1_REF_DIV]  = imx_clk_divider("video2_pll1_ref_div", "video2_pll1_ref_sel", base + 0x5c, 25, 3);

	clks[IMX8MQ_ARM_PLL] = imx_clk_frac_pll("arm_pll", "arm_pll_ref_div", base + 0x28);
	clks[IMX8MQ_GPU_PLL] = imx_clk_frac_pll("gpu_pll", "gpu_pll_ref_div", base + 0x18);
	clks[IMX8MQ_VPU_PLL] = imx_clk_frac_pll("vpu_pll", "vpu_pll_ref_div", base + 0x20);
	clks[IMX8MQ_AUDIO_PLL1] = imx_clk_frac_pll("audio_pll1", "audio_pll1_ref_div", base + 0x0);
	clks[IMX8MQ_AUDIO_PLL2] = imx_clk_frac_pll("audio_pll2", "audio_pll2_ref_div", base + 0x8);
	clks[IMX8MQ_VIDEO_PLL1] = imx_clk_frac_pll("video_pll1", "video_pll1_ref_div", base + 0x10);
	clks[IMX8MQ_SYS1_PLL1] = imx_clk_sccg_pll("sys1_pll1", "sys1_pll1_ref_div", base + 0x30, SCCG_PLL1);
	clks[IMX8MQ_SYS2_PLL1] = imx_clk_sccg_pll("sys2_pll1", "sys2_pll1_ref_div", base + 0x3c, SCCG_PLL1);
	clks[IMX8MQ_SYS3_PLL1] = imx_clk_sccg_pll("sys3_pll1", "sys3_pll1_ref_div", base + 0x48, SCCG_PLL1);
	clks[IMX8MQ_DRAM_PLL1] = imx_clk_sccg_pll("dram_pll1", "dram_pll1_ref_div", base + 0x60, SCCG_PLL1);
	clks[IMX8MQ_VIDEO2_PLL1] = imx_clk_sccg_pll("video2_pll1", "video2_pll1_ref_div", base + 0x5c, SCCG_PLL1);

	clks[IMX8MQ_SYS1_PLL2] = imx_clk_sccg_pll("sys1_pll2", "sys1_pll1_out_div", base + 0x30, SCCG_PLL2);
	clks[IMX8MQ_SYS2_PLL2] = imx_clk_sccg_pll("sys2_pll2", "sys2_pll1_out_div", base + 0x3c, SCCG_PLL2);
	clks[IMX8MQ_SYS3_PLL2] = imx_clk_sccg_pll("sys3_pll2", "sys3_pll1_out_div", base + 0x48, SCCG_PLL2);
	clks[IMX8MQ_DRAM_PLL2] = imx_clk_sccg_pll("dram_pll2", "dram_pll1_out_div", base + 0x60, SCCG_PLL2);
	clks[IMX8MQ_VIDEO2_PLL2] = imx_clk_sccg_pll("video2_pll2", "video2_pll1_out_div", base + 0x54, SCCG_PLL2);

	/* PLL divs */
	clks[IMX8MQ_SYS1_PLL1_OUT_DIV] = imx_clk_divider("sys1_pll1_out_div", "sys1_pll1_out", base + 0x38, 19, 6);
	clks[IMX8MQ_SYS2_PLL1_OUT_DIV] = imx_clk_divider("sys2_pll1_out_div", "sys2_pll1_out", base + 0x44, 19, 6);
	clks[IMX8MQ_SYS3_PLL1_OUT_DIV] = imx_clk_divider("sys3_pll1_out_div", "sys3_pll1_out", base + 0x50, 19, 6);
	clks[IMX8MQ_DRAM_PLL1_OUT_DIV] = imx_clk_divider("dram_pll1_out_div", "dram_pll1_out", base + 0x68, 19, 6);
	clks[IMX8MQ_VIDEO2_PLL1_OUT_DIV] = imx_clk_divider("video2_pll1_out_div", "video2_pll1_out", base + 0x5c, 19, 6);
	clks[IMX8MQ_SYS1_PLL2_DIV] = imx_clk_divider("sys1_pll2_div", "sys1_pll2", base + 0x38, 1, 6);
	clks[IMX8MQ_SYS2_PLL2_DIV] = imx_clk_divider("sys2_pll2_div", "sys2_pll2", base + 0x44, 1, 6);
	clks[IMX8MQ_SYS3_PLL2_DIV] = imx_clk_divider("sys3_pll2_div", "sys3_pll2", base + 0x50, 1, 6);
	clks[IMX8MQ_DRAM_PLL2_DIV] = imx_clk_divider("dram_pll2_div", "dram_pll2", base + 0x68, 1, 6);
	clks[IMX8MQ_VIDEO2_PLL2_DIV] = imx_clk_divider("video2_pll2_div", "video2_pll2", base + 0x5c, 1, 6);

	/* PLL bypass out */
	clks[IMX8MQ_ARM_PLL_BYPASS] = imx_clk_mux("arm_pll_bypass", base + 0x28, 14, 1, arm_pll_bypass_sels, ARRAY_SIZE(arm_pll_bypass_sels));
	clks[IMX8MQ_GPU_PLL_BYPASS] = imx_clk_mux("gpu_pll_bypass", base + 0x18, 14, 1, gpu_pll_bypass_sels, ARRAY_SIZE(gpu_pll_bypass_sels));
	clks[IMX8MQ_VPU_PLL_BYPASS] = imx_clk_mux("vpu_pll_bypass", base + 0x20, 14, 1, vpu_pll_bypass_sels, ARRAY_SIZE(vpu_pll_bypass_sels));
	clks[IMX8MQ_AUDIO_PLL1_BYPASS] = imx_clk_mux("audio_pll1_bypass", base + 0x0, 14, 1, audio_pll1_bypass_sels, ARRAY_SIZE(audio_pll1_bypass_sels));
	clks[IMX8MQ_AUDIO_PLL2_BYPASS] = imx_clk_mux("audio_pll2_bypass", base + 0x8, 14, 1, audio_pll2_bypass_sels, ARRAY_SIZE(audio_pll2_bypass_sels));
	clks[IMX8MQ_VIDEO_PLL1_BYPASS] = imx_clk_mux("video_pll1_bypass", base + 0x10, 14, 1, video_pll1_bypass_sels, ARRAY_SIZE(video_pll1_bypass_sels));

	clks[IMX8MQ_SYS1_PLL1_OUT] = imx_clk_mux("sys1_pll1_out", base + 0x30, 5, 1, sys1_pll1_out_sels, ARRAY_SIZE(sys1_pll1_out_sels));
	clks[IMX8MQ_SYS2_PLL1_OUT] = imx_clk_mux("sys2_pll1_out", base + 0x3c, 5, 1, sys2_pll1_out_sels, ARRAY_SIZE(sys2_pll1_out_sels));
	clks[IMX8MQ_SYS3_PLL1_OUT] = imx_clk_mux("sys3_pll1_out", base + 0x48, 5, 1, sys3_pll1_out_sels, ARRAY_SIZE(sys3_pll1_out_sels));
	clks[IMX8MQ_DRAM_PLL1_OUT] = imx_clk_mux("dram_pll1_out", base + 0x60, 5, 1, dram_pll1_out_sels, ARRAY_SIZE(dram_pll1_out_sels));
	clks[IMX8MQ_VIDEO2_PLL1_OUT] = imx_clk_mux("video2_pll1_out", base + 0x54, 5, 1, video2_pll1_out_sels, ARRAY_SIZE(video2_pll1_out_sels));
	clks[IMX8MQ_SYS1_PLL2_OUT] = imx_clk_mux("sys1_pll2_out", base + 0x30, 4, 1, sys1_pll2_out_sels, ARRAY_SIZE(sys1_pll2_out_sels));
	clks[IMX8MQ_SYS2_PLL2_OUT] = imx_clk_mux("sys2_pll2_out", base + 0x3c, 4, 1, sys2_pll2_out_sels, ARRAY_SIZE(sys2_pll2_out_sels));
	clks[IMX8MQ_SYS3_PLL2_OUT] = imx_clk_mux("sys3_pll2_out", base + 0x48, 4, 1, sys3_pll2_out_sels, ARRAY_SIZE(sys3_pll2_out_sels));
	clks[IMX8MQ_DRAM_PLL2_OUT] = imx_clk_mux("dram_pll2_out", base + 0x60, 4, 1, dram_pll2_out_sels, ARRAY_SIZE(dram_pll2_out_sels));
	clks[IMX8MQ_VIDEO2_PLL2_OUT] = imx_clk_mux("video2_pll2_out", base + 0x54, 4, 1, video2_pll2_out_sels, ARRAY_SIZE(video2_pll2_out_sels));

	/* unbypass all the plls */
	clk_set_parent(clks[IMX8MQ_GPU_PLL_BYPASS], clks[IMX8MQ_GPU_PLL]);
	clk_set_parent(clks[IMX8MQ_VPU_PLL_BYPASS], clks[IMX8MQ_VPU_PLL]);
	clk_set_parent(clks[IMX8MQ_AUDIO_PLL1_BYPASS], clks[IMX8MQ_AUDIO_PLL1]);
	clk_set_parent(clks[IMX8MQ_AUDIO_PLL2_BYPASS], clks[IMX8MQ_AUDIO_PLL2]);
	clk_set_parent(clks[IMX8MQ_VIDEO_PLL1_BYPASS], clks[IMX8MQ_VIDEO_PLL1]);
	clk_set_parent(clks[IMX8MQ_SYS3_PLL1_OUT], clks[IMX8MQ_SYS3_PLL1]);
	clk_set_parent(clks[IMX8MQ_SYS3_PLL2_OUT], clks[IMX8MQ_SYS3_PLL2_DIV]);

	/* PLL OUT GATE */
	clks[IMX8MQ_ARM_PLL_OUT] = imx_clk_gate("arm_pll_out", "arm_pll_bypass", base + 0x28, 21);
	clks[IMX8MQ_GPU_PLL_OUT] = imx_clk_gate("gpu_pll_out", "gpu_pll_bypass", base + 0x18, 21);
	clks[IMX8MQ_VPU_PLL_OUT] = imx_clk_gate("vpu_pll_out", "vpu_pll_bypass", base + 0x20, 21);
	clks[IMX8MQ_AUDIO_PLL1_OUT] = imx_clk_gate("audio_pll1_out", "audio_pll1_bypass", base + 0x0, 21);
	clks[IMX8MQ_AUDIO_PLL2_OUT] = imx_clk_gate("audio_pll2_out", "audio_pll2_bypass", base + 0x8, 21);
	clks[IMX8MQ_VIDEO_PLL1_OUT] = imx_clk_gate("video_pll1_out", "video_pll1_bypass", base + 0x10, 21);
	clks[IMX8MQ_SYS1_PLL_OUT] = imx_clk_gate("sys1_pll_out", "sys1_pll2_out", base + 0x30, 9);
	clks[IMX8MQ_SYS2_PLL_OUT] = imx_clk_gate("sys2_pll_out", "sys2_pll2_out", base + 0x3c, 9);
	clks[IMX8MQ_SYS3_PLL_OUT] = imx_clk_gate("sys3_pll_out", "sys3_pll2_out", base + 0x48, 9);
	clks[IMX8MQ_DRAM_PLL_OUT] = imx_clk_gate("dram_pll_out", "dram_pll2_out", base + 0x60, 9);
	clks[IMX8MQ_VIDEO2_PLL_OUT] = imx_clk_gate("video2_pll_out", "video2_pll2_out", base + 0x54, 9);

	/* SYS PLL fixed output */
	clks[IMX8MQ_SYS1_PLL_40M] = imx_clk_fixed_factor("sys1_pll_40m", "sys1_pll_out", 1, 20);
	clks[IMX8MQ_SYS1_PLL_80M] = imx_clk_fixed_factor("sys1_pll_80m", "sys1_pll_out", 1, 10);
	clks[IMX8MQ_SYS1_PLL_100M] = imx_clk_fixed_factor("sys1_pll_100m", "sys1_pll_out", 1, 8);
	clks[IMX8MQ_SYS1_PLL_133M] = imx_clk_fixed_factor("sys1_pll_133m", "sys1_pll_out", 1, 6);
	clks[IMX8MQ_SYS1_PLL_160M] = imx_clk_fixed_factor("sys1_pll_160m", "sys1_pll_out", 1, 5);
	clks[IMX8MQ_SYS1_PLL_200M] = imx_clk_fixed_factor("sys1_pll_200m", "sys1_pll_out", 1, 4);
	clks[IMX8MQ_SYS1_PLL_266M] = imx_clk_fixed_factor("sys1_pll_266m", "sys1_pll_out", 1, 3);
	clks[IMX8MQ_SYS1_PLL_400M] = imx_clk_fixed_factor("sys1_pll_400m", "sys1_pll_out", 1, 2);
	clks[IMX8MQ_SYS1_PLL_800M] = imx_clk_fixed_factor("sys1_pll_800m", "sys1_pll_out", 1, 1);

	clks[IMX8MQ_SYS2_PLL_50M] = imx_clk_fixed_factor("sys2_pll_50m", "sys2_pll_out", 1, 20);
	clks[IMX8MQ_SYS2_PLL_100M] = imx_clk_fixed_factor("sys2_pll_100m", "sys2_pll_out", 1, 10);
	clks[IMX8MQ_SYS2_PLL_125M] = imx_clk_fixed_factor("sys2_pll_125m", "sys2_pll_out", 1, 8);
	clks[IMX8MQ_SYS2_PLL_166M] = imx_clk_fixed_factor("sys2_pll_166m", "sys2_pll_out", 1, 6);
	clks[IMX8MQ_SYS2_PLL_200M] = imx_clk_fixed_factor("sys2_pll_200m", "sys2_pll_out", 1, 5);
	clks[IMX8MQ_SYS2_PLL_250M] = imx_clk_fixed_factor("sys2_pll_250m", "sys2_pll_out", 1, 4);
	clks[IMX8MQ_SYS2_PLL_333M] = imx_clk_fixed_factor("sys2_pll_333m", "sys2_pll_out", 1, 3);
	clks[IMX8MQ_SYS2_PLL_500M] = imx_clk_fixed_factor("sys2_pll_500m", "sys2_pll_out", 1, 2);
	clks[IMX8MQ_SYS2_PLL_1000M] = imx_clk_fixed_factor("sys2_pll_1000m", "sys2_pll_out", 1, 1);

	np = ccm_node;
	base = of_iomap(np, 0);
	WARN_ON(!base);
	/* CORE */
	clks[IMX8MQ_CLK_A53_SRC] = imx_clk_mux2("arm_a53_src", base + 0x8000, 24, 3, imx8mq_a53_sels, ARRAY_SIZE(imx8mq_a53_sels));
	clks[IMX8MQ_CLK_A53_CG] = imx_clk_gate3("arm_a53_cg", "arm_a53_src", base + 0x8000, 28);
	clks[IMX8MQ_CLK_A53_DIV] = imx_clk_divider2("arm_a53_div", "arm_a53_cg", base + 0x8000, 0, 3);

	/* BUS */
	clks[IMX8MQ_CLK_MAIN_AXI_SRC] = imx_clk_mux2("main_axi_src", base + 0x8800, 24, 3, imx8mq_main_axi_sels, ARRAY_SIZE(imx8mq_main_axi_sels));
	clks[IMX8MQ_CLK_ENET_AXI_SRC] = imx_clk_mux2("enet_axi_src", base + 0x8880, 24, 3, imx8mq_enet_axi_sels, ARRAY_SIZE(imx8mq_enet_axi_sels));
	clks[IMX8MQ_CLK_NAND_USDHC_BUS_SRC] = imx_clk_mux2("nand_usdhc_bus_src", base + 0x8900, 24, 3, imx8mq_nand_usdhc_sels, ARRAY_SIZE(imx8mq_nand_usdhc_sels));
	clks[IMX8MQ_CLK_USB_BUS_SRC] = imx_clk_mux2("usb_bus_src", base + 0x8b80, 24, 3, imx8mq_usb_bus_sels, ARRAY_SIZE(imx8mq_usb_bus_sels));
	clks[IMX8MQ_CLK_NOC_SRC] = imx_clk_mux2("noc_src", base + 0x8d00, 24, 3, imx8mq_noc_sels, ARRAY_SIZE(imx8mq_noc_sels));
	clks[IMX8MQ_CLK_NOC_APB_SRC] = imx_clk_mux2("noc_apb_src", base + 0x8d80, 24, 3, imx8mq_noc_apb_sels, ARRAY_SIZE(imx8mq_noc_apb_sels));

	clks[IMX8MQ_CLK_MAIN_AXI_CG] = imx_clk_gate3("main_axi_cg", "main_axi_src", base + 0x8800, 28);
	clks[IMX8MQ_CLK_ENET_AXI_CG] = imx_clk_gate3("enet_axi_cg", "enet_axi_src", base + 0x8880, 28);
	clks[IMX8MQ_CLK_NAND_USDHC_BUS_CG] = imx_clk_gate3("nand_usdhc_bus_cg", "nand_usdhc_bus_src", base + 0x8900, 28);
	clks[IMX8MQ_CLK_USB_BUS_CG] = imx_clk_gate3("usb_bus_cg", "usb_bus_src", base + 0x8b80, 28);
	clks[IMX8MQ_CLK_NOC_CG] = imx_clk_gate3("noc_cg", "noc_src", base + 0x8d00, 28);
	clks[IMX8MQ_CLK_NOC_APB_CG] = imx_clk_gate3("noc_apb_cg", "noc_apb_src", base + 0x8d80, 28);

	clks[IMX8MQ_CLK_MAIN_AXI_PRE_DIV] = imx_clk_divider2("main_axi_pre_div", "main_axi_cg", base + 0x8800, 16, 3);
	clks[IMX8MQ_CLK_ENET_AXI_PRE_DIV] = imx_clk_divider2("enet_axi_pre_div", "enet_axi_cg", base + 0x8880, 16, 3);
	clks[IMX8MQ_CLK_NAND_USDHC_BUS_PRE_DIV] = imx_clk_divider2("nand_usdhc_bus_pre_div", "nand_usdhc_bus_cg", base + 0x8900, 16, 3);
	clks[IMX8MQ_CLK_DISP_AXI_PRE_DIV] = imx_clk_divider2("disp_axi_pre_div", "disp_axi_cg", base + 0x8a00, 16, 3);
	clks[IMX8MQ_CLK_DISP_APB_PRE_DIV] = imx_clk_divider2("disp_apb_pre_div", "disp_apb_cg", base + 0x8a80, 16, 3);
	clks[IMX8MQ_CLK_DISP_RTRM_PRE_DIV] = imx_clk_divider2("disp_rtrm_pre_div", "disp_rtrm_cg", base + 0x8b00, 16, 3);
	clks[IMX8MQ_CLK_USB_BUS_PRE_DIV] = imx_clk_divider2("usb_bus_pre_div", "usb_bus_cg", base + 0x8b80, 16, 3);
	clks[IMX8MQ_CLK_GPU_AXI_PRE_DIV] = imx_clk_divider2("gpu_axi_pre_div", "gpu_axi_cg", base + 0x8c00, 16, 3);
	clks[IMX8MQ_CLK_GPU_AHB_PRE_DIV] = imx_clk_divider2("gpu_ahb_pre_div", "gpu_ahb_cg", base + 0x8c80, 16, 3);
	clks[IMX8MQ_CLK_NOC_PRE_DIV] = imx_clk_divider2("noc_pre_div", "noc_cg", base + 0x8d00, 16, 3);
	clks[IMX8MQ_CLK_NOC_APB_PRE_DIV] = imx_clk_divider2("noc_apb_pre_div", "noc_apb_cg", base + 0x8d80, 16, 3);

	clks[IMX8MQ_CLK_MAIN_AXI_DIV] = imx_clk_divider2("main_axi_div", "main_axi_pre_div", base + 0x8800, 0, 6);
	clks[IMX8MQ_CLK_ENET_AXI_DIV] = imx_clk_divider2("enet_axi_div", "enet_axi_pre_div", base + 0x8880, 0, 6);
	clks[IMX8MQ_CLK_NAND_USDHC_BUS_DIV] = imx_clk_divider2("nand_usdhc_bus_div", "nand_usdhc_bus_pre_div", base + 0x8900, 0, 6);
	clks[IMX8MQ_CLK_USB_BUS_DIV] = imx_clk_divider2("usb_bus_div", "usb_bus_pre_div", base + 0x8b80, 0, 6);
	clks[IMX8MQ_CLK_NOC_DIV] = imx_clk_divider2("noc_div", "noc_pre_div", base + 0x8d00, 0, 6);
	clks[IMX8MQ_CLK_NOC_APB_DIV] = imx_clk_divider2("noc_apb_div", "noc_apb_pre_div", base + 0x8d80, 0, 6);

	/* AHB */
	clks[IMX8MQ_CLK_AHB_SRC] = imx_clk_mux2("ahb_src", base + 0x9000, 24, 3, imx8mq_ahb_sels, ARRAY_SIZE(imx8mq_ahb_sels));
	clks[IMX8MQ_CLK_AHB_CG] = imx_clk_gate3("ahb_cg", "ahb_src", base + 0x9000, 28);
	clks[IMX8MQ_CLK_AHB_PRE_DIV] = imx_clk_divider2("ahb_pre_div", "ahb_cg", base + 0x9000, 16, 3);
	clks[IMX8MQ_CLK_AHB_DIV] = imx_clk_divider_flags("ahb_div", "ahb_pre_div", base + 0x9000, 0, 6, CLK_SET_RATE_PARENT | CLK_OPS_PARENT_ENABLE);

	/* IPG */
	clks[IMX8MQ_CLK_IPG_ROOT] = imx_clk_divider2("ipg_root", "ahb_div", base + 0x9080, 0, 1);

	/* IP */
	clks[IMX8MQ_CLK_DRAM_ALT_SRC] = imx_clk_mux2("dram_alt_src", base + 0xa000, 24, 3, imx8mq_dram_alt_sels, ARRAY_SIZE(imx8mq_dram_alt_sels));
	clks[IMX8MQ_CLK_DRAM_CORE] = imx_clk_mux2("dram_core_clk", base + 0x9800, 24, 1, imx8mq_dram_core_sels, ARRAY_SIZE(imx8mq_dram_core_sels));
	clks[IMX8MQ_CLK_DRAM_APB_SRC] = imx_clk_mux2("dram_apb_src", base + 0xa080, 24, 3, imx8mq_dram_apb_sels, ARRAY_SIZE(imx8mq_dram_apb_sels));
	clks[IMX8MQ_CLK_PCIE1_CTRL_SRC] = imx_clk_mux2("pcie1_ctrl_src", base + 0xa300, 24, 3, imx8mq_pcie1_ctrl_sels, ARRAY_SIZE(imx8mq_pcie1_ctrl_sels));
	clks[IMX8MQ_CLK_PCIE1_PHY_SRC] = imx_clk_mux2("pcie1_phy_src", base + 0xa380, 24, 3, imx8mq_pcie1_phy_sels, ARRAY_SIZE(imx8mq_pcie1_phy_sels));
	clks[IMX8MQ_CLK_PCIE1_AUX_SRC] = imx_clk_mux2("pcie1_aux_src", base + 0xa400, 24, 3, imx8mq_pcie1_aux_sels, ARRAY_SIZE(imx8mq_pcie1_aux_sels));
	clks[IMX8MQ_CLK_DC_PIXEL_SRC] = imx_clk_mux2("dc_pixel_src", base + 0xa480, 24, 3, imx8mq_dc_pixel_sels, ARRAY_SIZE(imx8mq_dc_pixel_sels));
	clks[IMX8MQ_CLK_LCDIF_PIXEL_SRC] = imx_clk_mux2("lcdif_pixel_src", base + 0xa500, 24, 3, imx8mq_lcdif_pixel_sels, ARRAY_SIZE(imx8mq_lcdif_pixel_sels));
	clks[IMX8MQ_CLK_ENET_REF_SRC] = imx_clk_mux2("enet_ref_src", base + 0xa980, 24, 3, imx8mq_enet_ref_sels, ARRAY_SIZE(imx8mq_enet_ref_sels));
	clks[IMX8MQ_CLK_ENET_TIMER_SRC] = imx_clk_mux2("enet_timer_src", base + 0xaa00, 24, 3, imx8mq_enet_timer_sels, ARRAY_SIZE(imx8mq_enet_timer_sels));
	clks[IMX8MQ_CLK_ENET_PHY_REF_SRC] = imx_clk_mux2("enet_phy_src", base + 0xaa80, 24, 3, imx8mq_enet_phy_sels, ARRAY_SIZE(imx8mq_enet_phy_sels));
	clks[IMX8MQ_CLK_NAND_SRC] = imx_clk_mux2("nand_src", base + 0xab00, 24, 3, imx8mq_nand_sels, ARRAY_SIZE(imx8mq_nand_sels));
	clks[IMX8MQ_CLK_QSPI_SRC] = imx_clk_mux2("qspi_src", base + 0xab80, 24, 3, imx8mq_qspi_sels, ARRAY_SIZE(imx8mq_qspi_sels));
	clks[IMX8MQ_CLK_USDHC1_SRC] = imx_clk_mux2("usdhc1_src", base + 0xac00, 24, 3, imx8mq_usdhc1_sels, ARRAY_SIZE(imx8mq_usdhc1_sels));
	clks[IMX8MQ_CLK_USDHC2_SRC] = imx_clk_mux2("usdhc2_src", base + 0xac80, 24, 3, imx8mq_usdhc2_sels, ARRAY_SIZE(imx8mq_usdhc2_sels));
	clks[IMX8MQ_CLK_I2C1_SRC] = imx_clk_mux2("i2c1_src", base + 0xad00, 24, 3, imx8mq_i2c1_sels, ARRAY_SIZE(imx8mq_i2c1_sels));
	clks[IMX8MQ_CLK_I2C2_SRC] = imx_clk_mux2("i2c2_src", base + 0xad80, 24, 3, imx8mq_i2c2_sels, ARRAY_SIZE(imx8mq_i2c2_sels));
	clks[IMX8MQ_CLK_I2C3_SRC] = imx_clk_mux2("i2c3_src", base + 0xae00, 24, 3, imx8mq_i2c3_sels, ARRAY_SIZE(imx8mq_i2c3_sels));
	clks[IMX8MQ_CLK_I2C4_SRC] = imx_clk_mux2("i2c4_src", base + 0xae80, 24, 3, imx8mq_i2c4_sels, ARRAY_SIZE(imx8mq_i2c4_sels));
	clks[IMX8MQ_CLK_UART1_SRC] = imx_clk_mux2("uart1_src", base + 0xaf00, 24, 3, imx8mq_uart1_sels, ARRAY_SIZE(imx8mq_uart1_sels));
	clks[IMX8MQ_CLK_UART2_SRC] = imx_clk_mux2("uart2_src", base + 0xaf80, 24, 3, imx8mq_uart2_sels, ARRAY_SIZE(imx8mq_uart2_sels));
	clks[IMX8MQ_CLK_UART3_SRC] = imx_clk_mux2("uart3_src", base + 0xb000, 24, 3, imx8mq_uart3_sels, ARRAY_SIZE(imx8mq_uart3_sels));
	clks[IMX8MQ_CLK_UART4_SRC] = imx_clk_mux2("uart4_src", base + 0xb080, 24, 3, imx8mq_uart4_sels, ARRAY_SIZE(imx8mq_uart4_sels));
	clks[IMX8MQ_CLK_USB_CORE_REF_SRC] = imx_clk_mux2("usb_core_ref_src", base + 0xb100, 24, 3, imx8mq_usb_core_sels, ARRAY_SIZE(imx8mq_usb_core_sels));
	clks[IMX8MQ_CLK_USB_PHY_REF_SRC] = imx_clk_mux2("usb_phy_ref_src", base + 0xb180, 24, 3, imx8mq_usb_phy_sels, ARRAY_SIZE(imx8mq_usb_phy_sels));
	clks[IMX8MQ_CLK_ECSPI1_SRC] = imx_clk_mux2("ecspi1_src", base + 0xb280, 24, 3, imx8mq_ecspi1_sels, ARRAY_SIZE(imx8mq_ecspi1_sels));
	clks[IMX8MQ_CLK_ECSPI2_SRC] = imx_clk_mux2("ecspi2_src", base + 0xb300, 24, 3, imx8mq_ecspi2_sels, ARRAY_SIZE(imx8mq_ecspi2_sels));
	clks[IMX8MQ_CLK_PWM1_SRC] = imx_clk_mux2("pwm1_src", base + 0xb380, 24, 3, imx8mq_pwm1_sels, ARRAY_SIZE(imx8mq_pwm1_sels));
	clks[IMX8MQ_CLK_PWM2_SRC] = imx_clk_mux2("pwm2_src", base + 0xb400, 24, 3, imx8mq_pwm2_sels, ARRAY_SIZE(imx8mq_pwm2_sels));
	clks[IMX8MQ_CLK_PWM3_SRC] = imx_clk_mux2("pwm3_src", base + 0xb480, 24, 3, imx8mq_pwm3_sels, ARRAY_SIZE(imx8mq_pwm3_sels));
	clks[IMX8MQ_CLK_PWM4_SRC] = imx_clk_mux2("pwm4_src", base + 0xb500, 24, 3, imx8mq_pwm4_sels, ARRAY_SIZE(imx8mq_pwm4_sels));
	clks[IMX8MQ_CLK_GPT1_SRC] = imx_clk_mux2("gpt1_src", base + 0xb580, 24, 3, imx8mq_gpt1_sels, ARRAY_SIZE(imx8mq_gpt1_sels));
	clks[IMX8MQ_CLK_WDOG_SRC] = imx_clk_mux2("wdog_src", base + 0xb900, 24, 3, imx8mq_wdog_sels, ARRAY_SIZE(imx8mq_wdog_sels));
	clks[IMX8MQ_CLK_WRCLK_SRC] = imx_clk_mux2("wrclk_src", base + 0xb980, 24, 3, imx8mq_wrclk_sels, ARRAY_SIZE(imx8mq_wrclk_sels));
	clks[IMX8MQ_CLK_CLKO2_SRC] = imx_clk_mux2("clko2_src", base + 0xba80, 24, 3, imx8mq_clko2_sels, ARRAY_SIZE(imx8mq_clko2_sels));
	clks[IMX8MQ_CLK_PCIE2_CTRL_SRC] = imx_clk_mux2("pcie2_ctrl_src", base + 0xc000, 24, 3, imx8mq_pcie2_ctrl_sels, ARRAY_SIZE(imx8mq_pcie2_ctrl_sels));
	clks[IMX8MQ_CLK_PCIE2_PHY_SRC] = imx_clk_mux2("pcie2_phy_src", base + 0xc080, 24, 3, imx8mq_pcie2_phy_sels, ARRAY_SIZE(imx8mq_pcie2_phy_sels));
	clks[IMX8MQ_CLK_PCIE2_AUX_SRC] = imx_clk_mux2("pcie2_aux_src", base + 0xc100, 24, 3, imx8mq_pcie2_aux_sels, ARRAY_SIZE(imx8mq_pcie2_aux_sels));
	clks[IMX8MQ_CLK_ECSPI3_SRC] = imx_clk_mux2("ecspi3_src", base + 0xc180, 24, 3, imx8mq_ecspi3_sels, ARRAY_SIZE(imx8mq_ecspi3_sels));

	clks[IMX8MQ_CLK_DRAM_ALT_CG] = imx_clk_gate3("dram_alt_cg", "dram_alt_src", base + 0xa000, 28);
	clks[IMX8MQ_CLK_DRAM_APB_CG] = imx_clk_gate3("dram_apb_cg", "dram_apb_src", base + 0xa080, 28);
	clks[IMX8MQ_CLK_PCIE1_CTRL_CG] = imx_clk_gate3("pcie1_ctrl_cg", "pcie1_ctrl_src", base + 0xa300, 28);
	clks[IMX8MQ_CLK_PCIE1_PHY_CG] = imx_clk_gate3("pcie1_phy_cg", "pcie1_phy_src", base + 0xa380, 28);
	clks[IMX8MQ_CLK_PCIE1_AUX_CG] = imx_clk_gate3("pcie1_aux_cg", "pcie1_aux_src", base + 0xa400, 28);
	clks[IMX8MQ_CLK_ENET_REF_CG] = imx_clk_gate3("enet_ref_cg", "enet_ref_src", base + 0xa980, 28);
	clks[IMX8MQ_CLK_ENET_TIMER_CG] = imx_clk_gate3("enet_timer_cg", "enet_timer_src", base + 0xaa00, 28);
	clks[IMX8MQ_CLK_ENET_PHY_REF_CG] = imx_clk_gate3("enet_phy_cg", "enet_phy_src", base + 0xaa80, 28);
	clks[IMX8MQ_CLK_NAND_CG] = imx_clk_gate3("nand_cg", "nand_src", base + 0xab00, 28);
	clks[IMX8MQ_CLK_QSPI_CG] = imx_clk_gate3("qspi_cg", "qspi_src", base + 0xab80, 28);
	clks[IMX8MQ_CLK_USDHC1_CG] = imx_clk_gate3("usdhc1_cg", "usdhc1_src", base + 0xac00, 28);
	clks[IMX8MQ_CLK_USDHC2_CG] = imx_clk_gate3("usdhc2_cg", "usdhc2_src", base + 0xac80, 28);
	clks[IMX8MQ_CLK_I2C1_CG] = imx_clk_gate3("i2c1_cg", "i2c1_src", base + 0xad00, 28);
	clks[IMX8MQ_CLK_I2C2_CG] = imx_clk_gate3("i2c2_cg", "i2c2_src", base + 0xad80, 28);
	clks[IMX8MQ_CLK_I2C3_CG] = imx_clk_gate3("i2c3_cg", "i2c3_src", base + 0xae00, 28);
	clks[IMX8MQ_CLK_I2C4_CG] = imx_clk_gate3("i2c4_cg", "i2c4_src", base + 0xae80, 28);
	clks[IMX8MQ_CLK_UART1_CG] = imx_clk_gate3("uart1_cg", "uart1_src", base + 0xaf00, 28);
	clks[IMX8MQ_CLK_UART2_CG] = imx_clk_gate3("uart2_cg", "uart2_src", base + 0xaf80, 28);
	clks[IMX8MQ_CLK_UART3_CG] = imx_clk_gate3("uart3_cg", "uart3_src", base + 0xb000, 28);
	clks[IMX8MQ_CLK_UART4_CG] = imx_clk_gate3("uart4_cg", "uart4_src", base + 0xb080, 28);
	clks[IMX8MQ_CLK_USB_CORE_REF_CG] = imx_clk_gate3("usb_core_ref_cg", "usb_core_ref_src", base + 0xb100, 28);
	clks[IMX8MQ_CLK_USB_PHY_REF_CG] = imx_clk_gate3("usb_phy_ref_cg", "usb_phy_ref_src", base + 0xb180, 28);
	clks[IMX8MQ_CLK_ECSPI1_CG] = imx_clk_gate3("ecspi1_cg", "ecspi1_src",  base + 0xb280, 28);
	clks[IMX8MQ_CLK_ECSPI2_CG] = imx_clk_gate3("ecspi2_cg", "ecspi2_src", base + 0xb300, 28);
	clks[IMX8MQ_CLK_PWM1_CG] = imx_clk_gate3("pwm1_cg", "pwm1_src", base + 0xb380, 28);
	clks[IMX8MQ_CLK_PWM2_CG] = imx_clk_gate3("pwm2_cg", "pwm2_src", base + 0xb400, 28);
	clks[IMX8MQ_CLK_PWM3_CG] = imx_clk_gate3("pwm3_cg", "pwm3_src", base + 0xb480, 28);
	clks[IMX8MQ_CLK_PWM4_CG] = imx_clk_gate3("pwm4_cg", "pwm4_src", base + 0xb500, 28);
	clks[IMX8MQ_CLK_GPT1_CG] = imx_clk_gate3("gpt1_cg", "gpt1_src", base + 0xb580, 28);
	clks[IMX8MQ_CLK_WDOG_CG] = imx_clk_gate3("wdog_cg", "wdog_src", base + 0xb900, 28);
	clks[IMX8MQ_CLK_WRCLK_CG] = imx_clk_gate3("wrclk_cg", "wrclk_src", base + 0xb980, 28);
	clks[IMX8MQ_CLK_CLKO2_CG] = imx_clk_gate3("clko2_cg", "clko2_src", base + 0xba80, 28);
	clks[IMX8MQ_CLK_PCIE2_CTRL_CG] = imx_clk_gate3("pcie2_ctrl_cg", "pcie2_ctrl_src", base + 0xc000, 28);
	clks[IMX8MQ_CLK_PCIE2_PHY_CG] = imx_clk_gate3("pcie2_phy_cg", "pcie2_phy_src", base + 0xc080, 28);
	clks[IMX8MQ_CLK_PCIE2_AUX_CG] = imx_clk_gate3("pcie2_aux_cg", "pcie2_aux_src", base + 0xc100, 28);
	clks[IMX8MQ_CLK_ECSPI3_CG] = imx_clk_gate3("ecspi3_cg", "ecspi3_src", base + 0xc180, 28);

	clks[IMX8MQ_CLK_DRAM_ALT_PRE_DIV] = imx_clk_divider2("dram_alt_pre_div", "dram_alt_cg", base + 0xa000, 16, 3);
	clks[IMX8MQ_CLK_DRAM_APB_PRE_DIV] = imx_clk_divider_flags("dram_apb_pre_div", "dram_apb_cg", base + 0xa080, 16, 3, CLK_OPS_PARENT_ENABLE);
	clks[IMX8MQ_CLK_PCIE1_CTRL_PRE_DIV] = imx_clk_divider2("pcie1_ctrl_pre_div", "pcie1_ctrl_cg", base + 0xa300, 16, 3);
	clks[IMX8MQ_CLK_PCIE1_PHY_PRE_DIV] = imx_clk_divider2("pcie1_phy_pre_div", "pcie1_phy_cg", base + 0xa380, 16, 3);
	clks[IMX8MQ_CLK_PCIE1_AUX_PRE_DIV] = imx_clk_divider2("pcie1_aux_pre_div", "pcie1_aux_cg", base + 0xa400, 16, 3);
	clks[IMX8MQ_CLK_DC_PIXEL_PRE_DIV] = imx_clk_divider2("dc_pixel_pre_div", "dc_pixel_cg", base + 0xa480, 16, 3);
	clks[IMX8MQ_CLK_LCDIF_PIXEL_PRE_DIV] = imx_clk_divider2("lcdif_pixel_pre_div", "lcdif_pixel_cg", base + 0xa500, 16, 3);
	clks[IMX8MQ_CLK_SPDIF1_PRE_DIV] = imx_clk_divider2("spdif1_pre_div", "spdif1_cg", base + 0xa880, 16, 3);
	clks[IMX8MQ_CLK_SPDIF2_PRE_DIV] = imx_clk_divider2("spdif2_pre_div", "spdif2_cg", base + 0xa900, 16, 3);
	clks[IMX8MQ_CLK_ENET_REF_PRE_DIV] = imx_clk_divider2("enet_ref_pre_div", "enet_ref_cg", base + 0xa980, 16, 3);
	clks[IMX8MQ_CLK_ENET_TIMER_PRE_DIV] = imx_clk_divider2("enet_timer_pre_div", "enet_timer_cg", base + 0xaa00, 16, 3);
	clks[IMX8MQ_CLK_ENET_PHY_REF_PRE_DIV] = imx_clk_divider2("enet_phy_pre_div", "enet_phy_cg", base + 0xaa80, 16, 3);
	clks[IMX8MQ_CLK_NAND_PRE_DIV] = imx_clk_divider2("nand_pre_div", "nand_cg", base + 0xab00, 16, 3);
	clks[IMX8MQ_CLK_QSPI_PRE_DIV] = imx_clk_divider2("qspi_pre_div", "qspi_cg", base + 0xab80, 16, 3);
	clks[IMX8MQ_CLK_USDHC1_PRE_DIV] = imx_clk_divider2("usdhc1_pre_div", "usdhc1_cg", base + 0xac00, 16, 3);
	clks[IMX8MQ_CLK_USDHC2_PRE_DIV] = imx_clk_divider2("usdhc2_pre_div", "usdhc2_cg", base + 0xac80, 16, 3);
	clks[IMX8MQ_CLK_I2C1_PRE_DIV] = imx_clk_divider2("i2c1_pre_div", "i2c1_cg", base + 0xad00, 16, 3);
	clks[IMX8MQ_CLK_I2C2_PRE_DIV] = imx_clk_divider2("i2c2_pre_div", "i2c2_cg", base + 0xad80, 16, 3);
	clks[IMX8MQ_CLK_I2C3_PRE_DIV] = imx_clk_divider2("i2c3_pre_div", "i2c3_cg", base + 0xae00, 16, 3);
	clks[IMX8MQ_CLK_I2C4_PRE_DIV] = imx_clk_divider2("i2c4_pre_div", "i2c4_cg", base + 0xae80, 16, 3);
	clks[IMX8MQ_CLK_UART1_PRE_DIV] = imx_clk_divider2("uart1_pre_div", "uart1_cg", base + 0xaf00, 16, 3);
	clks[IMX8MQ_CLK_UART2_PRE_DIV] = imx_clk_divider2("uart2_pre_div", "uart2_cg", base + 0xaf80, 16, 3);
	clks[IMX8MQ_CLK_UART3_PRE_DIV] = imx_clk_divider2("uart3_pre_div", "uart3_cg", base + 0xb000, 16, 3);
	clks[IMX8MQ_CLK_UART4_PRE_DIV] = imx_clk_divider2("uart4_pre_div", "uart4_cg", base + 0xb080, 16, 3);
	clks[IMX8MQ_CLK_USB_CORE_REF_PRE_DIV] = imx_clk_divider2("usb_core_ref_pre_div", "usb_core_ref_cg", base + 0xb100, 16, 3);
	clks[IMX8MQ_CLK_USB_PHY_REF_PRE_DIV] = imx_clk_divider2("usb_phy_ref_pre_div", "usb_phy_ref_cg", base + 0xb180, 16, 3);
	clks[IMX8MQ_CLK_ECSPI1_PRE_DIV] = imx_clk_divider2("ecspi1_pre_div", "ecspi1_cg", base + 0xb280, 16, 3);
	clks[IMX8MQ_CLK_ECSPI2_PRE_DIV] = imx_clk_divider2("ecspi2_pre_div", "ecspi2_cg", base + 0xb300, 16, 3);
	clks[IMX8MQ_CLK_PWM1_PRE_DIV] = imx_clk_divider2("pwm1_pre_div", "pwm1_cg", base + 0xb380, 16, 3);
	clks[IMX8MQ_CLK_PWM2_PRE_DIV] = imx_clk_divider2("pwm2_pre_div", "pwm2_cg", base + 0xb400, 16, 3);
	clks[IMX8MQ_CLK_PWM3_PRE_DIV] = imx_clk_divider2("pwm3_pre_div", "pwm3_cg", base + 0xb480, 16, 3);
	clks[IMX8MQ_CLK_PWM4_PRE_DIV] = imx_clk_divider2("pwm4_pre_div", "pwm4_cg", base + 0xb500, 16, 3);
	clks[IMX8MQ_CLK_GPT1_PRE_DIV] = imx_clk_divider2("gpt1_pre_div", "gpt1_cg", base + 0xb580, 16, 3);
	clks[IMX8MQ_CLK_WDOG_PRE_DIV] = imx_clk_divider2("wdog_pre_div", "wdog_cg", base + 0xb900, 16, 3);
	clks[IMX8MQ_CLK_WRCLK_PRE_DIV] = imx_clk_divider2("wrclk_pre_div", "wrclk_cg", base + 0xb980, 16, 3);
	clks[IMX8MQ_CLK_CLKO2_PRE_DIV] = imx_clk_divider2("clko2_pre_div", "clko2_cg", base + 0xba80, 16, 3);
	clks[IMX8MQ_CLK_PCIE2_CTRL_PRE_DIV] = imx_clk_divider2("pcie2_ctrl_pre_div", "pcie2_ctrl_cg", base + 0xc000, 16, 3);
	clks[IMX8MQ_CLK_PCIE2_PHY_PRE_DIV] = imx_clk_divider2("pcie2_phy_pre_div", "pcie2_phy_cg", base + 0xc080, 16, 3);
	clks[IMX8MQ_CLK_PCIE2_AUX_PRE_DIV] = imx_clk_divider2("pcie2_aux_pre_div", "pcie2_aux_cg", base + 0xc100, 16, 3);
	clks[IMX8MQ_CLK_ECSPI3_PRE_DIV] = imx_clk_divider2("ecspi3_pre_div", "ecspi3_cg", base + 0xc180, 16, 3);

	clks[IMX8MQ_CLK_DRAM_ALT_DIV] = imx_clk_divider2("dram_alt_div", "dram_alt_pre_div", base + 0xa000, 0, 6);
	clks[IMX8MQ_CLK_DRAM_APB_DIV] = imx_clk_divider2("dram_apb_div", "dram_apb_pre_div", base + 0xa080, 0, 6);
	clks[IMX8MQ_CLK_PCIE1_CTRL_DIV] = imx_clk_divider2("pcie1_ctrl_div", "pcie1_ctrl_pre_div", base + 0xa300, 0, 6);
	clks[IMX8MQ_CLK_PCIE1_PHY_DIV] = imx_clk_divider2("pcie1_phy_div", "pcie1_phy_pre_div", base + 0xa380, 0, 6);
	clks[IMX8MQ_CLK_PCIE1_AUX_DIV] = imx_clk_divider2("pcie1_aux_div", "pcie1_aux_pre_div", base + 0xa400, 0, 6);
	clks[IMX8MQ_CLK_DC_PIXEL_DIV] = imx_clk_divider2("dc_pixel_div", "dc_pixel_pre_div", base + 0xa480, 0, 6);
	clks[IMX8MQ_CLK_LCDIF_PIXEL_DIV] = imx_clk_divider2("lcdif_pixel_div", "lcdif_pixel_pre_div", base + 0xa500, 0, 6);
	clks[IMX8MQ_CLK_ENET_REF_DIV] = imx_clk_divider2("enet_ref_div", "enet_ref_pre_div", base + 0xa980, 0, 6);
	clks[IMX8MQ_CLK_ENET_TIMER_DIV] = imx_clk_divider2("enet_timer_div", "enet_timer_pre_div", base + 0xaa00, 0, 6);
	clks[IMX8MQ_CLK_ENET_PHY_REF_DIV] = imx_clk_divider2("enet_phy_div", "enet_phy_pre_div", base + 0xaa80, 0, 6);
	clks[IMX8MQ_CLK_NAND_DIV] = imx_clk_divider2("nand_div", "nand_pre_div", base + 0xab00, 0, 6);
	clks[IMX8MQ_CLK_QSPI_DIV] = imx_clk_divider2("qspi_div", "qspi_pre_div", base + 0xab80, 0, 6);
	clks[IMX8MQ_CLK_USDHC1_DIV] = imx_clk_divider2("usdhc1_div", "usdhc1_pre_div", base + 0xac00, 0, 6);
	clks[IMX8MQ_CLK_USDHC2_DIV] = imx_clk_divider2("usdhc2_div", "usdhc2_pre_div", base + 0xac80, 0, 6);
	clks[IMX8MQ_CLK_I2C1_DIV] = imx_clk_divider2("i2c1_div", "i2c1_pre_div", base + 0xad00, 0, 6);
	clks[IMX8MQ_CLK_I2C2_DIV] = imx_clk_divider2("i2c2_div", "i2c2_pre_div", base + 0xad80, 0, 6);
	clks[IMX8MQ_CLK_I2C3_DIV] = imx_clk_divider2("i2c3_div", "i2c3_pre_div", base + 0xae00, 0, 6);
	clks[IMX8MQ_CLK_I2C4_DIV] = imx_clk_divider2("i2c4_div", "i2c4_pre_div", base + 0xae80, 0, 6);
	clks[IMX8MQ_CLK_UART1_DIV] = imx_clk_divider2("uart1_div", "uart1_pre_div", base + 0xaf00, 0, 6);
	clks[IMX8MQ_CLK_UART2_DIV] = imx_clk_divider2("uart2_div", "uart2_pre_div", base + 0xaf80, 0, 6);
	clks[IMX8MQ_CLK_UART3_DIV] = imx_clk_divider2("uart3_div", "uart3_pre_div", base + 0xb000, 0, 6);
	clks[IMX8MQ_CLK_UART4_DIV] = imx_clk_divider2("uart4_div", "uart4_pre_div", base + 0xb080, 0, 6);
	clks[IMX8MQ_CLK_USB_CORE_REF_DIV] = imx_clk_divider2("usb_core_ref_div", "usb_core_ref_pre_div", base + 0xb100, 0, 6);
	clks[IMX8MQ_CLK_USB_PHY_REF_DIV] = imx_clk_divider2("usb_phy_ref_div", "usb_phy_ref_pre_div", base + 0xb180, 0, 6);
	clks[IMX8MQ_CLK_ECSPI1_DIV] = imx_clk_divider2("ecspi1_div", "ecspi1_pre_div", base + 0xb280, 0, 6);
	clks[IMX8MQ_CLK_ECSPI2_DIV] = imx_clk_divider2("ecspi2_div", "ecspi2_pre_div", base + 0xb300, 0, 6);
	clks[IMX8MQ_CLK_PWM1_DIV] = imx_clk_divider2("pwm1_div", "pwm1_pre_div", base + 0xb380, 0, 6);
	clks[IMX8MQ_CLK_PWM2_DIV] = imx_clk_divider2("pwm2_div", "pwm2_pre_div", base + 0xb400, 0, 6);
	clks[IMX8MQ_CLK_PWM3_DIV] = imx_clk_divider2("pwm3_div", "pwm3_pre_div", base + 0xb480, 0, 6);
	clks[IMX8MQ_CLK_PWM4_DIV] = imx_clk_divider2("pwm4_div", "pwm4_pre_div", base + 0xb500, 0, 6);
	clks[IMX8MQ_CLK_GPT1_DIV] = imx_clk_divider2("gpt1_div", "gpt1_pre_div", base + 0xb580, 0, 6);
	clks[IMX8MQ_CLK_WDOG_DIV] = imx_clk_divider2("wdog_div", "wdog_pre_div", base + 0xb900, 0, 6);
	clks[IMX8MQ_CLK_WRCLK_DIV] = imx_clk_divider2("wrclk_div", "wrclk_pre_div", base + 0xb980, 0, 6);
	clks[IMX8MQ_CLK_CLKO2_DIV] = imx_clk_divider2("clko2_div", "clko2_pre_div", base + 0xba80, 0, 6);
	clks[IMX8MQ_CLK_PCIE2_CTRL_DIV] = imx_clk_divider2("pcie2_ctrl_div", "pcie2_ctrl_pre_div", base + 0xc000, 0, 6);
	clks[IMX8MQ_CLK_PCIE2_PHY_DIV] = imx_clk_divider2("pcie2_phy_div", "pcie2_phy_pre_div", base + 0xc080, 0, 6);
	clks[IMX8MQ_CLK_PCIE2_AUX_DIV] = imx_clk_divider2("pcie2_aux_div", "pcie2_aux_pre_div", base + 0xc100, 0, 6);
	clks[IMX8MQ_CLK_ECSPI3_DIV] = imx_clk_divider2("ecspi3_div", "ecspi3_pre_div", base + 0xc180, 0, 6);

	/*FIXME, the doc is not ready now */
	clks[IMX8MQ_CLK_ECSPI1_ROOT] = imx_clk_gate4("ecspi1_root_clk", "ecspi1_div", base + 0x4070, 0);
	clks[IMX8MQ_CLK_ECSPI2_ROOT] = imx_clk_gate4("ecspi2_root_clk", "ecspi2_div", base + 0x4080, 0);
	clks[IMX8MQ_CLK_ECSPI3_ROOT] = imx_clk_gate4("ecspi3_root_clk", "ecspi3_div", base + 0x4090, 0);
	clks[IMX8MQ_CLK_ENET1_ROOT] = imx_clk_gate4("enet1_root_clk", "enet_axi_div", base + 0x40a0, 0);
	clks[IMX8MQ_CLK_GPT1_ROOT] = imx_clk_gate4("gpt1_root_clk", "gpt1_div", base + 0x4100, 0);
	clks[IMX8MQ_CLK_I2C1_ROOT] = imx_clk_gate4("i2c1_root_clk", "i2c1_div", base + 0x4170, 0);
	clks[IMX8MQ_CLK_I2C2_ROOT] = imx_clk_gate4("i2c2_root_clk", "i2c2_div", base + 0x4180, 0);
	clks[IMX8MQ_CLK_I2C3_ROOT] = imx_clk_gate4("i2c3_root_clk", "i2c3_div", base + 0x4190, 0);
	clks[IMX8MQ_CLK_I2C4_ROOT] = imx_clk_gate4("i2c4_root_clk", "i2c4_div", base + 0x41a0, 0);
	clks[IMX8MQ_CLK_MU_ROOT] = imx_clk_gate4("mu_root_clk", "ipg_root", base + 0x4210, 0);
	clks[IMX8MQ_CLK_OCOTP_ROOT] = imx_clk_gate4("ocotp_root_clk", "ipg_root", base + 0x4220, 0);
	clks[IMX8MQ_CLK_PCIE1_ROOT] = imx_clk_gate4("pcie1_root_clk", "pcie1_ctrl_div", base + 0x4250, 0);
	clks[IMX8MQ_CLK_PCIE2_ROOT] = imx_clk_gate4("pcie2_root_clk", "pcie2_ctrl_div", base + 0x4640, 0);
	clks[IMX8MQ_CLK_PWM1_ROOT] = imx_clk_gate4("pwm1_root_clk", "pwm1_div", base + 0x4280, 0);
	clks[IMX8MQ_CLK_PWM2_ROOT] = imx_clk_gate4("pwm2_root_clk", "pwm2_div", base + 0x4290, 0);
	clks[IMX8MQ_CLK_PWM3_ROOT] = imx_clk_gate4("pwm3_root_clk", "pwm3_div", base + 0x42a0, 0);
	clks[IMX8MQ_CLK_PWM4_ROOT] = imx_clk_gate4("pwm4_root_clk", "pwm4_div", base + 0x42b0, 0);
	clks[IMX8MQ_CLK_QSPI_ROOT] = imx_clk_gate4("qspi_root_clk", "qspi_div", base + 0x42f0, 0);
	clks[IMX8MQ_CLK_RAWNAND_ROOT] = imx_clk_gate4("nand_root_clk", "nand_div", base + 0x4300, 0);
	clks[IMX8MQ_CLK_NAND_USDHC_BUS_RAWNAND_CLK] = imx_clk_gate_shared("nand_usdhc_rawnand_clk", "nand_usdhc_bus_div", "nand_root_clk");
	clks[IMX8MQ_CLK_UART1_ROOT] = imx_clk_gate4("uart1_root_clk", "uart1_div", base + 0x4490, 0);
	clks[IMX8MQ_CLK_UART2_ROOT] = imx_clk_gate4("uart2_root_clk", "uart2_div", base + 0x44a0, 0);
	clks[IMX8MQ_CLK_UART3_ROOT] = imx_clk_gate4("uart3_root_clk", "uart3_div", base + 0x44b0, 0);
	clks[IMX8MQ_CLK_UART4_ROOT] = imx_clk_gate4("uart4_root_clk", "uart4_div", base + 0x44c0, 0);
	clks[IMX8MQ_CLK_USB1_CTRL_ROOT] = imx_clk_gate4("usb1_ctrl_root_clk", "usb_core_ref_div", base + 0x44d0, 0);
	clks[IMX8MQ_CLK_USB2_CTRL_ROOT] = imx_clk_gate4("usb2_ctrl_root_clk", "usb_core_ref_div", base + 0x44e0, 0);
	clks[IMX8MQ_CLK_USB1_PHY_ROOT] = imx_clk_gate4("usb1_phy_root_clk", "usb_phy_ref_div", base + 0x44f0, 0);
	clks[IMX8MQ_CLK_USB2_PHY_ROOT] = imx_clk_gate4("usb2_phy_root_clk", "usb_phy_ref_div", base + 0x4500, 0);
	clks[IMX8MQ_CLK_USDHC1_ROOT] = imx_clk_gate4("usdhc1_root_clk", "usdhc1_div", base + 0x4510, 0);
	clks[IMX8MQ_CLK_USDHC2_ROOT] = imx_clk_gate4("usdhc2_root_clk", "usdhc2_div", base + 0x4520, 0);
	clks[IMX8MQ_CLK_WDOG1_ROOT] = imx_clk_gate4("wdog1_root_clk", "wdog_div", base + 0x4530, 0);
	clks[IMX8MQ_CLK_WDOG2_ROOT] = imx_clk_gate4("wdog2_root_clk", "wdog_div", base + 0x4540, 0);
	clks[IMX8MQ_CLK_WDOG3_ROOT] = imx_clk_gate4("wdog3_root_clk", "wdog_div", base + 0x4550, 0);

	clks[IMX8MQ_GPT_3M_CLK] = imx_clk_fixed_factor("gpt_3m", "osc_25m", 1, 8);
	clks[IMX8MQ_CLK_DRAM_ALT_ROOT] = imx_clk_fixed_factor("dram_alt_root", "dram_alt_div", 1, 4);

	for (i = 0; i < IMX8MQ_CLK_END; i++)
		if (IS_ERR(clks[i]))
			pr_err("i.MX8mq clk %u register failed with %ld\n",
			       i, PTR_ERR(clks[i]));

	clk_data.clks = clks;
	clk_data.clk_num = ARRAY_SIZE(clks);
	of_clk_add_provider(np, of_clk_src_onecell_get, &clk_data);
}

CLK_OF_DECLARE(imx8mq, "fsl,imx8mq-ccm", imx8mq_clocks_init);
