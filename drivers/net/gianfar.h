/* SPDX-License-Identifier: GPL-2.0-only */
/*
 *  gianfar.h
 *
 *  Driver for the Motorola Triple Speed Ethernet Controller
 *
 * Copyright 2012 GE Intelligent Platforms, Inc.
 * Copyright 2004, 2007, 2009  Freescale Semiconductor, Inc.
 * (C) Copyright 2003, Motorola, Inc.
 * based on tsec.h by Xianghua Xiao and Andy Fleming 2003-2009
 */

#ifndef __GIANFAR_H
#define __GIANFAR_H

#include <net.h>
#include <config.h>
#include <mach/gianfar.h>

#define MAC_ADDR_LEN 6

/* TBI register addresses */
#define GFAR_TBI_CR		0x00
#define GFAR_TBI_SR		0x01
#define GFAR_TBI_ANA		0x04
#define GFAR_TBI_ANLPBPA	0x05
#define GFAR_TBI_ANEX		0x06
#define GFAR_TBI_TBICON		0x11

/* TBI MDIO register bit fields*/
#define GFAR_TBICON_CLK_SELECT		0x0020
#define GFAR_TBIANA_ASYMMETRIC_PAUSE	0x0100
#define GFAR_TBIANA_SYMMETRIC_PAUSE	0x0080
#define GFAR_TBIANA_HALF_DUPLEX		0x0040
#define GFAR_TBIANA_FULL_DUPLEX		0x0020
/* The two reserved bits below are used in AN3869 to enable SGMII. */
#define GFAR_TBIANA_RESERVED1		0x4000
#define GFAR_TBIANA_RESERVED15		0x0001
#define GFAR_TBICR_PHY_RESET		0x8000
#define GFAR_TBICR_ANEG_ENABLE		0x1000
#define GFAR_TBICR_RESTART_ANEG		0x0200
#define GFAR_TBICR_FULL_DUPLEX		0x0100
#define GFAR_TBICR_SPEED1_SET		0x0040

/* MAC register bits */
#define GFAR_MACCFG1_SOFT_RESET		0x80000000
#define GFAR_MACCFG1_RESET_RX_MC	0x00080000
#define GFAR_MACCFG1_RESET_TX_MC	0x00040000
#define GFAR_MACCFG1_RESET_RX_FUN	0x00020000
#define TESC_MACCFG1_RESET_TX_FUN	0x00010000
#define GFAR_MACCFG1_LOOPBACK		0x00000100
#define GFAR_MACCFG1_RX_FLOW		0x00000020
#define GFAR_MACCFG1_TX_FLOW		0x00000010
#define GFAR_MACCFG1_SYNCD_RX_EN	0x00000008
#define GFAR_MACCFG1_RX_EN		0x00000004
#define GFAR_MACCFG1_SYNCD_TX_EN	0x00000002
#define GFAR_MACCFG1_TX_EN		0x00000001

#define MACCFG2_INIT_SETTINGS		0x00007205
#define GFAR_MACCFG2_FULL_DUPLEX	0x00000001
#define GFAR_MACCFG2_IF			0x00000300
#define GFAR_MACCFG2_GMII		0x00000200
#define GFAR_MACCFG2_MII		0x00000100

#define ECNTRL_INIT_SETTINGS		0x00001000
#define GFAR_ECNTRL_TBI_MODE		0x00000020
#define GFAR_ECNTRL_R100		0x00000008
#define GFAR_ECNTRL_SGMII_MODE		0x00000002

#ifndef GFAR_TBIPA_VALUE
	#define GFAR_TBIPA_VALUE	0x1f
#endif
#define GFAR_MIIMCFG_INIT_VALUE		0x00000003
#define GFAR_MIIMCFG_RESET		0x80000000

#define GFAR_MIIMIND_BUSY		0x00000001
#define GFAR_MIIMIND_NOTVALID		0x00000004

#define GFAR_MIIM_CONTROL		0x00000000
#define GFAR_MIIM_CONTROL_RESET		0x00009140
#define GFAR_MIIM_CONTROL_INIT		0x00001140
#define GFAR_MIIM_CONTROL_RESTART	0x00001340
#define GFAR_MIIM_ANEN			0x00001000

#define GFAR_MIIM_CR			0x00000000
#define GFAR_MIIM_CR_RST		0x00008000
#define GFAR_MIIM_CR_INIT		0x00001000

#define GFAR_MIIM_STATUS		0x1
#define GFAR_MIIM_STATUS_AN_DONE	0x00000020
#define GFAR_MIIM_STATUS_LINK		0x0004

#define GFAR_MIIM_PHYIR1		0x2
#define GFAR_MIIM_PHYIR2		0x3

#define GFAR_MIIM_ANAR			0x4
#define GFAR_MIIM_ANAR_INIT		0x1e1

#define GFAR_MIIM_TBI_ANLPBPA		0x5
#define GFAR_MIIM_TBI_ANLPBPA_HALF	0x00000040
#define GFAR_MIIM_TBI_ANLPBPA_FULL	0x00000020

#define GFAR_MIIM_TBI_ANEX		0x6
#define GFAR_MIIM_TBI_ANEX_NP		0x00000004
#define GFAR_MIIM_TBI_ANEX_PRX		0x00000002

#define GFAR_MIIM_GBIT_CONTROL		0x9
#define GFAR_MIIM_GBIT_CONTROL_INIT	0xe00

#define GFAR_MIIM_EXT_PAGE_ACCESS	0x1f

#define GFAR_MIIM_GBIT_CON		0x09
#define GFAR_MIIM_GBIT_CON_ADVERT	0x0e00

#define GFAR_MIIM_READ_COMMAND		0x00000001

#define MRBLR_INIT_SETTINGS	1536

#define MINFLR_INIT_SETTINGS	0x00000040

#define DMACTRL_INIT_SETTINGS	0x000000c3
#define GFAR_DMACTRL_GRS	0x00000010
#define GFAR_DMACTRL_GTS	0x00000008

#define GFAR_TSTAT_CLEAR_THALT	0x80000000
#define GFAR_RSTAT_CLEAR_RHALT	0x00800000

#define GFAR_IEVENT_INIT_CLEAR	0xffffffff
#define GFAR_IEVENT_BABR	0x80000000
#define GFAR_IEVENT_RXC		0x40000000
#define GFAR_IEVENT_BSY		0x20000000
#define GFAR_IEVENT_EBERR	0x10000000
#define GFAR_IEVENT_MSRO	0x04000000
#define GFAR_IEVENT_GTSC	0x02000000
#define GFAR_IEVENT_BABT	0x01000000
#define GFAR_IEVENT_TXC		0x00800000
#define GFAR_IEVENT_TXE		0x00400000
#define GFAR_IEVENT_TXB		0x00200000
#define GFAR_IEVENT_TXF		0x00100000
#define GFAR_IEVENT_IE		0x00080000
#define GFAR_IEVENT_LC		0x00040000
#define GFAR_IEVENT_CRL		0x00020000
#define GFAR_IEVENT_XFUN	0x00010000
#define GFAR_IEVENT_RXB0	0x00008000
#define GFAR_IEVENT_GRSC	0x00000100
#define GFAR_IEVENT_RXF0	0x00000080

#define GFAR_IMASK_INIT_CLEAR	0x00000000

/* Default Attribute fields */
#define ATTR_INIT_SETTINGS     0x000000c0
#define ATTRELI_INIT_SETTINGS  0x00000000

/* TxBD status field bits */
#define TXBD_READY		0x8000
#define TXBD_PADCRC		0x4000
#define TXBD_WRAP		0x2000
#define TXBD_INTERRUPT		0x1000
#define TXBD_LAST		0x0800
#define TXBD_CRC		0x0400
#define TXBD_DEF		0x0200
#define TXBD_HUGEFRAME		0x0080
#define TXBD_LATECOLLISION	0x0080
#define TXBD_RETRYLIMIT		0x0040
#define	TXBD_RETRYCOUNTMASK	0x003c
#define TXBD_UNDERRUN		0x0002
#define TXBD_STATS		0x03ff

/* RxBD status field bits */
#define RXBD_EMPTY		0x8000
#define RXBD_RO1		0x4000
#define RXBD_WRAP		0x2000
#define RXBD_INTERRUPT		0x1000
#define RXBD_LAST		0x0800
#define RXBD_FIRST		0x0400
#define RXBD_MISS		0x0100
#define RXBD_BROADCAST		0x0080
#define RXBD_MULTICAST		0x0040
#define RXBD_LARGE		0x0020
#define RXBD_NONOCTET		0x0010
#define RXBD_SHORT		0x0008
#define RXBD_CRCERR		0x0004
#define RXBD_OVERRUN		0x0002
#define RXBD_TRUNCATED		0x0001
#define RXBD_STATS		0x003f

struct txbd8 {
	uint16_t     status;	     /* Status Fields */
	uint16_t     length;	     /* Buffer length */
	uint32_t     bufPtr;	     /* Buffer Pointer */
};

struct rxbd8 {
	uint16_t     status;	     /* Status Fields */
	uint16_t     length;	     /* Buffer Length */
	uint32_t     bufPtr;	     /* Buffer Pointer */
};

/* eTSEC general control and status registers */
#define GFAR_IEVENT_OFFSET	0x010	/* Interrupt Event */
#define GFAR_IMASK_OFFSET	0x014	/* Interrupt Mask */
#define GFAR_ECNTRL_OFFSET	0x020	/* Ethernet Control */
#define GFAR_MINFLR_OFFSET	0x024	/* Minimum Frame Length */
#define GFAR_DMACTRL_OFFSET	0x02c	/* DMA Control */

/* eTSEC transmit control and status register */
#define GFAR_TSTAT_OFFSET	0x104	/* transmit status register */
#define GFAR_TBASE0_OFFSET	0x204	/* TxBD Base Address */

/* eTSEC receive control and status register */
#define GFAR_RCTRL_OFFSET	0x300	/* Receive Control */
#define GFAR_RSTAT_OFFSET	0x304	/* transmit status register */
#define GFAR_MRBLR_OFFSET	0x340	/* Maximum Receive Buffer Length */
#define GFAR_RBASE0_OFFSET	0x404	/* RxBD Base Address */

/* eTSEC MAC registers */
#define GFAR_MACCFG1_OFFSET	0x500	/* MAC Configuration #1 */
#define GFAR_MACCFG2_OFFSET	0x504	/* MAC Configuration #2 */
#define GFAR_MIIMCFG_OFFSET	0x520	/* MII management configuration */
#define GFAR_MIIMCOM_OFFSET	0x524	/* MII management command */
#define GFAR_MIIMADD_OFFSET	0x528	/* MII management address */
#define GFAR_MIIMCON_OFFSET	0x52c	/* MII management control */
#define GFAR_MIIMSTAT_OFFSET	0x530	/* MII management status */
#define GFAR_MIIMMIND_OFFSET	0x534	/* MII management indicator */
#define GFAR_MACSTRADDR1_OFFSET 0x540	/* MAC station address #1 */
#define GFAR_MACSTRADDR2_OFFSET 0x544	/* MAC station address #2 */

/* eTSEC transmit and receive counters registers. */
#define GFAR_TR64_OFFSET	0x680
/* eTSEC counter control and TOE statistics registers */
#define GFAR_CAM1_OFFSET	0x738
#define GFAR_CAM2_OFFSET	0x73c

/* Individual/group address registers */
#define GFAR_IADDR0_OFFSET	0x800
#define GFAR_IADDR1_OFFSET	0x804
#define GFAR_IADDR2_OFFSET	0x808
#define GFAR_IADDR3_OFFSET	0x80c
#define GFAR_IADDR4_OFFSET	0x810
#define GFAR_IADDR5_OFFSET	0x814
#define GFAR_IADDR6_OFFSET	0x818
#define GFAR_IADDR7_OFFSET	0x81c

#define GFAR_IADDR(REGNUM) (GFAR_IADDR##REGNUM##_OFFSET)

/* Group address registers */
#define GFAR_GADDR0_OFFSET	0x880
#define GFAR_GADDR1_OFFSET	0x884
#define GFAR_GADDR2_OFFSET	0x888
#define GFAR_GADDR3_OFFSET	0x88c
#define GFAR_GADDR4_OFFSET	0x890
#define GFAR_GADDR5_OFFSET	0x894
#define GFAR_GADDR6_OFFSET	0x898
#define GFAR_GADDR7_OFFSET	0x89c

#define GFAR_GADDR(REGNUM) (GFAR_GADDR##REGNUM##_OFFSET)

/* eTSEC DMA attributes registers */
#define GFAR_ATTR_OFFSET	0xbf8	/* Default Attribute Register */
#define GFAR_ATTRELI_OFFSET	0xbfc	/* Default Attribute Extract Len/Idx */

struct gfar_phy {
	void __iomem *regs;
	struct device_d *dev;
	struct mii_bus miibus;
};

struct gfar_private {
	struct eth_device edev;
	void __iomem *regs;
	int mdiobus_tbi;
	struct gfar_phy *gfar_mdio;
	struct gfar_phy *gfar_tbi;
	struct phy_info *phyinfo;
	struct txbd8 __iomem *txbd;
	struct rxbd8 __iomem *rxbd;
	uint txidx;
	uint rxidx;
	uint phyaddr;
	uint tbicr;
	uint tbiana;
	uint link;
	uint duplexity;
	uint speed;
};
#endif /* __GIANFAR_H */
