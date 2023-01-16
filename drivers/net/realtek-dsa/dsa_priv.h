/* SPDX-License-Identifier: GPL-2.0-or-later */
/* SPDX-FileCopyrightText: (c) 2008-2009 Marvell Semiconductor */

#ifndef __DSA_PRIV_H
#define __DSA_PRIV_H

#include <net.h>
#include <linux/string.h>


/* Helper for removing DSA header tags from packets in the RX path.
 * Must not be called before skb_pull(len).
 *
 * Before:
 * packet
 * |
 * v
 * |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
 * +-----------------------+-----------------------+---------------+-------+
 * |    Destination MAC    |      Source MAC       |  DSA header   | EType |
 * +-----------------------+-----------------------+---------------+-------+
 *                                                 |               |
 *                                                 <----- len ----->
 * After:
 *
 * <----- len ----->
 *                 |
 *       >>>>>>>   v
 *       >>>>>>>   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
 *       >>>>>>>   +-----------------------+-----------------------+-------+
 *       >>>>>>>   |    Destination MAC    |      Source MAC       | EType |
 *                 +-----------------------+-----------------------+-------+
 *
 */
static inline void dsa_strip_etype_header(void *packet, int len)
{
	memmove(packet + len, packet, 2 * ETH_ALEN);
}

/* Helper for creating space for DSA header tags in TX path packets.
 *
 * Before:
 *
 *       <<<<<<<   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
 * ^     <<<<<<<   +-----------------------+-----------------------+-------+
 * |     <<<<<<<   |    Destination MAC    |      Source MAC       | EType |
 * |               +-----------------------+-----------------------+-------+
 * <----- len ----->
 * |
 * |
 * packet
 *
 * After:
 *
 * |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
 * +-----------------------+-----------------------+---------------+-------+
 * |    Destination MAC    |      Source MAC       |  DSA header   | EType |
 * +-----------------------+-----------------------+---------------+-------+
 * ^                                               |               |
 * |                                               <----- len ----->
 * packet
 */
static inline void dsa_alloc_etype_header(void *packet, int len)
{
	memmove(packet, packet + len, 2 * ETH_ALEN);
}

/* On TX, skb->data points to skb_mac_header(skb), which means that EtherType
 * header taggers start exactly where the EtherType is (the EtherType is
 * treated as part of the DSA header).
 */
static inline void *dsa_etype_header_pos(void *packet)
{
	return packet + 2 * ETH_ALEN;
}

#endif
