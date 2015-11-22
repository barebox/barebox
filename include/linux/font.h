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
#include <wchar.h>

struct font_index {
	wchar_t wc;	/* code of the char. */
	short index;	/* offset of the char in the bitmap. */
};
struct font_desc {
	const char *name;
	int width, height;
	struct font_index *index;
	const void *data;
	int num_chars;
	struct list_head list;
};

/* Max. length for the name of a predefined font */
#define MAX_FONT_NAME	32

extern int find_font_index(const struct font_desc *font, int ch);
extern const struct font_desc *find_font_enum(int n);
extern struct param_d *add_param_font(struct device_d *dev,
		int (*set)(struct param_d *p, void *priv),
		int (*get)(struct param_d *p, void *priv),
		int *value, void *priv);

int font_register(struct font_desc *font);

#endif /* _VIDEO_FONT_H */
