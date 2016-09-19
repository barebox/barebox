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
 */

#include <common.h>
#include <command.h>
#include <complete.h>
#include <driver.h>
#include <init.h>
#include <net.h>
#include <of.h>
#include <linux/phy.h>
#include <errno.h>
#include <malloc.h>

static struct eth_device *eth_current;
static uint64_t last_link_check;

LIST_HEAD(netdev_list);

struct eth_ethaddr {
	struct list_head list;
	u8 ethaddr[6];
	int ethid;
	struct device_node *node;
};

static LIST_HEAD(ethaddr_list);

int eth_set_ethaddr(struct eth_device *edev, const char *ethaddr)
{
	int ret;

	ret = edev->set_ethaddr(edev, ethaddr);
	if (ret)
		return ret;

	memcpy(edev->ethaddr, ethaddr, ETH_ALEN);

	return 0;
}

static void register_preset_mac_address(struct eth_device *edev, const char *ethaddr)
{
	unsigned char ethaddr_str[sizeof("xx:xx:xx:xx:xx:xx")];

	if (is_valid_ether_addr(ethaddr)) {
		ethaddr_to_string(ethaddr, ethaddr_str);
		dev_info(&edev->dev, "got preset MAC address: %s\n", ethaddr_str);
		eth_set_ethaddr(edev, ethaddr);
	}
}

static int eth_get_registered_ethaddr(struct eth_device *edev, void *buf)
{
	struct eth_ethaddr *addr;
	struct device_node *node = NULL;

	if (edev->parent)
		node = edev->parent->device_node;

	list_for_each_entry(addr, &ethaddr_list, list) {
		if ((node && node == addr->node) ||
				addr->ethid == edev->dev.id) {
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
	struct eth_device *edev;

	eth_drop_ethaddr(ethid);

	for_each_netdev(edev) {
		if (edev->dev.id == ethid) {
			register_preset_mac_address(edev, ethaddr);
			return;
		}
	}

	addr = xzalloc(sizeof(*addr));
	addr->ethid = ethid;
	memcpy(addr->ethaddr, ethaddr, 6);
	list_add_tail(&addr->list, &ethaddr_list);
}

static struct eth_device *eth_get_by_node(struct device_node *node)
{
	struct eth_device *edev;

	for_each_netdev(edev) {
		if (!edev->parent)
			continue;
		if (!edev->parent->device_node)
			continue;
		if (edev->parent->device_node == node)
			return edev;
	}
	return NULL;
}

void of_eth_register_ethaddr(struct device_node *node, const char *ethaddr)
{
	struct eth_ethaddr *addr;
	struct eth_device *edev;

	edev = eth_get_by_node(node);
	if (edev) {
		register_preset_mac_address(edev, ethaddr);
		return;
	}

	addr = xzalloc(sizeof(*addr));
	addr->node = node;
	memcpy(addr->ethaddr, ethaddr, 6);
	list_add_tail(&addr->list, &ethaddr_list);
}

void eth_set_current(struct eth_device *eth)
{
	eth_current = eth;
}

struct eth_device * eth_get_current(void)
{
	return eth_current;
}

struct eth_device *eth_get_byname(const char *ethname)
{
	struct eth_device *edev;

	for_each_netdev(edev) {
		if (!strcmp(ethname, eth_name(edev)))
			return edev;
	}
	return NULL;
}

#ifdef CONFIG_AUTO_COMPLETE
int eth_complete(struct string_list *sl, char *instr)
{
	struct eth_device *edev;
	int len;

	len = strlen(instr);

	for_each_netdev(edev) {
		if (strncmp(instr, eth_name(edev), len))
			continue;

		string_list_add_asprintf(sl, "%s ", eth_name(edev));
	}
	return COMPLETE_CONTINUE;
}
#endif

/*
 * Check for link if we haven't done so for longer.
 */
static int eth_carrier_check(struct eth_device *edev, int force)
{
	int ret;

	if (!IS_ENABLED(CONFIG_PHYLIB))
		return 0;

	if (!edev->phydev)
		return 0;

	if (force)
		phy_wait_aneg_done(edev->phydev);

	if (force || is_timeout(last_link_check, 5 * SECOND) ||
			!edev->phydev->link) {
		ret = phy_update_status(edev->phydev);
		if (ret)
			return ret;
		last_link_check = get_time_ns();
	}

	return edev->phydev->link ? 0 : -ENETDOWN;
}

/*
 * Check if we have a current ethernet device and
 * eventually open it if we have to.
 */
static int eth_check_open(struct eth_device *edev)
{
	int ret;

	if (edev->active)
		return 0;

	ret = edev->open(edev);
	if (ret)
		return ret;

	edev->active = 1;

	return eth_carrier_check(edev, 1);
}

int eth_send(struct eth_device *edev, void *packet, int length)
{
	int ret;

	ret = eth_check_open(edev);
	if (ret)
		return ret;

	ret = eth_carrier_check(edev, 0);
	if (ret)
		return ret;

	led_trigger_network(LED_TRIGGER_NET_TX);

	return edev->send(edev, packet, length);
}

static int __eth_rx(struct eth_device *edev)
{
	int ret;

	ret = eth_check_open(edev);
	if (ret)
		return ret;

	ret = eth_carrier_check(edev, 0);
	if (ret)
		return ret;

	return edev->recv(edev);
}

int eth_rx(void)
{
	struct eth_device *edev;

	for_each_netdev(edev) {
		if (edev->active)
			__eth_rx(edev);
	}

	return 0;
}

static int eth_param_set_ethaddr(struct param_d *param, void *priv)
{
	struct eth_device *edev = priv;

	return eth_set_ethaddr(edev, edev->ethaddr);
}

#ifdef CONFIG_OFTREE
static void eth_of_fixup_node(struct device_node *root,
			      const char *node_path, int ethid,
			      const u8 ethaddr[6])
{
	struct device_node *node;
	int ret;

	if (!is_valid_ether_addr(ethaddr)) {
		pr_debug("%s: no valid mac address, cannot fixup\n",
			 __func__);
		return;
	}

	if (node_path) {
		node = of_find_node_by_path_from(root, node_path);
	} else {
		char eth[12];
		sprintf(eth, "ethernet%d", ethid);
		node = of_find_node_by_alias(root, eth);
	}

	if (!node) {
		pr_debug("%s: no node to fixup\n", __func__);
		return;
	}

	ret = of_set_property(node, "mac-address", ethaddr, 6, 1);
	if (ret)
		pr_err("Setting mac-address property of %s failed with: %s\n",
		       node->full_name, strerror(-ret));
}

static int eth_of_fixup(struct device_node *root, void *unused)
{
	struct eth_ethaddr *addr;
	struct eth_device *edev;

	/*
	 * Add the mac-address property for each ethaddr and then each network
	 * device we find a node path for and which has a valid mac address.
	 * This will find both network devices barebox was told about as well as
	 * addresses registered by boards but for which no network device was
	 * ever loaded.
	 */
	list_for_each_entry(addr, &ethaddr_list, list)
		eth_of_fixup_node(root, addr->node ? addr->node->full_name : NULL,
				  addr->ethid, addr->ethaddr);

	for_each_netdev(edev)
		eth_of_fixup_node(root, edev->nodepath, edev->dev.id, edev->ethaddr);

	return 0;
}

static int eth_register_of_fixup(void)
{
	return of_register_fixup(eth_of_fixup, NULL);
}
late_initcall(eth_register_of_fixup);
#endif

int eth_register(struct eth_device *edev)
{
	struct device_d *dev = &edev->dev;
	unsigned char ethaddr[6];
	int ret, found = 0;

	if (!edev->get_ethaddr) {
		dev_err(dev, "no get_mac_address found for current eth device\n");
		return -1;
	}

	strcpy(edev->dev.name, "eth");

	if (edev->parent)
		edev->dev.parent = edev->parent;

	if (edev->dev.parent && edev->dev.parent->device_node) {
		edev->dev.id = of_alias_get_id(edev->dev.parent->device_node, "ethernet");
		if (edev->dev.id < 0)
			edev->dev.id = DEVICE_ID_DYNAMIC;
	} else {
		edev->dev.id = DEVICE_ID_DYNAMIC;
	}

	ret = register_device(&edev->dev);
	if (ret)
		return ret;

	edev->devname = xstrdup(dev_name(&edev->dev));

	dev_add_param_ip(dev, "ipaddr", NULL, NULL, &edev->ipaddr, edev);
	dev_add_param_ip(dev, "serverip", NULL, NULL, &edev->serverip, edev);
	dev_add_param_ip(dev, "gateway", NULL, NULL, &edev->gateway, edev);
	dev_add_param_ip(dev, "netmask", NULL, NULL, &edev->netmask, edev);
	dev_add_param_mac(dev, "ethaddr", eth_param_set_ethaddr, NULL,
			edev->ethaddr, edev);
	edev->bootarg = xstrdup("");
	dev_add_param_string(dev, "linux.bootargs", NULL, NULL, &edev->bootarg, NULL);

	if (edev->init)
		edev->init(edev);

	list_add_tail(&edev->list, &netdev_list);

	ret = eth_get_registered_ethaddr(edev, ethaddr);
	if (!ret)
		found = 1;

	if (!found) {
		ret = edev->get_ethaddr(edev, ethaddr);
		if (!ret)
			found = 1;
	}

	if (found)
		register_preset_mac_address(edev, ethaddr);

	if (IS_ENABLED(CONFIG_OFDEVICE) && edev->parent &&
			edev->parent->device_node)
		edev->nodepath = xstrdup(edev->parent->device_node->full_name);

	if (!eth_current)
		eth_current = edev;

	return 0;
}

void eth_unregister(struct eth_device *edev)
{
	if (edev == eth_current)
		eth_current = NULL;

	if (edev->active)
		edev->halt(edev);

	if (IS_ENABLED(CONFIG_OFDEVICE))
		free(edev->nodepath);

	free(edev->devname);

	unregister_device(&edev->dev);
	list_del(&edev->list);
}

void led_trigger_network(enum led_trigger trigger)
{
	led_trigger(trigger, TRIGGER_FLASH);
	led_trigger(LED_TRIGGER_NET_TXRX, TRIGGER_FLASH);
}
