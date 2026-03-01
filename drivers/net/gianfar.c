// SPDX-License-Identifier: GPL-2.0-only
/*
 * Freescale Three Speed Ethernet Controller driver
 *
 * Copyright 2012 GE Intelligent Platforms, Inc.
 * Copyright 2004-2010 Freescale Semiconductor, Inc.
 * (C) Copyright 2003, Motorola, Inc.
 * based on work by Andy Fleming
 */

#ifdef CONFIG_PPC
#include <asm/config.h>
#endif
#include <common.h>
#include <malloc.h>
#include <net.h>
#include <dma.h>
#include <init.h>
#include <driver.h>
#include <command.h>
#include <errno.h>
#include <of_address.h>
#include <linux/io.h>
#include <linux/phy.h>
#include <linux/err.h>
#include "gianfar.h"

/*
 * Initialize required registers to appropriate values, zeroing
 * those we don't care about (unless zero is bad, in which case,
 * choose a more appropriate value)
 */
static void gfar_init_registers(void __iomem *regs)
{
	out_be32(regs + GFAR_IEVENT_OFFSET, GFAR_IEVENT_INIT_CLEAR);

	out_be32(regs + GFAR_IMASK_OFFSET, GFAR_IMASK_INIT_CLEAR);

	out_be32(regs + GFAR_IADDR(0), 0);
	out_be32(regs + GFAR_IADDR(1), 0);
	out_be32(regs + GFAR_IADDR(2), 0);
	out_be32(regs + GFAR_IADDR(3), 0);
	out_be32(regs + GFAR_IADDR(4), 0);
	out_be32(regs + GFAR_IADDR(5), 0);
	out_be32(regs + GFAR_IADDR(6), 0);
	out_be32(regs + GFAR_IADDR(7), 0);

	out_be32(regs + GFAR_GADDR(0), 0);
	out_be32(regs + GFAR_GADDR(1), 0);
	out_be32(regs + GFAR_GADDR(2), 0);
	out_be32(regs + GFAR_GADDR(3), 0);
	out_be32(regs + GFAR_GADDR(4), 0);
	out_be32(regs + GFAR_GADDR(5), 0);
	out_be32(regs + GFAR_GADDR(6), 0);
	out_be32(regs + GFAR_GADDR(7), 0);

	out_be32(regs + GFAR_RCTRL_OFFSET, 0x00000000);

	memset((void *)(regs + GFAR_TR64_OFFSET), 0,
			GFAR_CAM2_OFFSET - GFAR_TR64_OFFSET);

	out_be32(regs + GFAR_CAM1_OFFSET, 0xffffffff);
	out_be32(regs + GFAR_CAM2_OFFSET, 0xffffffff);

	out_be32(regs + GFAR_MRBLR_OFFSET, MRBLR_INIT_SETTINGS);

	out_be32(regs + GFAR_MINFLR_OFFSET, MINFLR_INIT_SETTINGS);

	out_be32(regs + GFAR_ATTR_OFFSET, ATTR_INIT_SETTINGS);
	out_be32(regs + GFAR_ATTRELI_OFFSET, ATTRELI_INIT_SETTINGS);
}

/*
 * Configure maccfg2 based on negotiated speed and duplex
 * reported by PHY handling code
 */
static void gfar_adjust_link(struct eth_device *edev)
{
	struct gfar_private *priv = edev->priv;
	void __iomem *regs = priv->regs;
	u32 ecntrl, maccfg2;

	priv->link = edev->phydev->link;
	priv->duplexity = edev->phydev->duplex;

	if (edev->phydev->speed == SPEED_1000)
		priv->speed = 1000;
	else if (edev->phydev->speed == SPEED_100)
		priv->speed = 100;
	else
		priv->speed = 10;

	if (priv->link) {
		/* clear all bits relative with interface mode */
		ecntrl = in_be32(regs + GFAR_ECNTRL_OFFSET);
		ecntrl &= ~GFAR_ECNTRL_R100;

		maccfg2 = in_be32(regs + GFAR_MACCFG2_OFFSET);
		maccfg2 &= ~(GFAR_MACCFG2_IF | GFAR_MACCFG2_FULL_DUPLEX);

		if (priv->duplexity != 0)
			maccfg2 |= GFAR_MACCFG2_FULL_DUPLEX;
		else
			maccfg2 &= ~(GFAR_MACCFG2_FULL_DUPLEX);

		switch (priv->speed) {
		case 1000:
			maccfg2 |= GFAR_MACCFG2_GMII;
			break;
		case 100:
		case 10:
			maccfg2 |= GFAR_MACCFG2_MII;
			/*
			 * Set R100 bit in all modes although
			 * it is only used in RGMII mode
			 */
			if (priv->speed == 100)
				ecntrl |= GFAR_ECNTRL_R100;
			break;
		default:
			dev_info(&edev->dev, "Speed is unknown\n");
			break;
		}

		out_be32(regs + GFAR_ECNTRL_OFFSET, ecntrl);
		out_be32(regs + GFAR_MACCFG2_OFFSET, maccfg2);

		dev_dbg(&edev->dev, "Speed: %d, %s duplex\n", priv->speed,
				(priv->duplexity) ? "full" : "half");

	} else {
		dev_dbg(&edev->dev, "No link.\n");
	}
}

/* Stop the interface */
static void gfar_halt(struct eth_device *edev)
{
	struct gfar_private *priv = edev->priv;
	void __iomem *regs = priv->regs;
	int value;

	clrbits_be32(regs + GFAR_DMACTRL_OFFSET, GFAR_DMACTRL_GRS |
			GFAR_DMACTRL_GTS);
	setbits_be32(regs + GFAR_DMACTRL_OFFSET, GFAR_DMACTRL_GRS |
			GFAR_DMACTRL_GTS);

	value = in_be32(regs + GFAR_IEVENT_OFFSET);
	value &= (GFAR_IEVENT_GRSC | GFAR_IEVENT_GTSC);

	while (value != (GFAR_IEVENT_GRSC | GFAR_IEVENT_GTSC)) {
		value = in_be32(regs + GFAR_IEVENT_OFFSET);
		value &= (GFAR_IEVENT_GRSC | GFAR_IEVENT_GTSC);
	}

	clrbits_be32(regs + GFAR_MACCFG1_OFFSET,
			GFAR_MACCFG1_TX_EN | GFAR_MACCFG1_RX_EN);
}

/* Initializes registers for the controller. */
static int gfar_init(struct eth_device *edev)
{
	struct gfar_private *priv = edev->priv;
	void __iomem *regs = priv->regs;

	gfar_halt(edev);

	/* Init MACCFG2. Default to GMII */
	out_be32(regs + GFAR_MACCFG2_OFFSET, MACCFG2_INIT_SETTINGS);
	out_be32(regs + GFAR_ECNTRL_OFFSET, ECNTRL_INIT_SETTINGS);

	priv->rxidx = 0;
	priv->txidx = 0;

	gfar_init_registers(regs);

	return  0;
}

static int gfar_receive_packets_setup(struct gfar_private *priv, int ix)
{
	dma_addr_t dma;

	dma = dma_map_single(priv->dev, priv->rx_buffer[ix], PKTSIZE,
				DMA_FROM_DEVICE);
	if (dma_mapping_error(priv->dev, dma))
		return -EFAULT;

	out_be32(&priv->rxbd[ix].buf, dma);

	return 0;
}

static int gfar_open(struct eth_device *edev)
{
	struct gfar_private *priv = edev->priv;
	struct gfar_phy *phy = priv->gfar_mdio;
	void __iomem *regs = priv->regs;
	int ix, ret;

	ret = phy_device_connect(edev, (phy ? &phy->miibus : NULL), priv->phyaddr,
				 gfar_adjust_link, 0, priv->interface);
	if (ret)
		return ret;

	/* Point to the buffer descriptors */
	out_be32(regs + GFAR_TBASE0_OFFSET, virt_to_phys(priv->txbd));
	out_be32(regs + GFAR_RBASE0_OFFSET, virt_to_phys(priv->rxbd));

	/* Initialize the Rx Buffer descriptors */
	for (ix = 0; ix < RX_BUF_CNT; ix++) {
		out_be16(&priv->rxbd[ix].status, RXBD_EMPTY);
		out_be16(&priv->rxbd[ix].length, 0);
		if (gfar_receive_packets_setup(priv, ix)) {
			dev_err(&edev->dev, "RX packet dma mapping failed");
			return -EIO;
		}
	}
	out_be16(&priv->rxbd[RX_BUF_CNT - 1].status, RXBD_EMPTY | RXBD_WRAP);

	/* Initialize the TX Buffer Descriptors */
	for (ix = 0; ix < TX_BUF_CNT; ix++) {
		out_be16(&priv->txbd[ix].status, 0);
		out_be16(&priv->txbd[ix].length, 0);
		out_be32(&priv->txbd[ix].buf, 0);
	}
	out_be16(&priv->txbd[TX_BUF_CNT - 1].status, TXBD_WRAP);

	/* Enable Transmit and Receive */
	setbits_be32(regs + GFAR_MACCFG1_OFFSET, GFAR_MACCFG1_RX_EN |
			GFAR_MACCFG1_TX_EN);

	/* Tell the DMA it is clear to go */
	setbits_be32(regs + GFAR_DMACTRL_OFFSET, DMACTRL_INIT_SETTINGS);
	out_be32(regs + GFAR_TSTAT_OFFSET, GFAR_TSTAT_CLEAR_THALT);
	out_be32(regs + GFAR_RSTAT_OFFSET, GFAR_RSTAT_CLEAR_RHALT);
	clrbits_be32(regs + GFAR_DMACTRL_OFFSET, GFAR_DMACTRL_GRS |
			GFAR_DMACTRL_GTS);

	return 0;
}

static int gfar_get_ethaddr(struct eth_device *edev, unsigned char *mac)
{
	return -ENODEV;
}

static int gfar_set_ethaddr(struct eth_device *edev, const unsigned char *mac)
{
	struct gfar_private *priv = edev->priv;
	void __iomem *regs = priv->regs;
	uint tempval;

	tempval = (mac[5] << 24) | (mac[4] << 16) | (mac[3] << 8)  |  mac[2];

	out_be32(regs + GFAR_MACSTRADDR1_OFFSET, tempval);

	tempval = (mac[1] << 24) | (mac[0] << 16);

	out_be32(regs + GFAR_MACSTRADDR2_OFFSET, tempval);

	return 0;
}

/* Writes the given phy's reg with value, using the specified MDIO regs */
static int gfar_local_mdio_write(void __iomem *phyregs, uint addr, uint reg,
				uint value)
{
	uint64_t start;

	out_be32(phyregs + GFAR_MIIMADD_OFFSET, (addr << 8) | (reg & 0x1f));
	out_be32(phyregs + GFAR_MIIMCON_OFFSET, value);

	start = get_time_ns();
	while (!is_timeout(start, 10 * MSECOND)) {
		if (!(in_be32(phyregs + GFAR_MIIMMIND_OFFSET) &
					GFAR_MIIMIND_BUSY))
			return 0;
	}

	return -EIO;
}

/*
 * Reads register regnum on the device's PHY through the
 * specified registers. It lowers and raises the read
 * command, and waits for the data to become valid (miimind
 * notvalid bit cleared), and the bus to cease activity (miimind
 * busy bit cleared), and then returns the value
 */
static uint gfar_local_mdio_read(void __iomem *phyregs, uint phyid, uint regnum)
{
	uint64_t start;

	/* Put the address of the phy, and the register number into MIIMADD */
	out_be32(phyregs + GFAR_MIIMADD_OFFSET, (phyid << 8) | (regnum & 0x1f));

	/* Clear the command register, and wait */
	out_be32(phyregs + GFAR_MIIMCOM_OFFSET, 0);

	/* Initiate a read command, and wait */
	out_be32(phyregs + GFAR_MIIMCOM_OFFSET, GFAR_MIIM_READ_COMMAND);

	start = get_time_ns();
	while (!is_timeout(start, 10 * MSECOND)) {
		if (!(in_be32(phyregs + GFAR_MIIMMIND_OFFSET) &
			(GFAR_MIIMIND_NOTVALID | GFAR_MIIMIND_BUSY)))
			return in_be32(phyregs + GFAR_MIIMSTAT_OFFSET);
	}

	return -EIO;
}

static void gfar_configure_serdes(struct gfar_private *priv)
{
	struct gfar_phy *phy = priv->gfar_tbi;

	gfar_local_mdio_write(phy->regs,
			in_be32(priv->regs + GFAR_TBIPA_OFFSET), GFAR_TBI_ANA,
			priv->tbiana);
	gfar_local_mdio_write(phy->regs,
			in_be32(priv->regs + GFAR_TBIPA_OFFSET),
			GFAR_TBI_TBICON, GFAR_TBICON_CLK_SELECT);
	gfar_local_mdio_write(phy->regs,
			in_be32(priv->regs + GFAR_TBIPA_OFFSET), GFAR_TBI_CR,
			priv->tbicr);
}

static int gfar_bus_reset(struct mii_bus *bus)
{
	struct gfar_phy *phy = bus->priv;
	uint64_t start;

	/* Reset MII (due to new addresses) */
	out_be32(phy->regs + GFAR_MIIMCFG_OFFSET, GFAR_MIIMCFG_RESET);
	out_be32(phy->regs + GFAR_MIIMCFG_OFFSET, GFAR_MIIMCFG_INIT_VALUE);

	start = get_time_ns();
	while (!is_timeout(start, 10 * MSECOND)) {
		if (!(in_be32(phy->regs + GFAR_MIIMMIND_OFFSET) &
			GFAR_MIIMIND_BUSY))
			break;
	}
	return 0;
}

static void gfar_init_phy(struct eth_device *dev)
{
	struct gfar_private *priv = dev->priv;
	void __iomem *regs = priv->regs;
	uint32_t ecntrl;

	ecntrl = in_be32(regs + GFAR_ECNTRL_OFFSET);
	if (ecntrl & GFAR_ECNTRL_SGMII_MODE) {
		priv->interface = PHY_INTERFACE_MODE_SGMII;
		gfar_configure_serdes(priv);
	} else if (ecntrl & ECNTRL_REDUCED_MODE) {
		priv->interface = PHY_INTERFACE_MODE_RGMII;
	}
}

static int gfar_send(struct eth_device *edev, void *packet, int length)
{
	struct gfar_private *priv = edev->priv;
	void __iomem *regs = priv->regs;
	struct device *dev = edev->parent;
	dma_addr_t dma;
	uint64_t start;
	uint tidx;
	uint16_t status;

	tidx = priv->txidx;

	dma = dma_map_single(dev, packet, length, DMA_TO_DEVICE);
	if (dma_mapping_error(priv->dev, dma)) {
		dev_err(dev, "TX mapping packet failed");
		return -EFAULT;
	}
	out_be32(&priv->txbd[tidx].buf, (u32)dma);
	out_be16(&priv->txbd[tidx].length, length);
	out_be16(&priv->txbd[tidx].status,
		in_be16(&priv->txbd[tidx].status) |
		(TXBD_READY | TXBD_LAST | TXBD_CRC | TXBD_INTERRUPT));

	/* Tell the DMA to go */
	out_be32(regs + GFAR_TSTAT_OFFSET, GFAR_TSTAT_CLEAR_THALT);

	/* Wait for buffer to be transmitted */
	start = get_time_ns();
	while (in_be16(&priv->txbd[tidx].status) & TXBD_READY) {
		if (is_timeout(start, 5 * MSECOND))
			break;
	}
	dma_unmap_single(dev, dma, length, DMA_TO_DEVICE);

	status = in_be16(&priv->txbd[tidx].status);
	if (status & TXBD_READY) {
		dev_err(dev, "tx timeout: 0x%x\n", status);
		return -EBUSY;
	} else if (status & TXBD_STATS) {
		dev_err(dev, "TX error: 0x%x\n", status);
		return -EIO;
	}

	priv->txidx = (priv->txidx + 1) % TX_BUF_CNT;

	return 0;
}

static void gfar_recv(struct eth_device *edev)
{
	struct gfar_private *priv = edev->priv;
	struct device *dev = edev->parent;
	void __iomem *regs = priv->regs;
	uint16_t status, length;

	if (in_be16(&priv->rxbd[priv->rxidx].status) & RXBD_EMPTY)
		return;

	length = in_be16(&priv->rxbd[priv->rxidx].length);

	/* Send the packet up if there were no errors */
	status = in_be16(&priv->rxbd[priv->rxidx].status);
	if (!(status & RXBD_STATS)) {
		void *frame;

		frame = phys_to_virt(in_be32(&priv->rxbd[priv->rxidx].buf));
		dma_sync_single_for_cpu(dev, (unsigned long)frame, length,
					DMA_FROM_DEVICE);
		net_receive(edev, frame, length - 4);
		dma_sync_single_for_device(dev, (unsigned long)frame, length,
					   DMA_FROM_DEVICE);
	} else {
		dev_err(dev, "Got error %x\n", status & RXBD_STATS);
	}

	out_be16(&priv->rxbd[priv->rxidx].length, 0);

	status = RXBD_EMPTY;
	/* Set the wrap bit if this is the last element in the list */
	if ((priv->rxidx + 1) == RX_BUF_CNT)
		status |= RXBD_WRAP;

	out_be16(&priv->rxbd[priv->rxidx].status, status);
	priv->rxidx = (priv->rxidx + 1) % RX_BUF_CNT;

	if (in_be32(regs + GFAR_IEVENT_OFFSET) & GFAR_IEVENT_BSY) {
		out_be32(regs + GFAR_IEVENT_OFFSET, GFAR_IEVENT_BSY);
		out_be32(regs + GFAR_RSTAT_OFFSET, GFAR_RSTAT_CLEAR_RHALT);
	}
}

/* Read a MII PHY register. */
static int gfar_miiphy_read(struct mii_bus *bus, int addr, int reg)
{
	struct gfar_phy *phy = bus->priv;
	int ret;

	ret = gfar_local_mdio_read(phy->regs, addr, reg);
	if (ret == -EIO)
		dev_err(phy->dev, "Can't read PHY at address %d\n", addr);

	return ret;
}

/* Write a MII PHY register.  */
static int gfar_miiphy_write(struct mii_bus *bus, int addr, int reg,
				u16 value)
{
	struct gfar_phy *phy = bus->priv;
	unsigned short val = value;
	int ret;

	ret = gfar_local_mdio_write(phy->regs, addr, reg, val);

	if (ret)
		dev_err(phy->dev, "Can't write PHY at address %d\n", addr);

	return 0;
}

#ifdef CONFIG_OFDEVICE
static int gfar_probe_dt(struct gfar_private *priv)
{
	struct device *dev = priv->dev;
	struct device_node *np;
	uint32_t tbiaddr = 0x1f;

	priv->dev = dev;
	if (IS_ERR(priv->regs)) {
		struct device_node *child;

		child = of_get_next_child(dev->device_node, NULL);
		for_each_child_of_node(dev->device_node, child) {
			if (child->name && !strncmp(child->name, "queue-group",
						strlen("queue-group"))) {
				priv->regs = of_iomap(child, 0);
				if (IS_ERR(priv->regs)) {
					dev_err(dev, "Failed to acquire first group address\n");
					return -ENOENT;
				}
				break;
			}
		}
	}

	priv->phyaddr = -1;
	np = of_parse_phandle_from(dev->device_node, NULL, "phy-handle", 0);
	if (np) {
		uint32_t reg = 0;

		if (!of_property_read_u32(np, "reg", &reg))
			priv->phyaddr = reg;
	} else {
		dev_err(dev, "Could not get phy-handle address\n");
		return -ENOENT;
	}

	priv->tbicr = TSEC_TBICR_SETTINGS;
	priv->tbiana = TBIANA_SETTINGS;

	/* Handle to tbi node */
	np = of_parse_phandle_from(dev->device_node, NULL, "tbi-handle", 0);
	if (np) {
		struct device_node *parent;
		struct gfar_phy *tbiphy;
		struct device *bus_dev;
		struct mii_bus *bus;

		/* Get tbi address to be programmed in device */
		if (of_property_read_u32(np, "reg", &tbiaddr)) {
			dev_err(dev, "Failed to get tbi reg property\n");
			return -ENOENT;
		}
		/* MDIO is the parent  */
		parent = of_get_parent(np);
		if (!parent)  {
			dev_err(dev, "No parent node for TBI PHY?\n");
			return -ENOENT;
		}
		tbiphy = xzalloc(sizeof(*tbiphy));
		tbiphy->dev = parent->dev;
		tbiphy->regs = NULL;

		for_each_mii_bus(bus) {
			if (bus->dev.of_node == parent) {
				struct gfar_phy *phy;

				bus_dev = bus->parent;
				phy = bus_dev->priv;
				tbiphy->regs = phy->regs;
				break;
			}
		}
		if (!tbiphy->regs) {
			dev_err(dev, "Could not get TBI address\n");
			free(tbiphy);
			return PTR_ERR(tbiphy->regs);
		}

		tbiphy->miibus.read = gfar_miiphy_read;
		tbiphy->miibus.write = gfar_miiphy_write;
		tbiphy->miibus.priv = tbiphy;
		tbiphy->miibus.parent = dev;
		tbiphy->dev->priv = tbiphy;
		priv->gfar_tbi = tbiphy;
	}

	priv->tbiaddr = tbiaddr;
	out_be32(priv->regs + GFAR_TBIPA_OFFSET, priv->tbiaddr);

	return 0;
}

static int gfar_probe_pd(struct gfar_private *priv)
{
	return -ENODEV;
}
#else
static int gfar_probe_pd(struct gfar_private *priv)
{
	struct gfar_info_struct *gfar_info = priv->dev->platform_data;
	struct device *mdev;
	char devname[16];

	priv->mdiobus_tbi = gfar_info->mdiobus_tbi;
	priv->phyaddr = gfar_info->phyaddr;
	priv->tbicr = gfar_info->tbicr;
	priv->tbiana = gfar_info->tbiana;

	mdev = get_device_by_name("gfar-mdio0");
	if (!mdev) {
		pr_err("gfar-mdio0 was not found\n");
		return -ENODEV;
	}
	priv->gfar_mdio = mdev->priv;

	if (priv->mdiobus_tbi != 0) {
		sprintf(devname, "%s%d", "gfar-tbiphy", priv->mdiobus_tbi);
		mdev = get_device_by_name(devname);
		if (!mdev) {
			pr_err("%s was not found\n", devname);
			return -ENODEV;
		}
	}
	priv->gfar_tbi = mdev->priv;

	return 0;
}

static int gfar_probe_dt(struct gfar_private *priv)
{
	return -ENODEV;
}
#endif

/*
 * PPC only as there is no device tree support.
 * Initialize device structure. Returns success if
 * initialization succeeded.
 */
static int gfar_probe(struct device *dev)
{
	struct gfar_private *priv;
	struct eth_device *edev;
	size_t size;
	void *base;
	int ret;

	priv = xzalloc(sizeof(struct gfar_private));

	ret = net_alloc_packets(priv->rx_buffer, ARRAY_SIZE(priv->rx_buffer));
	if (ret)
		goto free_priv;

	priv->dev = dev;
	edev = &priv->edev;

	priv->regs = dev_get_mem_region(dev, 0);
	if (IS_ERR(priv->regs)) {
		ret = PTR_ERR(priv->regs);
		goto free_received_packets;
	}

	if (dev->of_node) {
		ret = gfar_probe_dt(priv);
		if (ret)
			goto free_received_packets;
	} else {
		ret = gfar_probe_pd(priv);
		if (ret)
			goto free_received_packets;
	}

	size = ((TX_BUF_CNT * sizeof(struct txbd8)) +
			(RX_BUF_CNT * sizeof(struct rxbd8)));
	if (IS_ENABLED(CONFIG_PPC)) {
		base = xmemalign(BUF_ALIGN, size);
	} else {
		base = dma_alloc_coherent(DMA_DEVICE_BROKEN, size, DMA_ADDRESS_BROKEN);
		dma_set_mask(dev, DMA_BIT_MASK(32));
	}
	priv->txbd = (struct txbd8 __iomem *)base;
	base += TX_BUF_CNT * sizeof(struct txbd8);
	priv->rxbd = (struct rxbd8 __iomem *)base;

	edev->priv = priv;
	edev->init = gfar_init;
	edev->open = gfar_open;
	edev->halt = gfar_halt;
	edev->send = gfar_send;
	edev->recv = gfar_recv;
	edev->get_ethaddr = gfar_get_ethaddr;
	edev->set_ethaddr = gfar_set_ethaddr;
	edev->parent = dev;

	setbits_be32(priv->regs + GFAR_MACCFG1_OFFSET, GFAR_MACCFG1_SOFT_RESET);
	udelay(2);
	clrbits_be32(priv->regs + GFAR_MACCFG1_OFFSET, GFAR_MACCFG1_SOFT_RESET);

	gfar_init_phy(edev);

	return eth_register(edev);

free_received_packets:
	free(priv->rx_buffer);
free_priv:
	free(priv);

	return ret;
}

static const struct of_device_id gfar_ids[] = {
	{ .compatible = "fsl,etsec2" },
	{ /* sentinel */ }
};

static struct driver gfar_eth_driver = {
	.name  = "gfar",
	.of_compatible = DRV_OF_COMPAT(gfar_ids),
	.probe = gfar_probe,
};
device_platform_driver(gfar_eth_driver);

static __maybe_unused const struct of_device_id gfar_mdio_ids[] = {
	{ .compatible = "gianfar" },
	{ /* sentinel */ }
};

static int gfar_phy_probe(struct device *dev)
{
	struct gfar_phy *phy;
	struct fsl_pq_mdio_data *data;
	int ret;

	data = (struct fsl_pq_mdio_data *)device_get_match_data(dev);
	phy = xzalloc(sizeof(*phy));
	phy->dev = dev;
	phy->regs = dev_get_mem_region(dev, 0);
	if (IS_ERR(phy->regs))
		return PTR_ERR(phy->regs);

	phy->miibus.read = gfar_miiphy_read;
	phy->miibus.write = gfar_miiphy_write;
	phy->miibus.priv = phy;
	phy->miibus.reset = gfar_bus_reset;
	phy->miibus.parent = dev;
	dev->priv = phy;

	ret = mdiobus_register(&phy->miibus);
	if (ret)
		return ret;

	return 0;
}

static struct driver gfar_phy_driver = {
	.name  = "gfar-mdio",
	.of_compatible = DRV_OF_COMPAT(gfar_mdio_ids),
	.probe = gfar_phy_probe,
};
register_driver_macro(coredevice, platform, gfar_phy_driver);

#ifndef CONFIG_OFDEVICE
static int gfar_tbiphy_probe(struct device *dev)
{
	struct gfar_phy *phy;
	int ret;

	phy = xzalloc(sizeof(*phy));
	phy->dev = dev;
	phy->regs = dev_get_mem_region(dev, 0);
	if (IS_ERR(phy->regs))
		return PTR_ERR(phy->regs);

	phy->miibus.read = gfar_miiphy_read;
	phy->miibus.write = gfar_miiphy_write;
	phy->miibus.priv = phy;
	phy->miibus.parent = dev;
	dev->priv = phy;

	ret = mdiobus_register(&phy->miibus);
	if (ret)
		return ret;

	return 0;
}

static struct driver gfar_tbiphy_driver = {
	.name  = "gfar-tbiphy",
	.probe = gfar_tbiphy_probe,
};
register_driver_macro(coredevice, platform, gfar_tbiphy_driver);
#endif
