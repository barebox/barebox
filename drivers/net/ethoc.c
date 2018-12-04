// SPDX-License-Identifier: GPL-2.0-only
/*
 * OpenCores 10/100 Mbps Ethernet driver
 *
 * Copyright (C) 2007-2008 Avionic Design Development GmbH
 * Copyright (C) 2008-2009 Avionic Design GmbH
 * Copyright (C) 2013 Beniamino Galvani <b.galvani@gmail.com>
 *
 * Originally written by Thierry Reding <thierry.reding@avionic-design.de>
 */

#include <common.h>
#include <net.h>
#include <init.h>
#include <io.h>
#include <asm/cache.h>
#include <linux/err.h>

/* register offsets */
#define	MODER		0x00
#define	INT_SOURCE	0x04
#define	INT_MASK	0x08
#define	IPGT		0x0c
#define	IPGR1		0x10
#define	IPGR2		0x14
#define	PACKETLEN	0x18
#define	COLLCONF	0x1c
#define	TX_BD_NUM	0x20
#define	CTRLMODER	0x24
#define	MIIMODER	0x28
#define	MIICOMMAND	0x2c
#define	MIIADDRESS	0x30
#define	MIITX_DATA	0x34
#define	MIIRX_DATA	0x38
#define	MIISTATUS	0x3c
#define	MAC_ADDR0	0x40
#define	MAC_ADDR1	0x44
#define	ETH_HASH0	0x48
#define	ETH_HASH1	0x4c
#define	ETH_TXCTRL	0x50

/* mode register */
#define	MODER_RXEN	(1 <<  0) /* receive enable */
#define	MODER_TXEN	(1 <<  1) /* transmit enable */
#define	MODER_NOPRE	(1 <<  2) /* no preamble */
#define	MODER_BRO	(1 <<  3) /* broadcast address */
#define	MODER_IAM	(1 <<  4) /* individual address mode */
#define	MODER_PRO	(1 <<  5) /* promiscuous mode */
#define	MODER_IFG	(1 <<  6) /* interframe gap for incoming frames */
#define	MODER_LOOP	(1 <<  7) /* loopback */
#define	MODER_NBO	(1 <<  8) /* no back-off */
#define	MODER_EDE	(1 <<  9) /* excess defer enable */
#define	MODER_FULLD	(1 << 10) /* full duplex */
#define	MODER_RESET	(1 << 11) /* FIXME: reset (undocumented) */
#define	MODER_DCRC	(1 << 12) /* delayed CRC enable */
#define	MODER_CRC	(1 << 13) /* CRC enable */
#define	MODER_HUGE	(1 << 14) /* huge packets enable */
#define	MODER_PAD	(1 << 15) /* padding enabled */
#define	MODER_RSM	(1 << 16) /* receive small packets */

/* interrupt source and mask registers */
#define	INT_MASK_TXF	(1 << 0) /* transmit frame */
#define	INT_MASK_TXE	(1 << 1) /* transmit error */
#define	INT_MASK_RXF	(1 << 2) /* receive frame */
#define	INT_MASK_RXE	(1 << 3) /* receive error */
#define	INT_MASK_BUSY	(1 << 4)
#define	INT_MASK_TXC	(1 << 5) /* transmit control frame */
#define	INT_MASK_RXC	(1 << 6) /* receive control frame */

#define	INT_MASK_TX	(INT_MASK_TXF | INT_MASK_TXE)
#define	INT_MASK_RX	(INT_MASK_RXF | INT_MASK_RXE)

#define	INT_MASK_ALL ( \
		INT_MASK_TXF | INT_MASK_TXE | \
		INT_MASK_RXF | INT_MASK_RXE | \
		INT_MASK_TXC | INT_MASK_RXC | \
		INT_MASK_BUSY \
	)

/* packet length register */
#define	PACKETLEN_MIN(min)		(((min) & 0xffff) << 16)
#define	PACKETLEN_MAX(max)		(((max) & 0xffff) <<  0)
#define	PACKETLEN_MIN_MAX(min, max)	(PACKETLEN_MIN(min) | \
					PACKETLEN_MAX(max))

/* transmit buffer number register */
#define	TX_BD_NUM_VAL(x)	(((x) <= 0x80) ? (x) : 0x80)

/* control module mode register */
#define	CTRLMODER_PASSALL	(1 << 0) /* pass all receive frames */
#define	CTRLMODER_RXFLOW	(1 << 1) /* receive control flow */
#define	CTRLMODER_TXFLOW	(1 << 2) /* transmit control flow */

/* MII mode register */
#define	MIIMODER_CLKDIV(x)	((x) & 0xfe) /* needs to be an even number */
#define	MIIMODER_NOPRE		(1 << 8) /* no preamble */

/* MII command register */
#define	MIICOMMAND_SCAN		(1 << 0) /* scan status */
#define	MIICOMMAND_READ		(1 << 1) /* read status */
#define	MIICOMMAND_WRITE	(1 << 2) /* write control data */

/* MII address register */
#define	MIIADDRESS_FIAD(x)		(((x) & 0x1f) << 0)
#define	MIIADDRESS_RGAD(x)		(((x) & 0x1f) << 8)
#define	MIIADDRESS_ADDR(phy, reg)	(MIIADDRESS_FIAD(phy) | \
					MIIADDRESS_RGAD(reg))

/* MII transmit data register */
#define	MIITX_DATA_VAL(x)	((x) & 0xffff)

/* MII receive data register */
#define	MIIRX_DATA_VAL(x)	((x) & 0xffff)

/* MII status register */
#define	MIISTATUS_LINKFAIL	(1 << 0)
#define	MIISTATUS_BUSY		(1 << 1)
#define	MIISTATUS_INVALID	(1 << 2)

/* TX buffer descriptor */
#define	TX_BD_CS		(1 <<  0) /* carrier sense lost */
#define	TX_BD_DF		(1 <<  1) /* defer indication */
#define	TX_BD_LC		(1 <<  2) /* late collision */
#define	TX_BD_RL		(1 <<  3) /* retransmission limit */
#define	TX_BD_RETRY_MASK	(0x00f0)
#define	TX_BD_RETRY(x)		(((x) & 0x00f0) >>  4)
#define	TX_BD_UR		(1 <<  8) /* transmitter underrun */
#define	TX_BD_CRC		(1 << 11) /* TX CRC enable */
#define	TX_BD_PAD		(1 << 12) /* pad enable for short packets */
#define	TX_BD_WRAP		(1 << 13)
#define	TX_BD_IRQ		(1 << 14) /* interrupt request enable */
#define	TX_BD_READY		(1 << 15) /* TX buffer ready */
#define	TX_BD_LEN(x)		(((x) & 0xffff) << 16)
#define	TX_BD_LEN_MASK		(0xffff << 16)

#define	TX_BD_STATS		(TX_BD_CS | TX_BD_DF | TX_BD_LC | \
				TX_BD_RL | TX_BD_RETRY_MASK | TX_BD_UR)

/* RX buffer descriptor */
#define	RX_BD_LC	(1 <<  0) /* late collision */
#define	RX_BD_CRC	(1 <<  1) /* RX CRC error */
#define	RX_BD_SF	(1 <<  2) /* short frame */
#define	RX_BD_TL	(1 <<  3) /* too long */
#define	RX_BD_DN	(1 <<  4) /* dribble nibble */
#define	RX_BD_IS	(1 <<  5) /* invalid symbol */
#define	RX_BD_OR	(1 <<  6) /* receiver overrun */
#define	RX_BD_MISS	(1 <<  7)
#define	RX_BD_CF	(1 <<  8) /* control frame */
#define	RX_BD_WRAP	(1 << 13)
#define	RX_BD_IRQ	(1 << 14) /* interrupt request enable */
#define	RX_BD_EMPTY	(1 << 15)
#define	RX_BD_LEN(x)	(((x) & 0xffff) << 16)

#define	RX_BD_STATS	(RX_BD_LC | RX_BD_CRC | RX_BD_SF | RX_BD_TL | \
			RX_BD_DN | RX_BD_IS | RX_BD_OR | RX_BD_MISS)

#define	ETHOC_BUFSIZ		1536
#define	ETHOC_ZLEN		64
#define	ETHOC_BD_BASE		0x400

/**
 * struct ethoc - driver-private device structure
 * @iobase:	pointer to I/O memory region
 * @num_tx:	number of send buffers
 * @cur_tx:	last send buffer written
 * @dty_tx:	last buffer actually sent
 * @num_rx:	number of receive buffers
 * @cur_rx:	current receive buffer
 */
struct ethoc {
	void __iomem *iobase;

	u32 num_tx;
	u32 cur_tx;
	u32 dty_tx;

	u32 num_rx;
	u32 cur_rx;

	struct mii_bus miibus;
};

/**
 * struct ethoc_bd - buffer descriptor
 * @stat:	buffer statistics
 * @addr:	physical memory address
 */
struct ethoc_bd {
	u32 stat;
	u32 addr;
};

static inline u32 ethoc_read(struct ethoc *dev, loff_t offset)
{
	return ioread32be(dev->iobase + offset);
}

static inline void ethoc_write(struct ethoc *dev, loff_t offset, u32 data)
{
	iowrite32be(data, dev->iobase + offset);
}

static inline void ethoc_read_bd(struct ethoc *dev, int index,
				 struct ethoc_bd *bd)
{
	loff_t offset = ETHOC_BD_BASE + (index * sizeof(struct ethoc_bd));
	bd->stat = ethoc_read(dev, offset + 0);
	bd->addr = ethoc_read(dev, offset + 4);
}

static inline void ethoc_write_bd(struct ethoc *dev, int index,
				  const struct ethoc_bd *bd)
{
	loff_t offset = ETHOC_BD_BASE + (index * sizeof(struct ethoc_bd));
	ethoc_write(dev, offset + 0, bd->stat);
	ethoc_write(dev, offset + 4, bd->addr);
}

static inline void ethoc_ack_irq(struct ethoc *dev, u32 mask)
{
	ethoc_write(dev, INT_SOURCE, mask);
}

static inline void ethoc_enable_rx_and_tx(struct ethoc *dev)
{
	u32 mode = ethoc_read(dev, MODER);
	mode |= MODER_RXEN | MODER_TXEN;
	ethoc_write(dev, MODER, mode);
}

static inline void ethoc_disable_rx_and_tx(struct ethoc *dev)
{
	u32 mode = ethoc_read(dev, MODER);
	mode &= ~(MODER_RXEN | MODER_TXEN);
	ethoc_write(dev, MODER, mode);
}

static int ethoc_init_ring(struct ethoc *dev)
{
	struct ethoc_bd bd;
	int i;

	dev->num_tx = 1;
	dev->num_rx = PKTBUFSRX;

	dev->cur_tx = 0;
	dev->dty_tx = 0;
	dev->cur_rx = 0;

	ethoc_write(dev, TX_BD_NUM, dev->num_tx);

	/* setup transmission buffers */
	bd.addr = 0;
	bd.stat = TX_BD_IRQ | TX_BD_CRC;

	for (i = 0; i < dev->num_tx; i++) {
		if (i == dev->num_tx - 1)
			bd.stat |= TX_BD_WRAP;

		ethoc_write_bd(dev, i, &bd);
	}

	bd.stat = RX_BD_EMPTY | RX_BD_IRQ;

	for (i = 0; i < dev->num_rx; i++) {
		if (i == dev->num_rx - 1)
			bd.stat |= RX_BD_WRAP;

		bd.addr = (u32)NetRxPackets[i];
		ethoc_write_bd(dev, dev->num_tx + i, &bd);

		flush_dcache_range(bd.addr, bd.addr + PKTSIZE);
	}

	return 0;
}

static int ethoc_reset(struct ethoc *dev)
{
	u32 mode;

	/* TODO: reset controller? */

	ethoc_disable_rx_and_tx(dev);

	/* TODO: setup registers */

	/* enable FCS generation and automatic padding */
	mode = ethoc_read(dev, MODER);
	mode |= MODER_CRC | MODER_PAD;
	ethoc_write(dev, MODER, mode);

	/* set full-duplex mode */
	mode = ethoc_read(dev, MODER);
	mode |= MODER_FULLD;
	ethoc_write(dev, MODER, mode);
	ethoc_write(dev, IPGT, 0x15);

	ethoc_write(dev, PACKETLEN, PACKETLEN_MIN_MAX(64, PKTSIZE));

	ethoc_ack_irq(dev, INT_MASK_ALL);
	ethoc_enable_rx_and_tx(dev);
	return 0;
}

static unsigned int ethoc_update_rx_stats(struct eth_device *edev,
					  struct ethoc_bd *bd)
{
	unsigned int ret = 0;

	if (bd->stat & RX_BD_TL) {
		dev_err(&edev->dev, "RX: frame too long\n");
		ret++;
	}

	if (bd->stat & RX_BD_SF) {
		dev_err(&edev->dev, "RX: frame too short\n");
		ret++;
	}

	if (bd->stat & RX_BD_DN)
		dev_err(&edev->dev, "RX: dribble nibble\n");

	if (bd->stat & RX_BD_CRC) {
		dev_err(&edev->dev, "RX: wrong CRC\n");
		ret++;
	}

	if (bd->stat & RX_BD_OR) {
		dev_err(&edev->dev, "RX: overrun\n");
		ret++;
	}

	if (bd->stat & RX_BD_LC) {
		dev_err(&edev->dev, "RX: late collision\n");
		ret++;
	}

	return ret;
}

static int ethoc_rx(struct eth_device *edev, int limit)
{
	struct ethoc *priv = edev->priv;
	int count;

	for (count = 0; count < limit; ++count) {
		unsigned int entry;
		struct ethoc_bd bd;

		entry = priv->num_tx + priv->cur_rx;
		ethoc_read_bd(priv, entry, &bd);
		if (bd.stat & RX_BD_EMPTY) {
			ethoc_ack_irq(priv, INT_MASK_RX);
			/* If packet (interrupt) came in between checking
			 * BD_EMTPY and clearing the interrupt source, then we
			 * risk missing the packet as the RX interrupt won't
			 * trigger right away when we reenable it; hence, check
			 * BD_EMTPY here again to make sure there isn't such a
			 * packet waiting for us...
			 */
			ethoc_read_bd(priv, entry, &bd);
			if (bd.stat & RX_BD_EMPTY)
				break;
		}

		if (ethoc_update_rx_stats(edev, &bd) == 0) {
			int size = bd.stat >> 16;

			size -= 4; /* strip the CRC */
			invalidate_dcache_range(bd.addr, bd.addr + PKTSIZE);
			net_receive(edev, (unsigned char *)bd.addr, size);
		}

		/* clear the buffer descriptor so it can be reused */
		bd.stat &= ~RX_BD_STATS;
		bd.stat |=  RX_BD_EMPTY;
		ethoc_write_bd(priv, entry, &bd);
		if (++priv->cur_rx == priv->num_rx)
			priv->cur_rx = 0;
	}

	return count;
}

static int ethoc_recv_packet(struct eth_device *edev)
{
	struct ethoc *priv = edev->priv;

	if (ethoc_read(priv, INT_SOURCE) & INT_MASK_RX)
		return ethoc_rx(edev, PKTBUFSRX);

	return 0;
}

static int ethoc_init_dev(struct eth_device *edev)
{
	return 0;
}


static int ethoc_open(struct eth_device *edev)
{
	struct ethoc *dev = edev->priv;

	ethoc_init_ring(dev);
	ethoc_reset(dev);

	return 0;
}

static void ethoc_halt(struct eth_device *edev)
{
	ethoc_disable_rx_and_tx(edev->priv);
}

static int ethoc_get_ethaddr(struct eth_device *edev, unsigned char *mac)
{
	struct ethoc *priv = edev->priv;
	u32 reg;

	reg = ethoc_read(priv, MAC_ADDR0);
	mac[2] = (reg >> 24) & 0xff;
	mac[3] = (reg >> 16) & 0xff;
	mac[4] = (reg >>  8) & 0xff;
	mac[5] = (reg >>  0) & 0xff;

	reg = ethoc_read(priv, MAC_ADDR1);
	mac[0] = (reg >>  8) & 0xff;
	mac[1] = (reg >>  0) & 0xff;

	return 0;
}

static int ethoc_set_ethaddr(struct eth_device *edev, const unsigned char *mac)
{
	struct ethoc *dev = edev->priv;

	ethoc_write(dev, MAC_ADDR0, (mac[2] << 24) | (mac[3] << 16) |
		    (mac[4] << 8) | (mac[5] << 0));
	ethoc_write(dev, MAC_ADDR1, (mac[0] << 8) | (mac[1] << 0));

	return 0;
}

static int ethoc_send_packet(struct eth_device *edev, void *packet, int length)
{
	struct ethoc *priv = edev->priv;
	struct ethoc_bd bd;
	u32 entry;
	u32 pending;
	u64 start;

	entry = priv->cur_tx % priv->num_tx;
	ethoc_read_bd(priv, entry, &bd);
	if (unlikely(length < ETHOC_ZLEN))
		bd.stat |= TX_BD_PAD;
	else
		bd.stat &= ~TX_BD_PAD;
	bd.addr = (u32)packet;

	flush_dcache_range(bd.addr, bd.addr + length);
	bd.stat &= ~(TX_BD_STATS | TX_BD_LEN_MASK);
	bd.stat |= TX_BD_LEN(length);
	ethoc_write_bd(priv, entry, &bd);

	bd.stat |= TX_BD_READY;
	ethoc_write_bd(priv, entry, &bd);

	start = get_time_ns();
	do {
		pending = ethoc_read(priv, INT_SOURCE);
		ethoc_ack_irq(priv, pending & INT_MASK_TX);

		if (is_timeout(start, 200 * MSECOND)) {
			dev_err(&edev->dev, "TX timeout\n");
			return -ETIMEDOUT;
		}
	} while (!(pending & INT_MASK_TX));

	return 0;
}

static int ethoc_mdio_read(struct mii_bus *bus, int phy, int reg)
{
	struct ethoc *priv = bus->priv;
	u64 start;
	u32 data;

	ethoc_write(priv, MIIADDRESS, MIIADDRESS_ADDR(phy, reg));
	ethoc_write(priv, MIICOMMAND, MIICOMMAND_READ);

	start = get_time_ns();
	while (ethoc_read(priv, MIISTATUS) & MIISTATUS_BUSY) {
		if (is_timeout(start, 2 * MSECOND)) {
			dev_err(bus->parent, "PHY command timeout\n");
			return -EBUSY;
		}
	}

	data = ethoc_read(priv, MIIRX_DATA);

	/* reset MII command register */
	ethoc_write(priv, MIICOMMAND, 0);

	return data;
}

static int ethoc_mdio_write(struct mii_bus *bus, int phy, int reg, u16 val)
{
	struct ethoc *priv = bus->priv;
	u64 start;

	ethoc_write(priv, MIIADDRESS, MIIADDRESS_ADDR(phy, reg));
	ethoc_write(priv, MIITX_DATA, val);
	ethoc_write(priv, MIICOMMAND, MIICOMMAND_WRITE);

	start = get_time_ns();
	while (ethoc_read(priv, MIISTATUS) & MIISTATUS_BUSY) {
		if (is_timeout(start, 2 * MSECOND)) {
			dev_err(bus->parent, "PHY command timeout\n");
			return -EBUSY;
		}
	}

	/* reset MII command register */
	ethoc_write(priv, MIICOMMAND, 0);

	return 0;
}

static int ethoc_probe(struct device_d *dev)
{
	struct resource *iores;
	struct eth_device *edev;
	struct ethoc *priv;

	edev = xzalloc(sizeof(struct eth_device) +
		       sizeof(struct ethoc));
	edev->priv = (struct ethoc *)(edev + 1);

	priv = edev->priv;
	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	priv->iobase = IOMEM(iores->start);

	priv->miibus.read = ethoc_mdio_read;
	priv->miibus.write = ethoc_mdio_write;
	priv->miibus.priv = priv;
	priv->miibus.parent = dev;

	edev->init = ethoc_init_dev;
	edev->open = ethoc_open;
	edev->send = ethoc_send_packet;
	edev->recv = ethoc_recv_packet;
	edev->halt = ethoc_halt;

	edev->get_ethaddr = ethoc_get_ethaddr;
	edev->set_ethaddr = ethoc_set_ethaddr;
	edev->parent = dev;

	mdiobus_register(&priv->miibus);

	eth_register(edev);

	return 0;
}

static struct of_device_id ethoc_dt_ids[] = {
	{ .compatible = "opencores,ethoc", },
	{ }
};

static struct driver_d ethoc_driver = {
	.name  = "ethoc",
	.probe = ethoc_probe,
	.of_compatible = DRV_OF_COMPAT(ethoc_dt_ids),
};
device_platform_driver(ethoc_driver);
