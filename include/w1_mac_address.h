/*
 * Copyright (C) 2012 Jean-Christophe PLAGNIOL-VILLARD <plagnio@jcrosoft.com>
 *
 * Under GPLv2 only
 */

#ifndef __W1_MAC_ADDRESS_H__
#define __W1_MAC_ADDRESS_H__

/**
 * w1_local_mac_address_register - use the first 3 byte of the id of a 1-wire
 * or 6 if no OUI provided device to provide an Ethernet address
 * @ethid: ethernet device id
 * @oui: Ethernet OUI (3 bytes)
 * @w1_dev: 1-wire device name
 *
 * Generate a local Ethernet address (MAC) that is not multicast using a 1-wire id.
 */
static inline int w1_local_mac_address_register(int ethid, char * oui, char *w1_dev)
{
	char addr[6];
	const char *val;
	u64 id;
	int nb_oui = 0;
	int i, shift;
	char *tmp = NULL;
	int ret = 0;

	if (oui) {
		nb_oui = 3;

		for (i = 0; i < nb_oui; i++)
			addr[i] = oui[i];
	}

	tmp = asprintf("%s.id", w1_dev);
	if (!tmp)
		return -ENOMEM;

	val = getenv(tmp);
	if (!val) {
		ret = -EINVAL;
		goto err;
	}

	id = simple_strtoull(val, NULL, 16);
	if (!id) {
		ret = -EINVAL;
		goto err;
	}

	for (i = nb_oui, shift = 40; i < 6; i++, shift -= 8)
		addr[i] = (id >> shift) & 0xff;

	addr[0] &= 0xfe;	/* clear multicast bit */
	addr[0] |= 0x02;	/* set local assignment bit (IEEE802) */

	eth_register_ethaddr(ethid, addr);

err:
	free(tmp);
	return ret;
}

#endif /* __W1_MAC_ADDRESS_H__ */
