// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * (C) Copyright 2010
 * Vipin Kumar, ST Micoelectronics, vipin.kumar@st.com.
 */

/*
 * Designware ethernet IP driver for u-boot
 */

#include <common.h>
#include <dma.h>
#include <init.h>
#include <io.h>
#include <net.h>
#include <of_net.h>
#include <platform_data/eth-designware.h>
#include <linux/phy.h>
#include <linux/err.h>
#include "designware.h"

/* Speed specific definitions */
#define SPEED_10M		1
#define SPEED_100M		2
#define SPEED_1000M		3

/* Duplex mode specific definitions */
#define HALF_DUPLEX		1
#define FULL_DUPLEX		2


static int dwc_ether_mii_read(struct mii_bus *dev, int addr, int reg)
{
	struct dw_eth_dev *priv = dev->priv;
	struct eth_mac_regs *mac_p = priv->mac_regs_p;
	u64 start;
	u32 miiaddr;

	miiaddr = ((addr << MIIADDRSHIFT) & MII_ADDRMSK) |
		  ((reg << MIIREGSHIFT) & MII_REGMSK);

	writel(miiaddr | MII_CLKRANGE_150_250M | MII_BUSY, &mac_p->miiaddr);

	start = get_time_ns();
	while (readl(&mac_p->miiaddr) & MII_BUSY) {
		if (is_timeout(start, 10 * MSECOND)) {
			dev_err(&priv->netdev.dev, "MDIO timeout\n");
			return -EIO;
		}
	}
	return readl(&mac_p->miidata) & 0xffff;
}

static int dwc_ether_mii_write(struct mii_bus *dev, int addr, int reg, u16 val)
{
	struct dw_eth_dev *priv = dev->priv;
	struct eth_mac_regs *mac_p = priv->mac_regs_p;
	u64 start;
	u32 miiaddr;

	writel(val, &mac_p->miidata);
	miiaddr = ((addr << MIIADDRSHIFT) & MII_ADDRMSK) |
		  ((reg << MIIREGSHIFT) & MII_REGMSK) | MII_WRITE;

	writel(miiaddr | MII_CLKRANGE_150_250M | MII_BUSY, &mac_p->miiaddr);

	start = get_time_ns();
	while (readl(&mac_p->miiaddr) & MII_BUSY) {
		if (is_timeout(start, 10 * MSECOND)) {
			dev_err(&priv->netdev.dev, "MDIO timeout\n");
			return -EIO;
		}
	}

	/* Needed as a fix for ST-Phy */
	dwc_ether_mii_read(dev, addr, reg);
	return 0;
}


static int mac_reset(struct eth_device *dev)
{
	struct dw_eth_dev *priv = dev->priv;
	struct eth_mac_regs *mac_p = priv->mac_regs_p;
	struct eth_dma_regs *dma_p = priv->dma_regs_p;
	u64 start;

	writel(readl(&dma_p->busmode) | DMAMAC_SRST, &dma_p->busmode);

	if (priv->interface != PHY_INTERFACE_MODE_RGMII)
		writel(MII_PORTSELECT, &mac_p->conf);

	start = get_time_ns();
	while (readl(&dma_p->busmode) & DMAMAC_SRST) {
		if (is_timeout(start, 10 * MSECOND)) {
			dev_err(&priv->netdev.dev, "MAC reset timeout\n");
			return -EIO;
		}
	}
	return 0;
}

static void tx_descs_init(struct eth_device *dev)
{
	struct dw_eth_dev *priv = dev->priv;
	struct eth_dma_regs *dma_p = priv->dma_regs_p;
	struct dmamacdescr *desc_table_p = &priv->tx_mac_descrtable[0];
	char *txbuffs = &priv->txbuffs[0];
	struct dmamacdescr *desc_p;
	u32 idx;

	for (idx = 0; idx < CONFIG_TX_DESCR_NUM; idx++) {
		desc_p = &desc_table_p[idx];
		desc_p->dmamac_addr = &txbuffs[idx * CONFIG_ETH_BUFSIZE];
		desc_p->dmamac_next = &desc_table_p[idx + 1];

		if (priv->enh_desc) {
			desc_p->txrx_status &= ~(DESC_ENH_TXSTS_TXINT | DESC_ENH_TXSTS_TXLAST |
					DESC_ENH_TXSTS_TXFIRST | DESC_ENH_TXSTS_TXCRCDIS |
					DESC_ENH_TXSTS_TXCHECKINSCTRL |
					DESC_ENH_TXSTS_TXRINGEND | DESC_ENH_TXSTS_TXPADDIS);

			desc_p->txrx_status |= DESC_ENH_TXSTS_TXCHAIN;
			desc_p->dmamac_cntl = 0;
			desc_p->txrx_status &= ~(DESC_ENH_TXSTS_MSK | DESC_ENH_TXSTS_OWNBYDMA);
		} else {
			desc_p->dmamac_cntl = DESC_TXCTRL_TXCHAIN;
			desc_p->txrx_status = 0;
		}
	}

	/* Correcting the last pointer of the chain */
	desc_p->dmamac_next = &desc_table_p[0];

	writel((ulong)&desc_table_p[0], &dma_p->txdesclistaddr);
	priv->tx_currdescnum = 0;
}

static void rx_descs_init(struct eth_device *dev)
{
	struct dw_eth_dev *priv = dev->priv;
	struct eth_dma_regs *dma_p = priv->dma_regs_p;
	struct dmamacdescr *desc_table_p = &priv->rx_mac_descrtable[0];
	char *rxbuffs = &priv->rxbuffs[0];
	struct dmamacdescr *desc_p;
	u32 idx;

	for (idx = 0; idx < CONFIG_RX_DESCR_NUM; idx++) {
		desc_p = &desc_table_p[idx];
		desc_p->dmamac_addr = &rxbuffs[idx * CONFIG_ETH_BUFSIZE];
		desc_p->dmamac_next = &desc_table_p[idx + 1];

		desc_p->dmamac_cntl = MAC_MAX_FRAME_SZ;
		if (priv->enh_desc)
			desc_p->dmamac_cntl |= DESC_ENH_RXCTRL_RXCHAIN;
		else
			desc_p->dmamac_cntl |= DESC_RXCTRL_RXCHAIN;

		dma_sync_single_for_cpu((unsigned long)desc_p->dmamac_addr,
					CONFIG_ETH_BUFSIZE, DMA_FROM_DEVICE);
		desc_p->txrx_status = DESC_RXSTS_OWNBYDMA;
	}

	/* Correcting the last pointer of the chain */
	desc_p->dmamac_next = &desc_table_p[0];

	writel((ulong)&desc_table_p[0], &dma_p->rxdesclistaddr);
	priv->rx_currdescnum = 0;
}

static void descs_init(struct eth_device *dev)
{
	tx_descs_init(dev);
	rx_descs_init(dev);
}

/* Get PHY out of power saving mode.  If this is needed elsewhere then
 * consider making it part of phy-core and adding a resume method to
 * the phy device ops.  */
static int phy_resume(struct phy_device *phydev)
{
	int bmcr;

	bmcr = phy_read(phydev, MII_BMCR);
	if (bmcr < 0)
		return bmcr;
	if (bmcr & BMCR_PDOWN) {
		bmcr &= ~BMCR_PDOWN;
		return phy_write(phydev, MII_BMCR, bmcr);
	}
	return 0;
}

static int dwc_ether_init(struct eth_device *dev)
{
	struct dw_eth_dev *priv = dev->priv;
	struct eth_mac_regs *mac_p = priv->mac_regs_p;
	struct eth_dma_regs *dma_p = priv->dma_regs_p;

	/* Before we reset the mac, we must insure the PHY is not powered down
	 * as the dw controller needs all clock domains to be running, including
	 * the PHY clock, to come out of a mac reset.  */
	phy_resume(dev->phydev);

	if (mac_reset(dev) < 0)
		return -1;

	/* HW MAC address is lost during MAC reset */
	dev->set_ethaddr(dev, priv->macaddr);

	writel(FIXEDBURST | PRIORXTX_41 | BURST_16, &dma_p->busmode);
	writel(readl(&dma_p->opmode) | FLUSHTXFIFO | STOREFORWARD |
	       TXSECONDFRAME, &dma_p->opmode);
	writel(FRAMEBURSTENABLE | DISABLERXOWN, &mac_p->conf);
	return 0;
}

static void dwc_update_linkspeed(struct eth_device *edev)
{
	struct dw_eth_dev *priv = edev->priv;
	struct eth_mac_regs *mac_p = priv->mac_regs_p;
	u32 conf;

	if (priv->fix_mac_speed)
		priv->fix_mac_speed(edev->phydev->speed);

	conf = readl(&mac_p->conf);
	if (edev->phydev->duplex)
		conf |= FULLDPLXMODE;
	else
		conf &= ~FULLDPLXMODE;
	if (edev->phydev->speed == SPEED_1000)
		conf &= ~MII_PORTSELECT;
	else
		conf |= MII_PORTSELECT;

	if ((edev->phydev->interface != PHY_INTERFACE_MODE_MII) &&
		(edev->phydev->interface != PHY_INTERFACE_MODE_GMII)) {

		if (edev->phydev->speed == 100)
			conf |= FES_100;
		else
			conf &= ~FES_100;
	}

	writel(conf, &mac_p->conf);
}

static int dwc_ether_open(struct eth_device *dev)
{
	struct dw_eth_dev *priv = dev->priv;
	struct eth_mac_regs *mac_p = priv->mac_regs_p;
	struct eth_dma_regs *dma_p = priv->dma_regs_p;
	int ret;

	ret = phy_device_connect(dev, &priv->miibus, priv->phy_addr,
				 dwc_update_linkspeed, 0, priv->interface);
	if (ret)
		return ret;

	dwc_ether_init(dev);

	descs_init(dev);

	/*
	 * Start/Enable xfer at dma as well as mac level
	 */
	writel(readl(&dma_p->opmode) | RXSTART, &dma_p->opmode);
	writel(readl(&dma_p->opmode) | TXSTART, &dma_p->opmode);
	writel(readl(&mac_p->conf) | RXENABLE | TXENABLE, &mac_p->conf);
	return 0;
}

static int dwc_ether_send(struct eth_device *dev, void *packet, int length)
{
	struct dw_eth_dev *priv = dev->priv;
	struct eth_dma_regs *dma_p = priv->dma_regs_p;
	u32 owndma, desc_num = priv->tx_currdescnum;
	struct dmamacdescr *desc_p = &priv->tx_mac_descrtable[desc_num];

	owndma = priv->enh_desc ? DESC_ENH_TXSTS_OWNBYDMA : DESC_TXSTS_OWNBYDMA;
	/* Check if the descriptor is owned by CPU */
	if (desc_p->txrx_status & owndma) {
		dev_err(&dev->dev, "CPU not owner of tx frame\n");
		return -1;
	}

	memcpy((void *)desc_p->dmamac_addr, packet, length);
	dma_sync_single_for_device((unsigned long)desc_p->dmamac_addr, length,
				   DMA_TO_DEVICE);

	if (priv->enh_desc) {
		desc_p->txrx_status |= DESC_ENH_TXSTS_TXFIRST | DESC_ENH_TXSTS_TXLAST;
		desc_p->dmamac_cntl |= (length << DESC_ENH_TXCTRL_SIZE1SHFT) &
				       DESC_ENH_TXCTRL_SIZE1MASK;

		desc_p->txrx_status &= ~(DESC_ENH_TXSTS_MSK);
		desc_p->txrx_status |= DESC_ENH_TXSTS_OWNBYDMA;
	} else {
		desc_p->dmamac_cntl |= ((length << DESC_TXCTRL_SIZE1SHFT) &
				       DESC_TXCTRL_SIZE1MASK) | DESC_TXCTRL_TXLAST |
				       DESC_TXCTRL_TXFIRST;

		desc_p->txrx_status = DESC_TXSTS_OWNBYDMA;
	}

	/* Test the wrap-around condition. */
	if (++desc_num >= CONFIG_TX_DESCR_NUM)
		desc_num = 0;

	priv->tx_currdescnum = desc_num;

	/* Start the transmission */
	writel(POLL_DATA, &dma_p->txpolldemand);
	dma_sync_single_for_cpu((unsigned long)desc_p->dmamac_addr, length,
				DMA_TO_DEVICE);

	return 0;
}

static int dwc_ether_rx(struct eth_device *dev)
{
	struct dw_eth_dev *priv = dev->priv;
	u32 desc_num = priv->rx_currdescnum;
	struct dmamacdescr *desc_p = &priv->rx_mac_descrtable[desc_num];

	u32 status = desc_p->txrx_status;
	int length = 0;

	/* Check  if the owner is the CPU */
	if (status & DESC_RXSTS_OWNBYDMA)
		return 0;

	length = (status & DESC_RXSTS_FRMLENMSK) >>
		 DESC_RXSTS_FRMLENSHFT;

	/*
	 * Make the current descriptor valid again and go to
	 * the next one
	 */
	dma_sync_single_for_cpu((unsigned long)desc_p->dmamac_addr, length,
				DMA_FROM_DEVICE);
	net_receive(dev, desc_p->dmamac_addr, length);
	dma_sync_single_for_device((unsigned long)desc_p->dmamac_addr, length,
				   DMA_FROM_DEVICE);

	desc_p->txrx_status |= DESC_RXSTS_OWNBYDMA;

	/* Test the wrap-around condition. */
	if (++desc_num >= CONFIG_RX_DESCR_NUM)
		desc_num = 0;

	priv->rx_currdescnum = desc_num;

	return length;
}

static void dwc_ether_halt (struct eth_device *dev)
{
	struct dw_eth_dev *priv = dev->priv;

	mac_reset(dev);
	priv->tx_currdescnum = priv->rx_currdescnum = 0;
}

static int dwc_ether_get_ethaddr(struct eth_device *dev, u8 adr[6])
{
	/* we have no EEPROM */
	return -1;
}

static int dwc_ether_set_ethaddr(struct eth_device *dev, const unsigned char *adr)
{
	struct dw_eth_dev *priv = dev->priv;
	struct eth_mac_regs *mac_p = priv->mac_regs_p;
	u32 macid_lo, macid_hi;

	macid_lo = adr[0] + (adr[1] << 8) +
		   (adr[2] << 16) + (adr[3] << 24);
	macid_hi = adr[4] + (adr[5] << 8);
	writel(macid_hi, &mac_p->macaddr0hi);
	writel(macid_lo, &mac_p->macaddr0lo);
	memcpy(priv->macaddr, adr, 6);
	return 0;
}

static void dwc_version(struct device_d *dev, u32 hwid)
{
	u32 uid = ((hwid & 0x0000ff00) >> 8);
	u32 synid = (hwid & 0x000000ff);

	dev_info(dev, "user ID: 0x%x, Synopsys ID: 0x%x\n",
		uid, synid);
}

static int dwc_probe_dt(struct device_d *dev, struct dw_eth_dev *priv)
{
	struct device_node *child;

	if (!IS_ENABLED(CONFIG_OFTREE))
		return -ENODEV;

	priv->phy_addr = -1;
	priv->interface = of_get_phy_mode(dev->device_node);

	/* Set MDIO bus device node, if present. */
	for_each_child_of_node(dev->device_node, child) {
		if (of_device_is_compatible(child, "snps,dwmac-mdio")) {
			priv->miibus.dev.device_node = child;
			break;
		}
	}

	return 0;
}

struct dw_eth_dev *dwc_drv_probe(struct device_d *dev)
{
	struct resource *iores;
	struct dw_eth_dev *priv;
	struct eth_device *edev;
	struct mii_bus *miibus;
	void __iomem *base;
	struct dwc_ether_platform_data *pdata = dev->platform_data;
	int ret;
	struct dw_eth_drvdata *drvdata;

	priv = xzalloc(sizeof(struct dw_eth_dev));

	ret = dev_get_drvdata(dev, (const void **)&drvdata);
	if (ret)
		return ERR_PTR(ret);

	if (drvdata && drvdata->enh_desc)
		priv->enh_desc = drvdata->enh_desc;
	else
		dev_warn(dev, "No drvdata specified\n");


	if (pdata) {
		priv->phy_addr = pdata->phy_addr;
		priv->interface = pdata->interface;
		priv->fix_mac_speed = pdata->fix_mac_speed;
	} else {
		ret = dwc_probe_dt(dev, priv);
		if (ret)
			return ERR_PTR(ret);
	}

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return ERR_CAST(iores);
	base = IOMEM(iores->start);

	priv->mac_regs_p = base;
	dwc_version(dev, readl(&priv->mac_regs_p->version));
	priv->dma_regs_p = base + DW_DMA_BASE_OFFSET;
	priv->tx_mac_descrtable = dma_alloc_coherent(
		CONFIG_TX_DESCR_NUM * sizeof(struct dmamacdescr),
		DMA_ADDRESS_BROKEN);
	priv->rx_mac_descrtable = dma_alloc_coherent(
		CONFIG_RX_DESCR_NUM * sizeof(struct dmamacdescr),
		DMA_ADDRESS_BROKEN);
	priv->txbuffs = dma_alloc(TX_TOTAL_BUFSIZE);
	priv->rxbuffs = dma_alloc(RX_TOTAL_BUFSIZE);

	edev = &priv->netdev;
	miibus = &priv->miibus;
	edev->priv = priv;

	dev->priv = edev;
	edev->parent = dev;
	edev->open = dwc_ether_open;
	edev->send = dwc_ether_send;
	edev->recv = dwc_ether_rx;
	edev->halt = dwc_ether_halt;
	edev->get_ethaddr = dwc_ether_get_ethaddr;
	edev->set_ethaddr = dwc_ether_set_ethaddr;

	miibus->parent = dev;
	miibus->read = dwc_ether_mii_read;
	miibus->write = dwc_ether_mii_write;
	miibus->priv = priv;

	mdiobus_register(miibus);
	eth_register(edev);

	return priv;
}

void dwc_drv_remove(struct device_d *dev)
{
	struct eth_device *edev = dev->priv;

	dwc_ether_halt(edev);
}
