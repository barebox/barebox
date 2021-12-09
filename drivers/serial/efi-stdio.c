// SPDX-License-Identifier: GPL-2.0
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
#include <efi/efi-device.h>
#include <efi/efi-stdio.h>

struct efi_console_priv {
	struct efi_simple_text_output_protocol *out;
	struct efi_simple_input_interface *in;
	struct efi_simple_text_input_ex_protocol *inex;
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

static int efi_read_key(struct efi_console_priv *priv, bool wait)
{
	unsigned long index;
	efi_status_t efiret;
	struct efi_key_data kd;

	/* wait until key is pressed */
	if (wait)
		BS->wait_for_event(1, priv->in->wait_for_key, &index);

	if (priv->inex) {
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

	efiret = priv->in->read_key_stroke(priv->in, &kd.key);

	if (EFI_ERROR(efiret))
		return -efi_errno(efiret);

	return xlate_keypress(&kd.key);
}

static void efi_console_putc(struct console_device *cdev, char c)
{
	uint16_t str[2] = {};
	struct efi_console_priv *priv = to_efi(cdev);
	struct efi_simple_text_output_protocol *con_out = priv->out;

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
		return 2;
	case 'K':
		clear_to_eol(priv);
		return 2;
	}

	if (*inp == '2' && *(inp + 1) == 'J') {
		priv->out->clear_screen(priv->out);
		return 3;
	}

	if (*inp == '0' && *(inp + 1) == 'm') {
		priv->out->set_attribute(priv->out,
				EFI_TEXT_ATTR(EFI_WHITE, EFI_BLACK));
		return 3;
	}

	if (*inp == '7' && *(inp + 1) == 'm') {
		priv->out->set_attribute(priv->out,
				EFI_TEXT_ATTR(EFI_BLACK, priv->current_color));
		return 3;
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
		return 6;
	}

	y = simple_strtoul(inp, &endp, 10);
	if (*endp == ';') {
		x = simple_strtoul(endp + 1, &endp, 10);
		if (*endp == 'H') {
			priv->out->set_cursor_position(priv->out, x - 1, y - 1);
			return endp - inp + 2;
		}
	}

	return 7;
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

static int efi_console_puts(struct console_device *cdev, const char *s,
			    size_t nbytes)
{
	struct efi_console_priv *priv = to_efi(cdev);
	int n = 0;

	while (nbytes--) {
		if (*s == 27) {
			priv->efi_console_buffer[n] = 0;
			priv->out->output_string(priv->out,
					priv->efi_console_buffer);
			n = 0;
			s += efi_process_escape(priv, s);
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
	efi_guid_t inex_guid = EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL_GUID;
	struct efi_simple_text_input_ex_protocol *inex;
	struct console_device *cdev;
	struct efi_console_priv *priv;
	efi_status_t efiret;

	int i;

	priv = xzalloc(sizeof(*priv));

	priv->out = efi_sys_table->con_out;
	priv->in = efi_sys_table->con_in;

	efiret = BS->open_protocol((void *)efi_sys_table->con_in_handle,
			     &inex_guid,
			     (void **)&inex,
			     efi_parent_image,
			     0,
			     EFI_OPEN_PROTOCOL_GET_PROTOCOL);

	if (!EFI_ERROR(efiret)) {
		priv->inex = inex;
		dev_dbg(dev, "Using simple_text_input_ex_protocol\n");
	}

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
