/*
 * (C) Copyright 2001
 * Gerald Van Baren, Custom IDEAS, vanbaren@cideas.com.
 *
 * (C) Copyright 2009 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#ifndef __PHYLIB_H__
#define __PHYLIB_H__

#include <driver.h>
#include <linux/mii.h>

struct mii_bus {
	struct device_d dev;
	struct device_d *parent;
	void *priv;

	int	(*read) (struct mii_bus *dev, int addr, int reg);
	int	(*write) (struct mii_bus *dev, int addr, int reg, int value);
};

int mdiobus_register(struct mii_bus *dev);
void mdiobus_unregister(struct mii_bus *bus);


#endif /* __PHYLIB_H__ */
