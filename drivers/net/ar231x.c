// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * ar231x.c: driver for the Atheros AR231x Ethernet device.
 * This device is build in to SoC on ar231x series.
 * All known of them are big endian.
 *
 * Based on Linux driver:
 *   Copyright (C) 2004 by Sameer Dekate <sdekate@arubanetworks.com>
 *   Copyright (C) 2006 Imre Kaloz <kaloz@openwrt.org>
 *   Copyright (C) 2006-2009 Felix Fietkau <nbd@openwrt.org>
 * Ported to Barebox:
 *   Copyright (C) 2013 Oleksij Rempel <linux@rempel-privat.de>
 */
/*
 * Known issues:
 * - broadcast packets are not filtered by hardware. On noisy network with
 * lots of bcast packages rx_buffer can be completely filled after. Currently
 * we clear rx_buffer transmit some package.
 */

#include <common.h>
#include <net.h>
#include <init.h>
#include <io.h>
#include <linux/err.h>

#include "ar231x.h"

static inline void dma_writel(struct ar231x_eth_priv *priv,
		u32 val, int reg)
{
	__raw_writel(val, priv->dma_regs + reg);
}

static inline u32 dma_readl(struct ar231x_eth_priv *priv, int reg)
{
	return __raw_readl(priv->dma_regs + reg);
}

static inline void eth_writel(struct ar231x_eth_priv *priv,
		u32 val, int reg)
{
	__raw_writel(val, priv->eth_regs + reg);
}

static inline u32 eth_readl(struct ar231x_eth_priv *priv, int reg)
{
	return __raw_readl(priv->eth_regs + reg);
}

static inline void phy_writel(struct ar231x_eth_priv *priv,
		u32 val, int reg)
{
	__raw_writel(val, priv->phy_regs + reg);
}

static inline u32 phy_readl(struct ar231x_eth_priv *priv, int reg)
{
	return __raw_readl(priv->phy_regs + reg);
}

static void ar231x_reset_bit_(struct ar231x_eth_priv *priv,
		u32 val, enum reset_state state)
{
	if (priv->reset_bit)
		(*priv->reset_bit)(val, state);
}

static int ar231x_set_ethaddr(struct eth_device *edev, const unsigned char *addr);
static void ar231x_reset_regs(struct eth_device *edev)
{
	struct ar231x_eth_priv *priv = edev->priv;
	struct ar231x_eth_platform_data *cfg = priv->cfg;
	u32 flags;

	ar231x_reset_bit_(priv, cfg->reset_mac, SET);
	mdelay(10);

	ar231x_reset_bit_(priv, cfg->reset_mac, REMOVE);
	mdelay(10);

	ar231x_reset_bit_(priv, cfg->reset_phy, SET);
	mdelay(10);

	ar231x_reset_bit_(priv, cfg->reset_phy, REMOVE);
	mdelay(10);

	dma_writel(priv, DMA_BUS_MODE_SWR, AR231X_DMA_BUS_MODE);
	mdelay(10);

	dma_writel(priv, ((32 << DMA_BUS_MODE_PBL_SHIFT) | DMA_BUS_MODE_BLE),
			AR231X_DMA_BUS_MODE);

	/* FIXME: priv->{t,r}x_ring are virtual addresses,
	 * use virt-to-phys convertion */
	dma_writel(priv, (u32)priv->tx_ring, AR231X_DMA_TX_RING);
	dma_writel(priv, (u32)priv->rx_ring, AR231X_DMA_RX_RING);

	dma_writel(priv, (DMA_CONTROL_SR | DMA_CONTROL_ST | DMA_CONTROL_SF),
			AR231X_DMA_CONTROL);

	eth_writel(priv, FLOW_CONTROL_FCE, AR231X_ETH_FLOW_CONTROL);
	/* TODO: not sure if we need it here. */
	eth_writel(priv, 0x8100, AR231X_ETH_VLAN_TAG);

	/* Enable Ethernet Interface */
	flags = (MAC_CONTROL_TE |	/* transmit enable */
			/* FIXME: MAC_CONTROL_PM - pass mcast.
			 * Seems like it makes no difference on some WiSoCs,
			 * for example ar2313.
			 * It should be tested on ar231[5,6,7] */
			MAC_CONTROL_PM |
			MAC_CONTROL_F  |	/* full duplex */
			MAC_CONTROL_HBD);	/* heart beat disabled */
	eth_writel(priv, flags, AR231X_ETH_MAC_CONTROL);
}

static void ar231x_flash_rxdsc(struct ar231x_descr *rxdsc)
{
	rxdsc->status = DMA_RX_OWN;
	rxdsc->devcs = ((AR2313_RX_BUFSIZE << DMA_RX1_BSIZE_SHIFT) |
			DMA_RX1_CHAINED);
}

static void ar231x_allocate_dma_descriptors(struct eth_device *edev)
{
	struct ar231x_eth_priv *priv = edev->priv;
	u16 ar231x_descr_size = sizeof(struct ar231x_descr);
	u16 i;

	priv->tx_ring = xmalloc(ar231x_descr_size);
	dev_dbg(&edev->dev, "allocate tx_ring @ %p\n", priv->tx_ring);

	priv->rx_ring = xmalloc(ar231x_descr_size * AR2313_RXDSC_ENTRIES);
	dev_dbg(&edev->dev, "allocate rx_ring @ %p\n", priv->rx_ring);

	priv->rx_buffer = xmalloc(AR2313_RX_BUFSIZE * AR2313_RXDSC_ENTRIES);
	dev_dbg(&edev->dev, "allocate rx_buffer @ %p\n", priv->rx_buffer);

	/* Initialize the rx Descriptors */
	for (i = 0; i < AR2313_RXDSC_ENTRIES; i++) {
		struct ar231x_descr *rxdsc = &priv->rx_ring[i];
		ar231x_flash_rxdsc(rxdsc);
		rxdsc->buffer_ptr =
			(u32)(priv->rx_buffer + AR2313_RX_BUFSIZE * i);
		rxdsc->next_dsc_ptr = (u32)&priv->rx_ring[DSC_NEXT(i)];
	}
	/* set initial position of ring descriptor */
	priv->next_rxdsc = &priv->rx_ring[0];
}

static void ar231x_adjust_link(struct eth_device *edev)
{
	struct ar231x_eth_priv *priv = edev->priv;
	u32 mc;

	if (edev->phydev->duplex != priv->oldduplex) {
		mc = eth_readl(priv, AR231X_ETH_MAC_CONTROL);
		mc &= ~(MAC_CONTROL_F | MAC_CONTROL_DRO);
		if (edev->phydev->duplex)
			mc |= MAC_CONTROL_F;
		else
			mc |= MAC_CONTROL_DRO;
		eth_writel(priv, mc, AR231X_ETH_MAC_CONTROL);
		priv->oldduplex = edev->phydev->duplex;
	}
}

static int ar231x_eth_init(struct eth_device *edev)
{
	struct ar231x_eth_priv *priv = edev->priv;

	ar231x_allocate_dma_descriptors(edev);
	ar231x_reset_regs(edev);
	ar231x_set_ethaddr(edev, priv->mac);
	return 0;
}

static int ar231x_eth_open(struct eth_device *edev)
{
	struct ar231x_eth_priv *priv = edev->priv;
	u32 tmp;

	/* Enable RX. Now the rx_buffer will be filled.
	 * If it is full we may lose first transmission. In this case
	 * barebox should retry it.
	 * Or TODO: - force HW to filter some how broadcasts
	 *			- disable RX if we do not need it. */
	tmp = eth_readl(priv, AR231X_ETH_MAC_CONTROL);
	eth_writel(priv, (tmp | MAC_CONTROL_RE), AR231X_ETH_MAC_CONTROL);

	return phy_device_connect(edev, &priv->miibus, (int)priv->phy_regs,
			ar231x_adjust_link, 0, PHY_INTERFACE_MODE_MII);
}

static int ar231x_eth_recv(struct eth_device *edev)
{
	struct ar231x_eth_priv *priv = edev->priv;

	while (1) {
		struct ar231x_descr *rxdsc = priv->next_rxdsc;
		u32 status = rxdsc->status;

		/* owned by DMA? */
		if (status & DMA_RX_OWN)
			break;

		/* Pick only packets what we can handle:
		 * - only complete packet per buffer
		 *   (First and Last at same time)
		 * - drop multicast */
		if (!priv->kill_rx_ring &&
				((status & DMA_RX_MASK) == DMA_RX_FSLS)) {
			u16 length =
				((status >> DMA_RX_LEN_SHIFT) & 0x3fff)
				- CRC_LEN;
			net_receive(edev, (void *)rxdsc->buffer_ptr, length);
		}
		/* Clean descriptor. now it is owned by DMA. */
		priv->next_rxdsc = (struct ar231x_descr *)rxdsc->next_dsc_ptr;
		ar231x_flash_rxdsc(rxdsc);
	}
	priv->kill_rx_ring = 0;
	return 0;
}

static int ar231x_eth_send(struct eth_device *edev, void *packet,
		int length)
{
	struct ar231x_eth_priv *priv = edev->priv;
	struct ar231x_descr *txdsc = priv->tx_ring;
	u32 rx_missed;

	/* We do not do async work.
	 * If rx_ring is full, there is nothing we can use. */
	rx_missed = dma_readl(priv, AR231X_DMA_RX_MISSED);
	if (rx_missed) {
		priv->kill_rx_ring = 1;
		ar231x_eth_recv(edev);
	}

	/* Setup the transmit descriptor. */
	txdsc->devcs = ((length << DMA_TX1_BSIZE_SHIFT) | DMA_TX1_DEFAULT);
	txdsc->buffer_ptr = (uint)packet;
	txdsc->status = DMA_TX_OWN;

	/* Trigger transmission */
	dma_writel(priv, 0, AR231X_DMA_TX_POLL);

	/* Take enough time to transmit packet. 100 is not enough. */
	wait_on_timeout(2000 * MSECOND,
		!(txdsc->status & DMA_TX_OWN));

	/* We can't do match here. If it is still in progress,
	 * then engine is probably stalled or we wait not enough. */
	if (txdsc->status & DMA_TX_OWN)
		dev_err(&edev->dev, "Frame is still in progress.\n");

	if (txdsc->status & DMA_TX_ERROR)
		dev_err(&edev->dev, "Frame was aborted by engine\n");

	/* Ready or not. Stop it. */
	txdsc->status = 0;
	return 0;
}

static void ar231x_eth_halt(struct eth_device *edev)
{
	struct ar231x_eth_priv *priv = edev->priv;
	u32 tmp;

	/* kill the MAC: disable RX and TX */
	tmp = eth_readl(priv, AR231X_ETH_MAC_CONTROL);
	eth_writel(priv, tmp & ~(MAC_CONTROL_RE | MAC_CONTROL_TE),
			AR231X_ETH_MAC_CONTROL);

	/* stop DMA */
	dma_writel(priv, 0, AR231X_DMA_CONTROL);
	dma_writel(priv, DMA_BUS_MODE_SWR, AR231X_DMA_BUS_MODE);

	/* place PHY and MAC in reset */
	ar231x_reset_bit_(priv, (priv->cfg->reset_mac | priv->cfg->reset_phy), SET);
}

static int ar231x_get_ethaddr(struct eth_device *edev, unsigned char *addr)
{
	struct ar231x_eth_priv *priv = edev->priv;

	/* MAC address is stored on flash, in some kind of atheros config
	 * area. Platform code should read it and pass to the driver. */
	memcpy(addr, priv->mac, 6);
	return 0;
}

/**
 * These device do not have build in MAC address.
 * It is located on atheros-config field on flash.
 */
static int ar231x_set_ethaddr(struct eth_device *edev, unsigned char *addr)
{
	struct ar231x_eth_priv *priv = edev->priv;

	eth_writel(priv,
			(addr[5] <<  8) | (addr[4]),
			AR231X_ETH_MAC_ADDR1);
	eth_writel(priv,
			(addr[3] << 24) | (addr[2] << 16) |
			(addr[1] <<  8) | addr[0],
			AR231X_ETH_MAC_ADDR2);

	mdelay(10);
	return 0;
}

#define MII_ADDR(phy, reg) \
	((reg << MII_ADDR_REG_SHIFT) | (phy << MII_ADDR_PHY_SHIFT))

static int ar231x_miibus_read(struct mii_bus *bus, int phy_id, int regnum)
{
	struct ar231x_eth_priv *priv = bus->priv;
	uint64_t time_start;

	phy_writel(priv, MII_ADDR(phy_id, regnum), AR231X_ETH_MII_ADDR);
	time_start = get_time_ns();
	while (phy_readl(priv, AR231X_ETH_MII_ADDR) & MII_ADDR_BUSY) {
		if (is_timeout(time_start, SECOND)) {
			dev_err(&bus->dev, "miibus read timeout\n");
			return -ETIMEDOUT;
		}
	}
	return phy_readl(priv, AR231X_ETH_MII_DATA) >> MII_DATA_SHIFT;
}

static int ar231x_miibus_write(struct mii_bus *bus, int phy_id,
			       int regnum, u16 val)
{
	struct ar231x_eth_priv *priv = bus->priv;
	uint64_t time_start = get_time_ns();

	while (phy_readl(priv, AR231X_ETH_MII_ADDR) & MII_ADDR_BUSY) {
		if (is_timeout(time_start, SECOND)) {
			dev_err(&bus->dev, "miibus write timeout\n");
			return -ETIMEDOUT;
		}
	}
	phy_writel(priv, val << MII_DATA_SHIFT, AR231X_ETH_MII_DATA);
	phy_writel(priv, MII_ADDR(phy_id, regnum) | MII_ADDR_WRITE,
		   AR231X_ETH_MII_ADDR);
	return 0;
}

static int ar231x_mdiibus_reset(struct mii_bus *bus)
{
	struct ar231x_eth_priv *priv = bus->priv;

	ar231x_reset_regs(&priv->edev);
	return 0;
}

static int ar231x_eth_probe(struct device_d *dev)
{
	struct resource *iores;
	struct ar231x_eth_priv *priv;
	struct eth_device *edev;
	struct mii_bus *miibus;
	struct ar231x_eth_platform_data *pdata;

	if (!dev->platform_data) {
		dev_err(dev, "no platform data\n");
		return -ENODEV;
	}

	pdata = dev->platform_data;

	priv = xzalloc(sizeof(struct ar231x_eth_priv));
	edev = &priv->edev;
	miibus = &priv->miibus;
	edev->priv = priv;

	/* link all platform depended regs */
	priv->mac = pdata->mac;
	priv->reset_bit = pdata->reset_bit;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores)) {
		dev_err(dev, "No eth_regs!!\n");
		return PTR_ERR(iores);
	}
	priv->eth_regs = IOMEM(iores->start);
	/* we have 0x100000 for eth, part of it are dma regs.
	 * So they are already requested */
	priv->dma_regs = (void *)(priv->eth_regs + 0x1000);

	iores = dev_request_mem_resource(dev, 1);
	if (IS_ERR(iores)) {
		dev_err(dev, "No phy_regs!!\n");
		return PTR_ERR(iores);
	}
	priv->phy_regs = IOMEM(iores->start);

	priv->cfg = pdata;
	edev->init = ar231x_eth_init;
	edev->open = ar231x_eth_open;
	edev->send = ar231x_eth_send;
	edev->recv = ar231x_eth_recv;
	edev->halt = ar231x_eth_halt;
	edev->get_ethaddr = ar231x_get_ethaddr;
	edev->set_ethaddr = ar231x_set_ethaddr;

	priv->miibus.read = ar231x_miibus_read;
	priv->miibus.write = ar231x_miibus_write;
	priv->miibus.reset = ar231x_mdiibus_reset;
	priv->miibus.priv = priv;
	priv->miibus.parent = dev;

	mdiobus_register(miibus);
	eth_register(edev);

	return 0;
}

static struct driver_d ar231x_eth_driver = {
	.name = "ar231x_eth",
	.probe = ar231x_eth_probe,
};

static int ar231x_eth_driver_init(void)
{
	return platform_driver_register(&ar231x_eth_driver);
}
device_initcall(ar231x_eth_driver_init);
