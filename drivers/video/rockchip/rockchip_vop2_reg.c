// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) Rockchip Electronics Co.Ltd
 * Author: Andy Yan <andy.yan@rock-chips.com>
 */

#include <linux/kernel.h>
#include <of.h>
#include <driver.h>
#include <video/fourcc.h>

#include "rockchip_drm_vop2.h"

static const uint32_t formats_smart[] = {
	DRM_FORMAT_XRGB8888,
	DRM_FORMAT_ARGB8888,
	DRM_FORMAT_XBGR8888,
	DRM_FORMAT_ABGR8888,
	DRM_FORMAT_RGB888,
	DRM_FORMAT_BGR888,
	DRM_FORMAT_RGB565,
	DRM_FORMAT_BGR565,
};

static const struct vop2_video_port_data rk3568_vop_video_ports[] = {
	{
		.id = 0,
		.feature = VOP2_VP_FEATURE_OUTPUT_10BIT,
		.gamma_lut_len = 1024,
		.cubic_lut_len = 9 * 9 * 9,
		.max_output = { 4096, 2304 },
		.pre_scan_max_dly = { 69, 53, 53, 42 },
		.offset = 0xc00,
	}, {
		.id = 1,
		.gamma_lut_len = 1024,
		.max_output = { 2048, 1536 },
		.pre_scan_max_dly = { 40, 40, 40, 40 },
		.offset = 0xd00,
	}, {
		.id = 2,
		.gamma_lut_len = 1024,
		.max_output = { 1920, 1080 },
		.pre_scan_max_dly = { 40, 40, 40, 40 },
		.offset = 0xe00,
	},
};

/*
 * rk3568 vop with 2 cluster, 2 esmart win, 2 smart win.
 * Every cluster can work as 4K win or split into two win.
 * All win in cluster support AFBCD.
 *
 * Every esmart win and smart win support 4 Multi-region.
 *
 * Scale filter mode:
 *
 * * Cluster:  bicubic for horizontal scale up, others use bilinear
 * * ESmart:
 *    * nearest-neighbor/bilinear/bicubic for scale up
 *    * nearest-neighbor/bilinear/average for scale down
 *
 *
 * @TODO describe the wind like cpu-map dt nodes;
 */
static const struct vop2_win_data rk3568_vop_win_data[] = {
	{
		.name = "Smart0-win0",
		.phys_id = ROCKCHIP_VOP2_SMART0,
		.base = 0x1c00,
		.formats = formats_smart,
		.nformats = ARRAY_SIZE(formats_smart),
		.layer_sel_id = 3,
		.type = DRM_PLANE_TYPE_PRIMARY,
		.max_upscale_factor = 8,
		.max_downscale_factor = 8,
		.dly = { 20, 47, 41 },
	}, {
		.name = "Smart1-win0",
		.phys_id = ROCKCHIP_VOP2_SMART1,
		.formats = formats_smart,
		.nformats = ARRAY_SIZE(formats_smart),
		.base = 0x1e00,
		.layer_sel_id = 7,
		.type = DRM_PLANE_TYPE_PRIMARY,
		.max_upscale_factor = 8,
		.max_downscale_factor = 8,
		.dly = { 20, 47, 41 },
	}, {
		.name = "Esmart1-win0",
		.phys_id = ROCKCHIP_VOP2_ESMART1,
		.formats = formats_smart,
		.nformats = ARRAY_SIZE(formats_smart),
		.base = 0x1a00,
		.layer_sel_id = 6,
		.type = DRM_PLANE_TYPE_PRIMARY,
		.max_upscale_factor = 8,
		.max_downscale_factor = 8,
		.dly = { 20, 47, 41 },
	}, {
		.name = "Esmart0-win0",
		.phys_id = ROCKCHIP_VOP2_ESMART0,
		.formats = formats_smart,
		.nformats = ARRAY_SIZE(formats_smart),
		.base = 0x1800,
		.layer_sel_id = 2,
		.type = DRM_PLANE_TYPE_PRIMARY,
		.max_upscale_factor = 8,
		.max_downscale_factor = 8,
		.dly = { 20, 47, 41 },
	},
};

static const struct vop2_video_port_data rk3588_vop_video_ports[] = {
	{
		.id = 0,
		.feature = VOP2_VP_FEATURE_OUTPUT_10BIT,
		.gamma_lut_len = 1024,
		.cubic_lut_len = 9 * 9 * 9, /* 9x9x9 */
		.max_output = { 4096, 2304 },
		/* hdr2sdr sdr2hdr hdr2hdr sdr2sdr */
		.pre_scan_max_dly = { 76, 65, 65, 54 },
		.offset = 0xc00,
	}, {
		.id = 1,
		.feature = VOP2_VP_FEATURE_OUTPUT_10BIT,
		.gamma_lut_len = 1024,
		.cubic_lut_len = 729, /* 9x9x9 */
		.max_output = { 4096, 2304 },
		.pre_scan_max_dly = { 76, 65, 65, 54 },
		.offset = 0xd00,
	}, {
		.id = 2,
		.feature = VOP2_VP_FEATURE_OUTPUT_10BIT,
		.gamma_lut_len = 1024,
		.cubic_lut_len = 17 * 17 * 17, /* 17x17x17 */
		.max_output = { 4096, 2304 },
		.pre_scan_max_dly = { 52, 52, 52, 52 },
		.offset = 0xe00,
	}, {
		.id = 3,
		.gamma_lut_len = 1024,
		.max_output = { 2048, 1536 },
		.pre_scan_max_dly = { 52, 52, 52, 52 },
		.offset = 0xf00,
	},
};

/*
 * rk3588 vop with 4 cluster, 4 esmart win.
 * Every cluster can work as 4K win or split into two win.
 * All win in cluster support AFBCD.
 *
 * Every esmart win and smart win support 4 Multi-region.
 *
 * Scale filter mode:
 *
 * * Cluster:  bicubic for horizontal scale up, others use bilinear
 * * ESmart:
 *    * nearest-neighbor/bilinear/bicubic for scale up
 *    * nearest-neighbor/bilinear/average for scale down
 *
 * AXI Read ID assignment:
 * Two AXI bus:
 * AXI0 is a read/write bus with a higher performance.
 * AXI1 is a read only bus.
 *
 * Every window on a AXI bus must assigned two unique
 * read id(yrgb_id/uv_id, valid id are 0x1~0xe).
 *
 * AXI0:
 * Cluster0/1, Esmart0/1, WriteBack
 *
 * AXI 1:
 * Cluster2/3, Esmart2/3
 *
 */
static const struct vop2_win_data rk3588_vop_win_data[] = {
	{
		.name = "Esmart0-win0",
		.phys_id = ROCKCHIP_VOP2_ESMART0,
		.formats = formats_smart,
		.nformats = ARRAY_SIZE(formats_smart),
		.base = 0x1800,
		.layer_sel_id = 2,
		.type = DRM_PLANE_TYPE_OVERLAY,
		.max_upscale_factor = 8,
		.max_downscale_factor = 8,
		.dly = { 23, 45, 48 },
	}, {
		.name = "Esmart1-win0",
		.phys_id = ROCKCHIP_VOP2_ESMART1,
		.formats = formats_smart,
		.nformats = ARRAY_SIZE(formats_smart),
		.base = 0x1a00,
		.layer_sel_id = 3,
		.type = DRM_PLANE_TYPE_OVERLAY,
		.max_upscale_factor = 8,
		.max_downscale_factor = 8,
		.dly = { 23, 45, 48 },
	}, {
		.name = "Esmart2-win0",
		.phys_id = ROCKCHIP_VOP2_ESMART2,
		.base = 0x1c00,
		.formats = formats_smart,
		.nformats = ARRAY_SIZE(formats_smart),
		.layer_sel_id = 6,
		.type = DRM_PLANE_TYPE_OVERLAY,
		.max_upscale_factor = 8,
		.max_downscale_factor = 8,
		.dly = { 23, 45, 48 },
	}, {
		.name = "Esmart3-win0",
		.phys_id = ROCKCHIP_VOP2_ESMART3,
		.formats = formats_smart,
		.nformats = ARRAY_SIZE(formats_smart),
		.base = 0x1e00,
		.layer_sel_id = 7,
		.type = DRM_PLANE_TYPE_OVERLAY,
		.max_upscale_factor = 8,
		.max_downscale_factor = 8,
		.dly = { 23, 45, 48 },
	},
};

static const struct vop2_data rk3566_vop = {
	.feature = VOP2_FEATURE_HAS_SYS_GRF,
	.nr_vps = 3,
	.max_input = { 4096, 2304 },
	.max_output = { 4096, 2304 },
	.vp = rk3568_vop_video_ports,
	.win = rk3568_vop_win_data,
	.win_size = ARRAY_SIZE(rk3568_vop_win_data),
	.soc_id = 3566,
};

static const struct vop2_data rk3568_vop = {
	.feature = VOP2_FEATURE_HAS_SYS_GRF,
	.nr_vps = 3,
	.max_input = { 4096, 2304 },
	.max_output = { 4096, 2304 },
	.vp = rk3568_vop_video_ports,
	.win = rk3568_vop_win_data,
	.win_size = ARRAY_SIZE(rk3568_vop_win_data),
	.soc_id = 3568,
};

static const struct vop2_data rk3588_vop = {
	.feature = VOP2_FEATURE_HAS_SYS_GRF | VOP2_FEATURE_HAS_VO1_GRF |
		   VOP2_FEATURE_HAS_VOP_GRF | VOP2_FEATURE_HAS_SYS_PMU,
	.nr_vps = 4,
	.max_input = { 4096, 4320 },
	.max_output = { 4096, 4320 },
	.vp = rk3588_vop_video_ports,
	.win = rk3588_vop_win_data,
	.win_size = ARRAY_SIZE(rk3588_vop_win_data),
	.soc_id = 3588,
};

static const struct of_device_id vop2_dt_match[] = {
	{
		.compatible = "rockchip,rk3566-vop",
		.data = &rk3566_vop,
	}, {
		.compatible = "rockchip,rk3568-vop",
		.data = &rk3568_vop,
	}, {
		.compatible = "rockchip,rk3588-vop",
		.data = &rk3588_vop
	}, {
	},
};
MODULE_DEVICE_TABLE(of, vop2_dt_match);

struct driver vop2_driver = {
	.probe = vop2_bind,
	.name = "rockchip-vop2",
	.of_compatible = vop2_dt_match,
};
device_platform_driver(vop2_driver);
