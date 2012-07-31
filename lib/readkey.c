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
	char val;
};

static const struct esc_cmds esccmds[] = {
	{"OA", KEY_UP},       // cursor key Up
	{"OB", KEY_DOWN},     // cursor key Down
	{"OC", KEY_RIGHT},    // Cursor Key Right
	{"OD", KEY_LEFT},     // cursor key Left
	{"OH", KEY_HOME},     // Cursor Key Home
	{"OF", KEY_END},      // Cursor Key End
	{"[A", KEY_UP},       // cursor key Up
	{"[B", KEY_DOWN},     // cursor key Down
	{"[C", KEY_RIGHT},    // Cursor Key Right
	{"[D", KEY_LEFT},     // cursor key Left
	{"[H", KEY_HOME},     // Cursor Key Home
	{"[F", KEY_END},      // Cursor Key End
	{"[1~", KEY_HOME},    // Cursor Key Home
	{"[2~", KEY_INSERT},  // Cursor Key Insert
	{"[3~", KEY_DEL},     // Cursor Key Delete
	{"[4~", KEY_END},     // Cursor Key End
	{"[5~", KEY_PAGEUP},  // Cursor Key Page Up
	{"[6~", KEY_PAGEDOWN},// Cursor Key Page Down
};

int read_key(void)
{
	char c;
	char esc[5];
	c = getc();

	if (c == 27) {
		int i = 0;
		esc[i++] = getc();
		esc[i++] = getc();
		if (isdigit(esc[1])) {
			while(1) {
				esc[i] = getc();
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
