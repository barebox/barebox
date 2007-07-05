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

int eth_send(volatile void *packet, int length)
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

static int eth_handle(struct device_d *dev)
{
	unsigned char ethaddr_tmp[20];
	unsigned char *ethaddr;
	struct eth_device *ndev = dev->driver->type_data;
	char *e = NULL;
	int i;

	printf("%s: %s\n",__FUNCTION__, dev->id);
	if (!ndev->get_mac_address) {
		printf("no get_mac_address found for current eth device\n");
		return -1;
	}

	ethaddr = ndev->enetaddr;

	/* Try to get a MAC address from the eeprom set 'ethaddr' to it.
	 * If this fails we rely on 'ethaddr' being set by the user.
	 */
	if (ndev->get_mac_address(ndev, ethaddr) == 0) {
		sprintf (ethaddr_tmp, "%02X:%02X:%02X:%02X:%02X:%02X",
			ethaddr[0], ethaddr[1], ethaddr[2], ethaddr[3], ethaddr[4], ethaddr[5]);
		printf("got MAC address from EEPROM: %s\n",ethaddr_tmp);
		setenv ("ethaddr", ethaddr_tmp);
	} else {
		ethaddr = getenv ("ethaddr");
		if (!ethaddr){
			printf("could not get MAC address from device and ethaddr not set\n");
			return -1;
		}

		printf("got MAC address from Environment: %s\n",ethaddr);
		for(i = 0; i < 6; i++) {
			ndev->enetaddr[i] = ethaddr ? simple_strtoul (ethaddr, &e, 16) : 0;
			if (ethaddr) {
				ethaddr = (*e) ? e + 1 : e;
			}
			ndev->set_mac_address(eth_current, ndev->enetaddr);
		}
	}

	eth_current = ndev;
	return 0;
}

int eth_initialize(void)
{
	register_device_type_handler(&eth_handle, DEVICE_TYPE_ETHER);

	return 0;
}

core_initcall(eth_initialize);
