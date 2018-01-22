/*
 * sandbox barebox libftdi1 support
 *
 * Copyright (C) 2016, 2017 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <stdio.h>
#include <ftdi.h>

#define GPIOF_DIR_OUT	(0 << 0)
#define GPIOF_DIR_IN	(1 << 0)

#define BIT(nr)			(1UL << (nr))

struct ft2232_bitbang {
	struct ftdi_context *ftdi;
	uint8_t odata;
	uint8_t dir;
};

static struct ft2232_bitbang ftbb;

static inline int ftdi_flush(struct ftdi_context *ftdi)
{
	uint8_t buf[1];
	int ret;

	buf[0] = SEND_IMMEDIATE;

	ret = ftdi_write_data(ftdi, buf, 1);

	return ret;
}

static int ftdi_get_high_byte_data(struct ftdi_context *ftdi, uint8_t *data)
{
	uint8_t obuf;
	int ret;

	obuf = GET_BITS_HIGH;
	ret = ftdi_write_data(ftdi, &obuf, 1);

	ret = ftdi_read_data(ftdi, data, 1);

	return ret;
}

static int ftdi_set_high_byte_data_dir(struct ft2232_bitbang *ftbb)
{
	uint8_t buf[3];
	int ret;

	buf[0] = SET_BITS_HIGH;
	buf[1] = ftbb->odata;
	buf[2] = ftbb->dir;

	ret = ftdi_write_data(ftbb->ftdi, buf, 3);
	ftdi_flush(ftbb->ftdi);

	return ret;
}

int barebox_libftdi1_update(struct ft2232_bitbang *ftbb)
{
	return ftdi_set_high_byte_data_dir(ftbb);
}

void barebox_libftdi1_gpio_direction(struct ft2232_bitbang *ftbb,
					unsigned off, unsigned dir)
{
	switch (dir) {
	case GPIOF_DIR_IN:
		ftbb->dir &= ~BIT(off);
		break;
	case GPIOF_DIR_OUT:
		ftbb->dir |= BIT(off);
		break;
	default:
		printf("%s:%d: invalid dir argument\n", __FILE__, __LINE__);
	}
}

int barebox_libftdi1_gpio_get_value(struct ft2232_bitbang *ftbb, unsigned off)
{
	uint8_t data;

	ftdi_get_high_byte_data(ftbb->ftdi, &data);

	return !!(data & BIT(off));
}

void barebox_libftdi1_gpio_set_value(struct ft2232_bitbang *ftbb,
					unsigned off, unsigned val)
{
	if (val)
		ftbb->odata |= BIT(off);
	else
		ftbb->odata &= ~BIT(off);
}

struct ft2232_bitbang *barebox_libftdi1_open(int id_vendor, int id_product,
						const char *serial)
{
	struct ftdi_context *ftdi;
	int ret;

	ftdi = ftdi_new();
	if (!ftdi) {
		fprintf(stderr, "ftdi_new failed\n");
		goto error;
	}

	ret = ftdi_usb_open_desc(ftdi, id_vendor, id_product, NULL, serial);
	if (ret < 0 && ret != -5) {
		fprintf(stderr, "unable to open ftdi device: %d (%s)\n",
			ret, ftdi_get_error_string(ftdi));
		goto error;
	}

	ftdi_set_interface(ftdi, INTERFACE_A);
	ftdi_set_bitmode(ftdi, 0x00, BITMODE_MPSSE);

	ftbb.ftdi = ftdi;

	/* reset pins to default neutral state */
	ftbb.dir = 0;
	ftbb.odata = 0;
	ftdi_set_high_byte_data_dir(&ftbb);

	return &ftbb;

error:
	return NULL;
}

void barebox_libftdi1_close(void)
{
	struct ftdi_context *ftdi = ftbb.ftdi;

	ftdi_set_interface(ftdi, INTERFACE_ANY);
	ftdi_usb_close(ftdi);
	ftdi_free(ftdi);
}
