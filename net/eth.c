/*
 * (C) Copyright 2001-2004
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <command.h>
#include <driver.h>
#include <init.h>
#include <net.h>
#include <miiphy.h>
#include <malloc.h>
#include <errno.h>

static struct eth_device *eth_current;

void eth_set_current(struct eth_device *eth)
{
	eth_current = eth;
}

struct eth_device * eth_get_current(void)
{
	return eth_current;
}

int eth_init(void)
{
	char mac[6];

	if (!eth_current)
		return 0;

	string_to_enet_addr(dev_get_param(eth_current->dev, "mac"), mac);

	eth_current->set_mac_address(eth_current, mac);

	eth_current->open(eth_current);

	return 1;
}

void eth_halt(void)
{
	if (!eth_current)
		return;

	eth_current->halt(eth_current);

	eth_current->state = ETH_STATE_PASSIVE;
}

int eth_send(void *packet, int length)
{
	if (!eth_current)
		return -1;

	return eth_current->send(eth_current, packet, length);
}

int eth_rx(void)
{
	if (!eth_current)
		return -1;

	return eth_current->recv(eth_current);
}

int eth_register(struct eth_device *edev)
{
        struct device_d *dev = edev->dev;
	unsigned char ethaddr_str[20];
	unsigned char ethaddr[6];

        printf("%s\n",__FUNCTION__);

	if (!edev->get_mac_address) {
		printf("no get_mac_address found for current eth device\n");
		return -1;
	}

	edev->param_ip.name = "ip";
	edev->param_mac.name = "mac";
	edev->param_gateway.name = "gateway";
	edev->param_netmask.name = "netmask";
	edev->param_serverip.name = "serverip";
	dev_add_param(dev, &edev->param_ip);
	dev_add_param(dev, &edev->param_mac);
	dev_add_param(dev, &edev->param_gateway);
	dev_add_param(dev, &edev->param_netmask);
	dev_add_param(dev, &edev->param_serverip);

	if (edev->get_mac_address(edev, ethaddr) == 0) {
		enet_addr_to_string(ethaddr, ethaddr_str);
		printf("got MAC address from EEPROM: %s\n",ethaddr_str);
		dev_set_param(dev, "mac", ethaddr_str);
//		memcpy(edev->enetaddr, ethaddr, 6);
	}

	eth_current = edev;

	return 0;
}

