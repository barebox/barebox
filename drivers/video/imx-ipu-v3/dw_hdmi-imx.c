// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2011-2013 Freescale Semiconductor, Inc.
 *
 * derived from imx-hdmi.c(renamed to bridge/dw_hdmi.c now)
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
#include <video/media-bus-format.h>
#include <mfd/imx6q-iomuxc-gpr.h>

#include "imx-ipu-v3.h"

struct imx_hdmi {
	struct device *dev;
	struct dw_hdmi *hdmi;
	struct regmap *regmap;
};

static const struct dw_hdmi_mpll_config imx_mpll_cfg[] = {
	{
		45250000, {
			{ 0x01e0, 0x0000 },
			{ 0x21e1, 0x0000 },
			{ 0x41e2, 0x0000 }
		},
	}, {
		92500000, {
			{ 0x0140, 0x0005 },
			{ 0x2141, 0x0005 },
			{ 0x4142, 0x0005 },
	},
	}, {
		148500000, {
			{ 0x00a0, 0x000a },
			{ 0x20a1, 0x000a },
			{ 0x40a2, 0x000a },
		},
	}, {
		216000000, {
			{ 0x00a0, 0x000a },
			{ 0x2001, 0x000f },
			{ 0x4002, 0x000f },
		},
	}, {
		~0UL, {
			{ 0x0000, 0x0000 },
			{ 0x0000, 0x0000 },
			{ 0x0000, 0x0000 },
		},
	}
};

static const struct dw_hdmi_curr_ctrl imx_cur_ctr[] = {
	/*      pixelclk     bpp8    bpp10   bpp12 */
	{
		54000000, { 0x091c, 0x091c, 0x06dc },
	}, {
		58400000, { 0x091c, 0x06dc, 0x06dc },
	}, {
		72000000, { 0x06dc, 0x06dc, 0x091c },
	}, {
		74250000, { 0x06dc, 0x0b5c, 0x091c },
	}, {
		118800000, { 0x091c, 0x091c, 0x06dc },
	}, {
		216000000, { 0x06dc, 0x0b5c, 0x091c },
	}, {
		~0UL, { 0x0000, 0x0000, 0x0000 },
	},
};

/*
 * Resistance term 133Ohm Cfg
 * PREEMP config 0.00
 * TX/CK level 10
 */
static const struct dw_hdmi_phy_config imx_phy_config[] = {
	/*pixelclk   symbol   term   vlev */
	{ 216000000, 0x800d, 0x0005, 0x01ad},
	{ ~0UL,      0x0000, 0x0000, 0x0000}
};

#define IMX6Q_GPR3_HDMI_MUX_CTL_SHIFT		2
#define IMX6Q_GPR3_HDMI_MUX_CTL_MASK		(0x3 << 2)

static void dw_hdmi_set_ipu_di_mux(struct imx_hdmi *hdmi, int ipu_di)
{
	regmap_update_bits(hdmi->regmap, IOMUXC_GPR3,
			   IMX6Q_GPR3_HDMI_MUX_CTL_MASK,
			   ipu_di << IMX6Q_GPR3_HDMI_MUX_CTL_SHIFT);
}

static int dw_hdmi_imx_vpl_ioctl(struct dw_hdmi *unused, void *data,
				 unsigned int port, unsigned int cmd, void *cmddata)
{
	struct imx_hdmi *hdmi = data;
	struct ipu_di_mode *mode;

	switch (cmd) {
	case VPL_PREPARE:
		dw_hdmi_set_ipu_di_mux(hdmi, port);
		return 0;
	case IMX_IPU_VPL_DI_MODE:
		mode = cmddata;

		mode->di_clkflags = IPU_DI_CLKMODE_EXT | IPU_DI_CLKMODE_SYNC;
		mode->bus_format = MEDIA_BUS_FMT_RGB888_1X24;

		return 0;
	}

	return 0;
}

static bool
imx6q_hdmi_mode_valid(struct dw_hdmi *hdmi, void *data,
		      const struct drm_display_info *info,
		      const struct drm_display_mode *mode)
{
	if (mode->clock < 13500)
		return false;
	/* FIXME: Hardware is capable of 266MHz, but setup data is missing. */
	if (mode->clock > 216000)
		return false;

	return true;
}

static bool
imx6dl_hdmi_mode_valid(struct dw_hdmi *hdmi, void *data,
		       const struct drm_display_info *info,
		       const struct drm_display_mode *mode)
{
	if (mode->clock < 13500)
		return false;
	/* FIXME: Hardware is capable of 270MHz, but setup data is missing. */
	if (mode->clock > 216000)
		return false;

	return true;
}

static struct dw_hdmi_plat_data imx6q_hdmi_drv_data = {
	.mpll_cfg   = imx_mpll_cfg,
	.cur_ctr    = imx_cur_ctr,
	.phy_config = imx_phy_config,
	.mode_valid = imx6q_hdmi_mode_valid,
	.vpl_ioctl  = dw_hdmi_imx_vpl_ioctl,
};

static struct dw_hdmi_plat_data imx6dl_hdmi_drv_data = {
	.mpll_cfg = imx_mpll_cfg,
	.cur_ctr  = imx_cur_ctr,
	.phy_config = imx_phy_config,
	.mode_valid = imx6dl_hdmi_mode_valid,
	.vpl_ioctl  = dw_hdmi_imx_vpl_ioctl,
};

static const struct of_device_id dw_hdmi_imx_dt_ids[] = {
	{ .compatible = "fsl,imx6q-hdmi",
	  .data = &imx6q_hdmi_drv_data
	}, {
	  .compatible = "fsl,imx6dl-hdmi",
	  .data = &imx6dl_hdmi_drv_data
	},
	{},
};
MODULE_DEVICE_TABLE(of, dw_hdmi_imx_dt_ids);

static int dw_hdmi_imx_probe(struct device *dev)
{
	struct dw_hdmi_plat_data *plat_data;
	struct device_node *np = dev->of_node;
	struct imx_hdmi *hdmi;
	int ret;

	hdmi = xzalloc(sizeof(*hdmi));

	ret = dev_get_drvdata(dev, (const void **)&plat_data);
	if (ret)
		return ret;

	plat_data = xmemdup(plat_data, sizeof(*plat_data));
	plat_data->phy_data = hdmi;
	plat_data->priv_data = hdmi;

	hdmi->dev = dev;

	hdmi->regmap = syscon_regmap_lookup_by_phandle(np, "gpr");
	if (IS_ERR(hdmi->regmap)) {
		dev_err(hdmi->dev, "Unable to get gpr\n");
		return PTR_ERR(hdmi->regmap);
	}

	hdmi->hdmi = dw_hdmi_bind(dev, plat_data);
	if (IS_ERR(hdmi->hdmi))
		ret = PTR_ERR(hdmi->hdmi);

	return ret;
}

struct driver dw_hdmi_imx_platform_driver = {
	.probe  = dw_hdmi_imx_probe,
	.name = "dwhdmi-imx",
	.of_compatible = dw_hdmi_imx_dt_ids,
};
device_platform_driver(dw_hdmi_imx_platform_driver);

MODULE_AUTHOR("Andy Yan <andy.yan@rock-chips.com>");
MODULE_AUTHOR("Yakir Yang <ykk@rock-chips.com>");
MODULE_DESCRIPTION("IMX6 Specific DW-HDMI Driver Extension");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:dwhdmi-imx");
