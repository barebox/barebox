/*
 * BCM2835 framebuffer driver
 *
 * Copyright (C) 2013 Andre Heider
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <driver.h>
#include <errno.h>
#include <fb.h>
#include <init.h>
#include <malloc.h>
#include <xfuncs.h>

#include <mach/mbox.h>

struct bcm2835fb_info {
	struct fb_info fbi;
	struct fb_videomode mode;
};

struct msg_fb_query {
	struct bcm2835_mbox_hdr hdr;
	struct bcm2835_mbox_tag_physical_w_h physical_w_h;
	u32 end_tag;
};

struct msg_fb_setup {
	struct bcm2835_mbox_hdr hdr;
	struct bcm2835_mbox_tag_physical_w_h physical_w_h;
	struct bcm2835_mbox_tag_virtual_w_h virtual_w_h;
	struct bcm2835_mbox_tag_depth depth;
	struct bcm2835_mbox_tag_pixel_order pixel_order;
	struct bcm2835_mbox_tag_alpha_mode alpha_mode;
	struct bcm2835_mbox_tag_virtual_offset virtual_offset;
	struct bcm2835_mbox_tag_allocate_buffer allocate_buffer;
	struct bcm2835_mbox_tag_pitch pitch;
	u32 end_tag;
};

static void bcm2835fb_enable(struct fb_info *info)
{
}

static void bcm2835fb_disable(struct fb_info *info)
{
}

static struct fb_ops bcm2835fb_ops = {
	.fb_enable		= bcm2835fb_enable,
	.fb_disable		= bcm2835fb_disable,
};

static int bcm2835fb_probe(struct device_d *dev)
{
	BCM2835_MBOX_STACK_ALIGN(struct msg_fb_query, msg_query);
	BCM2835_MBOX_STACK_ALIGN(struct msg_fb_setup, msg_setup);
	struct bcm2835fb_info *info;
	u32 w, h;
	int ret;

	BCM2835_MBOX_INIT_HDR(msg_query);
	BCM2835_MBOX_INIT_TAG_NO_REQ(&msg_query->physical_w_h,
					GET_PHYSICAL_W_H);
	ret = bcm2835_mbox_call_prop(BCM2835_MBOX_PROP_CHAN, &msg_query->hdr);
	if (ret) {
		dev_err(dev, "could not query display resolution\n");
		return ret;
	}

	w = msg_query->physical_w_h.body.resp.width;
	h = msg_query->physical_w_h.body.resp.height;

	BCM2835_MBOX_INIT_HDR(msg_setup);
	BCM2835_MBOX_INIT_TAG(&msg_setup->physical_w_h, SET_PHYSICAL_W_H);
	msg_setup->physical_w_h.body.req.width = w;
	msg_setup->physical_w_h.body.req.height = h;
	BCM2835_MBOX_INIT_TAG(&msg_setup->virtual_w_h, SET_VIRTUAL_W_H);
	msg_setup->virtual_w_h.body.req.width = w;
	msg_setup->virtual_w_h.body.req.height = h;
	BCM2835_MBOX_INIT_TAG(&msg_setup->depth, SET_DEPTH);
	msg_setup->depth.body.req.bpp = 16;
	BCM2835_MBOX_INIT_TAG(&msg_setup->pixel_order, SET_PIXEL_ORDER);
	msg_setup->pixel_order.body.req.order = BCM2835_MBOX_PIXEL_ORDER_BGR;
	BCM2835_MBOX_INIT_TAG(&msg_setup->alpha_mode, SET_ALPHA_MODE);
	msg_setup->alpha_mode.body.req.alpha = BCM2835_MBOX_ALPHA_MODE_IGNORED;
	BCM2835_MBOX_INIT_TAG(&msg_setup->virtual_offset, SET_VIRTUAL_OFFSET);
	msg_setup->virtual_offset.body.req.x = 0;
	msg_setup->virtual_offset.body.req.y = 0;
	BCM2835_MBOX_INIT_TAG(&msg_setup->allocate_buffer, ALLOCATE_BUFFER);
	msg_setup->allocate_buffer.body.req.alignment = 0x100;
	BCM2835_MBOX_INIT_TAG_NO_REQ(&msg_setup->pitch, GET_PITCH);

	ret = bcm2835_mbox_call_prop(BCM2835_MBOX_PROP_CHAN, &msg_setup->hdr);
	if (ret) {
		dev_err(dev, "could not configure display\n");
		return ret;
	}

	info = xzalloc(sizeof *info);
	info->fbi.fbops = &bcm2835fb_ops;
	info->fbi.screen_base =
	   (void *)msg_setup->allocate_buffer.body.resp.fb_address;
	info->fbi.xres = msg_setup->physical_w_h.body.resp.width;
	info->fbi.yres = msg_setup->physical_w_h.body.resp.height;
	info->fbi.bits_per_pixel = 16;
	info->fbi.line_length = msg_setup->pitch.body.resp.pitch;
	info->fbi.red.length = 5;
	info->fbi.red.offset = 11;
	info->fbi.green.length = 6;
	info->fbi.green.offset = 5;
	info->fbi.blue.length = 5;
	info->fbi.blue.offset = 0;

	info->fbi.mode = &info->mode;
	info->fbi.mode->xres = info->fbi.xres;
	info->fbi.mode->yres = info->fbi.yres;

	ret = register_framebuffer(&info->fbi);
	if (ret) {
		free(info);
		dev_err(dev, "failed to register framebuffer: %d\n", ret);
		return ret;
	}

	dev_info(dev, "registered\n");
	return 0;
}

static struct driver_d bcm2835fb_driver = {
	.name	= "bcm2835_fb",
	.probe	= bcm2835fb_probe,
};
device_platform_driver(bcm2835fb_driver);
