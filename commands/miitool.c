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
#include <net.h>
#include <linux/mii.h>
#include <linux/phy.h>
#include <linux/err.h>

const struct {
	char	*name;
	u_short	value[2];
} media[] = {
	/* The order through 100baseT4 matches bits in the BMSR */
	{ "10baseT-HD",	{ADVERTISE_10HALF} },
	{ "10baseT-FD",	{ADVERTISE_10FULL} },
	{ "100baseTx-HD",	{ADVERTISE_100HALF} },
	{ "100baseTx-FD",	{ADVERTISE_100FULL} },
	{ "100baseT4",	{LPA_100BASE4} },
	{ "100baseTx",	{ADVERTISE_100FULL | ADVERTISE_100HALF} },
	{ "10baseT",	{ADVERTISE_10FULL | ADVERTISE_10HALF} },
	{ "1000baseT-HD",	{0, ADVERTISE_1000HALF} },
	{ "1000baseT-FD",	{0, ADVERTISE_1000FULL} },
	{ "1000baseT",	{0, ADVERTISE_1000HALF | ADVERTISE_1000FULL} },
};
#define NMEDIA (sizeof(media)/sizeof(media[0]))

static const char *media_list(unsigned mask, unsigned mask2, int best)
{
	static char buf[256];
	int i;

	*buf = '\0';

	if (mask2) {
		if (mask2 & ADVERTISE_1000FULL) {
			strcat(buf, " ");
			strcat(buf, "1000baseT-FD");
			if (best)
				goto out;
		}

		if (mask2 & ADVERTISE_1000HALF) {
			strcat(buf, " ");
			strcat(buf, "1000baseT-HD");
			if (best)
				goto out;
		}
	}

	mask >>= 5;
	for (i = 4; i >= 0; i--) {
		if (mask & (1 << i)) {
			strcat(buf, " ");
			strcat(buf, media[i].name);
			if (best)
				break;
		}
	}

out:
	if (mask & (1 << 5))
		strcat(buf, " flow-control");

	return buf;
}

static int show_basic_mii(struct mii_bus *mii, struct phy_device *phydev,
	int verbose)
{
	char buf[100];
	int i, mii_val[32];
	unsigned bmcr, bmsr, advert, lkpar, bmcr2 = 0, lpa2 = 0;
	struct phy_driver *phydrv;
	int is_phy_gbit;

	/* Some bits in the BMSR are latched, but we can't rely on being
	   the only reader, so only the current values are meaningful */
	mii->read(mii, phydev->addr, MII_BMSR);

	for (i = 0; i < 32; i++)
		mii_val[i] = mii->read(mii, phydev->addr, i);

	printf((mii->parent->id) < 0 ? "%s: %s: " : "%s: %s%d: ",
	       phydev->cdev.name, mii->parent->name, mii->parent->id);


	if (mii_val[MII_BMCR] == 0xffff || mii_val[MII_BMSR] == 0x0000) {
		eprintf("  No MII transceiver present!.\n");
		return -1;
	}

	/* Descriptive rename. */
	bmcr = mii_val[MII_BMCR];
	bmsr = mii_val[MII_BMSR];
	advert = mii_val[MII_ADVERTISE];
	lkpar = mii_val[MII_LPA];

	phydrv = to_phy_driver(phydev->dev.driver);
	is_phy_gbit = phydrv->features &
		(SUPPORTED_1000baseT_Half | SUPPORTED_1000baseT_Full);

	if (is_phy_gbit) {
		bmcr2 = mii_val[MII_CTRL1000];
		lpa2 = mii_val[MII_STAT1000];
	}

	*buf = '\0';
	if (bmcr & BMCR_ANENABLE) {
		if (bmsr & BMSR_ANEGCOMPLETE) {
			if (advert & lkpar) {
				sprintf(buf, "%s%s, ", (lkpar & LPA_LPACK) ?
					"negotiated" : "no autonegotiation,",
					media_list(advert & lkpar,
						bmcr2 & lpa2 >> 2, 1));
			} else {
				sprintf(buf, "autonegotiation failed, ");
			}
		} else if (bmcr & BMCR_ANRESTART) {
			sprintf(buf, "autonegotiation restarted, ");
		}
	} else {
		int speed1000;

		speed1000 = ((bmcr2
				& (ADVERTISE_1000HALF
				| ADVERTISE_1000FULL)) & lpa2 >> 2);
		sprintf(buf, "%s Mbit, %s duplex, ",
			speed1000 ? "1000"
			: (bmcr & BMCR_SPEED100) ? "100" : "10",
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
		printf("  product info: %s ", phydrv->drv.name);
		printf("(vendor %02x:%02x:%02x, model %d rev %d)\n",
			mii_val[2] >> 10, (mii_val[2] >> 2) & 0xff,
			((mii_val[2] << 6) | (mii_val[3] >> 10)) & 0xff,
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
		printf("\n  capabilities:%s", media_list(bmsr >> 6, bmcr2, 0));
		printf("\n  advertising: %s", media_list(advert, bmcr2, 0));

#define LPA_ABILITY_MASK	(LPA_10HALF | LPA_10FULL \
				| LPA_100HALF | LPA_100FULL \
				| LPA_100BASE4 | LPA_PAUSE_CAP)

		if (lkpar & LPA_ABILITY_MASK)
			printf("\n  link partner:%s",
				media_list(lkpar, lpa2 >> 2, 0));
		printf("\n");
	}

	return 0;
}

static void mdiobus_show(struct device_d *dev, const char *phydevname,
			 int verbose)
{
	struct mii_bus *mii = to_mii_bus(dev);
	int i;

	for (i = 0; i < PHY_MAX_ADDR; i++) {
		struct phy_device *phydev;

		phydev = mdiobus_scan(mii, i);
		if (IS_ERR(phydev) || !phydev->registered)
			continue;

		/*
		 * If we are looking for a secific phy, called
		 * 'phydevname', but current phydev is not it, skip to
		 * the next iteration
		 */
		if (phydevname &&
		    strcmp(phydev->cdev.name, phydevname))
			continue;

		show_basic_mii(mii, phydev, verbose);

		/*
		 * We were looking for a specific device and at this
		 * point we already shown the info about it so end the
		 * loop and exit
		 */
		if (phydevname)
			break;
	}

	return;
}

enum miitool_operations {
	MIITOOL_NOOP,
	MIITOOL_SHOW,
	MIITOOL_REGISTER,
};

static int do_miitool(int argc, char *argv[])
{
	char *phydevname = NULL;
	char *regstr = NULL;
	char *endp;
	struct mii_bus *mii;
	int opt, ret;
	int verbose = 0;
	struct phy_device *phydev;
	enum miitool_operations action = MIITOOL_NOOP;
	int addr, bus;

	while ((opt = getopt(argc, argv, "vs:r:")) > 0) {
		switch (opt) {
		case 'a':
			addr = simple_strtol(optarg, NULL, 0);
			break;
		case 'b':
			bus = simple_strtoul(optarg, NULL, 0);
			break;
		case 's':
			action = MIITOOL_SHOW;
			phydevname = xstrdup(optarg);
			break;
		case 'r':
			action = MIITOOL_REGISTER;
			regstr = optarg;
			break;
		case 'v':
			verbose++;
			break;
		default:
			ret = COMMAND_ERROR_USAGE;
			goto free_phydevname;
		}
	}

	switch (action) {
	case MIITOOL_REGISTER:
		bus = simple_strtoul(regstr, &endp, 0);
		if (*endp != ':') {
			printf("No colon between bus and address\n");
			return COMMAND_ERROR_USAGE;
		}
		endp++;
		addr = simple_strtoul(endp, NULL, 0);

		if (addr >= PHY_MAX_ADDR)
			printf("Address out of range (max %d)\n", PHY_MAX_ADDR - 1);

		mii = mdiobus_get_bus(bus);
		if (!mii) {
			printf("Can't find MDIO bus #%d\n", bus);
			ret = COMMAND_ERROR;
			goto free_phydevname;
		}

		phydev = phy_device_create(mii, addr, -1);
		ret = phy_register_device(phydev);
		if (ret) {
			printf("failed to register phy %s: %s\n",
				dev_name(&phydev->dev), strerror(-ret));
			goto free_phydevname;
		} else {
			printf("registered phy %s\n", dev_name(&phydev->dev));
		}
		break;
	default:
	case MIITOOL_SHOW:
		for_each_mii_bus(mii) {
			mdiobus_detect(&mii->dev);
			mdiobus_show(&mii->dev,
				     devpath_to_name(phydevname),
				     verbose);
		}
		break;
	}

	ret = COMMAND_SUCCESS;

free_phydevname:
	free(phydevname);
	return ret;
}

BAREBOX_CMD_HELP_START(miitool)
BAREBOX_CMD_HELP_TEXT("This utility checks or sets the status of a network interface's")
BAREBOX_CMD_HELP_TEXT("Media Independent Interface (MII) unit as well as allowing to")
BAREBOX_CMD_HELP_TEXT("register dummy PHY devices for raw MDIO access. Most fast ethernet")
BAREBOX_CMD_HELP_TEXT("adapters use an MII to autonegotiate link speed and duplex setting.")
BAREBOX_CMD_HELP_TEXT("")
BAREBOX_CMD_HELP_TEXT("Options:")
BAREBOX_CMD_HELP_OPT("-v", "increase verbosity")
BAREBOX_CMD_HELP_OPT("-s <devpath/devname>", "show PHY status (not providing PHY prints status of all)")
BAREBOX_CMD_HELP_OPT("-r <busno>:<adr>", "register a PHY")
BAREBOX_CMD_HELP_END

BAREBOX_CMD_START(miitool)
	.cmd		= do_miitool,
	BAREBOX_CMD_DESC("view media-independent interface status")
	BAREBOX_CMD_OPTS("[-vsr]")
	BAREBOX_CMD_GROUP(CMD_GRP_NET)
	BAREBOX_CMD_HELP(cmd_miitool_help)
BAREBOX_CMD_END
