/*
 * miiphy.c - generic phy abstraction
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
#include <miiphy.h>
#include <clock.h>
#include <net.h>
#include <malloc.h>

int miiphy_restart_aneg(struct miiphy_device *mdev)
{
	uint16_t status;
	int timeout;

	/*
	 * Reset PHY, then delay 300ns
	 */
	mdev->write(mdev, mdev->address, MII_BMCR, BMCR_RESET);

	if (mdev->flags & MIIPHY_FORCE_LINK)
		return 0;

	udelay(1000);

	if (mdev->flags & MIIPHY_FORCE_10) {
		printf("Forcing 10 Mbps ethernet link... ");
		mdev->read(mdev, mdev->address, MII_BMSR, &status);
		mdev->write(mdev, mdev->address, MII_BMCR, BMCR_FULLDPLX | BMCR_CTST);

		timeout = 20;
		do {	/* wait for link status to go down */
			udelay(10000);
			if ((timeout--) == 0) {
				debug("hmmm, should not have waited...");
				break;
			}
			mdev->read(mdev, mdev->address, MII_BMSR, &status);
		} while (status & BMSR_LSTATUS);

	} else {	/* MII100 */
		/*
		 * Set the auto-negotiation advertisement register bits
		 */
		mdev->read(mdev, mdev->address, MII_ADVERTISE, &status);
		status |= ADVERTISE_ALL;
		mdev->write(mdev, mdev->address, MII_ADVERTISE, status);

		mdev->write(mdev, mdev->address, MII_BMCR, BMCR_ANENABLE | BMCR_ANRESTART);
	}

	return 0;
}

int miiphy_wait_aneg(struct miiphy_device *mdev)
{
	uint64_t start;
	uint16_t status;

	if (mdev->flags & MIIPHY_FORCE_LINK)
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

		if (mdev->read(mdev, mdev->address, MII_BMSR, &status)) {
			printf("%s: Autonegotiation failed. status: 0x%04x\n", mdev->cdev.name, status);
			return -1;
		}
	} while (!(status & BMSR_LSTATUS));

	return 0;
}

int miiphy_print_status(struct miiphy_device *mdev)
{
	uint16_t bmsr, bmcr, lpa;
	char *duplex;
	int speed;

	if (mdev->flags & MIIPHY_FORCE_LINK) {
		printf("Forcing link present...\n");
		return 0;
	}

	if (mdev->read(mdev, mdev->address, MII_BMSR, &bmsr) != 0)
		goto err_out;
	if (mdev->read(mdev, mdev->address, MII_BMCR, &bmcr) != 0)
		goto err_out;
	if (mdev->read(mdev, mdev->address, MII_LPA, &lpa) != 0)
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

static ssize_t miiphy_read(struct cdev *cdev, void *_buf, size_t count, ulong offset, ulong flags)
{
	int i = count;
	uint16_t *buf = _buf;
	struct miiphy_device *mdev = cdev->priv;

	while (i > 1) {
		mdev->read(mdev, mdev->address, offset, buf);
		buf++;
		i -= 2;
		offset++;
	}

	return count;
}

static ssize_t miiphy_write(struct cdev *cdev, const void *_buf, size_t count, ulong offset, ulong flags)
{
	int i = count;
	const uint16_t *buf = _buf;
	struct miiphy_device *mdev = cdev->priv;

	while (i > 1) {
		mdev->write(mdev, mdev->address, offset, *buf);
		buf++;
		i -= 2;
		offset++;
	}

	return count;
}

static struct file_operations miiphy_ops = {
	.read  = miiphy_read,
	.write = miiphy_write,
	.lseek = dev_lseek_default,
};

static int miiphy_probe(struct device_d *dev)
{
	struct miiphy_device *mdev = dev->priv;

	mdev->cdev.name = asprintf("phy%d", dev->id);
	mdev->cdev.size = 32;
	mdev->cdev.ops = &miiphy_ops;
	mdev->cdev.priv = mdev;
	mdev->cdev.dev = dev;
	devfs_create(&mdev->cdev);
	return 0;
}

static void miiphy_remove(struct device_d *dev)
{
	struct miiphy_device *mdev = dev->priv;

	free(mdev->cdev.name);
	devfs_remove(&mdev->cdev);
}

static struct driver_d miiphy_drv = {
        .name  = "miiphy",
        .probe = miiphy_probe,
	.remove = miiphy_remove,
};

int miiphy_register(struct miiphy_device *mdev)
{
	mdev->dev.priv = mdev;
	strcpy(mdev->dev.name, "miiphy");

	return register_device(&mdev->dev);
}

void miiphy_unregister(struct miiphy_device *mdev)
{
	unregister_device(&mdev->dev);
}

static int miiphy_init(void)
{
	register_driver(&miiphy_drv);
	return 0;
}

device_initcall(miiphy_init);

