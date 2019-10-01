// SPDX-License-Identifier: GPL-2.0-only
/*
 *  Atheros AR71xx built-in ethernet mac driver
 *
 *  Copyright (C) 2008-2010 Gabor Juhos <juhosg@openwrt.org>
 *  Copyright (C) 2008 Imre Kaloz <kaloz@openwrt.org>
 *
 *  Based on Atheros' AG7100 driver
 */

#include <common.h>
#include <driver.h>
#include <net.h>
#include <dma.h>
#include <init.h>
#include <io.h>
#include <linux/err.h>
#include <linux/phy.h>
#include <of_net.h>
#include <of_address.h>

#include <mach/ath79.h>

/* Register offsets */
#define AG71XX_REG_MAC_CFG1	0x0000
#define AG71XX_REG_MAC_CFG2	0x0004
#define AG71XX_REG_MAC_IPG	0x0008
#define AG71XX_REG_MAC_HDX	0x000c
#define AG71XX_REG_MAC_MFL	0x0010
#define AG71XX_REG_MII_CFG	0x0020
#define AG71XX_REG_MII_CMD	0x0024
#define AG71XX_REG_MII_ADDR	0x0028
#define AG71XX_REG_MII_CTRL	0x002c
#define AG71XX_REG_MII_STATUS	0x0030
#define AG71XX_REG_MII_IND	0x0034
#define AG71XX_REG_MAC_IFCTL	0x0038
#define AG71XX_REG_MAC_ADDR1	0x0040
#define AG71XX_REG_MAC_ADDR2	0x0044
#define AG71XX_REG_FIFO_CFG0	0x0048
#define AG71XX_REG_FIFO_CFG1	0x004c
#define AG71XX_REG_FIFO_CFG2	0x0050
#define AG71XX_REG_FIFO_CFG3	0x0054
#define AG71XX_REG_FIFO_CFG4	0x0058
#define AG71XX_REG_FIFO_CFG5	0x005c
#define AG71XX_REG_FIFO_RAM0	0x0060
#define AG71XX_REG_FIFO_RAM1	0x0064
#define AG71XX_REG_FIFO_RAM2	0x0068
#define AG71XX_REG_FIFO_RAM3	0x006c
#define AG71XX_REG_FIFO_RAM4	0x0070
#define AG71XX_REG_FIFO_RAM5	0x0074
#define AG71XX_REG_FIFO_RAM6	0x0078
#define AG71XX_REG_FIFO_RAM7	0x007c

#define AG71XX_REG_TX_CTRL	0x0180
#define AG71XX_REG_TX_DESC	0x0184
#define AG71XX_REG_TX_STATUS	0x0188
#define AG71XX_REG_RX_CTRL	0x018c
#define AG71XX_REG_RX_DESC	0x0190
#define AG71XX_REG_RX_STATUS	0x0194
#define AG71XX_REG_INT_ENABLE	0x0198
#define AG71XX_REG_INT_STATUS	0x019c

#define AG71XX_REG_FIFO_DEPTH	0x01a8
#define AG71XX_REG_RX_SM	0x01b0
#define AG71XX_REG_TX_SM	0x01b4

#define MAC_CFG1_TXE		BIT(0)	/* Tx Enable */
#define MAC_CFG1_STX		BIT(1)	/* Synchronize Tx Enable */
#define MAC_CFG1_RXE		BIT(2)	/* Rx Enable */
#define MAC_CFG1_SRX		BIT(3)	/* Synchronize Rx Enable */
#define MAC_CFG1_TFC		BIT(4)	/* Tx Flow Control Enable */
#define MAC_CFG1_RFC		BIT(5)	/* Rx Flow Control Enable */
#define MAC_CFG1_LB		BIT(8)	/* Loopback mode */
#define MAC_CFG1_TX_RST		BIT(18)	/* Tx Reset */
#define MAC_CFG1_RX_RST		BIT(19)	/* Rx Reset */
#define MAC_CFG1_SR		BIT(31)	/* Soft Reset */

#define MAC_CFG2_FDX		BIT(0)
#define MAC_CFG2_CRC_EN		BIT(1)
#define MAC_CFG2_PAD_CRC_EN	BIT(2)
#define MAC_CFG2_LEN_CHECK	BIT(4)
#define MAC_CFG2_HUGE_FRAME_EN	BIT(5)
#define MAC_CFG2_IF_1000	BIT(9)
#define MAC_CFG2_IF_10_100	BIT(8)

#define FIFO_CFG0_WTM		BIT(0)	/* Watermark Module */
#define FIFO_CFG0_RXS		BIT(1)	/* Rx System Module */
#define FIFO_CFG0_RXF		BIT(2)	/* Rx Fabric Module */
#define FIFO_CFG0_TXS		BIT(3)	/* Tx System Module */
#define FIFO_CFG0_TXF		BIT(4)	/* Tx Fabric Module */
#define FIFO_CFG0_ALL	(FIFO_CFG0_WTM | FIFO_CFG0_RXS | FIFO_CFG0_RXF \
			| FIFO_CFG0_TXS | FIFO_CFG0_TXF)

#define FIFO_CFG0_ENABLE_SHIFT	8

#define FIFO_CFG4_DE		BIT(0)	/* Drop Event */
#define FIFO_CFG4_DV		BIT(1)	/* RX_DV Event */
#define FIFO_CFG4_FC		BIT(2)	/* False Carrier */
#define FIFO_CFG4_CE		BIT(3)	/* Code Error */
#define FIFO_CFG4_CR		BIT(4)	/* CRC error */
#define FIFO_CFG4_LM		BIT(5)	/* Length Mismatch */
#define FIFO_CFG4_LO		BIT(6)	/* Length out of range */
#define FIFO_CFG4_OK		BIT(7)	/* Packet is OK */
#define FIFO_CFG4_MC		BIT(8)	/* Multicast Packet */
#define FIFO_CFG4_BC		BIT(9)	/* Broadcast Packet */
#define FIFO_CFG4_DR		BIT(10)	/* Dribble */
#define FIFO_CFG4_LE		BIT(11)	/* Long Event */
#define FIFO_CFG4_CF		BIT(12)	/* Control Frame */
#define FIFO_CFG4_PF		BIT(13)	/* Pause Frame */
#define FIFO_CFG4_UO		BIT(14)	/* Unsupported Opcode */
#define FIFO_CFG4_VT		BIT(15)	/* VLAN tag detected */
#define FIFO_CFG4_FT		BIT(16)	/* Frame Truncated */
#define FIFO_CFG4_UC		BIT(17)	/* Unicast Packet */

#define FIFO_CFG5_DE		BIT(0)	/* Drop Event */
#define FIFO_CFG5_DV		BIT(1)	/* RX_DV Event */
#define FIFO_CFG5_FC		BIT(2)	/* False Carrier */
#define FIFO_CFG5_CE		BIT(3)	/* Code Error */
#define FIFO_CFG5_LM		BIT(4)	/* Length Mismatch */
#define FIFO_CFG5_LO		BIT(5)	/* Length Out of Range */
#define FIFO_CFG5_OK		BIT(6)	/* Packet is OK */
#define FIFO_CFG5_MC		BIT(7)	/* Multicast Packet */
#define FIFO_CFG5_BC		BIT(8)	/* Broadcast Packet */
#define FIFO_CFG5_DR		BIT(9)	/* Dribble */
#define FIFO_CFG5_CF		BIT(10)	/* Control Frame */
#define FIFO_CFG5_PF		BIT(11)	/* Pause Frame */
#define FIFO_CFG5_UO		BIT(12)	/* Unsupported Opcode */
#define FIFO_CFG5_VT		BIT(13)	/* VLAN tag detected */
#define FIFO_CFG5_LE		BIT(14)	/* Long Event */
#define FIFO_CFG5_FT		BIT(15)	/* Frame Truncated */
#define FIFO_CFG5_16		BIT(16)	/* unknown */
#define FIFO_CFG5_17		BIT(17)	/* unknown */
#define FIFO_CFG5_SF		BIT(18)	/* Short Frame */
#define FIFO_CFG5_BM		BIT(19)	/* Byte Mode */

#define AG71XX_INT_TX_PS	BIT(0)
#define AG71XX_INT_TX_UR	BIT(1)
#define AG71XX_INT_TX_BE	BIT(3)
#define AG71XX_INT_RX_PR	BIT(4)
#define AG71XX_INT_RX_OF	BIT(6)
#define AG71XX_INT_RX_BE	BIT(7)

#define MAC_IFCTL_SPEED		BIT(16)

#define MII_CFG_CLK_DIV_4	0
#define MII_CFG_CLK_DIV_6	2
#define MII_CFG_CLK_DIV_8	3
#define MII_CFG_CLK_DIV_10	4
#define MII_CFG_CLK_DIV_14	5
#define MII_CFG_CLK_DIV_20	6
#define MII_CFG_CLK_DIV_28	7
#define MII_CFG_CLK_DIV_34	8
#define MII_CFG_CLK_DIV_42	9
#define MII_CFG_CLK_DIV_50	10
#define MII_CFG_CLK_DIV_58	11
#define MII_CFG_CLK_DIV_66	12
#define MII_CFG_CLK_DIV_74	13
#define MII_CFG_CLK_DIV_82	14
#define MII_CFG_CLK_DIV_98	15
#define MII_CFG_RESET		BIT(31)

#define MII_CMD_WRITE		0x0
#define MII_CMD_READ		0x1
#define MII_ADDR_SHIFT		8
#define MII_IND_BUSY		BIT(0)
#define MII_IND_INVALID		BIT(2)

#define TX_CTRL_TXE		BIT(0)	/* Tx Enable */

#define TX_STATUS_PS		BIT(0)	/* Packet Sent */
#define TX_STATUS_UR		BIT(1)	/* Tx Underrun */
#define TX_STATUS_BE		BIT(3)	/* Bus Error */

#define RX_CTRL_RXE		BIT(0)	/* Rx Enable */

#define RX_STATUS_PR		BIT(0)	/* Packet Received */
#define RX_STATUS_OF		BIT(2)	/* Rx Overflow */
#define RX_STATUS_BE		BIT(3)	/* Bus Error */

/*
 * GMAC register macros
 */
#define AG71XX_ETH_CFG_RGMII_GE0        (1<<0)
#define AG71XX_ETH_CFG_MII_GE0_SLAVE    (1<<4)

enum ag71xx_type {
	AG71XX_TYPE_AR9331_GE0,
	AG71XX_TYPE_AR9344_GMAC0,
};

/*
 * h/w descriptor
 */
typedef struct {
	uint32_t    pkt_start_addr;

	uint32_t    is_empty       :  1;
	uint32_t    res1           : 10;
	uint32_t    ftpp_override  :  5;
	uint32_t    res2           :  4;
	uint32_t    pkt_size       : 12;

	uint32_t    next_desc      ;
} ag7240_desc_t;

#define NO_OF_TX_FIFOS  8
#define NO_OF_RX_FIFOS  8
#define TX_RING_SZ (NO_OF_TX_FIFOS * sizeof(ag7240_desc_t))
#define MAX_RBUFF_SZ	0x600		/* 1518 rounded up */

#define MAX_WAIT        1000

struct ag71xx {
	struct device_d *dev;
	struct eth_device netdev;
	void __iomem *regs;
	void __iomem *regs_gmac;
	struct mii_bus miibus;
	const struct ag71xx_cfg *cfg;

	void *rx_buffer;

	unsigned char *rx_pkt[NO_OF_RX_FIFOS];
	ag7240_desc_t *fifo_tx;
	ag7240_desc_t *fifo_rx;
	dma_addr_t addr_tx;
	dma_addr_t addr_rx;

	int next_tx;
	int next_rx;
};

struct ag71xx_cfg {
	enum ag71xx_type type;
	void (*init_mii)(struct ag71xx *priv);
};

static inline void ag71xx_check_reg_offset(struct ag71xx *priv, int reg)
{
	switch (reg) {
	case AG71XX_REG_MAC_CFG1 ... AG71XX_REG_MAC_MFL:
	case AG71XX_REG_MAC_IFCTL ... AG71XX_REG_TX_SM:
	case AG71XX_REG_MII_CFG ... AG71XX_REG_MII_IND:
		break;

	default:
		BUG();
	}
}

static inline u32 ar7240_reg_rd(u32 reg)
{
	return __raw_readl(KSEG1ADDR(reg));
}

static inline void ar7240_reg_wr(u32 reg, u32 val)
{
	__raw_writel(val, KSEG1ADDR(reg));
}

static inline u32 ag71xx_gmac_rr(struct ag71xx *dev, int reg)
{
	return __raw_readl(dev->regs_gmac + reg);
}

static inline void ag71xx_gmac_wr(struct ag71xx *dev, int reg, u32 val)
{
	__raw_writel(val, dev->regs_gmac + reg);
}

static inline u32 ag71xx_rr(struct ag71xx *priv, int reg)
{
	ag71xx_check_reg_offset(priv, reg);

	return __raw_readl(priv->regs + reg);
}

static inline void ag71xx_wr(struct ag71xx *priv, int reg, u32 val)
{
	ag71xx_check_reg_offset(priv, reg);

	__raw_writel(val, priv->regs + reg);
	/* flush write */
	(void)__raw_readl(priv->regs + reg);
}


static int ag71xx_mii_wait(struct ag71xx *priv, int write)
{
	struct device_d *dev = priv->dev;
	uint64_t start;

	start = get_time_ns();
	while (ag71xx_rr(priv, AG71XX_REG_MII_IND) & MII_IND_BUSY) {
		if (!is_timeout_non_interruptible(start, 100 * USECOND))
			continue;

		dev_err(dev, "mii %s error: bus is still busy!\n",
			write ? "write" : "read");
		return -ETIMEDOUT;
	}

	return 0;
}

static int ag71xx_ether_mii_read(struct mii_bus *miidev, int phy_addr, int reg)
{
	struct ag71xx *priv = miidev->priv;
	const struct ag71xx_cfg *cfg = priv->cfg;
	u16 addr = (phy_addr << MII_ADDR_SHIFT) | reg, val;
	int ret;

	if (AG71XX_TYPE_AR9331_GE0 == cfg->type)
		return 0xffff;

	ret = ag71xx_mii_wait(priv, 0);
	if (ret)
		return ret;

	ag71xx_wr(priv, AG71XX_REG_MII_CMD, MII_CMD_WRITE);
	ag71xx_wr(priv, AG71XX_REG_MII_ADDR, addr);
	ag71xx_wr(priv, AG71XX_REG_MII_CMD, MII_CMD_READ);

	ret = ag71xx_mii_wait(priv, 0);
	if (ret)
		return ret;

	val = ag71xx_rr(priv, AG71XX_REG_MII_STATUS);
	ag71xx_wr(priv, AG71XX_REG_MII_CMD, MII_CMD_WRITE);

	return val;
}

static int ag71xx_ether_mii_write(struct mii_bus *miidev, int phy_addr,
				  int reg, u16 val)
{
	struct ag71xx *priv = miidev->priv;
	const struct ag71xx_cfg *cfg = priv->cfg;
	u16 addr = (phy_addr << MII_ADDR_SHIFT) | reg;
	int ret;

	if (AG71XX_TYPE_AR9331_GE0 == cfg->type)
		return 0;

	ret = ag71xx_mii_wait(priv, 1);
	if (ret)
		return ret;

	ag71xx_wr(priv, AG71XX_REG_MII_ADDR, addr);
	ag71xx_wr(priv, AG71XX_REG_MII_CTRL, val);

	ret = ag71xx_mii_wait(priv, 1);
	if (ret)
		return ret;

	return 0;
}

static int ag71xx_ether_set_ethaddr(struct eth_device *edev,
				    const unsigned char *adr)
{
	return 0;
}

static int ag71xx_ether_get_ethaddr(struct eth_device *edev, unsigned char *adr)
{
	/* We have no eeprom */
	return -ENODEV;
}

static void ag71xx_ether_halt(struct eth_device *edev)
{
	struct ag71xx *priv = edev->priv;
	struct device_d *dev = priv->dev;
	uint64_t start;

	ag71xx_wr(priv, AG71XX_REG_RX_CTRL, 0);
	start = get_time_ns();
	while (ag71xx_rr(priv, AG71XX_REG_RX_CTRL)) {
		if (is_timeout_non_interruptible(start, 100 * USECOND)) {
			dev_err(dev, "error: failed to stop device!\n");
			break;
		}
	}
}

static int ag71xx_ether_rx(struct eth_device *edev)
{
	struct ag71xx *priv = edev->priv;
	ag7240_desc_t *f;
	unsigned int work_done;

	for (work_done = 0; work_done < NO_OF_RX_FIFOS; work_done++) {
		unsigned int pktlen;
		unsigned char *rx_pkt;

		f = &priv->fifo_rx[priv->next_rx];

		if (f->is_empty)
			break;

		pktlen = f->pkt_size;
		rx_pkt = priv->rx_pkt[priv->next_rx];

		/* invalidate */
		dma_sync_single_for_cpu((unsigned long)rx_pkt, pktlen,
						DMA_FROM_DEVICE);

		net_receive(edev, rx_pkt, pktlen - 4);

		f->is_empty = 1;

		priv->next_rx = (priv->next_rx + 1) % NO_OF_RX_FIFOS;
	}

	if (!(ag71xx_rr(priv, AG71XX_REG_RX_CTRL) & RX_CTRL_RXE)) {
		f = &priv->fifo_rx[priv->next_rx];
		ag71xx_wr(priv, AG71XX_REG_RX_DESC, virt_to_phys(f));
		ag71xx_wr(priv, AG71XX_REG_RX_CTRL, RX_CTRL_RXE);
	}

	return work_done;
}

static int ag71xx_ether_send(struct eth_device *edev, void *packet, int length)
{
	struct ag71xx *priv = edev->priv;
	struct device_d *dev = priv->dev;
	ag7240_desc_t *f = &priv->fifo_tx[priv->next_tx];
	uint64_t start;
	int ret = 0;

	/* flush */
	dma_sync_single_for_device((unsigned long)packet, length, DMA_TO_DEVICE);

	f->pkt_start_addr = virt_to_phys(packet);
	f->res1 = 0;
	f->pkt_size = length;
	f->is_empty = 0;
	ag71xx_wr(priv, AG71XX_REG_TX_DESC, virt_to_phys(f));
	ag71xx_wr(priv, AG71XX_REG_TX_CTRL, TX_CTRL_TXE);

	/* flush again?! */
	dma_sync_single_for_cpu((unsigned long)packet, length, DMA_TO_DEVICE);

	start = get_time_ns();
	while (!f->is_empty) {
		if (!is_timeout_non_interruptible(start, 100 * USECOND))
			continue;

		dev_err(dev, "error: tx timed out\n");
		ret = -ETIMEDOUT;
		break;
	}

	f->pkt_start_addr = 0;
	f->pkt_size = 0;

	priv->next_tx = (priv->next_tx + 1) % NO_OF_TX_FIFOS;

	return ret;
}

static int ag71xx_ether_open(struct eth_device *edev)
{
	struct ag71xx *priv = edev->priv;
	const struct ag71xx_cfg *cfg = priv->cfg;

	if (AG71XX_TYPE_AR9344_GMAC0 == cfg->type)
		return phy_device_connect(edev, &priv->miibus, 0,
			NULL, 0, PHY_INTERFACE_MODE_RGMII_TXID);

	return 0;
}

static int ag71xx_ether_init(struct eth_device *edev)
{
	struct ag71xx *priv = edev->priv;
	int i;
	void *rxbuf = priv->rx_buffer;

	priv->next_rx = 0;

	for (i = 0; i < NO_OF_RX_FIFOS; i++) {
		ag7240_desc_t *fr = &priv->fifo_rx[i];

		priv->rx_pkt[i] = rxbuf;
		fr->pkt_start_addr = virt_to_phys(rxbuf);
		fr->pkt_size = MAX_RBUFF_SZ;
		fr->is_empty = 1;
		fr->next_desc = virt_to_phys(&priv->fifo_rx[(i + 1) % NO_OF_RX_FIFOS]);

		/* invalidate */
		dma_sync_single_for_device((unsigned long)rxbuf, MAX_RBUFF_SZ,
					DMA_FROM_DEVICE);

		rxbuf += MAX_RBUFF_SZ;
	}

	/* Clean Tx BD's */
	memset(priv->fifo_tx, 0, TX_RING_SZ);

	ag71xx_wr(priv, AG71XX_REG_RX_DESC, virt_to_phys(priv->fifo_rx));
	ag71xx_wr(priv, AG71XX_REG_RX_CTRL, RX_CTRL_RXE);

	return 0;
}

static void ag71xx_ar9331_ge0_mii_init(struct ag71xx *priv)
{
	u32 rd;

	rd = ag71xx_rr(priv, AG71XX_REG_MAC_CFG2);
	rd |= (MAC_CFG2_PAD_CRC_EN | MAC_CFG2_LEN_CHECK | MAC_CFG2_IF_10_100);
	ag71xx_wr(priv, AG71XX_REG_MAC_CFG2, rd);

	/* config FIFOs */
	ag71xx_wr(priv, AG71XX_REG_FIFO_CFG0, 0x1f00);

	rd = ag71xx_gmac_rr(priv, AG71XX_REG_MAC_CFG1);
	rd |= AG71XX_ETH_CFG_MII_GE0_SLAVE;
	ag71xx_gmac_wr(priv, 0, rd);
}

static void ag71xx_ar9344_gmac0_mii_init(struct ag71xx *priv)
{
	u32 rd;

	rd = ag71xx_rr(priv, AG71XX_REG_MAC_CFG2);
	rd |= (MAC_CFG2_PAD_CRC_EN | MAC_CFG2_LEN_CHECK | MAC_CFG2_IF_1000);
	ag71xx_wr(priv, AG71XX_REG_MAC_CFG2, rd);

	/* config FIFOs */
	ag71xx_wr(priv, AG71XX_REG_FIFO_CFG0, 0x1f00);

	ag71xx_gmac_wr(priv, AG71XX_REG_MAC_CFG1, 1);
	udelay(1000);
	ag71xx_wr(priv, AG71XX_REG_MII_CFG, 4 | (1 << 31));
	ag71xx_wr(priv, AG71XX_REG_MII_CFG, 4);
}

static struct ag71xx_cfg ag71xx_cfg_ar9331_ge0 = {
	.type = AG71XX_TYPE_AR9331_GE0,
	.init_mii = ag71xx_ar9331_ge0_mii_init,
};

static struct ag71xx_cfg ag71xx_cfg_ar9344_gmac0 = {
	.type = AG71XX_TYPE_AR9344_GMAC0,
	.init_mii = ag71xx_ar9344_gmac0_mii_init,
};

static int ag71xx_probe(struct device_d *dev)
{
	void __iomem *regs, *regs_gmac;
	struct mii_bus *miibus;
	struct eth_device *edev;
	struct ag71xx_cfg *cfg;
	struct ag71xx *priv;
	u32 mac_h, mac_l;
	u32 rd, mask;
	int ret;

	ret = dev_get_drvdata(dev, (const void **)&cfg);
	if (ret)
		return ret;

	regs_gmac = dev_request_mem_region_by_name(dev, "gmac");
	if (IS_ERR(regs_gmac))
		return PTR_ERR(regs_gmac);

	regs = dev_request_mem_region_by_name(dev, "ge0");
	if (IS_ERR(regs))
		return PTR_ERR(regs);

	priv = xzalloc(sizeof(struct ag71xx));
	edev = &priv->netdev;
	miibus = &priv->miibus;
	dev->priv = edev;
	edev->priv = priv;
	edev->parent = dev;

	edev->init = ag71xx_ether_init;
	edev->open = ag71xx_ether_open;
	edev->send = ag71xx_ether_send;
	edev->recv = ag71xx_ether_rx;
	edev->halt = ag71xx_ether_halt;
	edev->get_ethaddr = ag71xx_ether_get_ethaddr;
	edev->set_ethaddr = ag71xx_ether_set_ethaddr;

	priv->dev = dev;
	priv->regs = regs;
	priv->regs_gmac = regs_gmac;
	priv->cfg = cfg;

	miibus->read = ag71xx_ether_mii_read;
	miibus->write = ag71xx_ether_mii_write;
	miibus->priv = priv;
	miibus->parent = dev;

	/* enable switch core */
	rd = ar7240_reg_rd(AR71XX_PLL_BASE + AR933X_ETHSW_CLOCK_CONTROL_REG);
	rd &= ~(0x1f);
	rd |= 0x10;
	if ((ar7240_reg_rd(WASP_BOOTSTRAP_REG) & WASP_REF_CLK_25) == 0)
		rd |= 0x1;
	ar7240_reg_wr((AR71XX_PLL_BASE + AR933X_ETHSW_CLOCK_CONTROL_REG), rd);

	if (ath79_reset_rr(AR933X_RESET_REG_RESET_MODULE) != 0)
		ath79_reset_wr(AR933X_RESET_REG_RESET_MODULE, 0);

	/* reset GE0 MAC and MDIO */
	mask = AR933X_RESET_GE0_MAC | AR933X_RESET_GE0_MDIO
		| AR933X_RESET_SWITCH;

	rd = ath79_reset_rr(AR933X_RESET_REG_RESET_MODULE);
	rd |= mask;
	ath79_reset_wr(AR933X_RESET_REG_RESET_MODULE, rd);
	mdelay(100);

	rd = ath79_reset_rr(AR933X_RESET_REG_RESET_MODULE);
	rd &= ~(mask);
	ath79_reset_wr(AR933X_RESET_REG_RESET_MODULE, rd);
	mdelay(100);

	ag71xx_wr(priv, AG71XX_REG_MAC_CFG1,
		  (MAC_CFG1_SR | MAC_CFG1_TX_RST | MAC_CFG1_RX_RST));
	ag71xx_wr(priv, AG71XX_REG_MAC_CFG1,
		  (MAC_CFG1_RXE | MAC_CFG1_TXE));

	if (cfg->init_mii)
		cfg->init_mii(priv);

	ag71xx_wr(priv, AG71XX_REG_FIFO_CFG1, 0x10ffff);
	ag71xx_wr(priv, AG71XX_REG_FIFO_CFG2, 0xaaa0555);

	ag71xx_wr(priv, AG71XX_REG_FIFO_CFG4, 0x3ffff);
	/* bit 19 should be set to 1 for GE0 */
	ag71xx_wr(priv, AG71XX_REG_FIFO_CFG5, (0x66b82) | (1 << 19));
	ag71xx_wr(priv, AG71XX_REG_FIFO_CFG3, 0x1f00140);

	priv->rx_buffer = xmemalign(PAGE_SIZE, NO_OF_RX_FIFOS * MAX_RBUFF_SZ);
	priv->fifo_tx = dma_alloc_coherent(NO_OF_TX_FIFOS * sizeof(ag7240_desc_t),
					   &priv->addr_tx);
	priv->fifo_rx = dma_alloc_coherent(NO_OF_RX_FIFOS * sizeof(ag7240_desc_t),
					   &priv->addr_rx);
	priv->next_tx = 0;

	mac_l = 0x3344;
	mac_h = 0x0004d980;

	ag71xx_wr(priv, AG71XX_REG_MAC_ADDR1, mac_l);
	ag71xx_wr(priv, AG71XX_REG_MAC_ADDR2, mac_h);

	mdiobus_register(miibus);
	eth_register(edev);

	dev_info(dev, "network device registered\n");

	return 0;
}

static void ag71xx_remove(struct device_d *dev)
{
	struct eth_device *edev = dev->priv;

	ag71xx_ether_halt(edev);
}

static __maybe_unused struct of_device_id ag71xx_dt_ids[] = {
	{ .compatible = "qca,ar9330-eth", .data = &ag71xx_cfg_ar9331_ge0, },
	{ .compatible = "qca,ar9344-gmac0", .data = &ag71xx_cfg_ar9344_gmac0, },
	{ /* sentinel */ }
};

static struct driver_d ag71xx_driver = {
	.name	= "ag71xx-gmac",
	.probe		= ag71xx_probe,
	.remove		= ag71xx_remove,
	.of_compatible = DRV_OF_COMPAT(ag71xx_dt_ids),
};
device_platform_driver(ag71xx_driver);
