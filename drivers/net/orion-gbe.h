/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * (C) Copyright 2014
 *   Pengutronix, Michael Grzeschik <mgr@pengutronix.de>
 *   Sebastian Hesselbarth <sebastian.hesselbarth@gmail.com>
 *
 * based on kirkwood_egiga driver from u-boot
 *   (C) Copyright 2009
 *     Marvell Semiconductor <www.marvell.com>
 *     Written-by: Prafulla Wadaskar <prafulla@marvell.com>
 *
 *     based on - Driver for MV64360X ethernet ports
 *     Copyright (C) 2002 rabeeh@galileo.co.il
 */

#ifndef __ORION_GBE_
#define __ORION_GBE_

/* Ethernet Unit Base Address */
#define EUNIT_BA(x)		(0x200 + (x) * 0x8)
/* Ethernet Unit Size */
#define EUNIT_S(x)		(0x204 + (x) * 0x8)
/* Ethernet Unit High Address */
#define EUNIT_HA(x)		(0x280 + (x) * 0x4)
/* Ethernet Unit Base Address Enable */
#define EUNIT_BARE		0x290
/* Ethernet Unit Port Access Protect */
#define EUNIT_PAP		0x294
#define NO_ACCESS		0
#define ACCESS_READ_ONLY	1
#define ACCESS_FULL		3

/* Port Registers Offset */
#define PORTn_REGS(x)		(0x400 + (x) * 0x400)
/* Port Configuration */
#define PORT_C			0x000
#define  PROMISCUOUS_MODE	BIT(0)
#define  DEFAULT_RXQ(x)		((x) << 1)
#define  DEFAULT_ARPQ(x)	((x) << 4)
#define  BCAST_OTHER_REJECT	BIT(7)
#define  BCAST_IP_REJECT	BIT(8)
#define  BCAST_ARP_REJECT	BIT(9)
#define  AUTO_SET_NO_TX_ERR	BIT(12)
#define  TCPQ_CAPTURE_ENABLE	BIT(14)
#define  UDPQ_CAPTURE_ENABLE	BIT(15)
#define  DEFAULT_TCPQ(x)	((x) << 16)
#define  DEFAULT_UDPQ(x)	((x) << 19)
#define  DEFAULT_BPDUQ(x)	((x) << 22)
#define  RX_TCP_CHKSUM_HEADER	BIT(25)
/* Port Configuration Extended */
#define PORT_CX			0x004
#define  TX_CRC_GEN_DISABLE	BIT(3)
#define  BPDUQ_CAPTURE_ENABLE	BIT(0)
/* Port MAC Address High */
#define PORT_MACAL		0x014
/* Port MAC Address Low */
#define PORT_MACAH		0x018
/* Port SDMA Configuration */
#define PORT_SDC		0x01c
#define  TX_BURST_SIZE_1	(0 << 22)
#define  TX_BURST_SIZE_2	(1 << 22)
#define  TX_BURST_SIZE_4	(2 << 22)
#define  TX_BURST_SIZE_8	(3 << 22)
#define  TX_BURST_SIZE_16	(4 << 22)
#define  TX_BLM_SWAP		(0 << 5)
#define  TX_BLM_NO_SWAP		(1 << 5)
#define  RX_BLM_SWAP		(0 << 4)
#define  RX_BLM_NO_SWAP		(1 << 4)
#define  RX_BURST_SIZE_1	(0 << 1)
#define  RX_BURST_SIZE_2	(1 << 1)
#define  RX_BURST_SIZE_4	(2 << 1)
#define  RX_BURST_SIZE_8	(3 << 1)
#define  RX_BURST_SIZE_16	(4 << 1)
/* Port Serial Control 0 */
#define PORT_SC0		0x03c
#define  SC0_RESERVED		BIT(9)
#define  PORT_ENABLE		BIT(0)
#define  FORCE_LINK_PASS	BIT(1)
#define  DISABLE_ANEG_DUPLEX	BIT(2)
#define  DISABLE_ANEG_FLOWCTRL	BIT(3)
#define  ADVERTISE_PAUSE	BIT(4)
#define  FORCE_FLOWCTRL_OFF	(0 << 5)
#define  FORCE_FLOWCTRL_ON	(1 << 5)
#define  FORCE_FLOWCTRL_MASK	(3 << 5)
#define  FORCE_BACKPRESS_NO_JAM	(0 << 7)
#define  FORCE_BACKPRESS_JAM	(1 << 7)
#define  FORCE_BACKPRESS_MASK	(3 << 7)
#define  FORCE_NO_LINK_FAIL	BIT(10)
#define  DISABLE_ANEG_SPEED	BIT(13)
#define  ADVERTISE_DTE		BIT(14)
#define  MII_PHY_MODE		BIT(15)
#define  MII_SRC_SYNCHRONOUS	BIT(16)
#define  MRU_1518		(0 << 17)
#define  MRU_1522		(1 << 17)
#define  MRU_1552		(2 << 17)
#define  MRU_9022		(3 << 17)
#define  MRU_9192		(4 << 17)
#define  MRU_9700		(5 << 17)
#define  MRU_MASK		(7 << 17)
#define  SET_FULL_DUPLEX	BIT(21)
#define  SET_FLOWCTRL_ENABLE	BIT(22)
#define  SET_SPEED_1000		(1 << 23)
#define  SET_SPEED_10		(0 << 23)
#define  SET_SPEED_100		(2 << 23)
#define  SET_SPEED_MASK		(3 << 23)
/* Port Status 0 */
#define PORT_S0			0x044
/* Port Trasmit Queue Command */
#define PORT_TQC		0x048
/* Port Serial Control 1 */
#define PORT_SC1		0x04c
#define  SC1_RESERVED		(0x2 << 9)
#define  LOOPBACK_PCS		BIT(1)
#define  RGMII_ENABLE		BIT(3)
#define  PORT_RESET		BIT(4)
#define  CLK125_BYPASS		BIT(5)
#define  INBAND_ANEG		BIT(6)
#define  INBAND_ANEG_BYPASS	BIT(7)
#define  INBAND_ANEG_RESTART	BIT(8)
#define  LIMIT_TO_1000BASEX	BIT(11)
#define  COL_ON_BACKPRESS	BIT(15)
#define  COL_LIMIT(x)		(((x) & 0xfff) << 16)
#define  DEFAULT_COL_LIMIT	COL_LIMIT(0x23)
#define  COL_ON_BACKPRESS	BIT(15)
#define  EN_MII_ODD_PREAMBLE	BIT(22)
/* Port Status 1 */
#define PORT_S1			0x050
/* Port Interrupt Cause */
#define PORT_IC			0x060
/* Port Interrupt Mask */
#define PORT_IM			0x068
#define  INT_SUM		BIT(31)
#define  TX_END			BIT(19)
#define  RXQ_ERR		(15 << 11)
#define  RX_ERR			BIT(10)
#define  TXQ_ERR		(15 << 2)
#define  EXTENDED_INT		BIT(1)
#define  RX_RETURN		BIT(0)
/* Port Extended Interrupt Cause */
#define PORT_EIC		0x064
/* Port Extended Interrupt Mask */
#define PORT_EIM		0x06c
#define  EXTENDED_INT_SUM	BIT(31)
#define  PRBS_ERR		BIT(25)
#define  INTERNAL_ADDR_ERR	BIT(23)
#define  LINK_CHANGE		BIT(20)
#define  TX_UNDERRUN		BIT(19)
#define  RX_OVERRUN		BIT(18)
#define  PHY_CHANGE		BIT(16)
#define  TX_ERR			BIT(8)
#define  TX_RETURN		BIT(0)
/* Port Maximum Transmit Unit */
#define PORT_MTU		0x0e8
/* Port Current Receive Descriptor Pointer */
#define PORT_CRDP(x)		(0x20c + (x) * 0x10)
/* Port Receive Queue Command */
#define	PORT_RQC		0x280
/* Port Transmit Current Queue Descriptor Pointer */
#define	PORT_TCQDP(x)		(0x2c0 + (x) * 0x04)
/* Port Transmit Queue Token Bucket Counter */
#define	PORT_TQTBCNT(x)		(0x300 + (x) * 0x10)
/* Port Transmit Queue Token Bucket Configuration */
#define	PORT_TQTBC(x)		(0x304 + (x) * 0x10)

#define	PORT_DFSMT(x)		(0x1000 + ((x) * 0x04))
#define	PORT_DFOMT(x)		(0x1100 + ((x) * 0x04))
#define	PORT_DFUT(x)		(0x1200 + ((x) * 0x04))

#define DFT_ENTRY_MASK		0xff
#define DFT_ENTRY_SIZE		8
#define DFT_QUEUE_SHIFT		1
#define DFT_PASS		BIT(0)

#define RXDESC_ERROR		BIT(0)
#define RXDESC_ERROR_CRC	(0 << 1)
#define RXDESC_ERROR_OVERRUN	(1 << 1)
#define RXDESC_ERROR_MAXLEN	(2 << 1)
#define RXDESC_ERROR_RESOURCE	(3 << 1)
#define RXDESC_ERROR_MASK	(3 << 1)
#define RXDESC_ERROR_SHIFT	1
#define RXDESC_L4_CHECKSUM(x)	(((x) & (0xffff << 3)) >> 3)
#define RXDESC_VLAN_TAGGED	BIT(19)
#define RXDESC_BDPU		BIT(20)
#define RXDESC_FRAME_TCP	(0 << 21)
#define RXDESC_FRAME_UDP	(1 << 21)
#define RXDESC_FRAME_OTHER	(2 << 21)
#define RXDESC_FRAME_MASK	(3 << 21)
#define RXDESC_L2_IS_ETHERNET	BIT(23)
#define RXDESC_L4_IS_IPV4	BIT(24)
#define RXDESC_L4_HEADER_OK	BIT(25)
#define RXDESC_LAST		BIT(26)
#define RXDESC_FIRST		BIT(27)
#define RXDESC_UNKNOWN_DEST	BIT(28)
#define RXDESC_ENABLE_IRQ	BIT(29)
#define RXDESC_L4_CHECKSUM_OK	BIT(30)
#define RXDESC_OWNED_BY_DMA	BIT(31)

#define RXDESC_BYTECOUNT_FRAG	BIT(2)

#define TXDESC_ERROR			BIT(0)
#define TXDESC_ERROR_LATE_COLL		(0 << 1)
#define TXDESC_ERROR_UNDERRUN		(1 << 1)
#define TXDESC_ERROR_RET_LIMIT		(2 << 1)
#define TXDESC_ERROR_MASK		(3 << 1)
#define TXDESC_ERROR_SHIFT		1
#define TXDESC_LCC_SNAP			BIT(9)
#define TXDESC_L4_CHK_FIRST		BIT(10)
#define TXDESC_IPV4_HEADER_LEN(x)	(((x) & 0xf) << 11)
#define TXDESC_VLAN_TAGGED		BIT(15)
#define TXDESC_FRAME_TCP		(0 << 16)
#define TXDESC_FRAME_UDP		(1 << 16)
#define TXDESC_GEN_FRAME_CHECKSUM	BIT(17)
#define TXDESC_GEN_IPV4_CHECKSUM	BIT(18)
#define TXDESC_ZERO_PADDING		BIT(19)
#define TXDESC_LAST			BIT(20)
#define TXDESC_FIRST			BIT(21)
#define TXDESC_GEN_CRC			BIT(22) /* Orion5x only */
#define TXDESC_ENABLE_IRQ		BIT(23)
#define TXDESC_NO_AUTO_RETURN		BIT(30)
#define TXDESC_OWNED_BY_DMA		BIT(31)

#endif
