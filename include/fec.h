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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/**
 * @file
 * @brief Shared structures and constants between i.MX27's and MPC52xx's FEC
 */
#ifndef __INCLUDE_NETWORK_FEC_H
#define __INCLUDE_NETWORK_FEC_H

/*
 * Supported phy types on this platform
 */
typedef enum {
	SEVENWIRE,
	MII10,
	MII100,
	RMII,
} xceiver_type;

/*
 * Define the phy connected externally for FEC drivers
 * (like MPC52xx and i.MX27)
 */
struct fec_platform_data {
        xceiver_type	xcv_type;
	int		phy_addr;
};

#endif /* __INCLUDE_NETWORK_FEC_H */

