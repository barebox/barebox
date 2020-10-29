// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2015 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
// SPDX-FileCopyrightText: 1994-2000 Neil Russell
// SPDX-FileCopyrightText: 2000 Roland Borde
// SPDX-FileCopyrightText: 2000 Paolo Scaffardi
// SPDX-FileCopyrightText: 2000-2002 Wolfgang Denk <wd@denx.de>

/*
 * net.c - barebox networking support
 *
 * based on U-Boot (LiMon) code
 */

#include <common.h>
#include <net.h>
#include <linux/ctype.h>

int string_to_ethaddr(const char *str, u8 enetaddr[ETH_ALEN])
{
	int reg;
	char *e;

	if (!str || strlen(str) != 17) {
		memset(enetaddr, 0, ETH_ALEN);
		return -EINVAL;
	}

	if (str[2] != ':' || str[5] != ':' || str[8] != ':' ||
			str[11] != ':' || str[14] != ':')
		return -EINVAL;

	for (reg = 0; reg < ETH_ALEN; ++reg) {
		enetaddr[reg] = simple_strtoul(str, &e, 16);
		str = e + 1;
	}

	return 0;
}

void ethaddr_to_string(const u8 enetaddr[ETH_ALEN], char *str)
{
	sprintf(str, "%02x:%02x:%02x:%02x:%02x:%02x",
		 enetaddr[0], enetaddr[1], enetaddr[2], enetaddr[3],
		 enetaddr[4], enetaddr[5]);
}

int string_to_ip(const char *s, IPaddr_t *ip)
{
	IPaddr_t addr = 0;
	char *e;
	int i;

	if (!s)
		return -EINVAL;

	for (i = 0; i < 4; i++) {
		unsigned long val;

		if (!isdigit(*s))
			return -EINVAL;

		val = simple_strtoul(s, &e, 10);
		if (val > 255)
			return -EINVAL;

		addr = (addr << 8) | val;

		if (*e != '.' && i != 3)
			return -EINVAL;

		s = e + 1;
	}

	*ip = htonl(addr);

	return 0;
}
