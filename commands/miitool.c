/*
 * Copyright (C) 2012 Antony Pavlov <antonynpavlov@gmail.com>
 *
 * This file is part of barebox.
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is based on Donald Becker's "mii-diag" and
 * David A. Hinds' "mii-tool".
 *
 * mii-tool is written/copyright 2000 by David A. Hinds
 *     <dhinds@pcmcia.sourceforge.org>
 *
 * mii-diag is written/copyright 1997-2000 by Donald Becker
 *     <becker@scyld.com>
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
#include <command.h>
#include <init.h>
#include <driver.h>
#include <malloc.h>
#include <errno.h>
#include <fs.h>
#include <fcntl.h>
#include <getopt.h>
#include <linux/stat.h>
#include <xfuncs.h>
#include <linux/mii.h>

static u16 mdio_read(int fd, int offset)
{
	int ret;
	u16 buf;

	ret = lseek(fd, offset << 1, SEEK_SET);
	if (ret < 0)
		return 0;

	ret = read(fd, &buf, sizeof(u16));
	if (ret < 0)
		return 0;

	return buf;
}

/* Table of known MII's */
static const struct {
	u_short	id1, id2;
	u_short	mask1, mask2;
	char	*name;
} mii_id[] = {
	{ 0x0013, 0x78e0, 0xffff, 0xfff0, "Level One LXT971A" },
};
#define NMII (sizeof(mii_id)/sizeof(mii_id[0]))

const struct {
	char	*name;
	u_short	value;
} media[] = {
	/* The order through 100baseT4 matches bits in the BMSR */
	{ "10baseT-HD",	ADVERTISE_10HALF },
	{ "10baseT-FD",	ADVERTISE_10FULL },
	{ "100baseTx-HD",	ADVERTISE_100HALF },
	{ "100baseTx-FD",	ADVERTISE_100FULL },
	{ "100baseT4",	LPA_100BASE4 },
	{ "100baseTx",	ADVERTISE_100FULL | ADVERTISE_100HALF },
	{ "10baseT",	ADVERTISE_10FULL | ADVERTISE_10HALF },
};
#define NMEDIA (sizeof(media)/sizeof(media[0]))

static char *media_list(int mask, int best)
{
	static char buf[100];
	int i;

	*buf = '\0';
	mask >>= 5;
	for (i = 4; i >= 0; i--) {
		if (mask & (1 << i)) {
			strcat(buf, " ");
			strcat(buf, media[i].name);
			if (best)
				break;
		}
	}

	if (mask & (1 << 5))
		strcat(buf, " flow-control");

	return buf;
}

static int show_basic_mii(int fd, int verbose)
{
	char buf[100];
	int i, mii_val[32];
	int bmcr, bmsr, advert, lkpar;

	/* Some bits in the BMSR are latched, but we can't rely on being
	   the only reader, so only the current values are meaningful */
	mdio_read(fd, MII_BMSR);
	for (i = 0; i < ((verbose > 1) ? 32 : 8); i++)
		mii_val[i] = mdio_read(fd, i);

	if (mii_val[MII_BMCR] == 0xffff) {
		fprintf(stderr, "  No MII transceiver present!.\n");
		return -1;
	}

	/* Descriptive rename. */
	bmcr = mii_val[MII_BMCR];
	bmsr = mii_val[MII_BMSR];
	advert = mii_val[MII_ADVERTISE];
	lkpar = mii_val[MII_LPA];

	*buf = '\0';
	if (bmcr & BMCR_ANENABLE) {
		if (bmsr & BMSR_ANEGCOMPLETE) {
			if (advert & lkpar) {
				sprintf(buf, "%s%s, ", (lkpar & LPA_LPACK) ?
					"negotiated" : "no autonegotiation,",
					media_list(advert & lkpar, 1));
			} else {
				sprintf(buf, "autonegotiation failed, ");
			}
		} else if (bmcr & BMCR_ANRESTART) {
			sprintf(buf, "autonegotiation restarted, ");
		}
	} else {
		sprintf(buf, "%s Mbit, %s duplex, ",
			(bmcr & BMCR_SPEED100) ? "100" : "10",
			(bmcr & BMCR_FULLDPLX) ? "full" : "half");
	}

	strcat(buf, (bmsr & BMSR_LSTATUS) ? "link ok" : "no link");

	printf("%s\n", buf);

	if (verbose > 1) {
		printf("  registers for MII PHY: ");
		for (i = 0; i < 32; i++)
			printf("%s %4.4x",
				((i % 8) ? "" : "\n   "), mii_val[i]);

		printf("\n");
	}

	if (verbose) {
		printf("  product info: ");
		for (i = 0; i < NMII; i++)
			if ((mii_id[i].id1 == (mii_val[2] & mii_id[i].mask1)) &&
				(mii_id[i].id2 ==
					(mii_val[3] & mii_id[i].mask2)))
				break;

		if (i < NMII)
			printf("%s rev %d\n", mii_id[i].name, mii_val[3]&0x0f);
		else
			printf("vendor %02x:%02x:%02x, model %d rev %d\n",
				mii_val[2] >> 10, (mii_val[2] >> 2) & 0xff,
				((mii_val[2] << 6)|(mii_val[3] >> 10)) & 0xff,
				(mii_val[3] >> 4) & 0x3f, mii_val[3] & 0x0f);

		printf("  basic mode:   ");
		if (bmcr & BMCR_RESET)
			printf("software reset, ");
		if (bmcr & BMCR_LOOPBACK)
			printf("loopback, ");
		if (bmcr & BMCR_ISOLATE)
			printf("isolate, ");
		if (bmcr & BMCR_CTST)
			printf("collision test, ");
		if (bmcr & BMCR_ANENABLE) {
			printf("autonegotiation enabled\n");
		} else {
			printf("%s Mbit, %s duplex\n",
			   (bmcr & BMCR_SPEED100) ? "100" : "10",
			   (bmcr & BMCR_FULLDPLX) ? "full" : "half");
		}
		printf("  basic status: ");
		if (bmsr & BMSR_ANEGCOMPLETE)
			printf("autonegotiation complete, ");
		else if (bmcr & BMCR_ANRESTART)
			printf("autonegotiation restarted, ");
		if (bmsr & BMSR_RFAULT)
			printf("remote fault, ");
		printf((bmsr & BMSR_LSTATUS) ? "link ok" : "no link");
		printf("\n  capabilities:%s", media_list(bmsr >> 6, 0));
		printf("\n  advertising: %s", media_list(advert, 0));

#define LPA_ABILITY_MASK	(LPA_10HALF | LPA_10FULL \
				| LPA_100HALF | LPA_100FULL \
				| LPA_100BASE4 | LPA_PAUSE_CAP)

		if (lkpar & LPA_ABILITY_MASK)
			printf("\n  link partner:%s", media_list(lkpar, 0));
		printf("\n");
	}

	return 0;
}

static int do_miitool(int argc, char *argv[])
{
	char *filename;
	int opt;
	int argc_min;
	int fd;
	int verbose;

	verbose = 0;
	while ((opt = getopt(argc, argv, "v")) > 0) {
		switch (opt) {
		case 'v':
			verbose++;
			break;
		default:
			return COMMAND_ERROR_USAGE;
		}
	}

	argc_min = optind + 1;

	if (argc < argc_min)
		return COMMAND_ERROR_USAGE;

	filename = argv[optind];

	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		printf("unable to read %s\n", filename);
		return COMMAND_ERROR;
	}

	show_basic_mii(fd, verbose);

	close(fd);

	return COMMAND_SUCCESS;
}

BAREBOX_CMD_HELP_START(miitool)
BAREBOX_CMD_HELP_USAGE("miitool [[[-v] -v] -v] <phy>\n")
BAREBOX_CMD_HELP_SHORT("view status for MII <phy>.\n")
BAREBOX_CMD_HELP_END

/**
 * @page miitool_command
This utility checks or sets the status of a network interface's
Media Independent Interface (MII) unit. Most fast ethernet
adapters use an MII to autonegotiate link speed and duplex setting.
 */
BAREBOX_CMD_START(miitool)
	.cmd		= do_miitool,
	.usage		= "view media-independent interface status",
	BAREBOX_CMD_HELP(cmd_miitool_help)
BAREBOX_CMD_END
