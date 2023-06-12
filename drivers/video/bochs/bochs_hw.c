// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: Copyright (c) 2020 Ahmad Fatoum, Pengutronix
/*
 *  Driver for VGA with the Bochs VBE / QEMU stdvga extensions.
 *
 *  Based on the Linux v5.11-rc1 bochs-dispi DRM driver.
 */

#include <common.h>
#include <driver.h>
#include <fb.h>
#include "../edid.h"
#include "bochs_hw.h"

#define VBE_DISPI_INDEX_ID               0x0
#define VBE_DISPI_INDEX_XRES             0x1
#define VBE_DISPI_INDEX_YRES             0x2
#define VBE_DISPI_INDEX_BPP              0x3
#define VBE_DISPI_INDEX_ENABLE           0x4
#define VBE_DISPI_INDEX_BANK             0x5
#define VBE_DISPI_INDEX_VIRT_WIDTH       0x6
#define VBE_DISPI_INDEX_VIRT_HEIGHT      0x7
#define VBE_DISPI_INDEX_X_OFFSET         0x8
#define VBE_DISPI_INDEX_Y_OFFSET         0x9
#define VBE_DISPI_INDEX_VIDEO_MEMORY_64K 0xa

#define VBE_DISPI_ENABLED                0x01
#define VBE_DISPI_GETCAPS                0x02
#define VBE_DISPI_8BIT_DAC               0x20
#define VBE_DISPI_LFB_ENABLED            0x40
#define VBE_DISPI_NOCLEARMEM             0x80

/* Offsets for accessing ioports via PCI BAR1 (MMIO) */
#define VGA_MMIO_OFFSET (0x400 - 0x3c0)
#define VBE_MMIO_OFFSET 0x500

struct bochs {
	struct fb_info fb;
	void __iomem *fb_map;
	void __iomem *mmio;
};

static void bochs_vga_writeb(struct bochs *bochs, u16 ioport, u8 val)
{
	if (WARN_ON(ioport < 0x3c0 || ioport > 0x3df))
		return;

	if (bochs->mmio) {
		int offset = ioport + VGA_MMIO_OFFSET;
		writeb(val, bochs->mmio + offset);
	} else {
		outb(val, ioport);
	}
}

static u16 bochs_dispi_read(struct bochs *bochs, u16 reg)
{
	u16 ret = 0;

	if (bochs->mmio) {
		int offset = VBE_MMIO_OFFSET + (reg << 1);
		ret = readw(bochs->mmio + offset);
	} else {
		outw(reg, VBE_DISPI_IOPORT_INDEX);
		ret = inw(VBE_DISPI_IOPORT_DATA);
	}
	return ret;
}

static void bochs_dispi_write(struct bochs *bochs, u16 reg, u16 val)
{
	if (bochs->mmio) {
		int offset = VBE_MMIO_OFFSET + (reg << 1);
		writew(val, bochs->mmio + offset);
	} else {
		outw(reg, VBE_DISPI_IOPORT_INDEX);
		outw(val, VBE_DISPI_IOPORT_DATA);
	}
}

static void bochs_fb_enable(struct fb_info *fb)
{
	struct bochs *bochs = fb->priv;

	bochs_vga_writeb(bochs, 0x3c0, 0x20); /* unblank */

	bochs_dispi_write(bochs, VBE_DISPI_INDEX_ENABLE, 0);

	bochs_dispi_write(bochs, VBE_DISPI_INDEX_BPP,		fb->bits_per_pixel);
	bochs_dispi_write(bochs, VBE_DISPI_INDEX_XRES,		fb->xres);
	bochs_dispi_write(bochs, VBE_DISPI_INDEX_YRES,		fb->yres);
	bochs_dispi_write(bochs, VBE_DISPI_INDEX_BANK,		0);
	bochs_dispi_write(bochs, VBE_DISPI_INDEX_VIRT_WIDTH,	fb->xres);
	bochs_dispi_write(bochs, VBE_DISPI_INDEX_VIRT_HEIGHT,	fb->yres);
	bochs_dispi_write(bochs, VBE_DISPI_INDEX_X_OFFSET,	0);
	bochs_dispi_write(bochs, VBE_DISPI_INDEX_Y_OFFSET,	0);

	bochs_dispi_write(bochs, VBE_DISPI_INDEX_ENABLE,
			  VBE_DISPI_ENABLED | VBE_DISPI_LFB_ENABLED );
}

static void bochs_fb_disable(struct fb_info *fb)
{
	struct bochs *bochs = fb->priv;

	bochs_dispi_write(bochs, VBE_DISPI_INDEX_ENABLE,
			 bochs_dispi_read(bochs, VBE_DISPI_INDEX_ENABLE) &
			 ~VBE_DISPI_ENABLED);
}

static struct fb_ops bochs_fb_ops = {
	.fb_enable = bochs_fb_enable,
	.fb_disable = bochs_fb_disable,
};

static int bochs_hw_load_edid(struct bochs *bochs)
{
	u8 *edid;
	int i;

	edid = xzalloc(EDID_LENGTH);

	for (i = 0; i <= EDID_HEADER_END; i++)
		edid[i] = readb(bochs->mmio + i);

	/* check header to detect whenever edid support is enabled in qemu */
	if (!edid_check_header(edid)) {
		free(edid);
		return -EILSEQ;
	}

	for (i = EDID_HEADER_END + 1; i < EDID_LENGTH; i++)
		edid[i] = readb(bochs->mmio + i);

	bochs->fb.edid_data = edid;
	return 0;
}

static int bochs_hw_read_version(struct bochs *bochs)
{
	u16 ver;

	ver = bochs_dispi_read(bochs, VBE_DISPI_INDEX_ID);

	if ((ver & 0xB0C0) != 0xB0C0)
		return -ENODEV;

	return ver & 0xF;
}

int bochs_hw_probe(struct device *dev, void __iomem *fb_map,
		   void __iomem *mmio)
{
	struct bochs *bochs;
	struct fb_info *fb;
	int ret;

	bochs = xzalloc(sizeof(*bochs));

	bochs->fb_map	= fb_map;
	bochs->mmio	= mmio;

	ret = bochs_hw_read_version(bochs);
	if (ret < 0) {
		free(bochs);
		return ret;
	}

	dev_info(dev, "detected bochs dispi v%u\n", ret);

	fb = &bochs->fb;
	fb->screen_base = bochs->fb_map;

	fb->bits_per_pixel = 16;
	fb->red.length = 5;
	fb->green.length = 6;
	fb->blue.length = 5;
	fb->red.offset = 11;
	fb->green.offset = 5;
	fb->blue.offset = 0;

	/* EDID is only exposed over PCI */
	ret = -ENODEV;

	if (mmio) {
		ret = bochs_hw_load_edid(bochs);
		if (ret)
			dev_warn(dev, "couldn't read EDID information\n");
	}

	if (ret) {
		fb->mode = xzalloc(sizeof(*fb->mode));
		fb->modes.modes = fb->mode;
		fb->modes.num_modes = 1;

		fb->mode->name = "640x480";
		fb->mode->xres = 640;
		fb->mode->yres = 480;
	}

	fb->priv = bochs;
	fb->fbops = &bochs_fb_ops;

	fb->dev.parent = dev;
	return register_framebuffer(fb);
}
