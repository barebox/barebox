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
#include <errno.h>
#include <malloc.h>

static struct eth_device *eth_current;

static LIST_HEAD(netdev_list);

void eth_set_current(struct eth_device *eth)
{
	eth_current = eth;
}

struct eth_device * eth_get_current(void)
{
	return eth_current;
}

struct eth_device *eth_get_byname(char *ethname)
{
	struct eth_device *edev;
	char name[MAX_DRIVER_NAME];

	list_for_each_entry(edev, &netdev_list, list) {
		sprintf(name, "%s%d", edev->dev.name, edev->dev.id);
		if (!strcmp(ethname, name))
			return edev;
	}
	return NULL;
}

int eth_open(void)
{
	if (!eth_current)
		return -ENODEV;

	return eth_current->open(eth_current);
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
		return -ENODEV;

	return eth_current->send(eth_current, packet, length);
}

int eth_rx(void)
{
	if (!eth_current)
		return -ENODEV;

	return eth_current->recv(eth_current);
}

static int eth_set_ethaddr(struct device_d *dev, struct param_d *param, const char *val)
{
	struct eth_device *edev = dev->type_data;
	char ethaddr[sizeof("xx:xx:xx:xx:xx:xx")];

	if (string_to_ethaddr(val, ethaddr) < 0)
		return -EINVAL;

	free(param->value);
	param->value = strdup(val);

	edev->set_ethaddr(edev, ethaddr);

	return 0;
}

static int eth_set_ipaddr(struct device_d *dev, struct param_d *param, const char *val)
{
	IPaddr_t ip;

	if (string_to_ip(val, &ip))
		return -EINVAL;

	free(param->value);
	param->value = strdup(val);

	return 0;
}

int eth_register(struct eth_device *edev)
{
        struct device_d *dev = &edev->dev;
	unsigned char ethaddr_str[20];
	unsigned char ethaddr[6];

	if (!edev->get_ethaddr) {
		printf("no get_mac_address found for current eth device\n");
		return -1;
	}

	strcpy(edev->dev.name, "eth");
	register_device(&edev->dev);

	dev->type_data = edev;
	edev->param_ip.name = "ipaddr";
	edev->param_ip.set = &eth_set_ipaddr;
	edev->param_ethaddr.name = "ethaddr";
	edev->param_ethaddr.set = &eth_set_ethaddr;
	edev->param_gateway.name = "gateway";
	edev->param_gateway.set = &eth_set_ipaddr;
	edev->param_netmask.name = "netmask";
	edev->param_netmask.set = &eth_set_ipaddr;
	edev->param_serverip.name = "serverip";
	edev->param_serverip.set = &eth_set_ipaddr;
	dev_add_param(dev, &edev->param_ip);
	dev_add_param(dev, &edev->param_ethaddr);
	dev_add_param(dev, &edev->param_gateway);
	dev_add_param(dev, &edev->param_netmask);
	dev_add_param(dev, &edev->param_serverip);

	edev->init(edev);

	list_add_tail(&edev->list, &netdev_list);

	if (edev->get_ethaddr(edev, ethaddr) == 0) {
		ethaddr_to_string(ethaddr, ethaddr_str);
		printf("got MAC address from EEPROM: %s\n",ethaddr_str);
		dev_set_param(dev, "ethaddr", ethaddr_str);
	}

	if (!eth_current)
		eth_current = edev;

	return 0;
}

void eth_unregister(struct eth_device *edev)
{
	if (edev->param_ip.value)
		free(edev->param_ip.value);
	if (edev->param_ethaddr.value)
		free(edev->param_ethaddr.value);
	if (edev->param_gateway.value)
		free(edev->param_gateway.value);
	if (edev->param_netmask.value)
		free(edev->param_netmask.value);
	if (edev->param_serverip.value)
		free(edev->param_serverip.value);

	if (eth_current == edev)
		eth_current = NULL;

	list_del(&edev->list);
}

