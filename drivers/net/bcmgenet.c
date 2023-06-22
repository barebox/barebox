// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2019 Amit Singh Tomar <amittomer25@gmail.com>
 *
 * Driver for Broadcom GENETv5 Ethernet controller (as found on the RPi4)
 * This driver is based on the Linux driver:
 *      drivers/net/ethernet/broadcom/genet/bcmgenet.c
 *      which is: Copyright (c) 2014-2017 Broadcom
 *
 * The hardware supports multiple queues (16 priority queues and one
 * default queue), both for RX and TX. There are 256 DMA descriptors (both
 * for TX and RX), and they live in MMIO registers. The hardware allows
 * assigning descriptor ranges to queues, but we choose the most simple setup:
 * All 256 descriptors are assigned to the default queue (#16).
 * Also the Linux driver supports multiple generations of the MAC, whereas
 * we only support v5, as used in the Raspberry Pi 4.
 */

#include <common.h>
#include <dma.h>
#include <malloc.h>
#include <net.h>
#include <init.h>
#include <driver.h>
#include <io.h>
#include <clock.h>
#include <xfuncs.h>
#include <linux/phy.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <of_net.h>
#include <linux/iopoll.h>

/* Register definitions derived from Linux source */
#define SYS_REV_CTRL			0x00

#define SYS_PORT_CTRL			0x04
#define PORT_MODE_EXT_GPHY		3

#define GENET_SYS_OFF			0x0000
#define SYS_RBUF_FLUSH_CTRL		(GENET_SYS_OFF + 0x08)
#define SYS_TBUF_FLUSH_CTRL		(GENET_SYS_OFF + 0x0c)

#define GENET_EXT_OFF			0x0080
#define EXT_RGMII_OOB_CTRL		(GENET_EXT_OFF + 0x0c)
#define RGMII_LINK			BIT(4)
#define OOB_DISABLE			BIT(5)
#define RGMII_MODE_EN			BIT(6)
#define ID_MODE_DIS			BIT(16)

#define GENET_RBUF_OFF			0x0300
#define RBUF_TBUF_SIZE_CTRL		(GENET_RBUF_OFF + 0xb4)
#define RBUF_CTRL			(GENET_RBUF_OFF + 0x00)
#define RBUF_ALIGN_2B			BIT(1)

#define GENET_UMAC_OFF			0x0800
#define UMAC_MIB_CTRL			(GENET_UMAC_OFF + 0x580)
#define UMAC_MAX_FRAME_LEN		(GENET_UMAC_OFF + 0x014)
#define UMAC_MAC0			(GENET_UMAC_OFF + 0x00c)
#define UMAC_MAC1			(GENET_UMAC_OFF + 0x010)
#define UMAC_CMD			(GENET_UMAC_OFF + 0x008)
#define MDIO_CMD			(GENET_UMAC_OFF + 0x614)
#define UMAC_TX_FLUSH			(GENET_UMAC_OFF + 0x334)
#define MDIO_START_BUSY			BIT(29)
#define MDIO_READ_FAIL			BIT(28)
#define MDIO_RD				(2 << 26)
#define MDIO_WR				BIT(26)
#define MDIO_PMD_SHIFT			21
#define MDIO_PMD_MASK			0x1f
#define MDIO_REG_SHIFT			16
#define MDIO_REG_MASK			0x1f

#define CMD_TX_EN			BIT(0)
#define CMD_RX_EN			BIT(1)
#define UMAC_SPEED_10			0
#define UMAC_SPEED_100			1
#define UMAC_SPEED_1000			2
#define UMAC_SPEED_2500			3
#define CMD_SPEED_SHIFT			2
#define CMD_SPEED_MASK			3
#define CMD_SW_RESET			BIT(13)
#define CMD_LCL_LOOP_EN			BIT(15)
#define CMD_TX_EN			BIT(0)
#define CMD_RX_EN			BIT(1)

#define MIB_RESET_RX			BIT(0)
#define MIB_RESET_RUNT			BIT(1)
#define MIB_RESET_TX			BIT(2)

/* total number of Buffer Descriptors, same for Rx/Tx */
#define TOTAL_DESCS			256
#define RX_DESCS			TOTAL_DESCS
#define TX_DESCS			TOTAL_DESCS

#define DEFAULT_Q			0x10

#define ENET_MAX_MTU_SIZE		1536

/* Tx/Rx Dma Descriptor common bits */
#define DMA_EN				BIT(0)
#define DMA_RING_BUF_EN_SHIFT		0x01
#define DMA_RING_BUF_EN_MASK		0xffff
#define DMA_BUFLENGTH_MASK		0x0fff
#define DMA_BUFLENGTH_SHIFT		16
#define DMA_RING_SIZE_SHIFT		16
#define DMA_OWN				0x8000
#define DMA_EOP				0x4000
#define DMA_SOP				0x2000
#define DMA_WRAP			0x1000
#define DMA_MAX_BURST_LENGTH		0x8
/* Tx specific DMA descriptor bits */
#define DMA_TX_UNDERRUN			0x0200
#define DMA_TX_APPEND_CRC		0x0040
#define DMA_TX_OW_CRC			0x0020
#define DMA_TX_DO_CSUM			0x0010
#define DMA_TX_QTAG_SHIFT		7

/* DMA rings size */
#define DMA_RING_SIZE			0x40
#define DMA_RINGS_SIZE			(DMA_RING_SIZE * (DEFAULT_Q + 1))

/* DMA descriptor */
#define DMA_DESC_LENGTH_STATUS		0x00
#define DMA_DESC_ADDRESS_LO		0x04
#define DMA_DESC_ADDRESS_HI		0x08
#define DMA_DESC_SIZE			12

#define GENET_RX_OFF			0x2000
#define GENET_RDMA_REG_OFF		0x2c00
#define GENET_TX_OFF			0x4000
#define GENET_TDMA_REG_OFF		0x4c00

#define DMA_FC_THRESH_HI		(RX_DESCS >> 4)
#define DMA_FC_THRESH_LO		5
#define DMA_FC_THRESH_VALUE		((DMA_FC_THRESH_LO << 16) |	\
					  DMA_FC_THRESH_HI)

#define DMA_XOFF_THRESHOLD_SHIFT	16

#define TDMA_RING_REG_BASE		0x5000
#define TDMA_READ_PTR			(TDMA_RING_REG_BASE + 0x00)
#define TDMA_CONS_INDEX			(TDMA_RING_REG_BASE + 0x08)
#define TDMA_PROD_INDEX			(TDMA_RING_REG_BASE + 0x0c)
#define DMA_RING_BUF_SIZE		0x10
#define DMA_START_ADDR			0x14
#define DMA_END_ADDR			0x1c
#define DMA_MBUF_DONE_THRESH		0x24
#define TDMA_FLOW_PERIOD		(TDMA_RING_REG_BASE + 0x28)
#define TDMA_WRITE_PTR			(TDMA_RING_REG_BASE + 0x2c)

#define RDMA_RING_REG_BASE		0x3000
#define RDMA_WRITE_PTR			(RDMA_RING_REG_BASE + 0x00)
#define RDMA_PROD_INDEX			(RDMA_RING_REG_BASE + 0x08)
#define RDMA_CONS_INDEX			(RDMA_RING_REG_BASE + 0x0c)
#define RDMA_XON_XOFF_THRESH		(RDMA_RING_REG_BASE + 0x28)
#define RDMA_READ_PTR			(RDMA_RING_REG_BASE + 0x2c)

#define TDMA_REG_BASE			0x5040
#define RDMA_REG_BASE			0x3040
#define DMA_RING_CFG			0x00
#define DMA_CTRL			0x04
#define DMA_SCB_BURST_SIZE		0x0c

#define RX_BUF_LENGTH			2048
#define RX_TOTAL_BUFSIZE		(RX_BUF_LENGTH * RX_DESCS)
#define RX_BUF_OFFSET			2

struct bcmgenet_eth_priv {
	char *rxbuffer;
	void *mac_reg;
	int tx_index;
	int rx_index;
	int c_index;
	u32 interface;
	struct mii_bus miibus;
	struct eth_device edev;
	struct device *dev;
	unsigned char addr[6];
};

static void bcmgenet_umac_reset(struct bcmgenet_eth_priv *priv)
{
	u32 reg;

	reg = readl(priv->mac_reg + SYS_RBUF_FLUSH_CTRL);
	reg |= BIT(1);
	writel(reg, (priv->mac_reg + SYS_RBUF_FLUSH_CTRL));
	udelay(10);

	reg &= ~BIT(1);
	writel(reg, (priv->mac_reg + SYS_RBUF_FLUSH_CTRL));
	udelay(10);

	writel(0, (priv->mac_reg + SYS_RBUF_FLUSH_CTRL));
	udelay(10);

	writel(0, priv->mac_reg + UMAC_CMD);

	writel(CMD_SW_RESET | CMD_LCL_LOOP_EN, priv->mac_reg + UMAC_CMD);
	udelay(2);
	writel(0, priv->mac_reg + UMAC_CMD);

	/* clear tx/rx counter */
	writel(MIB_RESET_RX | MIB_RESET_TX | MIB_RESET_RUNT,
	       priv->mac_reg + UMAC_MIB_CTRL);
	writel(0, priv->mac_reg + UMAC_MIB_CTRL);

	writel(ENET_MAX_MTU_SIZE, priv->mac_reg + UMAC_MAX_FRAME_LEN);

	/* init rx registers, enable ip header optimization */
	reg = readl(priv->mac_reg + RBUF_CTRL);
	reg |= RBUF_ALIGN_2B;
	writel(reg, (priv->mac_reg + RBUF_CTRL));

	writel(1, (priv->mac_reg + RBUF_TBUF_SIZE_CTRL));
}

static int __bcmgenet_set_hwaddr(struct bcmgenet_eth_priv *priv)
{
	const unsigned char *addr = priv->addr;
	u32 reg;

	reg = addr[0] << 24 | addr[1] << 16 | addr[2] << 8 | addr[3];
	writel_relaxed(reg, priv->mac_reg + UMAC_MAC0);

	reg = addr[4] << 8 | addr[5];
	writel_relaxed(reg, priv->mac_reg + UMAC_MAC1);

	return 0;
}

static int bcmgenet_set_hwaddr(struct eth_device *dev, const unsigned char *addr)
{
	struct bcmgenet_eth_priv *priv = dev->priv;

	memcpy(priv->addr, addr, 6);

	__bcmgenet_set_hwaddr(priv);

	return 0;
}

static int bcmgenet_get_hwaddr(struct eth_device *edev, unsigned char *mac)
{
	return -1;
}

static void bcmgenet_disable_dma(struct bcmgenet_eth_priv *priv)
{
	clrbits_le32(priv->mac_reg + TDMA_REG_BASE + DMA_CTRL, DMA_EN);
	clrbits_le32(priv->mac_reg + RDMA_REG_BASE + DMA_CTRL, DMA_EN);

	writel(1, priv->mac_reg + UMAC_TX_FLUSH);
	udelay(10);
	writel(0, priv->mac_reg + UMAC_TX_FLUSH);
}

static void bcmgenet_enable_dma(struct bcmgenet_eth_priv *priv)
{
	u32 dma_ctrl = (1 << (DEFAULT_Q + DMA_RING_BUF_EN_SHIFT)) | DMA_EN;

	writel(dma_ctrl, priv->mac_reg + TDMA_REG_BASE + DMA_CTRL);
	setbits_le32(priv->mac_reg + RDMA_REG_BASE + DMA_CTRL, dma_ctrl);
}

static int bcmgenet_gmac_eth_send(struct eth_device *edev, void *packet, int length)
{
	struct bcmgenet_eth_priv *priv = edev->priv;
	void *desc_base = priv->mac_reg + GENET_TX_OFF + priv->tx_index * DMA_DESC_SIZE;
	u32 len_stat = length << DMA_BUFLENGTH_SHIFT;
	u32 prod_index, cons;
	u32 tries = 100;
	dma_addr_t dma;

	prod_index = readl(priv->mac_reg + TDMA_PROD_INDEX);

	dma = dma_map_single(priv->dev, packet, length, DMA_TO_DEVICE);
	if (dma_mapping_error(priv->dev, dma))
		return -EFAULT;

	len_stat |= 0x3f << DMA_TX_QTAG_SHIFT;
	len_stat |= DMA_TX_APPEND_CRC | DMA_SOP | DMA_EOP;

	/* Set-up packet for transmission */
	writel(lower_32_bits(dma), (desc_base + DMA_DESC_ADDRESS_LO));
	writel(upper_32_bits(dma), (desc_base + DMA_DESC_ADDRESS_HI));
	writel(len_stat, (desc_base + DMA_DESC_LENGTH_STATUS));

	/* Increment index and start transmission */
	if (++priv->tx_index >= TX_DESCS)
		priv->tx_index = 0;

	prod_index++;

	/* Start Transmisson */
	writel(prod_index, priv->mac_reg + TDMA_PROD_INDEX);

	do {
		cons = readl(priv->mac_reg + TDMA_CONS_INDEX);
	} while ((cons & 0xffff) < prod_index && --tries);

	dma_unmap_single(priv->dev, dma, length, DMA_TO_DEVICE);

	if (!tries) {
		dev_err(priv->dev, "sending timed out\n");
		return -ETIMEDOUT;
	}

	return 0;
}

static int bcmgenet_gmac_eth_recv(struct eth_device *edev)
{
	struct bcmgenet_eth_priv *priv = edev->priv;
	void *desc_base = priv->mac_reg + GENET_RX_OFF + priv->rx_index * DMA_DESC_SIZE;
	u32 prod_index = readl(priv->mac_reg + RDMA_PROD_INDEX);
	u32 length, addr_lo, addr_hi;
	dma_addr_t addr;

	if (prod_index == priv->c_index)
		return -EAGAIN;

	length = readl(desc_base + DMA_DESC_LENGTH_STATUS);
	length = (length >> DMA_BUFLENGTH_SHIFT) & DMA_BUFLENGTH_MASK;
	addr_lo = readl(desc_base + DMA_DESC_ADDRESS_LO);
	addr_hi = readl(desc_base + DMA_DESC_ADDRESS_HI);
	addr = (u64)addr_hi << 32 | addr_lo;

	dma_sync_single_for_cpu(priv->dev, addr, length, DMA_FROM_DEVICE);

	/* To cater for the IP header alignment the hardware does.
	 * This would actually not be needed if we don't program
	 * RBUF_ALIGN_2B
	 */
	net_receive(edev, (void *)addr + RX_BUF_OFFSET, length - RX_BUF_OFFSET);

	dma_sync_single_for_device(priv->dev, addr, length, DMA_FROM_DEVICE);

	/* Tell the MAC we have consumed that last receive buffer. */
	priv->c_index = (priv->c_index + 1) & 0xffff;
	writel(priv->c_index, priv->mac_reg + RDMA_CONS_INDEX);

	/* Forward our descriptor pointer, wrapping around if needed. */
	if (++priv->rx_index >= RX_DESCS)
		priv->rx_index = 0;

	return 0;
}

static void rx_descs_init(struct bcmgenet_eth_priv *priv)
{
	char *rxbuffs = priv->rxbuffer;
	u32 len_stat, i;
	void *desc_base = priv->mac_reg + GENET_RX_OFF;

	len_stat = (RX_BUF_LENGTH << DMA_BUFLENGTH_SHIFT) | DMA_OWN;

	for (i = 0; i < RX_DESCS; i++) {
		writel(lower_32_bits((uintptr_t)&rxbuffs[i * RX_BUF_LENGTH]),
		       desc_base + i * DMA_DESC_SIZE + DMA_DESC_ADDRESS_LO);
		writel(upper_32_bits((uintptr_t)&rxbuffs[i * RX_BUF_LENGTH]),
		       desc_base + i * DMA_DESC_SIZE + DMA_DESC_ADDRESS_HI);
		writel(len_stat,
		       desc_base + i * DMA_DESC_SIZE + DMA_DESC_LENGTH_STATUS);
	}
}

static void rx_ring_init(struct bcmgenet_eth_priv *priv)
{
	writel(DMA_MAX_BURST_LENGTH,
	       priv->mac_reg + RDMA_REG_BASE + DMA_SCB_BURST_SIZE);

	writel(0x0, priv->mac_reg + RDMA_RING_REG_BASE + DMA_START_ADDR);
	writel(0x0, priv->mac_reg + RDMA_READ_PTR);
	writel(0x0, priv->mac_reg + RDMA_WRITE_PTR);
	writel(RX_DESCS * DMA_DESC_SIZE / 4 - 1,
	       priv->mac_reg + RDMA_RING_REG_BASE + DMA_END_ADDR);

	/* cannot init RDMA_PROD_INDEX to 0, so align RDMA_CONS_INDEX on it instead */
	priv->c_index = readl(priv->mac_reg + RDMA_PROD_INDEX);
	writel(priv->c_index, priv->mac_reg + RDMA_CONS_INDEX);
	priv->rx_index = priv->c_index;
	priv->rx_index &= 0xff;
	writel((RX_DESCS << DMA_RING_SIZE_SHIFT) | RX_BUF_LENGTH,
	       priv->mac_reg + RDMA_RING_REG_BASE + DMA_RING_BUF_SIZE);
	writel(DMA_FC_THRESH_VALUE, priv->mac_reg + RDMA_XON_XOFF_THRESH);
	writel(1 << DEFAULT_Q, priv->mac_reg + RDMA_REG_BASE + DMA_RING_CFG);
}

static void tx_ring_init(struct bcmgenet_eth_priv *priv)
{
	writel(DMA_MAX_BURST_LENGTH,
	       priv->mac_reg + TDMA_REG_BASE + DMA_SCB_BURST_SIZE);

	writel(0x0, priv->mac_reg + TDMA_RING_REG_BASE + DMA_START_ADDR);
	writel(0x0, priv->mac_reg + TDMA_READ_PTR);
	writel(0x0, priv->mac_reg + TDMA_WRITE_PTR);
	writel(TX_DESCS * DMA_DESC_SIZE / 4 - 1,
	       priv->mac_reg + TDMA_RING_REG_BASE + DMA_END_ADDR);
	/* cannot init TDMA_CONS_INDEX to 0, so align TDMA_PROD_INDEX on it instead */
	priv->tx_index = readl(priv->mac_reg + TDMA_CONS_INDEX);
	writel(priv->tx_index, priv->mac_reg + TDMA_PROD_INDEX);
	priv->tx_index &= 0xFF;
	writel(0x1, priv->mac_reg + TDMA_RING_REG_BASE + DMA_MBUF_DONE_THRESH);
	writel(0x0, priv->mac_reg + TDMA_FLOW_PERIOD);
	writel((TX_DESCS << DMA_RING_SIZE_SHIFT) | RX_BUF_LENGTH,
	       priv->mac_reg + TDMA_RING_REG_BASE + DMA_RING_BUF_SIZE);

	writel(1 << DEFAULT_Q, priv->mac_reg + TDMA_REG_BASE + DMA_RING_CFG);
}

static void bcmgenet_adjust_link(struct eth_device *edev)
{
	struct bcmgenet_eth_priv *priv = edev->priv;
	struct phy_device *phy_dev = edev->phydev;
	u32 speed;

	switch (phy_dev->speed) {
	case SPEED_1000:
		speed = UMAC_SPEED_1000;
		break;
	case SPEED_100:
		speed = UMAC_SPEED_100;
		break;
	case SPEED_10:
		speed = UMAC_SPEED_10;
		break;
	default:
		dev_err(priv->dev, "bcmgenet: Unsupported PHY speed: %d\n", phy_dev->speed);
		return;
	}

	clrsetbits_le32(priv->mac_reg + EXT_RGMII_OOB_CTRL, OOB_DISABLE,
			RGMII_LINK | RGMII_MODE_EN);

	if (phy_dev->interface == PHY_INTERFACE_MODE_RGMII ||
	    phy_dev->interface == PHY_INTERFACE_MODE_RGMII_RXID) {
		setbits_le32(priv->mac_reg + EXT_RGMII_OOB_CTRL, ID_MODE_DIS);
		writel(PORT_MODE_EXT_GPHY, priv->mac_reg + SYS_PORT_CTRL);
	}

	clrsetbits_le32(priv->mac_reg + UMAC_CMD, CMD_SPEED_MASK << CMD_SPEED_SHIFT,
			speed << CMD_SPEED_SHIFT);
}

static int bcmgenet_gmac_eth_start(struct eth_device *edev)
{
	struct bcmgenet_eth_priv *priv = edev->priv;
	int ret;

	bcmgenet_umac_reset(priv);

	__bcmgenet_set_hwaddr(priv);

	/* Disable RX/TX DMA and flush TX queues */
	bcmgenet_disable_dma(priv);

	rx_ring_init(priv);
	rx_descs_init(priv);
	tx_ring_init(priv);
	bcmgenet_enable_dma(priv);

	ret = phy_device_connect(edev, &priv->miibus, -1,
				 bcmgenet_adjust_link, 0,
				 priv->interface);
	if (ret)
                return ret;

	/* Enable Rx/Tx */
	setbits_le32(priv->mac_reg + UMAC_CMD, CMD_TX_EN | CMD_RX_EN);

	return 0;
}

static void bcmgenet_mdio_start(struct bcmgenet_eth_priv *priv)
{
	setbits_le32(priv->mac_reg + MDIO_CMD, MDIO_START_BUSY);
}

static int bcmgenet_mdio_write(struct mii_bus *bus, int addr,
			       int reg, u16 value)
{
	struct bcmgenet_eth_priv *priv = bus->priv;
	u32 val;

	/* Prepare the read operation */
	val = MDIO_WR | (addr << MDIO_PMD_SHIFT) |
		(reg << MDIO_REG_SHIFT) | (0xffff & value);
	writel_relaxed(val,  priv->mac_reg + MDIO_CMD);

	/* Start MDIO transaction */
	bcmgenet_mdio_start(priv);

	return readl_poll_timeout(priv->mac_reg + MDIO_CMD, reg,
				  !(reg & MDIO_START_BUSY), 20);
}

static int bcmgenet_mdio_read(struct mii_bus *bus, int addr, int reg)
{
	struct bcmgenet_eth_priv *priv = bus->priv;
	u32 val;
	int ret;

	/* Prepare the read operation */
	val = MDIO_RD | (addr << MDIO_PMD_SHIFT) | (reg << MDIO_REG_SHIFT);
	writel_relaxed(val, priv->mac_reg + MDIO_CMD);

	/* Start MDIO transaction */
	bcmgenet_mdio_start(priv);

	ret = readl_poll_timeout(priv->mac_reg + MDIO_CMD, reg,
				 !(reg & MDIO_START_BUSY), 20);
	if (ret)
		return ret;

	val = readl_relaxed(priv->mac_reg + MDIO_CMD);

	return val & 0xffff;
}

static int bcmgenet_probe(struct device *dev)
{
	struct resource *iores;
	struct bcmgenet_eth_priv *priv;
	u32 reg;
	int ret;
	u8 major;
	struct eth_device *edev;

	priv = xzalloc(sizeof(*priv));
	edev = &priv->edev;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores)) {
		ret = PTR_ERR(iores);
		return ret;
	}
	priv->mac_reg = IOMEM(iores->start);
	priv->dev = dev;
	priv->rxbuffer = dma_alloc(RX_TOTAL_BUFSIZE);

	edev->open = bcmgenet_gmac_eth_start;
	edev->send = bcmgenet_gmac_eth_send;
	edev->recv = bcmgenet_gmac_eth_recv;
	edev->get_ethaddr = bcmgenet_get_hwaddr;
	edev->set_ethaddr = bcmgenet_set_hwaddr;
	edev->parent = dev;
	edev->priv = priv;
	dev->priv = priv;

	/* Read GENET HW version */
	reg = readl_relaxed(priv->mac_reg + SYS_REV_CTRL);
	major = (reg >> 24) & 0x0f;
	if (major != 6) {
		if (major == 5)
			major = 4;
		else if (major == 0)
			major = 1;

		dev_err(priv->dev, "Unsupported GENETv%d.%d\n", major, (reg >> 16) & 0x0f);
		return -ENODEV;
	}

	ret = of_get_phy_mode(dev->of_node);
	if (ret < 0)
		priv->interface = PHY_INTERFACE_MODE_MII;
	else
		priv->interface = ret;

	writel(0, priv->mac_reg + SYS_RBUF_FLUSH_CTRL);
	udelay(10);
	/* disable MAC while updating its registers */
	writel(0, priv->mac_reg + UMAC_CMD);
	/* issue soft reset with (rg)mii loopback to ensure a stable rxclk */
	writel(CMD_SW_RESET | CMD_LCL_LOOP_EN, priv->mac_reg + UMAC_CMD);

	priv->miibus.read = bcmgenet_mdio_read;
	priv->miibus.write = bcmgenet_mdio_write;

	priv->miibus.priv = priv;
	priv->miibus.parent = dev;
	priv->miibus.dev.of_node
		= of_get_compatible_child(dev->of_node, "brcm,genet-mdio-v5");

	ret = mdiobus_register(&priv->miibus);
	if (ret)
		return ret;

	ret = eth_register(edev);
	if (ret)
		return ret;

	return 0;
}

static void bcmgenet_gmac_eth_stop(struct bcmgenet_eth_priv *priv)
{
	clrbits_le32(priv->mac_reg + UMAC_CMD, CMD_TX_EN | CMD_RX_EN);

	bcmgenet_disable_dma(priv);
}

static void bcmgenet_remove(struct device *dev)
{
	struct bcmgenet_eth_priv *priv = dev->priv;

	bcmgenet_gmac_eth_stop(priv);
}

static struct of_device_id bcmgenet_ids[] = {
        {
                .compatible = "brcm,genet-v5",
        }, {
                .compatible = "brcm,bcm2711-genet-v5",
        }, {
                /* sentinel */
        },
};
MODULE_DEVICE_TABLE(of, bcmgenet_ids);

static struct driver bcmgenet_driver = {
        .name   = "brcm-genet",
        .probe  = bcmgenet_probe,
        .remove = bcmgenet_remove,
        .of_compatible = DRV_OF_COMPAT(bcmgenet_ids),
};
device_platform_driver(bcmgenet_driver);
