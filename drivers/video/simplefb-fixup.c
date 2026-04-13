/*
 * SimpleFB driver
 *
 * Copyright (C) 2013 Andre Heider
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#define pr_fmt(fmt) "simplefb: " fmt

#include <errno.h>
#include <fb.h>
#include <fcntl.h>
#include <fs.h>
#include <of.h>
#include <init.h>
#include <xfuncs.h>
#include <linux/io.h>
#include <linux/array_size.h>
#include <linux/platform_data/simplefb.h>

/*
 * These values have to match the kernel's simplefb driver.
 * See dts/Bindings/display/simple-framebuffer.yaml
 */
static const struct simplefb_format simplefb_formats[] = SIMPLEFB_FORMATS;

enum simplefb_mode {
	SIMPLEFB_DISABLED,
	SIMPLEFB_ENABLED,
	SIMPLEFB_STDOUT_PATH,
};

static const char * const simplefb_mode_names[] = {
	"disabled", "enabled", "stdout-path",
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

static const struct simplefb_format *simplefb_find_mode(const struct fb_info *fbi)
{
	const struct simplefb_format *mode;
	u32 i;

	for (i = 0; i < ARRAY_SIZE(simplefb_formats); ++i) {
		mode = &simplefb_formats[i];

		if (fbi->bits_per_pixel != mode->bits_per_pixel)
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
				const struct fb_info *fbi, const char *format,
				bool set_stdout_path)
{
	struct device_node *node;
	phys_addr_t screen_base;
	__be32 cells[4];
	int addr_cells = 2, size_cells = 1, ret;
	struct resource res = { };

	of_property_read_u32(root, "#address-cells", &addr_cells);
	of_property_read_u32(root, "#size-cells", &size_cells);

	if ((addr_cells + size_cells) > 4)
		return -EINVAL;

	node = of_create_node(root, "/framebuffer");
	if (!node)
		return -ENOMEM;

	ret = of_property_write_string(node, "status", "disabled");
	if (ret < 0)
		return ret;

	ret = of_property_write_string(node, "compatible", "simple-framebuffer");
	if (ret)
		return ret;

	screen_base = virt_to_phys(fbi->screen_base);

	of_write_number(cells, screen_base, addr_cells);
	of_write_number(cells + addr_cells, fbi->screen_size, size_cells);

	ret = of_set_property(node, "reg", cells, sizeof(cells[0]) * (addr_cells + size_cells), 1);
	if (ret < 0)
		return ret;

	res.name = "simple-framebuffer";
	resource_set_range(&res, screen_base, fbi->screen_size);
	reserve_resource(&res);

	of_fixup_reserved_memory(root, &res);

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

	ret = of_property_write_string(node, "status", "okay");
	if (ret < 0)
		return ret;

	/*
	 * Set stdout-path in /chosen to point to this framebuffer if not
	 * already set.  This lets the OS (e.g. OpenBSD) know to use
	 * the framebuffer as its boot console rather than falling back to
	 * serial.
	 */
	if (set_stdout_path) {
		struct device_node *chosen;

		chosen = of_create_node(root, "/chosen");
		if (!chosen)
			return -ENOMEM;

		of_property_write_string(chosen, "stdout-path", "/framebuffer");
	}

	return 0;
}

static int simplefb_of_fixup(struct device_node *root, void *ctx)
{
	struct fb_info *info = ctx;
	const struct simplefb_format *mode;
	int ret;

	/* only create node if we are requested to */
	if (info->register_simplefb == SIMPLEFB_DISABLED)
		return 0;

	/* do not create node for disabled framebuffers */
	if (!info->enabled)
		return 0;

	mode = simplefb_find_mode(info);
	if (!mode) {
		dev_err(&info->dev, "fb format is incompatible with simplefb\n");
		return -EINVAL;
	}

	ret = simplefb_create_node(root, info, mode->name,
				   info->register_simplefb == SIMPLEFB_STDOUT_PATH);
	if (ret)
		dev_err(&info->dev, "failed to create node: %d\n", ret);
	else
		dev_info(&info->dev, "created %s node\n", mode->name);

	return ret;
}

int fb_register_simplefb(struct fb_info *info)
{
	dev_add_param_enum(&info->dev, "register_simplefb",
			   NULL, NULL, &info->register_simplefb,
			   simplefb_mode_names, ARRAY_SIZE(simplefb_mode_names),
			   NULL);

	return of_register_fixup(simplefb_of_fixup, info);
}
