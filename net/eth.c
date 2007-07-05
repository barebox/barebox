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

static int enetaddr_set(struct device_d *dev, struct param_d *param, value_t val)
{
	struct eth_device *edev;
	char buf[6];

	if (dev->type != DEVICE_TYPE_ETHER)
		return -EINVAL;

	edev = dev->type_data;

	string_to_enet_addr(val.val_str, buf);
	edev->set_mac_address(edev, buf);
	memcpy(edev->enetaddr, buf, 6);

	if (param->value.val_str)
		free(param->value.val_str);
	param->value.val_str = strdup(val.val_str);


	return 0;
}

static struct param_d eth_params[] = {
        { .name = "ip", .type = PARAM_TYPE_IPADDR,},
        { .name = "mac", .type = PARAM_TYPE_STRING, .set = enetaddr_set,},
        { .name = "gateway", .type = PARAM_TYPE_IPADDR,},
        { .name = "netmask", .type = PARAM_TYPE_IPADDR,},
        { .name = "serverip", .type = PARAM_TYPE_IPADDR,},
};

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

	if (!eth_current)
		return 0;

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
	value_t val;
	int i;


        printf("%s\n",__FUNCTION__);

	if (!edev->get_mac_address) {
		printf("no get_mac_address found for current eth device\n");
		return -1;
	}

        for (i = 0; i < 5; i++)
                dev_add_parameter(dev, &eth_params[i]);

	if (edev->get_mac_address(edev, ethaddr) == 0) {
		sprintf (ethaddr_str, "%02X:%02X:%02X:%02X:%02X:%02X",
			ethaddr[0], ethaddr[1], ethaddr[2], ethaddr[3], ethaddr[4], ethaddr[5]);
		printf("got MAC address from EEPROM: %s\n",ethaddr_str);
		val.val_str = ethaddr_str;
		dev_set_param(dev, "mac", val);
		memcpy(edev->enetaddr, ethaddr, 6);
	}

	eth_current = edev;

	return 0;
}

