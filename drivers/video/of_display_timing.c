/*
 * OF helpers for parsing display timings
 *
 * Copyright (c) 2012 Steffen Trumtrar <s.trumtrar@pengutronix.de>, Pengutronix
 *
 * based on of_videomode.c by Sascha Hauer <s.hauer@pengutronix.de>
 *
 * This file is released under the GPLv2
 */
#include <common.h>
#include <of.h>
#include <fb.h>
#include <malloc.h>

void display_timings_release(struct display_timings *disp)
{
	free(disp->modes);
	free(disp);
}
EXPORT_SYMBOL_GPL(display_timings_release);

/**
 * parse_timing_property - parse timing_entry from device_node
 * @np: device_node with the property
 * @name: name of the property
 * @result: will be set to the return value
 *
 * DESCRIPTION:
 * Every display_timing can be specified with either just the typical value or
 * a range consisting of min/typ/max. This function helps handling this
 **/
static int parse_timing_property(const struct device_node *np, const char *name,
			  u32 *res)
{
	struct property *prop;
	int length, cells, ret;

	prop = of_find_property(np, name, &length);
	if (!prop) {
		pr_err("%s: could not find property %s\n",
			np->full_name, name);
		return -EINVAL;
	}

	cells = length / sizeof(u32);
	if (cells == 1) {
		ret = of_property_read_u32(np, name, res);
	} else {
		pr_err("%s: illegal timing specification in %s\n",
			np->full_name, name);
		return -EINVAL;
	}

	return ret;
}

/**
 * of_parse_display_timing - parse display_timing entry from device_node
 * @np: device_node with the properties
 **/
static int of_parse_display_timing(const struct device_node *np,
		struct fb_videomode *mode)
{
	u32 val = 0, pixelclock = 0;
	int ret = 0;

	memset(mode, 0, sizeof(*mode));

	ret |= parse_timing_property(np, "hback-porch", &mode->left_margin);
	ret |= parse_timing_property(np, "hfront-porch", &mode->right_margin);
	ret |= parse_timing_property(np, "hactive", &mode->xres);
	ret |= parse_timing_property(np, "hsync-len", &mode->hsync_len);
	ret |= parse_timing_property(np, "vback-porch", &mode->upper_margin);
	ret |= parse_timing_property(np, "vfront-porch", &mode->lower_margin);
	ret |= parse_timing_property(np, "vactive", &mode->yres);
	ret |= parse_timing_property(np, "vsync-len", &mode->vsync_len);
	ret |= parse_timing_property(np, "clock-frequency", &pixelclock);

	mode->pixclock = pixelclock ? KHZ2PICOS(pixelclock / 1000) : 0;

	if (!of_property_read_u32(np, "vsync-active", &val))
		mode->sync |= val ? FB_SYNC_VERT_HIGH_ACT : 0;
	if (!of_property_read_u32(np, "hsync-active", &val))
		mode->sync |= val ? FB_SYNC_HOR_HIGH_ACT : 0;
	if (!of_property_read_u32(np, "de-active", &val))
		mode->display_flags |= val ? DISPLAY_FLAGS_DE_HIGH :
				DISPLAY_FLAGS_DE_LOW;
	if (!of_property_read_u32(np, "pixelclk-active", &val))
		mode->display_flags |= val ? DISPLAY_FLAGS_PIXDATA_POSEDGE :
				DISPLAY_FLAGS_PIXDATA_NEGEDGE;

	if (ret) {
		pr_err("%s: error reading timing properties\n",
			np->full_name);
		return -EINVAL;
	}

	return 0;
}

/**
 * of_get_display_timings - parse all display_timing entries from a device_node
 * @np: device_node with the subnodes
 **/
struct display_timings *of_get_display_timings(struct device_node *np)
{
	struct device_node *timings_np;
	struct device_node *entry;
	struct device_node *native_mode;
	struct display_timings *disp;

	if (!np)
		return NULL;

	timings_np = of_get_child_by_name(np, "display-timings");
	if (!timings_np) {
		pr_debug("%s: could not find display-timings node\n",
			np->full_name);
		return NULL;
	}

	disp = xzalloc(sizeof(*disp));

	entry = of_parse_phandle(timings_np, "native-mode", 0);
	/* assume first child as native mode if none provided */
	if (!entry)
		entry = of_get_next_available_child(np, NULL);
	/* if there is no child, it is useless to go on */
	if (!entry) {
		pr_err("%s: no timing specifications given\n",
			np->full_name);
		goto entryfail;
	}

	pr_debug("%s: using %s as default timing\n",
		np->full_name, entry->name);

	native_mode = entry;

	disp->num_modes = of_get_child_count(timings_np);
	if (disp->num_modes == 0) {
		/* should never happen, as entry was already found above */
		pr_err("%s: no timings specified\n", np->full_name);
		goto entryfail;
	}

	disp->modes = xzalloc(sizeof(struct fb_videomode) * disp->num_modes);

	disp->num_modes = 0;
	disp->native_mode = 0;

	for_each_child_of_node(timings_np, entry) {
		struct fb_videomode *mode;
		int r;

		mode = &disp->modes[disp->num_modes];

		r = of_parse_display_timing(entry, mode);
		if (r) {
			/*
			 * to not encourage wrong devicetrees, fail in case of
			 * an error
			 */
			pr_err("%s: error in timing %d\n",
				np->full_name, disp->num_modes + 1);
			goto timingfail;
		}

		mode->name = xstrdup(entry->name);

		if (native_mode == entry)
			disp->native_mode = disp->num_modes;

		disp->num_modes++;
	}

	pr_debug("%s: got %d timings. Using timing #%d as default\n",
		np->full_name, disp->num_modes,
		disp->native_mode + 1);

	return disp;

timingfail:
	display_timings_release(disp);
entryfail:
	free(disp);

	return NULL;
}
EXPORT_SYMBOL_GPL(of_get_display_timings);
