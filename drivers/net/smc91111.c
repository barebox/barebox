// SPDX-License-Identifier: GPL-2.0-or-later
/*------------------------------------------------------------------------
 . smc91111.c
 . This is a driver for SMSC's 91C111 single-chip Ethernet device.
 .
 . (C) Copyright 2002
 . Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 . Rolf Offermanns <rof@sysgo.de>
 .
 . Copyright (C) 2001 Standard Microsystems Corporation (SMSC)
 .	 Developed by Simple Network Magic Corporation (SNMC)
 . Copyright (C) 1996 by Erik Stahlman (ES)
 .
 . Information contained in this file was obtained from the LAN91C111
 . manual from SMC.  To get a copy, if you really want one, you can find
 . information under www.smsc.com.
 .
 .
 . "Features" of the SMC chip:
 .   Integrated PHY/MAC for 10/100BaseT Operation
 .   Supports internal and external MII
 .   Integrated 8K packet memory
 .   EEPROM interface for configuration
 .
 . Arguments:
 .	io	= for the base address
 .	irq	= for the IRQ
 .
 . author:
 .	Erik Stahlman				( erik@vt.edu )
 .	Daris A Nevil				( dnevil@snmc.com )
 .
 .
 . Hardware multicast code from Peter Cammaert ( pc@denkart.be )
 .
 . Sources:
 .    o	  SMSC LAN91C111 databook (www.smsc.com)
 .    o	  smc9194.c by Erik Stahlman
 .    o	  skeleton.c by Donald Becker ( becker@cesdis.gsfc.nasa.gov )
 .
 . History:
 .	06/19/03  Richard Woodruff Made barebox environment aware and added mac addr checks.
 .	10/17/01  Marco Hasewinkel Modify for DNP/1110
 .	07/25/01  Woojung Huh	   Modify for ADS Bitsy
 .	04/25/01  Daris A Nevil	   Initial public release through SMSC
 .	03/16/01  Daris A Nevil	   Modified smc9194.c for use with LAN91C111
 ----------------------------------------------------------------------------*/

#include <common.h>

#include <command.h>
#include <net.h>
#include <malloc.h>
#include <init.h>
#include <xfuncs.h>
#include <errno.h>
#include <clock.h>
#include <io.h>
#include <linux/phy.h>
#include <linux/err.h>
#include <platform_data/eth-smc91111.h>

/*---------------------------------------------------------------
 .
 . A description of the SMSC registers is probably in order here,
 . although for details, the SMC datasheet is invaluable.
 .
 . Basically, the chip has 4 banks of registers ( 0 to 3 ), which
 . are accessed by writing a number into the BANK_SELECT register
 . ( I also use a SMC_SELECT_BANK macro for this ).
 .
 . The banks are configured so that for most purposes, bank 2 is all
 . that is needed for simple run time tasks.
 -----------------------------------------------------------------------*/

/*
 * Bank Select Register:
 *  yyyy yyyy 0000 00xx
 *                   xx = bank number
 *  yyyy yyyy           = 0x33, for identification purposes.
*/

#define BANK_SELECT	14

/* Transmit Control Register */
/* BANK 0  */
#define TCR_REG 	0x0000	/* transmit control register */
#define TCR_ENABLE	0x0001	/* When 1 we can transmit */
#define TCR_LOOP	0x0002	/* Controls output pin LBK */
#define TCR_FORCOL	0x0004	/* When 1 will force a collision */
#define TCR_PAD_EN	0x0080	/* When 1 will pad tx frames < 64 bytes w/0 */
#define TCR_NOCRC	0x0100	/* When 1 will not append CRC to tx frames */
#define TCR_MON_CSN	0x0400	/* When 1 tx monitors carrier */
#define TCR_FDUPLX	0x0800  /* When 1 enables full duplex operation */
#define TCR_STP_SQET	0x1000	/* When 1 stops tx if Signal Quality Error */
#define TCR_EPH_LOOP	0x2000	/* When 1 enables EPH block loopback */
#define TCR_SWFDUP	0x8000	/* When 1 enables Switched Full Duplex mode */

#define	TCR_CLEAR	0	/* do NOTHING */
/* the default settings for the TCR register : */
/* QUESTION: do I want to enable padding of short packets ? */
#define TCR_DEFAULT	TCR_ENABLE

/* EPH Status Register */
/* BANK 0  */
#define EPH_STATUS_REG	0x0002
#define ES_TX_SUC	0x0001	/* Last TX was successful */
#define ES_SNGL_COL	0x0002	/* Single collision detected for last tx */
#define ES_MUL_COL	0x0004	/* Multiple collisions detected for last tx */
#define ES_LTX_MULT	0x0008	/* Last tx was a multicast */
#define ES_16COL	0x0010	/* 16 Collisions Reached */
#define ES_SQET		0x0020	/* Signal Quality Error Test */
#define ES_LTXBRD	0x0040	/* Last tx was a broadcast */
#define ES_TXDEFR	0x0080	/* Transmit Deferred */
#define ES_LATCOL	0x0200	/* Late collision detected on last tx */
#define ES_LOSTCARR	0x0400	/* Lost Carrier Sense */
#define ES_EXC_DEF	0x0800	/* Excessive Deferral */
#define ES_CTR_ROL	0x1000	/* Counter Roll Over indication */
#define ES_LINK_OK	0x4000	/* Driven by inverted value of nLNK pin */
#define ES_TXUNRN	0x8000	/* Tx Underrun */


/* Receive Control Register */
/* BANK 0  */
#define RCR_REG		0x0004
#define RCR_RX_ABORT	0x0001	/* Set if a rx frame was aborted */
#define RCR_PRMS	0x0002	/* Enable promiscuous mode */
#define RCR_ALMUL	0x0004	/* When set accepts all multicast frames */
#define RCR_RXEN	0x0100	/* IFF this is set, we can receive packets */
#define RCR_STRIP_CRC	0x0200	/* When set strips CRC from rx packets */
#define RCR_ABORT_ENB	0x0200	/* When set will abort rx on collision */
#define RCR_FILT_CAR	0x0400	/* When set filters leading 12 bit s of carrier */
#define RCR_SOFTRST	0x8000 	/* resets the chip */

/* the normal settings for the RCR register : */
#define RCR_DEFAULT	(RCR_STRIP_CRC | RCR_RXEN)
#define RCR_CLEAR	0x0	/* set it to a base state */

/* Counter Register */
/* BANK 0  */
#define COUNTER_REG	0x0006

/* Memory Information Register */
/* BANK 0  */
#define MIR_REG		0x0008
#define MIR_FREE_MASK	0xff00

/* Receive/Phy Control Register */
/* BANK 0  */
#define RPC_REG		0x000A
#define RPC_SPEED	0x2000	/* When 1 PHY is in 100Mbps mode. */
#define RPC_DPLX	0x1000	/* When 1 PHY is in Full-Duplex Mode */
#define RPC_ANEG	0x0800	/* When 1 PHY is in Auto-Negotiate Mode */
#define RPC_LSXA_SHFT	5	/* Bits to shift LS2A,LS1A,LS0A to lsb */
#define RPC_LSXB_SHFT	2	/* Bits to get LS2B,LS1B,LS0B to lsb */
#define RPC_LED_100_10	(0x00)	/* LED = 100Mbps OR's with 10Mbps link detect */
#define RPC_LED_RES	(0x01)	/* LED = Reserved */
#define RPC_LED_10	(0x02)	/* LED = 10Mbps link detect */
#define RPC_LED_FD	(0x03)	/* LED = Full Duplex Mode */
#define RPC_LED_TX_RX	(0x04)	/* LED = TX or RX packet occurred */
#define RPC_LED_100	(0x05)	/* LED = 100Mbps link dectect */
#define RPC_LED_TX	(0x06)	/* LED = TX packet occurred */
#define RPC_LED_RX	(0x07)	/* LED = RX packet occurred */

/* SMSC reference design: LEDa --> green, LEDb --> yellow */
#define RPC_DEFAULT	( RPC_SPEED | RPC_DPLX | RPC_ANEG	\
			| (RPC_LED_100_10 << RPC_LSXA_SHFT)	\
			| (RPC_LED_TX_RX << RPC_LSXB_SHFT)	)

/* Bank 0 0x000C is reserved */

/* Bank Select Register */
/* All Banks */
#define BSR_REG		0x000E

/* Configuration Reg */
/* BANK 1 */
#define CONFIG_REG	0x0000
#define CONFIG_EXT_PHY	0x0200	/* 1=external MII, 0=internal Phy */
#define CONFIG_GPCNTRL	0x0400	/* Inverse value drives pin nCNTRL */
#define CONFIG_NO_WAIT	0x1000	/* When 1 no extra wait states on ISA bus */
#define CONFIG_EPH_POWER_EN 0x8000 /* When 0 EPH is placed into low power mode. */

/* Default is powered-up, Internal Phy, Wait States, and pin nCNTRL=low */
#define CONFIG_DEFAULT	(CONFIG_EPH_POWER_EN)

/* Base Address Register */
/* BANK 1 */
#define BASE_REG	0x0002

/* Individual Address Registers */
/* BANK 1 */
#define ADDR0_REG	0x0004
#define ADDR1_REG	0x0006
#define ADDR2_REG	0x0008

/* General Purpose Register */
/* BANK 1 */
#define GP_REG		0x000A

/* Control Register */
/* BANK 1 */
#define CTL_REG		0x000C
#define CTL_RCV_BAD	0x4000 /* When 1 bad CRC packets are received */
#define CTL_AUTO_RELEASE 0x0800 /* When 1 tx pages are released automatically */
#define CTL_LE_ENABLE	0x0080 /* When 1 enables Link Error interrupt */
#define CTL_CR_ENABLE	0x0040 /* When 1 enables Counter Rollover interrupt */
#define CTL_TE_ENABLE	0x0020 /* When 1 enables Transmit Error interrupt */
#define CTL_EEPROM_SELECT 0x0004 /* Controls EEPROM reload & store */
#define CTL_RELOAD	0x0002 /* When set reads EEPROM into registers */
#define CTL_STORE	0x0001 /* When set stores registers into EEPROM */
#define CTL_DEFAULT	(0x1A10) /* Autorelease enabled*/

/* MMU Command Register */
/* BANK 2 */
#define MMU_CMD_REG	0x0000
#define MC_BUSY		1	/* When 1 the last release has not completed */
#define MC_NOP		(0<<5)	/* No Op */
#define MC_ALLOC	(1<<5) 	/* OR with number of 256 byte packets */
#define MC_RESET	(2<<5)	/* Reset MMU to initial state */
#define MC_REMOVE	(3<<5) 	/* Remove the current rx packet */
#define MC_RELEASE  	(4<<5) 	/* Remove and release the current rx packet */
#define MC_FREEPKT  	(5<<5) 	/* Release packet in PNR register */
#define MC_ENQUEUE	(6<<5)	/* Enqueue the packet for transmit */
#define MC_RSTTXFIFO	(7<<5)	/* Reset the TX FIFOs */

/* Packet Number Register */
/* BANK 2 */
#define PN_REG		0x0002

/* Allocation Result Register */
/* BANK 2 */
#define AR_REG		0x0003
#define AR_FAILED	0x80	/* Alocation Failed */

/* RX FIFO Ports Register */
/* BANK 2 */
#define RXFIFO_REG	0x0004	/* Must be read as a unsigned short*/
#define RXFIFO_REMPTY	0x8000	/* RX FIFO Empty */

/* TX FIFO Ports Register */
/* BANK 2 */
#define TXFIFO_REG	RXFIFO_REG	/* Must be read as a unsigned short*/
#define TXFIFO_TEMPTY	0x80	/* TX FIFO Empty */

/* Pointer Register */
/* BANK 2 */
#define PTR_REG		0x0006
#define PTR_RCV		0x8000 /* 1=Receive area, 0=Transmit area */
#define PTR_AUTOINC 	0x4000 /* Auto increment the pointer on each access */
#define PTR_READ	0x2000 /* When 1 the operation is a read */
#define PTR_NOTEMPTY	0x0800 /* When 1 _do not_ write fifo DATA REG */

/* Data Register */
/* BANK 2 */
#define SMC91111_DATA_REG	0x0008

/* Interrupt Status/Acknowledge Register */
/* BANK 2 */
#define SMC91111_INT_REG	0x000C

/* Interrupt Mask Register */
/* BANK 2 */
#define IM_REG		0x000D
#define IM_MDINT	0x80 /* PHY MI Register 18 Interrupt */
#define IM_ERCV_INT	0x40 /* Early Receive Interrupt */
#define IM_EPH_INT	0x20 /* Set by Etheret Protocol Handler section */
#define IM_RX_OVRN_INT	0x10 /* Set by Receiver Overruns */
#define IM_ALLOC_INT	0x08 /* Set when allocation request is completed */
#define IM_TX_EMPTY_INT	0x04 /* Set if the TX FIFO goes empty */
#define IM_TX_INT	0x02 /* Transmit Interrrupt */
#define IM_RCV_INT	0x01 /* Receive Interrupt */

/* Multicast Table Registers */
/* BANK 3 */
#define MCAST_REG1	0x0000
#define MCAST_REG2	0x0002
#define MCAST_REG3	0x0004
#define MCAST_REG4	0x0006

/* Management Interface Register (MII) */
/* BANK 3 */
#define MII_REG		0x0008
#define MII_MSK_CRS100	0x4000 /* Disables CRS100 detection during tx half dup */
#define MII_MDOE	0x0008 /* MII Output Enable */
#define MII_MCLK	0x0004 /* MII Clock, pin MDCLK */
#define MII_MDI		0x0002 /* MII Input, pin MDI */
#define MII_MDO		0x0001 /* MII Output, pin MDO */

/* Revision Register */
/* BANK 3 */
#define REV_REG		0x000A /* ( hi: chip id   low: rev # ) */

/* Early RCV Register */
/* BANK 3 */
/* this is NOT on SMC9192 */
#define ERCV_REG	0x000C
#define ERCV_RCV_DISCRD	0x0080 /* When 1 discards a packet being received */
#define ERCV_THRESHOLD	0x001F /* ERCV Threshold Mask */

/* External Register */
/* BANK 7 */
#define EXT_REG		0x0000

#define CHIP_9192	3
#define CHIP_9194	4
#define CHIP_9195	5
#define CHIP_9196	6
#define CHIP_91100	7
#define CHIP_91100FD	8
#define CHIP_91111FD	9

/* Transmit status bits*/
#define TS_SUCCESS 0x0001
#define TS_LOSTCAR 0x0400
#define TS_LATCOL  0x0200
#define TS_16COL   0x0010

/* Receive status bits */
#define RS_ALGNERR	0x8000
#define RS_BRODCAST	0x4000
#define RS_BADCRC	0x2000
#define RS_ODDFRAME	0x1000	/* bug: the LAN91C111 never sets this on receive */
#define RS_TOOLONG	0x0800
#define RS_TOOSHORT	0x0400
#define RS_MULTICAST	0x0001
#define RS_ERRORS	(RS_ALGNERR | RS_BADCRC | RS_TOOLONG | RS_TOOSHORT)

/* PHY Register Addresses (LAN91C111 Internal PHY) */

/* PHY Control Register */
#define PHY_CNTL_REG		0x00
#define PHY_CNTL_RST		0x8000	/* 1=PHY Reset */
#define PHY_CNTL_LPBK		0x4000	/* 1=PHY Loopback */
#define PHY_CNTL_SPEED		0x2000	/* 1=100Mbps, 0=10Mpbs */
#define PHY_CNTL_ANEG_EN	0x1000 /* 1=Enable Auto negotiation */
#define PHY_CNTL_PDN		0x0800	/* 1=PHY Power Down mode */
#define PHY_CNTL_MII_DIS	0x0400	/* 1=MII 4 bit interface disabled */
#define PHY_CNTL_ANEG_RST	0x0200 /* 1=Reset Auto negotiate */
#define PHY_CNTL_DPLX		0x0100	/* 1=Full Duplex, 0=Half Duplex */
#define PHY_CNTL_COLTST		0x0080	/* 1= MII Colision Test */

/* PHY Status Register */
#define PHY_STAT_REG		0x01
#define PHY_STAT_CAP_T4		0x8000	/* 1=100Base-T4 capable */
#define PHY_STAT_CAP_TXF	0x4000	/* 1=100Base-X full duplex capable */
#define PHY_STAT_CAP_TXH	0x2000	/* 1=100Base-X half duplex capable */
#define PHY_STAT_CAP_TF		0x1000	/* 1=10Mbps full duplex capable */
#define PHY_STAT_CAP_TH		0x0800	/* 1=10Mbps half duplex capable */
#define PHY_STAT_CAP_SUPR	0x0040	/* 1=recv mgmt frames with not preamble */
#define PHY_STAT_ANEG_ACK	0x0020	/* 1=ANEG has completed */
#define PHY_STAT_REM_FLT	0x0010	/* 1=Remote Fault detected */
#define PHY_STAT_CAP_ANEG	0x0008	/* 1=Auto negotiate capable */
#define PHY_STAT_LINK		0x0004	/* 1=valid link */
#define PHY_STAT_JAB		0x0002	/* 1=10Mbps jabber condition */
#define PHY_STAT_EXREG		0x0001	/* 1=extended registers implemented */

/* PHY Identifier Registers */
#define PHY_ID1_REG		0x02	/* PHY Identifier 1 */
#define PHY_ID2_REG		0x03	/* PHY Identifier 2 */

/* PHY Auto-Negotiation Advertisement Register */
#define PHY_AD_REG		0x04
#define PHY_AD_NP		0x8000	/* 1=PHY requests exchange of Next Page */
#define PHY_AD_ACK		0x4000	/* 1=got link code word from remote */
#define PHY_AD_RF		0x2000	/* 1=advertise remote fault */
#define PHY_AD_T4		0x0200	/* 1=PHY is capable of 100Base-T4 */
#define PHY_AD_TX_FDX		0x0100	/* 1=PHY is capable of 100Base-TX FDPLX */
#define PHY_AD_TX_HDX		0x0080	/* 1=PHY is capable of 100Base-TX HDPLX */
#define PHY_AD_10_FDX		0x0040	/* 1=PHY is capable of 10Base-T FDPLX */
#define PHY_AD_10_HDX		0x0020	/* 1=PHY is capable of 10Base-T HDPLX */
#define PHY_AD_CSMA		0x0001	/* 1=PHY is capable of 802.3 CMSA */

/* PHY Auto-negotiation Remote End Capability Register */
#define PHY_RMT_REG		0x05
/* Uses same bit definitions as PHY_AD_REG */

/* PHY Configuration Register 1 */
#define PHY_CFG1_REG		0x10
#define PHY_CFG1_LNKDIS		0x8000	/* 1=Rx Link Detect Function disabled */
#define PHY_CFG1_XMTDIS		0x4000	/* 1=TP Transmitter Disabled */
#define PHY_CFG1_XMTPDN		0x2000	/* 1=TP Transmitter Powered Down */
#define PHY_CFG1_BYPSCR		0x0400	/* 1=Bypass scrambler/descrambler */
#define PHY_CFG1_UNSCDS		0x0200	/* 1=Unscramble Idle Reception Disable */
#define PHY_CFG1_EQLZR		0x0100	/* 1=Rx Equalizer Disabled */
#define PHY_CFG1_CABLE		0x0080	/* 1=STP(150ohm), 0=UTP(100ohm) */
#define PHY_CFG1_RLVL0		0x0040	/* 1=Rx Squelch level reduced by 4.5db */
#define PHY_CFG1_TLVL_SHIFT	2	/* Transmit Output Level Adjust */
#define PHY_CFG1_TLVL_MASK	0x003C
#define PHY_CFG1_TRF_MASK	0x0003	/* Transmitter Rise/Fall time */

/* PHY Configuration Register 2 */
#define PHY_CFG2_REG		0x11
#define PHY_CFG2_APOLDIS	0x0020	/* 1=Auto Polarity Correction disabled */
#define PHY_CFG2_JABDIS		0x0010	/* 1=Jabber disabled */
#define PHY_CFG2_MREG		0x0008	/* 1=Multiple register access (MII mgt) */
#define PHY_CFG2_INTMDIO	0x0004	/* 1=Interrupt signaled with MDIO pulseo */

/* PHY Status Output (and Interrupt status) Register */
#define PHY_INT_REG		0x12	/* Status Output (Interrupt Status) */
#define PHY_INT_INT		0x8000	/* 1=bits have changed since last read */
#define PHY_INT_LNKFAIL		0x4000	/* 1=Link Not detected */
#define PHY_INT_LOSSSYNC	0x2000	/* 1=Descrambler has lost sync */
#define PHY_INT_CWRD		0x1000	/* 1=Invalid 4B5B code detected on rx */
#define PHY_INT_SSD		0x0800	/* 1=No Start Of Stream detected on rx */
#define PHY_INT_ESD		0x0400	/* 1=No End Of Stream detected on rx */
#define PHY_INT_RPOL		0x0200	/* 1=Reverse Polarity detected */
#define PHY_INT_JAB		0x0100	/* 1=Jabber detected */
#define PHY_INT_SPDDET		0x0080	/* 1=100Base-TX mode, 0=10Base-T mode */
#define PHY_INT_DPLXDET		0x0040	/* 1=Device in Full Duplex */

/* PHY Interrupt/Status Mask Register */
#define PHY_MASK_REG		0x13	/* Interrupt Mask */
/* Uses the same bit definitions as PHY_INT_REG */

#define SMC_DEBUG 0

/* Autonegotiation timeout in seconds */
#define CONFIG_SMC_AUTONEG_TIMEOUT 10

/*
 . Wait time for memory to be free.  This probably shouldn't be
 . tuned that much, as waiting for this means nothing else happens
 . in the system
*/
#define MEMORY_WAIT_TIME 16

struct accessors {
	void (*ob)(unsigned, void __iomem *, unsigned, unsigned);
	void (*ow)(unsigned, void __iomem *, unsigned, unsigned);
	void (*ol)(unsigned long, void __iomem *, unsigned, unsigned);
	void (*osl)(void __iomem *, unsigned, const void  *, int, unsigned);
	unsigned (*ib)(void __iomem *, unsigned, unsigned);
	unsigned (*iw)(void __iomem *, unsigned, unsigned);
	unsigned long (*il)(void __iomem *, unsigned, unsigned);
	void (*isl)(void __iomem *, unsigned, void*, int, unsigned);
};

struct smc91c111_priv {
	struct mii_bus miibus;
	struct accessors a;
	void __iomem *base;
	int qemu_fixup;
	unsigned shift;
	int version;
	int revision;
	unsigned int control_setup;
	unsigned int config_setup;
};

#if (SMC_DEBUG > 2 )
#define PRINTK3(args...) printf(args)
#else
#define PRINTK3(args...)
#endif

#if SMC_DEBUG > 1
#define PRINTK2(args...) printf(args)
#else
#define PRINTK2(args...)
#endif

#ifdef SMC_DEBUG
#define PRINTK(args...) printf(args)
#else
#define PRINTK(args...)
#endif


#define SMC_DEV_NAME "SMC91111"
#define SMC_ALLOC_MAX_TRY 5
#define SMC_TX_TIMEOUT 30

#define SMC_PHY_CLOCK_DELAY 100

#define ETH_ZLEN 60

static void a8_outb(unsigned value, void __iomem *base, unsigned int offset,
		    unsigned shift)
{
	writeb(value, base + (offset << shift));
}

static void a16_outw(unsigned value, void __iomem *base, unsigned int offset,
		     unsigned shift)
{
	writew(value, base + (offset << shift));
}

static void a32_outl(unsigned long value, void __iomem *base,
		     unsigned int offset, unsigned shift)
{
	writel(value, base + (offset << shift));
}

static void a16_outsl(void __iomem *base, unsigned int offset,
		      const void *data, int count, unsigned shift)
{
	writesw(base + (offset << shift), data, count * 2);
}

static void a16_outl(unsigned long value, void __iomem *base,
		     unsigned int offset, unsigned shift)
{
	writew(value & 0xffff, base + (offset << shift));
	writew(value >> 16, base + ((offset + 2) << shift));
}

static void a32_outsl(void __iomem *base, unsigned int offset,
		      const void *data, int count, unsigned shift)
{
	writesw(base + (offset << shift), data, count * 2);
}

static unsigned a8_inb(void __iomem *base, unsigned int offset, unsigned shift)
{
	return readb(base + (offset << shift));
}

static unsigned a16_inw(void __iomem *base, unsigned int offset, unsigned shift)
{
	return readw(base + (offset << shift));
}

static unsigned long a16_inl(void __iomem *base, unsigned int offset,
			     unsigned shift)
{
	u32 value;

	value = readw(base + (offset << shift));
	value |= readw(base + (offset << shift)) << 16;

	return value;
}

static inline void a16_insl(void __iomem *base, unsigned int offset, void *data,
			    int count, unsigned shift)
{
	readsw(base + (offset << shift), data, count * 2);
}

static unsigned long a32_inl(void __iomem *base, unsigned int offset,
			     unsigned shift)
{
	return readl(base + (offset << shift));
}

static inline void a32_insl(void __iomem *base, unsigned int offset, void *data,
			    int count, unsigned shift)
{
	readsl(base + (offset << shift), data, count);
}

static void a16_outw_algn4(unsigned value, void __iomem *base,
			   unsigned int offset, unsigned shift)
{
	u32 v;

	if ((offset << shift) & 2) {
		v = value << 16;
		v |= (a32_inl(base, offset & ~2, shift) & 0xffff);
		a32_outl(v, base, offset & ~2, shift);
	} else {
		writew(value, base + (offset << shift));
	}
}

static const struct accessors access_via_16bit = {
	.ob = a8_outb,
	.ow = a16_outw,
	.ol = a16_outl,
	.osl = a16_outsl,
	.ib = a8_inb,
	.iw = a16_inw,
	.il = a16_inl,
	.isl = a16_insl,
};

/* access happens via a 32 bit bus */
static const struct accessors access_via_32bit = {
	.ob = a8_outb,
	.ow = a16_outw,
	.ol = a32_outl,
	.osl = a32_outsl,
	.ib = a8_inb,
	.iw = a16_inw,
	.il = a32_inl,
	.isl = a32_insl,
};

/* access happens via a 32 bit bus, writes must by word aligned */
static const struct accessors access_via_32bit_aligned_short_writes = {
	.ob = a8_outb,
	.ow = a16_outw_algn4,
	.ol = a32_outl,
	.osl = a32_outsl,
	.ib = a8_inb,
	.iw = a16_inw,
	.il = a32_inl,
	.isl = a32_insl,
};

/* ------------------------------------------------------------------------ */

static unsigned last_bank;
#define SMC_outb(p, value, offset) \
	do {								\
		PRINTK3("\t%s:%d outb: %s[%1d:0x%04x] = 0x%02x\n",	\
			__func__, __LINE__, #offset, last_bank,		\
			(offset), (value));				\
		((p)->a.ob)((value), (p)->base, (offset), (p)->shift);	\
	} while (0)

#define SMC_outw(p, value, offset)					\
	do {								\
		PRINTK3("\t%s:%d outw: %s[%1d:0x%04x] = 0x%04x\n",	\
			__func__, __LINE__, #offset, last_bank,		\
			(offset), (value));				\
		((p)->a.ow)((value), (p)->base, (offset), (p)->shift);	\
	} while (0)

#define SMC_outl(p, value, offset)					\
	do {								\
		PRINTK3("\t%s:%d outl: %s[%1d:0x%04x] = 0x%08lx\n",	\
			__func__, __LINE__, #offset, last_bank,		\
			(offset), (unsigned long)(value));		\
		((p)->a.ol)((value), (p)->base, (offset), (p)->shift);	\
	} while (0)

#define SMC_outsl(p, offset, data, count)\
	do {								\
		PRINTK3("\t%s:%d outsl: %5d@%p -> [%1d:0x%04x]\n",	\
			__func__, __LINE__, (count) * 4, data,		\
			last_bank, (offset));				\
		((p)->a.osl)((p)->base, (offset), data, (count),	\
			     (p)->shift);				\
	} while (0)

#define SMC_inb(p, offset)						\
	({								\
		unsigned _v = ((p)->a.ib)((p)->base, (offset),		\
					  (p)->shift);			\
		PRINTK3("\t%s:%d inb: %s[%1d:0x%04x] -> 0x%02x\n",	\
			__func__, __LINE__, #offset, last_bank,		\
			(offset), _v);					\
		_v; })

#define SMC_inw(p, offset)						\
	({								\
		unsigned _v = ((p)->a.iw)((p)->base, (offset),		\
					  (p)->shift);			\
		PRINTK3("\t%s:%d inw: %s[%1d:0x%04x] -> 0x%04x\n",	\
			__func__, __LINE__, #offset, last_bank,		\
			(offset), _v);					\
		_v; })

#define SMC_inl(p, offset)						\
	({								\
		unsigned long _v = ((p)->a.il)((p)->base, (offset),	\
			(p)->shift);					\
		PRINTK3("\t%s:%d inl: %s[%1d:0x%04x] -> 0x%08lx\n",	\
			__func__, __LINE__, #offset, last_bank,		\
			(offset), _v);					\
		_v; })

#define SMC_insl(p, offset, data, count)				\
	((p)->a.isl)((p)->base, (offset), data, (count), (p)->shift)

#define SMC_SELECT_BANK(p, bank)			\
	do {						\
		SMC_outw(p, bank & 0xf, BANK_SELECT);	\
		last_bank = bank & 0xf;			\
	} while (0)

#if SMC_DEBUG > 2
static void print_packet( unsigned char * buf, int length )
{
	int i;
	int remainder;
	int lines;

	printf("Packet of length %d \n", length );

#if SMC_DEBUG > 3
	lines = length / 16;
	remainder = length % 16;

	for ( i = 0; i < lines ; i ++ ) {
		int cur;

		for ( cur = 0; cur < 8; cur ++ ) {
			unsigned char a, b;

			a = *(buf ++ );
			b = *(buf ++ );
			printf("%02x%02x ", a, b );
		}
		printf("\n");
	}
	for ( i = 0; i < remainder/2 ; i++ ) {
		unsigned char a, b;

		a = *(buf ++ );
		b = *(buf ++ );
		printf("%02x%02x ", a, b );
	}
	printf("\n");
#endif
}
#endif

/* note: timeout in seconds */
static int poll4int(struct smc91c111_priv *priv, unsigned char mask,
			int timeout)
{
	unsigned old_bank = SMC_inw(priv, BSR_REG);
	int i;

	timeout *= 1000;
	SMC_SELECT_BANK(priv, 2);

	for (i = 0; i < timeout; i++) {
		if (SMC_inw(priv, SMC91111_INT_REG) & mask) {
			SMC_SELECT_BANK(priv, old_bank);
			return 0;	/* return happy */
		}
		mdelay(1);
	}

	SMC_SELECT_BANK(priv, old_bank);
	return 1;
}

static void smc_wait_mmu_release_complete(struct smc91c111_priv *priv)
{
	int count = 0;

	/* assume bank 2 selected */
	while (SMC_inw(priv, MMU_CMD_REG) & MC_BUSY) {
		udelay(1);	/* Wait until not busy */
		if (++count > 200)
			break;
	}
}

static int smc91c111_phy_write(struct mii_bus *bus, int phyaddr,
	int phyreg, u16 phydata)
{
	struct smc91c111_priv *priv = (struct smc91c111_priv *)bus->priv;
	int oldBank;
	int i;
	unsigned mask;
	unsigned short mii_reg;
	unsigned char bits[65];
	int clk_idx = 0;

	/* 32 consecutive ones on MDO to establish sync */
	for (i = 0; i < 32; ++i)
		bits[clk_idx++] = MII_MDOE | MII_MDO;

	/* Start code <01> */
	bits[clk_idx++] = MII_MDOE;
	bits[clk_idx++] = MII_MDOE | MII_MDO;

	/* Write command <01> */
	bits[clk_idx++] = MII_MDOE;
	bits[clk_idx++] = MII_MDOE | MII_MDO;

	/* Output the PHY address, msb first */
	mask =  0x10;
	for (i = 0; i < 5; ++i) {
		if (phyaddr & mask)
			bits[clk_idx++] = MII_MDOE | MII_MDO;
		else
			bits[clk_idx++] = MII_MDOE;

		/* Shift to next lowest bit */
		mask >>= 1;
	}

	/* Output the phy register number, msb first */
	mask = 0x10;
	for (i = 0; i < 5; ++i) {
		if (phyreg & mask)
			bits[clk_idx++] = MII_MDOE | MII_MDO;
		else
			bits[clk_idx++] = MII_MDOE;

		/* Shift to next lowest bit */
		mask >>= 1;
	}

	/* Tristate and turnaround (2 bit times) */
	bits[clk_idx++] = 0;
	bits[clk_idx++] = 0;

	/* Write out 16 bits of data, msb first */
	mask = 0x8000;
	for (i = 0; i < 16; ++i) {
		if (phydata & mask)
			bits[clk_idx++] = MII_MDOE | MII_MDO;
		else
			bits[clk_idx++] = MII_MDOE;

		/* Shift to next lowest bit */
		mask >>= 1;
	}

	/* Final clock bit (tristate) */
	bits[clk_idx++] = 0;

	/* Save the current bank */
	oldBank = SMC_inw(priv, BANK_SELECT);

	/* Select bank 3 */
	SMC_SELECT_BANK(priv, 3);

	/* Get the current MII register value */
	mii_reg = SMC_inw(priv, MII_REG);

	/* Turn off all MII Interface bits */
	mii_reg &= ~(MII_MDOE | MII_MCLK | MII_MDI | MII_MDO);

	/* Clock all cycles */
	for (i = 0; i < sizeof bits; ++i) {
		/* Clock Low - output data */
		SMC_outw(priv, mii_reg | bits[i], MII_REG);
		udelay(SMC_PHY_CLOCK_DELAY);

		/* Clock Hi - input data */
		SMC_outw(priv, mii_reg | bits[i] | MII_MCLK, MII_REG);
		udelay (SMC_PHY_CLOCK_DELAY);
		bits[i] |= SMC_inw(priv, MII_REG) & MII_MDI;
	}

	/* Return to idle state */
	/* Set clock to low, data to low, and output tristated */
	SMC_outw(priv, mii_reg, MII_REG);
	udelay(SMC_PHY_CLOCK_DELAY);

	/* Restore original bank select */
	SMC_SELECT_BANK(priv, oldBank);

	return 0;
}

static int smc91c111_phy_read(struct mii_bus *bus, int phyaddr, int phyreg)
{
	struct smc91c111_priv *priv = (struct smc91c111_priv *)bus->priv;
	int oldBank;
	int i;
	unsigned char mask;
	unsigned short mii_reg;
	unsigned char bits[64];
	int clk_idx = 0;
	int input_idx;
	uint16_t phydata;

	/* 32 consecutive ones on MDO to establish sync */
	for (i = 0; i < 32; ++i)
		bits[clk_idx++] = MII_MDOE | MII_MDO;

	/* Start code <01> */
	bits[clk_idx++] = MII_MDOE;
	bits[clk_idx++] = MII_MDOE | MII_MDO;

	/* Read command <10> */
	bits[clk_idx++] = MII_MDOE | MII_MDO;
	bits[clk_idx++] = MII_MDOE;

	/* Output the PHY address, msb first */
	mask = 0x10;
	for (i = 0; i < 5; ++i) {
		if (phyaddr & mask)
			bits[clk_idx++] = MII_MDOE | MII_MDO;
		else
			bits[clk_idx++] = MII_MDOE;

		/* Shift to next lowest bit */
		mask >>= 1;
	}

	/* Output the phy register number, msb first */
	mask = 0x10;
	for (i = 0; i < 5; ++i) {
		if (phyreg & mask)
			bits[clk_idx++] = MII_MDOE | MII_MDO;
		else
			bits[clk_idx++] = MII_MDOE;

		/* Shift to next lowest bit */
		mask >>= 1;
	}

	/* Tristate and turnaround (2 bit times) */
	bits[clk_idx++] = 0;
	/*bits[clk_idx++] = 0; */

	/* Input starts at this bit time */
	input_idx = clk_idx;

	/* Will input 16 bits */
	for (i = 0; i < 16; ++i)
		bits[clk_idx++] = 0;

	/* Final clock bit */
	bits[clk_idx++] = 0;

	/* Save the current bank */
	oldBank = SMC_inw(priv, BANK_SELECT);

	/* Select bank 3 */
	SMC_SELECT_BANK(priv, 3);

	/* Get the current MII register value */
	mii_reg = SMC_inw(priv, MII_REG);

	/* Turn off all MII Interface bits */
	mii_reg &= ~(MII_MDOE | MII_MCLK | MII_MDI | MII_MDO);

	/* Clock all 64 cycles */
	for (i = 0; i < sizeof bits; ++i) {
		/* Clock Low - output data */
		SMC_outw(priv, mii_reg | bits[i], MII_REG);
		udelay(SMC_PHY_CLOCK_DELAY);

		/* Clock Hi - input data */
		SMC_outw(priv, mii_reg | bits[i] | MII_MCLK, MII_REG);
		udelay(SMC_PHY_CLOCK_DELAY);
		bits[i] |= SMC_inw(priv, MII_REG) & MII_MDI;
	}

	/* Return to idle state */
	/* Set clock to low, data to low, and output tristated */
	SMC_outw(priv, mii_reg, MII_REG);
	udelay(SMC_PHY_CLOCK_DELAY);

	/* Restore original bank select */
	SMC_SELECT_BANK(priv, oldBank);

	/* Recover input data */
	phydata = 0;
	for (i = 0; i < 16; ++i) {
		phydata <<= 1;
		if (bits[input_idx++] & MII_MDI)
			phydata |= 0x0001;
	}

	return phydata;
}

static void smc91c111_reset(struct eth_device *edev)
{
	struct smc91c111_priv *priv = (struct smc91c111_priv *)edev->priv;
	int rev_vers;

	/* This resets the registers mostly to defaults, but doesn't
	   affect EEPROM.  That seems unnecessary */
	SMC_SELECT_BANK(priv, 0);
	SMC_outw(priv, RCR_SOFTRST, RCR_REG);

	/* Setup the Configuration Register */
	/* This is necessary because the CONFIG_REG is not affected */
	/* by a soft reset */

	SMC_SELECT_BANK(priv, 1);
	SMC_outw(priv, CONFIG_DEFAULT, CONFIG_REG);

	/* Release from possible power-down state */
	/* Configuration register is not affected by Soft Reset */
	if (priv->config_setup)
		SMC_outw(priv, priv->config_setup, CONFIG_REG);
	else
		SMC_outw(priv, SMC_inw(priv, CONFIG_REG) | CONFIG_EPH_POWER_EN,
			 CONFIG_REG);

	SMC_SELECT_BANK(priv, 0);

	/* this should pause enough for the chip to be happy */
	udelay (10);

	/* Disable transmit and receive functionality */
	SMC_outw(priv, RCR_CLEAR, RCR_REG);
	SMC_outw(priv, TCR_CLEAR, TCR_REG);

	/* set the control register */
	SMC_SELECT_BANK(priv, 1);
	if (priv->control_setup)
		SMC_outw(priv, priv->control_setup, CTL_REG);
	else
		SMC_outw(priv, CTL_DEFAULT, CTL_REG);

	/* Reset the MMU */
	SMC_SELECT_BANK(priv, 2);
	smc_wait_mmu_release_complete(priv);
	SMC_outw(priv, MC_RESET, MMU_CMD_REG);

	while (SMC_inw(priv, MMU_CMD_REG) & MC_BUSY)
		udelay(1);	/* Wait until not busy */

	/* Note:  It doesn't seem that waiting for the MMU busy is needed here,
	   but this is a place where future chipsets _COULD_ break.  Be wary
	   of issuing another MMU command right after this */

	/* Disable all interrupts */
	SMC_outb(priv, 0, IM_REG);

	/* Check chip revision (91c94, 91c96, 91c100, ...) */
	SMC_SELECT_BANK(priv, 3);
	rev_vers = SMC_inb(priv, REV_REG);
	priv->revision = (rev_vers >> 4) & 0xf;
	priv->version = rev_vers & 0xf;
	dev_info(edev->parent, "chip is revision=%2d, version=%2d\n",
		 priv->revision, priv->version);
}

static void smc91c111_enable(struct eth_device *edev)
{
	struct smc91c111_priv *priv = (struct smc91c111_priv *)edev->priv;

	SMC_SELECT_BANK(priv, 0);
	/* see the header file for options in TCR/RCR DEFAULT*/
	SMC_outw(priv, TCR_DEFAULT, TCR_REG );
	SMC_outw(priv, RCR_DEFAULT, RCR_REG );
}

static int smc91c111_eth_open(struct eth_device *edev)
{
	struct smc91c111_priv *priv = (struct smc91c111_priv *)edev->priv;
	int ret;

	/* Configure the Receive/Phy Control register */
	SMC_SELECT_BANK(priv, 0);
	if (priv->revision > 4)
		SMC_outw(priv, RPC_DEFAULT, RPC_REG);

	smc91c111_enable(edev);

	if (priv->revision <= 4) {
		dev_info(edev->parent, "force link at 10Mpbs on internal phy\n");
		return 0;
	}

	ret = phy_device_connect(edev, &priv->miibus, 0, NULL,
				 0, PHY_INTERFACE_MODE_NA);

	if (ret)
		return ret;

	if (priv->qemu_fixup && edev->phydev->phy_id == 0x00000000) {
		struct phy_device *dev = edev->phydev;

		dev->speed = SPEED_100;
		dev->duplex = DUPLEX_FULL;
		dev->autoneg = !AUTONEG_ENABLE;
		dev->force = 1;
		dev->link = 1;

		dev_info(edev->parent, "phy with id 0x%08x detected this might be qemu\n",
			dev->phy_id);
		dev_info(edev->parent, "force link at 100Mpbs\n");
	}

	return 0;
}

static void smc91c111_ensure_freemem(struct eth_device *edev)
{
	struct smc91c111_priv *priv = (struct smc91c111_priv *)edev->priv;
	u16 mir, rxfifo;

	SMC_SELECT_BANK(priv, 0);
	mir = SMC_inw(priv, MIR_REG);
	SMC_SELECT_BANK(priv, 2);

	if ((mir & MIR_FREE_MASK) == 0) {
		do {
			SMC_outw(priv, MC_RELEASE, MMU_CMD_REG);
			smc_wait_mmu_release_complete(priv);

			SMC_SELECT_BANK(priv, 0);
			mir = SMC_inw(priv, MIR_REG);
			SMC_SELECT_BANK(priv, 2);
			rxfifo = SMC_inw(priv, RXFIFO_REG);
			dev_dbg(&edev->dev, "%s: card memory saturated, tidying up (rx_tx_fifo=0x%04x mir=0x%04x)\n",
				SMC_DEV_NAME, rxfifo, mir);
		} while (!(rxfifo & RXFIFO_REMPTY));
	}
}

static int smc91c111_eth_send(struct eth_device *edev, void *packet,
				int packet_length)
{
	struct smc91c111_priv *priv = (struct smc91c111_priv *)edev->priv;
	unsigned char packet_no;
	unsigned char *buf;
	int length;
	int numPages;
	int try = 0;
	int time_out;
	unsigned char status;
	unsigned char saved_pnr;
	unsigned short saved_ptr;

	/* save PTR and PNR registers before manipulation */
	SMC_SELECT_BANK(priv, 2);
	saved_pnr = SMC_inb(priv, PN_REG );
	saved_ptr = SMC_inw(priv, PTR_REG );

	length = ETH_ZLEN < packet_length ? packet_length : ETH_ZLEN;

	/* allocate memory
	 ** The MMU wants the number of pages to be the number of 256 bytes
	 ** 'pages', minus 1 ( since a packet can't ever have 0 pages :) )
	 **
	 ** The 91C111 ignores the size bits, but the code is left intact
	 ** for backwards and future compatibility.
	 **
	 ** Pkt size for allocating is data length +6 (for additional status
	 ** words, length and ctl!)
	 **
	 ** If odd size then last byte is included in this header.
	 */
	numPages = ((length & 0xfffe) + 6);
	numPages >>= 8;		/* Divide by 256 */

	if (numPages > 7) {
		printf ("%s: Far too big packet error. \n", SMC_DEV_NAME);
		return -EOVERFLOW;
	}

	smc91c111_ensure_freemem(edev);
	/* now, try to allocate the memory */
	SMC_SELECT_BANK(priv, 2);
	SMC_outw(priv, MC_ALLOC | numPages, MMU_CMD_REG);

	/* FIXME: the ALLOC_INT bit never gets set *
	 * so the following will always give a	   *
	 * memory allocation error.		   *
	 * same code works in armboot though	   *
	 * -ro
	 */

again:
	try++;
	time_out = MEMORY_WAIT_TIME;
	do {
		status = SMC_inb(priv, SMC91111_INT_REG);
		if (status & IM_ALLOC_INT) {
			/* acknowledge the interrupt */
			SMC_outb(priv, IM_ALLOC_INT, SMC91111_INT_REG);
			break;
		}
	} while (--time_out);

	if (!time_out) {
		if (try < SMC_ALLOC_MAX_TRY)
			goto again;
		else
			return -ETIMEDOUT;
	}

	PRINTK2 ("%s: memory allocation, try %d succeeded ...\n",
		 SMC_DEV_NAME, try);

	/* I can send the packet now.. */

	buf = (unsigned char *) packet;

	/* If I get here, I _know_ there is a packet slot waiting for me */
	packet_no = SMC_inb(priv, AR_REG);
	if (packet_no & AR_FAILED) {
		/* or isn't there?  BAD CHIP! */
		printf ("%s: Memory allocation failed. \n", SMC_DEV_NAME);
		return -ENOMEM;
	}

	/* we have a packet address, so tell the card to use it */
	SMC_outb(priv, packet_no, PN_REG);

	/* do not write new ptr value if Write data fifo not empty */
	while ( saved_ptr & PTR_NOTEMPTY )
		printf ("Write data fifo not empty!\n");

	/* point to the beginning of the packet */
	SMC_outw(priv, PTR_AUTOINC, PTR_REG);

	/* send the packet length ( +6 for status, length and ctl byte )
	   and the status unsigned short( set to zeros ) */
	SMC_outl(priv, (length + 6) << 16, SMC91111_DATA_REG);

	/* send the actual data
	   . I _think_ it's faster to send the longs first, and then
	   . mop up by sending the last word.  It depends heavily
	   . on alignment, at least on the 486.	 Maybe it would be
	   . a good idea to check which is optimal?  But that could take
	   . almost as much time as is saved?
	 */
	SMC_outsl(priv, SMC91111_DATA_REG, buf, length >> 2);
	if (length & 0x2)
		SMC_outw(priv,
			*((unsigned short*) (buf + (length & 0xFFFFFFFC))),
			SMC91111_DATA_REG);

	/* Send the last byte, if there is one.	  */
	if ((length & 1) == 0)
		SMC_outw(priv, 0, SMC91111_DATA_REG);
	else
		SMC_outw(priv, buf[length - 1] | 0x2000, SMC91111_DATA_REG);

	/* and let the chipset deal with it */
	SMC_outw(priv, MC_ENQUEUE, MMU_CMD_REG);

	/* poll for TX INT */
	/* if (poll4int (IM_TX_INT, SMC_TX_TIMEOUT)) { */
	/* poll for TX_EMPTY INT - autorelease enabled */
	if (poll4int(priv, IM_TX_EMPTY_INT, SMC_TX_TIMEOUT)) {
		/* release packet */
		/* no need to release, MMU does that now */

		/* wait for MMU getting ready (low) */
		while (SMC_inw(priv, MMU_CMD_REG) & MC_BUSY)
			udelay (10);
		return 0;
	} else {
		/* ack. int */
		SMC_outb(priv, IM_TX_EMPTY_INT, SMC91111_INT_REG);

		/* release packet */
		/* no need to release, MMU does that now */

		/* wait for MMU getting ready (low) */
		while (SMC_inw(priv, MMU_CMD_REG) & MC_BUSY)
			udelay (10);
	}

	/* restore previously saved registers */
	SMC_outb(priv, saved_pnr, PN_REG );
	SMC_outw(priv, saved_ptr, PTR_REG );

	return 0;
}

static void smc91c111_eth_halt(struct eth_device *edev)
{
	struct smc91c111_priv *priv = (struct smc91c111_priv *)edev->priv;

	/* no more interrupts for me */
	SMC_SELECT_BANK(priv, 2);
	SMC_outb(priv, 0, IM_REG);

	/* and tell the card to stay away from that nasty outside world */
	SMC_SELECT_BANK(priv, 0);
	SMC_outb(priv, RCR_CLEAR, RCR_REG);
	SMC_outb(priv, TCR_CLEAR, TCR_REG);
}

static int smc91c111_eth_rx(struct eth_device *edev)
{
	struct smc91c111_priv *priv = (struct smc91c111_priv *)edev->priv;
	int	packet_number;
	unsigned short	status;
	unsigned short	packet_length;
	int	is_error = 0;
	unsigned long stat_len;
	unsigned char saved_pnr;
	unsigned short saved_ptr;

	SMC_SELECT_BANK(priv, 2);
	/* save PTR and PTR registers */
	saved_pnr = SMC_inb(priv, PN_REG );
	saved_ptr = SMC_inw(priv, PTR_REG );

	packet_number = SMC_inw(priv, RXFIFO_REG);

	if (packet_number & RXFIFO_REMPTY)
		return 0;

	/*  start reading from the start of the packet */
	SMC_outw(priv, PTR_READ | PTR_RCV | PTR_AUTOINC, PTR_REG );

	/* First two words are status and packet_length */
	stat_len = SMC_inl(priv, SMC91111_DATA_REG);
	status = stat_len & 0xffff;
	packet_length = stat_len >> 16;

	packet_length &= 0x07ff;  /* mask off top bits */

	if ( !(status & RS_ERRORS ) ){
		/* Adjust for having already read the first two words */
		packet_length -= 4; /*4; */


		/* set odd length for bug in LAN91C111, */
		/* which never sets RS_ODDFRAME */
		/* TODO ? */


		PRINTK3(" Reading %d dwords (and %d bytes) \n",
			packet_length >> 2, packet_length & 3 );
		/* QUESTION:  Like in the TX routine, do I want
		   to send the DWORDs or the bytes first, or some
		   mixture.  A mixture might improve already slow PIO
		   performance	*/
		SMC_insl(priv, SMC91111_DATA_REG , NetRxPackets[0],
				packet_length >> 2);
		/* read the left over bytes */
		if (packet_length & 3) {
			int i;

			unsigned char *tail =
					(unsigned char *)(NetRxPackets[0] +
					(packet_length & ~3));
			unsigned long leftover = SMC_inl(priv,
					SMC91111_DATA_REG);
			for (i=0; i<(packet_length & 3); i++)
				*tail++ =
				(unsigned char) (leftover >> (8*i)) & 0xff;
		}

#if	SMC_DEBUG > 2
		printf("Receiving Packet\n");
		print_packet( NetRxPackets[0], packet_length );
#endif
	} else {
		/* error ... */
		/* TODO ? */
		is_error = 1;
	}

	while (SMC_inw(priv, MMU_CMD_REG ) & MC_BUSY )
		udelay(1); /* Wait until not busy */

	/*  error or good, tell the card to get rid of this packet */
	SMC_outw(priv, MC_RELEASE, MMU_CMD_REG );

	while (SMC_inw(priv, MMU_CMD_REG ) & MC_BUSY )
		udelay(1); /* Wait until not busy */

	/* restore saved registers */
	SMC_outb(priv, saved_pnr, PN_REG );
	SMC_outw(priv, saved_ptr, PTR_REG );

	if (!is_error) {
		/* Pass the packet up to the protocol layers. */
		net_receive(edev, NetRxPackets[0], packet_length);
		return 0;
	}

	return -EINVAL;
}

static int smc91c111_get_ethaddr(struct eth_device *edev, unsigned char *m)
{
	struct smc91c111_priv *priv = (struct smc91c111_priv *)edev->priv;
	int valid = 0;
	int i;

	SMC_SELECT_BANK(priv, 1);

	for (i = 0; i < 6; ++i)
		valid += m[i] = SMC_inb(priv, (ADDR0_REG + i));

	/* no eeprom, no mac */
	if (!valid)
		return -1;

	return 0;
}

static int smc91c111_set_ethaddr(struct eth_device *edev,
					const unsigned char *mac_addr)
{
	struct smc91c111_priv *priv = (struct smc91c111_priv *)edev->priv;
	unsigned address;
	int i;

	SMC_SELECT_BANK(priv, 1);

	for (i = 0; i < 6; i += 2) {
		address = mac_addr[i + 1] << 8;
		address |= mac_addr[i];
		SMC_outw(priv, address, (ADDR0_REG + i));
	}

	return 0;
}

#if (SMC_DEBUG > 2 )

/*------------------------------------------------------------
 . Debugging function for viewing MII Management serial bitstream
 .-------------------------------------------------------------*/
static void smc_dump_mii_stream (unsigned char * bits, int size)
{
	int i;

	printf ("BIT#:");
	for (i = 0; i < size; ++i) {
		printf ("%d", i % 10);
	}

	printf ("\nMDOE:");
	for (i = 0; i < size; ++i) {
		if (bits[i] & MII_MDOE)
			printf ("1");
		else
			printf ("0");
	}

	printf ("\nMDO :");
	for (i = 0; i < size; ++i) {
		if (bits[i] & MII_MDO)
			printf ("1");
		else
			printf ("0");
	}

	printf ("\nMDI :");
	for (i = 0; i < size; ++i) {
		if (bits[i] & MII_MDI)
			printf ("1");
		else
			printf ("0");
	}

	printf ("\n");
}
#endif

static int smc91c111_init_dev(struct eth_device *edev)
{
	return 0;
}

static int smc91c111_probe(struct device_d *dev)
{
	struct resource *iores;
	struct eth_device *edev;
	struct smc91c111_priv *priv;

	edev = xzalloc(sizeof(struct eth_device) +
			sizeof(struct smc91c111_priv));
	edev->priv = (struct smc91c111_priv *)(edev + 1);

	priv = edev->priv;
	priv->a = access_via_32bit;

	if (dev->platform_data) {
		struct smc91c111_pdata *pdata = dev->platform_data;

		priv->qemu_fixup = pdata->qemu_fixup;
		priv->shift = pdata->addr_shift;
		if (pdata->bus_width == 16)
			priv->a = access_via_16bit;
		if (((pdata->bus_width == 0) || (pdata->bus_width == 32)) &&
		    (pdata->word_aligned_short_writes))
			priv->a = access_via_32bit_aligned_short_writes;
		priv->config_setup = pdata->config_setup;
		priv->control_setup = pdata->control_setup;
	}

	edev->init = smc91c111_init_dev;
	edev->open = smc91c111_eth_open;
	edev->send = smc91c111_eth_send;
	edev->recv = smc91c111_eth_rx;
	edev->halt = smc91c111_eth_halt;
	edev->get_ethaddr = smc91c111_get_ethaddr;
	edev->set_ethaddr = smc91c111_set_ethaddr;
	edev->parent = dev;

	priv->miibus.read = smc91c111_phy_read;
	priv->miibus.write = smc91c111_phy_write;
	priv->miibus.priv = priv;
	priv->miibus.parent = dev;
	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	priv->base = IOMEM(iores->start);

	smc91c111_reset(edev);

	if (priv->revision > 4)
		mdiobus_register(&priv->miibus);
	eth_register(edev);

	return 0;
}

static struct driver_d smc91c111_driver = {
        .name  = "smc91c111",
        .probe = smc91c111_probe,
};
device_platform_driver(smc91c111_driver);
