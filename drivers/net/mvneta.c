// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * (C) Copyright 2014 - Ezequiel Garcia <ezequiel.garcia@free-electrons.com>
 *
 * based on mvneta driver from linux
 *   (C) Copyright 2012 Marvell
 *   Rami Rosen <rosenr@marvell.com>
 *   Thomas Petazzoni <thomas.petazzoni@free-electrons.com>
 *
 * based on orion-gbe driver from barebox
 *   (C) Copyright 2014
 *   Pengutronix, Michael Grzeschik <mgr@pengutronix.de>
 *   Sebastian Hesselbarth <sebastian.hesselbarth@gmail.com>
 */

#include <common.h>
#include <dma.h>
#include <init.h>
#include <io.h>
#include <net.h>
#include <of_net.h>
#include <linux/sizes.h>
#include <asm/mmu.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/mbus.h>

/* Registers */

/* Rx queue */
#define MVNETA_RXQ_CONFIG_REG(q)                (0x1400 + ((q) << 2))
#define      MVNETA_RXQ_PKT_OFFSET_ALL_MASK     (0xf    << 8)
#define      MVNETA_RXQ_PKT_OFFSET_MASK(offs)   ((offs) << 8)
#define MVNETA_RXQ_THRESHOLD_REG(q)             (0x14c0 + ((q) << 2))
#define      MVNETA_RXQ_NON_OCCUPIED(v)         ((v) << 16)
#define MVNETA_RXQ_BASE_ADDR_REG(q)             (0x1480 + ((q) << 2))
#define MVNETA_RXQ_SIZE_REG(q)                  (0x14a0 + ((q) << 2))
#define      MVNETA_RXQ_BUF_SIZE_SHIFT          19
#define      MVNETA_RXQ_BUF_SIZE_MASK           (0x1fff << 19)
#define MVNETA_RXQ_STATUS_REG(q)                (0x14e0 + ((q) << 2))
#define      MVNETA_RXQ_OCCUPIED_ALL_MASK       0x3fff
#define MVNETA_RXQ_STATUS_UPDATE_REG(q)         (0x1500 + ((q) << 2))
#define      MVNETA_RXQ_ADD_NON_OCCUPIED_SHIFT  16
#define      MVNETA_RXQ_ADD_NON_OCCUPIED_MAX    255

#define MVNETA_PORT_RX_RESET                    0x1cc0
#define MVNETA_MBUS_RETRY                       0x2010
#define MVNETA_UNIT_INTR_CAUSE                  0x2080
#define MVNETA_UNIT_CONTROL                     0x20B0
#define MVNETA_WIN_BASE(w)                      (0x2200 + ((w) << 3))
#define MVNETA_WIN_SIZE(w)                      (0x2204 + ((w) << 3))
#define MVNETA_WIN_REMAP(w)                     (0x2280 + ((w) << 2))
#define MVNETA_BASE_ADDR_ENABLE                 0x2290
#define MVNETA_PORT_CONFIG                      0x2400
#define      MVNETA_DEF_RXQ(q)                  ((q) << 1)
#define      MVNETA_DEF_RXQ_ARP(q)              ((q) << 4)
#define      MVNETA_TX_UNSET_ERR_SUM            BIT(12)
#define      MVNETA_DEF_RXQ_TCP(q)              ((q) << 16)
#define      MVNETA_DEF_RXQ_UDP(q)              ((q) << 19)
#define      MVNETA_DEF_RXQ_BPDU(q)             ((q) << 22)
#define      MVNETA_RX_CSUM_WITH_PSEUDO_HDR     BIT(25)
#define      MVNETA_PORT_CONFIG_DEFL_VALUE(q)   (MVNETA_DEF_RXQ(q)       | \
						 MVNETA_DEF_RXQ_ARP(q)	 | \
						 MVNETA_DEF_RXQ_TCP(q)	 | \
						 MVNETA_DEF_RXQ_UDP(q)	 | \
						 MVNETA_DEF_RXQ_BPDU(q)	 | \
						 MVNETA_TX_UNSET_ERR_SUM | \
						 MVNETA_RX_CSUM_WITH_PSEUDO_HDR)
#define MVNETA_PORT_CONFIG_EXTEND                0x2404
#define MVNETA_MAC_ADDR_LOW                      0x2414
#define MVNETA_MAC_ADDR_HIGH                     0x2418
#define MVNETA_SDMA_CONFIG                       0x241c
#define      MVNETA_SDMA_BRST_SIZE_16            4
#define      MVNETA_RX_BRST_SZ_MASK(burst)       ((burst) << 1)
#define      MVNETA_RX_NO_DATA_SWAP              BIT(4)
#define      MVNETA_TX_NO_DATA_SWAP              BIT(5)
#define      MVNETA_DESC_SWAP                    BIT(6)
#define      MVNETA_TX_BRST_SZ_MASK(burst)       ((burst) << 22)
#define MVNETA_RX_MIN_FRAME_SIZE                 0x247c
#define MVNETA_SERDES_CFG			 0x24a0
#define      MVNETA_SGMII_SERDES		 0x0cc7
#define      MVNETA_QSGMII_SERDES		 0x0667
#define MVNETA_TYPE_PRIO                         0x24bc
#define MVNETA_TXQ_CMD_1                         0x24e4
#define MVNETA_TXQ_CMD                           0x2448
#define      MVNETA_TXQ_DISABLE_SHIFT            8
#define      MVNETA_TXQ_ENABLE_MASK              0x000000ff
#define MVNETA_ACC_MODE                          0x2500
#define MVNETA_CPU_MAP(cpu)                      (0x2540 + ((cpu) << 2))

#define MVNETA_INTR_NEW_CAUSE                    0x25a0
#define MVNETA_INTR_NEW_MASK                     0x25a4
#define MVNETA_INTR_OLD_CAUSE                    0x25a8
#define MVNETA_INTR_OLD_MASK                     0x25ac
#define MVNETA_INTR_MISC_CAUSE                   0x25b0
#define MVNETA_INTR_MISC_MASK                    0x25b4
#define MVNETA_INTR_ENABLE                       0x25b8

#define MVNETA_RXQ_CMD                           0x2680
#define      MVNETA_RXQ_DISABLE_SHIFT            8
#define      MVNETA_RXQ_ENABLE_MASK              0x000000ff
#define MVETH_TXQ_TOKEN_COUNT_REG(q)             (0x2700 + ((q) << 4))
#define MVETH_TXQ_TOKEN_CFG_REG(q)               (0x2704 + ((q) << 4))
#define MVNETA_GMAC_CTRL_0                       0x2c00
#define MVNETA_GMAC_CTRL_2                       0x2c08
#define      MVNETA_GMAC2_PCS_ENABLE             BIT(3)
#define      MVNETA_GMAC2_PORT_RGMII             BIT(4)
#define      MVNETA_GMAC2_PORT_RESET             BIT(6)
#define MVNETA_GMAC_AUTONEG_CONFIG               0x2c0c
#define      MVNETA_GMAC_FORCE_LINK_DOWN         BIT(0)
#define      MVNETA_GMAC_FORCE_LINK_PASS         BIT(1)
#define      MVNETA_GMAC_CONFIG_MII_SPEED        BIT(5)
#define      MVNETA_GMAC_CONFIG_GMII_SPEED       BIT(6)
#define      MVNETA_GMAC_AN_SPEED_EN             BIT(7)
#define      MVNETA_GMAC_CONFIG_FLOWCTRL         BIT(8)
#define      MVNETA_GMAC_CONFIG_FULL_DUPLEX      BIT(12)
#define      MVNETA_GMAC_AN_FLOWCTRL_EN          BIT(11)
#define      MVNETA_GMAC_AN_DUPLEX_EN            BIT(13)
#define MVNETA_MIB_COUNTERS_BASE                 0x3080
#define      MVNETA_MIB_LATE_COLLISION           0x7c
#define MVNETA_DA_FILT_SPEC_MCAST                0x3400
#define MVNETA_DA_FILT_OTH_MCAST                 0x3500
#define MVNETA_DA_FILT_UCAST_BASE                0x3600
#define MVNETA_TXQ_BASE_ADDR_REG(q)              (0x3c00 + ((q) << 2))
#define MVNETA_TXQ_SIZE_REG(q)                   (0x3c20 + ((q) << 2))
#define MVNETA_TXQ_UPDATE_REG(q)                 (0x3c60 + ((q) << 2))
#define      MVNETA_TXQ_DEC_SENT_SHIFT           16
#define MVNETA_TXQ_STATUS_REG(q)                 (0x3c40 + ((q) << 2))
#define MVNETA_PORT_TX_RESET                     0x3cf0
#define MVNETA_TX_MTU                            0x3e0c
#define MVNETA_TX_TOKEN_SIZE                     0x3e14
#define MVNETA_TXQ_TOKEN_SIZE_REG(q)             (0x3e40 + ((q) << 2))

/* The mvneta_tx_desc and mvneta_rx_desc structures describe the
 * layout of the transmit and reception DMA descriptors, and their
 * layout is therefore defined by the hardware design
 */

#define MVNETA_TX_L3_OFF_SHIFT	0
#define MVNETA_TX_IP_HLEN_SHIFT	8
#define MVNETA_TX_L4_UDP	BIT(16)
#define MVNETA_TX_L3_IP6	BIT(17)
#define MVNETA_TXD_IP_CSUM	BIT(18)
#define MVNETA_TXD_Z_PAD	BIT(19)
#define MVNETA_TXD_L_DESC	BIT(20)
#define MVNETA_TXD_F_DESC	BIT(21)
#define MVNETA_TXD_FLZ_DESC	(MVNETA_TXD_Z_PAD  | \
				 MVNETA_TXD_L_DESC | \
				 MVNETA_TXD_F_DESC)
#define MVNETA_TX_L4_CSUM_FULL	BIT(30)
#define MVNETA_TX_L4_CSUM_NOT	BIT(31)

#define MVNETA_TXD_ERROR	BIT(0)
#define TXD_ERROR_MASK		0x6
#define TXD_ERROR_SHIFT		1

#define MVNETA_RXD_ERR_CRC		0x0
#define MVNETA_RXD_ERR_SUMMARY		BIT(16)
#define MVNETA_RXD_ERR_OVERRUN		BIT(17)
#define MVNETA_RXD_ERR_LEN		BIT(18)
#define MVNETA_RXD_ERR_RESOURCE		(BIT(17) | BIT(18))
#define MVNETA_RXD_ERR_CODE_MASK	(BIT(17) | BIT(18))
#define MVNETA_RXD_L3_IP4		BIT(25)
#define MVNETA_RXD_FIRST_LAST_DESC	(BIT(26) | BIT(27))
#define MVNETA_RXD_L4_CSUM_OK		BIT(30)

#define MVNETA_MH_SIZE		2
#define TXQ_NUM			8
#define RX_RING_SIZE		4
#define TRANSFER_TIMEOUT	(10 * MSECOND)

struct rxdesc {
	u32  cmd_sts;		/* Info about received packet		*/
	u16  reserved1;		/* pnc_info - (for future use, PnC)	*/
	u16  data_size;		/* Size of received packet in bytes	*/

	u32  buf_phys_addr;	/* Physical address of the buffer	*/
	u32  reserved2;		/* pnc_flow_id  (for future use, PnC)	*/

	u32  buf_cookie;	/* cookie for access to RX buffer in rx path */
	u16  reserved3;		/* prefetch_cmd, for future use		*/
	u16  reserved4;		/* csum_l4 - (for future use, PnC)	*/

	u32  reserved5;		/* pnc_extra PnC (for future use, PnC)	*/
	u32  reserved6;		/* hw_cmd (for future use, PnC and HWF)	*/
};

struct txdesc {
	u32  cmd_sts;		/* Options used by HW for packet transmitting.*/
	u16  reserverd1;	/* csum_l4 (for future use)		*/
	u16  byte_cnt;		/* Data size of transmitted packet in bytes */
	u32  buf_ptr;		/* Physical addr of transmitted buffer	*/
	u8   error;		/*					*/
	u8   reserved2;		/* Reserved - (for future use)		*/
	u16  reserved3;		/* Reserved - (for future use)		*/
	u32  reserved4[4];	/* Reserved - (for future use)		*/
};

struct mvneta_port {
	void __iomem *reg;
	struct device_d dev;
	struct eth_device edev;
	struct clk *clk;

	struct txdesc *txdesc;
	struct rxdesc *rxdesc;
	int curr_rxdesc;
	u8 *rxbuf;
	phy_interface_t intf;
};

static void mvneta_conf_mbus_windows(struct mvneta_port *priv)
{
	const struct mbus_dram_target_info *dram = mvebu_mbus_dram_info();
	u32 win_enable, win_protect;
	int i;

	for (i = 0; i < 6; i++) {
		writel(0, priv->reg + MVNETA_WIN_BASE(i));
		writel(0, priv->reg + MVNETA_WIN_SIZE(i));

		if (i < 4)
			writel(0, priv->reg + MVNETA_WIN_REMAP(i));
	}

	win_enable = 0x3f;
	win_protect = 0;

	for (i = 0; i < dram->num_cs; i++) {
		const struct mbus_dram_window *cs = dram->cs + i;

		writel((cs->base & 0xffff0000) |
		       (cs->mbus_attr << 8) |
		       dram->mbus_dram_target_id,
		       priv->reg + MVNETA_WIN_BASE(i));

		writel((cs->size - 1) & 0xffff0000,
		       priv->reg + MVNETA_WIN_SIZE(i));

		win_enable &= ~(1 << i);
		win_protect |= 3 << (2 * i);
	}

	writel(win_enable, priv->reg + MVNETA_BASE_ADDR_ENABLE);
}

static void mvneta_clear_mcast_table(struct mvneta_port *priv)
{
	int offset;

	for (offset = 0; offset <= 0xfc; offset += 4) {
		writel(0, priv->reg + MVNETA_DA_FILT_UCAST_BASE + offset);
		writel(0, priv->reg + MVNETA_DA_FILT_SPEC_MCAST + offset);
		writel(0, priv->reg + MVNETA_DA_FILT_OTH_MCAST + offset);
	}
}

/* Set unicast address */
static void mvneta_set_ucast_addr(struct mvneta_port *priv, u8 last_nibble)
{
	unsigned int tbl_offset, reg_offset;
	int queue = 0;
	u32 val;

	/* Locate the Unicast table entry */
	last_nibble = (0xf & last_nibble);

	/* offset from unicast tbl base */
	tbl_offset = (last_nibble / 4) * 4;

	/* offset within the above reg  */
	reg_offset = last_nibble % 4;

	val = readl(priv->reg + MVNETA_DA_FILT_UCAST_BASE + tbl_offset);

	val &= ~(0xff << (8 * reg_offset));
	val |= ((0x01 | (queue << 1)) << (8 * reg_offset));

	writel(val, priv->reg + MVNETA_DA_FILT_UCAST_BASE + tbl_offset);
}

static void mvneta_clear_ucast_addr(struct mvneta_port *priv, u8 last_nibble)
{
	unsigned int tbl_offset, reg_offset;
	u32 val;

	/* Locate the Unicast table entry */
	last_nibble = (0xf & last_nibble);

	/* offset from unicast tbl base */
	tbl_offset = (last_nibble / 4) * 4;

	/* offset within the above reg  */
	reg_offset = last_nibble % 4;

	val = readl(priv->reg + MVNETA_DA_FILT_UCAST_BASE + tbl_offset);

	/* Clear accepts frame bit at specified unicast DA tbl entry */
	val &= ~(0xff << (8 * reg_offset));

	writel(val, priv->reg + MVNETA_DA_FILT_UCAST_BASE + tbl_offset);
}

static void mvneta_rx_unicast_promisc_clear(struct mvneta_port *priv)
{
	u32 portcfg, val;

	portcfg = readl(priv->reg + MVNETA_PORT_CONFIG);
	val = readl(priv->reg + MVNETA_TYPE_PRIO);

	/* Reject all Unicast addresses */
	writel(portcfg & ~BIT(0), priv->reg + MVNETA_PORT_CONFIG);
	writel(val & ~BIT(21), priv->reg + MVNETA_TYPE_PRIO);
}

/* Clear all MIB counters */
static void mvneta_mib_counters_clear(struct mvneta_port *priv)
{
	int i;

	/* Perform dummy reads from MIB counters */
	for (i = 0; i < MVNETA_MIB_LATE_COLLISION; i += 4)
		readl(priv->reg + MVNETA_MIB_COUNTERS_BASE + i);
}

static int mvneta_pending_tx(struct mvneta_port *priv)
{
	u32 val = readl(priv->reg + MVNETA_TXQ_STATUS_REG(0));

	return val & 0x3fff;
}

static int mvneta_pending_rx(struct mvneta_port *priv)
{
	u32 val = readl(priv->reg + MVNETA_RXQ_STATUS_REG(0));

	return val & 0x3fff;
}

static void mvneta_port_stop(struct mvneta_port *priv)
{
	u32 val;

	/* Stop all queues */
	writel(0xff << MVNETA_RXQ_DISABLE_SHIFT, priv->reg + MVNETA_RXQ_CMD);
	writel(0xff << MVNETA_TXQ_DISABLE_SHIFT, priv->reg + MVNETA_TXQ_CMD);

	/* Reset Tx */
	writel(BIT(0), priv->reg + MVNETA_PORT_TX_RESET);
	writel(0, priv->reg + MVNETA_PORT_TX_RESET);

	/* Reset Rx */
	writel(BIT(0), priv->reg + MVNETA_PORT_RX_RESET);
	writel(0, priv->reg + MVNETA_PORT_RX_RESET);

	/* Disable port 0 */
	val = readl(priv->reg + MVNETA_GMAC_CTRL_0);
	writel(val & ~BIT(0), priv->reg + MVNETA_GMAC_CTRL_0);
	udelay(200);

	/* Clear all Cause registers */
	writel(0, priv->reg + MVNETA_INTR_NEW_CAUSE);
	writel(0, priv->reg + MVNETA_INTR_OLD_CAUSE);
	writel(0, priv->reg + MVNETA_INTR_MISC_CAUSE);
	writel(0, priv->reg + MVNETA_UNIT_INTR_CAUSE);

	/* Mask all interrupts */
	writel(0, priv->reg + MVNETA_INTR_NEW_MASK);
	writel(0, priv->reg + MVNETA_INTR_OLD_MASK);
	writel(0, priv->reg + MVNETA_INTR_MISC_MASK);
	writel(0, priv->reg + MVNETA_INTR_ENABLE);
}

static void mvneta_halt(struct eth_device *edev)
{
	mvneta_port_stop((struct mvneta_port *)edev->priv);
}

static int mvneta_send(struct eth_device *edev, void *data, int len)
{
	struct mvneta_port *priv = edev->priv;
	struct txdesc *txdesc = priv->txdesc;
	int ret, error, last_desc;

	/* Flush transmit data */
	dma_sync_single_for_device((unsigned long)data, len, DMA_TO_DEVICE);

	memset(txdesc, 0, sizeof(*txdesc));
	/* Fill the Tx descriptor */
	txdesc->cmd_sts = MVNETA_TX_L4_CSUM_NOT | MVNETA_TXD_FLZ_DESC;
	txdesc->buf_ptr = (u32)data;
	txdesc->byte_cnt = len;

	/* Increase the number of prepared descriptors (one), by writing
	 * to the 'NoOfWrittenDescriptors' field in the PTXSU register.
	 */
	writel(1, priv->reg + MVNETA_TXQ_UPDATE_REG(0));

	/* The controller updates the number of transmitted descriptors in
	 * the Tx port status register (PTXS).
	 */
	ret = wait_on_timeout(TRANSFER_TIMEOUT, !mvneta_pending_tx(priv));
	dma_sync_single_for_cpu((unsigned long)data, len, DMA_TO_DEVICE);
	if (ret) {
		dev_err(&edev->dev, "transmit timeout\n");
		return ret;
	}

	last_desc = readl(&txdesc->cmd_sts) & MVNETA_TXD_L_DESC;
	error = readl(&txdesc->error);
	if (last_desc && error & MVNETA_TXD_ERROR) {
		dev_err(&edev->dev, "transmit error %d\n",
			(error & TXD_ERROR_MASK) >> TXD_ERROR_SHIFT);
		return -EIO;
	}

	/* Release the transmitted descriptor by writing to the
	 * 'NoOfReleasedBuffers' field in the PTXSU register.
	 */
	writel(1 << MVNETA_TXQ_DEC_SENT_SHIFT,
	       priv->reg + MVNETA_TXQ_UPDATE_REG(0));

	return 0;
}

static int mvneta_recv(struct eth_device *edev)
{
	struct mvneta_port *priv = edev->priv;
	struct rxdesc *rxdesc = &priv->rxdesc[priv->curr_rxdesc];
	int ret, pending;
	u32 cmd_sts;

	/* wait for received packet */
	pending = mvneta_pending_rx(priv);
	if (!pending)
		return 0;

	/* drop malicious packets */
	cmd_sts = readl(&rxdesc->cmd_sts);
	if ((cmd_sts & MVNETA_RXD_FIRST_LAST_DESC) !=
	    MVNETA_RXD_FIRST_LAST_DESC) {
		dev_err(&edev->dev, "rx packet spread on multiple descriptors\n");
		ret = -EIO;
		goto recv_err;
	}

	if (cmd_sts & MVNETA_RXD_ERR_SUMMARY) {
		dev_err(&edev->dev, "receive error\n");
		ret = -EIO;
		goto recv_err;
	}

	/* invalidate current receive buffer */
	dma_sync_single_for_cpu((unsigned long)rxdesc->buf_phys_addr,
				ALIGN(PKTSIZE, 8), DMA_FROM_DEVICE);

	/* received packet is padded with two null bytes (Marvell header) */
	net_receive(edev, (void *)(rxdesc->buf_phys_addr + MVNETA_MH_SIZE),
			  rxdesc->data_size - MVNETA_MH_SIZE);
	ret = 0;

	dma_sync_single_for_device((unsigned long)rxdesc->buf_phys_addr,
				   ALIGN(PKTSIZE, 8), DMA_FROM_DEVICE);

recv_err:
	/* reset this and get next rx descriptor*/
	rxdesc->data_size = 0;
	rxdesc->cmd_sts = 0;

	priv->curr_rxdesc++;
	if (priv->curr_rxdesc == RX_RING_SIZE)
		priv->curr_rxdesc = 0;

	/* Descriptor processed and refilled */
	writel(1 | 1 << MVNETA_RXQ_ADD_NON_OCCUPIED_SHIFT,
	       priv->reg + MVNETA_RXQ_STATUS_UPDATE_REG(0));
	return ret;
}

static int mvneta_set_ethaddr(struct eth_device *edev, const unsigned char *mac)
{
	struct mvneta_port *priv = edev->priv;
	u32 mac_h = (mac[0] << 24) | (mac[1] << 16) | (mac[2] << 8) | mac[3];
	u32 mac_l = (mac[4] << 8) | mac[5];

	mvneta_clear_ucast_addr(priv, mac[5]);

	writel(mac_l, priv->reg + MVNETA_MAC_ADDR_LOW);
	writel(mac_h, priv->reg + MVNETA_MAC_ADDR_HIGH);

	/* accept frames for this address */
	mvneta_set_ucast_addr(priv, mac[5]);

	return 0;
}

static int mvneta_get_ethaddr(struct eth_device *edev, unsigned char *mac)
{
	struct mvneta_port *priv = edev->priv;
	u32 mac_l = readl(priv->reg + MVNETA_MAC_ADDR_LOW);
	u32 mac_h = readl(priv->reg + MVNETA_MAC_ADDR_HIGH);

	mac[0] = (mac_h >> 24) & 0xff;
	mac[1] = (mac_h >> 16) & 0xff;
	mac[2] = (mac_h >> 8) & 0xff;
	mac[3] = mac_h & 0xff;
	mac[4] = (mac_l >> 8) & 0xff;
	mac[5] = mac_l & 0xff;

	return 0;
}

static void mvneta_adjust_link(struct eth_device *edev)
{
	struct mvneta_port *priv = edev->priv;
	struct phy_device *phy = edev->phydev;
	u32 val;

	if (!phy->link)
		return;

	val = readl(priv->reg + MVNETA_GMAC_AUTONEG_CONFIG);
	val &= ~(MVNETA_GMAC_CONFIG_MII_SPEED |
		 MVNETA_GMAC_CONFIG_GMII_SPEED |
		 MVNETA_GMAC_CONFIG_FULL_DUPLEX |
		 MVNETA_GMAC_AN_SPEED_EN |
		 MVNETA_GMAC_AN_FLOWCTRL_EN |
		 MVNETA_GMAC_AN_DUPLEX_EN);

	if (phy->speed == SPEED_1000)
		val |= MVNETA_GMAC_CONFIG_GMII_SPEED;
	else if (phy->speed == SPEED_100)
		val |= MVNETA_GMAC_CONFIG_MII_SPEED;

	if (phy->duplex)
		val |= MVNETA_GMAC_CONFIG_FULL_DUPLEX;

	if (phy->pause)
		val |= MVNETA_GMAC_CONFIG_FLOWCTRL;

	val |= MVNETA_GMAC_FORCE_LINK_PASS | MVNETA_GMAC_FORCE_LINK_DOWN;

	writel(val, priv->reg + MVNETA_GMAC_AUTONEG_CONFIG);

	mvneta_mib_counters_clear(priv);

	/* Enable first Tx and first Rx queues */
	writel(BIT(0), priv->reg + MVNETA_TXQ_CMD);
	writel(BIT(0), priv->reg + MVNETA_RXQ_CMD);
}

static int mvneta_open(struct eth_device *edev)
{
	struct mvneta_port *priv = edev->priv;
	int ret;
	u32 val;

	ret = phy_device_connect(&priv->edev, NULL, -1,
				 mvneta_adjust_link, 0, priv->intf);
	if (ret)
		return ret;

	/* Enable the first port */
	val = readl(priv->reg + MVNETA_GMAC_CTRL_0);
	writel(val | BIT(0), priv->reg + MVNETA_GMAC_CTRL_0);

	return 0;
}

static void mvneta_init_rx_ring(struct mvneta_port *priv)
{
	int i;

	for (i = 0; i < RX_RING_SIZE; i++) {
		struct rxdesc *desc = &priv->rxdesc[i];

		desc->buf_phys_addr = (u32)priv->rxbuf + i * ALIGN(PKTSIZE, 8);
	}

	priv->curr_rxdesc = 0;
}

void mvneta_setup_tx_rx(struct mvneta_port *priv)
{
	u32 val;

	/* Allocate descriptors and buffers */
	priv->txdesc = dma_alloc_coherent(ALIGN(sizeof(*priv->txdesc), 32),
					  DMA_ADDRESS_BROKEN);
	priv->rxdesc = dma_alloc_coherent(RX_RING_SIZE *
					  ALIGN(sizeof(*priv->rxdesc), 32),
					  DMA_ADDRESS_BROKEN);
	priv->rxbuf = dma_alloc(RX_RING_SIZE * ALIGN(PKTSIZE, 8));

	mvneta_init_rx_ring(priv);

	/* Configure the Rx queue */
	val = readl(priv->reg + MVNETA_RXQ_CONFIG_REG(0));
	val &= ~MVNETA_RXQ_PKT_OFFSET_ALL_MASK;
	writel(val, priv->reg + MVNETA_RXQ_CONFIG_REG(0));

	/* Configure the Tx descriptor */
	writel(1, priv->reg + MVNETA_TXQ_SIZE_REG(0));
	writel((u32)priv->txdesc, priv->reg + MVNETA_TXQ_BASE_ADDR_REG(0));

	/* Configure the Rx descriptor. Packet size is in 8-byte units. */
	val = RX_RING_SIZE;
	val |= ((PKTSIZE >> 3) << MVNETA_RXQ_BUF_SIZE_SHIFT);
	writel(val , priv->reg + MVNETA_RXQ_SIZE_REG(0));
	writel((u32)priv->rxdesc, priv->reg + MVNETA_RXQ_BASE_ADDR_REG(0));

	/* Set the number of available Rx descriptors */
	writel(RX_RING_SIZE << MVNETA_RXQ_ADD_NON_OCCUPIED_SHIFT,
	       priv->reg + MVNETA_RXQ_STATUS_UPDATE_REG(0));
}

static int mvneta_port_config(struct mvneta_port *priv)
{
	int queue;
	u32 val;

	/* Enable MBUS Retry bit16 */
	writel(0x20, priv->reg + MVNETA_MBUS_RETRY);

	/* Map the first Tx queue and first Rx queue to CPU0 */
	writel(BIT(0) | (BIT(0) << 8), priv->reg + MVNETA_CPU_MAP(0));

	/* Reset Tx/Rx DMA */
	writel(BIT(0), priv->reg + MVNETA_PORT_TX_RESET);
	writel(BIT(0), priv->reg + MVNETA_PORT_RX_RESET);

	/* Disable Legacy WRR, Disable EJP, Release from reset */
	writel(0, priv->reg + MVNETA_TXQ_CMD_1);

	/* Set maximum bandwidth for the first TX queue */
	writel(0x3ffffff, priv->reg + MVETH_TXQ_TOKEN_CFG_REG(0));
	writel(0x3ffffff, priv->reg + MVETH_TXQ_TOKEN_COUNT_REG(0));

	/* Minimum bandwidth on the rest of them */
	for (queue = 1; queue < TXQ_NUM; queue++) {
		writel(0, priv->reg + MVETH_TXQ_TOKEN_COUNT_REG(queue));
		writel(0, priv->reg + MVETH_TXQ_TOKEN_CFG_REG(queue));
	}

	writel(0, priv->reg + MVNETA_PORT_RX_RESET);
	writel(0, priv->reg + MVNETA_PORT_TX_RESET);

	/* Disable hardware PHY polling */
	val = readl(priv->reg + MVNETA_UNIT_CONTROL);
	writel(val & ~BIT(1), priv->reg + MVNETA_UNIT_CONTROL);

	/* Port Acceleration Mode */
	writel(0x1, priv->reg + MVNETA_ACC_MODE);

	/* Port default configuration for the first Rx queue */
	val = MVNETA_PORT_CONFIG_DEFL_VALUE(0);
	writel(val, priv->reg + MVNETA_PORT_CONFIG);
	writel(0, priv->reg + MVNETA_PORT_CONFIG_EXTEND);
	writel(64, priv->reg + MVNETA_RX_MIN_FRAME_SIZE);

	/* Default burst size */
	val = 0;
	val |= MVNETA_TX_BRST_SZ_MASK(MVNETA_SDMA_BRST_SIZE_16);
	val |= MVNETA_RX_BRST_SZ_MASK(MVNETA_SDMA_BRST_SIZE_16);
	val |= MVNETA_RX_NO_DATA_SWAP | MVNETA_TX_NO_DATA_SWAP;
	writel(val, priv->reg + MVNETA_SDMA_CONFIG);

	mvneta_clear_mcast_table(priv);
	mvneta_rx_unicast_promisc_clear(priv);

	/* Configure maximum MTU and token size */
	writel(0x0003ffff, priv->reg + MVNETA_TX_MTU);
	writel(0xffffffff, priv->reg + MVNETA_TX_TOKEN_SIZE);
	writel(0x7fffffff, priv->reg + MVNETA_TXQ_TOKEN_SIZE_REG(0));

	val = readl(priv->reg + MVNETA_GMAC_CTRL_2);

	/* Even though it might look weird, when we're configured in
	 * SGMII or QSGMII mode, the RGMII bit needs to be set.
	 */
	switch (priv->intf) {
	case PHY_INTERFACE_MODE_QSGMII:
		writel(MVNETA_QSGMII_SERDES, priv->reg + MVNETA_SERDES_CFG);
		val |= MVNETA_GMAC2_PCS_ENABLE | MVNETA_GMAC2_PORT_RGMII;
		break;
	case PHY_INTERFACE_MODE_SGMII:
		writel(MVNETA_SGMII_SERDES, priv->reg + MVNETA_SERDES_CFG);
		val |= MVNETA_GMAC2_PCS_ENABLE | MVNETA_GMAC2_PORT_RGMII;
		break;
	case PHY_INTERFACE_MODE_RGMII:
	case PHY_INTERFACE_MODE_RGMII_ID:
	case PHY_INTERFACE_MODE_RGMII_TXID:
	case PHY_INTERFACE_MODE_RGMII_RXID:
		val |= MVNETA_GMAC2_PORT_RGMII;
		break;
	default:
		return -EINVAL;
	}

	/* Cancel Port Reset */
	val &= ~MVNETA_GMAC2_PORT_RESET;
	writel(val, priv->reg + MVNETA_GMAC_CTRL_2);
	while (readl(priv->reg + MVNETA_GMAC_CTRL_2) & MVNETA_GMAC2_PORT_RESET)
		continue;

	return 0;
}

static int mvneta_probe(struct device_d *dev)
{
	struct mvneta_port *priv;
	int ret;

	priv = xzalloc(sizeof(*priv));

	priv->reg = dev_get_mem_region(dev, 0);

	priv->clk = clk_get(dev, 0);
	if (IS_ERR(priv->clk))
		return PTR_ERR(priv->clk);
	clk_enable(priv->clk);

	ret = of_get_phy_mode(dev->device_node);
	if (ret < 0)
		return ret;
	priv->intf = ret;

	mvneta_port_stop(priv);

	ret = mvneta_port_config(priv);
	if (ret)
		return ret;

	mvneta_conf_mbus_windows(priv);
	mvneta_setup_tx_rx(priv);

	/* register eth device */
	priv->edev.priv = priv;
	priv->edev.open = mvneta_open;
	priv->edev.send = mvneta_send;
	priv->edev.recv = mvneta_recv;
	priv->edev.halt = mvneta_halt;
	priv->edev.set_ethaddr = mvneta_set_ethaddr;
	priv->edev.get_ethaddr = mvneta_get_ethaddr;
	priv->edev.parent = dev;

	ret = eth_register(&priv->edev);
	if (ret)
		return ret;
	return 0;
}

static struct of_device_id mvneta_dt_ids[] = {
	{ .compatible = "marvell,armada-370-neta", },
	{ .compatible = "marvell,armada-xp-neta" },
	{ }
};

static struct driver_d mvneta_driver = {
	.name   = "mvneta",
	.probe  = mvneta_probe,
	.of_compatible = DRV_OF_COMPAT(mvneta_dt_ids),
};
device_platform_driver(mvneta_driver);
