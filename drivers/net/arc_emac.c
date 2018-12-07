// SPDX-License-Identifier: GPL-2.0-only
/*
 * Driver for ARC EMAC Ethernet controller
 *
 * Copyright (C) 2014 Beniamino Galvani <b.galvani@gmail.com>
 *
 * Based on Linux kernel driver, which is:
 *  Copyright (C) 2004-2013 Synopsys, Inc. (www.synopsys.com)
 */

#include <clock.h>
#include <common.h>
#include <dma.h>
#include <net.h>
#include <io.h>
#include <init.h>
#include <linux/err.h>
#include <linux/clk.h>

/* ARC EMAC register set combines entries for MAC and MDIO */
enum {
	R_ID = 0,
	R_STATUS,
	R_ENABLE,
	R_CTRL,
	R_POLLRATE,
	R_RXERR,
	R_MISS,
	R_TX_RING,
	R_RX_RING,
	R_ADDRL,
	R_ADDRH,
	R_LAFL,
	R_LAFH,
	R_MDIO,
};

/* STATUS and ENABLE Register bit masks */
#define TXINT_MASK	(1<<0)	/* Transmit interrupt */
#define RXINT_MASK	(1<<1)	/* Receive interrupt */
#define ERR_MASK	(1<<2)	/* Error interrupt */
#define TXCH_MASK	(1<<3)	/* Transmit chaining error interrupt */
#define MSER_MASK	(1<<4)	/* Missed packet counter error */
#define RXCR_MASK	(1<<8)	/* RXCRCERR counter rolled over  */
#define RXFR_MASK	(1<<9)	/* RXFRAMEERR counter rolled over */
#define RXFL_MASK	(1<<10)	/* RXOFLOWERR counter rolled over */
#define MDIO_MASK	(1<<12)	/* MDIO complete interrupt */
#define TXPL_MASK	(1<<31)	/* Force polling of BD by EMAC */

/* CONTROL Register bit masks */
#define EN_MASK		(1<<0)	/* VMAC enable */
#define TXRN_MASK	(1<<3)	/* TX enable */
#define RXRN_MASK	(1<<4)	/* RX enable */
#define DSBC_MASK	(1<<8)	/* Disable receive broadcast */
#define ENFL_MASK	(1<<10)	/* Enable Full-duplex */
#define PROM_MASK	(1<<11)	/* Promiscuous mode */

/* Buffer descriptor INFO bit masks */
#define OWN_MASK	(1<<31)	/* 0-CPU owns buffer, 1-EMAC owns buffer */
#define FIRST_MASK	(1<<16)	/* First buffer in chain */
#define LAST_MASK	(1<<17)	/* Last buffer in chain */
#define LEN_MASK	0x000007FF	/* last 11 bits */
#define CRLS		(1<<21)
#define DEFR		(1<<22)
#define DROP		(1<<23)
#define RTRY		(1<<24)
#define LTCL		(1<<28)
#define UFLO		(1<<29)

#define FIRST_OR_LAST_MASK	(FIRST_MASK | LAST_MASK)

#define FOR_EMAC	OWN_MASK
#define FOR_CPU		0

#define EMAC_ZLEN	64

/**
 * struct arc_emac_priv - Storage of EMAC's private information.
 * @bus:	Pointer to the current MII bus.
 * @regs:	Base address of EMAC memory-mapped control registers.
 * @rxbd:	Pointer to Rx BD ring.
 * @txbd:	Pointer to Tx BD ring.
 * @rxbuf:	Buffers for received packets.
 * @txbd_curr:	Index of Tx BD to use on the next "ndo_start_xmit".
 * @last_rx_bd:	Index of the last Rx BD we've got from EMAC.
 */
struct arc_emac_priv {
	struct mii_bus *bus;
	void __iomem *regs;
	struct arc_emac_bd *rxbd;
	struct arc_emac_bd *txbd;
	u8 *rxbuf;
	unsigned int txbd_curr;
	unsigned int last_rx_bd;
	struct clk *clk;
	struct clk *refclk;
};

/**
 * struct arc_emac_bd - EMAC buffer descriptor (BD).
 *
 * @info:	Contains status information on the buffer itself.
 * @data:	32-bit byte addressable pointer to the packet data.
 */
struct arc_emac_bd {
	__le32 info;
	dma_addr_t data;
};

/* Number of Rx/Tx BD's */
#define RX_BD_NUM	16
#define TX_BD_NUM	1

#define RX_RING_SZ	(RX_BD_NUM * sizeof(struct arc_emac_bd))
#define TX_RING_SZ	(TX_BD_NUM * sizeof(struct arc_emac_bd))

/**
 * arc_reg_set - Sets EMAC register with provided value.
 * @priv:	Pointer to ARC EMAC private data structure.
 * @reg:	Register offset from base address.
 * @value:	Value to set in register.
 */
static inline void arc_reg_set(struct arc_emac_priv *priv, int reg, int value)
{
	writel(value, priv->regs + reg * sizeof(int));
}

/**
 * arc_reg_get - Gets value of specified EMAC register.
 * @priv:	Pointer to ARC EMAC private data structure.
 * @reg:	Register offset from base address.
 *
 * returns:	Value of requested register.
 */
static inline unsigned int arc_reg_get(struct arc_emac_priv *priv, int reg)
{
	return readl(priv->regs + reg * sizeof(int));
}

/**
 * arc_reg_or - Applies mask to specified EMAC register - ("reg" | "mask").
 * @priv:	Pointer to ARC EMAC private data structure.
 * @reg:	Register offset from base address.
 * @mask:	Mask to apply to specified register.
 *
 * This function reads initial register value, then applies provided mask
 * to it and then writes register back.
 */
static inline void arc_reg_or(struct arc_emac_priv *priv, int reg, int mask)
{
	unsigned int value = arc_reg_get(priv, reg);
	arc_reg_set(priv, reg, value | mask);
}

/**
 * arc_reg_clr - Applies mask to specified EMAC register - ("reg" & ~"mask").
 * @priv:	Pointer to ARC EMAC private data structure.
 * @reg:	Register offset from base address.
 * @mask:	Mask to apply to specified register.
 *
 * This function reads initial register value, then applies provided mask
 * to it and then writes register back.
 */
static inline void arc_reg_clr(struct arc_emac_priv *priv, int reg, int mask)
{
	unsigned int value = arc_reg_get(priv, reg);
	arc_reg_set(priv, reg, value & ~mask);
}

static int arc_emac_init(struct eth_device *edev)
{
	return 0;
}

static int arc_emac_open(struct eth_device *edev)
{
	struct arc_emac_priv *priv = edev->priv;
	void *rxbuf;
	int ret, i;

	priv->last_rx_bd = 0;
	rxbuf = priv->rxbuf;

	/* Allocate and set buffers for Rx BD's */
	for (i = 0; i < RX_BD_NUM; i++) {
		unsigned int *last_rx_bd = &priv->last_rx_bd;
		struct arc_emac_bd *rxbd = &priv->rxbd[*last_rx_bd];

		rxbd->data = cpu_to_le32(rxbuf);

		/* Return ownership to EMAC */
		dma_sync_single_for_device((unsigned long)rxbuf, PKTSIZE,
					   DMA_FROM_DEVICE);
		rxbd->info = cpu_to_le32(FOR_EMAC | PKTSIZE);

		*last_rx_bd = (*last_rx_bd + 1) % RX_BD_NUM;
		rxbuf += PKTSIZE;
	}

	/* Clean Tx BD's */
	memset(priv->txbd, 0, TX_RING_SZ);

	/* Initialize logical address filter */
	arc_reg_set(priv, R_LAFL, 0x0);
	arc_reg_set(priv, R_LAFH, 0x0);

	/* Set BD ring pointers for device side */
	arc_reg_set(priv, R_RX_RING, (unsigned int)priv->rxbd);
	arc_reg_set(priv, R_TX_RING, (unsigned int)priv->txbd);

	/* Set CONTROL */
	arc_reg_set(priv, R_CTRL,
		     (RX_BD_NUM << 24) |	/* RX BD table length */
		     (TX_BD_NUM << 16) |	/* TX BD table length */
		     TXRN_MASK | RXRN_MASK);

	/* Enable EMAC */
	arc_reg_or(priv, R_CTRL, EN_MASK);

	ret = phy_device_connect(edev, priv->bus, -1, NULL, 0,
				 PHY_INTERFACE_MODE_NA);
	if (ret)
		return ret;

	return 0;
}

static int arc_emac_send(struct eth_device *edev, void *data, int length)
{
	struct arc_emac_priv *priv = edev->priv;
	struct arc_emac_bd *bd = &priv->txbd[priv->txbd_curr];
	char txbuf[EMAC_ZLEN];
	int ret;

	/* Pad short frames to minimum length */
	if (length < EMAC_ZLEN) {
		memcpy(txbuf, data, length);
		memset(txbuf + length, 0, EMAC_ZLEN - length);
		data = txbuf;
		length = EMAC_ZLEN;
	}

	dma_sync_single_for_device((unsigned long)data, length, DMA_TO_DEVICE);

	bd->data = cpu_to_le32(data);
	bd->info = cpu_to_le32(FOR_EMAC | FIRST_OR_LAST_MASK | length);
	arc_reg_set(priv, R_STATUS, TXPL_MASK);

	ret = wait_on_timeout(20 * MSECOND,
			      (arc_reg_get(priv, R_STATUS) & TXINT_MASK) != 0);

	dma_sync_single_for_cpu((unsigned long)data, length, DMA_TO_DEVICE);

	if (ret) {
		dev_err(&edev->dev, "transmit timeout\n");
		return ret;
	}

	arc_reg_set(priv, R_STATUS, TXINT_MASK);

	priv->txbd_curr++;
	priv->txbd_curr %= TX_BD_NUM;

	return 0;
}

static int arc_emac_recv(struct eth_device *edev)
{
	struct arc_emac_priv *priv = edev->priv;
	unsigned int work_done;

	for (work_done = 0; work_done < RX_BD_NUM; work_done++) {
		unsigned int *last_rx_bd = &priv->last_rx_bd;
		struct arc_emac_bd *rxbd = &priv->rxbd[*last_rx_bd];
		unsigned int pktlen, info = le32_to_cpu(rxbd->info);

		if (unlikely((info & OWN_MASK) == FOR_EMAC))
			break;

		/*
		 * Make a note that we saw a packet at this BD.
		 * So next time, driver starts from this + 1
		 */
		*last_rx_bd = (*last_rx_bd + 1) % RX_BD_NUM;

		if (unlikely((info & FIRST_OR_LAST_MASK) !=
			     FIRST_OR_LAST_MASK)) {
			/*
			 * We pre-allocate buffers of MTU size so incoming
			 * packets won't be split/chained.
			 */
			printk(KERN_DEBUG "incomplete packet received\n");

			/* Return ownership to EMAC */
			continue;
		}

		pktlen = info & LEN_MASK;

		dma_sync_single_for_cpu((unsigned long)rxbd->data, pktlen,
					DMA_FROM_DEVICE);

		net_receive(edev, (unsigned char *)rxbd->data, pktlen);

		dma_sync_single_for_device((unsigned long)rxbd->data, pktlen,
					   DMA_FROM_DEVICE);

		rxbd->info = cpu_to_le32(FOR_EMAC | PKTSIZE);
	}

	return work_done;
}

static void arc_emac_halt(struct eth_device *edev)
{
	struct arc_emac_priv *priv = edev->priv;

	/* Disable EMAC */
	arc_reg_clr(priv, R_CTRL, EN_MASK);
}

static int arc_emac_get_ethaddr(struct eth_device *edev, unsigned char *mac)
{
	return -1;
}

static int arc_emac_set_ethaddr(struct eth_device *edev, const unsigned char *mac)
{
	struct arc_emac_priv *priv = edev->priv;
	unsigned int addr_low, addr_hi;

	addr_low = le32_to_cpu(*(__le32 *) &mac[0]);
	addr_hi = le16_to_cpu(*(__le16 *) &mac[4]);

	arc_reg_set(priv, R_ADDRL, addr_low);
	arc_reg_set(priv, R_ADDRH, addr_hi);

	return 0;
}

static int arc_mdio_complete_wait(struct arc_emac_priv *priv)
{
	uint64_t start = get_time_ns();

	while (!is_timeout(start, 1000 * MSECOND)) {
		if (arc_reg_get(priv, R_STATUS) & MDIO_MASK) {
			/* Reset "MDIO complete" flag */
			arc_reg_set(priv, R_STATUS, MDIO_MASK);
			return 0;
		}
	}

	return -ETIMEDOUT;
}

static int arc_emac_mdio_read(struct mii_bus *bus, int phy_addr, int reg_num)
{
	struct arc_emac_priv *priv = bus->priv;
	int error;

	arc_reg_set(priv, R_MDIO,
		    0x60020000 | (phy_addr << 23) | (reg_num << 18));

	error = arc_mdio_complete_wait(priv);
	if (error < 0)
		return error;

	return arc_reg_get(priv, R_MDIO) & 0xffff;
}

static int arc_emac_mdio_write(struct mii_bus *bus, int phy_addr, int reg_num,
			       u16 value)
{
	struct arc_emac_priv *priv = bus->priv;

	arc_reg_set(priv, R_MDIO,
		     0x50020000 | (phy_addr << 23) | (reg_num << 18) | value);

	return arc_mdio_complete_wait(priv);
}

#define DEFAULT_EMAC_CLOCK_FREQUENCY 50000000UL;

static int arc_emac_probe(struct device_d *dev)
{
	struct resource *iores;
	struct eth_device *edev;
	struct arc_emac_priv *priv;
	unsigned long clock_frequency;
	struct mii_bus *miibus;
	u32 id;

	/* clock-frequency is dropped from DTS, so hardcode it here */
	clock_frequency = DEFAULT_EMAC_CLOCK_FREQUENCY;

	edev = xzalloc(sizeof(struct eth_device) +
		       sizeof(struct arc_emac_priv));
	edev->priv = (struct arc_emac_priv *)(edev + 1);
	miibus = xzalloc(sizeof(struct mii_bus));

	priv = edev->priv;
	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	priv->regs = IOMEM(iores->start);
	priv->bus = miibus;

	priv->clk = clk_get(dev, "hclk");
	clk_enable(priv->clk);

	priv->refclk = clk_get(dev, "macref");
	clk_set_rate(priv->refclk, clock_frequency);
	clk_enable(priv->refclk);

	id = arc_reg_get(priv, R_ID);
	/* Check for EMAC revision 5 or 7, magic number */
	if (!(id == 0x0005fd02 || id == 0x0007fd02)) {
		dev_err(dev, "ARC EMAC not detected, id=0x%x\n", id);
		free(edev);
		free(miibus);
		return -ENODEV;
	}
	dev_info(dev, "ARC EMAC detected with id: 0x%x\n", id);

	edev->init = arc_emac_init;
	edev->open = arc_emac_open;
	edev->send = arc_emac_send;
	edev->recv = arc_emac_recv;
	edev->halt = arc_emac_halt;
	edev->get_ethaddr = arc_emac_get_ethaddr;
	edev->set_ethaddr = arc_emac_set_ethaddr;
	edev->parent = dev;

	miibus->read = arc_emac_mdio_read;
	miibus->write = arc_emac_mdio_write;
	miibus->priv = priv;
	miibus->parent = dev;

	/* allocate rx/tx descriptors */
	priv->rxbd = dma_alloc_coherent(RX_BD_NUM * sizeof(struct arc_emac_bd),
					DMA_ADDRESS_BROKEN);
	priv->txbd = dma_alloc_coherent(TX_BD_NUM * sizeof(struct arc_emac_bd),
					DMA_ADDRESS_BROKEN);
	priv->rxbuf = dma_alloc(RX_BD_NUM * PKTSIZE);

	/* Set poll rate so that it polls every 1 ms */
	arc_reg_set(priv, R_POLLRATE, clock_frequency / 1000000);

	/* Disable interrupts */
	arc_reg_set(priv, R_ENABLE, 0);

	mdiobus_register(miibus);
	eth_register(edev);

	return 0;
}

static __maybe_unused struct of_device_id arc_emac_dt_ids[] = {
	{
		.compatible = "snps,arc-emac",
	}, {
		.compatible = "rockchip,rk3188-emac",
	}, {
		/* sentinel */
	}
};

static struct driver_d arc_emac_driver = {
	.name = "arc-emac",
	.probe = arc_emac_probe,
	.of_compatible = DRV_OF_COMPAT(arc_emac_dt_ids),
};
device_platform_driver(arc_emac_driver);
