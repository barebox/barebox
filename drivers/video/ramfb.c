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
	struct fb_videomode mode;
	u16 etcfb_select;
};

struct fw_cfg_etc_ramfb {
	u64 addr;
	u32 fourcc;
	u32 flags;
	u32 width;
	u32 height;
	u32 stride;
} __packed;

static int fw_cfg_find_file(struct device *dev, int fd, const char *filename)
{
	size_t filename_len = strlen(filename);
	ssize_t ret;
	__be32 count;
	int i;

	ioctl(fd, FW_CFG_SELECT, &(u16) { FW_CFG_FILE_DIR });

	lseek(fd, 0, SEEK_SET);

	ret = read(fd, &count, sizeof(count));
	if (ret < 0)
		return ret;

	for (i = 0; i < be32_to_cpu(count); i++) {
		struct fw_cfg_file qfile;

		read(fd, &qfile, sizeof(qfile));

		dev_dbg(dev, "enumerating file %s\n", qfile.name);

		if (memcmp(qfile.name, filename, filename_len))
			continue;

		return be16_to_cpu(qfile.select);
	}

	return -ENOENT;
}

static void ramfb_populate_modes(struct ramfb *ramfb)
{
	struct fb_info *info = &ramfb->info;

	ramfb->mode.name = "x8r8g8b8";
	info->xres = ramfb->mode.xres = 640;
	info->yres = ramfb->mode.yres = 480;

	info->mode = &ramfb->mode;
	info->bits_per_pixel = 32;
	info->red	= (struct fb_bitfield) {16, 8};
	info->green	= (struct fb_bitfield) {8, 8};
	info->blue	= (struct fb_bitfield) {0, 8};
	info->transp	= (struct fb_bitfield) {0, 0};
}

static int ramfb_activate_var(struct fb_info *fbi)
{
	struct ramfb *ramfb = fbi->priv;

	if (fbi->screen_base)
		dma_free_coherent(DMA_DEVICE_BROKEN,
				  fbi->screen_base, ramfb->screen_dma, fbi->screen_size);

	fbi->screen_size = fbi->xres * fbi->yres * fbi->bits_per_pixel;
	fbi->screen_base = dma_alloc_coherent(DMA_DEVICE_BROKEN,
					      fbi->screen_size, &ramfb->screen_dma);

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

	ioctl(ramfb->fd, FW_CFG_SELECT, &ramfb->etcfb_select);

	pwrite(ramfb->fd, etc_ramfb, sizeof(*etc_ramfb), 0);

	dma_free(etc_ramfb);
}

static struct fb_ops ramfb_ops = {
	.fb_activate_var = ramfb_activate_var,
	.fb_enable = ramfb_enable,
};

static int ramfb_probe(struct device *parent_dev, int fd)
{
	int ret;
	struct ramfb *ramfb;
	struct fb_info *fbi;

	ret = -ENODEV;

	ramfb = xzalloc(sizeof(*ramfb));

	ramfb->fd = fd;

	ret = fw_cfg_find_file(parent_dev, fd, "etc/ramfb");
	if (ret < 0) {
		dev_dbg(parent_dev, "ramfb: fw_cfg (etc/ramfb) file not found\n");
		return -ENODEV;
	}

	ramfb->etcfb_select = ret;
	dev_dbg(parent_dev, "etc/ramfb file at slot 0x%x\n", ramfb->etcfb_select);

	fbi = &ramfb->info;
	fbi->priv = ramfb;
	fbi->fbops = &ramfb_ops;
	fbi->dev.parent = parent_dev;

	ramfb_populate_modes(ramfb);

	ret = register_framebuffer(fbi);
	if (ret < 0) {
		dev_err(parent_dev, "Unable to register ramfb: %d\n", ret);
		return ret;
	}

	dev_info(parent_dev, "ramfb registered\n");

	return 0;
}

static int ramfb_driver_init(void)
{
	struct cdev *cdev;
	int err = 0;

	for_each_cdev(cdev) {
		int fd, ret;

		if (!strstarts(cdev->name, "fw_cfg"))
			continue;

		fd = cdev_fdopen(cdev, O_RDWR);
		if (fd < 0) {
			err = fd;
			continue;
		}

		ret = ramfb_probe(cdev->dev, fd);
		if (ret == 0)
			continue;
		if (ret != -ENODEV && ret != -ENXIO)
			err = ret;

		close(fd);
	}

	return err;
}
device_initcall(ramfb_driver_init);

MODULE_AUTHOR("Adrian Negreanu <adrian.negreanu@nxp.com>");
MODULE_DESCRIPTION("QEMU RamFB driver");
MODULE_LICENSE("GPL v2");
