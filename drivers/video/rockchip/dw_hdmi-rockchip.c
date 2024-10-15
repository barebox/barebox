// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2014, Fuzhou Rockchip Electronics Co., Ltd
 */

#include <linux/clk.h>
#include <driver.h>
#include <mfd/syscon.h>
#include <regulator.h>
#include <linux/bits.h>
#include <linux/regmap.h>
#include <video/dw_hdmi.h>
#include <linux/phy/phy.h>
#include <linux/math.h>
#include <video/drm/drm_connector.h>
#include <video/drm/drm_modes.h>
#include <fb.h>

#include "rockchip_drm_drv.h"

#define RK3568_GRF_VO_CON1		0x0364
#define RK3568_HDMI_SDAIN_MSK		BIT(15)
#define RK3568_HDMI_SCLIN_MSK		BIT(14)

#define HIWORD_UPDATE(val, mask)	(val | (mask) << 16)

/**
 * struct rockchip_hdmi_chip_data - splite the grf setting of kind of chips
 * @lcdsel_grf_reg: grf register offset of lcdc select
 * @lcdsel_big: reg value of selecting vop big for HDMI
 * @lcdsel_lit: reg value of selecting vop little for HDMI
 */
struct rockchip_hdmi_chip_data {
	int	lcdsel_grf_reg;
	u32	lcdsel_big;
	u32	lcdsel_lit;
};

struct rockchip_hdmi {
	struct device *dev;
	struct regmap *regmap;
	const struct rockchip_hdmi_chip_data *chip_data;
	const struct dw_hdmi_plat_data *plat_data;
	struct clk *ref_clk;
	struct clk *grf_clk;
	struct dw_hdmi *hdmi;
	struct regulator *avdd_0v9;
	struct regulator *avdd_1v8;
	struct phy *phy;
};

static const struct dw_hdmi_mpll_config rockchip_mpll_cfg[] = {
	{
		27000000, {
			{ 0x00b3, 0x0000},
			{ 0x2153, 0x0000},
			{ 0x40f3, 0x0000}
		},
	}, {
		36000000, {
			{ 0x00b3, 0x0000},
			{ 0x2153, 0x0000},
			{ 0x40f3, 0x0000}
		},
	}, {
		40000000, {
			{ 0x00b3, 0x0000},
			{ 0x2153, 0x0000},
			{ 0x40f3, 0x0000}
		},
	}, {
		54000000, {
			{ 0x0072, 0x0001},
			{ 0x2142, 0x0001},
			{ 0x40a2, 0x0001},
		},
	}, {
		65000000, {
			{ 0x0072, 0x0001},
			{ 0x2142, 0x0001},
			{ 0x40a2, 0x0001},
		},
	}, {
		66000000, {
			{ 0x013e, 0x0003},
			{ 0x217e, 0x0002},
			{ 0x4061, 0x0002}
		},
	}, {
		74250000, {
			{ 0x0072, 0x0001},
			{ 0x2145, 0x0002},
			{ 0x4061, 0x0002}
		},
	}, {
		83500000, {
			{ 0x0072, 0x0001},
		},
	}, {
		108000000, {
			{ 0x0051, 0x0002},
			{ 0x2145, 0x0002},
			{ 0x4061, 0x0002}
		},
	}, {
		106500000, {
			{ 0x0051, 0x0002},
			{ 0x2145, 0x0002},
			{ 0x4061, 0x0002}
		},
	}, {
		146250000, {
			{ 0x0051, 0x0002},
			{ 0x2145, 0x0002},
			{ 0x4061, 0x0002}
		},
	}, {
		148500000, {
			{ 0x0051, 0x0003},
			{ 0x214c, 0x0003},
			{ 0x4064, 0x0003}
		},
	}, {
		340000000, {
			{ 0x0040, 0x0003 },
			{ 0x3b4c, 0x0003 },
			{ 0x5a64, 0x0003 },
		},
	}, {
		~0UL, {
			{ 0x00a0, 0x000a },
			{ 0x2001, 0x000f },
			{ 0x4002, 0x000f },
		},
	}
};

static const struct dw_hdmi_curr_ctrl rockchip_cur_ctr[] = {
	/*      pixelclk    bpp8    bpp10   bpp12 */
	{
		40000000,  { 0x0018, 0x0018, 0x0018 },
	}, {
		65000000,  { 0x0028, 0x0028, 0x0028 },
	}, {
		66000000,  { 0x0038, 0x0038, 0x0038 },
	}, {
		74250000,  { 0x0028, 0x0038, 0x0038 },
	}, {
		83500000,  { 0x0028, 0x0038, 0x0038 },
	}, {
		146250000, { 0x0038, 0x0038, 0x0038 },
	}, {
		148500000, { 0x0000, 0x0038, 0x0038 },
	}, {
		600000000, { 0x0000, 0x0000, 0x0000 },
	}, {
		~0UL,      { 0x0000, 0x0000, 0x0000},
	}
};

static const struct dw_hdmi_phy_config rockchip_phy_config[] = {
	/*pixelclk   symbol   term   vlev*/
	{ 74250000,  0x8009, 0x0004, 0x0272},
	{ 148500000, 0x802b, 0x0004, 0x028d},
	{ 297000000, 0x8039, 0x0005, 0x028d},
	{ ~0UL,	     0x0000, 0x0000, 0x0000}
};

static int rockchip_hdmi_parse_dt(struct rockchip_hdmi *hdmi)
{
	struct device_node *np = hdmi->dev->of_node;

	hdmi->regmap = syscon_regmap_lookup_by_phandle(np, "rockchip,grf");
	if (IS_ERR(hdmi->regmap)) {
		dev_err(hdmi->dev, "Unable to get rockchip,grf\n");
		return PTR_ERR(hdmi->regmap);
	}

	hdmi->ref_clk = clk_get_optional(hdmi->dev, "ref");
	if (!hdmi->ref_clk)
		hdmi->ref_clk = clk_get_optional(hdmi->dev, "vpll");

	if (PTR_ERR(hdmi->ref_clk) == -EPROBE_DEFER) {
		return -EPROBE_DEFER;
	} else if (IS_ERR(hdmi->ref_clk)) {
		dev_err(hdmi->dev, "failed to get reference clock\n");
		return PTR_ERR(hdmi->ref_clk);
	}

	hdmi->grf_clk = clk_get(hdmi->dev, "grf");
	if (PTR_ERR(hdmi->grf_clk) == -ENOENT) {
		hdmi->grf_clk = NULL;
	} else if (PTR_ERR(hdmi->grf_clk) == -EPROBE_DEFER) {
		return -EPROBE_DEFER;
	} else if (IS_ERR(hdmi->grf_clk)) {
		dev_err(hdmi->dev, "failed to get grf clock\n");
		return PTR_ERR(hdmi->grf_clk);
	}

	hdmi->avdd_0v9 = regulator_get(hdmi->dev, "avdd-0v9");
	if (IS_ERR(hdmi->avdd_0v9))
		return PTR_ERR(hdmi->avdd_0v9);

	hdmi->avdd_1v8 = regulator_get(hdmi->dev, "avdd-1v8");
	if (IS_ERR(hdmi->avdd_1v8))
		return PTR_ERR(hdmi->avdd_1v8);

	return 0;
}

static bool
dw_hdmi_rockchip_mode_valid(struct dw_hdmi *dw_hdmi, void *data,
			    const struct drm_display_info *info,
			    const struct drm_display_mode *mode)
{
	struct rockchip_hdmi *hdmi = data;
	const struct dw_hdmi_mpll_config *mpll_cfg = rockchip_mpll_cfg;
	int pclk = mode->clock * 1000;
	bool exact_match = hdmi->plat_data->phy_force_vendor;
	int i;

	if (hdmi->ref_clk) {
		int rpclk = clk_round_rate(hdmi->ref_clk, pclk);

		if (abs(rpclk - pclk) > pclk / 1000)
			return false;
	}

	for (i = 0; mpll_cfg[i].mpixelclock != (~0UL); i++) {
		/*
		 * For vendor specific phys force an exact match of the pixelclock
		 * to preserve the original behaviour of the driver.
		 */
		if (exact_match && pclk == mpll_cfg[i].mpixelclock)
			return true;
		/*
		 * The Synopsys phy can work with pixelclocks up to the value given
		 * in the corresponding mpll_cfg entry.
		 */
		if (!exact_match && pclk <= mpll_cfg[i].mpixelclock)
			return true;
	}

	return false;
}

static int dw_hdmi_rockchip_mode_set(struct dw_hdmi *dw_hdmi, void *data,
				     const struct drm_display_mode *mode)
{
	struct rockchip_hdmi *hdmi = data;
	long rate;

	rate = clk_round_rate(hdmi->ref_clk, mode->clock * 1000);

	clk_set_rate(hdmi->ref_clk, rate);

	return 0;
}

static struct rockchip_hdmi_chip_data rk3568_chip_data = {
	.lcdsel_grf_reg = -1,
};

static const struct dw_hdmi_plat_data rk3568_hdmi_drv_data = {
	.mode_set   = dw_hdmi_rockchip_mode_set,
	.mode_valid = dw_hdmi_rockchip_mode_valid,
	.mpll_cfg   = rockchip_mpll_cfg,
	.cur_ctr    = rockchip_cur_ctr,
	.phy_config = rockchip_phy_config,
	.phy_data = &rk3568_chip_data,
	.use_drm_infoframe = true,
};

static const struct of_device_id dw_hdmi_rockchip_dt_ids[] = {
	{ .compatible = "rockchip,rk3568-dw-hdmi",
	  .data = &rk3568_hdmi_drv_data
	},
	{},
};
MODULE_DEVICE_TABLE(of, dw_hdmi_rockchip_dt_ids);

static int dw_hdmi_rockchip_probe(struct device *dev)
{
	const struct dw_hdmi_plat_data *data;
	struct dw_hdmi_plat_data *plat_data;
	struct rockchip_hdmi *hdmi;
	int ret;

	data = device_get_match_data(dev);
	if (!data)
		return dev_err_probe(dev, -EINVAL, "No match data\n");

	hdmi = xzalloc(sizeof(*hdmi));

	plat_data = xmemdup(data, sizeof(*data));

	hdmi->dev = dev;
	hdmi->plat_data = plat_data;
	hdmi->chip_data = plat_data->phy_data;
	plat_data->phy_data = hdmi;
	plat_data->priv_data = hdmi;

	ret = rockchip_hdmi_parse_dt(hdmi);
	if (ret)
		return dev_err_probe(dev, ret, "Unable to parse OF data\n");

	hdmi->phy = phy_optional_get(dev, "hdmi");
	if (IS_ERR(hdmi->phy))
		return dev_err_probe(dev, ret, "failed to get phy\n");

	ret = regulator_enable(hdmi->avdd_0v9);
	if (ret) {
		dev_err_probe(dev, ret, "failed to enable avdd0v9\n");
		goto err_avdd_0v9;
	}

	ret = regulator_enable(hdmi->avdd_1v8);
	if (ret) {
		dev_err_probe(dev, ret, "failed to enable avdd1v8\n");
		goto err_avdd_1v8;
	}

	ret = clk_prepare_enable(hdmi->ref_clk);
	if (ret) {
		dev_err_probe(dev, ret, "Failed to enable HDMI reference clock\n");
		goto err_clk;
	}

	if (hdmi->chip_data == &rk3568_chip_data) {
		regmap_write(hdmi->regmap, RK3568_GRF_VO_CON1,
			     HIWORD_UPDATE(RK3568_HDMI_SDAIN_MSK |
					   RK3568_HDMI_SCLIN_MSK,
					   RK3568_HDMI_SDAIN_MSK |
					   RK3568_HDMI_SCLIN_MSK));
	}

	hdmi->hdmi = dw_hdmi_bind(dev, plat_data);

	/*
	 * If dw_hdmi_bind() fails we'll never call dw_hdmi_unbind(),
	 * which would have called the encoder cleanup.  Do it manually.
	 */
	if (IS_ERR(hdmi->hdmi)) {
		ret = PTR_ERR(hdmi->hdmi);
		goto err_bind;
	}

	return 0;

err_bind:
	clk_disable(hdmi->ref_clk);
err_clk:
	regulator_disable(hdmi->avdd_1v8);
err_avdd_1v8:
	regulator_disable(hdmi->avdd_0v9);
err_avdd_0v9:
	return ret;
}

struct driver dw_hdmi_rockchip_driver = {
	.probe  = dw_hdmi_rockchip_probe,
	.name = "dwhdmi-rockchip",
	.of_compatible = dw_hdmi_rockchip_dt_ids,
};
device_platform_driver(dw_hdmi_rockchip_driver);
