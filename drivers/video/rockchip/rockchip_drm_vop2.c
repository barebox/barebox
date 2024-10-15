// SPDX-License-Identifier: (GPL-2.0+ OR MIT)
/*
 * Copyright (c) 2020 Rockchip Electronics Co., Ltd.
 * Author: Andy Yan <andy.yan@rock-chips.com>
 */

#include <linux/bitfield.h>
#include <linux/clk.h>
#include <linux/iopoll.h>
#include <linux/kernel.h>
#include <of_graph.h>
#include <linux/regmap.h>
#include <linux/swab.h>
#include <video/fourcc.h>
#include <linux/printk.h>
#include <video/media-bus-format.h>
#include <fb.h>
#include <dma.h>
#include <linux/log2.h>
#include <stdio.h>
#include <mfd/syscon.h>
#include <video/drm/drm_connector.h>
#include <video/vpl.h>
#include <video/videomode.h>

#include "rockchip_drm_vop2.h"
#include "rockchip_drm_drv.h"
#include <dt-bindings/soc/rockchip,vop2.h>

/*
 * VOP2 architecture
 *
 +----------+   +-------------+                                                        +-----------+
 |  Cluster |   | Sel 1 from 6|                                                        | 1 from 3  |
 |  window0 |   |    Layer0   |                                                        |    RGB    |
 +----------+   +-------------+              +---------------+    +-------------+      +-----------+
 +----------+   +-------------+              |N from 6 layers|    |             |
 |  Cluster |   | Sel 1 from 6|              |   Overlay0    +--->| Video Port0 |      +-----------+
 |  window1 |   |    Layer1   |              |               |    |             |      | 1 from 3  |
 +----------+   +-------------+              +---------------+    +-------------+      |   LVDS    |
 +----------+   +-------------+                                                        +-----------+
 |  Esmart  |   | Sel 1 from 6|
 |  window0 |   |   Layer2    |              +---------------+    +-------------+      +-----------+
 +----------+   +-------------+              |N from 6 Layers|    |             | +--> | 1 from 3  |
 +----------+   +-------------+   -------->  |   Overlay1    +--->| Video Port1 |      |   MIPI    |
 |  Esmart  |   | Sel 1 from 6|   -------->  |               |    |             |      +-----------+
 |  Window1 |   |   Layer3    |              +---------------+    +-------------+
 +----------+   +-------------+                                                        +-----------+
 +----------+   +-------------+                                                        | 1 from 3  |
 |  Smart   |   | Sel 1 from 6|              +---------------+    +-------------+      |   HDMI    |
 |  Window0 |   |    Layer4   |              |N from 6 Layers|    |             |      +-----------+
 +----------+   +-------------+              |   Overlay2    +--->| Video Port2 |
 +----------+   +-------------+              |               |    |             |      +-----------+
 |  Smart   |   | Sel 1 from 6|              +---------------+    +-------------+      |  1 from 3 |
 |  Window1 |   |    Layer5   |                                                        |    eDP    |
 +----------+   +-------------+                                                        +-----------+
 *
 */

enum vop2_data_format {
	VOP2_FMT_ARGB8888 = 0,
	VOP2_FMT_RGB888,
	VOP2_FMT_RGB565,
	VOP2_FMT_XRGB101010,
	VOP2_FMT_YUV420SP,
	VOP2_FMT_YUV422SP,
	VOP2_FMT_YUV444SP,
	VOP2_FMT_YUYV422 = 8,
	VOP2_FMT_YUYV420,
	VOP2_FMT_VYUY422,
	VOP2_FMT_VYUY420,
	VOP2_FMT_YUV420SP_TILE_8x4 = 0x10,
	VOP2_FMT_YUV420SP_TILE_16x2,
	VOP2_FMT_YUV422SP_TILE_8x4,
	VOP2_FMT_YUV422SP_TILE_16x2,
	VOP2_FMT_YUV420SP_10,
	VOP2_FMT_YUV422SP_10,
	VOP2_FMT_YUV444SP_10,
};

enum vop2_afbc_format {
	VOP2_AFBC_FMT_RGB565,
	VOP2_AFBC_FMT_ARGB2101010 = 2,
	VOP2_AFBC_FMT_YUV420_10BIT,
	VOP2_AFBC_FMT_RGB888,
	VOP2_AFBC_FMT_ARGB8888,
	VOP2_AFBC_FMT_YUV420 = 9,
	VOP2_AFBC_FMT_YUV422 = 0xb,
	VOP2_AFBC_FMT_YUV422_10BIT = 0xe,
	VOP2_AFBC_FMT_INVALID = -1,
};

union vop2_alpha_ctrl {
	u32 val;
	struct {
		/* [0:1] */
		u32 color_mode:1;
		u32 alpha_mode:1;
		/* [2:3] */
		u32 blend_mode:2;
		u32 alpha_cal_mode:1;
		/* [5:7] */
		u32 factor_mode:3;
		/* [8:9] */
		u32 alpha_en:1;
		u32 src_dst_swap:1;
		u32 reserved:6;
		/* [16:23] */
		u32 glb_alpha:8;
	} bits;
};

struct vop2_alpha {
	union vop2_alpha_ctrl src_color_ctrl;
	union vop2_alpha_ctrl dst_color_ctrl;
	union vop2_alpha_ctrl src_alpha_ctrl;
	union vop2_alpha_ctrl dst_alpha_ctrl;
};

struct vop2_alpha_config {
	bool src_premulti_en;
	bool dst_premulti_en;
	bool src_pixel_alpha_en;
	bool dst_pixel_alpha_en;
	u16 src_glb_alpha_value;
	u16 dst_glb_alpha_value;
};

struct vop2_video_port;

struct vop2_win {
	struct vop2 *vop2;
	struct vop2_video_port *vp;
	const struct vop2_win_data *data;
	struct regmap_field *reg[VOP2_WIN_MAX_REG];
	struct regmap *map;
	char *name;

	struct reg_field *reg_field;
	u32 regs[0x100 / sizeof(u32)];

	u8 delay;
	u32 offset;

	enum drm_plane_type type;

	struct list_head list;

	int zpos;

	struct fb_info info;
	dma_addr_t dma;
	u32 alpha;
	u32 pixel_blend_mode;
	bool enabled;
	struct fb_rect src;
	struct fb_rect dst;
};

struct vop2_video_port {
	struct vop2 *vop2;
	struct clk *dclk;
	unsigned int id;
	const struct vop2_video_port_data *data;

	/**
	 * @win_mask: Bitmask of windows attached to the video port;
	 */
	u32 win_mask;

	struct vop2_win *primary_plane;
	struct drm_pending_vblank_event *event;

	unsigned int nlayers;

	struct device_node *port;

	struct fb_videomode *modes;
	struct vpl vpl;

	struct list_head windows;
	u32 line_length;
	u32 max_yres;
	int crtc_endpoint_id;
};

struct vop2 {
	struct device *dev;
	struct vop2_video_port vps[4];

	const struct vop2_data *data;
	/*
	 * Number of windows that are registered as plane, may be less than the
	 * total number of hardware windows.
	 */
	u32 registered_num_wins;

	void __iomem *regs;
	struct regmap *map;

	struct regmap *sys_grf;
	struct regmap *vop_grf;
	struct regmap *vo1_grf;
	struct regmap *sys_pmu;

	/* physical map length of vop2 register */
	u32 len;

	void __iomem *lut_regs;

	int irq;

	/*
	 * Some global resources are shared between all video ports(crtcs), so
	 * we need a ref counter here.
	 */
	unsigned int enable_count;
	struct clk *hclk;
	struct clk *aclk;
	struct clk *pclk;

	/* optional internal rgb encoder */
	struct rockchip_rgb *rgb;

	/* must be put at the end of the struct */
	struct vop2_win win[];
};

#define vop2_output_if_is_hdmi(x)	((x) == ROCKCHIP_VOP2_EP_HDMI0 || \
					 (x) == ROCKCHIP_VOP2_EP_HDMI1)

#define vop2_output_if_is_dp(x)		((x) == ROCKCHIP_VOP2_EP_DP0 || \
					 (x) == ROCKCHIP_VOP2_EP_DP1)

#define vop2_output_if_is_edp(x)	((x) == ROCKCHIP_VOP2_EP_EDP0 || \
					 (x) == ROCKCHIP_VOP2_EP_EDP1)

#define vop2_output_if_is_mipi(x)	((x) == ROCKCHIP_VOP2_EP_MIPI0 || \
					 (x) == ROCKCHIP_VOP2_EP_MIPI1)

#define vop2_output_if_is_lvds(x)	((x) == ROCKCHIP_VOP2_EP_LVDS0 || \
					 (x) == ROCKCHIP_VOP2_EP_LVDS1)

#define vop2_output_if_is_dpi(x)	((x) == ROCKCHIP_VOP2_EP_RGB0)

static const struct regmap_config vop2_regmap_config;

static void vop2_writel(struct vop2 *vop2, u32 offset, u32 v)
{
	regmap_write(vop2->map, offset, v);
}

static void vop2_vp_write(struct vop2_video_port *vp, u32 offset, u32 v)
{
	regmap_write(vp->vop2->map, vp->data->offset + offset, v);
}

static u32 vop2_readl(struct vop2 *vop2, u32 offset)
{
	u32 val;

	regmap_read(vop2->map, offset, &val);

	return val;
}

static void vop2_win_write(const struct vop2_win *win, unsigned int reg, u32 v)
{
	u32 offset = win->reg_field[reg].reg;
	u32 idx = offset / sizeof(u32);

	regmap_field_write(win->reg[reg], v);

	writel(win->regs[idx], win->vop2->regs + win->offset + offset);
}

/*
 * Note:
 * The write mask function is documented but missing on rk3566/8, writes
 * to these bits have no effect. For newer soc(rk3588 and following) the
 * write mask is needed for register writes.
 *
 * GLB_CFG_DONE_EN has no write mask bit.
 *
 */
static void vop2_cfg_done(struct vop2_video_port *vp)
{
	struct vop2 *vop2 = vp->vop2;
	u32 val = RK3568_REG_CFG_DONE__GLB_CFG_DONE_EN;

	val |= BIT(vp->id) | (BIT(vp->id) << 16);

	regmap_set_bits(vop2->map, RK3568_REG_CFG_DONE, val);
}

static void vop2_win_disable(struct vop2_win *win)
{
	vop2_win_write(win, VOP2_WIN_ENABLE, 0);
}

#if 0
static enum vop2_data_format vop2_convert_format(u32 format)
{
	switch (format) {
	case DRM_FORMAT_XRGB8888:
	case DRM_FORMAT_ARGB8888:
	case DRM_FORMAT_XBGR8888:
	case DRM_FORMAT_ABGR8888:
		return VOP2_FMT_ARGB8888;
	case DRM_FORMAT_RGB888:
	case DRM_FORMAT_BGR888:
		return VOP2_FMT_RGB888;
	case DRM_FORMAT_RGB565:
	case DRM_FORMAT_BGR565:
		return VOP2_FMT_RGB565;
	default:
		pr_err("unsupported format[%08x]\n", format);
		return -EINVAL;
	}
}
#endif

static bool vop2_win_rb_swap(u32 format)
{
	switch (format) {
	case DRM_FORMAT_XBGR2101010:
	case DRM_FORMAT_ABGR2101010:
	case DRM_FORMAT_XBGR8888:
	case DRM_FORMAT_ABGR8888:
	case DRM_FORMAT_BGR888:
	case DRM_FORMAT_BGR565:
		return true;
	default:
		return false;
	}
}

static bool vop2_win_dither_up(u32 format)
{
	switch (format) {
	case DRM_FORMAT_BGR565:
	case DRM_FORMAT_RGB565:
		return true;
	default:
		return false;
	}
}

static bool vop2_output_rg_swap(struct vop2 *vop2, u32 bus_format)
{
	if (vop2->data->soc_id == 3588) {
		if (bus_format == MEDIA_BUS_FMT_YUV8_1X24 ||
		    bus_format == MEDIA_BUS_FMT_YUV10_1X30)
			return true;
	}

	return false;
}
#if 0
static u16 vop2_scale_factor(u32 src, u32 dst)
{
	u32 fac;
	int shift;

	if (src == dst)
		return 0;

	if (dst < 2)
		return U16_MAX;

	if (src < 2)
		return 0;

	if (src > dst)
		shift = 12;
	else
		shift = 16;

	src--;
	dst--;

	fac = DIV_ROUND_UP(src << shift, dst) - 1;

	if (fac > U16_MAX)
		return U16_MAX;

	return fac;
}
#endif

/*
 * colorspace path:
 *      Input        Win csc                     Output
 * 1. YUV(2020)  --> Y2R->2020To709->R2Y   --> YUV_OUTPUT(601/709)
 *    RGB        --> R2Y                  __/
 *
 * 2. YUV(2020)  --> bypasss               --> YUV_OUTPUT(2020)
 *    RGB        --> 709To2020->R2Y       __/
 *
 * 3. YUV(2020)  --> Y2R->2020To709        --> RGB_OUTPUT(709)
 *    RGB        --> R2Y                  __/
 *
 * 4. YUV(601/709)-> Y2R->709To2020->R2Y   --> YUV_OUTPUT(2020)
 *    RGB        --> 709To2020->R2Y       __/
 *
 * 5. YUV(601/709)-> bypass                --> YUV_OUTPUT(709)
 *    RGB        --> R2Y                  __/
 *
 * 6. YUV(601/709)-> bypass                --> YUV_OUTPUT(601)
 *    RGB        --> R2Y(601)             __/
 *
 * 7. YUV        --> Y2R(709)              --> RGB_OUTPUT(709)
 *    RGB        --> bypass               __/
 *
 * 8. RGB        --> 709To2020->R2Y        --> YUV_OUTPUT(2020)
 *
 * 9. RGB        --> R2Y(709)              --> YUV_OUTPUT(709)
 *
 * 10. RGB       --> R2Y(601)              --> YUV_OUTPUT(601)
 *
 * 11. RGB       --> bypass                --> RGB_OUTPUT(709)
 */

static int vop2_core_clks_prepare_enable(struct vop2 *vop2)
{
	int ret;

	ret = clk_prepare_enable(vop2->hclk);
	if (ret < 0) {
		dev_err(vop2->dev, "failed to enable hclk - %d\n", ret);
		return ret;
	}

	ret = clk_prepare_enable(vop2->aclk);
	if (ret < 0) {
		dev_err(vop2->dev, "failed to enable aclk - %d\n", ret);
		goto err;
	}

	ret = clk_prepare_enable(vop2->pclk);
	if (ret < 0) {
		dev_err(vop2->dev, "failed to enable pclk - %d\n", ret);
		goto err1;
	}

	return 0;
err1:
	clk_disable_unprepare(vop2->aclk);
err:
	clk_disable_unprepare(vop2->hclk);

	return ret;
}

static void rk3588_vop2_power_domain_enable_all(struct vop2 *vop2)
{
	u32 pd;

	pd = vop2_readl(vop2, RK3588_SYS_PD_CTRL);
	pd &= ~(VOP2_PD_CLUSTER0 | VOP2_PD_CLUSTER1 | VOP2_PD_CLUSTER2 |
		VOP2_PD_CLUSTER3 | VOP2_PD_ESMART);

	vop2_writel(vop2, RK3588_SYS_PD_CTRL, pd);
}

static void vop2_enable(struct vop2 *vop2)
{
	int ret;

	ret = vop2_core_clks_prepare_enable(vop2);
	if (ret)
		return;

	if (vop2->data->soc_id == 3566)
		vop2_writel(vop2, RK3568_OTP_WIN_EN, 1);

	if (vop2->data->soc_id == 3588)
		rk3588_vop2_power_domain_enable_all(vop2);

	vop2_writel(vop2, RK3568_REG_CFG_DONE, RK3568_REG_CFG_DONE__GLB_CFG_DONE_EN);

	/*
	 * Disable auto gating, this is a workaround to
	 * avoid display image shift when a window enabled.
	 */
	regmap_clear_bits(vop2->map, RK3568_SYS_AUTO_GATING_CTRL,
			  RK3568_SYS_AUTO_GATING_CTRL__AUTO_GATING_EN);

	vop2_writel(vop2, RK3568_SYS0_INT_CLR,
		    VOP2_INT_BUS_ERRPR << 16 | VOP2_INT_BUS_ERRPR);
	vop2_writel(vop2, RK3568_SYS0_INT_EN,
		    VOP2_INT_BUS_ERRPR << 16 | VOP2_INT_BUS_ERRPR);
	vop2_writel(vop2, RK3568_SYS1_INT_CLR,
		    VOP2_INT_BUS_ERRPR << 16 | VOP2_INT_BUS_ERRPR);
	vop2_writel(vop2, RK3568_SYS1_INT_EN,
		    VOP2_INT_BUS_ERRPR << 16 | VOP2_INT_BUS_ERRPR);
}

static void vop2_disable(struct vop2 *vop2)
{
	clk_disable_unprepare(vop2->pclk);
	clk_disable_unprepare(vop2->aclk);
	clk_disable_unprepare(vop2->hclk);
}

static void vop2_crtc_atomic_disable(struct vop2_video_port *vp)
{
	struct vop2 *vop2 = vp->vop2;

	vop2_vp_write(vp, RK3568_VP_DSP_CTRL, RK3568_VP_DSP_CTRL__STANDBY);

	clk_disable_unprepare(vp->dclk);

	vop2->enable_count--;

	if (!vop2->enable_count)
		vop2_disable(vop2);
}

static void vop2_plane_atomic_disable(struct vop2_win *win)
{
	struct vop2 *vop2 = win->vop2;

	dev_dbg(vop2->dev, "%s disable\n", win->data->name);

	vop2_win_disable(win);
	vop2_win_write(win, VOP2_WIN_YUV_CLIP, 0);
}

static void vop2_plane_atomic_update(struct vop2_video_port *vp, struct vop2_win *win,
				     dma_addr_t dma,
				     struct fb_rect *src, struct fb_rect *dest)
{
	u32 actual_w, actual_h, dsp_w, dsp_h;
	u32 act_info, dsp_info;
	u32 format = VOP2_FMT_ARGB8888;
	u32 rb_swap;
	u32 pitch = win->info.line_length;
	bool dither_up;

	actual_w = fb_rect_width(src);
	actual_h = fb_rect_height(src);
	dsp_w = win->info.xres;
	dsp_h = win->info.yres;

	act_info = (actual_h - 1) << 16 | ((actual_w - 1) & 0xffff);
	dsp_info = (dsp_h - 1) << 16 | ((dsp_w - 1) & 0xffff);

	vop2_win_write(win, VOP2_WIN_YRGB_VIR, DIV_ROUND_UP(pitch, 4));

	vop2_win_write(win, VOP2_WIN_YMIRROR, 0);

	vop2_win_write(win, VOP2_WIN_FORMAT, format);
	vop2_win_write(win, VOP2_WIN_YRGB_MST, dma);

	rb_swap = vop2_win_rb_swap(format);
	vop2_win_write(win, VOP2_WIN_RB_SWAP, rb_swap);
	vop2_win_write(win, VOP2_WIN_UV_SWAP, 0);

	vop2_win_write(win, VOP2_WIN_ACT_INFO, act_info);
	vop2_win_write(win, VOP2_WIN_DSP_INFO, dsp_info);
	vop2_win_write(win, VOP2_WIN_DSP_ST, dest->y1 << 16 | (dest->x1 & 0xffff));

	vop2_win_write(win, VOP2_WIN_Y2R_EN, 0);
	vop2_win_write(win, VOP2_WIN_R2Y_EN, 0);
	vop2_win_write(win, VOP2_WIN_CSC_MODE, 0);

	dither_up = vop2_win_dither_up(format);
	vop2_win_write(win, VOP2_WIN_DITHER_UP, dither_up);

	vop2_win_write(win, VOP2_WIN_ENABLE, 1);
}

static void vop2_dither_setup(u32 bus_format, u32 *dsp_ctrl)
{
	switch (bus_format) {
	case MEDIA_BUS_FMT_RGB565_1X16:
		*dsp_ctrl |= RK3568_VP_DSP_CTRL__DITHER_DOWN_EN;
		break;
	case MEDIA_BUS_FMT_RGB666_1X18:
	case MEDIA_BUS_FMT_RGB666_1X24_CPADHI:
	case MEDIA_BUS_FMT_RGB666_1X7X3_SPWG:
		*dsp_ctrl |= RK3568_VP_DSP_CTRL__DITHER_DOWN_EN;
		*dsp_ctrl |= RGB888_TO_RGB666;
		break;
	case MEDIA_BUS_FMT_YUV8_1X24:
		*dsp_ctrl |= RK3568_VP_DSP_CTRL__PRE_DITHER_DOWN_EN;
		break;
	default:
		break;
	}

	*dsp_ctrl |= FIELD_PREP(RK3568_VP_DSP_CTRL__DITHER_DOWN_SEL,
				DITHER_DOWN_ALLEGRO);
}

static void vop2_post_config(struct vop2_video_port *vp, struct drm_display_mode *mode)
{
	u16 hdisplay = mode->hdisplay;
	u16 hact_st = mode->htotal - mode->hsync_start;
	u16 vdisplay = mode->vdisplay;
	u16 vact_st = mode->vtotal - mode->vsync_start;
	u32 left_margin = 100, right_margin = 100;
	u32 top_margin = 100, bottom_margin = 100;
	u16 hsize = hdisplay * (left_margin + right_margin) / 200;
	u16 vsize = vdisplay * (top_margin + bottom_margin) / 200;
	u16 hsync_len = mode->hsync_end - mode->hsync_start;
	u16 hact_end, vact_end;
	u32 val;
	u32 bg_dly;
	u32 pre_scan_dly;

	bg_dly = vp->data->pre_scan_max_dly[3];
	vop2_writel(vp->vop2, RK3568_VP_BG_MIX_CTRL(vp->id),
		    FIELD_PREP(RK3568_VP_BG_MIX_CTRL__BG_DLY, bg_dly));

	pre_scan_dly = ((bg_dly + (hdisplay >> 1) - 1) << 16) | hsync_len;
	vop2_vp_write(vp, RK3568_VP_PRE_SCAN_HTIMING, pre_scan_dly);

	vsize = rounddown(vsize, 2);
	hsize = rounddown(hsize, 2);
	hact_st += hdisplay * (100 - left_margin) / 200;
	hact_end = hact_st + hsize;
	val = hact_st << 16;
	val |= hact_end;
	vop2_vp_write(vp, RK3568_VP_POST_DSP_HACT_INFO, val);
	vact_st += vdisplay * (100 - top_margin) / 200;
	vact_end = vact_st + vsize;
	val = vact_st << 16;
	val |= vact_end;
	vop2_vp_write(vp, RK3568_VP_POST_DSP_VACT_INFO, val);
	val = scl_cal_scale2(vdisplay, vsize) << 16;
	val |= scl_cal_scale2(hdisplay, hsize);
	vop2_vp_write(vp, RK3568_VP_POST_SCL_FACTOR_YRGB, val);

	val = 0;
	if (hdisplay != hsize)
		val |= RK3568_VP_POST_SCL_CTRL__HSCALEDOWN;
	if (vdisplay != vsize)
		val |= RK3568_VP_POST_SCL_CTRL__VSCALEDOWN;
	vop2_vp_write(vp, RK3568_VP_POST_SCL_CTRL, val);

	vop2_vp_write(vp, RK3568_VP_DSP_BG, 0);
}

static unsigned long rk3568_set_intf_mux(struct vop2_video_port *vp, int id, u32 polflags,
					 unsigned int clock)
{
	struct vop2 *vop2 = vp->vop2;
	u32 die, dip;

	die = vop2_readl(vop2, RK3568_DSP_IF_EN);
	dip = vop2_readl(vop2, RK3568_DSP_IF_POL);

	switch (id) {
	case ROCKCHIP_VOP2_EP_RGB0:
		die &= ~RK3568_SYS_DSP_INFACE_EN_RGB_MUX;
		die |= RK3568_SYS_DSP_INFACE_EN_RGB |
			   FIELD_PREP(RK3568_SYS_DSP_INFACE_EN_RGB_MUX, vp->id);
		dip &= ~RK3568_DSP_IF_POL__RGB_LVDS_PIN_POL;
		dip |= FIELD_PREP(RK3568_DSP_IF_POL__RGB_LVDS_PIN_POL, polflags);
		if (polflags & POLFLAG_DCLK_INV)
			regmap_write(vop2->sys_grf, RK3568_GRF_VO_CON1, BIT(3 + 16) | BIT(3));
		else
			regmap_write(vop2->sys_grf, RK3568_GRF_VO_CON1, BIT(3 + 16));
		break;
	case ROCKCHIP_VOP2_EP_HDMI0:
		die &= ~RK3568_SYS_DSP_INFACE_EN_HDMI_MUX;
		die |= RK3568_SYS_DSP_INFACE_EN_HDMI |
			   FIELD_PREP(RK3568_SYS_DSP_INFACE_EN_HDMI_MUX, vp->id);
		dip &= ~RK3568_DSP_IF_POL__HDMI_PIN_POL;
		dip |= FIELD_PREP(RK3568_DSP_IF_POL__HDMI_PIN_POL, polflags);
		break;
	case ROCKCHIP_VOP2_EP_EDP0:
		die &= ~RK3568_SYS_DSP_INFACE_EN_EDP_MUX;
		die |= RK3568_SYS_DSP_INFACE_EN_EDP |
			   FIELD_PREP(RK3568_SYS_DSP_INFACE_EN_EDP_MUX, vp->id);
		dip &= ~RK3568_DSP_IF_POL__EDP_PIN_POL;
		dip |= FIELD_PREP(RK3568_DSP_IF_POL__EDP_PIN_POL, polflags);
		break;
	case ROCKCHIP_VOP2_EP_MIPI0:
		die &= ~RK3568_SYS_DSP_INFACE_EN_MIPI0_MUX;
		die |= RK3568_SYS_DSP_INFACE_EN_MIPI0 |
			   FIELD_PREP(RK3568_SYS_DSP_INFACE_EN_MIPI0_MUX, vp->id);
		dip &= ~RK3568_DSP_IF_POL__MIPI_PIN_POL;
		dip |= FIELD_PREP(RK3568_DSP_IF_POL__MIPI_PIN_POL, polflags);
		break;
	case ROCKCHIP_VOP2_EP_MIPI1:
		die &= ~RK3568_SYS_DSP_INFACE_EN_MIPI1_MUX;
		die |= RK3568_SYS_DSP_INFACE_EN_MIPI1 |
			   FIELD_PREP(RK3568_SYS_DSP_INFACE_EN_MIPI1_MUX, vp->id);
		dip &= ~RK3568_DSP_IF_POL__MIPI_PIN_POL;
		dip |= FIELD_PREP(RK3568_DSP_IF_POL__MIPI_PIN_POL, polflags);
		break;
	case ROCKCHIP_VOP2_EP_LVDS0:
		die &= ~RK3568_SYS_DSP_INFACE_EN_LVDS0_MUX;
		die |= RK3568_SYS_DSP_INFACE_EN_LVDS0 |
			   FIELD_PREP(RK3568_SYS_DSP_INFACE_EN_LVDS0_MUX, vp->id);
		dip &= ~RK3568_DSP_IF_POL__RGB_LVDS_PIN_POL;
		dip |= FIELD_PREP(RK3568_DSP_IF_POL__RGB_LVDS_PIN_POL, polflags);
		break;
	case ROCKCHIP_VOP2_EP_LVDS1:
		die &= ~RK3568_SYS_DSP_INFACE_EN_LVDS1_MUX;
		die |= RK3568_SYS_DSP_INFACE_EN_LVDS1 |
			   FIELD_PREP(RK3568_SYS_DSP_INFACE_EN_LVDS1_MUX, vp->id);
		dip &= ~RK3568_DSP_IF_POL__RGB_LVDS_PIN_POL;
		dip |= FIELD_PREP(RK3568_DSP_IF_POL__RGB_LVDS_PIN_POL, polflags);
		break;
	default:
		dev_err(vop2->dev, "Invalid interface id %d on vp%d\n", id, vp->id);
		return 0;
	}

	dip |= RK3568_DSP_IF_POL__CFG_DONE_IMD;

	vop2_writel(vop2, RK3568_DSP_IF_EN, die);
	vop2_writel(vop2, RK3568_DSP_IF_POL, dip);

	return clock;
}

/*
 * calc the dclk on rk3588
 * the available div of dclk is 1, 2, 4
 */
static unsigned long rk3588_calc_dclk(unsigned long child_clk, unsigned long max_dclk)
{
	if (child_clk * 4 <= max_dclk)
		return child_clk * 4;
	else if (child_clk * 2 <= max_dclk)
		return child_clk * 2;
	else if (child_clk <= max_dclk)
		return child_clk;
	else
		return 0;
}

/*
 * 4 pixclk/cycle on rk3588
 * RGB/eDP/HDMI: if_pixclk >= dclk_core
 * DP: dp_pixclk = dclk_out <= dclk_core
 * DSI: mipi_pixclk <= dclk_out <= dclk_core
 */
static unsigned long rk3588_calc_cru_cfg(struct vop2_video_port *vp, int id,
					 int *dclk_core_div, int *dclk_out_div,
					 int *if_pixclk_div, int *if_dclk_div)
{
	struct vop2 *vop2 = vp->vop2;
	u32 crtc_clock = 0;
	unsigned long v_pixclk = crtc_clock * 1000LL; /* video timing pixclk */
	unsigned long dclk_core_rate = v_pixclk >> 2;
	unsigned long dclk_rate = v_pixclk;
	unsigned long dclk_out_rate;
	unsigned long if_pixclk_rate;
	int K = 1;

	if (vop2_output_if_is_hdmi(id)) {
		if_pixclk_rate = (dclk_core_rate << 1) / K;
		/*
		 * if_dclk_rate = dclk_core_rate / K;
		 * *if_pixclk_div = dclk_rate / if_pixclk_rate;
		 * *if_dclk_div = dclk_rate / if_dclk_rate;
		 */
		*if_pixclk_div = 2;
		*if_dclk_div = 4;
	} else if (vop2_output_if_is_edp(id)) {
		/*
		 * edp_pixclk = edp_dclk > dclk_core
		 */
		if_pixclk_rate = v_pixclk / K;
		dclk_rate = if_pixclk_rate * K;
		/*
		 * *if_pixclk_div = dclk_rate / if_pixclk_rate;
		 * *if_dclk_div = *if_pixclk_div;
		 */
		*if_pixclk_div = K;
		*if_dclk_div = K;
	} else if (vop2_output_if_is_dp(id)) {
		dclk_out_rate = v_pixclk >> 2;

		dclk_rate = rk3588_calc_dclk(dclk_out_rate, 600000);
		if (!dclk_rate) {
			dev_err(vop2->dev, "DP dclk_out_rate out of range, dclk_out_rate: %ld KHZ\n",
				dclk_out_rate);
			return 0;
		}
		*dclk_out_div = dclk_rate / dclk_out_rate;
	} else if (vop2_output_if_is_mipi(id)) {
		if_pixclk_rate = dclk_core_rate / K;
		/*
		 * dclk_core = dclk_out * K = if_pixclk * K = v_pixclk / 4
		 */
		dclk_out_rate = if_pixclk_rate;
		/*
		 * dclk_rate = N * dclk_core_rate N = (1,2,4 ),
		 * we get a little factor here
		 */
		dclk_rate = rk3588_calc_dclk(dclk_out_rate, 600000);
		if (!dclk_rate) {
			dev_err(vop2->dev, "MIPI dclk out of range, dclk_out_rate: %ld KHZ\n",
				dclk_out_rate);
			return 0;
		}
		*dclk_out_div = dclk_rate / dclk_out_rate;
		/*
		 * mipi pixclk == dclk_out
		 */
		*if_pixclk_div = 1;
	} else if (vop2_output_if_is_dpi(id)) {
		dclk_rate = v_pixclk;
	}

	*dclk_core_div = dclk_rate / dclk_core_rate;
	*if_pixclk_div = ilog2(*if_pixclk_div);
	*if_dclk_div = ilog2(*if_dclk_div);
	*dclk_core_div = ilog2(*dclk_core_div);
	*dclk_out_div = ilog2(*dclk_out_div);

	dev_dbg(vop2->dev, "dclk: %ld, pixclk_div: %d, dclk_div: %d\n",
		dclk_rate, *if_pixclk_div, *if_dclk_div);

	return dclk_rate;
}

/*
 * MIPI port mux on rk3588:
 * 0: Video Port2
 * 1: Video Port3
 * 3: Video Port 1(MIPI1 only)
 */
static u32 rk3588_get_mipi_port_mux(int vp_id)
{
	if (vp_id == 1)
		return 3;
	else if (vp_id == 3)
		return 1;
	else
		return 0;
}

static u32 rk3588_get_hdmi_pol(u32 flags)
{
	u32 val;

	val = (flags & FB_SYNC_HOR_HIGH_ACT) ? BIT(HSYNC_POSITIVE) : 0;
	val |= (flags & FB_SYNC_VERT_HIGH_ACT) ? BIT(VSYNC_POSITIVE) : 0;

	return val;
}

static unsigned long rk3588_set_intf_mux(struct vop2_video_port *vp, int id, u32 polflags,
					 unsigned int clock)
{
	struct vop2 *vop2 = vp->vop2;
	int dclk_core_div, dclk_out_div, if_pixclk_div, if_dclk_div;
	u32 die, dip, div, vp_clk_div, val;

	clock = rk3588_calc_cru_cfg(vp, id, &dclk_core_div, &dclk_out_div,
				    &if_pixclk_div, &if_dclk_div);
	if (!clock)
		return 0;

	vp_clk_div = FIELD_PREP(RK3588_VP_CLK_CTRL__DCLK_CORE_DIV, dclk_core_div);
	vp_clk_div |= FIELD_PREP(RK3588_VP_CLK_CTRL__DCLK_OUT_DIV, dclk_out_div);

	die = vop2_readl(vop2, RK3568_DSP_IF_EN);
	dip = vop2_readl(vop2, RK3568_DSP_IF_POL);
	div = vop2_readl(vop2, RK3568_DSP_IF_CTRL);

	switch (id) {
	case ROCKCHIP_VOP2_EP_HDMI0:
		div &= ~RK3588_DSP_IF_EDP_HDMI0_DCLK_DIV;
		div &= ~RK3588_DSP_IF_EDP_HDMI0_PCLK_DIV;
		div |= FIELD_PREP(RK3588_DSP_IF_EDP_HDMI0_DCLK_DIV, if_dclk_div);
		div |= FIELD_PREP(RK3588_DSP_IF_EDP_HDMI0_PCLK_DIV, if_pixclk_div);
		die &= ~RK3588_SYS_DSP_INFACE_EN_EDP_HDMI0_MUX;
		die |= RK3588_SYS_DSP_INFACE_EN_HDMI0 |
			    FIELD_PREP(RK3588_SYS_DSP_INFACE_EN_EDP_HDMI0_MUX, vp->id);
		val = rk3588_get_hdmi_pol(polflags);
		regmap_write(vop2->vop_grf, RK3588_GRF_VOP_CON2, HIWORD_UPDATE(1, 1, 1));
		regmap_write(vop2->vo1_grf, RK3588_GRF_VO1_CON0, HIWORD_UPDATE(val, 6, 5));
		break;
	case ROCKCHIP_VOP2_EP_HDMI1:
		div &= ~RK3588_DSP_IF_EDP_HDMI1_DCLK_DIV;
		div &= ~RK3588_DSP_IF_EDP_HDMI1_PCLK_DIV;
		div |= FIELD_PREP(RK3588_DSP_IF_EDP_HDMI1_DCLK_DIV, if_dclk_div);
		div |= FIELD_PREP(RK3588_DSP_IF_EDP_HDMI1_PCLK_DIV, if_pixclk_div);
		die &= ~RK3588_SYS_DSP_INFACE_EN_EDP_HDMI1_MUX;
		die |= RK3588_SYS_DSP_INFACE_EN_HDMI1 |
			    FIELD_PREP(RK3588_SYS_DSP_INFACE_EN_EDP_HDMI1_MUX, vp->id);
		val = rk3588_get_hdmi_pol(polflags);
		regmap_write(vop2->vop_grf, RK3588_GRF_VOP_CON2, HIWORD_UPDATE(1, 4, 4));
		regmap_write(vop2->vo1_grf, RK3588_GRF_VO1_CON0, HIWORD_UPDATE(val, 8, 7));
		break;
	case ROCKCHIP_VOP2_EP_EDP0:
		div &= ~RK3588_DSP_IF_EDP_HDMI0_DCLK_DIV;
		div &= ~RK3588_DSP_IF_EDP_HDMI0_PCLK_DIV;
		div |= FIELD_PREP(RK3588_DSP_IF_EDP_HDMI0_DCLK_DIV, if_dclk_div);
		div |= FIELD_PREP(RK3588_DSP_IF_EDP_HDMI0_PCLK_DIV, if_pixclk_div);
		die &= ~RK3588_SYS_DSP_INFACE_EN_EDP_HDMI0_MUX;
		die |= RK3588_SYS_DSP_INFACE_EN_EDP0 |
			   FIELD_PREP(RK3588_SYS_DSP_INFACE_EN_EDP_HDMI0_MUX, vp->id);
		regmap_write(vop2->vop_grf, RK3588_GRF_VOP_CON2, HIWORD_UPDATE(1, 0, 0));
		break;
	case ROCKCHIP_VOP2_EP_EDP1:
		div &= ~RK3588_DSP_IF_EDP_HDMI1_DCLK_DIV;
		div &= ~RK3588_DSP_IF_EDP_HDMI1_PCLK_DIV;
		div |= FIELD_PREP(RK3588_DSP_IF_EDP_HDMI0_DCLK_DIV, if_dclk_div);
		div |= FIELD_PREP(RK3588_DSP_IF_EDP_HDMI0_PCLK_DIV, if_pixclk_div);
		die &= ~RK3588_SYS_DSP_INFACE_EN_EDP_HDMI1_MUX;
		die |= RK3588_SYS_DSP_INFACE_EN_EDP1 |
			   FIELD_PREP(RK3588_SYS_DSP_INFACE_EN_EDP_HDMI1_MUX, vp->id);
		regmap_write(vop2->vop_grf, RK3588_GRF_VOP_CON2, HIWORD_UPDATE(1, 3, 3));
		break;
	case ROCKCHIP_VOP2_EP_MIPI0:
		div &= ~RK3588_DSP_IF_MIPI0_PCLK_DIV;
		div |= FIELD_PREP(RK3588_DSP_IF_MIPI0_PCLK_DIV, if_pixclk_div);
		die &= ~RK3588_SYS_DSP_INFACE_EN_MIPI0_MUX;
		val = rk3588_get_mipi_port_mux(vp->id);
		die |= RK3588_SYS_DSP_INFACE_EN_MIPI0 |
			   FIELD_PREP(RK3588_SYS_DSP_INFACE_EN_MIPI0_MUX, !!val);
		break;
	case ROCKCHIP_VOP2_EP_MIPI1:
		div &= ~RK3588_DSP_IF_MIPI1_PCLK_DIV;
		div |= FIELD_PREP(RK3588_DSP_IF_MIPI1_PCLK_DIV, if_pixclk_div);
		die &= ~RK3588_SYS_DSP_INFACE_EN_MIPI1_MUX;
		val = rk3588_get_mipi_port_mux(vp->id);
		die |= RK3588_SYS_DSP_INFACE_EN_MIPI1 |
			   FIELD_PREP(RK3588_SYS_DSP_INFACE_EN_MIPI1_MUX, val);
		break;
	case ROCKCHIP_VOP2_EP_DP0:
		die &= ~RK3588_SYS_DSP_INFACE_EN_DP0_MUX;
		die |= RK3588_SYS_DSP_INFACE_EN_DP0 |
			   FIELD_PREP(RK3588_SYS_DSP_INFACE_EN_DP0_MUX, vp->id);
		dip &= ~RK3588_DSP_IF_POL__DP0_PIN_POL;
		dip |= FIELD_PREP(RK3588_DSP_IF_POL__DP0_PIN_POL, polflags);
		break;
	case ROCKCHIP_VOP2_EP_DP1:
		die &= ~RK3588_SYS_DSP_INFACE_EN_MIPI1_MUX;
		die |= RK3588_SYS_DSP_INFACE_EN_MIPI1 |
			   FIELD_PREP(RK3588_SYS_DSP_INFACE_EN_MIPI1_MUX, vp->id);
		dip &= ~RK3588_DSP_IF_POL__DP1_PIN_POL;
		dip |= FIELD_PREP(RK3588_DSP_IF_POL__DP1_PIN_POL, polflags);
		break;
	default:
		dev_err(vop2->dev, "Invalid interface id %d on vp%d\n", id, vp->id);
		return 0;
	}

	dip |= RK3568_DSP_IF_POL__CFG_DONE_IMD;

	vop2_vp_write(vp, RK3588_VP_CLK_CTRL, vp_clk_div);
	vop2_writel(vop2, RK3568_DSP_IF_EN, die);
	vop2_writel(vop2, RK3568_DSP_IF_CTRL, div);
	vop2_writel(vop2, RK3568_DSP_IF_POL, dip);

	return clock;
}

static unsigned long vop2_set_intf_mux(struct vop2_video_port *vp, int ep_id, u32 polflags,
				       unsigned int clock)
{
	struct vop2 *vop2 = vp->vop2;

	if (vop2->data->soc_id == 3566 || vop2->data->soc_id == 3568)
		return rk3568_set_intf_mux(vp, ep_id, polflags, clock);
	else if (vop2->data->soc_id == 3588)
		return rk3588_set_intf_mux(vp, ep_id, polflags, clock);
	else
		return 0;
}

static int us_to_vertical_line(struct drm_display_mode *mode, int us)
{
	return us * mode->clock / mode->htotal / 1000;
}

static int drm_mode_vrefresh(const struct drm_display_mode *mode)
{
	unsigned int num, den;

	if (mode->htotal == 0 || mode->vtotal == 0)
		return 0;

	num = mode->clock;
	den = mode->htotal * mode->vtotal;

	return DIV_ROUND_CLOSEST_ULL(mul_u32_u32(num, 1000), den);
}

static void vop2_crtc_atomic_enable(struct vop2_video_port *vp,
				    struct drm_display_mode *mode,
				    struct rockchip_crtc_state *vcstate)
{
	struct vop2 *vop2 = vp->vop2;
	unsigned long clock;
	u16 hsync_len = mode->hsync_end - mode->hsync_start;
	u16 hdisplay = mode->hdisplay;
	u16 htotal = mode->htotal;
	u16 hact_st = mode->htotal - mode->hsync_start;
	u16 hact_end = hact_st + hdisplay;
	u16 vdisplay = mode->vdisplay;
	u16 vtotal = mode->vtotal;
	u16 vsync_len = mode->vsync_end - mode->vsync_start;
	u16 vact_st = mode->vtotal - mode->vsync_start;
	u16 vact_end = vact_st + vdisplay;
	u8 out_mode;
	u32 dsp_ctrl = 0;
	int act_end;
	u32 val, polflags;
	int ret;

	dev_dbg(vop2->dev, "Update mode to %dx%dp%d, type: %d for vp%d\n",
		hdisplay, vdisplay,
		drm_mode_vrefresh(mode), vcstate->output_type, vp->id);

	ret = clk_prepare_enable(vp->dclk);
	if (ret < 0) {
		dev_err(vop2->dev, "failed to enable dclk for video port%d - %d\n",
			vp->id, ret);
		return;
	}

	if (!vop2->enable_count)
		vop2_enable(vop2);

	vop2->enable_count++;

	polflags = 0;
	if (vcstate->bus_flags & DRM_BUS_FLAG_PIXDATA_DRIVE_NEGEDGE)
		polflags |= POLFLAG_DCLK_INV;
	if (mode->flags & DRM_MODE_FLAG_PHSYNC)
		polflags |= BIT(HSYNC_POSITIVE);
	if (mode->flags & DRM_MODE_FLAG_PVSYNC)
		polflags |= BIT(VSYNC_POSITIVE);

	/*
	 * for drive a high resolution(4KP120, 8K), vop on rk3588/rk3576 need
	 * process multi(1/2/4/8) pixels per cycle, so the dclk feed by the
	 * system cru may be the 1/2 or 1/4 of mode->clock.
	 */
	clock = vop2_set_intf_mux(vp, vp->crtc_endpoint_id, polflags, mode->clock * 1000);
	if (!clock)
		return;

	out_mode = vcstate->output_mode;

	dsp_ctrl |= FIELD_PREP(RK3568_VP_DSP_CTRL__OUT_MODE, out_mode);

	if (vop2_output_rg_swap(vop2, vcstate->bus_format))
		dsp_ctrl |= RK3568_VP_DSP_CTRL__DSP_RG_SWAP;

	if (vcstate->yuv_overlay)
		dsp_ctrl |= RK3568_VP_DSP_CTRL__POST_DSP_OUT_R2Y;

	vop2_dither_setup(vcstate->bus_format, &dsp_ctrl);

	vop2_vp_write(vp, RK3568_VP_DSP_HTOTAL_HS_END, (htotal << 16) | hsync_len);
	val = hact_st << 16;
	val |= hact_end;
	vop2_vp_write(vp, RK3568_VP_DSP_HACT_ST_END, val);

	val = vact_st << 16;
	val |= vact_end;
	vop2_vp_write(vp, RK3568_VP_DSP_VACT_ST_END, val);

	act_end = vact_end;

	vop2_writel(vop2, RK3568_VP_LINE_FLAG(vp->id),
		    (act_end - us_to_vertical_line(mode, 0)) << 16 | act_end);

	vop2_vp_write(vp, RK3568_VP_DSP_VTOTAL_VS_END, vtotal << 16 | vsync_len);

	vop2_vp_write(vp, RK3568_VP_MIPI_CTRL, 0);

	clk_set_rate(vp->dclk, clock);

	vop2_post_config(vp, mode);

	vop2_cfg_done(vp);

	vop2_vp_write(vp, RK3568_VP_DSP_CTRL, dsp_ctrl);
}

static bool is_opaque(u16 alpha)
{
	return (alpha >> 8) == 0xff;
}

static void vop2_parse_alpha(struct vop2_alpha_config *alpha_config,
			     struct vop2_alpha *alpha)
{
	int src_glb_alpha_en = is_opaque(alpha_config->src_glb_alpha_value) ? 0 : 1;
	int dst_glb_alpha_en = is_opaque(alpha_config->dst_glb_alpha_value) ? 0 : 1;
	int src_color_mode = alpha_config->src_premulti_en ?
				ALPHA_SRC_PRE_MUL : ALPHA_SRC_NO_PRE_MUL;
	int dst_color_mode = alpha_config->dst_premulti_en ?
				ALPHA_SRC_PRE_MUL : ALPHA_SRC_NO_PRE_MUL;

	alpha->src_color_ctrl.val = 0;
	alpha->dst_color_ctrl.val = 0;
	alpha->src_alpha_ctrl.val = 0;
	alpha->dst_alpha_ctrl.val = 0;

	if (!alpha_config->src_pixel_alpha_en)
		alpha->src_color_ctrl.bits.blend_mode = ALPHA_GLOBAL;
	else if (alpha_config->src_pixel_alpha_en && !src_glb_alpha_en)
		alpha->src_color_ctrl.bits.blend_mode = ALPHA_PER_PIX;
	else
		alpha->src_color_ctrl.bits.blend_mode = ALPHA_PER_PIX_GLOBAL;

	alpha->src_color_ctrl.bits.alpha_en = 1;

	if (alpha->src_color_ctrl.bits.blend_mode == ALPHA_GLOBAL) {
		alpha->src_color_ctrl.bits.color_mode = src_color_mode;
		alpha->src_color_ctrl.bits.factor_mode = SRC_FAC_ALPHA_SRC_GLOBAL;
	} else if (alpha->src_color_ctrl.bits.blend_mode == ALPHA_PER_PIX) {
		alpha->src_color_ctrl.bits.color_mode = src_color_mode;
		alpha->src_color_ctrl.bits.factor_mode = SRC_FAC_ALPHA_ONE;
	} else {
		alpha->src_color_ctrl.bits.color_mode = ALPHA_SRC_PRE_MUL;
		alpha->src_color_ctrl.bits.factor_mode = SRC_FAC_ALPHA_SRC_GLOBAL;
	}
	alpha->src_color_ctrl.bits.glb_alpha = alpha_config->src_glb_alpha_value >> 8;
	alpha->src_color_ctrl.bits.alpha_mode = ALPHA_STRAIGHT;
	alpha->src_color_ctrl.bits.alpha_cal_mode = ALPHA_SATURATION;

	alpha->dst_color_ctrl.bits.alpha_mode = ALPHA_STRAIGHT;
	alpha->dst_color_ctrl.bits.alpha_cal_mode = ALPHA_SATURATION;
	alpha->dst_color_ctrl.bits.blend_mode = ALPHA_GLOBAL;
	alpha->dst_color_ctrl.bits.glb_alpha = alpha_config->dst_glb_alpha_value >> 8;
	alpha->dst_color_ctrl.bits.color_mode = dst_color_mode;
	alpha->dst_color_ctrl.bits.factor_mode = ALPHA_SRC_INVERSE;

	alpha->src_alpha_ctrl.bits.alpha_mode = ALPHA_STRAIGHT;
	alpha->src_alpha_ctrl.bits.blend_mode = alpha->src_color_ctrl.bits.blend_mode;
	alpha->src_alpha_ctrl.bits.alpha_cal_mode = ALPHA_SATURATION;
	alpha->src_alpha_ctrl.bits.factor_mode = ALPHA_ONE;

	alpha->dst_alpha_ctrl.bits.alpha_mode = ALPHA_STRAIGHT;
	if (alpha_config->dst_pixel_alpha_en && !dst_glb_alpha_en)
		alpha->dst_alpha_ctrl.bits.blend_mode = ALPHA_PER_PIX;
	else
		alpha->dst_alpha_ctrl.bits.blend_mode = ALPHA_PER_PIX_GLOBAL;
	alpha->dst_alpha_ctrl.bits.alpha_cal_mode = ALPHA_NO_SATURATION;
	alpha->dst_alpha_ctrl.bits.factor_mode = ALPHA_SRC_INVERSE;
}

static int vop2_find_start_mixer_id_for_vp(struct vop2 *vop2, u8 port_id)
{
	struct vop2_video_port *vp;
	int used_layer = 0;
	int i;

	for (i = 0; i < port_id; i++) {
		vp = &vop2->vps[i];
		used_layer += hweight32(vp->win_mask);
	}

	return used_layer;
}

#define DRM_BLEND_ALPHA_OPAQUE               0xffff
#define DRM_MODE_BLEND_PREMULTI              0

static void vop2_setup_alpha(struct vop2_video_port *vp, u32 dst_global_alpha)
{
	struct vop2 *vop2 = vp->vop2;
	struct vop2_alpha_config alpha_config;
	struct vop2_alpha alpha;
	struct vop2_win *win;
	int pixel_alpha_en;
	int premulti_en;
	int mixer_id;
	u32 offset;
	int zpos = 0;

	mixer_id = vop2_find_start_mixer_id_for_vp(vop2, vp->id);
	alpha_config.dst_pixel_alpha_en = true; /* alpha value need transfer to next mix */

	list_for_each_entry(win, &vp->windows, list) {
		if (!win->enabled)
			continue;

		if (win->pixel_blend_mode == DRM_MODE_BLEND_PREMULTI)
			premulti_en = 1;
		else
			premulti_en = 0;

		pixel_alpha_en = win->info.transp.length != 0;

		alpha_config.src_premulti_en = premulti_en;

		/* Cd = Cs + (1 - As) * Cd */
		alpha_config.dst_premulti_en = true;
		alpha_config.src_pixel_alpha_en = pixel_alpha_en;
		alpha_config.src_glb_alpha_value = win->alpha;
		alpha_config.dst_glb_alpha_value = DRM_BLEND_ALPHA_OPAQUE;

		vop2_parse_alpha(&alpha_config, &alpha);

		offset = (mixer_id + zpos) * 0x10;
		zpos++;
		vop2_writel(vop2, RK3568_MIX0_SRC_COLOR_CTRL + offset,
			    alpha.src_color_ctrl.val);
		vop2_writel(vop2, RK3568_MIX0_DST_COLOR_CTRL + offset,
			    alpha.dst_color_ctrl.val);
		vop2_writel(vop2, RK3568_MIX0_SRC_ALPHA_CTRL + offset,
			    alpha.src_alpha_ctrl.val);
		vop2_writel(vop2, RK3568_MIX0_DST_ALPHA_CTRL + offset,
			    alpha.dst_alpha_ctrl.val);
	}

	if (vp->id == 0)
		vop2_writel(vop2, RK3568_HDR0_SRC_COLOR_CTRL, 0);

}

static void vop2_setup_layer_mixer(struct vop2_video_port *vp)
{
	struct vop2 *vop2 = vp->vop2;
	u32 layer_sel = 0;
	u32 port_sel;
	unsigned int nlayer, ofs;
	u32 ovl_ctrl;
	int i;
	struct vop2_video_port *vp0 = &vop2->vps[0];
	struct vop2_video_port *vp1 = &vop2->vps[1];
	struct vop2_video_port *vp2 = &vop2->vps[2];
	struct vop2_win *win;

	ovl_ctrl = vop2_readl(vop2, RK3568_OVL_CTRL);
	ovl_ctrl |= RK3568_OVL_CTRL__LAYERSEL_REGDONE_IMD;
	ovl_ctrl &= ~RK3568_OVL_CTRL__YUV_MODE(vp->id);

	vop2_writel(vop2, RK3568_OVL_CTRL, ovl_ctrl);

	port_sel = vop2_readl(vop2, RK3568_OVL_PORT_SEL);
	port_sel &= RK3568_OVL_PORT_SEL__SEL_PORT;

	if (vp0->nlayers)
		port_sel |= FIELD_PREP(RK3568_OVL_PORT_SET__PORT0_MUX,
				     vp0->nlayers - 1);
	else
		port_sel |= FIELD_PREP(RK3568_OVL_PORT_SET__PORT0_MUX, 8);

	if (vp1->nlayers)
		port_sel |= FIELD_PREP(RK3568_OVL_PORT_SET__PORT1_MUX,
				     (vp0->nlayers + vp1->nlayers - 1));
	else
		port_sel |= FIELD_PREP(RK3568_OVL_PORT_SET__PORT1_MUX, 8);

	if (vp2->nlayers)
		port_sel |= FIELD_PREP(RK3568_OVL_PORT_SET__PORT2_MUX,
			(vp2->nlayers + vp1->nlayers + vp0->nlayers - 1));
	else
		port_sel |= FIELD_PREP(RK3568_OVL_PORT_SET__PORT2_MUX, 8);

	layer_sel = vop2_readl(vop2, RK3568_OVL_LAYER_SEL);

	ofs = 0;
	for (i = 0; i < vp->id; i++)
		ofs += vop2->vps[i].nlayers;

	nlayer = 0;

	list_for_each_entry(win, &vp->windows, list) {
		if (!win->enabled)
			continue;

		switch (win->data->phys_id) {
		case ROCKCHIP_VOP2_CLUSTER0:
			port_sel &= ~RK3568_OVL_PORT_SEL__CLUSTER0;
			port_sel |= FIELD_PREP(RK3568_OVL_PORT_SEL__CLUSTER0, vp->id);
			break;
		case ROCKCHIP_VOP2_CLUSTER1:
			port_sel &= ~RK3568_OVL_PORT_SEL__CLUSTER1;
			port_sel |= FIELD_PREP(RK3568_OVL_PORT_SEL__CLUSTER1, vp->id);
			break;
		case ROCKCHIP_VOP2_CLUSTER2:
			port_sel &= ~RK3588_OVL_PORT_SEL__CLUSTER2;
			port_sel |= FIELD_PREP(RK3588_OVL_PORT_SEL__CLUSTER2, vp->id);
			break;
		case ROCKCHIP_VOP2_CLUSTER3:
			port_sel &= ~RK3588_OVL_PORT_SEL__CLUSTER3;
			port_sel |= FIELD_PREP(RK3588_OVL_PORT_SEL__CLUSTER3, vp->id);
			break;
		case ROCKCHIP_VOP2_ESMART0:
			port_sel &= ~RK3568_OVL_PORT_SEL__ESMART0;
			port_sel |= FIELD_PREP(RK3568_OVL_PORT_SEL__ESMART0, vp->id);
			break;
		case ROCKCHIP_VOP2_ESMART1:
			port_sel &= ~RK3568_OVL_PORT_SEL__ESMART1;
			port_sel |= FIELD_PREP(RK3568_OVL_PORT_SEL__ESMART1, vp->id);
			break;
		case ROCKCHIP_VOP2_ESMART2:
			port_sel &= ~RK3588_OVL_PORT_SEL__ESMART2;
			port_sel |= FIELD_PREP(RK3588_OVL_PORT_SEL__ESMART2, vp->id);
			break;
		case ROCKCHIP_VOP2_ESMART3:
			port_sel &= ~RK3588_OVL_PORT_SEL__ESMART3;
			port_sel |= FIELD_PREP(RK3588_OVL_PORT_SEL__ESMART3, vp->id);
			break;
		case ROCKCHIP_VOP2_SMART0:
			port_sel &= ~RK3568_OVL_PORT_SEL__SMART0;
			port_sel |= FIELD_PREP(RK3568_OVL_PORT_SEL__SMART0, vp->id);
			break;
		case ROCKCHIP_VOP2_SMART1:
			port_sel &= ~RK3568_OVL_PORT_SEL__SMART1;
			port_sel |= FIELD_PREP(RK3568_OVL_PORT_SEL__SMART1, vp->id);
			break;
		}

		layer_sel &= ~RK3568_OVL_LAYER_SEL__LAYER(nlayer + ofs,
							  0x7);
		layer_sel |= RK3568_OVL_LAYER_SEL__LAYER(nlayer + ofs,
							 win->data->layer_sel_id);
		nlayer++;
	}

	/* configure unused layers to 0x5 (reserved) */
	for (; nlayer < vp->nlayers + 2; nlayer++) {
		layer_sel &= ~RK3568_OVL_LAYER_SEL__LAYER(nlayer + ofs, 0x7);
		layer_sel |= RK3568_OVL_LAYER_SEL__LAYER(nlayer + ofs, 5);
	}

	vop2_writel(vop2, RK3568_OVL_LAYER_SEL, layer_sel);
	vop2_writel(vop2, RK3568_OVL_PORT_SEL, port_sel);
}

static void vop2_setup_dly_for_windows(struct vop2 *vop2)
{
	struct vop2_win *win;
	int i = 0;
	u32 cdly = 0, sdly = 0;

	for (i = 0; i < vop2->registered_num_wins; i++) {
		u32 dly;

		win = &vop2->win[i];
		dly = win->data->dly[VOP2_DLY_MODE_DEFAULT];

		switch (win->data->phys_id) {
		case ROCKCHIP_VOP2_CLUSTER0:
			cdly |= FIELD_PREP(RK3568_CLUSTER_DLY_NUM__CLUSTER0_0, dly);
			cdly |= FIELD_PREP(RK3568_CLUSTER_DLY_NUM__CLUSTER0_1, dly);
			break;
		case ROCKCHIP_VOP2_CLUSTER1:
			cdly |= FIELD_PREP(RK3568_CLUSTER_DLY_NUM__CLUSTER1_0, dly);
			cdly |= FIELD_PREP(RK3568_CLUSTER_DLY_NUM__CLUSTER1_1, dly);
			break;
		case ROCKCHIP_VOP2_ESMART0:
			sdly |= FIELD_PREP(RK3568_SMART_DLY_NUM__ESMART0, dly);
			break;
		case ROCKCHIP_VOP2_ESMART1:
			sdly |= FIELD_PREP(RK3568_SMART_DLY_NUM__ESMART1, dly);
			break;
		case ROCKCHIP_VOP2_SMART0:
			sdly |= FIELD_PREP(RK3568_SMART_DLY_NUM__SMART0, dly);
			break;
		case ROCKCHIP_VOP2_SMART1:
			sdly |= FIELD_PREP(RK3568_SMART_DLY_NUM__SMART1, dly);
			break;
		}
	}

	vop2_writel(vop2, RK3568_CLUSTER_DLY_NUM, cdly);
	vop2_writel(vop2, RK3568_SMART_DLY_NUM, sdly);
}

static void vop2_crtc_atomic_begin(struct vop2_video_port *vp)
{
	struct vop2 *vop2 = vp->vop2;
	struct vop2_win *win;

	vp->win_mask = 0;

	list_for_each_entry(win, &vp->windows, list) {
		if (!win->enabled)
			continue;

		vp->win_mask |= BIT(win->data->phys_id);
	}

	if (!vp->win_mask)
		return;

	vop2_setup_layer_mixer(vp);
	vop2_setup_alpha(vp, DRM_BLEND_ALPHA_OPAQUE);
	vop2_setup_dly_for_windows(vop2);
}

static void vop2_crtc_atomic_flush(struct vop2_video_port *vp, struct drm_display_mode *mode)
{
	vop2_post_config(vp, mode);

	vop2_cfg_done(vp);
}

static struct vop2_video_port *find_vp_without_primary(struct vop2 *vop2)
{
	int i;

	for (i = 0; i < vop2->data->nr_vps; i++) {
		struct vop2_video_port *vp = &vop2->vps[i];

		if (!vp->port)
			continue;

		if (vp->primary_plane)
			continue;

		return vp;
	}

	return NULL;
}

static int vop2_output_mode(u32 bus_format, int crtc_endpoint_id)
{
	if (vop2_output_if_is_hdmi(crtc_endpoint_id))
		return ROCKCHIP_OUT_MODE_AAAA;

	if (vop2_output_if_is_dpi(crtc_endpoint_id))
		switch (bus_format) {
		case MEDIA_BUS_FMT_RGB666_1X18:
			return ROCKCHIP_OUT_MODE_P666;
		case MEDIA_BUS_FMT_RGB565_1X16:
			return ROCKCHIP_OUT_MODE_P565;
		case MEDIA_BUS_FMT_RGB888_1X24:
		case MEDIA_BUS_FMT_RGB666_1X24_CPADHI:
		default:
			return ROCKCHIP_OUT_MODE_P888;
	}

	return -EINVAL;
}

static void vop2_enable_controller(struct fb_info *info)
{
	struct vop2_win *win = container_of(info, struct vop2_win, info);
	struct vop2_video_port *vp = win->vp;
	struct drm_display_mode mode = {};
	struct rockchip_crtc_state vcstate = {
		.bus_format = 0,
		.output_mode = ROCKCHIP_OUT_MODE_P666, //ROCKCHIP_OUT_MODE_AAAA,
	};
	struct drm_display_info display_info = {};
	int ret;

	if (!info->mode) {
		dev_err(vp->vop2->dev, "no modes, cannot enable\n");
		return;
	}

	fb_videomode_to_drm_display_mode(info->mode, &mode);

	win->enabled = true;

	ret = vpl_ioctl(&vp->vpl, vp->id, VPL_GET_BUS_FORMAT, &vcstate.bus_format);
	if (ret < 0) {
		dev_err(vp->vop2->dev, "Cannot determine bus format\n");
		return;
	}

	ret = vpl_ioctl(&vp->vpl, vp->id, VPL_GET_DISPLAY_INFO, &display_info);
	if (ret < 0) {
		dev_err(vp->vop2->dev, "Cannot get display info\n");
		return;
	}

	vcstate.bus_flags = display_info.bus_flags;
	vcstate.output_mode = vop2_output_mode(vcstate.bus_format, vp->crtc_endpoint_id);
	if (vcstate.output_mode < 0) {
		dev_err(vp->vop2->dev, "Cannot determine output mode\n");
		return;
	}

	dev_info(vp->vop2->dev, "vp%d: bus_format: 0x%08x bus_flags: 0x%08x\n",
		 vp->id, vcstate.bus_format, display_info.bus_flags);

	vpl_ioctl_prepare(&vp->vpl, vp->id, info->mode);

	vop2_crtc_atomic_enable(vp, &mode, &vcstate);
	vop2_crtc_atomic_begin(vp);

	list_for_each_entry(win, &vp->windows, list) {
		if (!win->enabled)
			continue;
		vop2_plane_atomic_update(vp, win, win->dma, &win->src, &win->dst);
	}

	vop2_crtc_atomic_flush(vp, &mode);

	vpl_ioctl_enable(&vp->vpl, vp->id);
}


static void vop2_disable_controller(struct fb_info *info)
{
	struct vop2_win *win = container_of(info, struct vop2_win, info);
	struct vop2_video_port *vp = win->vp;

	vpl_ioctl_disable(&vp->vpl, vp->id);

	list_for_each_entry(win, &vp->windows, list) {
		if (!win->enabled)
			continue;

		vop2_plane_atomic_disable(win);
	}

	vop2_crtc_atomic_disable(vp);
}

static int vop2_activate_var(struct fb_info *info)
{
	struct vop2_win *win = container_of(info, struct vop2_win, info);
	struct vop2_video_port *vp = win->vp;
	struct vop2_win *w;

	info->line_length = vp->line_length;
	win->src.x2 = info->xres;
	win->src.y2 = info->yres;
	win->dst.x2 = info->xres;
	win->dst.y2 = info->yres;

	list_for_each_entry(w, &vp->windows, list) {
		if (w == vp->primary_plane)
			continue;
		w->info.xres = info->xres;
		w->info.yres = info->yres;

		w->src.x2 = info->xres;
		w->src.y2 = info->yres;
		w->dst.x2 = info->xres;
		w->dst.y2 = info->yres;
	}

	return 0;
}

static void vop2_enable_plane(struct fb_info *info)
{
	struct vop2_win *win = container_of(info, struct vop2_win, info);
	struct vop2 *vop2 = win->vop2;

	win->enabled = true;
	vop2_crtc_atomic_begin(win->vp);
	vop2_plane_atomic_update(&vop2->vps[0], win, win->dma, &win->src, &win->dst);
	vop2_cfg_done(win->vp);
}

static struct fb_ops vop2_fb_ops = {
	.fb_enable = vop2_enable_controller,
	.fb_disable = vop2_disable_controller,
	.fb_activate_var = vop2_activate_var,
};

static void vop2_disable_plane(struct fb_info *info)
{
	struct vop2_win *win = container_of(info, struct vop2_win, info);

	win->enabled = false;
	vop2_win_disable(win);
	vop2_cfg_done(win->vp);
}

static struct fb_ops vop2_fb_plane_ops = {
	.fb_enable = vop2_enable_plane,
	.fb_disable = vop2_disable_plane,
};

static struct fb_bitfield red    = { .offset = 16, .length = 8, };
static struct fb_bitfield green  = { .offset =  8, .length = 8, };
static struct fb_bitfield blue   = { .offset =  0, .length = 8, };
static struct fb_bitfield transp = { .offset = 24, .length = 8, };

static int vop2_register_plane(struct vop2_video_port *vp, struct vop2_win *win)
{
	struct vop2 *vop2 = win->vop2;
	struct fb_info *info = &win->info;
	int i, ret;
	static unsigned int zpos;
	u32 xmax = 0, ymax = 0;

	info->bits_per_pixel = 32;
	info->red    = red;
	info->green  = green;
	info->blue   = blue;
	info->transp = transp;
	info->dev.parent = vop2->dev;

	if (win->type == DRM_PLANE_TYPE_PRIMARY) {
		ret = vpl_ioctl(&vp->vpl, vp->id, VPL_GET_VIDEOMODES, &info->modes);
		if (ret) {
			dev_err(vop2->dev, "failed to get modes: %s\n", strerror(-ret));
			return ret;
		}

		if (info->modes.num_modes) {
			for (i = 0; i < info->modes.num_modes; i++) {
				xmax = max(xmax, info->modes.modes[i].xres);
				ymax = max(ymax, info->modes.modes[i].yres);
			}
			info->xres = info->modes.modes[info->modes.native_mode].xres;
			info->yres = info->modes.modes[info->modes.native_mode].yres;
		} else {
			dev_notice(vop2->dev, "no modes found on vp%d\n", vp->id);
			xmax = info->xres = 640;
			ymax = info->yres = 480;
		}

		vp->line_length = xmax * (info->bits_per_pixel >> 3);
		vp->max_yres = ymax;
		info->fbops = &vop2_fb_ops;
	} else {
		info->fbops = &vop2_fb_plane_ops;
		info->xres = vp->primary_plane->info.xres;
		info->yres = vp->primary_plane->info.yres;
		info->base_plane = &vp->primary_plane->info;
	}

	win->vp = vp;

	win->alpha = 0xffff;
	win->src.x1 = 0;
	win->src.y1 = 0;
	win->src.x2 = info->xres;
	win->src.y2 = info->yres;
	win->dst.x1 = 0;
	win->dst.y1 = 0;
	win->dst.x2 = info->xres;
	win->dst.y2 = info->yres;

	info->line_length = vp->line_length;
	info->screen_base = dma_alloc_writecombine(vp->line_length * vp->max_yres,
							&win->dma);
	if (!info->screen_base)
		return -ENOMEM;

	win->zpos = zpos++;

	list_add_tail(&win->list, &vp->windows);

	ret = register_framebuffer(&win->info);
	if (ret)
		return ret;

	win->name = xasprintf("vp%d-%s-%s", vp->id,
				win->type == DRM_PLANE_TYPE_PRIMARY ? "primary" : "overlay",
				win->data->name);
	dev_add_param_string_ro(&win->info.dev, "name", &win->name);

	dev_info(vop2->dev, "Registered %s on VP%d, window %s, type %s\n",
		 info->cdev.name, vp->id, win->data->name,
		 win->type == DRM_PLANE_TYPE_PRIMARY ? "primary" : "overlay");

	return 0;
}

static struct vop2_win *vop2_find_unused_win(struct vop2 *vop2)
{
	int i;

	for (i = 0; i < vop2->registered_num_wins; i++) {
		struct vop2_win *win = &vop2->win[i];

		if (!win->vp)
			return win;
	}

	return NULL;
}

static int vop2_create_crtcs(struct vop2 *vop2)
{
	const struct vop2_data *vop2_data = vop2->data;
	struct device *dev = vop2->dev;
	struct device_node *port;
	struct vop2_video_port *vp;
	int i, nvp, nvps = 0, ret, overlay_per_vp;

	for (i = 0; i < vop2_data->nr_vps; i++) {
		const struct vop2_video_port_data *vp_data;
		struct device_node *np, *ep;
		char dclk_name[9];
		struct of_endpoint endpoint;

		vp_data = &vop2_data->vp[i];
		vp = &vop2->vps[i];
		vp->vop2 = vop2;
		vp->id = vp_data->id;
		vp->data = vp_data;

		INIT_LIST_HEAD(&vp->windows);

		snprintf(dclk_name, sizeof(dclk_name), "dclk_vp%d", vp->id);
		vp->dclk = clk_get(vop2->dev, dclk_name);
		if (IS_ERR(vp->dclk)) {
			dev_err(vop2->dev, "failed to get %s\n", dclk_name);
			return PTR_ERR(vp->dclk);
		}

		np = of_graph_get_remote_node(dev->of_node, i, -1);
		if (!np) {
			dev_dbg(vop2->dev, "%s: No remote for vp%d\n", __func__, i);
			continue;
		}
		of_node_put(np);

		port = of_graph_get_port_by_id(dev->of_node, i);
		if (!port) {
			dev_err(vop2->dev, "no port node found for video_port%d\n", i);
			return -ENOENT;
		}

		for_each_child_of_node(port, ep) {
			of_graph_parse_endpoint(ep, &endpoint);
			vp->crtc_endpoint_id = endpoint.id;
			break;
		}

		vp->port = port;

		vp->vpl.node = dev->of_node;
                ret = vpl_register(&vp->vpl);
                if (ret)
                        return ret;

		nvps++;
	}

	if (!nvps) {
		/*
		 * Not exactly an error, but also no point in continuing when this
		 * happens
		 */
		dev_notice(vop2->dev, "No configured ports found\n");
		return 0;
	}

	nvp = 0;
	for (i = 0; i < vop2->registered_num_wins; i++) {
		struct vop2_win *win = &vop2->win[i];

		if (win->type == DRM_PLANE_TYPE_PRIMARY) {
			vp = find_vp_without_primary(vop2);
			if (vp) {
				vp->primary_plane = win;
				win->vp = vp;

				nvp++;
			} else {
				/* change the unused primary window to overlay window */
				win->type = DRM_PLANE_TYPE_OVERLAY;
			}
		}
	}

	overlay_per_vp = vop2->registered_num_wins / nvps - 1;

	dev_dbg(vop2->dev, "have %d ports and %d windows, register %d overlay(s) per vp\n",
		nvps, vop2->registered_num_wins, overlay_per_vp);

	for (i = 0; i < vop2->registered_num_wins; i++) {
		struct vop2_win *win = &vop2->win[i];
		struct vop2_video_port *vp = win->vp;
		int j;

		if (!vp)
			continue;

		if (win->type != DRM_PLANE_TYPE_PRIMARY)
			continue;

		ret = vop2_register_plane(vp, win);
		if (ret)
			continue;

		for (j = 0; j < overlay_per_vp; j++) {
			win = vop2_find_unused_win(vop2);
			if (!win)
				break;

			win->vp = vp;

			ret = vop2_register_plane(vp, win);
			if (ret)
				return ret;
		}
		vp->nlayers = j + 1;
	}

	return 0;
}

static struct reg_field vop2_esmart_regs[VOP2_WIN_MAX_REG] = {
	[VOP2_WIN_ENABLE] = REG_FIELD(RK3568_SMART_REGION0_CTRL, 0, 0),
	[VOP2_WIN_FORMAT] = REG_FIELD(RK3568_SMART_REGION0_CTRL, 1, 5),
	[VOP2_WIN_DITHER_UP] = REG_FIELD(RK3568_SMART_REGION0_CTRL, 12, 12),
	[VOP2_WIN_RB_SWAP] = REG_FIELD(RK3568_SMART_REGION0_CTRL, 14, 14),
	[VOP2_WIN_UV_SWAP] = REG_FIELD(RK3568_SMART_REGION0_CTRL, 16, 16),
	[VOP2_WIN_ACT_INFO] = REG_FIELD(RK3568_SMART_REGION0_ACT_INFO, 0, 31),
	[VOP2_WIN_DSP_INFO] = REG_FIELD(RK3568_SMART_REGION0_DSP_INFO, 0, 31),
	[VOP2_WIN_DSP_ST] = REG_FIELD(RK3568_SMART_REGION0_DSP_ST, 0, 28),
	[VOP2_WIN_YRGB_MST] = REG_FIELD(RK3568_SMART_REGION0_YRGB_MST, 0, 31),
	[VOP2_WIN_UV_MST] = REG_FIELD(RK3568_SMART_REGION0_CBR_MST, 0, 31),
	[VOP2_WIN_YUV_CLIP] = REG_FIELD(RK3568_SMART_REGION0_CTRL, 17, 17),
	[VOP2_WIN_YRGB_VIR] = REG_FIELD(RK3568_SMART_REGION0_VIR, 0, 15),
	[VOP2_WIN_UV_VIR] = REG_FIELD(RK3568_SMART_REGION0_VIR, 16, 31),
	[VOP2_WIN_Y2R_EN] = REG_FIELD(RK3568_SMART_CTRL0, 0, 0),
	[VOP2_WIN_R2Y_EN] = REG_FIELD(RK3568_SMART_CTRL0, 1, 1),
	[VOP2_WIN_CSC_MODE] = REG_FIELD(RK3568_SMART_CTRL0, 2, 3),
	[VOP2_WIN_YMIRROR] = REG_FIELD(RK3568_SMART_CTRL1, 31, 31),
	[VOP2_WIN_COLOR_KEY] = REG_FIELD(RK3568_SMART_COLOR_KEY_CTRL, 0, 29),
	[VOP2_WIN_COLOR_KEY_EN] = REG_FIELD(RK3568_SMART_COLOR_KEY_CTRL, 31, 31),

	/* Scale */
	[VOP2_WIN_SCALE_YRGB_X] = REG_FIELD(RK3568_SMART_REGION0_SCL_FACTOR_YRGB, 0, 15),
	[VOP2_WIN_SCALE_YRGB_Y] = REG_FIELD(RK3568_SMART_REGION0_SCL_FACTOR_YRGB, 16, 31),
	[VOP2_WIN_SCALE_CBCR_X] = REG_FIELD(RK3568_SMART_REGION0_SCL_FACTOR_CBR, 0, 15),
	[VOP2_WIN_SCALE_CBCR_Y] = REG_FIELD(RK3568_SMART_REGION0_SCL_FACTOR_CBR, 16, 31),
	[VOP2_WIN_YRGB_HOR_SCL_MODE] = REG_FIELD(RK3568_SMART_REGION0_SCL_CTRL, 0, 1),
	[VOP2_WIN_YRGB_HSCL_FILTER_MODE] = REG_FIELD(RK3568_SMART_REGION0_SCL_CTRL, 2, 3),
	[VOP2_WIN_YRGB_VER_SCL_MODE] = REG_FIELD(RK3568_SMART_REGION0_SCL_CTRL, 4, 5),
	[VOP2_WIN_YRGB_VSCL_FILTER_MODE] = REG_FIELD(RK3568_SMART_REGION0_SCL_CTRL, 6, 7),
	[VOP2_WIN_CBCR_HOR_SCL_MODE] = REG_FIELD(RK3568_SMART_REGION0_SCL_CTRL, 8, 9),
	[VOP2_WIN_CBCR_HSCL_FILTER_MODE] = REG_FIELD(RK3568_SMART_REGION0_SCL_CTRL, 10, 11),
	[VOP2_WIN_CBCR_VER_SCL_MODE] = REG_FIELD(RK3568_SMART_REGION0_SCL_CTRL, 12, 13),
	[VOP2_WIN_CBCR_VSCL_FILTER_MODE] = REG_FIELD(RK3568_SMART_REGION0_SCL_CTRL, 14, 15),
	[VOP2_WIN_BIC_COE_SEL] = REG_FIELD(RK3568_SMART_REGION0_SCL_CTRL, 16, 17),
	[VOP2_WIN_VSD_YRGB_GT2] = REG_FIELD(RK3568_SMART_REGION0_CTRL, 8, 8),
	[VOP2_WIN_VSD_YRGB_GT4] = REG_FIELD(RK3568_SMART_REGION0_CTRL, 9, 9),
	[VOP2_WIN_VSD_CBCR_GT2] = REG_FIELD(RK3568_SMART_REGION0_CTRL, 10, 10),
	[VOP2_WIN_VSD_CBCR_GT4] = REG_FIELD(RK3568_SMART_REGION0_CTRL, 11, 11),
	[VOP2_WIN_XMIRROR] = { .reg = 0xffffffff },
	[VOP2_WIN_CLUSTER_ENABLE] = { .reg = 0xffffffff },
	[VOP2_WIN_AFBC_ENABLE] = { .reg = 0xffffffff },
	[VOP2_WIN_CLUSTER_LB_MODE] = { .reg = 0xffffffff },
	[VOP2_WIN_AFBC_FORMAT] = { .reg = 0xffffffff },
	[VOP2_WIN_AFBC_RB_SWAP] = { .reg = 0xffffffff },
	[VOP2_WIN_AFBC_UV_SWAP] = { .reg = 0xffffffff },
	[VOP2_WIN_AFBC_AUTO_GATING_EN] = { .reg = 0xffffffff },
	[VOP2_WIN_AFBC_BLOCK_SPLIT_EN] = { .reg = 0xffffffff },
	[VOP2_WIN_AFBC_PIC_VIR_WIDTH] = { .reg = 0xffffffff },
	[VOP2_WIN_AFBC_TILE_NUM] = { .reg = 0xffffffff },
	[VOP2_WIN_AFBC_PIC_OFFSET] = { .reg = 0xffffffff },
	[VOP2_WIN_AFBC_PIC_SIZE] = { .reg = 0xffffffff },
	[VOP2_WIN_AFBC_DSP_OFFSET] = { .reg = 0xffffffff },
	[VOP2_WIN_AFBC_TRANSFORM_OFFSET] = { .reg = 0xffffffff },
	[VOP2_WIN_AFBC_HDR_PTR] = { .reg = 0xffffffff },
	[VOP2_WIN_AFBC_HALF_BLOCK_EN] = { .reg = 0xffffffff },
	[VOP2_WIN_AFBC_ROTATE_270] = { .reg = 0xffffffff },
	[VOP2_WIN_AFBC_ROTATE_90] = { .reg = 0xffffffff },
};

static const struct regmap_config vop2_win_regmap_config = {
	.reg_bits	= 32,
	.val_bits	= 32,
	.reg_stride	= 4,
	.max_register	= 0x100,
	.name		= "vop2-win",
};

static int vop2_esmart_init(struct vop2_win *win)
{
	struct vop2 *vop2 = win->vop2;
	struct reg_field *esmart_regs;
	int i, ret;

	esmart_regs = kmemdup(vop2_esmart_regs, sizeof(vop2_esmart_regs),
			      GFP_KERNEL);
	if (!esmart_regs)
		return -ENOMEM;

	for (i = 0; i < 0x100 / sizeof(u32); i++)
		win->regs[i] = readl(vop2->regs + win->offset + i * sizeof(u32));

	win->reg_field = esmart_regs;
	win->map = regmap_init_mmio(vop2->dev, win->regs, &vop2_win_regmap_config);
	if (IS_ERR(vop2->map))
		return PTR_ERR(vop2->map);

	ret = regmap_field_bulk_alloc(win->map, win->reg,
				      esmart_regs,
				      ARRAY_SIZE(vop2_esmart_regs));

	return ret;
};

static int vop2_win_init(struct vop2 *vop2)
{
	const struct vop2_data *vop2_data = vop2->data;
	struct vop2_win *win;
	int i, ret, n = 0;

	for (i = 0; i < vop2_data->win_size; i++) {
		const struct vop2_win_data *win_data = &vop2_data->win[i];

		if (vop2->data->soc_id == 3566) {
			/*
			 * On RK3566 these windows don't have an independent
			 * framebuffer. They share the framebuffer with smart0,
			 * esmart0 and cluster0 respectively.
			 */
			switch (win_data->phys_id) {
			case ROCKCHIP_VOP2_SMART1:
			case ROCKCHIP_VOP2_ESMART1:
			case ROCKCHIP_VOP2_CLUSTER1:
				continue;
			}
		}

		win = &vop2->win[n];
		win->data = win_data;
		win->type = win_data->type;
		win->offset = win_data->base;
		win->vop2 = vop2;

		ret = vop2_esmart_init(win);
		if (ret)
			return ret;
		n++;
	}

	vop2->registered_num_wins = n;

	return 0;
}

static const struct regmap_config vop2_regmap_config = {
	.reg_bits	= 32,
	.val_bits	= 32,
	.reg_stride	= 4,
	.max_register	= 0x3000,
	.name		= "vop2",
};

int vop2_bind(struct device *dev)
{
	const struct vop2_data *vop2_data;
	struct vop2 *vop2;
	struct resource *res;
	size_t alloc_size;
	int ret;

	vop2_data = device_get_match_data(dev);
	if (!vop2_data)
		return dev_err_probe(dev, -EINVAL, "No match data\n");

	/* Allocate vop2 struct and its vop2_win array */
	alloc_size = struct_size(vop2, win, vop2_data->win_size);
	vop2 = xzalloc(alloc_size);

	vop2->dev = dev;
	vop2->data = vop2_data;

	res = dev_get_resource_by_name(dev, IORESOURCE_MEM, "vop");
	if (!res)
		return dev_err_probe(vop2->dev, -EINVAL, "failed to get vop2 register byname\n");

	vop2->regs = IOMEM(res->start);
	if (IS_ERR(vop2->regs))
		return PTR_ERR(vop2->regs);
	vop2->len = resource_size(res);

	vop2->map = regmap_init_mmio(dev, vop2->regs, &vop2_regmap_config);
	if (IS_ERR(vop2->map))
		return PTR_ERR(vop2->map);

	ret = vop2_win_init(vop2);
	if (ret)
		return ret;

	if (vop2_data->feature & VOP2_FEATURE_HAS_SYS_GRF) {
		vop2->sys_grf = syscon_regmap_lookup_by_phandle(dev->of_node, "rockchip,grf");
		if (IS_ERR(vop2->sys_grf))
			return dev_err_probe(dev, PTR_ERR(vop2->sys_grf), "cannot get sys_grf");
	}

	if (vop2_data->feature & VOP2_FEATURE_HAS_VOP_GRF) {
		vop2->vop_grf = syscon_regmap_lookup_by_phandle(dev->of_node, "rockchip,vop-grf");
		if (IS_ERR(vop2->vop_grf))
			return dev_err_probe(dev, PTR_ERR(vop2->vop_grf), "cannot get vop_grf");
	}

	if (vop2_data->feature & VOP2_FEATURE_HAS_VO1_GRF) {
		vop2->vo1_grf = syscon_regmap_lookup_by_phandle(dev->of_node, "rockchip,vo1-grf");
		if (IS_ERR(vop2->vo1_grf))
			return dev_err_probe(dev, PTR_ERR(vop2->vo1_grf), "cannot get vo1_grf");
	}

	if (vop2_data->feature & VOP2_FEATURE_HAS_SYS_PMU) {
		vop2->sys_pmu = syscon_regmap_lookup_by_phandle(dev->of_node, "rockchip,pmu");
		if (IS_ERR(vop2->sys_pmu))
			return dev_err_probe(dev, PTR_ERR(vop2->sys_pmu), "cannot get sys_pmu");
	}

	vop2->hclk = clk_get(vop2->dev, "hclk");
	if (IS_ERR(vop2->hclk))
		return dev_err_probe(vop2->dev, PTR_ERR(vop2->hclk),
				     "failed to get hclk source\n");

	vop2->aclk = clk_get(vop2->dev, "aclk");
	if (IS_ERR(vop2->aclk))
		return dev_err_probe(vop2->dev, PTR_ERR(vop2->aclk),
				     "failed to get aclk source\n");

	vop2->pclk = clk_get_optional(vop2->dev, "pclk_vop");
	if (IS_ERR(vop2->pclk))
		return dev_err_probe(vop2->dev, PTR_ERR(vop2->pclk),
				     "failed to get pclk source\n");

	ret = vop2_create_crtcs(vop2);
	if (ret)
		return ret;

	return 0;
}
