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
#include <miidev.h>
#include <errno.h>
#include <malloc.h>

static struct eth_device *eth_current;

static LIST_HEAD(netdev_list);

struct eth_ethaddr {
	struct list_head list;
	u8 ethaddr[6];
	int ethid;
};

static LIST_HEAD(ethaddr_list);

static int eth_get_registered_ethaddr(int ethid, void *buf)
{
	struct eth_ethaddr *addr;

	list_for_each_entry(addr, &ethaddr_list, list) {
		if (addr->ethid == ethid) {
			memcpy(buf, addr->ethaddr, 6);
			return 0;
		}
	}
	return -EINVAL;
}

static void eth_drop_ethaddr(int ethid)
{
	struct eth_ethaddr *addr, *tmp;

	list_for_each_entry_safe(addr, tmp, &ethaddr_list, list) {
		if (addr->ethid == ethid) {
			list_del(&addr->list);
			free(addr);
			return;
		}
	}
}

void eth_register_ethaddr(int ethid, const char *ethaddr)
{
	struct eth_ethaddr *addr;

	eth_drop_ethaddr(ethid);

	addr = xzalloc(sizeof(*addr));
	addr->ethid = ethid;
	memcpy(addr->ethaddr, ethaddr, 6);
	list_add_tail(&addr->list, &ethaddr_list);
}

void eth_set_current(struct eth_device *eth)
{
	if (eth_current && eth_current->active) {
		eth_current->halt(eth_current);
		eth_current->active = 0;
	}

	eth_current = eth;
	net_update_env();
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

int eth_send(void *packet, int length)
{
	int ret;

	if (!eth_current)
		return -ENODEV;

	if (!eth_current->active) {
		ret = eth_current->open(eth_current);
		if (ret)
			return ret;
		eth_current->active = 1;
	}

	led_trigger_network(LED_TRIGGER_NET_TX);

	return eth_current->send(eth_current, packet, length);
}

int eth_rx(void)
{
	int ret;

	if (!eth_current)
		return -ENODEV;

	if (!eth_current->active) {
		ret = eth_current->open(eth_current);
		if (ret)
			return ret;
		eth_current->active = 1;
	}

	return eth_current->recv(eth_current);
}

static int eth_set_ethaddr(struct device_d *dev, struct param_d *param, const char *val)
{
	struct eth_device *edev = dev_to_edev(dev);
	char ethaddr[sizeof("xx:xx:xx:xx:xx:xx")];

	if (!val)
		return dev_param_set_generic(dev, param, NULL);

	if (string_to_ethaddr(val, ethaddr) < 0)
		return -EINVAL;

	dev_param_set_generic(dev, param, val);

	edev->set_ethaddr(edev, ethaddr);

	if (edev == eth_current)
		net_update_env();

	return 0;
}

static int eth_set_ipaddr(struct device_d *dev, struct param_d *param, const char *val)
{
	struct eth_device *edev = dev_to_edev(dev);
	IPaddr_t ip;

	if (!val)
		return dev_param_set_generic(dev, param, NULL);

	if (string_to_ip(val, &ip))
		return -EINVAL;

	dev_param_set_generic(dev, param, val);

	if (edev == eth_current)
		net_update_env();

	return 0;
}

int eth_register(struct eth_device *edev)
{
        struct device_d *dev = &edev->dev;
	unsigned char ethaddr_str[20];
	unsigned char ethaddr[6];
	int ret, found = 0;

	if (!edev->get_ethaddr) {
		dev_err(dev, "no get_mac_address found for current eth device\n");
		return -1;
	}

	strcpy(edev->dev.name, "eth");
	edev->dev.id = -1;

	if (edev->parent)
		dev_add_child(edev->parent, &edev->dev);

	register_device(&edev->dev);

	dev_add_param(dev, "ipaddr", eth_set_ipaddr, NULL, 0);
	dev_add_param(dev, "ethaddr", eth_set_ethaddr, NULL, 0);
	dev_add_param(dev, "gateway", eth_set_ipaddr, NULL, 0);
	dev_add_param(dev, "netmask", eth_set_ipaddr, NULL, 0);
	dev_add_param(dev, "serverip", eth_set_ipaddr, NULL, 0);

	edev->init(edev);

	list_add_tail(&edev->list, &netdev_list);

	ret = eth_get_registered_ethaddr(dev->id, ethaddr);
	if (!ret)
		found = 1;

	if (!found) {
		ret = edev->get_ethaddr(edev, ethaddr);
		if (!ret)
			found = 1;
	}

	if (found) {
		ethaddr_to_string(ethaddr, ethaddr_str);
		if (is_valid_ether_addr(ethaddr)) {
			dev_info(dev, "got MAC address from EEPROM: %s\n", ethaddr_str);
			dev_set_param(dev, "ethaddr", ethaddr_str);
		}
	}

	if (!eth_current) {
		eth_current = edev;
		net_update_env();
	}

	return 0;
}

void eth_unregister(struct eth_device *edev)
{
	if (edev == eth_current)
		eth_current = NULL;

	dev_remove_parameters(&edev->dev);
	unregister_device(&edev->dev);
	list_del(&edev->list);
}

void led_trigger_network(enum led_trigger trigger)
{
	led_trigger(trigger, TRIGGER_FLASH);
	led_trigger(LED_TRIGGER_NET_TXRX, TRIGGER_FLASH);
}
