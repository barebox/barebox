// SPDX-License-Identifier: GPL-2.0-only
/*
 * efi_console.c - EFI console support
 *
 * Copyright (c) 2014 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
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
#include <efi/efi-payload.h>
#include <kfifo.h>
#include <efi/efi-device.h>
#include <efi/efi-stdio.h>

struct efi_console_priv {
	struct efi_simple_text_output_protocol *out;
	struct efi_simple_text_input_protocol *in;
	struct efi_simple_text_input_ex_protocol *inex;
	struct console_device cdev;
	u16 efi_console_buffer[CONFIG_CBSIZE + 1];
	int pos;

	struct kfifo *inputbuffer;

	unsigned long columns, rows;

	int fg;
	int bg;
	bool inverse;
	s16 *blank_line;

	struct param_d *param_mode;
	const char **mode_names;
	int *mode_num;
	unsigned int var_mode;
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

static int xlate_keypress(struct efi_input_key *k)
{
	int i;

	/* 32 bit modifier keys + 16 bit scan code + 16 bit unicode */
	for (i = 0; i < ARRAY_SIZE(ctrlkeys); i++) {
		if (ctrlkeys[i].scan_code == k->scan_code)
			return ctrlkeys[i].bb_key;

	}

	return k->unicode_char & 0xff;
}

static void efi_wait_single_event(struct efi_event *event)
{
	size_t index;

	/* wait until key is pressed */
	BS->wait_for_event(1, &event, &index);
}

static int efi_read_key(struct efi_console_priv *priv, bool wait)
{
	efi_status_t efiret;
	struct efi_key_data kd;

	if (priv->inex) {
		if (wait)
			efi_wait_single_event(priv->inex->wait_for_key_ex);

		efiret = priv->inex->read_key_stroke_ex(priv->inex, &kd);

		if (efiret == EFI_NOT_READY)
			return -EAGAIN;

		if (!EFI_ERROR(efiret)) {
			if ((kd.state.shift_state & EFI_SHIFT_STATE_VALID) &&
			    (kd.state.shift_state & EFI_CONTROL_PRESSED)) {
				int ch = tolower(kd.key.unicode_char & 0xff);

				if (isalpha(ch))
					return CHAR_CTRL(ch);
				if (ch == '\0') /* ctrl is pressed on its own */
					return -EAGAIN;
			}

			if (kd.key.unicode_char || kd.key.scan_code)
				return xlate_keypress(&kd.key);

			/* Some broken firmwares offer simple_text_input_ex_protocol,
			 * but never handle any key. Treat those as if
			 * read_key_stroke_ex failed and fall through
			 * to the basic simple_text_input_protocol.
			 */
			dev_dbg(priv->cdev.dev, "Falling back to simple_text_input_protocol\n");
		}
	}

	if (wait)
		efi_wait_single_event(priv->in->wait_for_key);

	efiret = priv->in->read_key_stroke(priv->in, &kd.key);

	if (EFI_ERROR(efiret))
		return -efi_errno(efiret);

	return xlate_keypress(&kd.key);
}

static void clear_to_eol(struct efi_console_priv *priv)
{
	int pos = priv->out->mode->cursor_column;

	priv->out->output_string(priv->out, priv->blank_line + pos);
}

static int ansi_to_efi_color(int ansi)
{
	switch (ansi) {
	case 30:
		return EFI_BLACK;
	case 31:
		return EFI_RED;
	case 32:
		return EFI_GREEN;
	case 33:
		return EFI_YELLOW;
	case 34:
		return EFI_BLUE;
	case 35:
		return EFI_MAGENTA;
	case 36:
		return EFI_CYAN;
	case 37:
		return EFI_WHITE;
	case 39:
		return EFI_LIGHTGRAY;
	}

	return -1;
}

static void set_fg_bg_colors(struct efi_console_priv *priv)
{
	int fg = priv->inverse ? priv->bg : priv->fg;
	int bg = priv->inverse ? priv->fg : priv->bg;

	priv->out->set_attribute(priv->out, EFI_TEXT_ATTR(fg , bg));
}

static int efi_process_square_bracket(struct efi_console_priv *priv, const char *inp)
{
	char *endp;
	int n, retlen;
	int arg0 = -1, arg1 = -1, arg2 = -1;
	char *buf;

	endp = strpbrk(inp, "ABCDEFGHJKmrnhl");
	if (!endp)
		return 0;

	retlen = endp - inp + 1;

	inp++;

	if (isdigit(*inp)) {
		char *e;
		arg0 = simple_strtoul(inp, &e, 10);
		if (*e == ';') {
			arg1 = simple_strtoul(e + 1, &e, 10);
			if (*e == ';')
				arg2 = simple_strtoul(e + 1, &e, 10);
		}
	} else if (*inp == '?') {
		arg0 = simple_strtoul(inp + 1, NULL, 10);
	}

	switch (*endp) {
	case 'K':
		switch (arg0) {
		case 0:
		case -1:
			clear_to_eol(priv);
			break;
		}
		break;
	case 'J':
		switch (arg0) {
		case 2:
			priv->out->clear_screen(priv->out);
			break;
		}
		break;
	case 'H':
		if (arg0 >= 0 && arg1 >= 0) {
			int row = min_t(int, arg0, priv->rows);
			int col = min_t(int, arg1, priv->columns);
			priv->out->set_cursor_position(priv->out, col - 1, row - 1);
		}
		break;
	case 'm':
		switch (arg0) {
		case 0:
			priv->inverse = false;
			priv->fg = EFI_LIGHTGRAY;
			priv->bg = EFI_BLACK;
			set_fg_bg_colors(priv);
			break;
		case 7:
			priv->inverse = true;
			set_fg_bg_colors(priv);
			break;
		case 1:
			priv->fg = ansi_to_efi_color(arg1);
			if (priv->fg < 0)
				priv->fg = EFI_LIGHTGRAY;
			priv->bg = ansi_to_efi_color(arg2);
			if (priv->bg < 0)
				priv->bg = EFI_BLACK;
			set_fg_bg_colors(priv);
			break;
		}
		break;
	case 'n':
		switch (arg0) {
		case 6:
			n = asprintf(&buf, "\033[%d;%dR", priv->out->mode->cursor_row + 1,
				priv->out->mode->cursor_column + 1);
			kfifo_put(priv->inputbuffer, buf, n);
			free(buf);
			break;
		}
		break;
	case 'h':
		if (*inp == '?' && arg0 == 25)
			priv->out->enable_cursor(priv->out, true);
		break;
	case 'l':
		if (*inp == '?' && arg0 == 25)
			priv->out->enable_cursor(priv->out, false);
		break;
	}

	return retlen;
}

static int efi_process_escape(struct efi_console_priv *priv, const char *inp)
{
	char c;

	c = *inp;

	inp++;

	if (*inp == '[')
		return efi_process_square_bracket(priv, inp) + 1;

	return 1;
}

static void efi_console_add_char(struct efi_console_priv *priv, int c)
{
	if (priv->pos >= CONFIG_CBSIZE)
		return;

	priv->efi_console_buffer[priv->pos] = c;
	priv->pos++;
}

static void efi_console_flush(struct efi_console_priv *priv)
{
	priv->efi_console_buffer[priv->pos] = 0;

	priv->out->output_string(priv->out, priv->efi_console_buffer);

	priv->pos = 0;
}

static int efi_console_puts(struct console_device *cdev, const char *s,
			    size_t nbytes)
{
	struct efi_console_priv *priv = to_efi(cdev);
	int n, pos = 0;

	while (pos < nbytes) {
		switch (s[pos]) {
		case 27:
			efi_console_flush(priv);
			pos += efi_process_escape(priv, s + pos);
			break;
		case '\n':
			efi_console_add_char(priv, '\r');
			efi_console_add_char(priv, '\n');
			pos++;
			break;
		case '\t':
			efi_console_flush(priv);
			n = 8 - priv->out->mode->cursor_column % 8;
			while (n--)
				efi_console_add_char(priv, ' ');
			pos++;
			break;
		case '\b':
			n = priv->out->mode->cursor_column;
			if (n > 0)
				priv->out->set_cursor_position(priv->out,
					n - 1, priv->out->mode->cursor_row);
			pos++;
			break;
		default:
			efi_console_add_char(priv, s[pos]);
			pos++;
			break;
		}
	}

	efi_console_flush(priv);

	return nbytes;
}

static void efi_console_putc(struct console_device *cdev, char c)
{
	efi_console_puts(cdev, &c, 1);
}

static int efi_console_tstc(struct console_device *cdev)
{
	struct efi_console_priv *priv = to_efi(cdev);
	int key;

	if (kfifo_len(priv->inputbuffer))
		return 1;

	key = efi_read_key(priv, 0);
	if (key < 0)
		return 0;

	kfifo_putc(priv->inputbuffer, key);

	return 1;
}

static int efi_console_getc(struct console_device *cdev)
{
	struct efi_console_priv *priv = to_efi(cdev);
	unsigned char c;

	if (!kfifo_getc(priv->inputbuffer, &c))
		return c;

	return efi_read_key(priv, 1);
}

static int efi_console_set_mode(struct param_d *param, void *p)
{
	struct efi_console_priv *priv = p;

	priv->out->set_mode(priv->out, priv->mode_num[priv->var_mode]);

	priv->out->query_mode(priv->out, priv->out->mode->mode,
			      &priv->columns, &priv->rows);
	return 0;
}

static void efi_set_mode(struct efi_console_priv *priv)
{
	int i;
	unsigned long rows, columns;
	int n = 0;
	efi_status_t efiret;

	priv->mode_names = xzalloc(priv->out->mode->max_mode * sizeof(*priv->mode_names));
	priv->mode_num = xzalloc(priv->out->mode->max_mode * sizeof(*priv->mode_num));

	priv->out->query_mode(priv->out, priv->out->mode->mode, &priv->columns, &priv->rows);

	for (i = 0; i < priv->out->mode->max_mode; i++) {
		efiret = priv->out->query_mode(priv->out, i, &columns, &rows);
		if (EFI_ERROR(efiret))
			continue;

		if (columns == priv->columns && rows == priv->rows)
			priv->var_mode = n;

		priv->mode_names[n] = basprintf("%ldx%ld", columns, rows);
		priv->mode_num[n] = i;
		n++;
	}

	priv->param_mode = dev_add_param_enum(&priv->cdev.class_dev, "mode",
					efi_console_set_mode, NULL, &priv->var_mode,
				       priv->mode_names, n, priv);
}

static int efi_console_probe(struct device *dev)
{
	efi_guid_t inex_guid = EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL_GUID;
	struct efi_simple_text_input_ex_protocol *inex;
	struct console_device *cdev;
	struct efi_console_priv *priv;
	efi_status_t efiret;
	int i, ret;

	priv = xzalloc(sizeof(*priv));

	priv->out = efi_sys_table->con_out;
	priv->in = efi_sys_table->con_in;

	priv->inputbuffer = kfifo_alloc(128);
	if (!priv->inputbuffer)
		return -ENOMEM;

	efiret = BS->open_protocol(efi_sys_table->con_in_handle,
			     &inex_guid,
			     (void **)&inex,
			     efi_parent_image,
			     0,
			     EFI_OPEN_PROTOCOL_GET_PROTOCOL);

	if (!EFI_ERROR(efiret)) {
		priv->inex = inex;
		dev_dbg(dev, "Using simple_text_input_ex_protocol\n");
	}

	priv->fg = EFI_LIGHTGRAY;
	priv->bg = EFI_BLACK;

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

	ret = console_register(cdev);
	if (ret)
		return ret;

	efi_set_mode(priv);

	return 0;
}

static struct driver efi_console_driver = {
        .name  = "efi-stdio",
        .probe = efi_console_probe,
};
console_platform_driver(efi_console_driver);
