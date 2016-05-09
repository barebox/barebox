/*
 * (C) Copyright 2007 Pengutronix, Sascha Hauer <s.hauer@pengutronix.de>
 * (C) Copyright 2007 Pengutronix, Juergen Beisert <j.beisert@pengutronix.de>
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

/**
 * @file
 * @brief Shared structures and constants between i.MX27's and MPC52xx's FEC
 */
#ifndef __INCLUDE_NETWORK_FEC_H
#define __INCLUDE_NETWORK_FEC_H

#include <linux/phy.h>

/*
 * Define the phy connected externally for FEC drivers
 * (like MPC52xx and i.MX27)
 */
struct fec_platform_data {
	phy_interface_t	xcv_type;
	int		phy_addr;
	void 		(*phy_init)(struct phy_device *dev);
};

#endif /* __INCLUDE_NETWORK_FEC_H */

