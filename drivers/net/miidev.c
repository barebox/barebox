/*
 * miidev.c - generic phy abstraction
 *
 * Copyright (c) 2007 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * See file CREDITS for list of people who contributed to this
 * project.
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <common.h>
#include <driver.h>
#include <init.h>
#include <miidev.h>
#include <clock.h>
#include <net.h>
#include <malloc.h>

int miidev_restart_aneg(struct mii_device *mdev)
{
	uint16_t status;
	int timeout;

	/*
	 * Reset PHY, then delay 300ns
	 */
	mii_write(mdev, mdev->address, MII_BMCR, BMCR_RESET);

	if (mdev->flags & MIIDEV_FORCE_LINK)
		return 0;

	udelay(1000);

	if (mdev->flags & MIIDEV_FORCE_10) {
		printf("Forcing 10 Mbps ethernet link... ");
		status = mii_read(mdev, mdev->address, MII_BMSR);
		mii_write(mdev, mdev->address, MII_BMCR, BMCR_FULLDPLX | BMCR_CTST);

		timeout = 20;
		do {	/* wait for link status to go down */
			udelay(10000);
			if ((timeout--) == 0) {
				debug("hmmm, should not have waited...");
				break;
			}
			status = mii_read(mdev, mdev->address, MII_BMSR);
		} while (status & BMSR_LSTATUS);

	} else {	/* MII100 */
		/*
		 * Set the auto-negotiation advertisement register bits
		 */
		status = mii_read(mdev, mdev->address, MII_ADVERTISE);
		status |= ADVERTISE_ALL;
		mii_write(mdev, mdev->address, MII_ADVERTISE, status);

		mii_write(mdev, mdev->address, MII_BMCR, BMCR_ANENABLE | BMCR_ANRESTART);
	}

	return 0;
}

int miidev_wait_aneg(struct mii_device *mdev)
{
	uint64_t start;
	int status;

	if (mdev->flags & MIIDEV_FORCE_LINK)
		return 0;

	/*
	 * Wait for AN completion
	 */
	start = get_time_ns();
	do {
		if (is_timeout(start, 5 * SECOND)) {
			printf("%s: Autonegotiation timeout\n", mdev->cdev.name);
			return -1;
		}

		status = mii_read(mdev, mdev->address, MII_BMSR);
		if (status < 0) {
			printf("%s: Autonegotiation failed. status: 0x%04x\n", mdev->cdev.name, status);
			return -1;
		}
	} while (!(status & BMSR_LSTATUS));

	return 0;
}

int miidev_print_status(struct mii_device *mdev)
{
	int bmsr, bmcr, lpa;
	char *duplex;
	int speed;

	if (mdev->flags & MIIDEV_FORCE_LINK) {
		printf("Forcing link present...\n");
		return 0;
	}

	bmsr = mii_read(mdev, mdev->address, MII_BMSR);
	if (bmsr < 0)
		goto err_out;
	bmcr = mii_read(mdev, mdev->address, MII_BMCR);
	if (bmcr < 0)
		goto err_out;
	lpa = mii_read(mdev, mdev->address, MII_LPA);
	if (lpa < 0)
		goto err_out;

	printf("%s: Link is %s", mdev->cdev.name,
			bmsr & BMSR_LSTATUS ? "up" : "down");

	if (bmcr & BMCR_ANENABLE) {
		duplex = lpa & LPA_DUPLEX ? "Full" : "Half";
		speed = lpa & LPA_100 ? 100 : 10;
	} else {
		duplex = bmcr & BMCR_FULLDPLX ? "Full" : "Half";
		speed = bmcr & BMCR_SPEED100 ? 100 : 10;
	}

	printf(" - %d/%s\n", speed, duplex);

	return 0;
err_out:
	printf("%s: failed to read\n", mdev->cdev.name);
	return -1;
}

static ssize_t miidev_read(struct cdev *cdev, void *_buf, size_t count, ulong offset, ulong flags)
{
	int i = count;
	uint16_t *buf = _buf;
	struct mii_device *mdev = cdev->priv;

	while (i > 1) {
		*buf = mii_read(mdev, mdev->address, offset);
		buf++;
		i -= 2;
		offset++;
	}

	return count;
}

static ssize_t miidev_write(struct cdev *cdev, const void *_buf, size_t count, ulong offset, ulong flags)
{
	int i = count;
	const uint16_t *buf = _buf;
	struct mii_device *mdev = cdev->priv;

	while (i > 1) {
		mii_write(mdev, mdev->address, offset, *buf);
		buf++;
		i -= 2;
		offset++;
	}

	return count;
}

static struct file_operations miidev_ops = {
	.read  = miidev_read,
	.write = miidev_write,
	.lseek = dev_lseek_default,
};

static int miidev_probe(struct device_d *dev)
{
	struct mii_device *mdev = dev->priv;

	mdev->cdev.name = asprintf("phy%d", dev->id);
	mdev->cdev.size = 64;
	mdev->cdev.ops = &miidev_ops;
	mdev->cdev.priv = mdev;
	mdev->cdev.dev = dev;
	devfs_create(&mdev->cdev);
	return 0;
}

static void miidev_remove(struct device_d *dev)
{
	struct mii_device *mdev = dev->priv;

	free(mdev->cdev.name);
	devfs_remove(&mdev->cdev);
}

static struct driver_d miidev_drv = {
        .name  = "miidev",
        .probe = miidev_probe,
	.remove = miidev_remove,
};

int mii_register(struct mii_device *mdev)
{
	mdev->dev.priv = mdev;
	mdev->dev.id = -1;
	strcpy(mdev->dev.name, "miidev");

	return register_device(&mdev->dev);
}

void mii_unregister(struct mii_device *mdev)
{
	unregister_device(&mdev->dev);
}

static int miidev_init(void)
{
	register_driver(&miidev_drv);
	return 0;
}

device_initcall(miidev_init);

