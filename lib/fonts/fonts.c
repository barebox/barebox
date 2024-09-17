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

static char *font_names;

static LIST_HEAD(fonts_list);

int font_register(struct font_desc *font)
{
	if (font_names)
		return -EBUSY;

	list_add_tail(&font->list, &fonts_list);

	return 0;
}
int find_font_index(const struct font_desc *font, int ch)
{
	int index;
	if (font->index == NULL) {
		index  = DIV_ROUND_UP(font->width, 8);
		index *= font->height;
		index *= ch;
	} else {
		/*
		* FIXME: use binary search instead!
		*/
		index = font->num_chars - 1;

		while (index && font->index[index].wc != ch)
			index--;

		/* return 0 if not found. */
		index = font->index->index;
	}

	return index;
}

const struct font_desc *find_font_enum(int n)
{
	struct font_desc *f;
	int i = 0;

	list_for_each_entry(f, &fonts_list, list) {
		if (i == n)
			return f;
		i++;
	}

	return NULL;
}

struct param_d *add_param_font(struct device *dev,
			       int (*set)(struct param_d *p, void *priv),
			       int (*get)(struct param_d *p, void *priv),
			       int *value, void *priv)
{
	struct font_desc *f;
	int num_fonts = 0;

	list_for_each_entry(f, &fonts_list, list)
		num_fonts++;

	if (!font_names) {
		int i = 0;

		font_names = xmalloc(sizeof(char *) * num_fonts);

		list_for_each_entry(f, &fonts_list, list) {
			((const char **)font_names)[i] = f->name;
			i++;
		}
	}

	return dev_add_param_enum(dev, "font",
			set, get, value,
			(const char **)font_names, num_fonts, priv);
}

MODULE_AUTHOR("James Simmons <jsimmons@users.sf.net>");
MODULE_DESCRIPTION("Console Fonts");
MODULE_LICENSE("GPL");
