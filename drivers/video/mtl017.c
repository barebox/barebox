/*
 * (C) Copyright 2014 Sascha Hauer, Pengutronix
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <common.h>
#include <driver.h>
#include <xfuncs.h>
#include <errno.h>
#include <regulator.h>
#include <of_gpio.h>
#include <gpio.h>
#include <fb.h>
#include <video/vpl.h>

#include <i2c/i2c.h>

struct mtl017 {
	struct vpl vpl;
	struct device_d *dev;
	struct i2c_client *client;
	u8 *regs;
	int enable_gpio;
	int enable_active_high;
	int reset_gpio;
	int reset_active_high;
	struct regulator *regulator;
};

/*
 * Unfortunately we know nothing about the mtl017 except the following
 * register tables which are derived from the Efika kernel. The displays
 * provide EDID data, but this does not work with the mtl017 or at least
 * not with the below register settings, so we have to provide hardcoded
 * modelines and use the EDID data only to match against the vendor/display.
 */
static u8 mtl017_44_54_tbl[] = {
	/* 44M to 53.9M */
	0x00, 0x20, 0xAF, 0x59, 0x2B, 0xDE, 0x51, 0x00,
	0x00, 0x04, 0x17, 0x00, 0x58, 0x02, 0x00, 0x00,
	0x00, 0x3B, 0x01, 0x08, 0x00, 0x1E, 0x01, 0x05,
	0x00, 0x01, 0x72, 0x05, 0x32, 0x00, 0x00, 0x04,
	0x00, 0x00, 0x20, 0xA8, 0x02, 0x12, 0x00, 0x58,
	0x02, 0x00, 0x00, 0x02, 0x00, 0x00, 0x02, 0x00,
	0x00, 0x02, 0x10, 0x01, 0x68, 0x03, 0xC2, 0x01,
	0x4A, 0x03, 0x46, 0x00, 0xF1, 0x01, 0x5C, 0x04,
	0x08, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07, 0x3A,
	0x18, 0x4B, 0x29, 0x5C, 0xDE, 0xF6, 0xE0, 0x1C,
	0x03, 0xFC, 0xE3, 0x1F, 0xF3, 0x75, 0x26, 0x45,
	0x4A, 0x91, 0x8A, 0xFF, 0x3F, 0x83, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x4E, 0x48,
	0x00, 0x01, 0x10, 0x01, 0x00, 0x00, 0x10, 0x04,
	0x02, 0x1F, 0x00, 0x00, 0x00, 0x0A, 0x00, 0x00,
	0x32, 0x00, 0x00, 0x04, 0x12, 0x00, 0x58, 0x02,
	0x02, 0x7C, 0x04, 0x98, 0x02, 0x11, 0x78, 0x18,
	0x30, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
};

#define REGMAP_LENGTH (sizeof(mtl017_44_54_tbl) / sizeof(u8))
#define BLOCK_TX_SIZE 32

struct mtl017_panel_info {
	char *manufacturer;
	char *product_name;
	struct fb_videomode *mode;
	u8  *regs;
};


static struct fb_videomode auo_bw101aw02_mode = {
	.name = "AUO B101AW02 1024x600",
	.refresh = 60,
	.xres = 1024,
	.yres = 600,
	.pixclock = 22800,
	.left_margin = 80,
	.right_margin = 40,
	.upper_margin = 21,
	.lower_margin = 21,
	.hsync_len = 4,
	.vsync_len = 4,
	.vmode = FB_VMODE_NONINTERLACED,
};

static struct mtl017_panel_info panels[] = {
	{
		.manufacturer = "AUO",
		.product_name = "B101AW02 V0",
		.mode = &auo_bw101aw02_mode,
		.regs = mtl017_44_54_tbl,
	},
	{
		.manufacturer = "CMO",
		.product_name = "N101L6-L0D",
		.mode = &auo_bw101aw02_mode,
		.regs = mtl017_44_54_tbl,
	},
};

static int mtl017_get_videomodes(struct mtl017 *mtl017, struct display_timings *timings)
{
	int ret, i;

	ret = vpl_ioctl(&mtl017->vpl, 1, VPL_GET_VIDEOMODES, timings);
	if (ret)
		return ret;

	for (i = 0; i < ARRAY_SIZE(panels); i ++) {
		if (memcmp(timings->edid + 0x71, panels[i].product_name,
					strlen(panels[i].product_name)))
			continue;

		dev_dbg(mtl017->dev, "found LCD Panel: %s %s\n",
				panels[i].manufacturer,
				panels[i].product_name);
		mtl017->regs = panels[i].regs;

		timings->modes[0].name =  panels[i].mode->name;
		timings->modes[0].refresh =  panels[i].mode->refresh;
		timings->modes[0].xres =  panels[i].mode->xres;
		timings->modes[0].yres =  panels[i].mode->yres;
		timings->modes[0].pixclock =  panels[i].mode->pixclock;
		timings->modes[0].left_margin =  panels[i].mode->left_margin;
		timings->modes[0].right_margin =  panels[i].mode->right_margin;
		timings->modes[0].upper_margin =  panels[i].mode->upper_margin;
		timings->modes[0].lower_margin =  panels[i].mode->lower_margin;
		timings->modes[0].hsync_len =  panels[i].mode->hsync_len;
		timings->modes[0].vsync_len =  panels[i].mode->vsync_len;
		timings->num_modes = 1;
	}

	return 0;
}

static int mtl017_enable(struct mtl017 *mtl017)
{
	int i;
	int ret;
	int retry = 5;
	u8 *reg_tbl = mtl017->regs;

	dev_dbg(mtl017->dev, "Initializing MTL017 LVDS Controller\n");

	ret = regulator_enable(mtl017->regulator);
	if (ret)
		return ret;

	gpio_direction_output(mtl017->reset_gpio, mtl017->reset_active_high);
	mdelay(5);
	gpio_direction_output(mtl017->reset_gpio, !mtl017->reset_active_high);
	mdelay(5);
	gpio_direction_output(mtl017->enable_gpio, mtl017->enable_active_high);

	/* Write configuration table */
	for (i = 0; i < REGMAP_LENGTH; i+=BLOCK_TX_SIZE) {
retry:
		mdelay(1);
		ret = i2c_smbus_write_i2c_block_data(mtl017->client, i, BLOCK_TX_SIZE, &(reg_tbl[i]));
		if (ret < 0) {
			dev_warn(mtl017->dev, "failed to initialize: %d\n", ret);
			if (retry-- > 0)
				goto retry;
			return -EIO;
		}
	}

	return 0;
}

static int mtl017_ioctl(struct vpl *vpl, unsigned int port,
			unsigned int cmd, void *ptr)
{
	struct mtl017 *mtl017 = container_of(vpl,
			struct mtl017, vpl);
	int ret = 0;

	switch (cmd) {
	case VPL_PREPARE:
		dev_dbg(mtl017->dev, "VPL_PREPARE\n");
		goto forward;
	case VPL_ENABLE:
		dev_dbg(mtl017->dev, "VPL_ENABLE\n");

		ret = mtl017_enable(mtl017);
		if (ret < 0)
			break;

		goto forward;
	case VPL_DISABLE:
		dev_dbg(mtl017->dev, "VPL_DISABLE\n");

		goto forward;
	case VPL_GET_VIDEOMODES:
		dev_dbg(mtl017->dev, "VPL_GET_VIDEOMODES\n");

		ret = mtl017_get_videomodes(mtl017, ptr);
		break;
	default:
		break;
	}

	return ret;

forward:
	return vpl_ioctl(&mtl017->vpl, 1, cmd, ptr);
}

static int mtl017_probe(struct device_d *dev)
{
	struct mtl017 *mtl017;
	int ret;
	enum of_gpio_flags flags;

	mtl017 = xzalloc(sizeof(struct mtl017));
	mtl017->vpl.node = dev->device_node;
	mtl017->vpl.ioctl = mtl017_ioctl;
	mtl017->dev = dev;
	mtl017->client = to_i2c_client(dev);
	mtl017->regulator = regulator_get(dev, "vdd");

	if (IS_ERR(mtl017->regulator))
		mtl017->regulator = NULL;

	mtl017->enable_gpio = of_get_named_gpio_flags(dev->device_node,
			"enable-gpios", 0, &flags);
	if (gpio_is_valid(mtl017->enable_gpio)) {
		if (!(flags & OF_GPIO_ACTIVE_LOW))
			mtl017->enable_active_high = 1;
	}

	mtl017->reset_gpio = of_get_named_gpio_flags(dev->device_node,
			"reset-gpios", 0, &flags);
	if (gpio_is_valid(mtl017->reset_gpio)) {
		if (!(flags & OF_GPIO_ACTIVE_LOW))
			mtl017->reset_active_high = 1;
	}

	ret = vpl_register(&mtl017->vpl);
	if (ret)
		return ret;

	return 0;
}

static struct driver_d mtl_driver = {
	.name  = "mtl017",
	.probe = mtl017_probe,
};
device_i2c_driver(mtl_driver);
