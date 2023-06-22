// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2016, NVIDIA CORPORATION.
 * Copyright (c) 2019, Ahmad Fatoum, Pengutronix
 *
 * Portions based on U-Boot's rtl8169.c and dwc_eth_qos.
 */

#include <common.h>
#include <init.h>
#include <gpio.h>
#include <linux/gpio/consumer.h>
#include <dma.h>
#include <net.h>
#include <of_net.h>
#include <of_gpio.h>
#include <linux/iopoll.h>
#include <linux/time.h>
#include <linux/sizes.h>

#include "designware_eqos.h"

/* Core registers */

#define EQOS_MAC_REGS_BASE 0x000
struct eqos_mac_regs {
	u32 config;				/* 0x000 */
	u32 ext_config;				/* 0x004 */
	u32 packet_filter;			/* 0x008 */
	u32 unused_004[(0x070 - 0x00C) / 4];	/* 0x00C */
	u32 q0_tx_flow_ctrl;			/* 0x070 */
	u32 unused_070[(0x090 - 0x074) / 4];	/* 0x074 */
	u32 rx_flow_ctrl;			/* 0x090 */
	u32 unused_094;				/* 0x094 */
	u32 txq_prty_map0;			/* 0x098 */
	u32 unused_09c;				/* 0x09c */
	u32 rxq_ctrl0;				/* 0x0a0 */
	u32 unused_0a4;				/* 0x0a4 */
	u32 rxq_ctrl2;				/* 0x0a8 */
	u32 unused_0ac[(0x0dc - 0x0ac) / 4];	/* 0x0ac */
	u32 us_tic_counter;			/* 0x0dc */
	u32 unused_0e0[(0x11c - 0x0e0) / 4];	/* 0x0e0 */
	u32 hw_feature0;			/* 0x11c */
	u32 hw_feature1;			/* 0x120 */
	u32 hw_feature2;			/* 0x124 */
	u32 unused_128[(0x200 - 0x128) / 4];	/* 0x128 */
	u32 mdio_address;			/* 0x200 */
	u32 mdio_data;				/* 0x204 */
	u32 unused_208[(0x300 - 0x208) / 4];	/* 0x208 */
	u32 macaddr0hi;				/* 0x300 */
	u32 macaddr0lo;				/* 0x304 */
};

#define EQOS_MAC_CONFIGURATION_GPSLCE			BIT(23)
#define EQOS_MAC_CONFIGURATION_CST			BIT(21)
#define EQOS_MAC_CONFIGURATION_ACS			BIT(20)
#define EQOS_MAC_CONFIGURATION_WD			BIT(19)
#define EQOS_MAC_CONFIGURATION_JD			BIT(17)
#define EQOS_MAC_CONFIGURATION_JE			BIT(16)
#define EQOS_MAC_CONFIGURATION_PS			BIT(15)
#define EQOS_MAC_CONFIGURATION_FES			BIT(14)
#define EQOS_MAC_CONFIGURATION_DM			BIT(13)
#define EQOS_MAC_CONFIGURATION_TE			BIT(1)
#define EQOS_MAC_CONFIGURATION_RE			BIT(0)

#define EQOS_MAC_PACKET_FILTER_PR			BIT(0)	/* Promiscuous mode */
#define EQOS_MAC_PACKET_FILTER_PCF			BIT(7)	/* Pass Control Frames */

#define EQOS_MAC_Q0_TX_FLOW_CTRL_PT_SHIFT		16
#define EQOS_MAC_Q0_TX_FLOW_CTRL_PT_MASK		0xffff
#define EQOS_MAC_Q0_TX_FLOW_CTRL_TFE			BIT(1)

#define EQOS_MAC_RX_FLOW_CTRL_RFE			BIT(0)

#define EQOS_MAC_TXQ_PRTY_MAP0_PSTQ0_SHIFT		0
#define EQOS_MAC_TXQ_PRTY_MAP0_PSTQ0_MASK		0xff

#define EQOS_MAC_RXQ_CTRL2_PSRQ0_SHIFT			0
#define EQOS_MAC_RXQ_CTRL2_PSRQ0_MASK			0xff

#define EQOS_MAC_HW_FEATURE1_TXFIFOSIZE_SHIFT		6
#define EQOS_MAC_HW_FEATURE1_TXFIFOSIZE_MASK		0x1f
#define EQOS_MAC_HW_FEATURE1_RXFIFOSIZE_SHIFT		0
#define EQOS_MAC_HW_FEATURE1_RXFIFOSIZE_MASK		0x1f

#define EQOS_MTL_REGS_BASE 0xd00
struct eqos_mtl_regs {
	u32 txq0_operation_mode;			/* 0xd00 */
	u32 unused_d04;				/* 0xd04 */
	u32 txq0_debug;				/* 0xd08 */
	u32 unused_d0c[(0xd18 - 0xd0c) / 4];	/* 0xd0c */
	u32 txq0_quantum_weight;			/* 0xd18 */
	u32 unused_d1c[(0xd30 - 0xd1c) / 4];	/* 0xd1c */
	u32 rxq0_operation_mode;			/* 0xd30 */
	u32 unused_d34;				/* 0xd34 */
	u32 rxq0_debug;				/* 0xd38 */
};

#define EQOS_MTL_TXQ0_OPERATION_MODE_TQS_SHIFT		16
#define EQOS_MTL_TXQ0_OPERATION_MODE_TQS_MASK		0x1ff
#define EQOS_MTL_TXQ0_OPERATION_MODE_TXQEN_SHIFT	2
#define EQOS_MTL_TXQ0_OPERATION_MODE_TXQEN_MASK		3
#define EQOS_MTL_TXQ0_OPERATION_MODE_TXQEN_ENABLED	2
#define EQOS_MTL_TXQ0_OPERATION_MODE_TSF		BIT(1)
#define EQOS_MTL_TXQ0_OPERATION_MODE_FTQ		BIT(0)

#define EQOS_MTL_TXQ0_DEBUG_TXQSTS			BIT(4)
#define EQOS_MTL_TXQ0_DEBUG_TRCSTS_SHIFT		1
#define EQOS_MTL_TXQ0_DEBUG_TRCSTS_MASK			3

#define EQOS_MTL_RXQ0_OPERATION_MODE_RQS_SHIFT		20
#define EQOS_MTL_RXQ0_OPERATION_MODE_RQS_MASK		0x3ff
#define EQOS_MTL_RXQ0_OPERATION_MODE_RFD_SHIFT		14
#define EQOS_MTL_RXQ0_OPERATION_MODE_RFD_MASK		0x3f
#define EQOS_MTL_RXQ0_OPERATION_MODE_RFA_SHIFT		8
#define EQOS_MTL_RXQ0_OPERATION_MODE_RFA_MASK		0x3f
#define EQOS_MTL_RXQ0_OPERATION_MODE_EHFC		BIT(7)
#define EQOS_MTL_RXQ0_OPERATION_MODE_RSF		BIT(5)
#define EQOS_MTL_RXQ0_OPERATION_MODE_FEP		BIT(4)
#define EQOS_MTL_RXQ0_OPERATION_MODE_FUP		BIT(3)

#define EQOS_MTL_RXQ0_DEBUG_PRXQ_SHIFT			16
#define EQOS_MTL_RXQ0_DEBUG_PRXQ_MASK			0x7fff
#define EQOS_MTL_RXQ0_DEBUG_RXQSTS_SHIFT		4
#define EQOS_MTL_RXQ0_DEBUG_RXQSTS_MASK			3

#define EQOS_DMA_REGS_BASE 0x1000
struct eqos_dma_regs {
	u32 mode;				/* 0x1000 */
	u32 sysbus_mode;			/* 0x1004 */
	u32 unused_1008[(0x1100 - 0x1008) / 4];	/* 0x1008 */
	u32 ch0_control;			/* 0x1100 */
	u32 ch0_tx_control;			/* 0x1104 */
	u32 ch0_rx_control;			/* 0x1108 */
	u32 unused_110c;			/* 0x110c */
	u32 ch0_txdesc_list_haddress;		/* 0x1110 */
	u32 ch0_txdesc_list_address;		/* 0x1114 */
	u32 ch0_rxdesc_list_haddress;		/* 0x1118 */
	u32 ch0_rxdesc_list_address;		/* 0x111c */
	u32 ch0_txdesc_tail_pointer;		/* 0x1120 */
	u32 unused_1124;			/* 0x1124 */
	u32 ch0_rxdesc_tail_pointer;		/* 0x1128 */
	u32 ch0_txdesc_ring_length;		/* 0x112c */
	u32 ch0_rxdesc_ring_length;		/* 0x1130 */
};

#define EQOS_DMA_MODE_SWR				BIT(0)

#define EQOS_DMA_SYSBUS_MODE_RD_OSR_LMT_SHIFT		16
#define EQOS_DMA_SYSBUS_MODE_RD_OSR_LMT_MASK		0xf
#define EQOS_DMA_SYSBUS_MODE_EAME			BIT(11)
#define EQOS_DMA_SYSBUS_MODE_BLEN16			BIT(3)
#define EQOS_DMA_SYSBUS_MODE_BLEN8			BIT(2)
#define EQOS_DMA_SYSBUS_MODE_BLEN4			BIT(1)

#define EQOS_DMA_CH0_CONTROL_PBLX8			BIT(16)

#define EQOS_DMA_CH0_TX_CONTROL_TXPBL_SHIFT		16
#define EQOS_DMA_CH0_TX_CONTROL_TXPBL_MASK		0x3f
#define EQOS_DMA_CH0_TX_CONTROL_OSP			BIT(4)
#define EQOS_DMA_CH0_TX_CONTROL_ST			BIT(0)

#define EQOS_DMA_CH0_RX_CONTROL_RXPBL_SHIFT		16
#define EQOS_DMA_CH0_RX_CONTROL_RXPBL_MASK		0x3f
#define EQOS_DMA_CH0_RX_CONTROL_RBSZ_SHIFT		1
#define EQOS_DMA_CH0_RX_CONTROL_RBSZ_MASK		0x3fff
#define EQOS_DMA_CH0_RX_CONTROL_SR			BIT(0)

/* Descriptors */

#define EQOS_DESCRIPTOR_WORDS	4
#define EQOS_DESCRIPTOR_SIZE	(EQOS_DESCRIPTOR_WORDS * 4)
/* We assume ARCH_DMA_MINALIGN >= 16; 16 is the EQOS HW minimum */
#define EQOS_DESCRIPTOR_ALIGN	64
#define EQOS_DESCRIPTORS_TX	4
#define EQOS_DESCRIPTORS_RX	64
#define EQOS_DESCRIPTORS_NUM	(EQOS_DESCRIPTORS_TX + EQOS_DESCRIPTORS_RX)
#define EQOS_DESCRIPTORS_SIZE	ALIGN(EQOS_DESCRIPTORS_NUM * \
				      EQOS_DESCRIPTOR_SIZE, EQOS_DESCRIPTOR_ALIGN)
#define EQOS_BUFFER_ALIGN	EQOS_DESCRIPTOR_ALIGN
#define EQOS_MAX_PACKET_SIZE	ALIGN(1568, EQOS_DESCRIPTOR_ALIGN)

struct eqos_desc {
	u32 des0; /* PA of buffer 1 or TSO header */
	u32 des1; /* PA of buffer 2 with descriptor rings */
	u32 des2; /* Length, VLAN, Timestamps, Interrupts */
	u32 des3; /* All other flags */
};

#define EQOS_DESC3_OWN		BIT(31)
#define EQOS_DESC3_FD		BIT(29)
#define EQOS_DESC3_LD		BIT(28)
#define EQOS_DESC3_BUF1V	BIT(24)

#define EQOS_MDIO_ADDR(reg)		((addr << 21) & GENMASK(25, 21))
#define EQOS_MDIO_REG(reg)		((reg << 16) & GENMASK(20, 16))
#define EQOS_MDIO_CLK_CSR(clk_csr)	((clk_csr << 8) & GENMASK(11, 8))

#define MII_BUSY		(1 << 0)

static int eqos_phy_reset(struct device *dev, struct eqos *eqos)
{
	struct gpio_desc *phy_reset;
	u32 delays[3] = { 0, 0, 0 };

	phy_reset = gpiod_get_optional(dev, "snps,reset", GPIOF_OUT_INIT_ACTIVE);
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

static int eqos_mdio_wait_idle(struct eqos *eqos)
{
	u32 idle;
	return readl_poll_timeout(&eqos->mac_regs->mdio_address, idle,
				  !(idle & MII_BUSY), 10 * USEC_PER_MSEC);
}

static int eqos_mdio_read(struct mii_bus *bus, int addr, int reg)
{
	struct eqos *eqos = bus->priv;
	u32 miiaddr = MII_BUSY;
	int ret;

	ret = eqos_mdio_wait_idle(eqos);
	if (ret) {
		dev_err(&bus->dev, "MDIO not idle at entry\n");
		return ret;
	}

	miiaddr |= EQOS_MDIO_ADDR(addr) | EQOS_MDIO_REG(reg);
	miiaddr |= EQOS_MDIO_CLK_CSR(eqos->ops->clk_csr);
	miiaddr |= EQOS_MDIO_ADDR_GOC_READ << EQOS_MDIO_ADDR_GOC_SHIFT;

	writel(0, &eqos->mac_regs->mdio_data);
	writel(miiaddr, &eqos->mac_regs->mdio_address);

	ret = eqos_mdio_wait_idle(eqos);
	if (ret) {
		dev_err(&bus->dev, "MDIO read didn't complete\n");
		return ret;
	}

	return readl(&eqos->mac_regs->mdio_data) & 0xffff;
}

static int eqos_mdio_write(struct mii_bus *bus, int addr, int reg, u16 data)
{
	struct eqos *eqos = bus->priv;
	u32 miiaddr = MII_BUSY;
	int ret;

	ret = eqos_mdio_wait_idle(eqos);
	if (ret) {
		dev_err(&bus->dev, "MDIO not idle at entry\n");
		return ret;
	}

	miiaddr |= EQOS_MDIO_ADDR(addr) | EQOS_MDIO_REG(reg);
	miiaddr |= EQOS_MDIO_CLK_CSR(eqos->ops->clk_csr);
	miiaddr |= EQOS_MDIO_ADDR_GOC_WRITE << EQOS_MDIO_ADDR_GOC_SHIFT;

	writel(data, &eqos->mac_regs->mdio_data);
	writel(miiaddr, &eqos->mac_regs->mdio_address);

	ret = eqos_mdio_wait_idle(eqos);
	if (ret)
		dev_err(&bus->dev, "MDIO read didn't complete\n");

	return ret;
}


static inline void eqos_set_full_duplex(struct eqos *eqos)
{
	setbits_le32(&eqos->mac_regs->config, EQOS_MAC_CONFIGURATION_DM);
}

static inline void eqos_set_half_duplex(struct eqos *eqos)
{
	clrbits_le32(&eqos->mac_regs->config, EQOS_MAC_CONFIGURATION_DM);

	/* WAR: Flush TX queue when switching to half-duplex */
	setbits_le32(&eqos->mtl_regs->txq0_operation_mode,
		     EQOS_MTL_TXQ0_OPERATION_MODE_FTQ);
}

static inline void eqos_set_gmii_speed(struct eqos *eqos)
{
	clrbits_le32(&eqos->mac_regs->config,
		     EQOS_MAC_CONFIGURATION_PS | EQOS_MAC_CONFIGURATION_FES);
}

static inline void eqos_set_mii_speed_100(struct eqos *eqos)
{
	setbits_le32(&eqos->mac_regs->config,
		     EQOS_MAC_CONFIGURATION_PS | EQOS_MAC_CONFIGURATION_FES);
}

static inline void eqos_set_mii_speed_10(struct eqos *eqos)
{
	clrsetbits_le32(&eqos->mac_regs->config,
			EQOS_MAC_CONFIGURATION_FES, EQOS_MAC_CONFIGURATION_PS);
}

void eqos_adjust_link(struct eth_device *edev)
{
	struct eqos *eqos = edev->priv;
	unsigned speed = edev->phydev->speed;

	if (edev->phydev->duplex)
		eqos_set_full_duplex(eqos);
	else
		eqos_set_half_duplex(eqos);

	switch (speed) {
	case SPEED_1000:
		eqos_set_gmii_speed(eqos);
		break;
	case SPEED_100:
		eqos_set_mii_speed_100(eqos);
		break;
	case SPEED_10:
		eqos_set_mii_speed_10(eqos);
		break;
	default:
		eqos_warn(eqos, "invalid speed %d\n", speed);
		return;
	}
}

int eqos_get_ethaddr(struct eth_device *edev, unsigned char *mac)
{
	return -EOPNOTSUPP;
}

int eqos_set_ethaddr(struct eth_device *edev, const unsigned char *mac)
{
	struct eqos *eqos = edev->priv;
	__le32 mac_hi, mac_lo;

	memcpy(eqos->macaddr, mac, ETH_ALEN);

	/* Update the MAC address */
	memcpy(&mac_hi, &mac[4], 2);
	memcpy(&mac_lo, &mac[0], 4);

	__raw_writel(mac_hi, &eqos->mac_regs->macaddr0hi);
	__raw_writel(mac_lo, &eqos->mac_regs->macaddr0lo);

	return 0;
}

static int eqos_set_promisc(struct eth_device *edev, bool enable)
{
	struct eqos *eqos = edev->priv;
	u32 mask;

	mask = EQOS_MAC_PACKET_FILTER_PR | EQOS_MAC_PACKET_FILTER_PCF;

	if (enable)
		setbits_le32(&eqos->mac_regs->packet_filter, mask);
	else
		clrbits_le32(&eqos->mac_regs->packet_filter, mask);

	return 0;
}

/* Get PHY out of power saving mode.  If this is needed elsewhere then
 * consider making it part of phy-core and adding a resume method to
 * the phy device ops.  */
static int phy_resume(struct phy_device *phydev)
{
	int bmcr;

	// Bus will be NULL if a fixed-link is used.
	if (!phydev->bus)
		return 0;

	bmcr = phy_read(phydev, MII_BMCR);
	if (bmcr < 0)
		return bmcr;

	if (bmcr & BMCR_PDOWN) {
		bmcr &= ~BMCR_PDOWN;
		return phy_write(phydev, MII_BMCR, bmcr);
	}

	return 0;
}

static int eqos_start(struct eth_device *edev)
{
	struct eqos *eqos = edev->priv;
	u32 val, tx_fifo_sz, rx_fifo_sz, tqs, rqs, pbl;
	unsigned long last_rx_desc;
	unsigned long rate;
	u32 mode_set;
	int ret;
	int i;

	ret = phy_device_connect(edev, &eqos->miibus, eqos->phy_addr,
				 eqos->ops->adjust_link, 0, eqos->interface);
	if (ret)
		return ret;

	setbits_le32(&eqos->dma_regs->mode, EQOS_DMA_MODE_SWR);

	ret = readl_poll_timeout(&eqos->dma_regs->mode, mode_set,
				 !(mode_set & EQOS_DMA_MODE_SWR),
				 100 * USEC_PER_MSEC);
	if (ret) {
		eqos_err(eqos, "EQOS_DMA_MODE_SWR stuck: 0x%08x\n", mode_set);
		return ret;
	}

	/* Reset above clears MAC address */
	eqos_set_ethaddr(edev, eqos->macaddr);

	/* Required for accurate time keeping with EEE counters */
	rate = eqos->ops->get_csr_clk_rate(eqos);

	val = (rate / USEC_PER_SEC) - 1; /* -1 because the data sheet says so */
	writel(val, &eqos->mac_regs->us_tic_counter);

	/* Before we reset the mac, we must insure the PHY is not powered down
	 * as the dw controller needs all clock domains to be running, including
	 * the PHY clock, to come out of a mac reset.  */
	if (edev->phydev) {
		ret = phy_resume(edev->phydev);
		if (ret)
			return ret;
	}

	/* Configure MTL */

	/* Enable Store and Forward mode for TX */
	/* Program Tx operating mode */
	setbits_le32(&eqos->mtl_regs->txq0_operation_mode,
		     EQOS_MTL_TXQ0_OPERATION_MODE_TSF |
		     (EQOS_MTL_TXQ0_OPERATION_MODE_TXQEN_ENABLED <<
		      EQOS_MTL_TXQ0_OPERATION_MODE_TXQEN_SHIFT));

	/* Transmit Queue weight */
	writel(0x10, &eqos->mtl_regs->txq0_quantum_weight);

	/* Enable Store and Forward mode for RX, since no jumbo frame */
	setbits_le32(&eqos->mtl_regs->rxq0_operation_mode,
		     EQOS_MTL_RXQ0_OPERATION_MODE_RSF |
		     EQOS_MTL_RXQ0_OPERATION_MODE_FEP |
		     EQOS_MTL_RXQ0_OPERATION_MODE_FUP);

	/* Transmit/Receive queue fifo size; use all RAM for 1 queue */
	val = readl(&eqos->mac_regs->hw_feature1);
	tx_fifo_sz = (val >> EQOS_MAC_HW_FEATURE1_TXFIFOSIZE_SHIFT) &
		EQOS_MAC_HW_FEATURE1_TXFIFOSIZE_MASK;
	rx_fifo_sz = (val >> EQOS_MAC_HW_FEATURE1_RXFIFOSIZE_SHIFT) &
		EQOS_MAC_HW_FEATURE1_RXFIFOSIZE_MASK;

	/*
	 * r/tx_fifo_sz is encoded as log2(n / 128). Undo that by shifting.
	 * r/tqs is encoded as (n / 256) - 1.
	 */
	tqs = (128 << tx_fifo_sz) / 256 - 1;
	rqs = (128 << rx_fifo_sz) / 256 - 1;

	clrsetbits_le32(&eqos->mtl_regs->txq0_operation_mode,
			EQOS_MTL_TXQ0_OPERATION_MODE_TQS_MASK <<
			EQOS_MTL_TXQ0_OPERATION_MODE_TQS_SHIFT,
			tqs << EQOS_MTL_TXQ0_OPERATION_MODE_TQS_SHIFT);

	clrsetbits_le32(&eqos->mtl_regs->rxq0_operation_mode,
			EQOS_MTL_RXQ0_OPERATION_MODE_RQS_MASK <<
			EQOS_MTL_RXQ0_OPERATION_MODE_RQS_SHIFT,
			rqs << EQOS_MTL_RXQ0_OPERATION_MODE_RQS_SHIFT);

	/* Flow control used only if each channel gets 4KB or more FIFO */
	if (rqs >= ((SZ_4K / 256) - 1)) {
		u32 rfd, rfa;

		setbits_le32(&eqos->mtl_regs->rxq0_operation_mode,
			     EQOS_MTL_RXQ0_OPERATION_MODE_EHFC);

		/*
		 * Set Threshold for Activating Flow Contol space for min 2
		 * frames ie, (1500 * 1) = 1500 bytes.
		 *
		 * Set Threshold for Deactivating Flow Contol for space of
		 * min 1 frame (frame size 1500bytes) in receive fifo
		 */
		if (rqs == ((SZ_4K / 256) - 1)) {
			/*
			 * This violates the above formula because of FIFO size
			 * limit therefore overflow may occur inspite of this.
			 */
			rfd = 0x3;	/* Full-3K */
			rfa = 0x1;	/* Full-1.5K */
		} else if (rqs == ((SZ_8K / 256) - 1)) {
			rfd = 0x6;	/* Full-4K */
			rfa = 0xa;	/* Full-6K */
		} else if (rqs == ((16384 / 256) - 1)) {
			rfd = 0x6;	/* Full-4K */
			rfa = 0x12;	/* Full-10K */
		} else {
			rfd = 0x6;	/* Full-4K */
			rfa = 0x1E;	/* Full-16K */
		}

		clrsetbits_le32(&eqos->mtl_regs->rxq0_operation_mode,
				(EQOS_MTL_RXQ0_OPERATION_MODE_RFD_MASK <<
				 EQOS_MTL_RXQ0_OPERATION_MODE_RFD_SHIFT) |
				(EQOS_MTL_RXQ0_OPERATION_MODE_RFA_MASK <<
				 EQOS_MTL_RXQ0_OPERATION_MODE_RFA_SHIFT),
				(rfd <<
				 EQOS_MTL_RXQ0_OPERATION_MODE_RFD_SHIFT) |
				(rfa <<
				 EQOS_MTL_RXQ0_OPERATION_MODE_RFA_SHIFT));
	}

	/* Configure MAC */

	clrsetbits_le32(&eqos->mac_regs->rxq_ctrl0,
			EQOS_MAC_RXQ_CTRL0_RXQ0EN_MASK <<
			EQOS_MAC_RXQ_CTRL0_RXQ0EN_SHIFT,
			eqos->ops->config_mac <<
			EQOS_MAC_RXQ_CTRL0_RXQ0EN_SHIFT);

	/* Set TX flow control parameters */
	/* Set Pause Time */
	setbits_le32(&eqos->mac_regs->q0_tx_flow_ctrl,
		     0xffff << EQOS_MAC_Q0_TX_FLOW_CTRL_PT_SHIFT);
	/* Assign priority for TX flow control */
	clrbits_le32(&eqos->mac_regs->txq_prty_map0,
		     EQOS_MAC_TXQ_PRTY_MAP0_PSTQ0_MASK <<
		     EQOS_MAC_TXQ_PRTY_MAP0_PSTQ0_SHIFT);
	/* Assign priority for RX flow control */
	clrbits_le32(&eqos->mac_regs->rxq_ctrl2,
		     EQOS_MAC_RXQ_CTRL2_PSRQ0_MASK <<
		     EQOS_MAC_RXQ_CTRL2_PSRQ0_SHIFT);
	/* Enable flow control */
	setbits_le32(&eqos->mac_regs->q0_tx_flow_ctrl,
		     EQOS_MAC_Q0_TX_FLOW_CTRL_TFE);
	setbits_le32(&eqos->mac_regs->rx_flow_ctrl,
		     EQOS_MAC_RX_FLOW_CTRL_RFE);

	clrsetbits_le32(&eqos->mac_regs->config,
			EQOS_MAC_CONFIGURATION_GPSLCE |
			EQOS_MAC_CONFIGURATION_WD |
			EQOS_MAC_CONFIGURATION_JD |
			EQOS_MAC_CONFIGURATION_JE,
			EQOS_MAC_CONFIGURATION_CST |
			EQOS_MAC_CONFIGURATION_ACS);

	/* Configure DMA */

	/* Enable OSP mode */
	setbits_le32(&eqos->dma_regs->ch0_tx_control,
		     EQOS_DMA_CH0_TX_CONTROL_OSP);

	/* RX buffer size. Must be a multiple of bus width */
	clrsetbits_le32(&eqos->dma_regs->ch0_rx_control,
			EQOS_DMA_CH0_RX_CONTROL_RBSZ_MASK <<
			EQOS_DMA_CH0_RX_CONTROL_RBSZ_SHIFT,
			EQOS_MAX_PACKET_SIZE <<
			EQOS_DMA_CH0_RX_CONTROL_RBSZ_SHIFT);

	setbits_le32(&eqos->dma_regs->ch0_control,
		     EQOS_DMA_CH0_CONTROL_PBLX8);

	/*
	 * Burst length must be < 1/2 FIFO size.
	 * FIFO size in tqs is encoded as (n / 256) - 1.
	 * Each burst is n * 8 (PBLX8) * 16 (AXI width) == 128 bytes.
	 * Half of n * 256 is n * 128, so pbl == tqs, modulo the -1.
	 */
	pbl = tqs + 1;
	if (pbl > 32)
		pbl = 32;
	clrsetbits_le32(&eqos->dma_regs->ch0_tx_control,
			EQOS_DMA_CH0_TX_CONTROL_TXPBL_MASK <<
			EQOS_DMA_CH0_TX_CONTROL_TXPBL_SHIFT,
			pbl << EQOS_DMA_CH0_TX_CONTROL_TXPBL_SHIFT);

	clrsetbits_le32(&eqos->dma_regs->ch0_rx_control,
			EQOS_DMA_CH0_RX_CONTROL_RXPBL_MASK <<
			EQOS_DMA_CH0_RX_CONTROL_RXPBL_SHIFT,
			8 << EQOS_DMA_CH0_RX_CONTROL_RXPBL_SHIFT);

	/* DMA performance configuration */
	val = (2 << EQOS_DMA_SYSBUS_MODE_RD_OSR_LMT_SHIFT) |
		EQOS_DMA_SYSBUS_MODE_EAME | EQOS_DMA_SYSBUS_MODE_BLEN16 |
		EQOS_DMA_SYSBUS_MODE_BLEN8 | EQOS_DMA_SYSBUS_MODE_BLEN4;
	writel(val, &eqos->dma_regs->sysbus_mode);

	/* Set up descriptors */

	eqos->tx_currdescnum = eqos->rx_currdescnum = 0;

	for (i = 0; i < EQOS_DESCRIPTORS_RX; i++) {
		struct eqos_desc *rx_desc = &eqos->rx_descs[i];

		writel(EQOS_DESC3_BUF1V | EQOS_DESC3_OWN, &rx_desc->des3);
	}

	writel(0, &eqos->dma_regs->ch0_txdesc_list_haddress);
	writel((ulong)eqos->tx_descs, &eqos->dma_regs->ch0_txdesc_list_address);
	writel(EQOS_DESCRIPTORS_TX - 1,
	       &eqos->dma_regs->ch0_txdesc_ring_length);

	writel(0, &eqos->dma_regs->ch0_rxdesc_list_haddress);
	writel((ulong)eqos->rx_descs, &eqos->dma_regs->ch0_rxdesc_list_address);
	writel(EQOS_DESCRIPTORS_RX - 1,
	       &eqos->dma_regs->ch0_rxdesc_ring_length);

	/* Enable everything */

	setbits_le32(&eqos->mac_regs->config,
		     EQOS_MAC_CONFIGURATION_TE | EQOS_MAC_CONFIGURATION_RE);

	setbits_le32(&eqos->dma_regs->ch0_tx_control,
		     EQOS_DMA_CH0_TX_CONTROL_ST);
	setbits_le32(&eqos->dma_regs->ch0_rx_control,
		     EQOS_DMA_CH0_RX_CONTROL_SR);

	/* TX tail pointer not written until we need to TX a packet */
	/*
	 * Point RX tail pointer at last descriptor. Ideally, we'd point at the
	 * first descriptor, implying all descriptors were available. However,
	 * that's not distinguishable from none of the descriptors being
	 * available.
	 */
	last_rx_desc = (ulong)&eqos->rx_descs[(EQOS_DESCRIPTORS_RX - 1)];
	writel(last_rx_desc, &eqos->dma_regs->ch0_rxdesc_tail_pointer);

	return 0;
}

static void eqos_stop(struct eth_device *edev)
{
	struct eqos *eqos = edev->priv;
	int i;

	/* Disable TX DMA */
	clrbits_le32(&eqos->dma_regs->ch0_tx_control,
		     EQOS_DMA_CH0_TX_CONTROL_ST);

	/* Wait for TX all packets to drain out of MTL */
	for (i = 0; i < 1000000; i++) {
		u32 val = readl(&eqos->mtl_regs->txq0_debug);
		u32 trcsts = (val >> EQOS_MTL_TXQ0_DEBUG_TRCSTS_SHIFT) &
			EQOS_MTL_TXQ0_DEBUG_TRCSTS_MASK;
		u32 txqsts = val & EQOS_MTL_TXQ0_DEBUG_TXQSTS;
		if ((trcsts != 1) && (!txqsts))
			break;
	}

	/* Turn off MAC TX and RX */
	clrbits_le32(&eqos->mac_regs->config,
		     EQOS_MAC_CONFIGURATION_TE | EQOS_MAC_CONFIGURATION_RE);

	/* Wait for all RX packets to drain out of MTL */
	for (i = 0; i < 1000000; i++) {
		u32 val = readl(&eqos->mtl_regs->rxq0_debug);
		u32 prxq = (val >> EQOS_MTL_RXQ0_DEBUG_PRXQ_SHIFT) &
			EQOS_MTL_RXQ0_DEBUG_PRXQ_MASK;
		u32 rxqsts = (val >> EQOS_MTL_RXQ0_DEBUG_RXQSTS_SHIFT) &
			EQOS_MTL_RXQ0_DEBUG_RXQSTS_MASK;
		if ((!prxq) && (!rxqsts))
			break;
	}

	/* Turn off RX DMA */
	clrbits_le32(&eqos->dma_regs->ch0_rx_control,
		     EQOS_DMA_CH0_RX_CONTROL_SR);
}

static int eqos_send(struct eth_device *edev, void *packet, int length)
{
	struct eqos *eqos = edev->priv;
	struct eqos_desc *tx_desc;
	dma_addr_t dma;
	u32 des3;
	int ret;

	tx_desc = &eqos->tx_descs[eqos->tx_currdescnum];
	eqos->tx_currdescnum++;
	eqos->tx_currdescnum %= EQOS_DESCRIPTORS_TX;

	dma = dma_map_single(edev->parent, packet, length, DMA_TO_DEVICE);
	if (dma_mapping_error(edev->parent, dma))
		return -EFAULT;

	tx_desc->des0 = (unsigned long)dma;
	tx_desc->des1 = 0;
	tx_desc->des2 = length;
	/*
	 * Make sure the compiler doesn't reorder the _OWN write below, before
	 * the writes to the rest of the descriptor.
	 */
	barrier();

	writel(EQOS_DESC3_OWN | EQOS_DESC3_FD | EQOS_DESC3_LD | length, &tx_desc->des3);
	writel((ulong)(tx_desc + 1), &eqos->dma_regs->ch0_txdesc_tail_pointer);

	ret = readl_poll_timeout(&tx_desc->des3, des3,
				  !(des3 & EQOS_DESC3_OWN),
				  100 * USEC_PER_MSEC);

	dma_unmap_single(edev->parent, dma, length, DMA_TO_DEVICE);

	if (ret == -ETIMEDOUT)
		eqos_dbg(eqos, "TX timeout\n");

	return ret;
}

static int eqos_recv(struct eth_device *edev)
{
	struct eqos *eqos = edev->priv;
	struct eqos_desc *rx_desc;
	void *frame;
	int length;

	rx_desc = &eqos->rx_descs[eqos->rx_currdescnum];
	if (readl(&rx_desc->des3) & EQOS_DESC3_OWN)
		return 0;

	frame = phys_to_virt(rx_desc->des0);
	length = rx_desc->des3 & 0x7fff;

	dma_sync_single_for_cpu(edev->parent, (unsigned long)frame,
				length, DMA_FROM_DEVICE);
	net_receive(edev, frame, length);
	dma_sync_single_for_device(edev->parent, (unsigned long)frame,
				   length, DMA_FROM_DEVICE);

	rx_desc->des0 = (unsigned long)frame;
	rx_desc->des1 = 0;
	rx_desc->des2 = 0;
	/*
	 * Make sure that if HW sees the _OWN write below, it will see all the
	 * writes to the rest of the descriptor too.
	 */
	rx_desc->des3 |= EQOS_DESC3_BUF1V;
	rx_desc->des3 |= EQOS_DESC3_OWN;
	barrier();

	writel((ulong)rx_desc, &eqos->dma_regs->ch0_rxdesc_tail_pointer);

	eqos->rx_currdescnum++;
	eqos->rx_currdescnum %= EQOS_DESCRIPTORS_RX;

	return 0;
}

static int eqos_init_resources(struct eqos *eqos)
{
	struct eth_device *edev = &eqos->netdev;
	int ret = -ENOMEM;
	void *descs;
	void *p;
	int i;

	descs = dma_alloc_coherent(EQOS_DESCRIPTORS_SIZE, DMA_ADDRESS_BROKEN);
	if (!descs)
		goto err;

	eqos->tx_descs = (struct eqos_desc *)descs;
	eqos->rx_descs = (eqos->tx_descs + EQOS_DESCRIPTORS_TX);

	p = dma_alloc(EQOS_DESCRIPTORS_RX * EQOS_MAX_PACKET_SIZE);
	if (!p)
		goto err_free_desc;

	for (i = 0; i < EQOS_DESCRIPTORS_RX; i++) {
		struct eqos_desc *rx_desc = &eqos->rx_descs[i];
		dma_addr_t dma;

		dma = dma_map_single(edev->parent, p, EQOS_MAX_PACKET_SIZE, DMA_FROM_DEVICE);
		if (dma_mapping_error(edev->parent, dma)) {
			ret = -EFAULT;
			goto err_free_rx_bufs;
		}

		rx_desc->des0 = dma;

		p += EQOS_MAX_PACKET_SIZE;
	}

	return 0;

err_free_rx_bufs:
	dma_free(phys_to_virt(eqos->rx_descs[0].des0));
err_free_desc:
	dma_free_coherent(descs, 0, EQOS_DESCRIPTORS_SIZE);
err:

	return ret;
}

static int eqos_init(struct device *dev, struct eqos *eqos)
{
	int ret;

	ret = eqos_init_resources(eqos);
	if (ret) {
		dev_err(dev, "eqos_init_resources() failed: %s\n", strerror(-ret));
		return ret;
	}

	if (eqos->ops->init)
		ret = eqos->ops->init(dev, eqos);

	return ret;
}

static void eqos_probe_dt(struct device *dev, struct eqos *eqos)
{
	struct device_node *child;

	eqos->interface = of_get_phy_mode(dev->of_node);
	eqos->phy_addr = -1;

	/* Set MDIO bus device node, if present. */
	for_each_child_of_node(dev->of_node, child) {
		if (of_device_is_compatible(child, "snps,dwmac-mdio") ||
		    (child->name && !of_node_cmp(child->name, "mdio"))) {
			eqos->miibus.dev.of_node = child;
			break;
		}
	}
}

int eqos_probe(struct device *dev, const struct eqos_ops *ops, void *priv)
{
	struct device_node *np = dev->of_node;
	struct mii_bus *miibus;
	struct resource *iores;
	struct eqos *eqos;
	struct eth_device *edev;
	int ret;

	eqos = xzalloc(sizeof(*eqos));

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	eqos->regs = IOMEM(iores->start);

	eqos->mac_regs = eqos->regs + EQOS_MAC_REGS_BASE;
	eqos->mtl_regs = eqos->regs + EQOS_MTL_REGS_BASE;
	eqos->dma_regs = eqos->regs + EQOS_DMA_REGS_BASE;
	eqos->ops = ops;
	eqos->priv = priv;

	eqos_probe_dt(dev, eqos);

	edev = &eqos->netdev;

	dev->priv = edev->priv = eqos;

	edev->parent = dev;
	edev->open = eqos_start;
	edev->send = eqos_send;
	edev->recv = eqos_recv;
	edev->halt = eqos_stop;
	edev->get_ethaddr = ops->get_ethaddr;
	edev->set_ethaddr = ops->set_ethaddr;
	edev->set_promisc = eqos_set_promisc;

	miibus = &eqos->miibus;
	miibus->parent = edev->parent;
	miibus->read = eqos_mdio_read;
	miibus->write = eqos_mdio_write;
	miibus->priv = eqos;

	miibus->dev.of_node = of_get_compatible_child(np, "snps,dwmac-mdio");
	if (!miibus->dev.of_node)
		miibus->dev.of_node = of_get_child_by_name(np, "mdio");

	ret = eqos_init(dev, eqos);
	if (ret)
		return ret;

	ret = eqos_phy_reset(dev, eqos);
	if (ret)
		return ret;

	ret = mdiobus_register(miibus);
	if (ret)
		return ret;

	return eth_register(edev);
}

void eqos_remove(struct device *dev)
{
	struct eqos *eqos = dev->priv;

	eth_unregister(&eqos->netdev);

	mdiobus_unregister(&eqos->miibus);

	dma_free(phys_to_virt(eqos->rx_descs[0].des0));
	dma_free_coherent(eqos->tx_descs, 0, EQOS_DESCRIPTORS_SIZE);
}
