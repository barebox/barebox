/*
 * ethtool.h: Defines for Linux ethtool.
 *
 * Copyright (C) 1998 David S. Miller (davem@redhat.com)
 * Copyright 2001 Jeff Garzik <jgarzik@pobox.com>
 * Portions Copyright 2001 Sun Microsystems (thockin@sun.com)
 * Portions Copyright 2002 Intel (eli.kupermann@intel.com,
 *                                christopher.leech@intel.com,
 *                                scott.feldman@intel.com)
 * Portions Copyright (C) Sun Microsystems 2008
 */

#ifndef _LINUX_ETHTOOL_H
#define _LINUX_ETHTOOL_H

/* Indicates what features are supported by the interface. */
#define SUPPORTED_10baseT_Half		(1 << 0)
#define SUPPORTED_10baseT_Full		(1 << 1)
#define SUPPORTED_100baseT_Half		(1 << 2)
#define SUPPORTED_100baseT_Full		(1 << 3)
#define SUPPORTED_1000baseT_Half	(1 << 4)
#define SUPPORTED_1000baseT_Full	(1 << 5)
#define SUPPORTED_Autoneg		(1 << 6)
#define SUPPORTED_TP			(1 << 7)
#define SUPPORTED_AUI			(1 << 8)
#define SUPPORTED_MII			(1 << 9)
#define SUPPORTED_FIBRE			(1 << 10)
#define SUPPORTED_BNC			(1 << 11)
#define SUPPORTED_10000baseT_Full	(1 << 12)
#define SUPPORTED_Pause			(1 << 13)
#define SUPPORTED_Asym_Pause		(1 << 14)
#define SUPPORTED_2500baseX_Full	(1 << 15)
#define SUPPORTED_Backplane		(1 << 16)
#define SUPPORTED_1000baseKX_Full	(1 << 17)
#define SUPPORTED_10000baseKX4_Full	(1 << 18)
#define SUPPORTED_10000baseKR_Full	(1 << 19)
#define SUPPORTED_10000baseR_FEC	(1 << 20)

/* Indicates what features are advertised by the interface. */
#define ADVERTISED_10baseT_Half		(1 << 0)
#define ADVERTISED_10baseT_Full		(1 << 1)
#define ADVERTISED_100baseT_Half	(1 << 2)
#define ADVERTISED_100baseT_Full	(1 << 3)
#define ADVERTISED_1000baseT_Half	(1 << 4)
#define ADVERTISED_1000baseT_Full	(1 << 5)
#define ADVERTISED_Autoneg		(1 << 6)
#define ADVERTISED_TP			(1 << 7)
#define ADVERTISED_AUI			(1 << 8)
#define ADVERTISED_MII			(1 << 9)
#define ADVERTISED_FIBRE		(1 << 10)
#define ADVERTISED_BNC			(1 << 11)
#define ADVERTISED_10000baseT_Full	(1 << 12)
#define ADVERTISED_Pause		(1 << 13)
#define ADVERTISED_Asym_Pause		(1 << 14)
#define ADVERTISED_2500baseX_Full	(1 << 15)
#define ADVERTISED_Backplane		(1 << 16)
#define ADVERTISED_1000baseKX_Full	(1 << 17)
#define ADVERTISED_10000baseKX4_Full	(1 << 18)
#define ADVERTISED_10000baseKR_Full	(1 << 19)
#define ADVERTISED_10000baseR_FEC	(1 << 20)

/* The following are all involved in forcing a particular link
 * mode for the device for setting things.  When getting the
 * devices settings, these indicate the current mode and whether
 * it was foced up into this mode or autonegotiated.
 */

/* The forced speed, 10Mb, 100Mb, gigabit, 2.5Gb, 10GbE. */
#define SPEED_10		10
#define SPEED_100		100
#define SPEED_1000		1000
#define SPEED_2500		2500
#define SPEED_10000		10000

#define SPEED_UNKNOWN		-1

/* Duplex, half or full. */
#define DUPLEX_HALF		0x00
#define DUPLEX_FULL		0x01
#define DUPLEX_UNKNOWN		0xff

/* Which connector port. */
#define PORT_TP			0x00
#define PORT_AUI		0x01
#define PORT_MII		0x02
#define PORT_FIBRE		0x03
#define PORT_BNC		0x04
#define PORT_OTHER		0xff

/* Which transceiver to use. */
#define XCVR_INTERNAL		0x00
#define XCVR_EXTERNAL		0x01
#define XCVR_DUMMY1		0x02
#define XCVR_DUMMY2		0x03
#define XCVR_DUMMY3		0x04

/* Enable or disable autonegotiation.  If this is set to enable,
 * the forced link modes above are completely ignored.
 */
#define AUTONEG_DISABLE		0x00
#define AUTONEG_ENABLE		0x01

/* Mode MDI or MDI-X */
#define ETH_TP_MDI_INVALID	0x00
#define ETH_TP_MDI		0x01
#define ETH_TP_MDI_X		0x02

/* Wake-On-Lan options. */
#define WAKE_PHY		(1 << 0)
#define WAKE_UCAST		(1 << 1)
#define WAKE_MCAST		(1 << 2)
#define WAKE_BCAST		(1 << 3)
#define WAKE_ARP		(1 << 4)
#define WAKE_MAGIC		(1 << 5)
#define WAKE_MAGICSECURE	(1 << 6) /* only meaningful if WAKE_MAGIC */

#endif /* _LINUX_ETHTOOL_H */
