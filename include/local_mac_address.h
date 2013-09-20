/*
 * Copyright (C) 2012 Jean-Christophe PLAGNIOL-VILLARD <plagnio@jcrosoft.com>
 *
 * Under GPLv2 only
 */

#ifndef __LOCAL_MAC_ADDRESS_H__
#define __LOCAL_MAC_ADDRESS_H__

#include <net.h>

/**
 * local_mac_address_register - use random number with fix
 * OUI provided device to provide an Ethernet address
 * @ethid: ethernet device id
 * @oui: Ethernet OUI (3 bytes)
 *
 * Generate a local Ethernet address (MAC) that is not multicast using a 1-wire id.
 */
static inline int local_mac_address_register(int ethid, char * oui)
{
	char addr[6];
	int nb_oui = 3;
	int i;

	if (!oui)
		return -EINVAL;

	random_ether_addr(addr);

	for (i = 0; i < nb_oui; i++)
		addr[i] = oui[i];

	addr[0] &= 0xfe;	/* clear multicast bit */
	addr[0] |= 0x02;	/* set local assignment bit (IEEE802) */

	eth_register_ethaddr(ethid, addr);

	return 0;
}

#endif /* __LOCAL_MAC_ADDRESS_H__ */
