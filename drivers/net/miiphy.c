#include <common.h>
#include <driver.h>
#include <init.h>
#include <mii_phy.h>

int miiphy_restart_aneg(struct miiphy_device *mdev)
{
	uint16_t status;
	int timeout;

	/*
	 * Reset PHY, then delay 300ns
	 */
	mdev->write(mdev, mdev->address, PHY_BMCR, PHY_BMCR_RESET);
	udelay(1000);

	if (mdev->flags & MIIPHY_FORCE_10) {
		printf("Forcing 10 Mbps ethernet link... ");
		mdev->read(mdev, mdev->address, PHY_BMSR, &status);
		mdev->write(mdev, mdev->address, PHY_BMCR, PHY_BMCR_DPLX | PHY_BMCR_COL_TST);

		timeout = 20;
		do {	/* wait for link status to go down */
			udelay(10000);
			if ((timeout--) == 0) {
#if (DEBUG & 0x2)
				printf("hmmm, should not have waited...");
#endif
				break;
			}
			mdev->read(mdev, mdev->address, PHY_BMSR, &status);
		} while (status & PHY_BMSR_LS);

	} else {	/* MII100 */
		/*
		 * Set the auto-negotiation advertisement register bits
		 */
		mdev->write(mdev, mdev->address, PHY_ANAR, 0x01e1);

		mdev->write(mdev, mdev->address, PHY_BMCR, PHY_BMCR_AUTON | PHY_BMCR_RST_NEG);
	}

	return 0;
}

int miiphy_wait_aneg(struct miiphy_device *mdev)
{
	int timeout = 1;
	uint16_t status;

	/*
	 * Wait for AN completion
	 */
	timeout = 5000;
	do {
		udelay(1000);

		if (!timeout--) {
			printf("%s: Autonegotiation timeout\n", mdev->dev.id);
			return -1;
		}

		if (mdev->read(mdev, mdev->address, PHY_BMSR, &status)) {
			printf("%s: Autonegotiation failed. status: 0x%04x\n", mdev->dev.id, status);
			return -1;
		}
	} while (!(status & PHY_BMSR_LS));

	return 0;
}

int miiphy_print_status(struct miiphy_device *mdev)
{
	uint16_t bmsr, bmcr, anlpar;
	char *duplex;
	int speed;

	if (mdev->read(mdev, mdev->address, PHY_BMSR, &bmsr) != 0)
		goto err_out;
	if (mdev->read(mdev, mdev->address, PHY_BMCR, &bmcr) != 0)
		goto err_out;
	if (mdev->read(mdev, mdev->address, PHY_ANLPAR, &anlpar) != 0)
		goto err_out;

	printf("%s: Link is %s", mdev->dev.id,
			bmsr & PHY_BMSR_LS ? "up" : "down");

	if (bmcr & PHY_BMCR_AUTON) {
		duplex = (PHY_ANLPAR_10FD | PHY_ANLPAR_TXFD) ? "Full" : "Half";
		speed = anlpar & PHY_ANLPAR_100 ? 100 : 10;
	} else {
		duplex = bmcr & PHY_BMCR_DPLX ? "Full" : "Half";
		speed = bmcr & PHY_BMCR_100MB ? 100 : 10;
	}

	printf(" - %d/%s\n", speed, duplex);

	return 0;
err_out:
	printf("%s: failed to read\n", mdev->dev.id);
	return -1;
}

ssize_t miiphy_read(struct device_d *dev, void *_buf, size_t count, ulong offset, ulong flags)
{
	int i = count;
	uint16_t *buf = _buf;
	struct miiphy_device *mdev = dev->priv;

	while (i > 1) {
		mdev->read(mdev, mdev->address, offset, buf);
		buf++;
		i -= 2;
		offset++;
	}

	return count;
}

ssize_t miiphy_write(struct device_d *dev, const void *_buf, size_t count, ulong offset, ulong flags)
{
	int i = count;
	const uint16_t *buf = _buf;
	struct miiphy_device *mdev = dev->priv;

	while (i > 1) {
		mdev->write(mdev, mdev->address, offset, *buf);
		buf++;
		i -= 2;
		offset++;
	}

	return count;
}

static int miiphy_probe(struct device_d *dev)
{
	return 0;
}

int miiphy_register(struct miiphy_device *mdev)
{
	strcpy(mdev->dev.name, "miiphy");
	get_free_deviceid(mdev->dev.id, "phy");
	mdev->dev.type = DEVICE_TYPE_MIIPHY;
	mdev->dev.priv = mdev;
	mdev->dev.size = 32;

	return register_device(&mdev->dev);
}

static struct driver_d miiphy_drv = {
        .name  = "miiphy",
        .probe = miiphy_probe,
	.read  = miiphy_read,
	.write = miiphy_write,
	.type  = DEVICE_TYPE_MIIPHY,
};

static int miiphy_init(void)
{
	register_driver(&miiphy_drv);
	return 0;
}

device_initcall(miiphy_init);

