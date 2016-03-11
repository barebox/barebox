/*
 * (C) Copyright 2016 Alexander Kurz <akurz@blala.de>
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

#ifndef __NET_EP93XX_ETH_H
#define __NET_EP93XX_ETH_H

#include <linux/phy.h>

struct ep93xx_eth_platform_data {
	phy_interface_t	xcv_type;
	int		phy_addr;
};

#endif /* __NET_EP93XX_ETH_H */
