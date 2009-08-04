#ifndef __FB_H
#define __FB_H

#include <ioctl.h>
#include <param.h>

#define FB_VISUAL_TRUECOLOR		2	/* True color	*/
#define FB_VISUAL_PSEUDOCOLOR		3	/* Pseudo color (like atari) */
#define FB_VISUAL_DIRECTCOLOR		4	/* Direct color */
#define FB_VISUAL_STATIC_PSEUDOCOLOR	5	/* Pseudo color readonly */

struct fb_videomode {
	const char *name;	/* optional */
	u32 refresh;		/* optional */
	u32 xres;
	u32 yres;
	u32 pixclock;
	u32 left_margin;
	u32 right_margin;
	u32 upper_margin;
	u32 lower_margin;
	u32 hsync_len;
	u32 vsync_len;
	u32 sync;
	u32 vmode;
	u32 flag;
};

/* Interpretation of offset for color fields: All offsets are from the right,
 * inside a "pixel" value, which is exactly 'bits_per_pixel' wide (means: you
 * can use the offset as right argument to <<). A pixel afterwards is a bit
 * stream and is written to video memory as that unmodified.
 *
 * For pseudocolor: offset and length should be the same for all color
 * components. Offset specifies the position of the least significant bit
 * of the pallette index in a pixel value. Length indicates the number
 * of available palette entries (i.e. # of entries = 1 << length).
 */
struct fb_bitfield {
	u32 offset;			/* beginning of bitfield	*/
	u32 length;			/* length of bitfield		*/
	u32 msb_right;			/* != 0 : Most significant bit is */ 
					/* right */ 
};

struct fb_info;

struct fb_ops {
	/* set color register */
	int (*fb_setcolreg)(unsigned regno, unsigned red, unsigned green,
			    unsigned blue, unsigned transp, struct fb_info *info);
	void (*fb_enable)(struct fb_info *info);
	void (*fb_disable)(struct fb_info *info);
};

struct fb_info {
	struct fb_videomode *mode;

	struct fb_ops *fbops;
	struct device_d dev;		/* This is this fb device */
	struct param_d param_enable;
	char enable_string[1];

	void *screen_base;

	void *priv;

	struct cdev cdev;

	u32 xres;			/* visible resolution		*/
	u32 yres;
	u32 bits_per_pixel;		/* guess what			*/

	u32 grayscale;			/* != 0 Graylevels instead of colors */

	struct fb_bitfield red;		/* bitfield in fb mem if true color, */
	struct fb_bitfield green;	/* else only length is significant */
	struct fb_bitfield blue;
	struct fb_bitfield transp;	/* transparency			*/	
};

int register_framebuffer(struct fb_info *info);

#define FBIOGET_SCREENINFO	_IOR('F', 1, loff_t)
#define	FBIO_ENABLE		_IO('F', 2)
#define	FBIO_DISABLE		_IO('F', 3)

#endif /* __FB_H */

