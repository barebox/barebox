/*
 * Copyright (c) 2009 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
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
 *
 */

#include <common.h>
#include <linux/phy.h>
#include <init.h>

static struct phy_driver generic_phy = {
	.drv.name = "Generic PHY",
	.phy_id = PHY_ANY_UID,
	.phy_id_mask = PHY_ANY_UID,
	.features = 0,
};

static int generic_phy_register(void)
{
	return phy_driver_register(&generic_phy);
}
device_initcall(generic_phy_register);
