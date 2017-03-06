/*
 * Copyright 2011 Intel Corporation; author Matt Fleming
 * Copyright (c) 2017 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * GPL v2
 */

#include <common.h>
#include <driver.h>
#include <init.h>
#include <malloc.h>
#include <fb.h>
#include <errno.h>
#include <gui/graphic_utils.h>
#include <efi.h>
#include <efi/efi.h>
#include <efi/efi-device.h>

#define PIXEL_RGB_RESERVED_8BIT_PER_COLOR		0
#define PIXEL_BGR_RESERVED_8BIT_PER_COLOR		1
#define PIXEL_BIT_MASK					2
#define PIXEL_BLT_ONLY					3
#define PIXEL_FORMAT_MAX				4

struct efi_pixel_bitmask {
	u32 red_mask;
	u32 green_mask;
	u32 blue_mask;
	u32 reserved_mask;
};

struct efi_graphics_output_mode_info {
	u32 version;
	u32 horizontal_resolution;
	u32 vertical_resolution;
	int pixel_format;
	struct efi_pixel_bitmask pixel_information;
	u32 pixels_per_scan_line;
};

struct efi_graphics_output_protocol_mode {
	uint32_t max_mode;
	uint32_t mode;
	struct efi_graphics_output_mode_info *info;
	unsigned long size_of_info;
	void *frame_buffer_base;
	unsigned long frame_buffer_size;
};

struct efi_graphics_output_protocol {
	efi_status_t (EFIAPI *query_mode) (struct efi_graphics_output_protocol *This,
			uint32_t mode_number, unsigned long *size_of_info,
			struct efi_graphics_output_mode_info **info);
	efi_status_t (EFIAPI *set_mode) (struct efi_graphics_output_protocol *This,
			uint32_t mode_number);
	efi_status_t (EFIAPI *blt)(struct efi_graphics_output_protocol *This,
			void *buffer,
			unsigned long operation,
			unsigned long sourcex, unsigned long sourcey,
			unsigned long destinationx, unsigned long destinationy,
			unsigned long width, unsigned long height, unsigned
			long delta);
	struct efi_graphics_output_protocol_mode *mode;
};

struct efi_gop_priv {
	struct device_d *dev;
	struct fb_info fb;

	uint32_t mode;
	struct efi_graphics_output_protocol *gop;
};

static void find_bits(unsigned long mask, u32 *pos, u32 *size)
{
	u8 first, len;

	first = 0;
	len = 0;

	if (mask) {
		while (!(mask & 0x1)) {
			mask = mask >> 1;
			first++;
		}

		while (mask & 0x1) {
			mask = mask >> 1;
			len++;
		}
	}

	*pos = first;
	*size = len;
}

static void setup_pixel_info(struct fb_info *fb, u32 pixels_per_scan_line,
		 struct efi_pixel_bitmask pixel_info, int pixel_format)
{
	if (pixel_format == PIXEL_RGB_RESERVED_8BIT_PER_COLOR) {
		fb->bits_per_pixel = 32;
		fb->line_length = pixels_per_scan_line * 4;
		fb->red.length = 8;
		fb->red.offset = 0;
		fb->green.length = 8;
		fb->green.offset = 8;
		fb->blue.length = 8;
		fb->blue.offset = 16;
		fb->transp.length = 8;
		fb->transp.offset = 24;
	} else if (pixel_format == PIXEL_BGR_RESERVED_8BIT_PER_COLOR) {
		fb->bits_per_pixel = 32;
		fb->line_length = pixels_per_scan_line * 4;
		fb->red.length = 8;
		fb->red.offset = 16;
		fb->green.length = 8;
		fb->green.offset = 8;
		fb->blue.length = 8;
		fb->blue.offset = 0;
		fb->transp.length = 8;
		fb->transp.offset = 24;
	} else if (pixel_format == PIXEL_BIT_MASK) {
		find_bits(pixel_info.red_mask, &fb->red.offset, &fb->red.length);
		find_bits(pixel_info.green_mask, &fb->green.offset,
			  &fb->green.length);
		find_bits(pixel_info.blue_mask, &fb->blue.offset, &fb->blue.length);
		find_bits(pixel_info.reserved_mask, &fb->transp.offset,
			  &fb->transp.length);
		fb->bits_per_pixel = fb->red.length + fb->green.length +
			fb->blue.length + fb->transp.length;
		fb->line_length = (pixels_per_scan_line * fb->bits_per_pixel) / 8;
	} else {
		fb->bits_per_pixel = 4;
		fb->line_length = fb->xres / 2;
		fb->red.length = 0;
		fb->red.offset = 0;
		fb->green.length = 0;
		fb->green.offset = 0;
		fb->blue.length = 0;
		fb->blue.offset = 0;
		fb->transp.length = 0;
		fb->transp.offset = 0;
	}
}

static int efi_gop_query(struct efi_gop_priv *priv)
{
	struct efi_graphics_output_protocol_mode *mode;
	struct efi_graphics_output_mode_info *info;
	efi_status_t efiret;
	unsigned long size = 0;
	int i;
	struct fb_videomode *vmode;

	mode = priv->gop->mode;
	vmode = xzalloc(sizeof(*vmode) * mode->max_mode);

	priv->fb.modes.num_modes = mode->max_mode;
	priv->fb.modes.modes = vmode;

	for (i = 0; i < mode->max_mode; i++, vmode++) {
		efiret = priv->gop->query_mode(priv->gop, i, &size, &info);
		if (EFI_ERROR(efiret))
			continue;

		vmode->name = basprintf("%d", i);
		vmode->xres = info->horizontal_resolution;
		vmode->yres = info->vertical_resolution;
	}

	priv->fb.screen_base = mode->frame_buffer_base;
	priv->mode = mode->mode;
	priv->fb.xres = priv->fb.mode->xres;
	priv->fb.yres = priv->fb.mode->yres;

	return 0;
}

static int efi_gop_fb_activate_var(struct fb_info *fb_info)
{
	struct efi_gop_priv *priv = fb_info->priv;
	struct efi_graphics_output_mode_info *info;
	int num;
	unsigned long size = 0;
	efi_status_t efiret;

	num = simple_strtoul(fb_info->mode->name, NULL, 0);

	if (priv->mode != num) {
		efiret = priv->gop->set_mode(priv->gop, num);
		if (EFI_ERROR(efiret))
			return -efi_errno(efiret);
		priv->mode = num;
	}

	efiret = priv->gop->query_mode(priv->gop, num, &size, &info);
	if (EFI_ERROR(efiret))
		return -efi_errno(efiret);

	setup_pixel_info(&priv->fb, info->pixels_per_scan_line,
			 info->pixel_information, info->pixel_format);

	return 0;
}

static struct fb_ops efi_gop_ops = {
	.fb_activate_var = efi_gop_fb_activate_var,
};

static int efi_gop_probe(struct efi_device *efidev)
{
	struct efi_gop_priv *priv;
	int ret = 0;
	efi_status_t efiret;
	efi_guid_t got_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
	void *protocol;

	efiret = BS->handle_protocol(efidev->handle, &got_guid, &protocol);
	if (EFI_ERROR(efiret))
		return  -efi_errno(efiret);

	priv = xzalloc(sizeof(struct efi_gop_priv));
	priv->gop = protocol;
	priv->dev = &efidev->dev;

	if (!priv->gop) {
		ret = -EINVAL;
		goto err;
	}

	ret = efi_gop_query(priv);
	if (ret)
		goto err;

	priv->fb.priv = priv;
	priv->fb.dev.parent = priv->dev;
	priv->fb.fbops = &efi_gop_ops;
	priv->fb.p_enable = 1;
	priv->fb.current_mode = priv->mode;

	ret = register_framebuffer(&priv->fb);
	if (!ret) {
		priv->dev->priv = &priv->fb;
		return 0;
	}

	if (priv->fb.modes.modes) {
		int i;

		for (i = 0; i < priv->fb.modes.num_modes; i++)
			free((void*)priv->fb.modes.modes[i].name);

		free((void*)priv->fb.modes.modes);
	}
err:
	free(priv);
        return ret;
}

static struct efi_driver efi_gop_driver = {
        .driver = {
		.name  = "efi-gop",
	},
	.probe = efi_gop_probe,
	.guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID,
};
device_efi_driver(efi_gop_driver);
