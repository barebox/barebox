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
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301 USA
 */
#include <common.h>
#include <dma.h>
#include <init.h>
#include <io.h>
#include <net.h>
#include <of_net.h>
#include <linux/sizes.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/mbus.h>

#include "orion-gbe.h"

struct rxdesc {
	u32 cmd_sts;		/* Descriptor command status */
	u16 buf_size;		/* Buffer size */
	u16 byte_cnt;		/* Descriptor buffer byte count */
	void *buf_ptr;		/* Descriptor buffer pointer */
	struct rxdesc *nxtdesc;	/* Next descriptor pointer */
};

struct txdesc {
	u32 cmd_sts;		/* Descriptor command status */
	u16 l4i_chk;		/* CPU provided TCP Checksum */
	u16 byte_cnt;		/* Descriptor buffer byte count */
	void *buf_ptr;		/* Descriptor buffer ptr */
	struct txdesc *nxtdesc;	/* Next descriptor ptr */
};

struct port_priv {
	struct device_d dev;
	struct eth_device edev;
	void __iomem *regs;
	struct device_node *np;
	int portno;
	struct txdesc *txdesc;
	struct rxdesc *rxdesc;
	struct rxdesc *current_rxdesc;
	u8 *rxbuf;
	phy_interface_t intf;
};

struct orion_gbe {
	void __iomem *regs;
	struct clk *clk;
	struct port_priv *ports;
	int num_ports;
};

#define UTXQ			0	/* Used Tx queue number */
#define URXQ			0	/* Used Rx queue number */
#define RX_RING_SIZE		4
#define TRANSFER_TIMEOUT	(10 * MSECOND)

#define NR_ADDR_WINS		6	/* number of address windows */
#define NR_HIGH_ADDR_WINS	4	/* number of high address windows */

#define ACCEPT_MAC_ADDR		0
#define REJECT_MAC_ADDR		1

/* setup DRAM access windows provided by mbus */
static void eunit_set_dram_access(struct orion_gbe *gbe)
{
	const struct mbus_dram_target_info *dram = mvebu_mbus_dram_info();
	u32 bare = ~0, epap = 0, reg;
	int n;

	for (n = 0; n < NR_ADDR_WINS; n++) {
		if (n >= dram->num_cs)
			continue;

		/* enable BAR */
		bare &= ~BIT(n);
		/* set port access protect to R/W */
		epap |= ACCESS_FULL << (n * 2);

		/* configure Base Address and Size */
		reg = ((dram->cs[n].size / SZ_64K) - 1) << 16;
		writel(reg, gbe->regs + EUNIT_S(n));

		reg = dram->cs[n].base & 0xffff0000;
		reg |= dram->cs[n].mbus_attr << 8;
		reg |= dram->mbus_dram_target_id;
		writel(reg, gbe->regs + EUNIT_BA(n));
		if (n < NR_HIGH_ADDR_WINS)
			writel(0, gbe->regs + EUNIT_HA(n));
	}

	writel(epap, gbe->regs + EUNIT_PAP);
	writel(bare, gbe->regs + EUNIT_BARE);
}

/* clear entries in unicast, special multicast, and other multicast tables */
static void port_clear_mac_tables(struct port_priv *port)
{
	int n;

	/* clear unicast tables (DFUTn) */
	for (n = 0; n < 4; n++)
		writel(0, port->regs + PORT_DFUT(n));

	/* clear special (DFSMTn) and other (DFOMTn) multicast tables */
	for (n = 0; n < 64; n++) {
		writel(0, port->regs + PORT_DFSMT(n));
		writel(0, port->regs + PORT_DFOMT(n));
	}
}

/*
 * set the port unicast address table
 *
 * This function adds/removes MAC addresses from the port unicast
 * address table.
 *
 * Locate the proper entry in the Unicast table for the specified MAC
 * nibble and set its properties according to function parameters.
 *
 * @nibble	Unicast MAC address, last nibble
 * @reject      0 = Accept, 1 = Reject MAC address
 */
static void port_set_unicast_filter(struct port_priv *port,
				    u8 nibble, int reject)
{
	u8 table, entry, shift;
	u32 reg;

	/* Locate the Unicast table entry by nibble */
	nibble &= 0xf;
	table = nibble / 4;
	entry = nibble % 4;
	shift = (DFT_ENTRY_SIZE * entry);

	reg = readl(port->regs + PORT_DFUT(table));
	reg &= DFT_ENTRY_MASK << shift;
	if (!reject)
		reg |= (DFT_PASS | (URXQ << DFT_QUEUE_SHIFT)) << shift;
	writel(reg, port->regs + PORT_DFUT(table));
}

/* initialize rx descriptor ring */
static void port_init_rxdesc_ring(struct port_priv *port)
{
	struct rxdesc *rxdesc, *nxtdesc;
	void *rxbuf;
	int n;

	/* initialize aligned rx descriptor ring-buffer */
	rxdesc = port->rxdesc;
	rxbuf = port->rxbuf;
	for (n = 0; n < RX_RING_SIZE; n++) {
		nxtdesc = ((void *)rxdesc) + ALIGN(sizeof(*port->rxdesc), 16);

		rxdesc->cmd_sts = RXDESC_OWNED_BY_DMA;
		rxdesc->buf_size = ALIGN(PKTSIZE, 8);
		rxdesc->byte_cnt = 0;
		rxdesc->buf_ptr = rxbuf;
		if (n == RX_RING_SIZE-1)
			rxdesc->nxtdesc = port->rxdesc;
		else
			rxdesc->nxtdesc = nxtdesc;

		rxbuf += ALIGN(PKTSIZE, 8);

		rxdesc = nxtdesc;
	}

	port->current_rxdesc = port->rxdesc;
}

/* stop a queue and check for termination */
static void port_stop_queue(void __iomem *ctrl)
{
	u32 reg = readl(ctrl);

	if (!(reg & 0xff))
		return;

	/* stop active channels only */
	writel((reg << 8), ctrl);
	/* wait for all queues to terminate */
	while (readl(ctrl) & 0xff)
		;
}

static void port_stop(struct port_priv *port)
{
	/* stop all queues */
	port_stop_queue(port->regs + PORT_TQC);
	port_stop_queue(port->regs + PORT_RQC);
	/* disable port, release reset */
	writel(readl(port->regs + PORT_SC0) & ~PORT_ENABLE,
	       port->regs + PORT_SC0);
	writel(readl(port->regs + PORT_SC1) & ~PORT_RESET,
	       port->regs + PORT_SC1);
	/* clear and mask all interrupts */
	writel(0, port->regs + PORT_IC);
	writel(0, port->regs + PORT_IM);
	writel(0, port->regs + PORT_EIC);
	writel(0, port->regs + PORT_EIM);
}

static void port_halt(struct eth_device *edev)
{
	struct port_priv *port = edev->priv;

	port_stop(port);
}

static int port_send(struct eth_device *edev, void *data, int len)
{
	struct port_priv *port = edev->priv;
	struct txdesc *txdesc = port->txdesc;
	u32 cmd_sts;
	int ret;

	/* flush transmit data */
	dma_sync_single_for_device((unsigned long)data, len, DMA_TO_DEVICE);

	txdesc->cmd_sts = TXDESC_OWNED_BY_DMA;
	txdesc->cmd_sts |= TXDESC_FIRST | TXDESC_LAST;
	txdesc->cmd_sts |= TXDESC_ZERO_PADDING | TXDESC_GEN_CRC;
	txdesc->buf_ptr = data;
	txdesc->byte_cnt = len;

	/* assign tx descriptor and issue send command */
	writel((u32)txdesc, port->regs + PORT_TCQDP(UTXQ));
	writel(BIT(UTXQ), port->regs + PORT_TQC);

	/* wait for packet transmit completion */
	ret = wait_on_timeout(TRANSFER_TIMEOUT,
		      (readl(&txdesc->cmd_sts) & TXDESC_OWNED_BY_DMA) == 0);
	dma_sync_single_for_cpu((unsigned long)data, len, DMA_TO_DEVICE);
	if (ret) {
		dev_err(&edev->dev, "transmit timeout\n");
		return ret;
	}

	cmd_sts = readl(&txdesc->cmd_sts);
	if ((cmd_sts & TXDESC_LAST) && (cmd_sts & TXDESC_ERROR)) {
		dev_err(&edev->dev, "transmit error %d\n",
			(cmd_sts & TXDESC_ERROR_MASK) >> TXDESC_ERROR_SHIFT);
		return ret;
	}

	return 0;
}

static int port_recv(struct eth_device *edev)
{
	struct port_priv *port = edev->priv;
	struct rxdesc *rxdesc = port->current_rxdesc;
	u32 cmd_sts;
	int ret = 0;

	/* wait for received packet */
	if (readl(&rxdesc->cmd_sts) & RXDESC_OWNED_BY_DMA)
		return 0;

	/* drop malicious packets */
	cmd_sts = readl(&rxdesc->cmd_sts);
	if ((cmd_sts & (RXDESC_FIRST | RXDESC_LAST)) !=
	    (RXDESC_FIRST | RXDESC_LAST)) {
		dev_err(&edev->dev, "rx packet spread on multiple descriptors\n");
		ret = -EIO;
		goto recv_err;
	}

	if (cmd_sts & RXDESC_ERROR) {
		dev_err(&edev->dev, "receive error %d\n",
			(cmd_sts & RXDESC_ERROR_MASK) >> RXDESC_ERROR_SHIFT);
		ret = -EIO;
		goto recv_err;
	}

	/* invalidate current receive buffer */
	dma_sync_single_for_cpu((unsigned long)rxdesc->buf_ptr,
				ALIGN(PKTSIZE, 8), DMA_FROM_DEVICE);

	/* received packet is padded with two null bytes */
	net_receive(edev, rxdesc->buf_ptr + 0x2, rxdesc->byte_cnt - 0x2);

	dma_sync_single_for_device((unsigned long)rxdesc->buf_ptr,
				   ALIGN(PKTSIZE, 8), DMA_FROM_DEVICE);

	ret = 0;

recv_err:
	/* reset this and get next rx descriptor*/
	rxdesc->byte_cnt = 0;
	rxdesc->buf_size = ALIGN(PKTSIZE, 8);
	rxdesc->cmd_sts = RXDESC_OWNED_BY_DMA;
	writel((u32)rxdesc->nxtdesc, &port->current_rxdesc);

	return ret;
}

static int port_set_ethaddr(struct eth_device *edev, const unsigned char *mac)
{
	struct port_priv *port = edev->priv;
	u32 mac_h = (mac[0] << 24) | (mac[1] << 16) | (mac[2] << 8) | mac[3];
	u32 mac_l = (mac[4] << 8) | mac[5];

	port_clear_mac_tables(port);

	writel(mac_l, port->regs + PORT_MACAL);
	writel(mac_h, port->regs + PORT_MACAH);

	/* accept frames for this address */
	port_set_unicast_filter(port, mac[5], ACCEPT_MAC_ADDR);

	return 0;
}

static int port_get_ethaddr(struct eth_device *edev, unsigned char *mac)
{
	struct port_priv *port = edev->priv;
	u32 reg;

	reg = readl(port->regs + PORT_MACAH);
	mac[0] = (u8)(reg >> 24) & 0xff;
	mac[1] = (u8)(reg >> 16) & 0xff;
	mac[2] = (u8)(reg >> 8) & 0xff;
	mac[3] = (u8)reg & 0xff;

	reg = readl(port->regs + PORT_MACAL);
	mac[4] = (u8)(reg >> 8) & 0xff;
	mac[5] = (u8)reg & 0xff;

	return 0;
}

static void port_adjust_link(struct eth_device *edev)
{
	struct port_priv *port = edev->priv;
	struct phy_device *phy = edev->phydev;
	u32 reg;

	/* disable port */
	reg = readl(port->regs + PORT_SC0);
	reg &= ~PORT_ENABLE;
	writel(reg, port->regs + PORT_SC0);

	/* setup and enable port */
	reg &= ~(SET_SPEED_MASK | SET_FULL_DUPLEX | SET_FLOWCTRL_ENABLE);
	if (phy->speed == SPEED_1000)
		reg |= SET_SPEED_1000;
	else if (phy->speed == SPEED_100)
		reg |= SET_SPEED_100;
	else if (phy->speed == SPEED_10)
		reg |= SET_SPEED_10;
	if (phy->duplex)
		reg |= SET_FULL_DUPLEX;
	if (phy->pause)
		reg |= SET_FLOWCTRL_ENABLE;
	reg |= FORCE_LINK_PASS | FORCE_NO_LINK_FAIL | PORT_ENABLE;

	writel(reg, port->regs + PORT_SC0);
}

static int port_open(struct eth_device *edev)
{
	struct port_priv *port = edev->priv;
	int ret;

	ret = phy_device_connect(&port->edev, NULL, -1, port_adjust_link, 0,
			port->intf);
	if (ret)
		return ret;

	/* enable receive queue */
	writel(BIT(URXQ), port->regs + PORT_RQC);

	return 0;
}

static int port_probe(struct device_d *parent, struct port_priv *port)
{
	struct orion_gbe *gbe = parent->priv;
	struct device_d *dev = &port->dev;
	u32 reg;
	int ret;

	/* assume port0 but warn on missing port reg property */
	if (of_property_read_u32(port->np, "reg", &port->portno))
		dev_warn(parent, "port node is missing reg property\n");

	ret = of_get_phy_mode(port->np);
	if (ret > 0)
		port->intf = ret;
	else
		port->intf = PHY_INTERFACE_MODE_RGMII;

	port->regs = dev_get_mem_region(parent, 0) + PORTn_REGS(port->portno);
	if (IS_ERR(port->regs))
		return PTR_ERR(port->regs);

	/* allocate rx/tx descriptors and buffers */
	port->txdesc = dma_alloc_coherent(ALIGN(sizeof(*port->txdesc), 16),
					  DMA_ADDRESS_BROKEN);
	port->rxdesc = dma_alloc_coherent(RX_RING_SIZE *
					  ALIGN(sizeof(*port->rxdesc), 16),
					  DMA_ADDRESS_BROKEN);
	port->rxbuf = dma_alloc(RX_RING_SIZE * ALIGN(PKTSIZE, 8));

	port_stop(port);
	port_init_rxdesc_ring(port);

	/* disable port bandwidth limitation */
	writel(~0, port->regs + PORT_TQTBCNT(UTXQ));
	writel(~0, port->regs + PORT_TQTBC(UTXQ));
	writel(0, port->regs + PORT_MTU);
	/* assign initial rx descriptor */
	writel((u32)port->current_rxdesc, port->regs + PORT_CRDP(URXQ));
	/* setup SDMA with maximum burst and no swap */
	reg = RX_BURST_SIZE_16 | RX_BLM_NO_SWAP |
		TX_BURST_SIZE_16 | TX_BLM_NO_SWAP;
	writel(reg, port->regs + PORT_SDC);

	/* port configuration */
	reg = DEFAULT_RXQ(URXQ) | DEFAULT_ARPQ(URXQ);
	writel(reg, port->regs + PORT_C);
	writel(0, port->regs + PORT_CX);

	reg = SC0_RESERVED | MRU_1518;
	reg |= DISABLE_ANEG_DUPLEX | DISABLE_ANEG_FLOWCTRL | DISABLE_ANEG_SPEED;
	writel(reg, port->regs + PORT_SC0);

	reg = SC1_RESERVED;
	reg |= DEFAULT_COL_LIMIT | COL_ON_BACKPRESS | INBAND_ANEG_BYPASS;
	if (port->intf == PHY_INTERFACE_MODE_RGMII ||
	    port->intf == PHY_INTERFACE_MODE_RGMII_ID ||
	    port->intf == PHY_INTERFACE_MODE_RGMII_RXID ||
	    port->intf == PHY_INTERFACE_MODE_RGMII_TXID)
		reg |= RGMII_ENABLE;
	writel(reg, port->regs + PORT_SC1);

	dev_set_name(dev, "%08x.ethernet-port", (u32)gbe->regs);
	dev->id = port->portno;
	dev->parent = parent;
	dev->device_node = port->np;
	ret = register_device(dev);
	if (ret)
		return ret;

	/* register eth device */
	port->edev.priv = port;
	port->edev.open = port_open;
	port->edev.send = port_send;
	port->edev.recv = port_recv;
	port->edev.halt = port_halt;
	port->edev.set_ethaddr = port_set_ethaddr;
	port->edev.get_ethaddr = port_get_ethaddr;
	port->edev.parent = dev;

	ret = eth_register(&port->edev);
	if (ret)
		return ret;

	return 0;
}

static int orion_gbe_probe(struct device_d *dev)
{
	struct orion_gbe *gbe;
	struct port_priv *ppriv;
	struct device_node *pnp;
	int ret;

	gbe = xzalloc(sizeof(*gbe));
	dev->priv = gbe;

	gbe->regs = dev_get_mem_region(dev, 0);
	if (IS_ERR(gbe->regs))
		return PTR_ERR(gbe->regs);

	gbe->clk = clk_get(dev, 0);
	if (!IS_ERR(gbe->clk))
		clk_enable(gbe->clk);

	eunit_set_dram_access(gbe);

	/*
	 * Orion SoCs only have one port per controller, but the
	 * IP itself supports more than one port per controller.
	 * Although untested, the driver should also be able to
	 * deal with multi-port controllers.
	 */
	for_each_child_of_node(dev->device_node, pnp)
		gbe->num_ports++;

	gbe->ports = xzalloc(gbe->num_ports * sizeof(*gbe->ports));

	ppriv = gbe->ports;
	for_each_child_of_node(dev->device_node, pnp) {
		ppriv->np = pnp;

		ret = port_probe(dev, ppriv);
		if (ret)
			return ret;

		ppriv++;
	}

	return 0;
}

static void orion_gbe_remove(struct device_d *dev)
{
	struct orion_gbe *gbe = dev->priv;
	int n;

	for (n = 0; n < gbe->num_ports; n++)
		port_halt(&gbe->ports[n].edev);

	/* disable all address windows */
	writel(~0, gbe->regs + EUNIT_BARE);

	if (!IS_ERR(gbe->clk))
		clk_disable(gbe->clk);
}

static struct of_device_id orion_gbe_dt_ids[] = {
	{ .compatible = "marvell,orion-eth", },
	{ .compatible = "marvell,kirkwood-eth", },
	{ }
};

static struct driver_d orion_gbe_driver = {
	.name   = "orion-gbe",
	.probe  = orion_gbe_probe,
	.remove = orion_gbe_remove,
	.of_compatible = DRV_OF_COMPAT(orion_gbe_dt_ids),
};
device_platform_driver(orion_gbe_driver);
