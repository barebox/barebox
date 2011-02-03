/*
 * tap.c - A tap ethernet driver for barebox
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
#include <malloc.h>
#include <net.h>
#include <init.h>
#include <mach/linux.h>

struct tap_priv {
	int fd;
	char *name;
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
		net_receive(NetRxPackets[0], length);

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

static int tap_get_ethaddr(struct eth_device *edev, unsigned char *adr)
{
	return -1;
}

static int tap_set_ethaddr(struct eth_device *edev, unsigned char *adr)
{
	return 0;
}

int tap_probe(struct device_d *dev)
{
	struct eth_device *edev;
	struct tap_priv *priv;
	int ret = 0;

	priv = xmalloc(sizeof(struct tap_priv));
	priv->name = "barebox";

	priv->fd = tap_alloc(priv->name);
	if (priv->fd < 0) {
		ret = priv->fd;
		goto out;
	}

	edev = xmalloc(sizeof(struct eth_device));
	dev->type_data = edev;
	edev->priv = priv;

	edev->init = tap_eth_open;
	edev->open = tap_eth_open;
	edev->send = tap_eth_send;
	edev->recv = tap_eth_rx;
	edev->halt = tap_eth_halt;
	edev->get_ethaddr = tap_get_ethaddr;
	edev->set_ethaddr = tap_set_ethaddr;

	eth_register(edev);

        return 0;
out:
	free(priv);
	return ret;
}

static struct driver_d tap_driver = {
        .name  = "tap",
        .probe = tap_probe,
};

static int tap_init(void)
{
        register_driver(&tap_driver);
        return 0;
}

device_initcall(tap_init);
