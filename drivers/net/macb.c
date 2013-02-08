/*
 * Copyright (C) 2005-2006 Atmel Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <common.h>

/*
 * The barebox networking stack is a little weird.  It seems like the
 * networking core allocates receive buffers up front without any
 * regard to the hardware that's supposed to actually receive those
 * packets.
 *
 * The MACB receives packets into 128-byte receive buffers, so the
 * buffers allocated by the core isn't very practical to use.  We'll
 * allocate our own, but we need one such buffer in case a packet
 * wraps around the DMA ring so that we have to copy it.
 *
 * Therefore, define CFG_RX_ETH_BUFFER to 1 in the board-specific
 * configuration header.  This way, the core allocates one RX buffer
 * and one TX buffer, each of which can hold a ethernet packet of
 * maximum size.
 *
 * For some reason, the networking core unconditionally specifies a
 * 32-byte packet "alignment" (which really should be called
 * "padding").  MACB shouldn't need that, but we'll refrain from any
 * core modifications here...
 */

#include <net.h>
#include <clock.h>
#include <malloc.h>
#include <xfuncs.h>
#include <init.h>
#include <errno.h>
#include <io.h>
#include <mach/board.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <asm/mmu.h>
#include <linux/phy.h>

#include "macb.h"

#define CFG_MACB_RX_BUFFER_SIZE		4096
#define CFG_MACB_RX_RING_SIZE		(CFG_MACB_RX_BUFFER_SIZE / 128)
#define CFG_MACB_TX_TIMEOUT		1000
#define CFG_MACB_AUTONEG_TIMEOUT	5000000

struct macb_device {
	void			__iomem *regs;

	unsigned int		rx_tail;
	unsigned int		tx_tail;

	void			*rx_buffer;
	void			*tx_buffer;
	struct macb_dma_desc	*rx_ring;
	struct macb_dma_desc	*tx_ring;

	int			phy_addr;

	const struct device_d	*dev;
	struct eth_device	netdev;

	phy_interface_t		interface;

	struct mii_bus	miibus;

	unsigned int		phy_flags;
};

static int macb_send(struct eth_device *edev, void *packet,
		     int length)
{
	struct macb_device *macb = edev->priv;
	unsigned long ctrl;
	int ret;

	dev_dbg(macb->dev, "%s\n", __func__);

	ctrl = MACB_BF(TX_FRMLEN, length);
	ctrl |= MACB_BIT(TX_LAST) | MACB_BIT(TX_WRAP);

	macb->tx_ring[0].ctrl = ctrl;
	macb->tx_ring[0].addr = (ulong)packet;
	barrier();
	dma_flush_range((ulong) packet, (ulong)packet + length);
	macb_writel(macb, NCR, MACB_BIT(TE) | MACB_BIT(RE) | MACB_BIT(TSTART));

	ret = wait_on_timeout(100 * MSECOND,
		!(macb->tx_ring[0].ctrl & MACB_BIT(TX_USED)));

	ctrl = macb->tx_ring[0].ctrl;

	if (ctrl & MACB_BIT(TX_UNDERRUN))
		dev_err(macb->dev, "TX underrun\n");
	if (ctrl & MACB_BIT(TX_BUF_EXHAUSTED))
		dev_err(macb->dev, "TX buffers exhausted in mid frame\n");
	if (ret)
		dev_err(macb->dev,"TX timeout\n");

	return ret;
}

static void reclaim_rx_buffers(struct macb_device *macb,
			       unsigned int new_tail)
{
	unsigned int i;

	dev_dbg(macb->dev, "%s\n", __func__);

	i = macb->rx_tail;
	while (i > new_tail) {
		macb->rx_ring[i].addr &= ~MACB_BIT(RX_USED);
		i++;
		if (i > CFG_MACB_RX_RING_SIZE)
			i = 0;
	}

	while (i < new_tail) {
		macb->rx_ring[i].addr &= ~MACB_BIT(RX_USED);
		i++;
	}

	barrier();
	macb->rx_tail = new_tail;
}

static int macb_recv(struct eth_device *edev)
{
	struct macb_device *macb = edev->priv;
	unsigned int rx_tail = macb->rx_tail;
	void *buffer;
	int length;
	int wrapped = 0;
	u32 status;

	dev_dbg(macb->dev, "%s\n", __func__);

	for (;;) {
		if (!(macb->rx_ring[rx_tail].addr & MACB_BIT(RX_USED)))
			return -1;

		status = macb->rx_ring[rx_tail].ctrl;
		if (status & MACB_BIT(RX_SOF)) {
			if (rx_tail != macb->rx_tail)
				reclaim_rx_buffers(macb, rx_tail);
			wrapped = 0;
		}

		if (status & MACB_BIT(RX_EOF)) {
			buffer = macb->rx_buffer + 128 * macb->rx_tail;
			length = MACB_BFEXT(RX_FRMLEN, status);
			if (wrapped) {
				unsigned int headlen, taillen;

				headlen = 128 * (CFG_MACB_RX_RING_SIZE
						 - macb->rx_tail);
				taillen = length - headlen;
				memcpy((void *)NetRxPackets[0],
				       buffer, headlen);
				memcpy((void *)NetRxPackets[0] + headlen,
				       macb->rx_buffer, taillen);
				buffer = (void *)NetRxPackets[0];
			}

			net_receive(buffer, length);
			if (++rx_tail >= CFG_MACB_RX_RING_SIZE)
				rx_tail = 0;
			reclaim_rx_buffers(macb, rx_tail);
		} else {
			if (++rx_tail >= CFG_MACB_RX_RING_SIZE) {
				wrapped = 1;
				rx_tail = 0;
			}
		}
		barrier();
	}

	return 0;
}

static void macb_adjust_link(struct eth_device *edev)
{
	struct macb_device *macb = edev->priv;
	u32 reg;

	reg = macb_readl(macb, NCFGR);
	reg &= ~(MACB_BIT(SPD) | MACB_BIT(FD));

	if (edev->phydev->duplex)
		reg |= MACB_BIT(FD);
	if (edev->phydev->speed == SPEED_100)
		reg |= MACB_BIT(SPD);

	macb_writel(macb, NCFGR, reg);
}

static int macb_open(struct eth_device *edev)
{
	struct macb_device *macb = edev->priv;

	dev_dbg(macb->dev, "%s\n", __func__);

	/* Obtain the PHY's address/id */
	return phy_device_connect(edev, &macb->miibus, macb->phy_addr,
			       macb_adjust_link, macb->phy_flags,
			       macb->interface);
}

static void macb_init(struct macb_device *macb)
{
	unsigned long paddr, val = 0;
	int i;

	dev_dbg(macb->dev, "%s\n", __func__);

	/*
	 * macb_halt should have been called at some point before now,
	 * so we'll assume the controller is idle.
	 */

	/* initialize DMA descriptors */
	paddr = (ulong)macb->rx_buffer;
	for (i = 0; i < CFG_MACB_RX_RING_SIZE; i++) {
		if (i == (CFG_MACB_RX_RING_SIZE - 1))
			paddr |= MACB_BIT(RX_WRAP);
		macb->rx_ring[i].addr = paddr;
		macb->rx_ring[i].ctrl = 0;
		paddr += 128;
	}

	macb->tx_ring[0].addr = 0;
	macb->tx_ring[0].ctrl = MACB_BIT(TX_USED) | MACB_BIT(TX_WRAP);

	macb->rx_tail = macb->tx_tail = 0;

	macb_writel(macb, RBQP, (ulong)macb->rx_ring);
	macb_writel(macb, TBQP, (ulong)macb->tx_ring);

	if (macb->interface == PHY_INTERFACE_MODE_RMII)
		val |= MACB_BIT(RMII);
	else
		val &= ~MACB_BIT(RMII);

#if defined(CONFIG_ARCH_AT91)
	val |= MACB_BIT(CLKEN);
#endif
	macb_writel(macb, USRIO, val);

	/* Enable TX and RX */
	macb_writel(macb, NCR, MACB_BIT(TE) | MACB_BIT(RE));
}

static void macb_halt(struct eth_device *edev)
{
	struct macb_device *macb = edev->priv;
	u32 ncr, tsr;

	/* Halt the controller and wait for any ongoing transmission to end. */
	ncr = macb_readl(macb, NCR);
	ncr |= MACB_BIT(THALT);
	macb_writel(macb, NCR, ncr);

	do {
		tsr = macb_readl(macb, TSR);
	} while (tsr & MACB_BIT(TGO));

	/* Disable TX and RX, and clear statistics */
	macb_writel(macb, NCR, MACB_BIT(CLRSTAT));
}

static int macb_phy_read(struct mii_bus *bus, int addr, int reg)
{
	struct macb_device *macb = bus->priv;

	unsigned long netctl;
	unsigned long frame;
	int value;
	uint64_t start;

	dev_dbg(macb->dev, "%s\n", __func__);

	netctl = macb_readl(macb, NCR);
	netctl |= MACB_BIT(MPE);
	macb_writel(macb, NCR, netctl);

	frame = (MACB_BF(SOF, MACB_MAN_SOF)
		 | MACB_BF(RW, MACB_MAN_READ)
		 | MACB_BF(PHYA, addr)
		 | MACB_BF(REGA, reg)
		 | MACB_BF(CODE, MACB_MAN_CODE));
	macb_writel(macb, MAN, frame);

	start = get_time_ns();
	do {
		if (is_timeout(start, SECOND)) {
			dev_err(macb->dev, "phy read timed out\n");
			return -1;
		}
	} while (!MACB_BFEXT(IDLE, macb_readl(macb, NSR)));

	frame = macb_readl(macb, MAN);
	value = MACB_BFEXT(DATA, frame);

	netctl = macb_readl(macb, NCR);
	netctl &= ~MACB_BIT(MPE);
	macb_writel(macb, NCR, netctl);

	return value;
}

static int macb_phy_write(struct mii_bus *bus, int addr, int reg, u16 value)
{
	struct macb_device *macb = bus->priv;
	unsigned long netctl;
	unsigned long frame;

	dev_dbg(macb->dev, "%s\n", __func__);

	netctl = macb_readl(macb, NCR);
	netctl |= MACB_BIT(MPE);
	macb_writel(macb, NCR, netctl);

	frame = (MACB_BF(SOF, MACB_MAN_SOF)
		 | MACB_BF(RW, MACB_MAN_WRITE)
		 | MACB_BF(PHYA, addr)
		 | MACB_BF(REGA, reg)
		 | MACB_BF(CODE, MACB_MAN_CODE)
		 | MACB_BF(DATA, value));
	macb_writel(macb, MAN, frame);

	while (!MACB_BFEXT(IDLE, macb_readl(macb, NSR)))
		;

	netctl = macb_readl(macb, NCR);
	netctl &= ~MACB_BIT(MPE);
	macb_writel(macb, NCR, netctl);

	return 0;
}

static int macb_get_ethaddr(struct eth_device *edev, unsigned char *adr)
{
	struct macb_device *macb = edev->priv;

	dev_dbg(macb->dev, "%s\n", __func__);

	return -1;
}

static int macb_set_ethaddr(struct eth_device *edev, unsigned char *adr)
{
	struct macb_device *macb = edev->priv;

	dev_dbg(macb->dev, "%s\n", __func__);

	/* set hardware address */
	macb_writel(macb, SA1B, adr[0] | adr[1] << 8 | adr[2] << 16 | adr[3] << 24);
	macb_writel(macb, SA1T, adr[4] | adr[5] << 8);

	return 0;
}

static int macb_probe(struct device_d *dev)
{
	struct eth_device *edev;
	struct macb_device *macb;
	unsigned long macb_hz;
	u32 ncfgr;
	struct at91_ether_platform_data *pdata;
	struct clk *pclk;

	if (!dev->platform_data) {
		dev_err(dev, "macb: no platform_data\n");
		return -ENODEV;
	}
	pdata = dev->platform_data;

	edev = xzalloc(sizeof(struct eth_device) + sizeof(struct macb_device));
	edev->priv = (struct macb_device *)(edev + 1);
	macb = edev->priv;

	macb->dev = dev;

	edev->open = macb_open;
	edev->send = macb_send;
	edev->recv = macb_recv;
	edev->halt = macb_halt;
	edev->get_ethaddr = pdata->get_ethaddr ? pdata->get_ethaddr : macb_get_ethaddr;
	edev->set_ethaddr = macb_set_ethaddr;
	edev->parent = dev;

	macb->miibus.read = macb_phy_read;
	macb->miibus.write = macb_phy_write;
	macb->phy_addr = pdata->phy_addr;
	macb->miibus.priv = macb;
	macb->miibus.parent = dev;

	if (pdata->phy_interface == PHY_INTERFACE_MODE_NA)
		macb->interface = PHY_INTERFACE_MODE_MII;
	else
		macb->interface = pdata->phy_interface;

	macb->phy_flags = pdata->phy_flags;

	macb->rx_buffer = dma_alloc_coherent(CFG_MACB_RX_BUFFER_SIZE);
	macb->rx_ring = dma_alloc_coherent(CFG_MACB_RX_RING_SIZE * sizeof(struct macb_dma_desc));
	macb->tx_ring = dma_alloc_coherent(sizeof(struct macb_dma_desc));

	macb->regs = dev_request_mem_region(dev, 0);

	/*
	 * Do some basic initialization so that we at least can talk
	 * to the PHY
	 */
	pclk = clk_get(dev, "macb_clk");
	if (IS_ERR(pclk)) {
		dev_err(dev, "no macb_clk\n");
		return PTR_ERR(pclk);
	}

	clk_enable(pclk);
	macb_hz = clk_get_rate(pclk);
	if (macb_hz < 20000000)
		ncfgr = MACB_BF(CLK, MACB_CLK_DIV8);
	else if (macb_hz < 40000000)
		ncfgr = MACB_BF(CLK, MACB_CLK_DIV16);
	else if (macb_hz < 80000000)
		ncfgr = MACB_BF(CLK, MACB_CLK_DIV32);
	else
		ncfgr = MACB_BF(CLK, MACB_CLK_DIV64);

	macb_writel(macb, NCFGR, ncfgr);

	macb_init(macb);

	mdiobus_register(&macb->miibus);
	eth_register(edev);

	return 0;
}

static struct driver_d macb_driver = {
	.name  = "macb",
	.probe = macb_probe,
};

static int macb_driver_init(void)
{
	debug("%s\n", __func__);
	platform_driver_register(&macb_driver);
	return 0;
}
device_initcall(macb_driver_init);
