#ifndef __INCLUDE_VIDEO_VIDEOMODE_H
#define __INCLUDE_VIDEO_VIDEOMODE_H

#include <fb.h>
#include <video/drm/drm_modes.h>

/*
 * Subsystem independent description of a videomode.
 * Can be generated from struct display_timing.
 */
struct videomode {
	unsigned long pixelclock;	/* pixelclock in Hz */

	u32 hactive;
	u32 hfront_porch;
	u32 hback_porch;
	u32 hsync_len;

	u32 vactive;
	u32 vfront_porch;
	u32 vback_porch;
	u32 vsync_len;

	enum display_flags flags; /* display flags */
};

void videomode_to_drm_display_mode(const struct videomode *vm,
				   struct drm_display_mode *dmode);
void drm_display_mode_to_videomode(const struct drm_display_mode *dmode,
				   struct videomode *vm);
int videomode_to_fb_videomode(const struct videomode *vm,
			      struct fb_videomode *fbmode);
void fb_videomode_to_videomode(const struct fb_videomode *fbmode,
			       struct videomode *vm);
void fb_videomode_to_drm_display_mode(const struct fb_videomode *fbmode,
				      struct drm_display_mode *dmode);
int drm_display_mode_to_fb_videomode(const struct drm_display_mode *dmode,
				     struct fb_videomode *fbmode);

#endif /* __INCLUDE_VIDEO_VIDEOMODE_H */
