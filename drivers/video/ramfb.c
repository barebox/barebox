// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: (C) 2022 Adrian Negreanu

#define pr_fmt(fmt) "ramfb: " fmt

#include <common.h>
#include <fb.h>
#include <fcntl.h>
#include <dma.h>
#include <init.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fs.h>
#include <linux/qemu_fw_cfg.h>
#include <video/fourcc.h>

struct ramfb {
	int fd;
	struct fb_info info;
	dma_addr_t screen_dma;
};

struct fw_cfg_etc_ramfb {
	u64 addr;
	u32 fourcc;
	u32 flags;
	u32 width;
	u32 height;
	u32 stride;
} __packed;

static int ramfb_activate_var(struct fb_info *fbi)
{
	struct ramfb *ramfb = fbi->priv;
	struct device *hwdev = fbi->dev.parent->parent;

	if (fbi->screen_base)
		dma_free_coherent(hwdev, fbi->screen_base, ramfb->screen_dma,
				  fbi->screen_size);

	fbi->screen_size = fbi->xres * fbi->yres * fbi->bits_per_pixel / BITS_PER_BYTE;
	fbi->screen_base = dma_alloc_coherent(hwdev, fbi->screen_size,
					      &ramfb->screen_dma);

	return 0;
}

static void ramfb_enable(struct fb_info *fbi)
{
	struct ramfb *ramfb = fbi->priv;
	struct fw_cfg_etc_ramfb *etc_ramfb;

	etc_ramfb = dma_alloc(sizeof(*etc_ramfb));

	etc_ramfb->addr = cpu_to_be64(ramfb->screen_dma);
	etc_ramfb->fourcc = cpu_to_be32(DRM_FORMAT_XRGB8888);
	etc_ramfb->flags  = cpu_to_be32(0);
	etc_ramfb->width  = cpu_to_be32(fbi->xres);
	etc_ramfb->height = cpu_to_be32(fbi->yres);
	etc_ramfb->stride = cpu_to_be32(fbi->line_length);

	pwrite(ramfb->fd, etc_ramfb, sizeof(*etc_ramfb), 0);

	dma_free(etc_ramfb);
}

static struct fb_videomode ramfb_modes[] = {
	{ .name = "640x480",   .xres =  640, .yres =  480 },
	{ .name = "800x600",   .xres =  800, .yres =  600 },
	{ .name = "1280x720",  .xres = 1280, .yres =  720 },
	{ .name = "1920x1080", .xres = 1920, .yres = 1080 },
};

static struct fb_ops ramfb_ops = {
	.fb_activate_var = ramfb_activate_var,
	.fb_enable = ramfb_enable,
};

static int ramfb_probe(struct device *dev)
{
	int ret;
	struct ramfb *ramfb;
	struct fb_info *fbi;

	ramfb = xzalloc(sizeof(*ramfb));

	ramfb->fd = (int)(uintptr_t)dev->platform_data;

	fbi = &ramfb->info;
	fbi->priv = ramfb;
	fbi->fbops = &ramfb_ops;
	fbi->dev.parent = dev;

	fbi->modes.modes = ramfb_modes;
	fbi->modes.num_modes = ARRAY_SIZE(ramfb_modes);
	/* current_mode = 0 (640x480) from xzalloc */

	fbi->bits_per_pixel = 32;
	fbi->red	= (struct fb_bitfield) {16, 8};
	fbi->green	= (struct fb_bitfield) {8, 8};
	fbi->blue	= (struct fb_bitfield) {0, 8};
	fbi->transp	= (struct fb_bitfield) {0, 0};

	ret = register_framebuffer(fbi);
	if (ret < 0) {
		dev_err(dev, "Unable to register ramfb: %d\n", ret);
		return ret;
	}

	dev_info(dev, "ramfb registered\n");

	return 0;
}

static struct driver ramfb_driver = {
	.probe = ramfb_probe,
	.name = "qemu-ramfb",
};
device_platform_driver(ramfb_driver);

MODULE_AUTHOR("Adrian Negreanu <adrian.negreanu@nxp.com>");
MODULE_DESCRIPTION("QEMU RamFB driver");
MODULE_LICENSE("GPL v2");
