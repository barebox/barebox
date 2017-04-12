/*
 * Copyright (C) 2011 Juergen Beisert, Pengutronix
 *
 * Davicom DM9000(E/A/B) NIC fast Ethernet driver for Barebox
 *
 * In some ways inspired by code
 *
 *   Copyright (C) 1997  Sten Wang
 *   1997-1998 DAVICOM Semiconductor,Inc.
 *   2003 Weilun Huang <weilun_huang@davicom.com.tw>
 *   2003 <saschahauer@web.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#undef DEBUG

#include <init.h>
#include <common.h>
#include <driver.h>
#include <net.h>
#include <io.h>
#include <xfuncs.h>
#include <platform_data/eth-dm9000.h>
#include <errno.h>
#include <linux/phy.h>

#define DM9K_ID		0x90000A46
#define CHIPR_DM9000A	0x19
#define CHIPR_DM9000B	0x1A

#define DM9K_PKT_NONE	0x00	/* no packet received (end marker) */
#define DM9K_PKT_RDY	0x01	/* packet ready to read */
#define DM9K_PKT_ERR	0x02	/* chip error */
#define DM9K_PKT_MAX	1536	/* packet max size */

#define DM9K_NCR	0x00
# define NCR_EXT_PHY	(1 << 7)
# define NCR_WAKEEN	(1 << 6)
# define NCR_FCOL	(1 << 4)
# define NCR_FDX	(1 << 3)
# define NCR_LBK	(3 << 1)
# define NCR_MAC_LBK	(1 << 1)
# define NCR_RST	(1 << 0)

#define DM9K_NSR	0x01
# define NSR_SPEED	(1 << 7)
# define NSR_LINKST	(1 << 6)
# define NSR_WAKEST	(1 << 5)
# define NSR_TX2END	(1 << 3)
# define NSR_TX1END	(1 << 2)
# define NSR_RXOV	(1 << 1)

#define DM9K_TCR	0x02
# define TCR_TJDIS	(1 << 6)
# define TCR_EXCECM	(1 << 5)
# define TCR_PAD_DIS2	(1 << 4)
# define TCR_CRC_DIS2	(1 << 3)
# define TCR_PAD_DIS1	(1 << 2)
# define TCR_CRC_DIS1	(1 << 1)
# define TCR_TXREQ	(1 << 0)

#define DM9K_TSR1	0x03
#define DM9K_TSR2	0x04
# define TSR_TJTO	(1 << 7)
# define TSR_LC		(1 << 6)
# define TSR_NC		(1 << 5)
# define TSR_LCOL	(1 << 4)
# define TSR_COL	(1 << 3)
# define TSR_EC		(1 << 2)

#define DM9K_RCR	0x05
# define RCR_WTDIS	(1 << 6)
# define RCR_DIS_LONG	(1 << 5)
# define RCR_DIS_CRC	(1 << 4)
# define RCR_ALL	(1 << 3)
# define RCR_RUNT	(1 << 2)
# define RCR_PRMSC	(1 << 1)
# define RCR_RXEN	(1 << 0)

#define DM9K_RSR	0x06
# define RSR_RF		(1 << 7)
# define RSR_MF		(1 << 6)
# define RSR_LCS	(1 << 5)
# define RSR_RWTO	(1 << 4)
# define RSR_PLE	(1 << 3)
# define RSR_AE		(1 << 2)
# define RSR_CE		(1 << 1)
# define RSR_FOE	(1 << 0)
# define RSR_ERR_MASK (RSR_FOE | RSR_CE | RSR_AE | RSR_PLE | RSR_RWTO | RSR_LCS | RSR_RF)

#define DM9K_ROCR	0x07
#define DM9K_BPTR	0x08
#define DM9K_FCTR	0x09
#define DM9K_FCR	0x0A
#define DM9K_EPCR	0x0B
#define DM9K_EPAR	0x0C
#define DM9K_EPDRL	0x0D
#define DM9K_EPDRH	0x0E
#define DM9K_WCR	0x0F

#define DM9K_PAR	0x10
#define DM9K_MAR	0x16

#define DM9K_GPCR	0x1e
#define DM9K_GPR	0x1f
#define DM9K_TRPAL	0x22
#define DM9K_TRPAH	0x23
#define DM9K_RWPAL	0x24
#define DM9K_RWPAH	0x25

#define DM9K_VIDL	0x28
#define DM9K_VIDH	0x29
#define DM9K_PIDL	0x2A
#define DM9K_PIDH	0x2B

#define DM9K_CHIPR	0x2C
#define DM9K_SMCR	0x2F

#define DM9K_PHY	0x40	/* PHY address 0x01 */

#define DM9K_MRCMDX	0xF0
#define DM9K_MRCMD	0xF2
#define DM9K_MRRL	0xF4
#define DM9K_MRRH	0xF5
#define DM9K_MWCMDX	0xF6
#define DM9K_MWCMD	0xF8
#define DM9K_MWRL	0xFA
#define DM9K_MWRH	0xFB
#define DM9K_TXPLL	0xFC
#define DM9K_TXPLH	0xFD

#define DM9K_ISR	0xFE
# define ISR_IOM0	(1 << 7) /* 0: 16 bit, 1: 8 bit*/
# define ISR_LNKCHG	(1 << 5) /* link status change */
# define ISR_UDRUN	(1 << 4) /* transmit underrun */
# define ISR_ROO	(1 << 3) /* receive overflow counter overflow */
# define ISR_ROS	(1 << 2) /* receive overflow */
# define ISR_PT		(1 << 1) /* packet transmitted */
# define ISR_PR		(1 << 0) /* packet received */
# define ISR_CLEAR_MASK (ISR_PR | ISR_PT | ISR_ROS | ISR_ROO | ISR_UDRUN | ISR_LNKCHG)

#define DM9K_IMR	0xFF
# define IMR_PAR	(1 << 7)
# define IMR_ROOM	(1 << 3)
# define IMR_ROM	(1 << 2)
# define IMR_PTM	(1 << 1)
# define IMR_PRM	(1 << 0)

#define FCTR_HWOT(ot)	(( ot & 0xf ) << 4 )
#define FCTR_LWOT(ot)	( ot & 0xf )

struct dm9k {
	void __iomem *iobase;
	void __iomem *iodata;
	struct mii_bus miibus;
	int buswidth;
	int srom;
	uint8_t pckt[2048];
};

/* ------------------ register access functions -------------------------- */

static uint8_t dm9k_ior(struct dm9k *priv, int reg)
{
	writeb(reg, priv->iobase);
	return readb(priv->iodata);
}

static void dm9k_iow(struct dm9k *priv, int reg, uint8_t value)
{
	writeb(reg, priv->iobase);
	writeb(value, priv->iodata);
}

/* ------------------- data move functions ---------------------------- */

static void dm9k_wd_8(void __iomem *port, const void *src, int length)
{
	const uint8_t *from = (const uint8_t *)src;

	while (length--)
		writeb(*from++, port);
}

static void dm9k_rd_8(void __iomem *port, void *dst, unsigned length)
{
	uint8_t *to = (uint8_t *)dst;

	while (length--)
		*to++ = readb(port);
}

static void dm9k_dump_8(void __iomem *port, unsigned length)
{
	while (length--)
		readb(port);
}

static unsigned dm9k_read_packet_status_8(void __iomem *port, unsigned *status)
{
	uint16_t st, le;

	dm9k_rd_8(port, &st, sizeof(st));
	dm9k_rd_8(port, &le, sizeof(le));

	*status = st >> 8;
	return le;
}

static void dm9k_wd_16(void __iomem *port, const void *src, int length)
{
	const uint16_t *from = (const uint16_t *)src;

	length += 1;
	length /= 2;
	while (length--)
		writew(*from++, port);
}

static void dm9k_rd_16(void __iomem *port, void *dst, unsigned length)
{
	uint16_t *to = (uint16_t *)dst;

	length += 1;
	length >>= 1;
	while (length--)
		*to++ = readw(port);
}

static void dm9k_dump_16(void __iomem *port, unsigned length)
{
	length += 1;
	length >>= 1;
	while (length--)
		readw(port);
}

static unsigned dm9k_read_packet_status_16(void __iomem *port, unsigned *status)
{
	*status = readw(port) >> 8;
	return le16_to_cpu(readw(port));
}

static void dm9k_wd_32(void __iomem *port, const void *src, int length)
{
	const uint32_t *from = (const uint32_t *)src;

	length += 3;
	length /= 4;
	while (length--)
		writel(*from++, port);
}

static void dm9k_rd_32(void __iomem *port, void *dst, unsigned length)
{
	uint32_t *to = (uint32_t *)dst;

	length += 3;
	length >>= 2;
	while (length--)
		*to++ = readl(port);
}

static void dm9k_dump_32(void __iomem *port, unsigned length)
{
	length += 3;
	length >>= 2;
	while (length--)
		readl(port);
}

static unsigned dm9k_read_packet_status_32(void __iomem *port, unsigned *status)
{
	uint32_t tmp = readl(port);

	*status = (tmp >> 8) & 0xff;
	return tmp >> 16;
}

static unsigned dm9k_read_packet_status(int b_width, void __iomem *port, unsigned *status)
{
	unsigned rc;

	switch (b_width) {
	case IORESOURCE_MEM_8BIT:
		rc = dm9k_read_packet_status_8(port, status);
		break;
	case IORESOURCE_MEM_16BIT:
		rc = dm9k_read_packet_status_16(port, status);
		break;
	case IORESOURCE_MEM_32BIT:
		rc = dm9k_read_packet_status_32(port, status);
		break;
	default:
		rc = -EINVAL;
	}

	return rc;
}

static void dm9k_dump(int b_width, void __iomem *port, unsigned length)
{
	switch (b_width) {
	case IORESOURCE_MEM_8BIT:
		dm9k_dump_8(port, length);
		break;
	case IORESOURCE_MEM_16BIT:
		dm9k_dump_16(port, length);
		break;
	case IORESOURCE_MEM_32BIT:
		dm9k_dump_32(port, length);
		break;
	}
}

static void dm9k_rd(int b_width, void __iomem *port, void *dst, unsigned length)
{
	switch (b_width) {
	case IORESOURCE_MEM_8BIT:
		dm9k_rd_8(port, dst, length);
		break;
	case IORESOURCE_MEM_16BIT:
		dm9k_rd_16(port, dst, length);
		break;
	case IORESOURCE_MEM_32BIT:
		dm9k_rd_32(port, dst, length);
		break;
	}
}

static void dm9k_wd(int b_width, void __iomem *port, const void *src, int length)
{
	switch (b_width) {
	case IORESOURCE_MEM_8BIT:
		dm9k_wd_8(port, src, length);
		break;
	case IORESOURCE_MEM_16BIT:
		dm9k_wd_16(port, src, length);
		break;
	case IORESOURCE_MEM_32BIT:
		dm9k_wd_32(port, src, length);
		break;
	}
}

/* ----------------- end of data move functions -------------------------- */

static int dm9k_phy_read(struct mii_bus *bus, int addr, int reg)
{
	unsigned val;
	struct dm9k *priv = bus->priv;
	struct device_d *dev = &bus->dev;

	/* only internal phy supported by now, so show only one phy on miibus */
	if (addr != 0) {
		return 0xffff;
	}

	/* Fill the phyxcer register into REG_0C */
	dm9k_iow(priv, DM9K_EPAR, DM9K_PHY | reg);
	dm9k_iow(priv, DM9K_EPCR, 0xc);	/* Issue phyxcer read command */
	udelay(100);			/* Wait read complete */
	dm9k_iow(priv, DM9K_EPCR, 0x0);	/* Clear phyxcer read command */
	val = dm9k_ior(priv, DM9K_EPDRH);
	val <<= 8;
	val |= dm9k_ior(priv, DM9K_EPDRL);

	/* The read data keeps on REG_0D & REG_0E */
	dev_dbg(dev, "phy_read(%d): %d\n", reg, val);
	return val;
}

static int dm9k_phy_write(struct mii_bus *bus, int addr, int reg, u16 val)
{
	struct dm9k *priv = bus->priv;
	struct device_d *dev = &bus->dev;

	/* only internal phy supported by now, so show only one phy on miibus */
	if (addr != 0) {
		return 0;
	}

	/* Fill the phyxcer register into REG_0C */
	dm9k_iow(priv, DM9K_EPAR, DM9K_PHY | reg);

	/* Fill the written data into REG_0D & REG_0E */
	dm9k_iow(priv, DM9K_EPDRL, val);
	dm9k_iow(priv, DM9K_EPDRH, val >> 8);
	dm9k_iow(priv, DM9K_EPCR, 0xa);	/* Issue phyxcer write command */
	udelay(500);			/* Wait write complete */
	dm9k_iow(priv, DM9K_EPCR, 0x0);	/* Clear phyxcer write command */

	dev_dbg(dev, "phy_write(reg:%d, value:%d)\n", reg, val);

	return 0;
}

static int dm9k_check_id(struct dm9k *priv)
{
	struct device_d *dev = priv->miibus.parent;
	u32 id;
	char c;

	id = dm9k_ior(priv, DM9K_VIDL);
	id |= dm9k_ior(priv, DM9K_VIDH) << 8;
	id |= dm9k_ior(priv, DM9K_PIDL) << 16;
	id |= dm9k_ior(priv, DM9K_PIDH) << 24;

	if (id != DM9K_ID) {
		dev_err(dev, "dm9000 not found at 0x%p id: 0x%08x\n",
				priv->iobase, id);
		return -ENODEV;
	}

	id = dm9k_ior(priv, DM9K_CHIPR);
	dev_dbg(dev, "dm9000 revision 0x%02x\n", id);

	switch (id) {
	case CHIPR_DM9000A:
		c = 'A';
		break;
	case CHIPR_DM9000B:
		c = 'B';
		break;
	default:
		c = 'E';
	}
	dev_info(dev, "Found DM9000%c at i/o: 0x%p\n", c, priv->iobase);

	return 0;
}

static void dm9k_enable(struct dm9k *priv)
{
	/* only intern phy supported by now */
	dm9k_iow(priv, DM9K_NCR, 0x00);
	/* TX Polling clear */
	dm9k_iow(priv, DM9K_TCR, 0x00);
	/* Less 3Kb, 200us */
	dm9k_iow(priv, DM9K_BPTR, 0x3f);
	/* Flow Control : High/Low Water */
	dm9k_iow(priv, DM9K_FCTR, FCTR_HWOT(3) | FCTR_LWOT(8));
	/* SH FIXME: This looks strange! Flow Control */
	dm9k_iow(priv, DM9K_FCR, 0x00);
	/* Special Mode */
	dm9k_iow(priv, DM9K_SMCR, 0x00);
	/* clear TX status */
	dm9k_iow(priv, DM9K_NSR, NSR_WAKEST | NSR_TX2END | NSR_TX1END);
	/* Clear interrupt status */
	dm9k_iow(priv, DM9K_IMR, IMR_PAR);
	dm9k_iow(priv, DM9K_ISR, ISR_CLEAR_MASK);

	/* Activate DM9000 */
	dm9k_iow(priv, DM9K_GPCR, 0x01); /* Let GPIO0 output */
	dm9k_iow(priv, DM9K_GPR, 0x00);	/* Enable PHY */
	/* RX enable */
	dm9k_iow(priv, DM9K_RCR, RCR_DIS_LONG | RCR_DIS_CRC | RCR_RXEN);
	/* Enable TX/RX interrupt mask */
	dm9k_iow(priv, DM9K_IMR, IMR_PAR | IMR_PRM | IMR_PTM);
}

static void dm9k_reset(struct dm9k *priv)
{
	struct device_d *dev = priv->miibus.parent;

	dev_dbg(dev, "%s\n", __func__);

	/* Reset DM9000, see DM9000 Application Notes V1.22 Jun 11, 2004 page 29
	 * The essential point is that we have to do a double reset, and the
	 * instruction is to set LBK into MAC internal loopback mode.
	 */

	/* Make all GPIO pins outputs */
	dm9k_iow(priv, DM9K_GPCR, 0x0F);
	/* Power internal PHY by writing 0 to GPIO0 pin */
	dm9k_iow(priv, DM9K_GPR, 0);

	dm9k_iow(priv, DM9K_NCR, NCR_RST | NCR_MAC_LBK);
	udelay(100); /* Application note says at least 20 us */
	if (dm9k_ior(priv, DM9K_NCR) & NCR_RST)
		dev_err(dev, "dm9000 did not respond to first reset\n");

	dm9k_iow(priv, DM9K_NCR, 0);
	dm9k_iow(priv, DM9K_NCR, NCR_RST | NCR_MAC_LBK);
	udelay(100);

	if (dm9k_ior(priv, DM9K_NCR) & NCR_RST)
		dev_err(dev, "dm9000 did not respond to second reset\n");
}

static int dm9k_eth_open(struct eth_device *edev)
{
	struct dm9k *priv = (struct dm9k *)edev->priv;

	return phy_device_connect(edev, &priv->miibus, 0, NULL,
				 0, PHY_INTERFACE_MODE_NA);
}

static void dm9k_write_length(struct dm9k *priv, unsigned length)
{
	dm9k_iow(priv, DM9K_TXPLL, length);
	dm9k_iow(priv, DM9K_TXPLH, length >> 8);
}

static int dm9k_wait_for_trans_end(struct dm9k *priv)
{
	struct device_d *dev = priv->miibus.parent;
	static const uint64_t toffs = 1 * SECOND;
	uint8_t status;
	uint64_t start = get_time_ns();

	do {
		status = dm9k_ior(priv, DM9K_NSR);
		if (status & (NSR_TX1END | NSR_TX2END)) {
			dev_dbg(dev, "transmit done\n");
			return 0;
		}
		status = dm9k_ior(priv, DM9K_ISR);
		if (status & IMR_PTM) {
			/* Clear Tx bit in ISR */
			dm9k_iow(priv, DM9K_ISR, IMR_PTM);
			dev_dbg(dev, "transmit done\n");
			return 0;
		}
	} while (!is_timeout(start, toffs));

	return -ETIMEDOUT;
}

static int dm9k_eth_send(struct eth_device *edev, void *packet, int length)
{
	struct dm9k *priv = (struct dm9k *)edev->priv;
	struct device_d *dev = priv->miibus.parent;

	dev_dbg(dev, "%s: %d bytes\n", __func__, length);

	/* arm the Tx bit */
	dm9k_iow(priv, DM9K_ISR, IMR_PTM);

	/* Prepare for TX-data */
	writeb(DM9K_MWCMD, priv->iobase);

	/* Move the packet into the DM9k's TX RAM */
	dm9k_wd(priv->buswidth, priv->iodata, packet, length);

	/* Set TX length of the packet */
	dm9k_write_length(priv, length);

	/* Issue TX polling command */
	dm9k_iow(priv, DM9K_TCR, TCR_TXREQ); /* Cleared after TX complete */

	/* wait for end of transmission */
	return dm9k_wait_for_trans_end(priv);
}

static int dm9k_check_for_rx_packet(struct dm9k *priv)
{
	uint8_t status;
	struct device_d *dev = priv->miibus.parent;

	status = dm9k_ior(priv, DM9K_ISR);
	if (!(status & ISR_PR))
		return 0;	/* no packet */

	dev_dbg(dev, "Packet present\n");
	return 1; /* packet present */
}

static int dm9k_validate_entry(struct dm9k *priv)
{
	struct device_d *dev = priv->miibus.parent;

	uint8_t p_stat;
	/*
	 * setup read pointer to current packet
	 * but without address increment
	 */
	dm9k_ior(priv, DM9K_MRCMDX);

	/* read the entry's status according to the app note */
	p_stat = readb(priv->iodata) & 0x03;
	dev_dbg(dev, "%s packet status %02X\n", __func__, p_stat);

	switch (p_stat) {
	case DM9K_PKT_NONE: /* there is no packet (or the last in the chain) */
		return 0;

	case DM9K_PKT_ERR: /* chip in invalid state. Needs a software reset */
		dev_dbg(dev, "Confused chip.\n");
		dm9k_iow(priv, DM9K_RCR, 0x00);	/* Stop Device */
		dm9k_iow(priv, DM9K_ISR, 0x80);	/* Stop INT request */
		dm9k_reset(priv);
		dm9k_enable(priv);
		return 0;
	}

	return 1; /* entry is valid */
}

static int dm9k_eth_rx(struct eth_device *edev)
{
	struct dm9k *priv = (struct dm9k *)edev->priv;
	struct device_d *dev = edev->parent;
	unsigned rx_stat = 0, rx_len = 0;
	bool p_valid;

	if (dm9k_check_for_rx_packet(priv) == 0)
		return 0;	/* no data present */

	do {
		if (!dm9k_validate_entry(priv)) {
			dm9k_iow(priv, DM9K_ISR, ISR_PR); /* clear PR status latched in bit 0 */
			return 0;
		}

		/* assume this packet is valid */
		p_valid = true;

		 /* read with automatic address increment now */
		writeb(DM9K_MRCMD, priv->iobase);
		rx_len = dm9k_read_packet_status(priv->buswidth, priv->iodata, &rx_stat);
		if (rx_len < 0x40) {
			dev_dbg(dev, "Packet too short (%u bytes)\n", rx_len);
			p_valid = false;
		}

		/* validate packet */
		if (rx_stat & RSR_ERR_MASK) {
			if (rx_stat & RSR_FOE)
				dev_warn(dev, "rx fifo overflow error\n");
			if (rx_stat & RSR_CE)
				dev_warn(dev, "rx crc error\n");
			if (rx_stat & RSR_AE)
				dev_warn(dev, "rx Alignment Error error\n");
			if (rx_stat & RSR_PLE)
				dev_warn(dev, "rx Physical Layer Error error\n");
			if (rx_stat & RSR_RWTO)
				dev_warn(dev, "rx Receive Watchdog Time Out error\n");
			if (rx_stat & RSR_LCS)
				dev_warn(dev, "rx Late Collision Seen error\n");
			if (rx_stat & RSR_RF)
				dev_warn(dev, "rx length error (runt frame)\n");
			p_valid = false;
		}

		if (rx_len > DM9K_PKT_MAX) {
			dev_warn(dev, "rx length too big\n");
			/* discard packet */
			dm9k_dump(priv->buswidth, priv->iodata, rx_len);
			dm9k_reset(priv);
			dm9k_enable(priv);
			return 0;
		}

		if (p_valid == true) {
			dev_dbg(dev, "Receiving packet\n");
			dm9k_rd(priv->buswidth, priv->iodata, priv->pckt, rx_len);
			dev_dbg(dev, "passing %u bytes packet to upper layer\n", rx_len);
			net_receive(edev, priv->pckt, rx_len);
			return 0;
		} else {
			dev_dbg(dev, "Discarding packet\n");
			dm9k_dump(priv->buswidth, priv->iodata, rx_len); /* discard packet */
		}
	} while (1);

	return 0;
}


static void dm9k_eth_halt(struct eth_device *edev)
{
	struct dm9k *priv = (struct dm9k *)edev->priv;

	dm9k_iow(priv, DM9K_GPR, 0x01);	/* Power-Down PHY */
	dm9k_iow(priv, DM9K_IMR, 0x80);	/* Disable all interrupt */
	dm9k_iow(priv, DM9K_RCR, 0x00);	/* Disable RX */
}

static u16 read_srom_word(struct dm9k *priv, int offset)
{
	dm9k_iow(priv, DM9K_EPAR, offset);
	dm9k_iow(priv, DM9K_EPCR, 0x4);
	udelay(200);
	dm9k_iow(priv, DM9K_EPCR, 0x0);
	return (dm9k_ior(priv, DM9K_EPDRL) + (dm9k_ior(priv, DM9K_EPDRH) << 8));
}

static int dm9k_get_ethaddr(struct eth_device *edev, unsigned char *adr)
{
	struct dm9k *priv = (struct dm9k *)edev->priv;
	int i, oft;

	if (priv->srom) {
		for (i = 0; i < 3; i++)
			((u16 *) adr)[i] = read_srom_word(priv, i);
	} else {
		for (i = 0, oft = DM9K_PAR; i < 6; i++, oft++)
			adr[i] = dm9k_ior(priv, oft);
	}

	return 0;
}

static int dm9k_set_ethaddr(struct eth_device *edev, const unsigned char *adr)
{
	struct dm9k *priv = (struct dm9k *)edev->priv;
	int i, oft;

	for (i = 0, oft = DM9K_PAR; i < 6; i++, oft++)
		dm9k_iow(priv, oft, adr[i]);
	for (i = 0, oft = DM9K_MAR; i < 8; i++, oft++)
		dm9k_iow(priv, oft, 0xff);

	return 0;
}

static int dm9k_init_dev(struct eth_device *edev)
{
	return 0;
}

static int dm9000_parse_dt(struct device_d *dev, struct dm9k *priv)
{
	struct device_node *np = dev->device_node;
	uint32_t prop;

	if (!IS_ENABLED(CONFIG_OFDEVICE) || !np)
		return -ENODEV;

	if (of_property_read_bool(np, "davicom,no-eeprom")) {
		priv->srom = 0;
	} else {
		priv->srom = 1;
	}

	if (of_property_read_u32(np, "reg-io-width", &prop)) {
		/* Use 8-bit registers by default */
		prop = 1;
	}

	switch (prop) {
	case 1:
		priv->buswidth = IORESOURCE_MEM_8BIT;
		break;
	case 2:
		priv->buswidth = IORESOURCE_MEM_16BIT;
		break;
	case 4:
		priv->buswidth = IORESOURCE_MEM_32BIT;
		break;
	default:
		dev_err(dev, "Wrong io resource size\n");
		return -EINVAL;
	}

	return 0;
}

static int dm9000_parse_pdata(struct device_d *dev, struct dm9k *priv)
{
	struct dm9000_platform_data *pdata = dev->platform_data;

	priv->srom = pdata->srom;

	priv->buswidth = dev->resource[0].flags & IORESOURCE_MEM_TYPE_MASK;

	return 0;
}

static int dm9k_probe(struct device_d *dev)
{
	struct resource *iores;
	unsigned io_mode;
	struct eth_device *edev;
	struct dm9k *priv;
	int ret;

	if (dev->num_resources < 2) {
		dev_err(dev, "Need 2 resources base and data");
		return -ENODEV;
	}

	edev = xzalloc(sizeof(struct eth_device) + sizeof(struct dm9k));
	edev->priv = (struct dm9k *)(edev + 1);
	priv = edev->priv;

	if (dev->platform_data) {
		ret = dm9000_parse_pdata(dev, priv);
	} else {
		ret = dm9000_parse_dt(dev, priv);
	}

	if (ret)
		goto err;

	iores = dev_request_mem_resource(dev, 1);
	if (IS_ERR(iores)) {
		ret = PTR_ERR(iores);
		goto err;
	}
	priv->iodata = IOMEM(iores->start);

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores)) {
		ret = PTR_ERR(iores);
		goto err;
	}
	priv->iobase = IOMEM(iores->start);

	edev->init = dm9k_init_dev;
	edev->open = dm9k_eth_open;
	edev->send = dm9k_eth_send;
	edev->recv = dm9k_eth_rx;
	edev->halt = dm9k_eth_halt;
	edev->set_ethaddr = dm9k_set_ethaddr;
	edev->get_ethaddr = dm9k_get_ethaddr;
	edev->parent = dev;

	priv->miibus.read = dm9k_phy_read;
	priv->miibus.write = dm9k_phy_write;
	priv->miibus.priv = priv;
	priv->miibus.parent = dev;

	/* RESET device */
	dm9k_reset(priv);
	if (dm9k_check_id(priv)) {
		ret = -ENODEV;
		goto err;
	}

	io_mode = dm9k_ior(priv, DM9K_ISR) >> 6;
	switch (io_mode) {
	case 0:
		dev_dbg(dev, "16 bit data bus\n");
		if (priv->buswidth != IORESOURCE_MEM_16BIT)
			dev_err(dev,
			"Wrong databus width defined at compile time\n");
		break;
	case 1:
		dev_dbg(dev, "32 bit data bus\n");
		if (priv->buswidth != IORESOURCE_MEM_32BIT)
			dev_err(dev,
			"Wrong databus width defined at compile time\n");
		break;
	case 2:
		dev_dbg(dev, "8 bit data bus\n");
		if (priv->buswidth != IORESOURCE_MEM_8BIT)
			dev_err(dev,
			"Wrong databus width defined at compile time\n");
		break;
	default:
		dev_warn(dev, "Unknown data width\n");
	}

	dm9k_enable(priv);

	mdiobus_register(&priv->miibus);
	eth_register(edev);

	return 0;

err:
	free(edev);

	return ret;
}

static struct of_device_id dm9000_of_matches[] = {
	{ .compatible = "davicom,dm9000", },
	{ /* sentinel */ }
};

static struct driver_d dm9k_driver = {
	.name  = "dm9000",
	.probe = dm9k_probe,
	.of_compatible = DRV_OF_COMPAT(dm9000_of_matches),
};
device_platform_driver(dm9k_driver);
