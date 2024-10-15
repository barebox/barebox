// SPDX-License-Identifier: GPL-2.0-or-later
#include <fb.h>
#include <video/drm/drm_modes.h>
#include <video/videomode.h>

/**
 * drm_display_mode_from_videomode - fill in @dmode using @vm,
 * @vm: videomode structure to use as source
 * @dmode: drm_display_mode structure to use as destination
 *
 * Fills out @dmode using the display mode specified in @vm.
 */
void videomode_to_drm_display_mode(const struct videomode *vm,
				   struct drm_display_mode *dmode)
{
	dmode->hdisplay = vm->hactive;
	dmode->hsync_start = dmode->hdisplay + vm->hfront_porch;
	dmode->hsync_end = dmode->hsync_start + vm->hsync_len;
	dmode->htotal = dmode->hsync_end + vm->hback_porch;

	dmode->vdisplay = vm->vactive;
	dmode->vsync_start = dmode->vdisplay + vm->vfront_porch;
	dmode->vsync_end = dmode->vsync_start + vm->vsync_len;
	dmode->vtotal = dmode->vsync_end + vm->vback_porch;

	dmode->clock = vm->pixelclock / 1000;

	dmode->flags = 0;
	if (vm->flags & DISPLAY_FLAGS_HSYNC_HIGH)
		dmode->flags |= DRM_MODE_FLAG_PHSYNC;
	else if (vm->flags & DISPLAY_FLAGS_HSYNC_LOW)
		dmode->flags |= DRM_MODE_FLAG_NHSYNC;
	if (vm->flags & DISPLAY_FLAGS_VSYNC_HIGH)
		dmode->flags |= DRM_MODE_FLAG_PVSYNC;
	else if (vm->flags & DISPLAY_FLAGS_VSYNC_LOW)
		dmode->flags |= DRM_MODE_FLAG_NVSYNC;
	if (vm->flags & DISPLAY_FLAGS_INTERLACED)
		dmode->flags |= DRM_MODE_FLAG_INTERLACE;
	if (vm->flags & DISPLAY_FLAGS_DOUBLESCAN)
		dmode->flags |= DRM_MODE_FLAG_DBLSCAN;
	if (vm->flags & DISPLAY_FLAGS_DOUBLECLK)
		dmode->flags |= DRM_MODE_FLAG_DBLCLK;
}

/**
 * drm_display_mode_to_videomode - fill in @vm using @dmode,
 * @dmode: drm_display_mode structure to use as source
 * @vm: videomode structure to use as destination
 *
 * Fills out @vm using the display mode specified in @dmode.
 */
void drm_display_mode_to_videomode(const struct drm_display_mode *dmode,
				   struct videomode *vm)
{
	vm->hactive = dmode->hdisplay;
	vm->hfront_porch = dmode->hsync_start - dmode->hdisplay;
	vm->hsync_len = dmode->hsync_end - dmode->hsync_start;
	vm->hback_porch = dmode->htotal - dmode->hsync_end;

	vm->vactive = dmode->vdisplay;
	vm->vfront_porch = dmode->vsync_start - dmode->vdisplay;
	vm->vsync_len = dmode->vsync_end - dmode->vsync_start;
	vm->vback_porch = dmode->vtotal - dmode->vsync_end;

	vm->pixelclock = dmode->clock * 1000;

	vm->flags = 0;
	if (dmode->flags & DRM_MODE_FLAG_PHSYNC)
		vm->flags |= DISPLAY_FLAGS_HSYNC_HIGH;
	else if (dmode->flags & DRM_MODE_FLAG_NHSYNC)
		vm->flags |= DISPLAY_FLAGS_HSYNC_LOW;
	if (dmode->flags & DRM_MODE_FLAG_PVSYNC)
		vm->flags |= DISPLAY_FLAGS_VSYNC_HIGH;
	else if (dmode->flags & DRM_MODE_FLAG_NVSYNC)
		vm->flags |= DISPLAY_FLAGS_VSYNC_LOW;
	if (dmode->flags & DRM_MODE_FLAG_INTERLACE)
		vm->flags |= DISPLAY_FLAGS_INTERLACED;
	if (dmode->flags & DRM_MODE_FLAG_DBLSCAN)
		vm->flags |= DISPLAY_FLAGS_DOUBLESCAN;
	if (dmode->flags & DRM_MODE_FLAG_DBLCLK)
		vm->flags |= DISPLAY_FLAGS_DOUBLECLK;
}

int videomode_to_fb_videomode(const struct videomode *vm,
			      struct fb_videomode *fbmode)
{
	unsigned int htotal, vtotal, total;

	fbmode->xres = vm->hactive;
	fbmode->left_margin = vm->hback_porch;
	fbmode->right_margin = vm->hfront_porch;
	fbmode->hsync_len = vm->hsync_len;

	fbmode->yres = vm->vactive;
	fbmode->upper_margin = vm->vback_porch;
	fbmode->lower_margin = vm->vfront_porch;
	fbmode->vsync_len = vm->vsync_len;

	/* prevent division by zero in KHZ2PICOS macro */
	fbmode->pixclock = vm->pixelclock ?
			KHZ2PICOS(vm->pixelclock / 1000) : 0;

	fbmode->sync = 0;
	fbmode->vmode = 0;
	if (vm->flags & DISPLAY_FLAGS_HSYNC_HIGH)
		fbmode->sync |= FB_SYNC_HOR_HIGH_ACT;
	if (vm->flags & DISPLAY_FLAGS_VSYNC_HIGH)
		fbmode->sync |= FB_SYNC_VERT_HIGH_ACT;
	if (vm->flags & DISPLAY_FLAGS_INTERLACED)
		fbmode->vmode |= FB_VMODE_INTERLACED;
	if (vm->flags & DISPLAY_FLAGS_DOUBLESCAN)
		fbmode->vmode |= FB_VMODE_DOUBLE;

	htotal = vm->hactive + vm->hfront_porch + vm->hback_porch +
		 vm->hsync_len;
	vtotal = vm->vactive + vm->vfront_porch + vm->vback_porch +
		 vm->vsync_len;
	/* prevent division by zero */
	total = htotal * vtotal;
	if (total) {
		fbmode->refresh = vm->pixelclock / total;
	/* a mode must have htotal and vtotal != 0 or it is invalid */
	} else {
		fbmode->refresh = 0;
		return -EINVAL;
	}

	return 0;
}

void fb_videomode_to_videomode(const struct fb_videomode *fbmode,
			       struct videomode *vm)
{
	vm->pixelclock = fbmode->pixclock ?
			 (PICOS2KHZ(fbmode->pixclock) * 1000) : 0;
	vm->hactive = fbmode->xres;
	vm->hfront_porch = fbmode->right_margin;
	vm->hback_porch = fbmode->left_margin;
	vm->hsync_len = fbmode->hsync_len;
	vm->vactive = fbmode->yres;
	vm->vfront_porch = fbmode->lower_margin;
	vm->vback_porch = fbmode->upper_margin;
	vm->vsync_len = fbmode->vsync_len;

	vm->flags = 0;

	if (fbmode->sync & FB_SYNC_HOR_HIGH_ACT)
		vm->flags |= DISPLAY_FLAGS_HSYNC_HIGH;
	if (fbmode->sync & FB_SYNC_VERT_HIGH_ACT)
		vm->flags |= DISPLAY_FLAGS_VSYNC_HIGH;
	if (fbmode->vmode & FB_VMODE_INTERLACED)
		vm->flags |= DISPLAY_FLAGS_INTERLACED;
	if (fbmode->vmode & FB_VMODE_DOUBLE)
		vm->flags |= DISPLAY_FLAGS_DOUBLESCAN;
}

void fb_videomode_to_drm_display_mode(const struct fb_videomode *fbmode,
				      struct drm_display_mode *dmode)
{
	struct videomode vm = {};

	fb_videomode_to_videomode(fbmode, &vm);
	videomode_to_drm_display_mode(&vm, dmode);
}

int drm_display_mode_to_fb_videomode(const struct drm_display_mode *dmode,
				     struct fb_videomode *fbmode)
{
	struct videomode vm = {};

	drm_display_mode_to_videomode(dmode, &vm);

	return videomode_to_fb_videomode(&vm, fbmode);
}
