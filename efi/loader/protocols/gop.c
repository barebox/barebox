// SPDX-License-Identifier: GPL-2.0+
// SPDX-Comment: Origin-URL: https://github.com/u-boot/u-boot/blob/459dbcb24837bb57750e854ad3ec6e49e11171ec/lib/efi_loader/efi_gop.c
/*
 *  EFI application disk support
 *
 *  Copyright (c) 2016 Alexander Graf
 */

#define pr_fmt(fmt) "efi-loader: gop: " fmt

#include <efi/guid.h>
#include <efi/loader.h>
#include <efi/error.h>
#include <efi/loader/object.h>
#include <efi/loader/trace.h>
#include <efi/protocol/gop.h>
#include <malloc.h>
#include <fb.h>
#include <gui/graphic_utils.h>

/**
 * struct efi_gop_obj - graphical output protocol object
 *
 * @header:	EFI object header
 * @ops:	graphical output protocol interface
 * @info:	graphical output mode information
 * @mode:	graphical output mode
 * @fbi:	backing video device's framebuffer info
 * @fb:		frame buffer
 */
struct efi_gop_obj {
	struct efi_object header;
	struct efi_graphics_output_protocol ops;
	struct efi_graphics_output_mode_info info;
	struct efi_graphics_output_protocol_mode mode;
	struct fb_info *fbi;
	/* Fields we only have access to during init */
	void *fb;
};

static efi_status_t EFIAPI gop_query_mode(struct efi_graphics_output_protocol *this,
					  u32 mode_number,
					  efi_uintn_t *size_of_info,
					  struct efi_graphics_output_mode_info **info)
{
	struct efi_gop_obj *gopobj;
	efi_status_t ret = EFI_SUCCESS;

	EFI_ENTRY("%p, %x, %p, %p", this, mode_number, size_of_info, info);

	if (!this || !size_of_info || !info || mode_number) {
		ret = EFI_INVALID_PARAMETER;
		goto out;
	}

	gopobj = container_of(this, struct efi_gop_obj, ops);
	ret = efi_allocate_pool(EFI_BOOT_SERVICES_DATA, sizeof(gopobj->info),
				(void **)info, "gop");
	if (ret != EFI_SUCCESS)
		goto out;
	*size_of_info = sizeof(gopobj->info);
	memcpy(*info, &gopobj->info, sizeof(gopobj->info));

out:
	return EFI_EXIT(ret);
}

static __always_inline struct efi_pixel_bitmask efi_vid30_to_blt_col(u32 vid)
{
	struct efi_pixel_bitmask blt = {
		.reserved_mask = 0,
	};

	blt.blue_mask  = (vid & 0x3ff) >> 2;
	vid >>= 10;
	blt.green_mask = (vid & 0x3ff) >> 2;
	vid >>= 10;
	blt.red_mask   = (vid & 0x3ff) >> 2;
	return blt;
}

static __always_inline u32 efi_blt_col_to_vid30(struct efi_pixel_bitmask *blt)
{
	return (u32)(blt->red_mask   << 2) << 20 |
	       (u32)(blt->green_mask << 2) << 10 |
	       (u32)(blt->blue_mask  << 2);
}

static __always_inline struct efi_pixel_bitmask efi_vid16_to_blt_col(u16 vid)
{
	struct efi_pixel_bitmask blt = {
		.reserved_mask = 0,
	};

	blt.blue_mask  = (vid & 0x1f) << 3;
	vid >>= 5;
	blt.green_mask = (vid & 0x3f) << 2;
	vid >>= 6;
	blt.red_mask   = (vid & 0x1f) << 3;
	return blt;
}

static __always_inline u16 efi_blt_col_to_vid16(struct efi_pixel_bitmask *blt)
{
	return (u16)(blt->red_mask   >> 3) << 11 |
	       (u16)(blt->green_mask >> 2) <<  5 |
	       (u16)(blt->blue_mask  >> 3);
}

static __always_inline efi_status_t gop_blt_int(struct efi_graphics_output_protocol *this,
						struct efi_pixel_bitmask *bufferp,
						u32 operation, efi_uintn_t sx,
						efi_uintn_t sy, efi_uintn_t dx,
						efi_uintn_t dy,
						efi_uintn_t width,
						efi_uintn_t height,
						efi_uintn_t delta,
						efi_uintn_t vid_bpp)
{
	struct efi_gop_obj *gopobj = container_of(this, struct efi_gop_obj, ops);
	efi_uintn_t i, j, linelen, slineoff = 0, dlineoff, swidth, dwidth;
	u32 *fb32 = gopobj->fb;
	u16 *fb16 = gopobj->fb;
	struct efi_pixel_bitmask *buffer = __builtin_assume_aligned(bufferp, 4);
	bool blt_to_video = (operation != EFI_BLT_VIDEO_TO_BLT_BUFFER);

	if (delta) {
		/* Check for 4 byte alignment */
		if (delta & 3)
			return EFI_INVALID_PARAMETER;
		linelen = delta >> 2;
	} else {
		linelen = width;
	}

	/* Check source rectangle */
	switch (operation) {
	case EFI_BLT_VIDEO_FILL:
		break;
	case EFI_BLT_BUFFER_TO_VIDEO:
		if (sx + width > linelen)
			return EFI_INVALID_PARAMETER;
		break;
	case EFI_BLT_VIDEO_TO_BLT_BUFFER:
	case EFI_BLT_VIDEO_TO_VIDEO:
		if (sx + width > gopobj->info.horizontal_resolution ||
		    sy + height > gopobj->info.vertical_resolution)
			return EFI_INVALID_PARAMETER;
		break;
	default:
		return EFI_INVALID_PARAMETER;
	}

	/* Check destination rectangle */
	switch (operation) {
	case EFI_BLT_VIDEO_FILL:
	case EFI_BLT_BUFFER_TO_VIDEO:
	case EFI_BLT_VIDEO_TO_VIDEO:
		if (dx + width > gopobj->info.horizontal_resolution ||
		    dy + height > gopobj->info.vertical_resolution)
			return EFI_INVALID_PARAMETER;
		break;
	case EFI_BLT_VIDEO_TO_BLT_BUFFER:
		if (dx + width > linelen)
			return EFI_INVALID_PARAMETER;
		break;
	}

	/* Calculate line width */
	switch (operation) {
	case EFI_BLT_BUFFER_TO_VIDEO:
		swidth = linelen;
		break;
	case EFI_BLT_VIDEO_TO_BLT_BUFFER:
	case EFI_BLT_VIDEO_TO_VIDEO:
		swidth = gopobj->info.horizontal_resolution;
		if (!vid_bpp)
			return EFI_UNSUPPORTED;
		break;
	case EFI_BLT_VIDEO_FILL:
		swidth = 0;
		break;
	}

	switch (operation) {
	case EFI_BLT_BUFFER_TO_VIDEO:
	case EFI_BLT_VIDEO_FILL:
	case EFI_BLT_VIDEO_TO_VIDEO:
		dwidth = gopobj->info.horizontal_resolution;
		if (!vid_bpp)
			return EFI_UNSUPPORTED;
		break;
	case EFI_BLT_VIDEO_TO_BLT_BUFFER:
		dwidth = linelen;
		break;
	}

	slineoff = swidth * sy;
	dlineoff = dwidth * dy;
	for (i = 0; i < height; i++) {
		for (j = 0; j < width; j++) {
			struct efi_pixel_bitmask pix;

			/* Read source pixel */
			switch (operation) {
			case EFI_BLT_VIDEO_FILL:
				pix = *buffer;
				break;
			case EFI_BLT_BUFFER_TO_VIDEO:
				pix = buffer[slineoff + j + sx];
				break;
			case EFI_BLT_VIDEO_TO_BLT_BUFFER:
			case EFI_BLT_VIDEO_TO_VIDEO:
				if (vid_bpp == 32)
					pix = *(struct efi_pixel_bitmask *)&fb32[
						slineoff + j + sx];
				else if (vid_bpp == 30)
					pix = efi_vid30_to_blt_col(fb32[
						slineoff + j + sx]);
				else
					pix = efi_vid16_to_blt_col(fb16[
						slineoff + j + sx]);
				break;
			}

			/* Write destination pixel */
			switch (operation) {
			case EFI_BLT_VIDEO_TO_BLT_BUFFER:
				buffer[dlineoff + j + dx] = pix;
				break;
			case EFI_BLT_BUFFER_TO_VIDEO:
			case EFI_BLT_VIDEO_FILL:
			case EFI_BLT_VIDEO_TO_VIDEO:
				if (vid_bpp == 32)
					fb32[dlineoff + j + dx] = *(u32 *)&pix;
				else if (vid_bpp == 30)
					fb32[dlineoff + j + dx] =
						efi_blt_col_to_vid30(&pix);
				else
					fb16[dlineoff + j + dx] =
						efi_blt_col_to_vid16(&pix);
				break;
			}
		}
		slineoff += swidth;
		dlineoff += dwidth;
	}

	if (blt_to_video) {
		struct fb_rect rect = { dx, dy, width, height };
		fb_damage(gopobj->fbi, &rect);
	}

	return EFI_SUCCESS;
}

static efi_uintn_t gop_get_bpp(struct efi_graphics_output_protocol *this)
{
	struct efi_gop_obj *gopobj = container_of(this, struct efi_gop_obj, ops);
	efi_uintn_t vid_bpp = 0;

	switch (gopobj->fbi->bits_per_pixel) {
	case 32:
		if (gopobj->info.pixel_format == EFI_GOT_BGRA8)
			vid_bpp = 32;
		else
			vid_bpp = 30;
		break;
	case 16:
		vid_bpp = 16;
		break;
	}

	return vid_bpp;
}

/*
 * GCC can't optimize our BLT function well, but we need to make sure that
 * our 2-dimensional loop gets executed very quickly, otherwise the system
 * will feel slow.
 *
 * By manually putting all obvious branch targets into functions which call
 * our generic BLT function with constants, the compiler can successfully
 * optimize for speed.
 */
static efi_status_t gop_blt_video_fill(struct efi_graphics_output_protocol *this,
				       struct efi_pixel_bitmask *buffer,
				       u32 foo, efi_uintn_t sx,
				       efi_uintn_t sy, efi_uintn_t dx,
				       efi_uintn_t dy, efi_uintn_t width,
				       efi_uintn_t height, efi_uintn_t delta,
				       efi_uintn_t vid_bpp)
{
	return gop_blt_int(this, buffer, EFI_BLT_VIDEO_FILL, sx, sy, dx,
			   dy, width, height, delta, vid_bpp);
}

static efi_status_t gop_blt_buf_to_vid16(struct efi_graphics_output_protocol *this,
					 struct efi_pixel_bitmask *buffer,
					 u32 foo, efi_uintn_t sx,
					 efi_uintn_t sy, efi_uintn_t dx,
					 efi_uintn_t dy, efi_uintn_t width,
					 efi_uintn_t height, efi_uintn_t delta)
{
	return gop_blt_int(this, buffer, EFI_BLT_BUFFER_TO_VIDEO, sx, sy, dx,
			   dy, width, height, delta, 16);
}

static efi_status_t gop_blt_buf_to_vid30(struct efi_graphics_output_protocol *this,
					 struct efi_pixel_bitmask *buffer,
					 u32 foo, efi_uintn_t sx,
					 efi_uintn_t sy, efi_uintn_t dx,
					 efi_uintn_t dy, efi_uintn_t width,
					 efi_uintn_t height, efi_uintn_t delta)
{
	return gop_blt_int(this, buffer, EFI_BLT_BUFFER_TO_VIDEO, sx, sy, dx,
			   dy, width, height, delta, 30);
}

static efi_status_t gop_blt_buf_to_vid32(struct efi_graphics_output_protocol *this,
					 struct efi_pixel_bitmask *buffer,
					 u32 foo, efi_uintn_t sx,
					 efi_uintn_t sy, efi_uintn_t dx,
					 efi_uintn_t dy, efi_uintn_t width,
					 efi_uintn_t height, efi_uintn_t delta)
{
	return gop_blt_int(this, buffer, EFI_BLT_BUFFER_TO_VIDEO, sx, sy, dx,
			   dy, width, height, delta, 32);
}

static efi_status_t gop_blt_vid_to_vid(struct efi_graphics_output_protocol *this,
				       struct efi_pixel_bitmask *buffer,
				       u32 foo, efi_uintn_t sx,
				       efi_uintn_t sy, efi_uintn_t dx,
				       efi_uintn_t dy, efi_uintn_t width,
				       efi_uintn_t height, efi_uintn_t delta,
				       efi_uintn_t vid_bpp)
{
	return gop_blt_int(this, buffer, EFI_BLT_VIDEO_TO_VIDEO, sx, sy, dx,
			   dy, width, height, delta, vid_bpp);
}

static efi_status_t gop_blt_vid_to_buf(struct efi_graphics_output_protocol *this,
				       struct efi_pixel_bitmask *buffer,
				       u32 foo, efi_uintn_t sx,
				       efi_uintn_t sy, efi_uintn_t dx,
				       efi_uintn_t dy, efi_uintn_t width,
				       efi_uintn_t height, efi_uintn_t delta,
				       efi_uintn_t vid_bpp)
{
	return gop_blt_int(this, buffer, EFI_BLT_VIDEO_TO_BLT_BUFFER, sx, sy,
			   dx, dy, width, height, delta, vid_bpp);
}

/**
 * gop_set_mode() - set graphical output mode
 *
 * This function implements the SetMode() service.
 *
 * See the Unified Extensible Firmware Interface (UEFI) specification for
 * details.
 *
 * @this:		the graphical output protocol
 * @mode_number:	the mode to be set
 * Return:		status code
 */
static efi_status_t EFIAPI gop_set_mode(struct efi_graphics_output_protocol *this,
					u32 mode_number)
{
	struct efi_gop_obj *gopobj;
	struct efi_pixel_bitmask buffer = {0, 0, 0, 0};
	efi_uintn_t vid_bpp;
	efi_status_t ret = EFI_SUCCESS;

	EFI_ENTRY("%p, %x", this, mode_number);

	if (!this) {
		ret = EFI_INVALID_PARAMETER;
		goto out;
	}
	if (mode_number) {
		ret = EFI_UNSUPPORTED;
		goto out;
	}
	gopobj = container_of(this, struct efi_gop_obj, ops);
	vid_bpp = gop_get_bpp(this);
	ret = gop_blt_video_fill(this, &buffer, EFI_BLT_VIDEO_FILL, 0, 0, 0, 0,
				 gopobj->info.horizontal_resolution,
				 gopobj->info.vertical_resolution, 0,
				 vid_bpp);
out:
	return EFI_EXIT(ret);
}

/*
 * Copy rectangle.
 *
 * This function implements the Blt service of the EFI_GRAPHICS_OUTPUT_PROTOCOL.
 * See the Unified Extensible Firmware Interface (UEFI) specification for
 * details.
 *
 * @this:	EFI_GRAPHICS_OUTPUT_PROTOCOL
 * @buffer:	pixel buffer
 * @sx:		source x-coordinate
 * @sy:		source y-coordinate
 * @dx:		destination x-coordinate
 * @dy:		destination y-coordinate
 * @width:	width of rectangle
 * @height:	height of rectangle
 * @delta:	length in bytes of a line in the pixel buffer (optional)
 * Return:	status code
 */
static efi_status_t EFIAPI gop_blt(struct efi_graphics_output_protocol *this,
				   struct efi_pixel_bitmask *buffer,
				   u32 operation, efi_uintn_t sx,
				   efi_uintn_t sy, efi_uintn_t dx,
				   efi_uintn_t dy, efi_uintn_t width,
				   efi_uintn_t height, efi_uintn_t delta)
{
	efi_status_t ret = EFI_INVALID_PARAMETER;
	struct efi_gop_obj *gopobj = container_of(this, struct efi_gop_obj, ops);
	efi_uintn_t vid_bpp;

	EFI_ENTRY("%p, %p, %u, %zu, %zu, %zu, %zu, %zu, %zu, %zu", this,
		  buffer, operation, sx, sy, dx, dy, width, height, delta);

	vid_bpp = gop_get_bpp(this);

	/* Allow for compiler optimization */
	switch (operation) {
	case EFI_BLT_VIDEO_FILL:
		ret = gop_blt_video_fill(this, buffer, operation, sx, sy, dx,
					 dy, width, height, delta, vid_bpp);
		break;
	case EFI_BLT_BUFFER_TO_VIDEO:
		/* This needs to be super-fast, so duplicate for 16/32bpp */
		if (vid_bpp == 32)
			ret = gop_blt_buf_to_vid32(this, buffer, operation, sx,
						   sy, dx, dy, width, height,
						   delta);
		else if (vid_bpp == 30)
			ret = gop_blt_buf_to_vid30(this, buffer, operation, sx,
						   sy, dx, dy, width, height,
						   delta);
		else
			ret = gop_blt_buf_to_vid16(this, buffer, operation, sx,
						   sy, dx, dy, width, height,
						   delta);
		break;
	case EFI_BLT_VIDEO_TO_VIDEO:
		ret = gop_blt_vid_to_vid(this, buffer, operation, sx, sy, dx,
					 dy, width, height, delta, vid_bpp);
		break;
	case EFI_BLT_VIDEO_TO_BLT_BUFFER:
		ret = gop_blt_vid_to_buf(this, buffer, operation, sx, sy, dx,
					 dy, width, height, delta, vid_bpp);
		break;
	default:
		ret = EFI_INVALID_PARAMETER;
	}

	if (ret != EFI_SUCCESS)
		return EFI_EXIT(ret);

	fb_flush(gopobj->fbi);

	return EFI_EXIT(EFI_SUCCESS);
}

static bool format_is_x2r10g10b10(struct fb_info *fbi)
{
	return fbi->red.length == 10 && fbi->green.length == 10 &&
		fbi->blue.length == 10;
}

static char *fbdev = "/dev/fb0";

/*
 * Install graphical output protocol if possible.
 */
static efi_status_t efi_gop_register(void *data)
{
	struct efi_gop_obj *gopobj;
	u32 col, row;
	efi_status_t ret;
	struct fb_info *fbi;
	struct screen *sc;
	struct efi_pixel_bitmask *pixel_information;

	sc = fb_open(fbdev);
	if (IS_ERR(sc)) {
		pr_debug("fb_open: %pe\n", sc);
		return EFI_UNSUPPORTED;
	}

	fbi = sc->info;
	col = fbi->xres;
	row = fbi->yres;

	switch (fbi->bits_per_pixel) {
	case 16:
	case 32:
		break;
	default:
		/* So far, we only work in 16 or 32 bit mode */
		pr_err("Unsupported video mode\n");
		return EFI_UNSUPPORTED;
	}

	gopobj = calloc(1, sizeof(*gopobj));
	if (!gopobj) {
		pr_err("Out of memory\n");
		return EFI_OUT_OF_RESOURCES;
	}

	/* Hook up to the device list */
	efi_add_handle(&gopobj->header);

	/* Fill in object data */
	ret = efi_add_protocol(&gopobj->header, &efi_gop_guid,
			       &gopobj->ops);
	if (ret != EFI_SUCCESS) {
		pr_err("Failure adding GOP protocol\n");
		return ret;
	}
	gopobj->ops.query_mode = gop_query_mode;
	gopobj->ops.set_mode = gop_set_mode;
	gopobj->ops.blt = gop_blt;
	gopobj->ops.mode = &gopobj->mode;

	gopobj->mode.max_mode = 1;
	gopobj->mode.info = &gopobj->info;
	gopobj->mode.size_of_info = sizeof(gopobj->info);

	gopobj->mode.frame_buffer_base = fbi->screen_base;
	gopobj->mode.frame_buffer_size = fbi->line_length * fbi->yres;

	gopobj->info.version = 0;
	gopobj->info.horizontal_resolution = col;
	gopobj->info.vertical_resolution = row;

	pixel_information = &gopobj->info.pixel_information;

	if (fbi->bits_per_pixel == 32) {
		if (format_is_x2r10g10b10(fbi)) {
			gopobj->info.pixel_format = EFI_GOT_BITMASK;
			pixel_information->red_mask = 0x3ff00000;
			pixel_information->green_mask = 0x000ffc00;
			pixel_information->blue_mask = 0x000003ff;
			pixel_information->reserved_mask = 0xc0000000;
		} else {
			gopobj->info.pixel_format = EFI_GOT_BGRA8;
		}
	} else {
		gopobj->info.pixel_format = EFI_GOT_BITMASK;
		pixel_information->red_mask = 0xf800;
		pixel_information->green_mask = 0x07e0;
		pixel_information->blue_mask = 0x001f;
	}
	gopobj->info.pixels_per_scan_line = col;
	gopobj->fb = fb_get_screen_base(fbi);
	gopobj->fbi = fbi;

	return EFI_SUCCESS;
}

static int efi_gop_init(void)
{
	efi_register_deferred_init(efi_gop_register, NULL);
	return 0;
}
device_initcall(efi_gop_init);
