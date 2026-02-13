// SPDX-License-Identifier: GPL-2.0-only
/*
 * Ilitek ILI9488 TFT LCD driver for WF35-320480UD panel.
 *
 * Supports MIPI DBI Type C Option 1 SPI interface with 9-bit transfers.
 *
 * Copyright (C) 2026 Giandomenico Rossi <rossi@amelchem.com>
 * Copyright (C) 2026 Nicola Fontana <ntd@entidi.it>
 * Copyright (C) 2026 Amel s.r.l
 */

#include <common.h>
#include <linux/bitops.h>
#include <linux/gpio/consumer.h>
#include <of.h>
#include <regulator.h>
#include <spi/spi.h>
#include <video/backlight.h>
#include <video/vpl.h>
#include <video/mipi_dbi.h>

#include <video/mipi_display.h>

/**
 * DOC: overview
 *
 * This driver supports the MIPI DBI Type C Option 1 SPI interface with
 * 9-bit transfers required by this panel.
 *
 * Features:
 * - 320x480 resolution with 18-bit RGB666 pixel format
 * - Custom SPI transfer using mipi_dbi_spi1_transfer() for 9-bit SPI support
 * - Framebuffer support with RGB565 to RGB666 conversion
 * - Backlight control via device tree backlight node
 *
 * This driver is designed specifically for the WF35-320480UD panel
 * by Amel s.r.l, which uses the Ilitek ILI9488 controller chip.
 *
 * Device Tree binding documentation:
 * Documentation/devicetree/bindings/video/panel-ilitek-ili9488.yaml
 */

/* Level 1 commands (from the display datasheet) */
#define ILI9488_CMD_NOP                             0x00
#define ILI9488_CMD_SOFTWARE_RESET                  0x01
#define ILI9488_CMD_READ_DISP_ID                    0x04
#define ILI9488_CMD_READ_ERROR_DSI                  0x05
#define ILI9488_CMD_READ_DISP_STATUS                0x09
#define ILI9488_CMD_READ_DISP_POWER_MODE            0x0A
#define ILI9488_CMD_READ_DISP_MADCTRL               0x0B
#define ILI9488_CMD_READ_DISP_PIXEL_FORMAT          0x0C
#define ILI9488_CMD_READ_DISP_IMAGE_MODE            0x0D
#define ILI9488_CMD_READ_DISP_SIGNAL_MODE           0x0E
#define ILI9488_CMD_READ_DISP_SELF_DIAGNOSTIC       0x0F
#define ILI9488_CMD_ENTER_SLEEP_MODE                0x10
#define ILI9488_CMD_SLEEP_OUT                       0x11
#define ILI9488_CMD_PARTIAL_MODE_ON                 0x12
#define ILI9488_CMD_NORMAL_DISP_MODE_ON             0x13
#define ILI9488_CMD_DISP_INVERSION_OFF              0x20
#define ILI9488_CMD_DISP_INVERSION_ON               0x21
#define ILI9488_CMD_PIXEL_OFF                       0x22
#define ILI9488_CMD_PIXEL_ON                        0x23
#define ILI9488_CMD_DISPLAY_OFF                     0x28
#define ILI9488_CMD_DISPLAY_ON                      0x29
#define ILI9488_CMD_COLUMN_ADDRESS_SET              0x2A
#define ILI9488_CMD_PAGE_ADDRESS_SET                0x2B
#define ILI9488_CMD_MEMORY_WRITE                    0x2C
#define ILI9488_CMD_MEMORY_READ                     0x2E
#define ILI9488_CMD_PARTIAL_AREA                    0x30
#define ILI9488_CMD_VERT_SCROLL_DEFINITION          0x33
#define ILI9488_CMD_TEARING_EFFECT_LINE_OFF         0x34
#define ILI9488_CMD_TEARING_EFFECT_LINE_ON          0x35
#define ILI9488_CMD_MEMORY_ACCESS_CONTROL           0x36
#define ILI9488_CMD_VERT_SCROLL_START_ADDRESS       0x37
#define ILI9488_CMD_IDLE_MODE_OFF                   0x38
#define ILI9488_CMD_IDLE_MODE_ON                    0x39
#define ILI9488_CMD_COLMOD_PIXEL_FORMAT_SET         0x3A
#define ILI9488_CMD_WRITE_MEMORY_CONTINUE           0x3C
#define ILI9488_CMD_READ_MEMORY_CONTINUE            0x3E
#define ILI9488_CMD_SET_TEAR_SCANLINE               0x44
#define ILI9488_CMD_GET_SCANLINE                    0x45
#define ILI9488_CMD_WRITE_DISPLAY_BRIGHTNESS        0x51
#define ILI9488_CMD_READ_DISPLAY_BRIGHTNESS         0x52
#define ILI9488_CMD_WRITE_CTRL_DISPLAY              0x53
#define ILI9488_CMD_READ_CTRL_DISPLAY               0x54
#define ILI9488_CMD_WRITE_CONTENT_ADAPT_BRIGHTNESS  0x55
#define ILI9488_CMD_READ_CONTENT_ADAPT_BRIGHTNESS   0x56
#define ILI9488_CMD_WRITE_MIN_CAB_LEVEL             0x5E
#define ILI9488_CMD_READ_MIN_CAB_LEVEL              0x5F
#define ILI9488_CMD_READ_ABC_SELF_DIAG_RES          0x68
#define ILI9488_CMD_READ_ID1                        0xDA
#define ILI9488_CMD_READ_ID2                        0xDB
#define ILI9488_CMD_READ_ID3                        0xDC

/* Level 2 commands (from the display datasheet) */
#define ILI9488_CMD_INTERFACE_MODE_CONTROL          0xB0
#define ILI9488_CMD_FRAME_RATE_CONTROL_NORMAL       0xB1
#define ILI9488_CMD_FRAME_RATE_CONTROL_IDLE_8COLOR  0xB2
#define ILI9488_CMD_FRAME_RATE_CONTROL_PARTIAL      0xB3
#define ILI9488_CMD_DISPLAY_INVERSION_CONTROL       0xB4
#define ILI9488_CMD_BLANKING_PORCH_CONTROL          0xB5
#define ILI9488_CMD_DISPLAY_FUNCTION_CONTROL        0xB6
#define ILI9488_CMD_ENTRY_MODE_SET                  0xB7
#define ILI9488_CMD_BACKLIGHT_CONTROL_1             0xB9
#define ILI9488_CMD_BACKLIGHT_CONTROL_2             0xBA
#define ILI9488_CMD_HS_LANES_CONTROL                0xBE
#define ILI9488_CMD_POWER_CONTROL_1                 0xC0
#define ILI9488_CMD_POWER_CONTROL_2                 0xC1
#define ILI9488_CMD_POWER_CONTROL_NORMAL_3          0xC2
#define ILI9488_CMD_POWER_CONTROL_IDEL_4            0xC3
#define ILI9488_CMD_POWER_CONTROL_PARTIAL_5         0xC4
#define ILI9488_CMD_VCOM_CONTROL_1                  0xC5
#define ILI9488_CMD_CABC_CONTROL_1                  0xC6
#define ILI9488_CMD_CABC_CONTROL_2                  0xC8
#define ILI9488_CMD_CABC_CONTROL_3                  0xC9
#define ILI9488_CMD_CABC_CONTROL_4                  0xCA
#define ILI9488_CMD_CABC_CONTROL_5                  0xCB
#define ILI9488_CMD_CABC_CONTROL_6                  0xCC
#define ILI9488_CMD_CABC_CONTROL_7                  0xCD
#define ILI9488_CMD_CABC_CONTROL_8                  0xCE
#define ILI9488_CMD_CABC_CONTROL_9                  0xCF
#define ILI9488_CMD_NVMEM_WRITE                     0xD0
#define ILI9488_CMD_NVMEM_PROTECTION_KEY            0xD1
#define ILI9488_CMD_NVMEM_STATUS_READ               0xD2
#define ILI9488_CMD_READ_ID4                        0xD3
#define ILI9488_CMD_ADJUST_CONTROL_1                0xD7
#define ILI9488_CMD_READ_ID_VERSION                 0xD8
#define ILI9488_CMD_POSITIVE_GAMMA_CORRECTION       0xE0
#define ILI9488_CMD_NEGATIVE_GAMMA_CORRECTION       0xE1
#define ILI9488_CMD_DIGITAL_GAMMA_CONTROL_1         0xE2
#define ILI9488_CMD_DIGITAL_GAMMA_CONTROL_2         0xE3
#define ILI9488_CMD_SET_IMAGE_FUNCTION              0xE9
#define ILI9488_CMD_ADJUST_CONTROL_2                0xF2
#define ILI9488_CMD_ADJUST_CONTROL_3                0xF7
#define ILI9488_CMD_ADJUST_CONTROL_4                0xF8
#define ILI9488_CMD_ADJUST_CONTROL_5                0xF9
#define ILI9488_CMD_SPI_READ_SETTINGS               0xFB
#define ILI9488_CMD_ADJUST_CONTROL_6                0xFC
#define ILI9488_CMD_ADJUST_CONTROL_7                0xFF

/* ILI9488 memory access control flags */
#define ILI9488_MY   BIT(7)   /* Row Address Order */
#define ILI9488_MX   BIT(6)   /* Column Address Order */
#define ILI9488_MV   BIT(5)   /* Row / Column Exchange */
#define ILI9488_ML   BIT(4)   /* Vertical Refresh Order */
#define ILI9488_BGR  BIT(3)   /* BGR Order, if set */
#define ILI9488_MH   BIT(2)   /* Horizontal Refresh Order */

static void rgb565_to_rgb666_line(void *dst, void *src, unsigned int pixels)
{
	unsigned int x;
	u8 *dst8 = dst;
	u16 *src16 = src;

	for (x = 0; x < pixels; x++) {
		*dst8++ = (*src16 & 0xF800) >> 8;
		*dst8++ = (*src16 & 0x07E0) >> 3;
		*dst8++ = (*src16 & 0x001F) << 3;
		src16++;
	}
}

/**
 * copy_buffer - Copy a framebuffer, transforming it to RGB666
 * @dst: The destination buffer
 * @info: The source framebuffer
 * @clip: Clipping rectangle of the area to be copied
 */
static void copy_buffer(u8 *dst, struct fb_info *info, struct fb_rect *clip)
{
	u16 *src = (u16 *)info->screen_base;
	unsigned int height = clip->y2 - clip->y1;
	unsigned int width = clip->x2 - clip->x1;
	int y;

	src += clip->y1 * info->xres + clip->x1;
	for (y = 0; y < height; y++) {
		rgb565_to_rgb666_line(dst, src, width);
		dst += info->xres * 3;   /* u8 single byte */
		src += info->xres;   /* u16 just doubled */
	}
}

static void ili9488_set_window_address(struct mipi_dbi_dev *dbidev,
				       unsigned int xs, unsigned int xe,
				       unsigned int ys, unsigned int ye)
{
	struct mipi_dbi *dbi = &dbidev->dbi;

	xs += dbidev->mode.left_margin;
	xe += dbidev->mode.left_margin;
	ys += dbidev->mode.upper_margin;
	ye += dbidev->mode.upper_margin;

	mipi_dbi_command(dbi, ILI9488_CMD_COLUMN_ADDRESS_SET, (xs >> 8) & 0xff,
			xs & 0xff, (xe >> 8) & 0xff, xe & 0xff);
	mipi_dbi_command(dbi, ILI9488_CMD_PAGE_ADDRESS_SET, (ys >> 8) & 0xff,
			ys & 0xff, (ye >> 8) & 0xff, ye & 0xff);
}

static void ili9488_fb_dirty(struct mipi_dbi_dev *dbidev,
			     struct fb_info *info, struct fb_rect *rect)
{
	unsigned int height = rect->y2 - rect->y1;
	unsigned int width = rect->x2 - rect->x1;
	struct mipi_dbi *dbi = &dbidev->dbi;
	int ret;
	void *tr;

	tr = dbidev->tx_buf;
	copy_buffer(tr, info, rect);

	ili9488_set_window_address(dbidev, rect->x1, rect->x2 - 1, rect->y1, rect->y2 - 1);
	ret = mipi_dbi_command_buf(dbi, ILI9488_CMD_MEMORY_WRITE, tr, width * height * 3);
	if (ret)
		pr_err_once("Failed to update display %d\n", ret);

	dbidev->damage.x1 = 0;
	dbidev->damage.y1 = 0;
	dbidev->damage.x2 = 0;
	dbidev->damage.y2 = 0;
}

static void ili9488_fb_flush(struct fb_info *info)
{
	struct mipi_dbi_dev *dbidev = container_of(info, struct mipi_dbi_dev, info);

	if (!dbidev->damage.x2 || !dbidev->damage.y2) {
		dbidev->damage.x1 = 0;
		dbidev->damage.y1 = 0;
		dbidev->damage.x2 = info->xres;
		dbidev->damage.y2 = info->yres;
	}

	ili9488_fb_dirty(dbidev, info, &dbidev->damage);
}

static void ili9488_fb_enable(struct fb_info *info)
{
	struct mipi_dbi_dev *dbidev = container_of(info, struct mipi_dbi_dev, info);
	struct mipi_dbi *dbi = &dbidev->dbi;
	u8 addr_mode;
	int ret;

	if (!info->mode) {
		dev_err(dbidev->dev, "No valid mode found\n");
		return;
	}

	if (dbidev->backlight_node && !dbidev->backlight) {
		dbidev->backlight = of_backlight_find(dbidev->backlight_node);
		if (!dbidev->backlight)
			dev_warn(dbidev->dev, "backlight not available\n");
		else
			backlight_set_brightness_default(dbidev->backlight);
	}

	ret = mipi_dbi_poweron_conditional_reset(dbidev);
	if (ret < 0) {
		dev_err(dbidev->dev, "power_on_conditional error\n");
		return;
	}

	/* already powerd */
	if (ret == 1)
		dev_dbg(dbidev->dev, "display was not reset, already powered on\n");

	mipi_dbi_command(dbi, ILI9488_CMD_DISP_INVERSION_ON);
	mipi_dbi_command(dbi, ILI9488_CMD_POWER_CONTROL_NORMAL_3, 0x33);
	mipi_dbi_command(dbi, ILI9488_CMD_FRAME_RATE_CONTROL_NORMAL, 0xB0);
	mipi_dbi_command(dbi, ILI9488_CMD_MEMORY_ACCESS_CONTROL, 0x28);

	mipi_dbi_command(dbi, MIPI_DCS_SET_PIXEL_FORMAT, MIPI_DCS_PIXEL_FMT_18BIT);
	mipi_dbi_command(dbi, MIPI_DCS_EXIT_SLEEP_MODE);
	mdelay(120);
	mipi_dbi_command(dbi, MIPI_DCS_SET_DISPLAY_ON);
	mdelay(100);

	switch (dbidev->rotation) {
	case 270:
		addr_mode = ILI9488_MX | ILI9488_MY | ILI9488_MV | ILI9488_ML;
		break;
	case 180:
		addr_mode = ILI9488_MY | ILI9488_ML;
		break;
	case 90:
		addr_mode = ILI9488_MV;
		break;
	case 0:
	default:
		addr_mode = ILI9488_MX;
		break;
	}

	addr_mode |= ILI9488_BGR;
	mipi_dbi_command(dbi, ILI9488_CMD_MEMORY_ACCESS_CONTROL, addr_mode);

	ili9488_fb_flush(info);
}

static struct fb_ops ili9488_fb_ops = {
	.fb_enable = ili9488_fb_enable,
	.fb_disable = mipi_dbi_fb_disable,
	.fb_damage = mipi_dbi_fb_damage,
	.fb_flush =  ili9488_fb_flush,
};

static int ili9488_dbi_dev_init(struct mipi_dbi_dev *dbidev,
				struct fb_ops *ops,
				struct fb_videomode *mode)
{
	struct fb_info *info = &dbidev->info;

	info->mode = mode;
	info->fbops = ops;
	info->dev.parent = dbidev->dev;

	info->xres = mode->xres;
	info->yres = mode->yres;

	info->bits_per_pixel = 16;
	info->line_length = info->xres * 2;
	info->screen_size = info->xres * info->yres * 2;
	info->screen_base = dma_zalloc(info->screen_size);
	if (!info->screen_base)
		return -ENOMEM;

	dbidev->tx_buf = dma_alloc(info->xres * info->yres * 3);
	if (!dbidev->tx_buf)
		return -ENOMEM;

	info->red.length = 5;
	info->red.offset = 11;

	info->green.length = 6;
	info->green.offset = 5;

	info->blue.length = 5;
	info->blue.offset = 0;

	dev_dbg(dbidev->dev,
			"bit per pixel: %d\n"
			"line_length: %d\n"
			"screen_base: %p\n"
			"screen_size: %ld\n"
			"tx_buf %d\n",
			info->bits_per_pixel,
			info->line_length,
			info->screen_base,
			info->screen_size,
			info->xres * info->yres * 3);

	return 0;
}

/* SPI */
static int ili9488_typec1_command(struct mipi_dbi *dbi,
				  u8 *cmd, u8 *parameters,
				  size_t num)
{
	int ret;

	ret = mipi_dbi_spi1_transfer(dbi, 0, cmd, 1, 8);
	if (ret || !num)
		return ret;

	/*
	 * ILI9488 uses RGB666 (3 bytes per pixel) so pixel data is
	 * transferred as a byte stream, not as 16-bit words.
	 */
	return mipi_dbi_spi1_transfer(dbi, 1, parameters, num, 8);
}

static int ili9488_dbi_get_mode(struct mipi_dbi_dev *dbidev,
				struct fb_videomode *mode)
{
	struct device *dev = dbidev->dev;
	int ret;

	ret = of_get_display_timing(dev->of_node, "panel-timing", mode);
	if (ret) {
		dev_err(dev, "%pOF: failed to get panel-timing (error=%d)\n",
			dev->of_node, ret);
		return ret;
	}

	/*
	 * Make sure width and height are set and that only back porch and
	 * pixelclock are set in the other timing values. Also check that
	 * width and height don't exceed the 16-bit value specified by MIPI DCS.
	 */
	if (!mode->xres || !mode->yres || mode->display_flags ||
		mode->right_margin || mode->hsync_len ||
		(mode->left_margin + mode->xres) > 0xffff ||
		mode->lower_margin || mode->vsync_len ||
		(mode->upper_margin + mode->yres) > 0xffff) {
		dev_err(dev, "%pOF: panel-timing out of bounds\n", dev->of_node);
		return -EINVAL;
	}

	return 0;
}

static int ili9488_probe(struct device *dev)
{
	struct mipi_dbi_dev *dbidev;
	struct spi_device *spi = to_spi_device(dev);
	struct mipi_dbi *dbi;
	struct fb_info *info;
	int ret;
	struct fb_videomode *ili9488_mode;

	dbidev = kzalloc(sizeof(*dbidev), GFP_KERNEL);
	if (!dbidev)
		return -ENOMEM;

	dbidev->dev = dev;
	dbi = &dbidev->dbi;

	ret = ili9488_dbi_get_mode(dbidev, &dbidev->mode);
	if (ret)
		return ret;

	ili9488_mode = &dbidev->mode;
	dbidev->regulator = regulator_get(dev, "power");
	if (IS_ERR(dbidev->regulator))
		return dev_err_probe(dev, PTR_ERR(dbidev->regulator),
							 "Failed to get regulator 'power'\n");

	dbidev->backlight_node = of_parse_phandle(dev->of_node, "backlight", 0);

	dbi->reset = gpiod_get_optional(dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(dbi->reset))
		return dev_errp_probe(dev, dbi->reset, "Failed to get GPIO 'reset'\n");

	ret = of_property_read_u32(dev->of_node, "rotation", &dbidev->rotation);
	if (ret) {
		dev_warn(dev, "Missing LCD rotation property. Set to 270 degs\n");
		dbidev->rotation = 270;
	} else {
		dev_dbg(dev, "LCD rotation: %d",  dbidev->rotation);
	}

	/* spi */
	ret = mipi_dbi_spi_init(spi, dbi, NULL);
	if (ret)
		return ret;

	dbi->read_commands = NULL;

	/*
	 * SPI transfer function need an override. Display wf35 accepts only
	 * RGB111 or RGB666 and the last is 24 bit transfer per pixel so
	 * mipi dbi typec1 requires 24 bits per word.
	 */
	dbi->command = ili9488_typec1_command;

	info = &dbidev->info;
	ret = ili9488_dbi_dev_init(dbidev, &ili9488_fb_ops, ili9488_mode);
	if (ret)
		return ret;

	ret = register_framebuffer(info);
	if (ret < 0)
		return dev_err_probe(dev, ret, "Failed to register framebuffer\n");

	dev_dbg(dev, "Driver ilitek 9488 loaded\n");
	return 0;
}

static const struct of_device_id ili9488_of_match[] = {
	{
		.compatible = "winstar,wf35-320480ud",
	}, {
		.compatible = "ilitek,ili9488",
	},
	{ }
};
MODULE_DEVICE_TABLE(of, ili9488_of_match);

static struct driver ili9488_driver = {
	.name = "ili9488",
	.of_compatible = ili9488_of_match,
	.probe = ili9488_probe,
};
device_spi_driver(ili9488_driver);

MODULE_AUTHOR("Giandomenico Rossi <rossi@amelchem.com>");
MODULE_DESCRIPTION("ILI9488 LCD panel driver");
MODULE_LICENSE("GPL v2");
