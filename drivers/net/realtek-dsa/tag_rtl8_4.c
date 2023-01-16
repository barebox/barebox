// SPDX-License-Identifier: GPL-2.0
/*
 * Handler for Realtek 8 byte switch tags
 *
 * Copyright (C) 2021 Alvin Šipraga <alsi@bang-olufsen.dk>
 *
 * NOTE: Currently only supports protocol "4" found in the RTL8365MB, hence
 * named tag_rtl8_4.
 *
 * This tag has the following format:
 *
 *  0                                  7|8                                 15
 *  |-----------------------------------+-----------------------------------|---
 *  |                               (16-bit)                                | ^
 *  |                       Realtek EtherType [0x8899]                      | |
 *  |-----------------------------------+-----------------------------------| 8
 *  |              (8-bit)              |              (8-bit)              |
 *  |          Protocol [0x04]          |              REASON               | b
 *  |-----------------------------------+-----------------------------------| y
 *  |   (1)  | (1) | (2) |   (1)  | (3) | (1)  | (1) |    (1)    |   (5)    | t
 *  | FID_EN |  X  | FID | PRI_EN | PRI | KEEP |  X  | LEARN_DIS |    X     | e
 *  |-----------------------------------+-----------------------------------| s
 *  |   (1)  |                       (15-bit)                               | |
 *  |  ALLOW |                        TX/RX                                 | v
 *  |-----------------------------------+-----------------------------------|---
 *
 * With the following field descriptions:
 *
 *    field      | description
 *   ------------+-------------
 *    Realtek    | 0x8899: indicates that this is a proprietary Realtek tag;
 *     EtherType |         note that Realtek uses the same EtherType for
 *               |         other incompatible tag formats (e.g. tag_rtl4_a.c)
 *    Protocol   | 0x04: indicates that this tag conforms to this format
 *    X          | reserved
 *   ------------+-------------
 *    REASON     | reason for forwarding packet to CPU
 *               | 0: packet was forwarded or flooded to CPU
 *               | 80: packet was trapped to CPU
 *    FID_EN     | 1: packet has an FID
 *               | 0: no FID
 *    FID        | FID of packet (if FID_EN=1)
 *    PRI_EN     | 1: force priority of packet
 *               | 0: don't force priority
 *    PRI        | priority of packet (if PRI_EN=1)
 *    KEEP       | preserve packet VLAN tag format
 *    LEARN_DIS  | don't learn the source MAC address of the packet
 *    ALLOW      | 1: treat TX/RX field as an allowance port mask, meaning the
 *               |    packet may only be forwarded to ports specified in the
 *               |    mask
 *               | 0: no allowance port mask, TX/RX field is the forwarding
 *               |    port mask
 *    TX/RX      | TX (switch->CPU): port number the packet was received on
 *               | RX (CPU->switch): forwarding port mask (if ALLOW=0)
 *               |                   allowance port mask (if ALLOW=1)
 *
 * The tag can be positioned before Ethertype, using tag "rtl8_4":
 *
 *  +--------+--------+------------+------+-----
 *  | MAC DA | MAC SA | 8 byte tag | Type | ...
 *  +--------+--------+------------+------+-----
 *
 * The tag can also appear between the end of the payload and before the CRC,
 * using tag "rtl8_4t":
 *
 * +--------+--------+------+-----+---------+------------+-----+
 * | MAC DA | MAC SA | TYPE | ... | payload | 8-byte tag | CRC |
 * +--------+--------+------+-----+---------+------------+-----+
 *
 * The added bytes after the payload will break most checksums, either in
 * software or hardware. We don't care for checksums in barebox, so this
 * is just ignored.
 *
 */

#include <linux/bitfield.h>
#include <net.h>

#include "realtek.h"
#include "dsa_priv.h"

/* Protocols supported:
 *
 * 0x04 = RTL8365MB DSA protocol
 */

#define ETH_P_REALTEK			0x8899

#define RTL8_4_TAG_LEN			8

#define RTL8_4_PROTOCOL			GENMASK(15, 8)
#define   RTL8_4_PROTOCOL_RTL8365MB	0x04
#define RTL8_4_REASON			GENMASK(7, 0)
#define   RTL8_4_REASON_FORWARD		0
#define   RTL8_4_REASON_TRAP		80

#define RTL8_4_LEARN_DIS		BIT(5)

#define RTL8_4_TX			GENMASK(3, 0)
#define RTL8_4_RX			GENMASK(10, 0)

static void rtl8_4_write_tag(int port, void *tag)
{
	__be16 tag16[RTL8_4_TAG_LEN / 2];

	/* Set Realtek EtherType */
	tag16[0] = htons(ETH_P_REALTEK);

	/* Set Protocol; zero REASON */
	tag16[1] = htons(FIELD_PREP(RTL8_4_PROTOCOL, RTL8_4_PROTOCOL_RTL8365MB));

	/* Zero FID_EN, FID, PRI_EN, PRI, KEEP; set LEARN_DIS */
	tag16[2] = htons(FIELD_PREP(RTL8_4_LEARN_DIS, 1));

	/* Zero ALLOW; set RX (CPU->switch) forwarding port mask */
	tag16[3] = htons(FIELD_PREP(RTL8_4_RX, BIT(port)));

	memcpy(tag, tag16, RTL8_4_TAG_LEN);
}

static int rtl8_4_tag_xmit(struct dsa_port *dp, int port, void *packet, int length)
{
	dsa_alloc_etype_header(packet, RTL8_4_TAG_LEN);

	rtl8_4_write_tag(port, dsa_etype_header_pos(packet));

	return 0;
}

static int rtl8_4t_tag_xmit(struct dsa_port *dp, int port, void *packet, int length)
{
	rtl8_4_write_tag(port, packet + length - dp->ds->needed_tx_tailroom);

	return 0;
}

static int rtl8_4_read_tag(int *port, struct device *dev, void *tag)
{
	__be16 tag16[RTL8_4_TAG_LEN / 2];
	u16 etype;
	u8 proto;

	memcpy(tag16, tag, RTL8_4_TAG_LEN);

	/* Parse Realtek EtherType */
	etype = ntohs(tag16[0]);
	if (unlikely(etype != ETH_P_REALTEK)) {
		dev_warn(dev, "non-realtek ethertype 0x%04x\n", etype);
		return -EPROTO;
	}

	/* Parse Protocol */
	proto = FIELD_GET(RTL8_4_PROTOCOL, ntohs(tag16[1]));
	if (unlikely(proto != RTL8_4_PROTOCOL_RTL8365MB)) {
		dev_warn(dev, "unknown realtek protocol 0x%02x\n", proto);
		return -EPROTO;
	}

	/* Parse TX (switch->CPU) */
	*port = FIELD_GET(RTL8_4_TX, ntohs(tag16[3]));

	return 0;
}

static int rtl8_4_tag_rcv(struct dsa_switch *ds, int *port, void *packet, int length)
{
	int ret;

	ret = rtl8_4_read_tag(port, ds->dev, dsa_etype_header_pos(packet));
	if (unlikely(ret))
		return ret;

	dsa_strip_etype_header(packet, RTL8_4_TAG_LEN);

	return 0;
}

static int rtl8_4t_tag_rcv(struct dsa_switch *ds, int *port, void *packet, int length)
{
	return rtl8_4_read_tag(port, ds->dev, packet + length - ds->needed_rx_tailroom);
}

/* Ethertype version */
const struct dsa_device_ops rtl8_4_netdev_ops = {
	.name = "rtl8_4",
	.proto = DSA_TAG_PROTO_RTL8_4,
	.xmit = rtl8_4_tag_xmit,
	.rcv = rtl8_4_tag_rcv,
	.needed_headroom = RTL8_4_TAG_LEN,
};

MODULE_ALIAS_DSA_TAG_DRIVER(DSA_TAG_PROTO_RTL8_4);

/* Tail version */
const struct dsa_device_ops rtl8_4t_netdev_ops = {
	.name = "rtl8_4t",
	.proto = DSA_TAG_PROTO_RTL8_4T,
	.xmit = rtl8_4t_tag_xmit,
	.rcv = rtl8_4t_tag_rcv,
	.needed_tailroom = RTL8_4_TAG_LEN,
};

MODULE_ALIAS_DSA_TAG_DRIVER(DSA_TAG_PROTO_RTL8_4T);

MODULE_LICENSE("GPL");
