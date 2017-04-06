/*
 * Copyright (C) 2015 Juergen Borleis <kernel@pengutronix.de>
 *
 * Based on code from:
 * Copyright (C) 2013 Jan Luebbe <j.luebbe@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <common.h>
#include <errno.h>
#include <fcntl.h>
#include <fs.h>
#include <globalvar.h>
#include <xfuncs.h>
#include <init.h>
#include <net.h>

#define PCM030_EEPROM_DEVICE "/dev/eeprom0"

/*
 * The first 2048 bytes contain the U-Boot environment shipped with the boards.
 * After that an area begins where some board specific and partially unique data
 * is stored. All of this data is plain test, string delimiter is the semicolon.
 * the last string terminates with a '\0'.
 * One example found in the 'product' area: PCM-030-02REI;017822;0050C2875974\0
 * The first string seems to be the product type, the second string some kind
 * of serial number and the last string the boards unique MAC.
 */
struct pcm030_eeprom {
	char env[0x800]; /* u-boot environment */
	char product[0x80]; /* <product>;<serno>;<mac>\0 */
} __attribute__((packed));

static void pcm030_read_factory_data(const struct pcm030_eeprom *buf)
{
	unsigned u, l;
	char *board, *serial;
	char *full_mac = "xx:xx:xx:xx:xx:xx";
	u8 enetaddr[6];

	u = 0;

	for (l = 0; (l + u) < sizeof(buf->product); l++) {
		if (buf->product[u + l] != ';')
			continue;
		board = xstrndup(&buf->product[u], l);
		u += l + 1;
		globalvar_add_simple_string_fixed("model.type", board);
		free(board);
	}

	for (l = 0; (l + u) < sizeof(buf->product); l++) {
		if (buf->product[u + l] != ';')
			continue;
		serial = xstrndup(&buf->product[u], l);
		u += l + 1;
		globalvar_add_simple_string_fixed("model.serial", serial);
		free(serial);
	}

	/* for the MAC do a simple duck test */
	if (buf->product[u] != ';' && buf->product[u + 12] == '\0') {
		full_mac[0] = buf->product[u + 0];
		full_mac[1] = buf->product[u + 1];
		full_mac[3] = buf->product[u + 2];
		full_mac[4] = buf->product[u + 3];
		full_mac[6] = buf->product[u + 4];
		full_mac[7] = buf->product[u + 5];
		full_mac[9] = buf->product[u + 6];
		full_mac[10] = buf->product[u + 7];
		full_mac[12] = buf->product[u + 8];
		full_mac[13] = buf->product[u + 9];
		full_mac[15] = buf->product[u + 10];
		full_mac[16] = buf->product[u + 11];
		string_to_ethaddr(full_mac, enetaddr);
		eth_register_ethaddr(0, enetaddr);
		return;
	}

	printf("EEPROM contains no ethernet MAC\n");
}

static int pcm030_eeprom_read(void)
{
	int fd, ret;
	struct pcm030_eeprom *buf;

	fd = open(PCM030_EEPROM_DEVICE, O_RDONLY);
	if (fd < 0) {
		perror("failed to open " PCM030_EEPROM_DEVICE);
		return fd;
	}

	buf = xmalloc(sizeof(struct pcm030_eeprom));

	ret = pread(fd, buf, sizeof(struct pcm030_eeprom), 0);
	if (ret < sizeof(struct pcm030_eeprom)) {
		perror("failed to read " PCM030_EEPROM_DEVICE);
		goto out;
	}

	pcm030_read_factory_data(buf);
	ret = 0;
out:
	free(buf);
	close(fd);

	return ret;
}
late_initcall(pcm030_eeprom_read);
