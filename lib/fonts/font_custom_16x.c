/*
 * by Du Huanpeng <u74147@gmail.com>
 */

#include <init.h>
#include <module.h>
#include <linux/font.h>
#include <common.h>

/* place real font data here or set fontdata_custom_16x points to
 * the address of font data and also setup the index.
 */

static const unsigned char fontdata_custom_16x[] = {
	0xFF, 0xFF,	/*OOOOOOOOOOOOOOOO*/
	0x80, 0x01,	/*O______________O*/
	0x80, 0x01,	/*O______________O*/
	0x80, 0x01,	/*O______________O*/
	0x80, 0x01,	/*O______________O*/
	0x80, 0x01,	/*O______________O*/
	0x80, 0x01,	/*O______________O*/
	0x80, 0x01,	/*O______________O*/
	0x80, 0x01,	/*O______________O*/
	0x80, 0x01,	/*O______________O*/
	0x80, 0x01,	/*O______________O*/
	0x80, 0x01,	/*O______________O*/
	0x80, 0x01,	/*O______________O*/
	0x80, 0x01,	/*O______________O*/
	0x80, 0x01,	/*O______________O*/
	0xFF, 0xFF,	/*OOOOOOOOOOOOOOOO*/
};

static struct font_index fontdata_custom_16x_index[] = {
	{ 0x0000, 0x0000 },
};

static struct font_desc font_custom_16x = {
	.name	= "CUSTOM-16x",
	.width	= 16,
	.height	= 16,
	.data	= fontdata_custom_16x,
	.index	= fontdata_custom_16x_index,
	.num_chars = ARRAY_SIZE(fontdata_custom_16x_index),
};

static int font_custom_16x_register(void)
{
	return font_register(&font_custom_16x);
}
postcore_initcall(font_custom_16x_register);
