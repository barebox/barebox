// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) STMicroelectronics SA 2017
 *
 * Authors: Philippe Cornu <philippe.cornu@st.com>
 *          Yannick Fertre <yannick.fertre@st.com>
 */

#include <video/backlight.h>
#include <video/vpl.h>
#include <clock.h>
#include <linux/kernel.h>
#include <linux/gpio/consumer.h>
#include <video/media-bus-format.h>
#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <regulator.h>

#include <video/mipi_display.h>

#include <video/mipi_dsi.h>
#include <video/mipi_display.h>
#include <video/drm/drm_modes.h>
#include <video/videomode.h>

#define OTM8009A_BACKLIGHT_DEFAULT	240
#define OTM8009A_BACKLIGHT_MAX		255

/* Manufacturer Command Set */
#define MCS_ADRSFT	0x0000	/* Address Shift Function */
#define MCS_PANSET	0xB3A6	/* Panel Type Setting */
#define MCS_SD_CTRL	0xC0A2	/* Source Driver Timing Setting */
#define MCS_P_DRV_M	0xC0B4	/* Panel Driving Mode */
#define MCS_OSC_ADJ	0xC181	/* Oscillator Adjustment for Idle/Normal mode */
#define MCS_RGB_VID_SET	0xC1A1	/* RGB Video Mode Setting */
#define MCS_SD_PCH_CTRL	0xC480	/* Source Driver Precharge Control */
#define MCS_NO_DOC1	0xC48A	/* Command not documented */
#define MCS_PWR_CTRL1	0xC580	/* Power Control Setting 1 */
#define MCS_PWR_CTRL2	0xC590	/* Power Control Setting 2 for Normal Mode */
#define MCS_PWR_CTRL4	0xC5B0	/* Power Control Setting 4 for DC Voltage */
#define MCS_PANCTRLSET1	0xCB80	/* Panel Control Setting 1 */
#define MCS_PANCTRLSET2	0xCB90	/* Panel Control Setting 2 */
#define MCS_PANCTRLSET3	0xCBA0	/* Panel Control Setting 3 */
#define MCS_PANCTRLSET4	0xCBB0	/* Panel Control Setting 4 */
#define MCS_PANCTRLSET5	0xCBC0	/* Panel Control Setting 5 */
#define MCS_PANCTRLSET6	0xCBD0	/* Panel Control Setting 6 */
#define MCS_PANCTRLSET7	0xCBE0	/* Panel Control Setting 7 */
#define MCS_PANCTRLSET8	0xCBF0	/* Panel Control Setting 8 */
#define MCS_PANU2D1	0xCC80	/* Panel U2D Setting 1 */
#define MCS_PANU2D2	0xCC90	/* Panel U2D Setting 2 */
#define MCS_PANU2D3	0xCCA0	/* Panel U2D Setting 3 */
#define MCS_PAND2U1	0xCCB0	/* Panel D2U Setting 1 */
#define MCS_PAND2U2	0xCCC0	/* Panel D2U Setting 2 */
#define MCS_PAND2U3	0xCCD0	/* Panel D2U Setting 3 */
#define MCS_GOAVST	0xCE80	/* GOA VST Setting */
#define MCS_GOACLKA1	0xCEA0	/* GOA CLKA1 Setting */
#define MCS_GOACLKA3	0xCEB0	/* GOA CLKA3 Setting */
#define MCS_GOAECLK	0xCFC0	/* GOA ECLK Setting */
#define MCS_NO_DOC2	0xCFD0	/* Command not documented */
#define MCS_GVDDSET	0xD800	/* GVDD/NGVDD */
#define MCS_VCOMDC	0xD900	/* VCOM Voltage Setting */
#define MCS_GMCT2_2P	0xE100	/* Gamma Correction 2.2+ Setting */
#define MCS_GMCT2_2N	0xE200	/* Gamma Correction 2.2- Setting */
#define MCS_NO_DOC3	0xF5B6	/* Command not documented */
#define MCS_CMD2_ENA1	0xFF00	/* Enable Access Command2 "CMD2" */
#define MCS_CMD2_ENA2	0xFF80	/* Enable Access Orise Command2 */

#define OTM8009A_HDISPLAY	480
#define OTM8009A_VDISPLAY	800

struct otm8009a {
	struct device *dev;
	struct vpl vpl;
	struct backlight_device bl_dev;
	struct gpio_desc *reset_gpio;
	struct regulator *supply;
	bool prepared;
};

static const struct drm_display_mode modes[] = {
	{ /* 50 Hz, preferred */
		.clock = 29700,
		.hdisplay = 480,
		.hsync_start = 480 + 98,
		.hsync_end = 480 + 98 + 32,
		.htotal = 480 + 98 + 32 + 98,
		.vdisplay = 800,
		.vsync_start = 800 + 15,
		.vsync_end = 800 + 15 + 10,
		.vtotal = 800 + 15 + 10 + 14,
		.flags = DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC,
		.width_mm = 52,
		.height_mm = 86,
	},
	{ /* 60 Hz */
		.clock = 33000,
		.hdisplay = 480,
		.hsync_start = 480 + 70,
		.hsync_end = 480 + 70 + 32,
		.htotal = 480 + 70 + 32 + 72,
		.vdisplay = 800,
		.vsync_start = 800 + 15,
		.vsync_end = 800 + 15 + 10,
		.vtotal = 800 + 15 + 10 + 16,
		.flags = DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC,
		.width_mm = 52,
		.height_mm = 86,
	},
};

static inline struct otm8009a *panel_to_otm8009a(struct vpl *vpl)
{
	return container_of(vpl, struct otm8009a, vpl);
}

static void otm8009a_dcs_write_buf(struct otm8009a *ctx, const void *data,
				   size_t len)
{
	struct mipi_dsi_device *dsi = to_mipi_dsi_device(ctx->dev);

	if (mipi_dsi_dcs_write_buffer(dsi, data, len) < 0)
		dev_warn(ctx->dev, "mipi dsi dcs %swrite buffer [0x%02x..] failed\n",
			 dsi->mode_flags & MIPI_DSI_MODE_LPM ? "" : "hs ",
			 *(u8 *)data);
}

#define dcs_write_seq(ctx, seq...)			\
({							\
	static const u8 d[] = { seq };			\
	otm8009a_dcs_write_buf(ctx, d, ARRAY_SIZE(d));	\
})

#define dcs_write_cmd_at(ctx, cmd, seq...)		\
({							\
	dcs_write_seq(ctx, MCS_ADRSFT, (cmd) & 0xFF);	\
	dcs_write_seq(ctx, (cmd) >> 8, seq);		\
})

static int otm8009a_init_sequence(struct otm8009a *ctx)
{
	struct mipi_dsi_device *dsi = to_mipi_dsi_device(ctx->dev);
	int ret;

	/* Enter CMD2 */
	dcs_write_cmd_at(ctx, MCS_CMD2_ENA1, 0x80, 0x09, 0x01);

	/* Enter Orise Command2 */
	dcs_write_cmd_at(ctx, MCS_CMD2_ENA2, 0x80, 0x09);

	dcs_write_cmd_at(ctx, MCS_SD_PCH_CTRL, 0x30);
	mdelay(10);

	dcs_write_cmd_at(ctx, MCS_NO_DOC1, 0x40);
	mdelay(10);

	dcs_write_cmd_at(ctx, MCS_PWR_CTRL4 + 1, 0xA9);
	dcs_write_cmd_at(ctx, MCS_PWR_CTRL2 + 1, 0x34);
	dcs_write_cmd_at(ctx, MCS_P_DRV_M, 0x50);
	dcs_write_cmd_at(ctx, MCS_VCOMDC, 0x4E);
	dcs_write_cmd_at(ctx, MCS_OSC_ADJ, 0x66); /* 65Hz */
	dcs_write_cmd_at(ctx, MCS_PWR_CTRL2 + 2, 0x01);
	dcs_write_cmd_at(ctx, MCS_PWR_CTRL2 + 5, 0x34);
	dcs_write_cmd_at(ctx, MCS_PWR_CTRL2 + 4, 0x33);
	dcs_write_cmd_at(ctx, MCS_GVDDSET, 0x79, 0x79);
	dcs_write_cmd_at(ctx, MCS_SD_CTRL + 1, 0x1B);
	dcs_write_cmd_at(ctx, MCS_PWR_CTRL1 + 2, 0x83);
	dcs_write_cmd_at(ctx, MCS_SD_PCH_CTRL + 1, 0x83);
	dcs_write_cmd_at(ctx, MCS_RGB_VID_SET, 0x0E);
	dcs_write_cmd_at(ctx, MCS_PANSET, 0x00, 0x01);

	dcs_write_cmd_at(ctx, MCS_GOAVST, 0x85, 0x01, 0x00, 0x84, 0x01, 0x00);
	dcs_write_cmd_at(ctx, MCS_GOACLKA1, 0x18, 0x04, 0x03, 0x39, 0x00, 0x00,
			 0x00, 0x18, 0x03, 0x03, 0x3A, 0x00, 0x00, 0x00);
	dcs_write_cmd_at(ctx, MCS_GOACLKA3, 0x18, 0x02, 0x03, 0x3B, 0x00, 0x00,
			 0x00, 0x18, 0x01, 0x03, 0x3C, 0x00, 0x00, 0x00);
	dcs_write_cmd_at(ctx, MCS_GOAECLK, 0x01, 0x01, 0x20, 0x20, 0x00, 0x00,
			 0x01, 0x02, 0x00, 0x00);

	dcs_write_cmd_at(ctx, MCS_NO_DOC2, 0x00);

	dcs_write_cmd_at(ctx, MCS_PANCTRLSET1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	dcs_write_cmd_at(ctx, MCS_PANCTRLSET2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			 0, 0, 0, 0, 0);
	dcs_write_cmd_at(ctx, MCS_PANCTRLSET3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			 0, 0, 0, 0, 0);
	dcs_write_cmd_at(ctx, MCS_PANCTRLSET4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	dcs_write_cmd_at(ctx, MCS_PANCTRLSET5, 0, 4, 4, 4, 4, 4, 0, 0, 0, 0,
			 0, 0, 0, 0, 0);
	dcs_write_cmd_at(ctx, MCS_PANCTRLSET6, 0, 0, 0, 0, 0, 0, 4, 4, 4, 4,
			 4, 0, 0, 0, 0);
	dcs_write_cmd_at(ctx, MCS_PANCTRLSET7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	dcs_write_cmd_at(ctx, MCS_PANCTRLSET8, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			 0xFF, 0xFF, 0xFF, 0xFF, 0xFF);

	dcs_write_cmd_at(ctx, MCS_PANU2D1, 0x00, 0x26, 0x09, 0x0B, 0x01, 0x25,
			 0x00, 0x00, 0x00, 0x00);
	dcs_write_cmd_at(ctx, MCS_PANU2D2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			 0x00, 0x00, 0x00, 0x00, 0x00, 0x26, 0x0A, 0x0C, 0x02);
	dcs_write_cmd_at(ctx, MCS_PANU2D3, 0x25, 0x00, 0x00, 0x00, 0x00, 0x00,
			 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
	dcs_write_cmd_at(ctx, MCS_PAND2U1, 0x00, 0x25, 0x0C, 0x0A, 0x02, 0x26,
			 0x00, 0x00, 0x00, 0x00);
	dcs_write_cmd_at(ctx, MCS_PAND2U2, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			 0x00, 0x00, 0x00, 0x00, 0x00, 0x25, 0x0B, 0x09, 0x01);
	dcs_write_cmd_at(ctx, MCS_PAND2U3, 0x26, 0x00, 0x00, 0x00, 0x00, 0x00,
			 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

	dcs_write_cmd_at(ctx, MCS_PWR_CTRL1 + 1, 0x66);

	dcs_write_cmd_at(ctx, MCS_NO_DOC3, 0x06);

	dcs_write_cmd_at(ctx, MCS_GMCT2_2P, 0x00, 0x09, 0x0F, 0x0E, 0x07, 0x10,
			 0x0B, 0x0A, 0x04, 0x07, 0x0B, 0x08, 0x0F, 0x10, 0x0A,
			 0x01);
	dcs_write_cmd_at(ctx, MCS_GMCT2_2N, 0x00, 0x09, 0x0F, 0x0E, 0x07, 0x10,
			 0x0B, 0x0A, 0x04, 0x07, 0x0B, 0x08, 0x0F, 0x10, 0x0A,
			 0x01);

	/* Exit CMD2 */
	dcs_write_cmd_at(ctx, MCS_CMD2_ENA1, 0xFF, 0xFF, 0xFF);

	ret = mipi_dsi_dcs_nop(dsi);
	if (ret)
		return ret;

	ret = mipi_dsi_dcs_exit_sleep_mode(dsi);
	if (ret)
		return ret;

	/* Wait for sleep out exit */
	mdelay(120);

	/* Default portrait 480x800 rgb24 */
	dcs_write_seq(ctx, MIPI_DCS_SET_ADDRESS_MODE, 0x00);

	ret = mipi_dsi_dcs_set_column_address(dsi, 0, OTM8009A_HDISPLAY - 1);
	if (ret)
		return ret;

	ret = mipi_dsi_dcs_set_page_address(dsi, 0, OTM8009A_VDISPLAY - 1);
	if (ret)
		return ret;

	/* See otm8009a driver documentation for pixel format descriptions */
	ret = mipi_dsi_dcs_set_pixel_format(dsi, MIPI_DCS_PIXEL_FMT_24BIT |
					    MIPI_DCS_PIXEL_FMT_24BIT << 4);
	if (ret)
		return ret;

	/* Disable CABC feature */
	dcs_write_seq(ctx, MIPI_DCS_WRITE_POWER_SAVE, 0x00);

	ret = mipi_dsi_dcs_set_display_on(dsi);
	if (ret)
		return ret;

	ret = mipi_dsi_dcs_nop(dsi);
	if (ret)
		return ret;

	/* Send Command GRAM memory write (no parameters) */
	dcs_write_seq(ctx, MIPI_DCS_WRITE_MEMORY_START);

	/* Wait a short while to let the panel be ready before the 1st frame */
	mdelay(10);

	return 0;
}

static int otm8009a_disable(struct otm8009a *ctx)
{
	struct mipi_dsi_device *dsi = to_mipi_dsi_device(ctx->dev);
	int ret;

	backlight_disable(&ctx->bl_dev);

	ret = mipi_dsi_dcs_set_display_off(dsi);
	if (ret)
		return ret;

	ret = mipi_dsi_dcs_enter_sleep_mode(dsi);
	if (ret)
		return ret;

	mdelay(120);

	return 0;
}

static int otm8009a_unprepare(struct otm8009a *ctx)
{
	if (ctx->reset_gpio) {
		gpiod_set_value_cansleep(ctx->reset_gpio, 1);
		mdelay(20);
	}

	regulator_disable(ctx->supply);

	ctx->prepared = false;

	return 0;
}

static int otm8009a_prepare(struct otm8009a *ctx)
{
	int ret;

	ret = regulator_enable(ctx->supply);
	if (ret < 0) {
		dev_err(ctx->dev, "failed to enable supply: %pe\n", ERR_PTR(ret));
		return ret;
	}

	if (ctx->reset_gpio) {
		gpiod_set_value_cansleep(ctx->reset_gpio, 0);
		gpiod_set_value_cansleep(ctx->reset_gpio, 1);
		mdelay(1); /* >50us */
		gpiod_set_value_cansleep(ctx->reset_gpio, 0);
		mdelay(10); /* >5ms */
	}

	ret = otm8009a_init_sequence(ctx);
	if (ret)
		return ret;

	ctx->prepared = true;

	return 0;
}

static int otm8009a_enable(struct otm8009a *ctx)
{
	backlight_enable(&ctx->bl_dev);

	return 0;
}

static int otm8009a_get_modes(struct otm8009a *ctx,
			      struct display_timings *timings)
{
	struct fb_videomode *video_modes;
	unsigned int num_modes = ARRAY_SIZE(modes);
	unsigned int i;

	video_modes = calloc(num_modes, sizeof(*video_modes));
	if (!video_modes)
		return -ENOMEM;

	for (i = 0; i < num_modes; i++)
		drm_display_mode_to_fb_videomode(&modes[i], &video_modes[i]);

	timings->modes = video_modes;
	timings->num_modes = num_modes;

	return 0;
}

static int otm8009a_ioctl(struct vpl *vpl, unsigned int port,
			  unsigned int cmd, void *ptr)
{
	struct otm8009a *ctx = panel_to_otm8009a(vpl);

	switch (cmd) {
	case VPL_PREPARE:
		return otm8009a_prepare(ctx);
	case VPL_ENABLE:
		return otm8009a_enable(ctx);
	case VPL_DISABLE:
		return otm8009a_disable(ctx);
	case VPL_UNPREPARE:
		return otm8009a_unprepare(ctx);
	case VPL_GET_VIDEOMODES:
		return otm8009a_get_modes(ctx, ptr);
	case VPL_GET_BUS_FORMAT:
		*(u32 *)ptr = MEDIA_BUS_FMT_RGB888_1X24;
		return 0;
	default:
		return 0;
	}
}

/*
 * DSI-BASED BACKLIGHT
 */

static int otm8009a_backlight_update_status(struct backlight_device *bd,
					    int brightness)
{
	struct otm8009a *ctx = container_of(bd, struct otm8009a, bl_dev);
	u8 data[2];

	if (!ctx->prepared) {
		dev_dbg(&bd->dev, "lcd not ready yet for setting its backlight!\n");
		return -ENXIO;
	}

	if (brightness > 0) {
		/* Power on the backlight with the requested brightness
		 * Note We can not use mipi_dsi_dcs_set_display_brightness()
		 * as otm8009a driver support only 8-bit brightness (1 param).
		 */
		data[0] = MIPI_DCS_SET_DISPLAY_BRIGHTNESS;
		data[1] = brightness;
		otm8009a_dcs_write_buf(ctx, data, ARRAY_SIZE(data));

		/* set Brightness Control & Backlight on */
		data[1] = 0x24;

	} else {
		/* Power off the backlight: set Brightness Control & Bl off */
		data[1] = 0;
	}

	/* Update Brightness Control & Backlight */
	data[0] = MIPI_DCS_WRITE_CONTROL_DISPLAY;
	otm8009a_dcs_write_buf(ctx, data, ARRAY_SIZE(data));

	/* Need to wait a few time before sending the first image */
	mdelay(10);

	return 0;
}

static int otm8009a_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct backlight_device *bl_dev;
	struct otm8009a *ctx;
	int ret;

	ctx = devm_kzalloc(dev, sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	ctx->reset_gpio = gpiod_get_optional(dev, "reset", GPIOD_OUT_LOW);
	if (IS_ERR(ctx->reset_gpio)) {
		dev_err(dev, "cannot get reset-gpio\n");
		return PTR_ERR(ctx->reset_gpio);
	}

	ctx->supply = regulator_get(dev, "power");
	if (IS_ERR(ctx->supply)) {
		ret = PTR_ERR(ctx->supply);
		if (ret != -EPROBE_DEFER)
			dev_err(dev, "failed to request regulator: %pe\n", ERR_PTR(ret));
		return ret;
	}

	mipi_dsi_set_drvdata(dsi, ctx);

	ctx->dev = dev;

	dsi->lanes = 2;
	dsi->format = MIPI_DSI_FMT_RGB888;
	dsi->mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_BURST |
			  MIPI_DSI_MODE_LPM | MIPI_DSI_CLOCK_NON_CONTINUOUS;

	ctx->vpl.node = dev->of_node;
	ctx->vpl.ioctl = otm8009a_ioctl;

	ret = vpl_register(&ctx->vpl);
	if (ret)
		return ret;

	ret = mipi_dsi_attach(dsi);
	if (ret < 0) {
		dev_err(dev, "mipi_dsi_attach failed. Is host ready?\n");
		return ret;
	}

	bl_dev = &ctx->bl_dev;

	bl_dev->brightness_max = OTM8009A_BACKLIGHT_MAX;
	bl_dev->brightness_default = OTM8009A_BACKLIGHT_DEFAULT;
	bl_dev->brightness_set = otm8009a_backlight_update_status;
	bl_dev->dev.parent = dev;
	bl_dev->node = dev->of_node;

	ret = backlight_register(bl_dev);
	if (ret) {
		dev_err(dev, "failed to register backlight: %pe\n", ERR_PTR(ret));
		return ret;
	}

	return 0;
}

static const struct of_device_id orisetech_otm8009a_of_match[] = {
	{ .compatible = "orisetech,otm8009a" },
	{ }
};
MODULE_DEVICE_TABLE(of, orisetech_otm8009a_of_match);

static struct mipi_dsi_driver orisetech_otm8009a_driver = {
	.probe  = otm8009a_probe,
	.driver = {
		.name = "panel-orisetech-otm8009a",
		.of_match_table = orisetech_otm8009a_of_match,
	},
};
module_mipi_dsi_driver(orisetech_otm8009a_driver);

MODULE_AUTHOR("Philippe Cornu <philippe.cornu@st.com>");
MODULE_AUTHOR("Yannick Fertre <yannick.fertre@st.com>");
MODULE_DESCRIPTION("DRM driver for Orise Tech OTM8009A MIPI DSI panel");
MODULE_LICENSE("GPL v2");
