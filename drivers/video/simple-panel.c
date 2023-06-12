// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * simple panel support for barebox
 *
 * (C) Copyright 2014 Sascha Hauer, Pengutronix
 */
#include <common.h>
#include <malloc.h>
#include <init.h>
#include <linux/err.h>
#include <of.h>
#include <fb.h>
#include <gpio.h>
#include <of_gpio.h>
#include <video/backlight.h>
#include <video/vpl.h>
#include <i2c/i2c.h>

struct simple_panel {
	struct device *dev;
	struct vpl vpl;
	int enable_gpio;
	int enable_active_high;
	int enabled;
	struct device_node *backlight_node;
	struct backlight_device *backlight;
	struct device_node *ddc_node;
	int enable_delay;
};

static int simple_panel_enable(struct simple_panel *panel)
{
	int ret;

	dev_dbg(panel->dev, "enabling\n");

	if (panel->backlight_node && !panel->backlight) {
		panel->backlight = of_backlight_find(panel->backlight_node);
		if (!panel->backlight) {
			dev_err(panel->dev, "Cannot find backlight\n");
			return -ENODEV;
		}
	}

	if (gpio_is_valid(panel->enable_gpio))
		gpio_direction_output(panel->enable_gpio,
			panel->enable_active_high);

	if (panel->enable_delay)
		mdelay(panel->enable_delay);

	if (panel->backlight) {
		ret = backlight_set_brightness_default(panel->backlight);
		if (ret)
			return ret;
	}

	return 0;
}

static int simple_panel_disable(struct simple_panel *panel)
{
	dev_dbg(panel->dev, "disabling\n");

	if (panel->backlight)
		backlight_set_brightness(panel->backlight, 0);

	if (gpio_is_valid(panel->enable_gpio))
		gpio_direction_output(panel->enable_gpio,
			!panel->enable_active_high);

	return 0;
}

static int simple_panel_get_modes(struct simple_panel *panel, struct display_timings *timings)
{
	struct display_timings *modes;
	int ret;

	if (panel->ddc_node && IS_ENABLED(CONFIG_DRIVER_VIDEO_EDID) &&
	    IS_ENABLED(CONFIG_I2C)) {
		struct i2c_adapter *i2c;

                i2c = of_find_i2c_adapter_by_node(panel->ddc_node);
		if (!i2c) {
			dev_err(panel->dev, "cannot find edid i2c node\n");
			return -ENODEV;
		}
		timings->edid = edid_read_i2c(i2c);
		if (!timings->edid) {
			dev_err(panel->dev, "cannot read edid data\n");
			return -EINVAL;
		}

		ret = edid_to_display_timings(timings, timings->edid);
		if (ret) {
			dev_err(panel->dev, "cannot convert edid data to timings\n");
			return ret;
		}
	}

	modes = of_get_display_timings(panel->dev->of_node);
	if (modes) {
		timings->modes = modes->modes;
		timings->num_modes = modes->num_modes;
		return 0;
	}

	dev_err(panel->dev, "No modes found\n");

	return -ENOENT;
}

static int simple_panel_ioctl(struct vpl *vpl, unsigned int port,
			unsigned int cmd, void *ptr)
{
	struct simple_panel *panel = container_of(vpl,
			struct simple_panel, vpl);

	switch (cmd) {
	case VPL_ENABLE:
		return simple_panel_enable(panel);
	case VPL_DISABLE:
		return simple_panel_disable(panel);
	case VPL_GET_VIDEOMODES:
		return simple_panel_get_modes(panel, ptr);
	default:
		return 0;
	}
}

static int simple_panel_probe(struct device *dev)
{
	struct simple_panel *panel;
	struct device_node *node = dev->of_node;
	enum of_gpio_flags flags;
	int ret;

	panel = xzalloc(sizeof(*panel));

	panel->enable_gpio = of_get_named_gpio_flags(node, "enable-gpios", 0, &flags);
	panel->vpl.node = node;
	panel->vpl.ioctl = simple_panel_ioctl;
	panel->dev = dev;

	if (gpio_is_valid(panel->enable_gpio)) {
		if (!(flags & OF_GPIO_ACTIVE_LOW))
			panel->enable_active_high = 1;
	}

	panel->ddc_node = of_parse_phandle(node, "ddc-i2c-bus", 0);

	of_property_read_u32(node, "enable-delay", &panel->enable_delay);

	panel->backlight_node = of_parse_phandle(node, "backlight", 0);

	ret = vpl_register(&panel->vpl);
	if (ret)
		return ret;

	return 0;
}

static struct of_device_id simple_panel_of_ids[] = {
	{ .compatible = "simple-panel", },
	{ }
};
MODULE_DEVICE_TABLE(of, simple_panel_of_ids);

static struct driver simple_panel_driver = {
	.name  = "simple-panel",
	.probe = simple_panel_probe,
	.of_compatible = DRV_OF_COMPAT(simple_panel_of_ids),
};
device_platform_driver(simple_panel_driver);
