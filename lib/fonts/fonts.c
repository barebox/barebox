/*
 * `Soft' font definitions
 *
 *    Created 1995 by Geert Uytterhoeven
 *    Rewritten 1998 by Martin Mares <mj@ucw.cz>
 *
 *	2001 - Documented with DocBook
 *	- Brad Douglas <brad@neruo.com>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file COPYING in the main directory of this archive
 * for more details.
 */

#include <module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/font.h>

#define NO_FONTS

static const struct font_desc *fonts[] = {
#ifdef CONFIG_FONT_8x16
#undef NO_FONTS
    &font_vga_8x16,
#endif
#ifdef CONFIG_FONT_7x14
#undef NO_FONTS
    &font_7x14,
#endif
#ifdef CONFIG_FONT_MINI_4x6
#undef NO_FONTS
    &font_mini_4x6,
#endif
};

#define num_fonts ARRAY_SIZE(fonts)

#ifdef NO_FONTS
#error No fonts configured.
#endif

static char *font_names;

const struct font_desc *find_font_enum(int n)
{
	if (n > num_fonts)
		return NULL;

	return fonts[n];
}

struct param_d *add_param_font(struct device_d *dev,
		int (*set)(struct param_d *p, void *priv),
		int (*get)(struct param_d *p, void *priv),
		int *value, void *priv)
{
	unsigned int i;

	if (!font_names) {
		font_names = xmalloc(sizeof(char *) * num_fonts);

		for (i = 0; i < num_fonts; i++)
			((const char **)font_names)[i] = fonts[i]->name;
	}

	return dev_add_param_enum(dev, "font",
			set, get, value,
			(const char **)font_names, num_fonts, priv);
}

MODULE_AUTHOR("James Simmons <jsimmons@users.sf.net>");
MODULE_DESCRIPTION("Console Fonts");
MODULE_LICENSE("GPL");
