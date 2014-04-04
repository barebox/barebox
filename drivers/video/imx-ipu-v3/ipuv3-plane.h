#ifndef __IPUV3_PLANE_H__
#define __IPUV3_PLANE_H__

struct drm_plane;
struct drm_device;
struct ipu_soc;
struct drm_crtc;
struct drm_framebuffer;

struct ipuv3_channel;
struct dmfc_channel;
struct ipu_dp;

struct ipu_plane {
	struct ipu_soc		*ipu;
	struct ipuv3_channel	*ipu_ch;
	struct dmfc_channel	*dmfc;
	struct ipu_dp		*dp;

	int			dma;
	int			dp_flow;

	int			x;
	int			y;

	bool			enabled;
};

struct ipu_plane *ipu_plane_init(struct ipu_soc *ipu,
				 int dma, int dp);

/* Init IDMAC, DMFC, DP */
int ipu_plane_mode_set(struct ipu_plane *ipu_plane,
		       struct fb_videomode *mode,
		       struct fb_info *info, u32 pixel_format,
		       int crtc_x, int crtc_y,
		       unsigned int crtc_w, unsigned int crtc_h,
		       uint32_t src_x, uint32_t src_y,
		       uint32_t src_w, uint32_t src_h);

void ipu_plane_enable(struct ipu_plane *plane);
void ipu_plane_disable(struct ipu_plane *plane);
int ipu_plane_set_base(struct ipu_plane *ipu_plane, struct fb_info *info,
		       int x, int y);

int ipu_plane_get_resources(struct ipu_plane *plane);
void ipu_plane_put_resources(struct ipu_plane *plane);

int ipu_plane_irq(struct ipu_plane *plane);

#endif
