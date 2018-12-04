// SPDX-License-Identifier: GPL-2.0-only
/*
 * Freescale Three Speed Ethernet Controller driver
 *
 * Copyright 2012 GE Intelligent Platforms, Inc.
 * Copyright 2004-2010 Freescale Semiconductor, Inc.
 * (C) Copyright 2003, Motorola, Inc.
 * based on work by Andy Fleming
 */

#include <config.h>
#include <common.h>
#include <malloc.h>
#include <net.h>
#include <init.h>
#include <driver.h>
#include <command.h>
#include <errno.h>
#include <asm/io.h>
#include <linux/phy.h>
#include <linux/err.h>
#include "gianfar.h"

/* 2 seems to be the minimum number of TX descriptors to make it work. */
#define TX_BUF_CNT 	2
#define RX_BUF_CNT 	PKTBUFSRX
#define BUF_ALIGN	8

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
	priv->duplexity =edev->phydev->duplex;

	if (edev->phydev->speed == SPEED_1000)
		priv->speed = 1000;
	if (edev->phydev->speed == SPEED_100)
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

		dev_info(&edev->dev, "Speed: %d, %s duplex\n", priv->speed,
		       (priv->duplexity) ? "full" : "half");

	} else {
		dev_info(&edev->dev, "No link.\n");
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

static int gfar_open(struct eth_device *edev)
{
	int ix;
	struct gfar_private *priv = edev->priv;
	struct gfar_phy *phy = priv->gfar_mdio;
	void __iomem *regs = priv->regs;
	int ret;

	ret = phy_device_connect(edev, &phy->miibus, priv->phyaddr,
				 gfar_adjust_link, 0, PHY_INTERFACE_MODE_NA);
	if (ret)
		return ret;

	/* Point to the buffer descriptors */
	out_be32(regs + GFAR_TBASE0_OFFSET, (unsigned int)priv->txbd);
	out_be32(regs + GFAR_RBASE0_OFFSET, (unsigned int)priv->rxbd);

	/* Initialize the Rx Buffer descriptors */
	for (ix = 0; ix < RX_BUF_CNT; ix++) {
		out_be16(&priv->rxbd[ix].status, RXBD_EMPTY);
		out_be16(&priv->rxbd[ix].length, 0);
		out_be32(&priv->rxbd[ix].bufPtr, (uint) NetRxPackets[ix]);
	}
	out_be16(&priv->rxbd[RX_BUF_CNT - 1].status, RXBD_EMPTY | RXBD_WRAP);

	/* Initialize the TX Buffer Descriptors */
	for (ix = 0; ix < TX_BUF_CNT; ix++) {
		out_be16(&priv->txbd[ix].status, 0);
		out_be16(&priv->txbd[ix].length, 0);
		out_be32(&priv->txbd[ix].bufPtr, 0);
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
	char tmpbuf[MAC_ADDR_LEN];
	uint tempval;
	int ix;

	for (ix = 0; ix < MAC_ADDR_LEN; ix++)
		tmpbuf[MAC_ADDR_LEN - 1 - ix] = mac[ix];

	tempval = (tmpbuf[0] << 24) | (tmpbuf[1] << 16) | (tmpbuf[2] << 8) |
		  tmpbuf[3];

	out_be32(regs + GFAR_MACSTRADDR1_OFFSET, tempval);

	tempval = *((uint *)(tmpbuf + 4));

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

/* Reset the external PHYs. */
static void gfar_init_phy(struct eth_device *dev)
{
	struct gfar_private *priv = dev->priv;
	struct gfar_phy *phy = priv->gfar_mdio;
	void __iomem *regs = priv->regs;
	uint64_t start;

	gfar_local_mdio_write(phy->regs, priv->phyaddr, GFAR_MIIM_CR,
			GFAR_MIIM_CR_RST);

	start = get_time_ns();
	while (!is_timeout(start, 10 * MSECOND)) {
		if (!(gfar_local_mdio_read(phy->regs, priv->phyaddr,
					GFAR_MIIM_CR) & GFAR_MIIM_CR_RST))
			break;
	}

	if (in_be32(regs + GFAR_ECNTRL_OFFSET) & GFAR_ECNTRL_SGMII_MODE)
		gfar_configure_serdes(priv);
}

static int gfar_send(struct eth_device *edev, void *packet, int length)
{
	struct gfar_private *priv = edev->priv;
	void __iomem *regs = priv->regs;
	struct device_d *dev = edev->parent;
	uint64_t start;
	uint tidx;
	uint16_t status;

	tidx = priv->txidx;
	out_be32(&priv->txbd[tidx].bufPtr, (u32) packet);
	out_be16(&priv->txbd[tidx].length, length);
	out_be16(&priv->txbd[tidx].status,
		in_be16(&priv->txbd[tidx].status) |
		(TXBD_READY | TXBD_LAST | TXBD_CRC | TXBD_INTERRUPT));

	/* Tell the DMA to go */
	out_be32(regs + GFAR_TSTAT_OFFSET, GFAR_TSTAT_CLEAR_THALT);

	/* Wait for buffer to be transmitted */
	start = get_time_ns();
	while (in_be16(&priv->txbd[tidx].status) & TXBD_READY) {
		if (is_timeout(start, 5 * MSECOND)) {
			break;
		}
	}

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

static int gfar_recv(struct eth_device *edev)
{
	struct gfar_private *priv = edev->priv;
	struct device_d *dev = edev->parent;
	void __iomem *regs = priv->regs;
	uint16_t status, length;

	if (in_be16(&priv->rxbd[priv->rxidx].status) & RXBD_EMPTY)
		return 0;

	length = in_be16(&priv->rxbd[priv->rxidx].length);

	/* Send the packet up if there were no errors */
	status = in_be16(&priv->rxbd[priv->rxidx].status);
	if (!(status & RXBD_STATS))
		net_receive(edev, NetRxPackets[priv->rxidx], length - 4);
	else
		dev_err(dev, "Got error %x\n", status & RXBD_STATS);

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

	return 0;
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

/*
 * Initialize device structure. Returns success if
 * initialization succeeded.
 */
static int gfar_probe(struct device_d *dev)
{
	struct gfar_info_struct *gfar_info = dev->platform_data;
	struct eth_device *edev;
	struct gfar_private *priv;
	struct device_d *mdev;
	size_t size;
	char devname[16];
	char *p;

	priv = xzalloc(sizeof(struct gfar_private));

	edev = &priv->edev;

	priv->mdiobus_tbi = gfar_info->mdiobus_tbi;
	priv->regs = dev_get_mem_region(dev, 0);
	if (IS_ERR(priv->regs))
		return PTR_ERR(priv->regs);
	priv->phyaddr = gfar_info->phyaddr;
	priv->tbicr = gfar_info->tbicr;
	priv->tbiana = gfar_info->tbiana;

	mdev = get_device_by_name("gfar-mdio0");
	if (mdev == NULL) {
		pr_err("gfar-mdio0 was not found\n");
		return -ENODEV;
	}
	priv->gfar_mdio = mdev->priv;

	if (priv->mdiobus_tbi != 0) {
		sprintf(devname, "%s%d", "gfar-tbiphy", priv->mdiobus_tbi);
		mdev = get_device_by_name(devname);
		if (mdev == NULL) {
			pr_err("%s was not found\n", devname);
			return -ENODEV;
		}
	}
	priv->gfar_tbi = mdev->priv;
	/*
	 * Allocate descriptors 64-bit aligned. Descriptors
	 * are 8 bytes in size.
	 */
	size = ((TX_BUF_CNT * sizeof(struct txbd8)) +
		(RX_BUF_CNT * sizeof(struct rxbd8))) + BUF_ALIGN;
	p = (char *)xmemalign(BUF_ALIGN, size);
	priv->txbd = (struct txbd8 __iomem *)p;
	priv->rxbd = (struct rxbd8 __iomem *)(p +
			(TX_BUF_CNT * sizeof(struct txbd8)));

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
}

static struct driver_d gfar_eth_driver = {
	.name  = "gfar",
	.probe = gfar_probe,
};
device_platform_driver(gfar_eth_driver);

static int gfar_phy_probe(struct device_d *dev)
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
	phy->miibus.reset = gfar_bus_reset;
	phy->miibus.parent = dev;
	dev->priv = phy;

	ret = mdiobus_register(&phy->miibus);
	if (ret)
		return ret;

	return 0;
}

static struct driver_d gfar_phy_driver = {
	.name  = "gfar-mdio",
	.probe = gfar_phy_probe,
};
register_driver_macro(coredevice, platform, gfar_phy_driver);

static int gfar_tbiphy_probe(struct device_d *dev)
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

static struct driver_d gfar_tbiphy_driver = {
	.name  = "gfar-tbiphy",
	.probe = gfar_tbiphy_probe,
};
register_driver_macro(coredevice, platform, gfar_tbiphy_driver);
