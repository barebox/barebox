/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) STMicroelectronics SA 2017
 *
 * Authors: Philippe Cornu <philippe.cornu@st.com>
 *          Yannick Fertre <yannick.fertre@st.com>
 */

#ifndef __DW_MIPI_DSI__
#define __DW_MIPI_DSI__

#include <linux/types.h>
#include <linux/compiler.h>

#include <video/drm/drm_modes.h>

struct drm_display_mode;
struct dw_mipi_dsi;
struct mipi_dsi_device;
struct device;

/**
 * struct dw_mipi_dsi_dphy_timing - DSI host phy timings
 * @data_hs2lp: High Speed to Low Speed Data Transition Time
 * @data_lp2hs: Low Speed to High Speed Data Transition Time
 * @clk_hs2lp: High Speed to Low Speed Clock Transition Time
 * @clk_lp2hs: Low Speed to High Speed Clock Transition Time
 */
struct dw_mipi_dsi_dphy_timing {
	u16 data_hs2lp;
	u16 data_lp2hs;
	u16 clk_hs2lp;
	u16 clk_lp2hs;
};

struct dw_mipi_dsi_phy_ops {
	int (*init)(void *priv_data);
	void (*power_on)(void *priv_data);
	void (*power_off)(void *priv_data);
	int (*get_lane_mbps)(void *priv_data,
			     const struct drm_display_mode *mode,
			     unsigned long mode_flags, u32 lanes, u32 format,
			     unsigned int *lane_mbps);
	int (*get_timing)(void *priv_data, unsigned int lane_mbps,
			  struct dw_mipi_dsi_dphy_timing *timing);
	int (*get_esc_clk_rate)(void *priv_data, unsigned int *esc_clk_rate);
};

struct dw_mipi_dsi_host_ops {
	int (*attach)(void *priv_data,
		      struct mipi_dsi_device *dsi);
	int (*detach)(void *priv_data,
		      struct mipi_dsi_device *dsi);
};

struct dw_mipi_dsi_plat_data {
	void __iomem *base;
	unsigned int max_data_lanes;

	enum drm_mode_status (*mode_valid)(void *priv_data,
					   const struct drm_display_mode *mode,
					   unsigned long mode_flags,
					   u32 lanes, u32 format);

	bool (*mode_fixup)(void *priv_data, const struct drm_display_mode *mode,
			   struct drm_display_mode *adjusted_mode);

	const struct dw_mipi_dsi_phy_ops *phy_ops;
	const struct dw_mipi_dsi_host_ops *host_ops;

	void *priv_data;
};

struct dw_mipi_dsi *
dw_mipi_dsi_bind(struct device *dev,
		 const struct dw_mipi_dsi_plat_data *plat_data);
static inline void dw_mipi_dsi_unbind(struct dw_mipi_dsi *dsi) { }

struct device_node *dw_mipi_dsi_crtc_node(struct dw_mipi_dsi *dsi);

#endif /* __DW_MIPI_DSI__ */
