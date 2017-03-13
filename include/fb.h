#ifndef __FB_H
#define __FB_H

#include <ioctl.h>
#include <param.h>
#include <driver.h>
#include <linux/bitops.h>

#define FB_VISUAL_TRUECOLOR		2	/* True color	*/
#define FB_VISUAL_PSEUDOCOLOR		3	/* Pseudo color (like atari) */
#define FB_VISUAL_DIRECTCOLOR		4	/* Direct color */
#define FB_VISUAL_STATIC_PSEUDOCOLOR	5	/* Pseudo color readonly */

#define FB_SYNC_HOR_HIGH_ACT	1	/* horizontal sync high active	*/
#define FB_SYNC_VERT_HIGH_ACT	2	/* vertical sync high active	*/
#define FB_SYNC_EXT		4	/* external sync		*/
#define FB_SYNC_COMP_HIGH_ACT	8	/* composite sync high active   */
#define FB_SYNC_BROADCAST	16	/* broadcast video timings      */
					/* vtotal = 144d/288n/576i => PAL  */
					/* vtotal = 121d/242n/484i => NTSC */
#define FB_SYNC_ON_GREEN	32	/* sync on green */

#define FB_VMODE_NONINTERLACED  0	/* non interlaced */
#define FB_VMODE_INTERLACED	1	/* interlaced	*/
#define FB_VMODE_DOUBLE		2	/* double scan */
#define FB_VMODE_ODD_FLD_FIRST	4	/* interlaced: top line first */
#define FB_VMODE_MASK		255

#define FB_VMODE_YWRAP		256	/* ywrap instead of panning     */
#define FB_VMODE_SMOOTH_XPAN	512	/* smooth xpan possible (internally used) */
#define FB_VMODE_CONUPDATE	512	/* don't update x/yoffset	*/

#define PICOS2KHZ(a) (1000000000UL/(a))
#define KHZ2PICOS(a) (1000000000UL/(a))

enum display_flags {
	/* data enable flag */
	DISPLAY_FLAGS_DE_LOW		= BIT(4),
	DISPLAY_FLAGS_DE_HIGH		= BIT(5),
	/* drive data on pos. edge */
	DISPLAY_FLAGS_PIXDATA_POSEDGE	= BIT(6),
	/* drive data on neg. edge */
	DISPLAY_FLAGS_PIXDATA_NEGEDGE	= BIT(7),
};

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
	u32 display_flags;
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
	int (*fb_activate_var)(struct fb_info *info);
	void (*fb_flush)(struct fb_info *info);
};

/*
 * This describes all timing settings a display provides.
 * The native_mode is the default setting for this display.
 * Drivers that can handle multiple videomodes should work with this struct and
 * convert each entry to the desired end result.
 */
struct display_timings {
	unsigned int native_mode;

	unsigned int num_modes;
	struct fb_videomode *modes;
	void *edid;
};

struct i2c_adapter;

struct fb_info {
	struct fb_videomode *mode;
	struct display_timings modes;

	int current_mode;

	void *edid_data;
	struct i2c_adapter *edid_i2c_adapter;
	struct display_timings edid_modes;

	struct fb_ops *fbops;
	struct device_d dev;		/* This is this fb device */

	void *screen_base;
	void *screen_base_shadow;
	unsigned long screen_size;

	void *priv;

	struct cdev cdev;

	u32 xres;			/* visible resolution		*/
	u32 yres;
	u32 bits_per_pixel;		/* guess what			*/
	u32 line_length;		/* length of a line in bytes	*/

	u32 grayscale;			/* != 0 Graylevels instead of colors */

	struct fb_bitfield red;		/* bitfield in fb mem if true color, */
	struct fb_bitfield green;	/* else only length is significant */
	struct fb_bitfield blue;
	struct fb_bitfield transp;	/* transparency			*/

	int enabled;
	int p_enable;
	int register_simplefb;		/* If true a simplefb device node will
					 * be created.
					 */
	int shadowfb;
};

struct display_timings *of_get_display_timings(struct device_node *np);
void display_timings_release(struct display_timings *);

int register_framebuffer(struct fb_info *info);

int fb_enable(struct fb_info *info);
int fb_disable(struct fb_info *info);
void fb_flush(struct fb_info *info);

#define FBIOGET_SCREENINFO	_IOR('F', 1, loff_t)
#define	FBIO_ENABLE		_IO('F', 2)
#define	FBIO_DISABLE		_IO('F', 3)

extern struct bus_type fb_bus;

/* fb internal functions */

int fb_register_simplefb(struct fb_info *info);

int edid_to_display_timings(struct display_timings *, unsigned char *edid);
void *edid_read_i2c(struct i2c_adapter *adapter);
void fb_edid_add_modes(struct fb_info *info);
void fb_of_reserve_add_fixup(struct fb_info *info);

int register_fbconsole(struct fb_info *fb);
void *fb_get_screen_base(struct fb_info *info);

#endif /* __FB_H */
