/*
 * readkey.c - read keystrokes and decode standard escape sequences
 *
 * Partly based on busybox vi
 *
 * Copyright (c) 2007 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <linux/ctype.h>
#include <readkey.h>

struct esc_cmds {
	const char *seq;
	unsigned char val;
};

static const struct esc_cmds esccmds[] = {
	{"OA", BB_KEY_UP},       // cursor key Up
	{"OB", BB_KEY_DOWN},     // cursor key Down
	{"OC", BB_KEY_RIGHT},    // Cursor Key Right
	{"OD", BB_KEY_LEFT},     // cursor key Left
	{"OH", BB_KEY_HOME},     // Cursor Key Home
	{"OF", BB_KEY_END},      // Cursor Key End
	{"[A", BB_KEY_UP},       // cursor key Up
	{"[B", BB_KEY_DOWN},     // cursor key Down
	{"[C", BB_KEY_RIGHT},    // Cursor Key Right
	{"[D", BB_KEY_LEFT},     // cursor key Left
	{"[H", BB_KEY_HOME},     // Cursor Key Home
	{"[F", BB_KEY_END},      // Cursor Key End
	{"[1~", BB_KEY_HOME},    // Cursor Key Home
	{"[2~", BB_KEY_INSERT},  // Cursor Key Insert
	{"[3~", BB_KEY_DEL},     // Cursor Key Delete
	{"[4~", BB_KEY_END},     // Cursor Key End
	{"[5~", BB_KEY_PAGEUP},  // Cursor Key Page Up
	{"[6~", BB_KEY_PAGEDOWN},// Cursor Key Page Down
};

int read_key(void)
{
	unsigned char c;
	unsigned char esc[5];
	c = getchar();

	if (c == 27) {
		int i = 0;
		esc[i++] = getchar();
		esc[i++] = getchar();
		if (isdigit(esc[1])) {
			while(1) {
				esc[i] = getchar();
				if (esc[i++] == '~')
					break;
				if (i == ARRAY_SIZE(esc))
					return -1;
			}
		}
		esc[i] = 0;
		for (i = 0; i < ARRAY_SIZE(esccmds); i++){
			if (!strcmp(esc, esccmds[i].seq))
				return esccmds[i].val;
		}
		return -1;
	}
	return c;
}
EXPORT_SYMBOL(read_key);
