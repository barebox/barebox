/*
 * SimpleFB driver
 *
 * Copyright (C) 2013 Andre Heider
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#define pr_fmt(fmt) "simplefb: " fmt

#include <common.h>
#include <errno.h>
#include <fb.h>
#include <fcntl.h>
#include <fs.h>
#include <init.h>
#include <xfuncs.h>

struct simplefb_mode {
	const char *format;
	u32 bpp;
	struct fb_bitfield red;
	struct fb_bitfield green;
	struct fb_bitfield blue;
	struct fb_bitfield transp;
};

/*
 * These values have to match the kernel's simplefb driver.
 * See Documentation/devicetree/bindings/video/simple-framebuffer.txt
 */
static const struct simplefb_mode simplefb_modes[] = {
	{
		.format	= "r5g6b5",
		.bpp	= 16,
		.red	= { .length = 5, .offset = 11 },
		.green	= { .length = 6, .offset = 5 },
		.blue	= { .length = 5, .offset = 0 },
		.transp	= { .length = 0, .offset = 0 },
	}, {
		.format	= "a8r8g8b8",
		.bpp	= 32,
		.red	= { .length = 8, .offset = 16 },
		.green	= { .length = 8, .offset = 8 },
		.blue	= { .length = 8, .offset = 0 },
		.transp	= { .length = 8, .offset = 24 },
	},

};

static bool simplefb_bitfield_cmp(const struct fb_bitfield *a,
					const struct fb_bitfield *b)
{
	if (a->offset != b->offset)
		return false;
	if (a->length != b->length)
		return false;
	if (a->msb_right != b->msb_right)
		return false;

	return true;
}

static const struct simplefb_mode *simplefb_find_mode(const struct fb_info *fbi)
{
	const struct simplefb_mode *mode;
	u32 i;

	for (i = 0; i < ARRAY_SIZE(simplefb_modes); ++i) {
		mode = &simplefb_modes[i];

		if (fbi->bits_per_pixel != mode->bpp)
			continue;
		if (!simplefb_bitfield_cmp(&fbi->red, &mode->red))
			continue;
		if (!simplefb_bitfield_cmp(&fbi->green, &mode->green))
			continue;
		if (!simplefb_bitfield_cmp(&fbi->blue, &mode->blue))
			continue;
		if (!simplefb_bitfield_cmp(&fbi->transp, &mode->transp))
			continue;

		return mode;
	}

	return NULL;
}

static int simplefb_create_node(struct device_node *root,
				const struct fb_info *fbi, const char *format)
{
	struct device_node *node;
	u32 cells[2];
	int ret;

	node = of_create_node(root, "/framebuffer");
	if (!node)
		return -ENOMEM;

	ret = of_property_write_string(node, "status", "disabled");
	if (ret < 0)
		return ret;

	ret = of_property_write_string(node, "compatible", "simple-framebuffer");
	if (ret)
		return ret;

	cells[0] = cpu_to_be32((u32)fbi->screen_base);
	cells[1] = cpu_to_be32(fbi->line_length * fbi->yres);
	ret = of_set_property(node, "reg", cells, sizeof(cells[0]) * 2, 1);
	if (ret < 0)
		return ret;

	cells[0] = cpu_to_be32(fbi->xres);
	ret = of_set_property(node, "width", cells, sizeof(cells[0]), 1);
	if (ret < 0)
		return ret;

	cells[0] = cpu_to_be32(fbi->yres);
	ret = of_set_property(node, "height", cells, sizeof(cells[0]), 1);
	if (ret < 0)
		return ret;

	cells[0] = cpu_to_be32(fbi->line_length);
	ret = of_set_property(node, "stride", cells, sizeof(cells[0]), 1);
	if (ret < 0)
		return ret;

	ret = of_property_write_string(node, "format", format);
	if (ret < 0)
		return ret;

	of_add_reserve_entry((u32)fbi->screen_base,
			(u32)fbi->screen_base + fbi->screen_size);

	return of_property_write_string(node, "status", "okay");
}

static int simplefb_of_fixup(struct device_node *root, void *ctx)
{
	struct fb_info *info = ctx;
	const struct simplefb_mode *mode;
	int ret;

	/* only create node if we are requested to */
	if (!info->register_simplefb)
		return 0;

	/* do not create node for disabled framebuffers */
	if (!info->enabled)
		return 0;

	mode = simplefb_find_mode(info);
	if (!mode) {
		dev_err(&info->dev, "fb format is incompatible with simplefb\n");
		return -EINVAL;
	}

	ret = simplefb_create_node(root, info, mode->format);
	if (ret)
		dev_err(&info->dev, "failed to create node: %d\n", ret);
	else
		dev_info(&info->dev, "created %s node\n", mode->format);

	return ret;
}

int fb_register_simplefb(struct fb_info *info)
{
	dev_add_param_bool(&info->dev, "register_simplefb",
			NULL, NULL, &info->register_simplefb, NULL);

	return of_register_fixup(simplefb_of_fixup, info);
}
