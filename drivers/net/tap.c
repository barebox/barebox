#include <common.h>
#include <driver.h>
#include <malloc.h>
#include <net.h>
#include <init.h>
#include <asm/arch/linux.h>

struct tap_priv {
	int fd;
	char name[20];
};

int tap_eth_send (struct eth_device *edev, void *packet, int length)
{
	struct tap_priv *priv = edev->priv;

	linux_write(priv->fd, packet, length);
	return 0;
}

int tap_eth_rx (struct eth_device *edev)
{
	struct tap_priv *priv = edev->priv;
	int length;

	length = linux_read_nonblock(priv->fd, NetRxPackets[0], PKTSIZE);

	if (length > 0)
		NetReceive(NetRxPackets[0], length);

	return 0;
}

int tap_eth_open(struct eth_device *edev)
{
	return 0;
}

void tap_eth_halt (struct eth_device *edev)
{
	/* nothing to do here */
}

static int tap_get_mac_address(struct eth_device *edev, unsigned char *adr)
{
	return -1;
}

static int tap_set_mac_address(struct eth_device *edev, unsigned char *adr)
{
	return 0;
}

int tap_probe(struct device_d *dev)
{
	struct eth_device *edev;
	struct tap_priv *priv;
	int ret = 0;

	priv = malloc(sizeof(struct tap_priv));

	priv->fd = tap_alloc(priv->name);
	if (priv->fd < 0) {
		ret = priv->fd;
		goto out;
	}

	edev = malloc(sizeof(struct eth_device));
	dev->type_data = edev;
	edev->dev = dev;
	edev->priv = priv;

	edev->init = tap_eth_open;
	edev->open = tap_eth_open;
	edev->send = tap_eth_send;
	edev->recv = tap_eth_rx;
	edev->halt = tap_eth_halt;
	edev->get_mac_address = tap_get_mac_address;
	edev->set_mac_address = tap_set_mac_address;

	eth_register(edev);

        return 0;
out:
	free(priv);
	return ret;
}

static struct driver_d tap_driver = {
        .name  = "tap",
        .probe = tap_probe,
        .type  = DEVICE_TYPE_ETHER,
};

static int tap_init(void)
{
        register_driver(&tap_driver);
        return 0;
}

device_initcall(tap_init);
