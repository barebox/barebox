/*
 * tap.c - host side functions for tap driver
 *
 * Copyright (c) 2007 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
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

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <string.h>

#ifdef __has_include
#  if !__has_include(<linux/if_tun.h>)
#    define IFF_TAP		0x0002
#    define IFF_NO_PI	0x1000
#    define TUNSETIFF	_IOW('T', 202, int)
#  endif
#endif
#ifndef IFF_TAP
#  include <linux/if_tun.h>
#endif

int tap_alloc(const char *dev)
{
	struct ifreq ifr;
	int fd, err;

	if ((fd = open("/dev/net/tun", O_RDWR | O_CLOEXEC)) < 0) {
		perror("could not open /dev/net/tun");
		return -1;
	}

	memset(&ifr, 0, sizeof(ifr));

	/* Flags: IFF_TUN   - TUN device (no Ethernet headers)
	 *        IFF_TAP   - TAP device
	 *
	 *        IFF_NO_PI - Do not provide packet information
	 */
	ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
	if (*dev)
		strncpy(ifr.ifr_name, dev, IFNAMSIZ);

	if ((err = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0) {
		perror("could not get tap device");
		close(fd);
		return err;
	}

	return fd;
}

/**
 * @file
 * @brief Host side functions for tap driver
 */
