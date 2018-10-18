/*
 * Driver for the Solomon SSD1307 OLED controller family
 *
 * Supports:
 *      - SSD1305 (untested)
 *      - SSD1306
 *      - SSD1309 (untested)
 *
 * The SSD1307 controller is currently unsupported as the PWM parts were not
 * ported.
 *
 * Copyright 2012 Maxime Ripard <maxime.ripard@free-electrons.com>, Free Electrons
 *
 * Ported to barebox from linux v4.10
 * Copyright (C) 2017 Pengutronix, Bastian Stender <kernel@pengutronix.de>
 *
 * Licensed under the GPLv2 or later.
 */

#include <common.h>
#include <fb.h>
#include <i2c/i2c.h>
#include <of_device.h>
#include <gpio.h>
#include <of_gpio.h>
#include <regulator.h>

#define SSD1307FB_DATA                          0x40
#define SSD1307FB_COMMAND                       0x80

#define SSD1307FB_SET_ADDRESS_MODE              0x20
#define SSD1307FB_SET_ADDRESS_MODE_HORIZONTAL   (0x00)
#define SSD1307FB_SET_ADDRESS_MODE_VERTICAL     (0x01)
#define SSD1307FB_SET_ADDRESS_MODE_PAGE         (0x02)
#define SSD1307FB_SET_COL_RANGE                 0x21
#define SSD1307FB_SET_PAGE_RANGE                0x22
#define SSD1307FB_CONTRAST                      0x81
#define SSD1307FB_CHARGE_PUMP                   0x8d
#define SSD1307FB_SEG_REMAP_ON                  0xa1
#define SSD1307FB_DISPLAY_OFF                   0xae
#define SSD1307FB_SET_MULTIPLEX_RATIO           0xa8
#define SSD1307FB_DISPLAY_ON                    0xaf
#define SSD1307FB_START_PAGE_ADDRESS            0xb0
#define SSD1307FB_SET_DISPLAY_OFFSET            0xd3
#define SSD1307FB_SET_CLOCK_FREQ                0xd5
#define SSD1307FB_SET_PRECHARGE_PERIOD          0xd9
#define SSD1307FB_SET_COM_PINS_CONFIG           0xda
#define SSD1307FB_SET_VCOMH                     0xdb

struct ssd1307fb_deviceinfo {
	u32 default_vcomh;
	u32 default_dclk_div;
	u32 default_dclk_frq;
	int need_chargepump;
};

struct ssd1307fb_par {
	u32 com_invdir;
	u32 com_lrremap;
	u32 com_offset;
	u32 com_seq;
	u32 contrast;
	u32 dclk_div;
	u32 dclk_frq;
	const struct ssd1307fb_deviceinfo *device_info;
	struct i2c_client *client;
	u32 height;
	struct fb_info *info;
	u32 page_offset;
	u32 prechargep1;
	u32 prechargep2;
	int reset;
	struct regulator *vbat;
	u32 seg_remap;
	u32 vcomh;
	u32 width;
};

struct ssd1307fb_array {
	u8 type;
	u8 data[0];
};

static struct ssd1307fb_array *ssd1307fb_alloc_array(u32 len, u8 type)
{
	struct ssd1307fb_array *array;

	array = kzalloc(sizeof(struct ssd1307fb_array) + len, GFP_KERNEL);
	if (!array)
		return NULL;

	array->type = type;

	return array;
}

static int ssd1307fb_write_array(struct i2c_client *client,
				 struct ssd1307fb_array *array, u32 len)
{
	int ret;

	len += sizeof(struct ssd1307fb_array);

	ret = i2c_master_send(client, (u8 *)array, len);
	if (ret != len) {
		dev_err(&client->dev, "Couldn't send I2C command.\n");
		return ret;
	}

	return 0;
}

static inline int ssd1307fb_write_cmd(struct i2c_client *client, u8 cmd)
{
	struct ssd1307fb_array *array;
	int ret;

	array = ssd1307fb_alloc_array(1, SSD1307FB_COMMAND);
	if (!array)
		return -ENOMEM;

	array->data[0] = cmd;

	ret = ssd1307fb_write_array(client, array, 1);
	kfree(array);

	return ret;
}

static void ssd1307fb_update_display(struct ssd1307fb_par *par)
{
	struct ssd1307fb_array *array;
	u8 *vmem = par->info->screen_base;
	int i, j, k;

	array = ssd1307fb_alloc_array(par->width * par->height / 8,
				      SSD1307FB_DATA);
	if (!array)
		return;

	/*
	 * The screen is divided in pages, each having a height of 8
	 * pixels, and the width of the screen. When sending a byte of
	 * data to the controller, it gives the 8 bits for the current
	 * column. I.e, the first byte are the 8 bits of the first
	 * column, then the 8 bits for the second column, etc.
	 *
	 *
	 * Representation of the screen, assuming it is 5 bits
	 * wide. Each letter-number combination is a bit that controls
	 * one pixel.
	 *
	 * A0 A1 A2 A3 A4
	 * B0 B1 B2 B3 B4
	 * C0 C1 C2 C3 C4
	 * D0 D1 D2 D3 D4
	 * E0 E1 E2 E3 E4
	 * F0 F1 F2 F3 F4
	 * G0 G1 G2 G3 G4
	 * H0 H1 H2 H3 H4
	 *
	 * If you want to update this screen, you need to send 5 bytes:
	 *  (1) A0 B0 C0 D0 E0 F0 G0 H0
	 *  (2) A1 B1 C1 D1 E1 F1 G1 H1
	 *  (3) A2 B2 C2 D2 E2 F2 G2 H2
	 *  (4) A3 B3 C3 D3 E3 F3 G3 H3
	 *  (5) A4 B4 C4 D4 E4 F4 G4 H4
	 */

	for (i = 0; i < (par->height / 8); i++) {
		for (j = 0; j < par->width; j++) {
			u32 array_idx = i * par->width + j;
			array->data[array_idx] = 0;
			for (k = 0; k < 8; k++) {
				u32 page_length = par->width * i * 8;
				u32 index = page_length + (par->width * k + j);
				u8 byte = *(vmem + index);
				/* convert to 1 bit per pixel */
				u8 bit = byte > 0;
				array->data[array_idx] |= bit << k;
			}
		}
	}

	ssd1307fb_write_array(par->client, array, par->width * par->height / 8);
	kfree(array);
}

static void ssd1307fb_enable(struct fb_info *info)
{
	struct ssd1307fb_par *par = info->priv;
	ssd1307fb_write_cmd(par->client, SSD1307FB_DISPLAY_ON);
}

static void ssd1307fb_disable(struct fb_info *info)
{
	struct ssd1307fb_par *par = info->priv;
	ssd1307fb_write_cmd(par->client, SSD1307FB_DISPLAY_OFF);
}

static void ssd1307fb_flush(struct fb_info *info)
{
	struct ssd1307fb_par *par = info->priv;
	ssd1307fb_update_display(par);
}

static struct fb_ops ssd1307fb_ops = {
	.fb_enable	= ssd1307fb_enable,
	.fb_disable	= ssd1307fb_disable,
	.fb_flush	= ssd1307fb_flush,
};

static int ssd1307fb_init(struct ssd1307fb_par *par)
{
	int ret;
	u32 precharge, dclk, com_invdir, compins;

	/* Set initial contrast */
	ret = ssd1307fb_write_cmd(par->client, SSD1307FB_CONTRAST);
	if (ret < 0)
		return ret;

	ret = ssd1307fb_write_cmd(par->client, par->contrast);
	if (ret < 0)
		return ret;

	/* Set segment re-map */
	if (par->seg_remap) {
		ret = ssd1307fb_write_cmd(par->client, SSD1307FB_SEG_REMAP_ON);
		if (ret < 0)
			return ret;
	};

	/* Set COM direction */
	com_invdir = 0xc0 | (par->com_invdir & 0x1) << 3;
	ret = ssd1307fb_write_cmd(par->client,  com_invdir);
	if (ret < 0)
		return ret;

	/* Set multiplex ratio value */
	ret = ssd1307fb_write_cmd(par->client, SSD1307FB_SET_MULTIPLEX_RATIO);
	if (ret < 0)
		return ret;

	ret = ssd1307fb_write_cmd(par->client, par->height - 1);
	if (ret < 0)
		return ret;

	/* set display offset value */
	ret = ssd1307fb_write_cmd(par->client, SSD1307FB_SET_DISPLAY_OFFSET);
	if (ret < 0)
		return ret;

	ret = ssd1307fb_write_cmd(par->client, par->com_offset);
	if (ret < 0)
		return ret;

	/* Set clock frequency */
	ret = ssd1307fb_write_cmd(par->client, SSD1307FB_SET_CLOCK_FREQ);
	if (ret < 0)
		return ret;

	dclk = ((par->dclk_div - 1) & 0xf) | (par->dclk_frq & 0xf) << 4;
	ret = ssd1307fb_write_cmd(par->client, dclk);
	if (ret < 0)
		return ret;

	/* Set precharge period in number of ticks from the internal clock */
	ret = ssd1307fb_write_cmd(par->client, SSD1307FB_SET_PRECHARGE_PERIOD);
	if (ret < 0)
		return ret;

	precharge = (par->prechargep1 & 0xf) | (par->prechargep2 & 0xf) << 4;
	ret = ssd1307fb_write_cmd(par->client, precharge);
	if (ret < 0)
		return ret;

	/* Set COM pins configuration */
	ret = ssd1307fb_write_cmd(par->client, SSD1307FB_SET_COM_PINS_CONFIG);
	if (ret < 0)
		return ret;

	compins = 0x02 | !(par->com_seq & 0x1) << 4
				   | (par->com_lrremap & 0x1) << 5;
	ret = ssd1307fb_write_cmd(par->client, compins);
	if (ret < 0)
		return ret;

	/* Set VCOMH */
	ret = ssd1307fb_write_cmd(par->client, SSD1307FB_SET_VCOMH);
	if (ret < 0)
		return ret;

	ret = ssd1307fb_write_cmd(par->client, par->vcomh);
	if (ret < 0)
		return ret;

	/* Turn on the DC-DC Charge Pump */
	ret = ssd1307fb_write_cmd(par->client, SSD1307FB_CHARGE_PUMP);
	if (ret < 0)
		return ret;

	ret = ssd1307fb_write_cmd(par->client,
		BIT(4) | (par->device_info->need_chargepump ? BIT(2) : 0));
	if (ret < 0)
		return ret;

	/* Switch to horizontal addressing mode */
	ret = ssd1307fb_write_cmd(par->client, SSD1307FB_SET_ADDRESS_MODE);
	if (ret < 0)
		return ret;

	ret = ssd1307fb_write_cmd(par->client,
				  SSD1307FB_SET_ADDRESS_MODE_HORIZONTAL);
	if (ret < 0)
		return ret;

	/* Set column range */
	ret = ssd1307fb_write_cmd(par->client, SSD1307FB_SET_COL_RANGE);
	if (ret < 0)
		return ret;

	ret = ssd1307fb_write_cmd(par->client, 0x0);
	if (ret < 0)
		return ret;

	ret = ssd1307fb_write_cmd(par->client, par->width - 1);
	if (ret < 0)
		return ret;

	/* Set page range */
	ret = ssd1307fb_write_cmd(par->client, SSD1307FB_SET_PAGE_RANGE);
	if (ret < 0)
		return ret;

	ret = ssd1307fb_write_cmd(par->client, 0x0);
	if (ret < 0)
		return ret;

	ret = ssd1307fb_write_cmd(par->client,
				  par->page_offset + (par->height / 8) - 1);
	if (ret < 0)
		return ret;

	/* Turn on the display */
	ret = ssd1307fb_write_cmd(par->client, SSD1307FB_DISPLAY_ON);
	if (ret < 0)
		return ret;

	return 0;
}

static struct ssd1307fb_deviceinfo ssd1307fb_ssd1305_deviceinfo = {
	.default_vcomh = 0x34,
	.default_dclk_div = 1,
	.default_dclk_frq = 7,
};

static struct ssd1307fb_deviceinfo ssd1307fb_ssd1306_deviceinfo = {
	.default_vcomh = 0x20,
	.default_dclk_div = 1,
	.default_dclk_frq = 8,
	.need_chargepump = 1,
};

static struct ssd1307fb_deviceinfo ssd1307fb_ssd1309_deviceinfo = {
	.default_vcomh = 0x34,
	.default_dclk_div = 1,
	.default_dclk_frq = 10,
};

static const struct of_device_id ssd1307fb_of_match[] = {
	{
		.compatible = "solomon,ssd1305fb-i2c",
		.data = (void *)&ssd1307fb_ssd1305_deviceinfo,
	},
	{
		.compatible = "solomon,ssd1306fb-i2c",
		.data = (void *)&ssd1307fb_ssd1306_deviceinfo,
	},
	{
		.compatible = "solomon,ssd1309fb-i2c",
		.data = (void *)&ssd1307fb_ssd1309_deviceinfo,
	},
	{},
};

static int ssd1307fb_probe(struct device_d *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct fb_info *info;
	struct device_node *node = dev->device_node;
	const struct of_device_id *match =
		of_match_node(ssd1307fb_of_match, dev->device_node);
	u32 vmem_size;
	struct ssd1307fb_par *par;
	struct ssd1307fb_array *array;
	u8 *vmem;
	int ret;
	int i, j;

	if (!node) {
		dev_err(&client->dev, "No device tree data found!\n");
		return -EINVAL;
	}

	info = xzalloc(sizeof(struct fb_info));

	par = info->priv;
	par->info = info;
	par->client = client;

	par->device_info = (struct ssd1307fb_deviceinfo *)match->data;

	par->reset = of_get_named_gpio(node,
					 "reset-gpios", 0);
	if (!gpio_is_valid(par->reset)) {
		ret = par->reset;
		if (ret != -EPROBE_DEFER)
			dev_err(&client->dev,
				"Couldn't get named gpio 'reset-gpios': %s.\n",
				strerror(-ret));
		goto fb_alloc_error;
	}

	par->vbat = regulator_get(&client->dev, "vbat-supply");
	if (IS_ERR(par->vbat)) {
		dev_info(&client->dev, "Will not use VBAT");
		par->vbat = NULL;
	}

	ret = of_property_read_u32(node, "solomon,width", &par->width);
	if (ret) {
		dev_err(&client->dev,
			"Couldn't find 'solomon,width' in device tree.\n");
		goto panel_init_error;
	}

	ret = of_property_read_u32(node, "solomon,height", &par->height);
	if (ret) {
		dev_err(&client->dev,
			"Couldn't find 'solomon,height' in device tree.\n");
		goto panel_init_error;
	}

	ret = of_property_read_u32(node, "solomon,page-offset",
				   &par->page_offset);
	if (ret) {
		dev_err(&client->dev,
			"Couldn't find 'solomon,page_offset' in device tree.\n");
		goto panel_init_error;
	}

	if (of_property_read_u32(node, "solomon,com-offset", &par->com_offset))
		par->com_offset = 0;

	if (of_property_read_u32(node, "solomon,prechargep1",
				 &par->prechargep1))
		par->prechargep1 = 2;

	if (of_property_read_u32(node, "solomon,prechargep2",
				 &par->prechargep2))
		par->prechargep2 = 2;

	par->seg_remap = !of_property_read_bool(node,
						"solomon,segment-no-remap");
	par->com_seq = of_property_read_bool(node, "solomon,com-seq");
	par->com_lrremap = of_property_read_bool(node, "solomon,com-lrremap");
	par->com_invdir = of_property_read_bool(node, "solomon,com-invdir");

	par->contrast = 127;
	par->vcomh = par->device_info->default_vcomh;

	/* Setup display timing */
	par->dclk_div = par->device_info->default_dclk_div;
	par->dclk_frq = par->device_info->default_dclk_frq;

	vmem_size = par->width * par->height;

	vmem = malloc(vmem_size);
	if (!vmem) {
		dev_err(&client->dev, "Couldn't allocate graphical memory.\n");
		ret = -ENOMEM;
		goto fb_alloc_error;
	}

	info->fbops = &ssd1307fb_ops;
	info->line_length = par->width;

	info->xres = par->width;
	info->yres = par->height;

	/* emulate 8 bit per pixel */
	info->bits_per_pixel = 8;

	info->red.length = 3;
	info->red.offset = 0;
	info->green.length = 3;
	info->green.offset = 0;
	info->blue.length = 2;
	info->blue.offset = 0;

	info->screen_base = (u8 __force __iomem *)vmem;

	ret = gpio_request_one(par->reset,
			       GPIOF_OUT_INIT_HIGH,
			       "oled-reset");
	if (ret) {
		dev_err(&client->dev,
			"failed to request gpio %d: %d\n",
			par->reset, ret);
		goto reset_oled_error;
	}

	if (par->vbat) {
		ret = regulator_disable(par->vbat);
		if (ret < 0)
			goto reset_oled_error;
	}

	i2c_set_clientdata(client, info);

	/* Reset the screen */
	gpio_set_value(par->reset, 0);
	udelay(4);

	if (par->vbat) {
		ret = regulator_enable(par->vbat);
		if (ret < 0)
			goto reset_oled_error;
	}

	mdelay(100);

	gpio_set_value(par->reset, 1);
	udelay(4);

	ret = ssd1307fb_init(par);
	if (ret)
		goto reset_oled_error;

	ret = register_framebuffer(info);
	if (ret) {
		dev_err(&client->dev, "Couldn't register the framebuffer\n");
		goto panel_init_error;
	}

	/* clear display */
	array = ssd1307fb_alloc_array(par->width * par->height / 8,
				      SSD1307FB_DATA);
	if (!array)
		return -ENOMEM;

	for (i = 0; i < (par->height / 8); i++) {
		for (j = 0; j < par->width; j++) {
			u32 array_idx = i * par->width + j;
			array->data[array_idx] = 0;
		}
	}

	ssd1307fb_write_array(par->client, array, par->width * par->height / 8);
	kfree(array);

	dev_info(&client->dev,
		 "ssd1307 framebuffer device registered, using %d bytes of video memory\n",
		 vmem_size);

	return 0;

panel_init_error:
reset_oled_error:
fb_alloc_error:
	regulator_disable(par->vbat);
	free(info);
	return ret;
}

static struct driver_d ssd1307fb_driver = {
	.name = "ssd1307fb",
	.probe = ssd1307fb_probe,
	.of_compatible = DRV_OF_COMPAT(ssd1307fb_of_match),
};
device_i2c_driver(ssd1307fb_driver);
