// SPDX-License-Identifier: GPL-2.0
/*
 * Handler for Realtek 4 byte DSA switch tags
 * Currently only supports protocol "A" found in RTL8366RB
 * Copyright (c) 2020 Linus Walleij <linus.walleij@linaro.org>
 *
 * This "proprietary tag" header looks like so:
 *
 * -------------------------------------------------
 * | MAC DA | MAC SA | 0x8899 | 2 bytes tag | Type |
 * -------------------------------------------------
 *
 * The 2 bytes tag form a 16 bit big endian word. The exact
 * meaning has been guessed from packet dumps from ingress
 * frames.
 */

#include <net.h>

#include "realtek.h"
#include "dsa_priv.h"

#define RTL4_A_HDR_LEN		4
#define RTL4_A_ETHERTYPE	0x8899
#define RTL4_A_PROTOCOL_SHIFT	12
/*
 * 0x1 = Realtek Remote Control protocol (RRCP)
 * 0x2/0x3 seems to be used for loopback testing
 * 0x9 = RTL8306 DSA protocol
 * 0xa = RTL8366RB DSA protocol
 */
#define RTL4_A_PROTOCOL_RTL8366RB	0xa

static int rtl4a_tag_xmit(struct dsa_port *dp, int port, void *packet, int length)
{
	struct device *dev = dp->ds->dev;
	__be16 *p;
	u8 *tag;
	u16 out;

	/* DSA core already pads out to at least 60 bytes */

	dev_dbg(dev, "add realtek tag to package to port %d\n", port);

	dsa_alloc_etype_header(packet, RTL4_A_HDR_LEN);
	tag = dsa_etype_header_pos(packet);

	/* Set Ethertype */
	p = (__be16 *)tag;
	*p = htons(RTL4_A_ETHERTYPE);

	out = (RTL4_A_PROTOCOL_RTL8366RB << RTL4_A_PROTOCOL_SHIFT);
	/* The lower bits indicate the port number */
	out |= BIT(port);

	p = (__be16 *)(tag + 2);
	*p = htons(out);

	return 0;
}

static int rtl4a_tag_rcv(struct dsa_switch *ds, int *port, void *packet, int length)
{
	struct device *dev = ds->dev;
	u16 protport;
	__be16 *p;
	u16 etype;
	u8 *tag;
	u8 prot;

	tag = packet + 2 * ETH_ALEN;
	p = (__be16 *)tag;
	etype = ntohs(*p);
	if (etype != RTL4_A_ETHERTYPE) {
		/* Not custom, just pass through */
		dev_dbg(dev, "non-realtek ethertype 0x%04x\n", etype);
		return -EINVAL;
	}
	p = (__be16 *)(tag + 2);
	protport = ntohs(*p);
	/* The 4 upper bits are the protocol */
	prot = (protport >> RTL4_A_PROTOCOL_SHIFT) & 0x0f;
	if (prot != RTL4_A_PROTOCOL_RTL8366RB) {
		dev_err(dev, "unknown realtek protocol 0x%01x\n", prot);
		return -EPROTO;
	}
	*port = protport & 0xff;

	dsa_strip_etype_header(packet, RTL4_A_HDR_LEN);

	return 0;
}

const struct dsa_device_ops rtl4a_netdev_ops = {
	.name	= "rtl4a",
	.proto	= DSA_TAG_PROTO_RTL4_A,
	.xmit	= rtl4a_tag_xmit,
	.rcv	= rtl4a_tag_rcv,
	.needed_headroom = RTL4_A_HDR_LEN,
};

MODULE_LICENSE("GPL");
MODULE_ALIAS_DSA_TAG_DRIVER(DSA_TAG_PROTO_RTL4_A);
