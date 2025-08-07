// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2023, Intel Corporation.
 *
 * Portions based on U-Boot's dwc_eth_qos.c.
 */

/*
 * This driver supports the Synopsys Designware Ethernet XGMAC (10G Ethernet
 * MAC) IP block. The IP supports multiple options for bus type, clocking/
 * reset structure, and feature list.
 *
 * The driver is written such that generic core logic is kept separate from
 * configuration-specific logic. Code that interacts with configuration-
 * specific resources is split out into separate functions to avoid polluting
 * common code. If/when this driver is enhanced to support multiple
 * configurations, the core code should be adapted to call all configuration-
 * specific functions through function pointers, with the definition of those
 * function pointers being supplied by struct udevice_id xgmac_ids[]'s .data
 * field.
 *
 * This configuration uses an AXI master/DMA bus, an AHB slave/register bus,
 * contains the DMA, MTL, and MAC sub-blocks, and supports a single RGMII PHY.
 * This configuration also has SW control over all clock and reset signals to
 * the HW block.
 */

#include <common.h>
#include <init.h>
#include <io.h>
#include <dma.h>
#include <errno.h>
#include <gpio.h>
#include <linux/gpio/consumer.h>
#include <malloc.h>
#include <net.h>
#include <of_gpio.h>
#include <of_net.h>
#include <linux/iopoll.h>
#include <linux/phy.h>
#include "designware_xgmac.h"

static inline struct xgmac_priv *to_xgmac(struct eth_device *edev)
{
	return container_of(edev, struct xgmac_priv, netdev);
}

static int xgmac_mdio_wait_idle(struct xgmac_priv *xgmac)
{
	u32 idle;

	return readl_poll_timeout(&xgmac->mac_regs->mdio_data, idle,
				  !(idle & XGMAC_MAC_MDIO_ADDRESS_SBUSY),
				  XGMAC_TIMEOUT_100MS);
}

static int xgmac_mdio_read(struct mii_bus *bus, int mdio_addr, int mdio_reg)
{
	struct xgmac_priv *xgmac = bus->priv;
	u32 val;
	u32 hw_addr;
	u32 idle;
	int ret;

	ret = readl_poll_timeout(&xgmac->mac_regs->mdio_data, idle,
				 !(idle & XGMAC_MAC_MDIO_ADDRESS_SBUSY),
				 XGMAC_TIMEOUT_100MS);
	if (ret) {
		dev_err(xgmac->dev, "MDIO not idle at entry: %d\n", ret);
		return ret;
	}

	/* Set clause 22 format */
	val = BIT(mdio_addr);
	writel(val, &xgmac->mac_regs->mdio_clause_22_port);

	hw_addr = (mdio_addr << XGMAC_MAC_MDIO_ADDRESS_PA_SHIFT) |
		   (mdio_reg & XGMAC_MAC_MDIO_REG_ADDR_C22P_MASK);

	val = xgmac->config->config_mac_mdio <<
	      XGMAC_MAC_MDIO_ADDRESS_CR_SHIFT;

	val |= XGMAC_MAC_MDIO_ADDRESS_SADDR |
	       XGMAC_MDIO_SINGLE_CMD_ADDR_CMD_READ |
	       XGMAC_MAC_MDIO_ADDRESS_SBUSY;

	ret = readl_poll_timeout(&xgmac->mac_regs->mdio_data, idle,
				 !(idle & XGMAC_MAC_MDIO_ADDRESS_SBUSY),
				 XGMAC_TIMEOUT_100MS);
	if (ret) {
		dev_err(xgmac->dev, "MDIO not idle at entry: %d\n", ret);
		return ret;
	}

	writel(hw_addr, &xgmac->mac_regs->mdio_address);
	writel(val, &xgmac->mac_regs->mdio_data);

	ret = readl_poll_timeout(&xgmac->mac_regs->mdio_data, idle,
				 !(idle & XGMAC_MAC_MDIO_ADDRESS_SBUSY),
				 XGMAC_TIMEOUT_100MS);
	if (ret) {
		dev_err(xgmac->dev, "MDIO read didn't complete: %d\n", ret);
		return ret;
	}

	val = readl(&xgmac->mac_regs->mdio_data);
	val &= XGMAC_MAC_MDIO_DATA_GD_MASK;

	return val;
}

static int xgmac_mdio_write(struct mii_bus *bus, int mdio_addr, int mdio_reg,
			    u16 mdio_val)
{
	struct xgmac_priv *xgmac = bus->priv;
	u32 val;
	u32 hw_addr;
	int ret;

	ret = xgmac_mdio_wait_idle(xgmac);
	if (ret) {
		dev_err(xgmac->dev, "MDIO not idle at entry: %d\n", ret);
		return ret;
	}

	/* Set clause 22 format */
	val = BIT(mdio_addr);
	writel(val, &xgmac->mac_regs->mdio_clause_22_port);

	hw_addr = (mdio_addr << XGMAC_MAC_MDIO_ADDRESS_PA_SHIFT) |
		   (mdio_reg & XGMAC_MAC_MDIO_REG_ADDR_C22P_MASK);

	hw_addr |= (mdio_reg >> XGMAC_MAC_MDIO_ADDRESS_PA_SHIFT) <<
		    XGMAC_MAC_MDIO_ADDRESS_DA_SHIFT;

	val = (xgmac->config->config_mac_mdio <<
	       XGMAC_MAC_MDIO_ADDRESS_CR_SHIFT);

	val |= XGMAC_MAC_MDIO_ADDRESS_SADDR |
		mdio_val | XGMAC_MDIO_SINGLE_CMD_ADDR_CMD_WRITE |
		XGMAC_MAC_MDIO_ADDRESS_SBUSY;

	ret = xgmac_mdio_wait_idle(xgmac);
	if (ret) {
		dev_err(xgmac->dev, "MDIO not idle at entry: %d\n", ret);
		return ret;
	}

	writel(hw_addr, &xgmac->mac_regs->mdio_address);
	writel(val, &xgmac->mac_regs->mdio_data);

	ret = xgmac_mdio_wait_idle(xgmac);
	if (ret) {
		dev_err(xgmac->dev, "MDIO write didn't complete: %d\n", ret);
		return ret;
	}

	return 0;
}

static void xgmac_set_full_duplex(struct xgmac_priv *xgmac)
{
	clrbits_le32(&xgmac->mac_regs->mac_extended_conf, XGMAC_MAC_EXT_CONF_HD);
}

static void xgmac_set_half_duplex(struct xgmac_priv *xgmac)
{
	setbits_le32(&xgmac->mac_regs->mac_extended_conf, XGMAC_MAC_EXT_CONF_HD);

	/* WAR: Flush TX queue when switching to half-duplex */
	setbits_le32(&xgmac->mtl_regs->txq0_operation_mode,
		     XGMAC_MTL_TXQ0_OPERATION_MODE_FTQ);
}

static void xgmac_set_gmii_speed(struct xgmac_priv *xgmac)
{
	clrsetbits_le32(&xgmac->mac_regs->tx_configuration,
			XGMAC_MAC_CONF_SS_SHIFT_MASK,
			XGMAC_MAC_CONF_SS_1G_GMII << XGMAC_MAC_CONF_SS_SHIFT);
}

static void xgmac_set_mii_speed_100(struct xgmac_priv *xgmac)
{
	clrsetbits_le32(&xgmac->mac_regs->tx_configuration,
			XGMAC_MAC_CONF_SS_SHIFT_MASK,
			XGMAC_MAC_CONF_SS_100M_MII << XGMAC_MAC_CONF_SS_SHIFT);
}

static void xgmac_set_mii_speed_10(struct xgmac_priv *xgmac)
{
	clrsetbits_le32(&xgmac->mac_regs->tx_configuration,
			XGMAC_MAC_CONF_SS_SHIFT_MASK,
			XGMAC_MAC_CONF_SS_2_10M_MII << XGMAC_MAC_CONF_SS_SHIFT);
}

static void xgmac_adjust_link(struct eth_device *edev)
{
	struct xgmac_priv *xgmac = to_xgmac(edev);
	struct device *dev = edev->parent;

	if (edev->phydev->duplex)
		xgmac_set_full_duplex(xgmac);
	else
		xgmac_set_half_duplex(xgmac);

	switch (edev->phydev->speed) {
	case SPEED_1000:
		xgmac_set_gmii_speed(xgmac);
		break;
	case SPEED_100:
		xgmac_set_mii_speed_100(xgmac);
		break;
	case SPEED_10:
		xgmac_set_mii_speed_10(xgmac);
		break;
	default:
		dev_err(dev, "invalid speed %d\n", edev->phydev->speed);
	}
}

static int xgmac_write_hwaddr(struct eth_device *edev, const unsigned char *mac)
{
	struct xgmac_priv *xgmac = to_xgmac(edev);
	u32 val;

	memcpy(xgmac->macaddr, mac, ETH_ALEN);

	/*
	 * This function may be called before start() or after stop(). At that
	 * time, on at least some configurations of the XGMAC HW, all clocks to
	 * the XGMAC HW block will be stopped, and a reset signal applied. If
	 * any register access is attempted in this state, bus timeouts or CPU
	 * hangs may occur. This check prevents that.
	 *
	 * A simple solution to this problem would be to not implement
	 * write_hwaddr(), since start() always writes the MAC address into HW
	 * anyway. However, it is desirable to implement write_hwaddr() to
	 * support the case of SW that runs subsequent to U-Boot which expects
	 * the MAC address to already be programmed into the XGMAC registers,
	 * which must happen irrespective of whether the U-Boot user (or
	 * scripts) actually made use of the XGMAC device, and hence
	 * irrespective of whether start() was ever called.
	 *
	 */
	if (!xgmac->config->reg_access_always_ok && !xgmac->reg_access_ok)
		return 0;

	/* Update the MAC address */
	val = (xgmac->macaddr[5] << 8) |
		(xgmac->macaddr[4]);
	writel(val, &xgmac->mac_regs->address0_high);
	val = (xgmac->macaddr[3] << 24) |
		(xgmac->macaddr[2] << 16) |
		(xgmac->macaddr[1] << 8) |
		(xgmac->macaddr[0]);
	writel(val, &xgmac->mac_regs->address0_low);
	return 0;
}

static int xgmac_read_rom_hwaddr(struct eth_device *edev, unsigned char *mac)
{
	struct xgmac_priv *xgmac = to_xgmac(edev);
	int ret;

	ret = xgmac->config->ops->xgmac_get_enetaddr(edev->parent);
	if (ret < 0)
		return ret;

	if (is_valid_ether_addr(xgmac->macaddr))
		return 0;

	return -EINVAL;
}

static int xgmac_start(struct eth_device *edev)
{
	struct xgmac_priv *xgmac = to_xgmac(edev);
	int ret, i;
	u32 val, tx_fifo_sz, rx_fifo_sz, tqs, rqs, pbl;
	ulong last_rx_desc;
	ulong desc_pad;
	u32 idle;

	ret = phy_device_connect(edev, &xgmac->miibus, xgmac->phy_addr,
				 xgmac_adjust_link, 0, xgmac->interface);
	if (ret)
		return ret;

	ret = xgmac->config->ops->xgmac_start_resets(edev->parent);
	if (ret < 0) {
		dev_err(&edev->dev, "xgmac_start_resets() failed: %d\n", ret);
		goto err;
	}

	xgmac->reg_access_ok = true;

	setbits_le32(&xgmac->dma_regs->mode, XGMAC_DMA_MODE_SWR);

	ret = readl_poll_timeout(&xgmac->dma_regs->mode, idle,
				  !(idle & XGMAC_DMA_MODE_SWR),
				  XGMAC_TIMEOUT_100MS);
	if (ret) {
		dev_err(&edev->dev, "XGMAC_DMA_MODE_SWR stuck: %d\n", ret);
		goto err;
	}

	/* Configure MTL */

	/* Enable Store and Forward mode for TX */
	/* Program Tx operating mode */
	setbits_le32(&xgmac->mtl_regs->txq0_operation_mode,
		     XGMAC_MTL_TXQ0_OPERATION_MODE_TSF |
		     (XGMAC_MTL_TXQ0_OPERATION_MODE_TXQEN_ENABLED <<
		      XGMAC_MTL_TXQ0_OPERATION_MODE_TXQEN_SHIFT));

	/* Transmit Queue weight */
	writel(0x10, &xgmac->mtl_regs->txq0_quantum_weight);

	/* Enable Store and Forward mode for RX, since no jumbo frame */
	setbits_le32(&xgmac->mtl_regs->rxq0_operation_mode,
		     XGMAC_MTL_RXQ0_OPERATION_MODE_RSF);

	/* Transmit/Receive queue fifo size; use all RAM for 1 queue */
	val = readl(&xgmac->mac_regs->hw_feature1);
	tx_fifo_sz = (val >> XGMAC_MAC_HW_FEATURE1_TXFIFOSIZE_SHIFT) &
		XGMAC_MAC_HW_FEATURE1_TXFIFOSIZE_MASK;
	rx_fifo_sz = (val >> XGMAC_MAC_HW_FEATURE1_RXFIFOSIZE_SHIFT) &
		XGMAC_MAC_HW_FEATURE1_RXFIFOSIZE_MASK;

	/*
	 * r/tx_fifo_sz is encoded as log2(n / 128). Undo that by shifting.
	 * r/tqs is encoded as (n / 256) - 1.
	 */
	tqs = (128 << tx_fifo_sz) / 256 - 1;
	rqs = (128 << rx_fifo_sz) / 256 - 1;

	clrsetbits_le32(&xgmac->mtl_regs->txq0_operation_mode,
			XGMAC_MTL_TXQ0_OPERATION_MODE_TQS_MASK <<
			XGMAC_MTL_TXQ0_OPERATION_MODE_TQS_SHIFT,
			tqs << XGMAC_MTL_TXQ0_OPERATION_MODE_TQS_SHIFT);
	clrsetbits_le32(&xgmac->mtl_regs->rxq0_operation_mode,
			XGMAC_MTL_RXQ0_OPERATION_MODE_RQS_MASK <<
			XGMAC_MTL_RXQ0_OPERATION_MODE_RQS_SHIFT,
			rqs << XGMAC_MTL_RXQ0_OPERATION_MODE_RQS_SHIFT);

	setbits_le32(&xgmac->mtl_regs->rxq0_operation_mode,
		     XGMAC_MTL_RXQ0_OPERATION_MODE_EHFC);

	/* Configure MAC */
	clrsetbits_le32(&xgmac->mac_regs->rxq_ctrl0,
			XGMAC_MAC_RXQ_CTRL0_RXQ0EN_MASK <<
			XGMAC_MAC_RXQ_CTRL0_RXQ0EN_SHIFT,
			xgmac->config->config_mac <<
			XGMAC_MAC_RXQ_CTRL0_RXQ0EN_SHIFT);

	/* Multicast and Broadcast Queue Enable */
	setbits_le32(&xgmac->mac_regs->rxq_ctrl1,
		     XGMAC_MAC_RXQ_CTRL1_MCBCQEN);

	/* enable promisc mode and receive all mode */
	setbits_le32(&xgmac->mac_regs->mac_packet_filter,
		     XGMAC_MAC_PACKET_FILTER_RA |
			 XGMAC_MAC_PACKET_FILTER_PR);

	/* Set TX flow control parameters */
	/* Set Pause Time */
	setbits_le32(&xgmac->mac_regs->q0_tx_flow_ctrl,
		     XGMAC_MAC_Q0_TX_FLOW_CTRL_PT_MASK <<
		     XGMAC_MAC_Q0_TX_FLOW_CTRL_PT_SHIFT);

	/* Assign priority for RX flow control */
	clrbits_le32(&xgmac->mac_regs->rxq_ctrl2,
		     XGMAC_MAC_RXQ_CTRL2_PSRQ0_MASK <<
		     XGMAC_MAC_RXQ_CTRL2_PSRQ0_SHIFT);

	/* Enable flow control */
	setbits_le32(&xgmac->mac_regs->q0_tx_flow_ctrl,
		     XGMAC_MAC_Q0_TX_FLOW_CTRL_TFE);
	setbits_le32(&xgmac->mac_regs->rx_flow_ctrl,
		     XGMAC_MAC_RX_FLOW_CTRL_RFE);

	clrbits_le32(&xgmac->mac_regs->tx_configuration,
		     XGMAC_MAC_CONF_JD);

	clrbits_le32(&xgmac->mac_regs->rx_configuration,
		     XGMAC_MAC_CONF_JE |
		     XGMAC_MAC_CONF_GPSLCE |
		     XGMAC_MAC_CONF_WD);

	setbits_le32(&xgmac->mac_regs->rx_configuration,
		     XGMAC_MAC_CONF_ACS |
		     XGMAC_MAC_CONF_CST);

	/* Configure DMA */
	clrsetbits_le32(&xgmac->dma_regs->sysbus_mode,
			XGMAC_DMA_SYSBUS_MODE_AAL,
			XGMAC_DMA_SYSBUS_MODE_EAME |
			XGMAC_DMA_SYSBUS_MODE_UNDEF);

	/* Enable OSP mode */
	setbits_le32(&xgmac->dma_regs->ch0_tx_control,
		     XGMAC_DMA_CH0_TX_CONTROL_OSP);

	/* RX buffer size. Must be a multiple of bus width */
	clrsetbits_le32(&xgmac->dma_regs->ch0_rx_control,
			XGMAC_DMA_CH0_RX_CONTROL_RBSZ_MASK <<
			XGMAC_DMA_CH0_RX_CONTROL_RBSZ_SHIFT,
			XGMAC_MAX_PACKET_SIZE <<
			XGMAC_DMA_CH0_RX_CONTROL_RBSZ_SHIFT);

	desc_pad = 0;

	setbits_le32(&xgmac->dma_regs->ch0_control,
		     XGMAC_DMA_CH0_CONTROL_PBLX8 |
		     (desc_pad << XGMAC_DMA_CH0_CONTROL_DSL_SHIFT));

	/*
	 * Burst length must be < 1/2 FIFO size.
	 * FIFO size in tqs is encoded as (n / 256) - 1.
	 * Each burst is n * 8 (PBLX8) * 16 (AXI width) == 128 bytes.
	 * Half of n * 256 is n * 128, so pbl == tqs, modulo the -1.
	 */
	pbl = tqs + 1;
	if (pbl > 32)
		pbl = 32;

	clrsetbits_le32(&xgmac->dma_regs->ch0_tx_control,
			XGMAC_DMA_CH0_TX_CONTROL_TXPBL_MASK <<
			XGMAC_DMA_CH0_TX_CONTROL_TXPBL_SHIFT,
			pbl << XGMAC_DMA_CH0_TX_CONTROL_TXPBL_SHIFT);

	clrsetbits_le32(&xgmac->dma_regs->ch0_rx_control,
			XGMAC_DMA_CH0_RX_CONTROL_RXPBL_MASK <<
			XGMAC_DMA_CH0_RX_CONTROL_RXPBL_SHIFT,
			8 << XGMAC_DMA_CH0_RX_CONTROL_RXPBL_SHIFT);

	/* DMA performance configuration */
	val = (XGMAC_DMA_SYSBUS_MODE_RD_OSR_LMT_MASK <<
	       XGMAC_DMA_SYSBUS_MODE_RD_OSR_LMT_SHIFT) |
	       (XGMAC_DMA_SYSBUS_MODE_WR_OSR_LMT_MASK <<
	       XGMAC_DMA_SYSBUS_MODE_WR_OSR_LMT_SHIFT) |
	       XGMAC_DMA_SYSBUS_MODE_EAME |
	       XGMAC_DMA_SYSBUS_MODE_BLEN16 |
	       XGMAC_DMA_SYSBUS_MODE_BLEN8 |
	       XGMAC_DMA_SYSBUS_MODE_BLEN4 |
	       XGMAC_DMA_SYSBUS_MODE_BLEN32;

	writel(val, &xgmac->dma_regs->sysbus_mode);

	/* Set up descriptors */

	xgmac->tx_desc_idx = 0;
	xgmac->rx_desc_idx = 0;

	for (i = 0; i < XGMAC_DESCRIPTORS_NUM; i++) {
		struct xgmac_desc *rx_desc = &xgmac->rx_descs[i];

		writel(XGMAC_DESC3_OWN, &rx_desc->des3);
	}

	writel(0, &xgmac->dma_regs->ch0_txdesc_list_haddress);
	writel(xgmac->tx_descs_phys, &xgmac->dma_regs->ch0_txdesc_list_address);
	writel(XGMAC_DESCRIPTORS_NUM - 1,
	       &xgmac->dma_regs->ch0_txdesc_ring_length);

	writel(0, &xgmac->dma_regs->ch0_rxdesc_list_haddress);
	writel(xgmac->rx_descs_phys, &xgmac->dma_regs->ch0_rxdesc_list_address);
	writel(XGMAC_DESCRIPTORS_NUM - 1,
	       &xgmac->dma_regs->ch0_rxdesc_ring_length);

	/* Enable everything */
	setbits_le32(&xgmac->dma_regs->ch0_tx_control,
		     XGMAC_DMA_CH0_TX_CONTROL_ST);
	setbits_le32(&xgmac->dma_regs->ch0_rx_control,
		     XGMAC_DMA_CH0_RX_CONTROL_SR);
	setbits_le32(&xgmac->mac_regs->tx_configuration,
		     XGMAC_MAC_CONF_TE);
	setbits_le32(&xgmac->mac_regs->rx_configuration,
		     XGMAC_MAC_CONF_RE);

	/* TX tail pointer not written until we need to TX a packet */
	/*
	 * Point RX tail pointer at last descriptor. Ideally, we'd point at the
	 * first descriptor, implying all descriptors were available. However,
	 * that's not distinguishable from none of the descriptors being
	 * available.
	 */
	last_rx_desc = (ulong)&xgmac->rx_descs[(XGMAC_DESCRIPTORS_NUM - 1)];
	writel(last_rx_desc, &xgmac->dma_regs->ch0_rxdesc_tail_pointer);

	xgmac->started = true;

	return 0;

err:
	return ret;
}

static void xgmac_stop(struct eth_device *edev)
{
	struct xgmac_priv *xgmac = to_xgmac(edev);
	u64 start_time;
	u32 val;
	u32 trcsts;
	u32 txqsts;
	u32 prxq;
	u32 rxqsts;

	if (!xgmac->started)
		return;

	xgmac->started = false;
	xgmac->reg_access_ok = false;

	/* Disable TX DMA */
	clrbits_le32(&xgmac->dma_regs->ch0_tx_control,
		     XGMAC_DMA_CH0_TX_CONTROL_ST);

	/* Wait for TX all packets to drain out of MTL */
	start_time = get_time_ns();

	while (!is_timeout(start_time, XGMAC_TIMEOUT_100MS)) {
		val = readl(&xgmac->mtl_regs->txq0_debug);

		trcsts = (val >> XGMAC_MTL_TXQ0_DEBUG_TRCSTS_SHIFT) &
			  XGMAC_MTL_TXQ0_DEBUG_TRCSTS_MASK;

		txqsts = val & XGMAC_MTL_TXQ0_DEBUG_TXQSTS;

		if (trcsts != XGMAC_MTL_TXQ0_DEBUG_TRCSTS_READ_STATE && !txqsts)
			break;
	}

	/* Turn off MAC TX and RX */
	clrbits_le32(&xgmac->mac_regs->tx_configuration,
		     XGMAC_MAC_CONF_RE);
	clrbits_le32(&xgmac->mac_regs->rx_configuration,
		     XGMAC_MAC_CONF_RE);

	/* Wait for all RX packets to drain out of MTL */
	start_time = get_time_ns();

	while (!is_timeout(start_time, XGMAC_TIMEOUT_100MS)) {
		val = readl(&xgmac->mtl_regs->rxq0_debug);

		prxq = (val >> XGMAC_MTL_RXQ0_DEBUG_PRXQ_SHIFT) &
			XGMAC_MTL_RXQ0_DEBUG_PRXQ_MASK;

		rxqsts = (val >> XGMAC_MTL_RXQ0_DEBUG_RXQSTS_SHIFT) &
			  XGMAC_MTL_RXQ0_DEBUG_RXQSTS_MASK;

		if (!prxq && !rxqsts)
			break;
	}

	/* Turn off RX DMA */
	clrbits_le32(&xgmac->dma_regs->ch0_rx_control,
		     XGMAC_DMA_CH0_RX_CONTROL_SR);
}

static int xgmac_send(struct eth_device *edev, void *packet, int length)
{
	struct xgmac_priv *xgmac = to_xgmac(edev);
	struct xgmac_desc *tx_desc;
	dma_addr_t dma;
	u32 des3_prev, des3;
	int ret;

	tx_desc = &xgmac->tx_descs[xgmac->tx_desc_idx];
	xgmac->tx_desc_idx++;
	xgmac->tx_desc_idx %= XGMAC_DESCRIPTORS_NUM;

	dma = dma_map_single(edev->parent, packet, length, DMA_TO_DEVICE);
	if (dma_mapping_error(edev->parent, dma))
		return -EFAULT;

	tx_desc->des0 = lower_32_bits(dma);
	tx_desc->des1 = upper_32_bits(dma);
	tx_desc->des2 = length;
	/*
	 * Make sure that if HW sees the _OWN write below, it will see all the
	 * writes to the rest of the descriptor too.
	 */
	barrier();

	des3_prev = XGMAC_DESC3_OWN | XGMAC_DESC3_FD | XGMAC_DESC3_LD | length;
	writel(des3_prev, &tx_desc->des3);
	writel((ulong)(tx_desc + 1), &xgmac->dma_regs->ch0_txdesc_tail_pointer);

	ret = readl_poll_timeout(&tx_desc->des3, des3,
				 !(des3 & XGMAC_DESC3_OWN),
				 100 * USEC_PER_MSEC);

	dma_unmap_single(edev->parent, dma, length, DMA_TO_DEVICE);

	if (ret == -ETIMEDOUT)
		dev_dbg(xgmac->dev, "%s: TX timeout 0x%08x\n", __func__, des3);

	return ret;
}

static void xgmac_recv(struct eth_device *edev)
{
	struct xgmac_priv *xgmac = to_xgmac(edev);
	struct xgmac_desc *rx_desc;
	dma_addr_t dma;
	void *pkt;
	int length;

	rx_desc = &xgmac->rx_descs[xgmac->rx_desc_idx];

	if (rx_desc->des3 & XGMAC_DESC3_OWN)
		return;

	dma = xgmac->dma_rx_buf_phys[xgmac->rx_desc_idx];
	pkt = xgmac->dma_rx_buf_virt[xgmac->rx_desc_idx];
	length = rx_desc->des3 & XGMAC_RDES3_PKT_LENGTH_MASK;

	dma_sync_single_for_cpu(edev->parent, dma, length, DMA_FROM_DEVICE);
	net_receive(edev, pkt, length);
	dma_sync_single_for_device(edev->parent, dma, length, DMA_FROM_DEVICE);

	/* Read Format RX descriptor */
	rx_desc = &xgmac->rx_descs[xgmac->rx_desc_idx];
	rx_desc->des0 = lower_32_bits(dma);
	rx_desc->des1 = upper_32_bits(dma);
	rx_desc->des2 = 0;
	/*
	 * Make sure that if HW sees the _OWN write below, it will see all the
	 * writes to the rest of the descriptor too.
	 */
	rx_desc->des3 = XGMAC_DESC3_OWN;
	barrier();

	writel(xgmac->rx_descs_phys + (xgmac->rx_desc_idx * XGMAC_DESCRIPTOR_SIZE),
	       &xgmac->dma_regs->ch0_rxdesc_tail_pointer);

	xgmac->rx_desc_idx++;
	xgmac->rx_desc_idx %= XGMAC_DESCRIPTORS_NUM;
}

static int xgmac_probe_resources_core(struct xgmac_priv *xgmac)
{
	unsigned int desc_step;
	dma_addr_t dma;
	int ret = 0;
	void *p;
	int i;

	/* Maximum distance between neighboring descriptors, in Bytes. */
	desc_step = sizeof(struct xgmac_desc);

	xgmac->tx_descs = dma_alloc_coherent(xgmac->dev,
					     XGMAC_DESCRIPTORS_SIZE,
					     &xgmac->tx_descs_phys);
	if (!xgmac->tx_descs) {
		ret = -ENOMEM;
		goto err;
	}

	xgmac->rx_descs = dma_alloc_coherent(xgmac->dev,
					     XGMAC_DESCRIPTORS_SIZE,
					     &xgmac->rx_descs_phys);
	if (!xgmac->rx_descs) {
		ret = -ENOMEM;
		goto err_free_tx_descs;
	}

	p = dma_alloc(XGMAC_RX_BUFFER_SIZE);
	if (!p)
		goto err_free_descs;

	for (i = 0; i < XGMAC_DESCRIPTORS_NUM; i++) {
		struct xgmac_desc *rx_desc = &xgmac->rx_descs[i];

		dma = dma_map_single(xgmac->dev, p, XGMAC_MAX_PACKET_SIZE, DMA_FROM_DEVICE);
		if (dma_mapping_error(xgmac->dev, dma)) {
			ret = -EFAULT;
			goto err_free_dma_bufs;
		}

		rx_desc->des0 = lower_32_bits(dma);
		rx_desc->des1 = upper_32_bits(dma);
		xgmac->dma_rx_buf_phys[i] = dma;
		xgmac->dma_rx_buf_virt[i] = p;

		p += XGMAC_MAX_PACKET_SIZE;
	}

	return 0;

err_free_dma_bufs:
	dma_unmap_single(xgmac->dev, dma, XGMAC_MAX_PACKET_SIZE, DMA_FROM_DEVICE);
	dma_free(&dma);
err_free_descs:
	dma_free_coherent(xgmac->dev, xgmac->rx_descs, xgmac->rx_descs_phys,
			  XGMAC_DESCRIPTORS_SIZE);
err_free_tx_descs:
	dma_free_coherent(xgmac->dev, xgmac->tx_descs, xgmac->tx_descs_phys,
			  XGMAC_DESCRIPTORS_SIZE);
err:

	return ret;
}

static int xgmac_remove_resources_core(struct xgmac_priv *xgmac)
{
	dma_free_coherent(xgmac->dev, xgmac->rx_descs, xgmac->rx_descs_phys,
			  XGMAC_DESCRIPTORS_SIZE);
	dma_free_coherent(xgmac->dev, xgmac->tx_descs, xgmac->tx_descs_phys,
			  XGMAC_DESCRIPTORS_SIZE);

	return 0;
}

static void xgmac_probe_dt(struct device *dev, struct xgmac_priv *xgmac)
{
	struct device_node *child;

	xgmac->interface = of_get_phy_mode(dev->of_node);
	xgmac->phy_addr = -1;

	/* Set MDIO bus device node, if present. */
	for_each_child_of_node(dev->of_node, child) {
		if (of_device_is_compatible(child, "snps,dwmac-mdio") ||
		    (child->name && !of_node_cmp(child->name, "mdio"))) {
			xgmac->miibus.dev.of_node = child;
			break;
		}
	}
}

static int xgmac_phy_reset(struct device *dev)
{
	struct gpio_desc *phy_reset;
	u32 delays[3] = {0, 0, 0};

	phy_reset = gpiod_get_optional(dev, "snps,reset",
				       GPIOF_OUT_INIT_ACTIVE);
	if (IS_ERR(phy_reset)) {
		dev_warn(dev, "Failed to get 'snps,reset' GPIO (ignored)\n");
	} else if (phy_reset) {
		of_property_read_u32_array(dev->of_node,
					   "snps,reset-delays-us",
					   delays, ARRAY_SIZE(delays));
		udelay(delays[1]);
		gpiod_set_value(phy_reset, false);
		udelay(delays[2]);
	}

	return 0;
}

int xgmac_probe(struct device *dev)
{
	struct mii_bus *miibus;
	struct xgmac_priv *xgmac;
	struct resource *iores;
	struct eth_device *edev;
	int ret = 0;

	xgmac = xzalloc(sizeof(*xgmac));

	xgmac->dev = dev;
	xgmac->config = device_get_match_data(dev);
	if (!xgmac->config) {
		dev_err(dev, "failed to get driver data: %d\n", ret);
		return ret;
	}

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	xgmac->regs = IOMEM(iores->start);

	xgmac->mac_regs = xgmac->regs + XGMAC_MAC_REGS_BASE;
	xgmac->mtl_regs = xgmac->regs + XGMAC_MTL_REGS_BASE;
	xgmac->dma_regs = xgmac->regs + XGMAC_DMA_REGS_BASE;

	xgmac_probe_dt(dev, xgmac);

	edev = &xgmac->netdev;
	dev->priv = xgmac;
	edev->priv = xgmac;

	edev->parent = dev;
	edev->open = xgmac_start;
	edev->send = xgmac_send;
	edev->halt = xgmac_stop;
	edev->recv = xgmac_recv;
	edev->get_ethaddr = xgmac_read_rom_hwaddr;
	edev->set_ethaddr = xgmac_write_hwaddr;

	miibus = &xgmac->miibus;
	miibus->parent = edev->parent;
	miibus->read = xgmac_mdio_read;
	miibus->write = xgmac_mdio_write;
	miibus->priv = xgmac;

	ret = xgmac_phy_reset(dev);
	if (ret)
		return ret;

	ret = xgmac_probe_resources_core(xgmac);
	if (ret < 0) {
		dev_err(dev, "xgmac_probe_resources_core() failed: %d\n", ret);
		return ret;
	}

	ret = xgmac->config->ops->xgmac_probe_resources(dev);
	if (ret < 0) {
		dev_err(dev, "xgmac_probe_resources() failed: %d\n", ret);
		goto err_remove_resources_core;
	}

	ret = mdiobus_register(miibus);
	if (ret)
		return ret;

	return eth_register(edev);

err_remove_resources_core:
	xgmac_remove_resources_core(xgmac);

	return ret;
}

void xgmac_remove(struct device *dev)
{
	struct xgmac_priv *xgmac = dev->priv;

	eth_unregister(&xgmac->netdev);
	mdiobus_unregister(&xgmac->miibus);

	xgmac_remove_resources_core(xgmac);
}
