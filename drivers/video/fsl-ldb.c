// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 Marek Vasut <marex@denx.de>
 *
 * Limitations:
 * - ldb is currently hardcoded to MEDIA_BUS_FMT_RGB888_1X7X4_SPWG
 */

#include <linux/clk.h>
#include <video/media-bus-format.h>
#include <mfd/syscon.h>
#include <linux/module.h>
#include <of.h>
#include <of_graph.h>
#include <regmap.h>

#include <video/vpl.h>

#define LDB_CTRL_CH0_ENABLE			BIT(0)
#define LDB_CTRL_CH0_DI_SELECT			BIT(1)
#define LDB_CTRL_CH1_ENABLE			BIT(2)
#define LDB_CTRL_CH1_DI_SELECT			BIT(3)
#define LDB_CTRL_SPLIT_MODE			BIT(4)
#define LDB_CTRL_CH0_DATA_WIDTH			BIT(5)
#define LDB_CTRL_CH0_BIT_MAPPING		BIT(6)
#define LDB_CTRL_CH1_DATA_WIDTH			BIT(7)
#define LDB_CTRL_CH1_BIT_MAPPING		BIT(8)
#define LDB_CTRL_DI0_VSYNC_POLARITY		BIT(9)
#define LDB_CTRL_DI1_VSYNC_POLARITY		BIT(10)
#define LDB_CTRL_REG_CH0_FIFO_RESET		BIT(11)
#define LDB_CTRL_REG_CH1_FIFO_RESET		BIT(12)
#define LDB_CTRL_ASYNC_FIFO_ENABLE		BIT(24)
#define LDB_CTRL_ASYNC_FIFO_THRESHOLD_MASK	GENMASK(27, 25)

#define LVDS_CTRL_CH0_EN			BIT(0)
#define LVDS_CTRL_CH1_EN			BIT(1)
/*
 * LVDS_CTRL_LVDS_EN bit is poorly named in i.MX93 reference manual.
 * Clear it to enable LVDS and set it to disable LVDS.
 */
#define LVDS_CTRL_LVDS_EN			BIT(1)
#define LVDS_CTRL_VBG_EN			BIT(2)
#define LVDS_CTRL_HS_EN				BIT(3)
#define LVDS_CTRL_PRE_EMPH_EN			BIT(4)
#define LVDS_CTRL_PRE_EMPH_ADJ(n)		(((n) & 0x7) << 5)
#define LVDS_CTRL_PRE_EMPH_ADJ_MASK		GENMASK(7, 5)
#define LVDS_CTRL_CM_ADJ(n)			(((n) & 0x7) << 8)
#define LVDS_CTRL_CM_ADJ_MASK			GENMASK(10, 8)
#define LVDS_CTRL_CC_ADJ(n)			(((n) & 0x7) << 11)
#define LVDS_CTRL_CC_ADJ_MASK			GENMASK(13, 11)
#define LVDS_CTRL_SLEW_ADJ(n)			(((n) & 0x7) << 14)
#define LVDS_CTRL_SLEW_ADJ_MASK			GENMASK(16, 14)
#define LVDS_CTRL_VBG_ADJ(n)			(((n) & 0x7) << 17)
#define LVDS_CTRL_VBG_ADJ_MASK			GENMASK(19, 17)

enum fsl_ldb_devtype {
	IMX6SX_LDB,
	IMX8MP_LDB,
	IMX93_LDB,
};

struct fsl_ldb_devdata {
	u32 ldb_ctrl;
	u32 lvds_ctrl;
	bool lvds_en_bit;
	bool single_ctrl_reg;
};

static const struct fsl_ldb_devdata fsl_ldb_devdata[] = {
	[IMX6SX_LDB] = {
		.ldb_ctrl = 0x18,
		.single_ctrl_reg = true,
	},
	[IMX8MP_LDB] = {
		.ldb_ctrl = 0x5c,
		.lvds_ctrl = 0x128,
	},
	[IMX93_LDB] = {
		.ldb_ctrl = 0x20,
		.lvds_ctrl = 0x24,
		.lvds_en_bit = true,
	},
};

struct fsl_ldb {
	struct device *dev;
	struct clk *clk;
	struct vpl vpl;
	struct regmap *regmap;
	u32 bus_format;
	const struct fsl_ldb_devdata *devdata;
	struct display_timings *modes;
	int output_port;
	bool ch0_enabled;
	bool ch1_enabled;
};

static bool fsl_ldb_is_dual(const struct fsl_ldb *fsl_ldb)
{
	return (fsl_ldb->ch0_enabled && fsl_ldb->ch1_enabled);
}

static unsigned long fsl_ldb_link_frequency(struct fsl_ldb *fsl_ldb, int clock)
{
	if (fsl_ldb_is_dual(fsl_ldb))
		return clock * 3500;
	else
		return clock * 7000;
}

static void fsl_ldb_atomic_enable(struct fsl_ldb *fsl_ldb,
				  struct fb_videomode *mode)
{
	unsigned long configured_link_freq;
	unsigned long requested_link_freq;
	bool lvds_format_24bpp;
	bool lvds_format_jeida;
	u32 reg;

	switch (fsl_ldb->bus_format) {
	case MEDIA_BUS_FMT_RGB666_1X7X3_SPWG:
		lvds_format_24bpp = false;
		lvds_format_jeida = true;
		break;
	case MEDIA_BUS_FMT_RGB888_1X7X4_JEIDA:
		lvds_format_24bpp = true;
		lvds_format_jeida = true;
		break;
	case MEDIA_BUS_FMT_RGB888_1X7X4_SPWG:
		lvds_format_24bpp = true;
		lvds_format_jeida = false;
		break;
	default:
		/*
		 * Some bridges still don't set the correct LVDS bus pixel
		 * format, use SPWG24 default format until those are fixed.
		 */
		lvds_format_24bpp = true;
		lvds_format_jeida = false;
		dev_warn(fsl_ldb->dev,
			 "Unsupported LVDS bus format 0x%04x, please check output bridge driver. Falling back to SPWG24.\n",
			 fsl_ldb->bus_format);
		break;
	}

	if (mode->pixclock.ps > (fsl_ldb_is_dual(fsl_ldb) ? 160000 : 80000))
		return;

	requested_link_freq = fsl_ldb_link_frequency(fsl_ldb, PICOS2KHZ(mode->pixclock));
	clk_set_rate(fsl_ldb->clk, requested_link_freq);

	configured_link_freq = clk_get_rate(fsl_ldb->clk);
	if (configured_link_freq != requested_link_freq)
		dev_warn(fsl_ldb->dev, "Configured LDB clock (%lu Hz) does not match requested LVDS clock: %lu Hz\n",
			 configured_link_freq,
			 requested_link_freq);

	clk_prepare_enable(fsl_ldb->clk);

	/* Program LDB_CTRL */
	reg =	(fsl_ldb->ch0_enabled ? LDB_CTRL_CH0_ENABLE : 0) |
		(fsl_ldb->ch1_enabled ? LDB_CTRL_CH1_ENABLE : 0) |
		(fsl_ldb_is_dual(fsl_ldb) ? LDB_CTRL_SPLIT_MODE : 0);

	if (lvds_format_24bpp)
		reg |=	(fsl_ldb->ch0_enabled ? LDB_CTRL_CH0_DATA_WIDTH : 0) |
			(fsl_ldb->ch1_enabled ? LDB_CTRL_CH1_DATA_WIDTH : 0);

	if (lvds_format_jeida)
		reg |=	(fsl_ldb->ch0_enabled ? LDB_CTRL_CH0_BIT_MAPPING : 0) |
			(fsl_ldb->ch1_enabled ? LDB_CTRL_CH1_BIT_MAPPING : 0);

	if (mode->sync & FB_SYNC_VERT_HIGH_ACT)
		reg |=	(fsl_ldb->ch0_enabled ? LDB_CTRL_DI0_VSYNC_POLARITY : 0) |
			(fsl_ldb->ch1_enabled ? LDB_CTRL_DI1_VSYNC_POLARITY : 0);

	regmap_write(fsl_ldb->regmap, fsl_ldb->devdata->ldb_ctrl, reg);

	if (fsl_ldb->devdata->single_ctrl_reg)
		return;

	/* Program LVDS_CTRL */
	reg = LVDS_CTRL_CC_ADJ(2) | LVDS_CTRL_PRE_EMPH_EN |
	      LVDS_CTRL_PRE_EMPH_ADJ(3) | LVDS_CTRL_VBG_EN;
	regmap_write(fsl_ldb->regmap, fsl_ldb->devdata->lvds_ctrl, reg);

	/* Wait for VBG to stabilize. */
	udelay(20);

	reg |=	(fsl_ldb->ch0_enabled ? LVDS_CTRL_CH0_EN : 0) |
		(fsl_ldb->ch1_enabled ? LVDS_CTRL_CH1_EN : 0);

	regmap_write(fsl_ldb->regmap, fsl_ldb->devdata->lvds_ctrl, reg);
}

static void fsl_ldb_atomic_disable(struct fsl_ldb *fsl_ldb)
{
	/* Stop channel(s). */
	if (fsl_ldb->devdata->lvds_en_bit)
		/* Set LVDS_CTRL_LVDS_EN bit to disable. */
		regmap_write(fsl_ldb->regmap, fsl_ldb->devdata->lvds_ctrl,
			     LVDS_CTRL_LVDS_EN);
	else
		if (!fsl_ldb->devdata->single_ctrl_reg)
			regmap_write(fsl_ldb->regmap, fsl_ldb->devdata->lvds_ctrl, 0);
	regmap_write(fsl_ldb->regmap, fsl_ldb->devdata->ldb_ctrl, 0);

	clk_disable_unprepare(fsl_ldb->clk);
}

static int fsl_ldb_ioctl(struct vpl *vpl, unsigned int port,
		unsigned int cmd, void *data)
{
	struct fsl_ldb *fsl_ldb = container_of(vpl, struct fsl_ldb, vpl);

	switch (cmd) {
	case VPL_ENABLE:
		break;
	case VPL_DISABLE:
		break;
	case VPL_UNPREPARE:
		fsl_ldb_atomic_disable(fsl_ldb);
		break;
	case VPL_PREPARE:
		fsl_ldb_atomic_enable(fsl_ldb, data);
		break;
	case VPL_GET_VIDEOMODES:
		if (fsl_ldb->modes) {
			struct display_timings *timings = data;

			timings->num_modes = fsl_ldb->modes->num_modes;
			timings->modes = fsl_ldb->modes->modes;
			dev_info(fsl_ldb->dev, "Using ldb provided timings\n");
			return 0;
		}
	}

	if (fsl_ldb->output_port > 0)
		return vpl_ioctl(vpl, fsl_ldb->output_port, cmd, data);

	return 0;
}

static int fsl_ldb_probe(struct device *dev)
{
	struct device_node *remote1, *remote2;
	struct device_node *panel_node;
	struct fsl_ldb *fsl_ldb;

	fsl_ldb = xzalloc(sizeof(*fsl_ldb));
	if (!fsl_ldb)
		return -ENOMEM;

	fsl_ldb->devdata = device_get_match_data(dev);
	if (!fsl_ldb->devdata)
		return -EINVAL;

	fsl_ldb->dev = dev;

	fsl_ldb->clk = clk_get(dev, "ldb");
	if (IS_ERR(fsl_ldb->clk))
		return PTR_ERR(fsl_ldb->clk);

	fsl_ldb->regmap = syscon_node_to_regmap(dev->of_node->parent);
	if (IS_ERR(fsl_ldb->regmap))
		return PTR_ERR(fsl_ldb->regmap);

	/* Locate the remote ports and the panel node */
	remote1 = of_graph_get_remote_node(dev->of_node, 1, 0);
	remote2 = of_graph_get_remote_node(dev->of_node, 2, 0);
	panel_node = of_node_get(remote1 ? remote1 : remote2);
	fsl_ldb->ch0_enabled = (remote1 != NULL);
	fsl_ldb->ch1_enabled = (remote2 != NULL);

	if (!fsl_ldb->ch0_enabled && !fsl_ldb->ch1_enabled)
		return dev_err_probe(dev, -ENXIO, "No panel node found");

	fsl_ldb->modes = of_get_display_timings(dev->of_node);

	/* Only MEDIA_BUS_FMT_RGB888_1X7X4_SPWG is supported for now */
	fsl_ldb->bus_format = MEDIA_BUS_FMT_RGB888_1X7X4_SPWG;
	fsl_ldb->output_port = remote1 ? 1 : remote2 ? 2 : 0;

	dev_dbg(dev, "Using %s\n",
		fsl_ldb_is_dual(fsl_ldb) ? "dual-link mode" :
		fsl_ldb->ch0_enabled ? "channel 0" : "channel 1");

	/* Only DRM_LVDS_DUAL_LINK_ODD_EVEN_PIXELS is supported */

	dev_info(dev, "LVDS channel pixel swap not supported.\n");

	fsl_ldb->vpl.node = dev->of_node;
	fsl_ldb->vpl.ioctl = &fsl_ldb_ioctl;
	return vpl_register(&fsl_ldb->vpl);
}

static const struct of_device_id fsl_ldb_match[] = {
	{ .compatible = "fsl,imx6sx-ldb",
	  .data = &fsl_ldb_devdata[IMX6SX_LDB], },
	{ .compatible = "fsl,imx8mp-ldb",
	  .data = &fsl_ldb_devdata[IMX8MP_LDB], },
	{ .compatible = "fsl,imx93-ldb",
	  .data = &fsl_ldb_devdata[IMX93_LDB], },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, fsl_ldb_match);

static struct driver fsl_ldb_driver = {
	.probe	= fsl_ldb_probe,
	.name		= "fsl-ldb",
	.of_compatible	= fsl_ldb_match,
};
device_platform_driver(fsl_ldb_driver);
