/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * ar231x.h: Linux driver for the Atheros AR231x Ethernet device.
 * Based on Linux driver:
 *		Copyright (C) 2004 by Sameer Dekate <sdekate@arubanetworks.com>
 *		Copyright (C) 2006 Imre Kaloz <kaloz@openwrt.org>
 *		Copyright (C) 2006-2009 Felix Fietkau <nbd@openwrt.org>
 * Ported to Barebox:
 *		Copyright (C) 2013 Oleksij Rempel <linux@rempel-privat.de>
 */

#ifndef _AR2313_2_H_
#define _AR2313_2_H_

#include <net.h>
#include <mach/ar231x_platform.h>

/* Allocate 64 RX buffers. This will reduce packet loss, until we will start
 * processing them. It is important in noisy network with lots of broadcasts. */
#define AR2313_RXDSC_ENTRIES	64
#define DSC_NEXT(idx)		(((idx) + 1) & (AR2313_RXDSC_ENTRIES - 1))

/* Use system default buffers size. At the moment of writing it was 1518 */
#define AR2313_RX_BUFSIZE	PKTSIZE
#define CRC_LEN			4

/**
 * DMA controller
 */
#define AR231X_DMA_BUS_MODE		0x00 /* (CSR0) */
#define AR231X_DMA_TX_POLL		0x04 /* (CSR1) */
#define AR231X_DMA_RX_POLL		0x08 /* (CSR2) */
#define AR231X_DMA_RX_RING		0x0c /* (CSR3) */
#define AR231X_DMA_TX_RING		0x10 /* (CSR4) */
#define AR231X_DMA_STATUS		0x14 /* (CSR5) */
#define AR231X_DMA_CONTROL		0x18 /* (CSR6) */
#define AR231X_DMA_INTR_ENA		0x1c /* (CSR7) */
#define AR231X_DMA_RX_MISSED		0x20 /* (CSR8) */
/* reserverd 0x24-0x4c (CSR9-19) */
#define AR231X_DMA_CUR_TX_BUF_ADDR	0x50 /* (CSR20) */
#define AR231X_DMA_CUR_RX_BUF_ADDR	0x54 /* (CSR21) */

/**
 * Ethernet controller
 */
#define AR231X_ETH_MAC_CONTROL		0x00
#define AR231X_ETH_MAC_ADDR1		0x04
#define AR231X_ETH_MAC_ADDR2		0x08
#define AR231X_ETH_MCAST_TABLE1		0x0c
#define AR231X_ETH_MCAST_TABLE2		0x10
#define AR231X_ETH_MII_ADDR		0x14
#define AR231X_ETH_MII_DATA		0x18
#define AR231X_ETH_FLOW_CONTROL		0x1c
#define AR231X_ETH_VLAN_TAG		0x20
/* pad 0x24 - 0x3c */
/* ucast_table 0x40-0x5c */

/**
 * RX descriptor status bits. ar231x_descr.status
 */
#define DMA_RX_ERR_CRC		BIT(1)
#define DMA_RX_ERR_DRIB		BIT(2)
#define DMA_RX_ERR_MII		BIT(3)
#define DMA_RX_EV2		BIT(5)
#define DMA_RX_ERR_COL		BIT(6)
#define DMA_RX_LONG		BIT(7)
#define DMA_RX_LS		BIT(8)	/* last descriptor */
#define DMA_RX_FS		BIT(9)	/* first descriptor */
#define DMA_RX_MF		BIT(10)	/* multicast frame */
#define DMA_RX_ERR_RUNT		BIT(11)	/* runt frame */
#define DMA_RX_ERR_LENGTH	BIT(12)	/* length error */
#define DMA_RX_ERR_DESC		BIT(14)	/* descriptor error */
#define DMA_RX_ERROR		BIT(15)	/* error summary */
#define DMA_RX_LEN_MASK		0x3fff0000
#define DMA_RX_LEN_SHIFT	16
#define DMA_RX_FILT		BIT(30)
#define DMA_RX_OWN		BIT(31)	/* desc owned by DMA controller */
#define DMA_RX_FSLS		(DMA_RX_LS | DMA_RX_FS)
#define DMA_RX_MASK		(DMA_RX_FSLS | DMA_RX_MF | DMA_RX_ERROR)

/**
 * RX descriptor configuration bits. ar231x_descr.devcs
 */
#define DMA_RX1_BSIZE_MASK	0x000007ff
#define DMA_RX1_BSIZE_SHIFT	0
#define DMA_RX1_CHAINED		BIT(24)
#define DMA_RX1_RER		BIT(25)

/**
 * TX descriptor status fields. ar231x_descr.status
 */
#define DMA_TX_ERR_UNDER	BIT(1)	/* underflow error */
#define DMA_TX_ERR_DEFER	BIT(2)	/* excessive deferral */
#define DMA_TX_COL_MASK		0x78
#define DMA_TX_COL_SHIFT	3
#define DMA_TX_ERR_HB		BIT(7)	/* hearbeat failure */
#define DMA_TX_ERR_COL		BIT(8)	/* excessive collisions */
#define DMA_TX_ERR_LATE		BIT(9)	/* late collision */
#define DMA_TX_ERR_LINK		BIT(10)	/* no carrier */
#define DMA_TX_ERR_LOSS		BIT(11)	/* loss of carrier */
#define DMA_TX_ERR_JABBER	BIT(14)	/* transmit jabber timeout */
#define DMA_TX_ERROR		BIT(15)	/* frame aborted */
#define DMA_TX_OWN		BIT(31)	/* descr owned by DMA controller */

/**
 * TX descriptor configuration bits. ar231x_descr.devcs
 */
#define DMA_TX1_BSIZE_MASK	0x000007ff
#define DMA_TX1_BSIZE_SHIFT	0
#define DMA_TX1_CHAINED		BIT(24)	/* chained descriptors */
#define DMA_TX1_TER		BIT(25)	/* transmit end of ring */
#define DMA_TX1_FS		BIT(29)	/* first segment */
#define DMA_TX1_LS		BIT(30)	/* last segment */
#define DMA_TX1_IC		BIT(31)	/* interrupt on completion */
#define DMA_TX1_DEFAULT		(DMA_TX1_FS | DMA_TX1_LS | DMA_TX1_TER)

#define MAC_CONTROL_RE		BIT(2)	/* receive enable */
#define MAC_CONTROL_TE		BIT(3)	/* transmit enable */
#define MAC_CONTROL_DC		BIT(5)	/* Deferral check */
#define MAC_CONTROL_ASTP	BIT(8)	/* Auto pad strip */
#define MAC_CONTROL_DRTY	BIT(10)	/* Disable retry */
#define MAC_CONTROL_DBF		BIT(11)	/* Disable bcast frames */
#define MAC_CONTROL_LCC		BIT(12)	/* late collision ctrl */
#define MAC_CONTROL_HP		BIT(13)	/* Hash Perfect filtering */
#define MAC_CONTROL_HASH	BIT(14)	/* Unicast hash filtering */
#define MAC_CONTROL_HO		BIT(15)	/* Hash only filtering */
#define MAC_CONTROL_PB		BIT(16)	/* Pass Bad frames */
#define MAC_CONTROL_IF		BIT(17)	/* Inverse filtering */
#define MAC_CONTROL_PR		BIT(18)	/* promiscuous mode
					 * (valid frames only) */
#define MAC_CONTROL_PM		BIT(19)	/* pass multicast */
#define MAC_CONTROL_F		BIT(20)	/* full-duplex */
#define MAC_CONTROL_DRO		BIT(23)	/* Disable Receive Own */
#define MAC_CONTROL_HBD		BIT(28)	/* heart-beat disabled (MUST BE SET) */
#define MAC_CONTROL_BLE		BIT(30)	/* big endian mode */
#define MAC_CONTROL_RA		BIT(31)	/* receive all
					 * (valid and invalid frames) */

#define MII_ADDR_BUSY		BIT(0)
#define MII_ADDR_WRITE		BIT(1)
#define MII_ADDR_REG_SHIFT	6
#define MII_ADDR_PHY_SHIFT	11
#define MII_DATA_SHIFT		0

#define FLOW_CONTROL_FCE	BIT(1)

#define DMA_BUS_MODE_SWR	BIT(0)	/* software reset */
#define DMA_BUS_MODE_BLE	BIT(7)	/* big endian mode */
#define DMA_BUS_MODE_PBL_SHIFT	8	/* programmable burst length 32 */
#define DMA_BUS_MODE_DBO	BIT(20)	/* big-endian descriptors */

#define DMA_STATUS_TI		BIT(0)	/* transmit interrupt */
#define DMA_STATUS_TPS		BIT(1)	/* transmit process stopped */
#define DMA_STATUS_TU		BIT(2)	/* transmit buffer unavailable */
#define DMA_STATUS_TJT		BIT(3)	/* transmit buffer timeout */
#define DMA_STATUS_UNF		BIT(5)	/* transmit underflow */
#define DMA_STATUS_RI		BIT(6)	/* receive interrupt */
#define DMA_STATUS_RU		BIT(7)	/* receive buffer unavailable */
#define DMA_STATUS_RPS		BIT(8)	/* receive process stopped */
#define DMA_STATUS_ETI		BIT(10)	/* early transmit interrupt */
#define DMA_STATUS_FBE		BIT(13)	/* fatal bus interrupt */
#define DMA_STATUS_ERI		BIT(14)	/* early receive interrupt */
#define DMA_STATUS_AIS		BIT(15)	/* abnormal interrupt summary */
#define DMA_STATUS_NIS		BIT(16)	/* normal interrupt summary */
#define DMA_STATUS_RS_SHIFT	17	/* receive process state */
#define DMA_STATUS_TS_SHIFT	20	/* transmit process state */
#define DMA_STATUS_EB_SHIFT	23	/* error bits */

#define DMA_CONTROL_SR		BIT(1)	/* start receive */
#define DMA_CONTROL_ST		BIT(13)	/* start transmit */
#define DMA_CONTROL_SF		BIT(21)	/* store and forward */


struct ar231x_descr {
	u32 status;		/* OWN, Device control and status. */
	u32 devcs;		/* Packet control bitmap + Length. */
	u32 buffer_ptr;		/* Pointer to packet buffer. */
	u32 next_dsc_ptr;	/* Pointer to next descriptor in chain. */
};

/**
 * Struct private for the Sibyte.
 *
 * Elements are grouped so variables used by the tx handling goes
 * together, and will go into the same cache lines etc. in order to
 * avoid cache line contention between the rx and tx handling on SMP.
 *
 * Frequently accessed variables are put at the beginning of the
 * struct to help the compiler generate better/shorter code.
 */
struct ar231x_eth_priv {
	struct ar231x_eth_platform_data *cfg;
	u8 *mac;
	void __iomem *phy_regs;
	void __iomem *eth_regs;
	void __iomem *dma_regs;
	void __iomem *reset_regs;

	struct eth_device edev;
	struct mii_bus miibus;

	struct ar231x_descr *tx_ring;
	struct ar231x_descr *rx_ring;
	struct ar231x_descr *next_rxdsc;
	u8 kill_rx_ring;
	void *rx_buffer;

	int oldduplex;
	void (*reset_bit)(u32 val, enum reset_state state);
};

#endif							/* _AR2313_H_ */
