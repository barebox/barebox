/*
 * [origin Linux: arch/arm/mach-at91/include/mach/board.h]
 *
 *  Copyright (C) 2005 HP Labs
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
 *
 */
#ifndef __MACB_H
#define __MACB_H

#include <linux/phy.h>

struct macb_platform_data {
	unsigned int phy_flags;
	unsigned int flags;
	int phy_addr;
	phy_interface_t phy_interface;
	int (*get_ethaddr)(struct eth_device*, unsigned char *adr);
};

#endif
