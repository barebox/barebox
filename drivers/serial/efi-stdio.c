/*
 * efi_console.c - EFI console support
 *
 * Copyright (c) 2014 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
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
#include <driver.h>
#include <init.h>
#include <malloc.h>
#include <console.h>
#include <xfuncs.h>
#include <efi.h>
#include <readkey.h>
#include <linux/ctype.h>
#include <mach/efi.h>

#define EFI_SHIFT_STATE_VALID           0x80000000
#define EFI_RIGHT_CONTROL_PRESSED       0x00000004
#define EFI_LEFT_CONTROL_PRESSED        0x00000008
#define EFI_RIGHT_ALT_PRESSED           0x00000010
#define EFI_LEFT_ALT_PRESSED            0x00000020

#define EFI_CONTROL_PRESSED             (EFI_RIGHT_CONTROL_PRESSED | EFI_LEFT_CONTROL_PRESSED)
#define EFI_ALT_PRESSED                 (EFI_RIGHT_ALT_PRESSED | EFI_LEFT_ALT_PRESSED)
#define KEYPRESS(keys, scan, uni) ((((uint64_t)keys) << 32) | ((scan) << 16) | (uni))
#define KEYCHAR(k) ((k) & 0xffff)
#define CHAR_CTRL(c) ((c) - 'a' + 1)

#define EFI_BLACK   0x00
#define EFI_BLUE    0x01
#define EFI_GREEN   0x02
#define EFI_CYAN            (EFI_BLUE | EFI_GREEN)
#define EFI_RED     0x04
#define EFI_MAGENTA         (EFI_BLUE | EFI_RED)
#define EFI_BROWN           (EFI_GREEN | EFI_RED)
#define EFI_LIGHTGRAY       (EFI_BLUE | EFI_GREEN | EFI_RED)
#define EFI_BRIGHT  0x08
#define EFI_DARKGRAY        (EFI_BRIGHT)
#define EFI_LIGHTBLUE       (EFI_BLUE | EFI_BRIGHT)
#define EFI_LIGHTGREEN      (EFI_GREEN | EFI_BRIGHT)
#define EFI_LIGHTCYAN       (EFI_CYAN | EFI_BRIGHT)
#define EFI_LIGHTRED        (EFI_RED | EFI_BRIGHT)
#define EFI_LIGHTMAGENTA    (EFI_MAGENTA | EFI_BRIGHT)
#define EFI_YELLOW          (EFI_BROWN | EFI_BRIGHT)
#define EFI_WHITE           (EFI_BLUE | EFI_GREEN | EFI_RED | EFI_BRIGHT)

#define EFI_TEXT_ATTR(f,b)  ((f) | ((b) << 4))

#define EFI_BACKGROUND_BLACK        0x00
#define EFI_BACKGROUND_BLUE         0x10
#define EFI_BACKGROUND_GREEN        0x20
#define EFI_BACKGROUND_CYAN         (EFI_BACKGROUND_BLUE | EFI_BACKGROUND_GREEN)
#define EFI_BACKGROUND_RED          0x40
#define EFI_BACKGROUND_MAGENTA      (EFI_BACKGROUND_BLUE | EFI_BACKGROUND_RED)
#define EFI_BACKGROUND_BROWN        (EFI_BACKGROUND_GREEN | EFI_BACKGROUND_RED)
#define EFI_BACKGROUND_LIGHTGRAY    (EFI_BACKGROUND_BLUE | EFI_BACKGROUND_GREEN | EFI_BACKGROUND_RED)

struct efi_console_priv {
	struct efi_simple_text_output_protocol *out;
	struct efi_simple_input_interface *in;
	struct console_device cdev;
	int lastkey;
	u16 efi_console_buffer[CONFIG_CBSIZE];

	unsigned long columns, rows;

	int current_color;
	s16 *blank_line;
};

static inline struct efi_console_priv *to_efi(struct console_device *cdev)
{
	return container_of(cdev, struct efi_console_priv, cdev);
}

struct efi_ctrlkey {
	u8 scan_code;
	u8 bb_key;
};

static struct efi_ctrlkey ctrlkeys[] = {
	{ 0x01, BB_KEY_UP },
	{ 0x02, BB_KEY_DOWN },
	{ 0x03, BB_KEY_RIGHT },
	{ 0x04, BB_KEY_LEFT },
	{ 0x05, BB_KEY_HOME },
	{ 0x06, BB_KEY_END },
	{ 0x07, BB_KEY_INSERT },
	{ 0x08, BB_KEY_DEL7 },
	{ 0x09, BB_KEY_PAGEUP },
	{ 0x0a, BB_KEY_PAGEDOWN },
	{ 0x17, 27 /* escape key */ },
};

static int efi_read_key(struct efi_console_priv *priv, bool wait)
{
	unsigned long index;
	efi_status_t efiret;
	struct efi_input_key k;
	int i;

	/* wait until key is pressed */
	if (wait)
		BS->wait_for_event(1, priv->in->wait_for_key, &index);

        efiret = priv->in->read_key_stroke(efi_sys_table->con_in, &k);
        if (EFI_ERROR(efiret))
		return -efi_errno(efiret);

	/* 32 bit modifier keys + 16 bit scan code + 16 bit unicode */
	for (i = 0; i < ARRAY_SIZE(ctrlkeys); i++) {
		if (ctrlkeys[i].scan_code == k.scan_code)
			return ctrlkeys[i].bb_key;

	}

	return k.unicode_char & 0xff;
}

static void efi_console_putc(struct console_device *cdev, char c)
{
	uint16_t str[2] = {};
	struct efi_simple_text_output_protocol *con_out = efi_sys_table->con_out;

	str[0] = c;

	con_out->output_string(con_out, str);
}

static void clear_to_eol(struct efi_console_priv *priv)
{
	int pos = priv->out->mode->cursor_column;

	priv->out->output_string(priv->out, priv->blank_line + pos);
}

static int efi_process_square_bracket(struct efi_console_priv *priv, const char *inp)
{
	int x, y;
	char *endp;

	inp++;

	switch (*inp) {
	case 'A':
		/* Cursor up */
	case 'B':
		/* Cursor down */
	case 'C':
		/* Cursor right */
	case 'D':
		/* Cursor left */
	case 'H':
		/* home */
	case 'F':
		/* end */
		return 3;
	case 'K':
		clear_to_eol(priv);
		return 3;
	}

	if (*inp == '2' && *(inp + 1) == 'J') {
		priv->out->clear_screen(priv->out);
		return 4;
	}

	if (*inp == '0' && *(inp + 1) == 'm') {
		priv->out->set_attribute(priv->out,
				EFI_TEXT_ATTR(EFI_WHITE, EFI_BLACK));
		return 4;
	}

	if (*inp == '7' && *(inp + 1) == 'm') {
		priv->out->set_attribute(priv->out,
				EFI_TEXT_ATTR(EFI_BLACK, priv->current_color));
		return 4;
	}

	if (*inp == '1' &&
			*(inp + 1) == ';' &&
			*(inp + 2) == '3' &&
			*(inp + 3) &&
			*(inp + 4) == 'm') {
		int color;
		switch (*(inp + 3)) {
		case '1': color = EFI_RED; break;
		case '4': color = EFI_BLUE; break;
		case '2': color = EFI_GREEN; break;
		case '6': color = EFI_CYAN; break;
		case '3': color = EFI_YELLOW; break;
		case '5': color = EFI_MAGENTA; break;
		case '7': color = EFI_WHITE; break;
		default: color = EFI_WHITE; break;
		}

		priv->current_color = color;

		priv->out->set_attribute(priv->out,
				EFI_TEXT_ATTR(color, EFI_BLACK));
		return 7;
	}

	y = simple_strtoul(inp, &endp, 10);
	if (*endp == ';') {
		x = simple_strtoul(endp + 1, &endp, 10);
		if (*endp == 'H') {
			priv->out->set_cursor_position(priv->out, x - 1, y - 1);
			return endp - inp + 3;
		}
	}

	return 8;
}

static int efi_process_key(struct efi_console_priv *priv, const char *inp)
{
	char c;

	c = *inp;

	if (c != 27)
		return 0;

	inp++;

	if (*inp == '[')
		return efi_process_square_bracket(priv, inp);

	return 1;
}

static int efi_console_puts(struct console_device *cdev, const char *s)
{
	struct efi_console_priv *priv = to_efi(cdev);
	int n = 0;

	while (*s) {
		if (*s == 27) {
			priv->efi_console_buffer[n] = 0;
			priv->out->output_string(priv->out,
					priv->efi_console_buffer);
			n = 0;
			s += efi_process_key(priv, s);
			continue;
		}

		if (*s == '\n')
			priv->efi_console_buffer[n++] = '\r';
		priv->efi_console_buffer[n] = *s;
		s++;
		n++;
	}

	priv->efi_console_buffer[n] = 0;

	priv->out->output_string(priv->out, priv->efi_console_buffer);

	return n;
}

static int efi_console_tstc(struct console_device *cdev)
{
	struct efi_console_priv *priv = to_efi(cdev);
	int key;

	if (priv->lastkey > 0)
		return 1;

	key = efi_read_key(priv, 0);
	if (key < 0)
		return 0;

	priv->lastkey = key;

	return 1;
}

static int efi_console_getc(struct console_device *cdev)
{
	struct efi_console_priv *priv = to_efi(cdev);
	int key;

	if (priv->lastkey > 0) {
		key = priv->lastkey;
		priv->lastkey = -1;
		return key;
	}

	return efi_read_key(priv, 1);
}

static void efi_set_mode(struct efi_console_priv *priv)
{
#if 0
	int i;
	unsigned long rows, columns, best = 0, mode = 0;
	efi_status_t efiret;

	for (i = 0; i < priv->out->mode->max_mode; i++) {
		priv->out->query_mode(priv->out, i, &columns, &rows);
		printf("%d: %ld %ld\n", i, columns, rows);
		if (rows * columns > best) {
			best = rows * columns;
			mode = i;
		}
	}

	/*
	 * Setting the mode doesn't work as expected. set_mode succeeds, but
	 * the graphics resolution is not changed.
	 */
	priv->out->set_mode(priv->out, mode);
#endif
	priv->out->query_mode(priv->out, priv->out->mode->mode, &priv->columns, &priv->rows);
}

static int efi_console_probe(struct device_d *dev)
{
	struct console_device *cdev;
	struct efi_console_priv *priv;
	int i;

	priv = xzalloc(sizeof(*priv));

	priv->out = efi_sys_table->con_out;
	priv->in = efi_sys_table->con_in;

	priv->current_color = EFI_WHITE;

	efi_set_mode(priv);

	priv->out->enable_cursor(priv->out, 1);

	priv->blank_line = xzalloc((priv->columns + 1) * sizeof(s16));
	for (i = 0; i < priv->columns; i++)
		priv->blank_line[i] = ' ';

	cdev = &priv->cdev;
	cdev->dev = dev;
	cdev->tstc = efi_console_tstc;
	cdev->getc = efi_console_getc;
	cdev->putc = efi_console_putc;
	cdev->puts = efi_console_puts;

	priv->lastkey = -1;

	return console_register(cdev);
}

static struct driver_d efi_console_driver = {
        .name  = "efi-stdio",
        .probe = efi_console_probe,
};
console_platform_driver(efi_console_driver);
