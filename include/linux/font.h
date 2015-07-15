/*
 *  font.h -- `Soft' font definitions
 *
 *  Created 1995 by Geert Uytterhoeven
 *
 *  This file is subject to the terms and conditions of the GNU General Public
 *  License.  See the file COPYING in the main directory of this archive
 *  for more details.
 */

#ifndef _VIDEO_FONT_H
#define _VIDEO_FONT_H

#include <param.h>

struct font_desc {
	const char *name;
	int width, height;
	const void *data;
};

extern const struct font_desc	font_vga_8x16,
			font_7x14,
			font_mini_4x6;

/* Max. length for the name of a predefined font */
#define MAX_FONT_NAME	32

extern const struct font_desc *find_font_enum(int n);
extern struct param_d *add_param_font(struct device_d *dev,
		int (*set)(struct param_d *p, void *priv),
		int (*get)(struct param_d *p, void *priv),
		int *value, void *priv);

#endif /* _VIDEO_FONT_H */
