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
#ifndef __MIIDEV_H__
#define __MIIDEV_H__

#include <driver.h>
#include <linux/mii.h>

#define MIIDEV_FORCE_10		(1 << 0)
#define MIIDEV_FORCE_LINK	(1 << 1)

#define MIIDEV_CAPABLE_1000M	(1 << 0)

struct mii_device {
	struct device_d dev;
	struct device_d *parent;

	int address;	/* The address the phy has on the bus */
	int	(*read) (struct mii_device *dev, int addr, int reg);
	int	(*write) (struct mii_device *dev, int addr, int reg, int value);

	int flags;
	int capabilities;

	struct eth_device *edev;
	struct cdev cdev;
	struct list_head list;
};

int mii_register(struct mii_device *dev);
void mii_unregister(struct mii_device *mdev);
int miidev_restart_aneg(struct mii_device *mdev);
int miidev_wait_aneg(struct mii_device *mdev);
int miidev_get_status(struct mii_device *mdev);
#define MIIDEV_STATUS_IS_UP		(1 << 0)
#define MIIDEV_STATUS_IS_FULL_DUPLEX	(1 << 1)
#define MIIDEV_STATUS_IS_10MBIT		(1 << 2)
#define MIIDEV_STATUS_IS_100MBIT	(1 << 3)
#define MIIDEV_STATUS_IS_1000MBIT	(1 << 4)
int miidev_print_status(struct mii_device *mdev);

static int inline mii_write(struct mii_device *dev, int addr, int reg, int value)
{
	return dev->write(dev, addr, reg, value);
}

static int inline mii_read(struct mii_device *dev, int addr, int reg)
{
	return dev->read(dev, addr, reg);
}

struct mii_device *mii_open(const char *name);
void mii_close(struct mii_device *mdev);

#endif /* __MIIDEV_H__ */
