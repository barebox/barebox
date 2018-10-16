// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2012 Jan Luebbe <j.luebbe@pengutronix.de>
 *
 * Ethernet driver for TI TMS320DM644x (DaVinci) chips.
 *
 * Copyright (C) 2007 Sergey Kubushyn <ksi@koi8.net>
 *
 * Parts shamelessly stolen from TI's dm644x_emac.c. Original copyright
 * follows:
 *
 * ----------------------------------------------------------------------------
 *
 * dm644x_emac.c
 *
 * TI DaVinci (DM644X) EMAC peripheral driver source for DV-EVM
 *
 * Copyright (C) 2005 Texas Instruments.
 *
 * Modifications:
 * ver. 1.0: Sep 2005, Anant Gole - Created EMAC version for uBoot.
 * ver  1.1: Nov 2005, Anant Gole - Extended the RX logic for multiple descriptors
 */

#include <common.h>
#include <dma.h>
#include <io.h>
#include <clock.h>
#include <net.h>
#include <malloc.h>
#include <init.h>
#include <asm/system.h>
#include <linux/phy.h>
#include <mach/emac_defs.h>
#include <of_net.h>
#include "davinci_emac.h"

struct davinci_emac_priv {
	struct device_d *dev;
	struct eth_device edev;

	/* EMAC Addresses */
	void __iomem *adap_emac; /* = EMAC_BASE_ADDR */
	void __iomem *adap_ewrap; /* = EMAC_WRAPPER_BASE_ADDR */

	/* EMAC descriptors */
	void __iomem *emac_desc_base; /* = EMAC_WRAPPER_RAM_ADDR */
	void __iomem *emac_rx_desc; /* = EMAC_WRAPPER_RAM_ADDR + EMAC_RX_DESC_BASE */
	void __iomem *emac_tx_desc; /* = EMAC_WRAPPER_RAM_ADDR + EMAC_TX_DESC_BASE */
	void __iomem *emac_rx_active_head; /* = 0 */
	void __iomem *emac_rx_active_tail; /* = 0 */
	int emac_rx_queue_active; /* = 0 */

	/* Receive packet buffers */
	unsigned char *emac_rx_buffers; /* [EMAC_MAX_RX_BUFFERS * (EMAC_MAX_ETHERNET_PKT_SIZE + EMAC_PKT_ALIGN)] */

	/* PHY-specific information */
	phy_interface_t interface;

	/* mac_addr[0] goes out on the wire first */
	uint8_t mac_addr[6];
};

#ifdef EMAC_HW_RAM_ADDR
static inline uint32_t BD_TO_HW(void __iomem *x)
{
	if (x == 0)
		return 0;

	return (uint32_t)(x) - EMAC_WRAPPER_RAM_ADDR + EMAC_HW_RAM_ADDR;
}

static inline void __iomem *HW_TO_BD(uint32_t x)
{
	if (x == 0)
		return 0;

	return (struct emac_desc*)(x - EMAC_HW_RAM_ADDR + EMAC_WRAPPER_RAM_ADDR);
}
#else
#define BD_TO_HW(x)     (x)
#define HW_TO_BD(x)     (x)
#endif

struct davinci_mdio_priv {
	struct device_d *dev;
	struct mii_bus miibus;
	void __iomem *adap_mdio; /* = EMAC_MDIO_BASE_ADDR */
};

static void davinci_eth_mdio_enable(struct davinci_mdio_priv *priv)
{
	uint32_t	clkdiv;

	clkdiv = (EMAC_MDIO_BUS_FREQ / EMAC_MDIO_CLOCK_FREQ) - 1;

	dev_dbg(priv->dev, "mdio_enable + 0x%08x\n",
		readl(priv->adap_mdio + EMAC_MDIO_CONTROL));
	writel((clkdiv & 0xff) |
		MDIO_CONTROL_ENABLE |
		MDIO_CONTROL_FAULT |
		MDIO_CONTROL_FAULT_ENABLE,
		priv->adap_mdio + EMAC_MDIO_CONTROL);
	dev_dbg(priv->dev, "mdio_enable - 0x%08x\n",
		readl(priv->adap_mdio + EMAC_MDIO_CONTROL));

	while (readl(priv->adap_mdio + EMAC_MDIO_CONTROL) & MDIO_CONTROL_IDLE);
}

/* wait until hardware is ready for another user access */
static int wait_for_user_access(struct davinci_mdio_priv *priv, uint32_t *val)
{
	u32 tmp;
	uint64_t start = get_time_ns();

	do {
		tmp = readl(priv->adap_mdio + EMAC_MDIO_USERACCESS0);

		if (!(tmp & MDIO_USERACCESS0_GO))
			break;

		if (is_timeout(start, 100 * MSECOND)) {
			dev_err(priv->dev, "timeout waiting for user access\n");
			return -ETIMEDOUT;
		}
	} while (1);

	if (val)
		*val = tmp;

	return 0;
}


static int davinci_miibus_read(struct mii_bus *bus, int addr, int reg)
{
	struct davinci_mdio_priv *priv = bus->priv;
	uint16_t value;
	int tmp, ret;

	ret = wait_for_user_access(priv, NULL);
	if (ret)
		return ret;

	writel(MDIO_USERACCESS0_GO |
		MDIO_USERACCESS0_WRITE_READ |
		((reg & 0x1f) << 21) |
		((addr & 0x1f) << 16),
		priv->adap_mdio + EMAC_MDIO_USERACCESS0);

	ret = wait_for_user_access(priv, &tmp);
	if (ret)
		return ret;

	if (tmp & MDIO_USERACCESS0_ACK) {
		value = tmp & 0xffff;
		dev_dbg(priv->dev, "davinci_miibus_read: addr=0x%02x reg=0x%02x value=0x%04x\n",
			   addr, reg, value);
		return value;
	}

	return -1;
}

static int davinci_miibus_write(struct mii_bus *bus, int addr, int reg, u16 value)
{
	struct davinci_mdio_priv *priv = bus->priv;
	int ret;

	ret = wait_for_user_access(priv, NULL);
	if (ret)
		return ret;

	dev_dbg(priv->dev, "davinci_miibus_write: addr=0x%02x reg=0x%02x value=0x%04x\n",
		   addr, reg, value);
	writel(MDIO_USERACCESS0_GO |
				MDIO_USERACCESS0_WRITE_WRITE |
				((reg & 0x1f) << 21) |
				((addr & 0x1f) << 16) |
				(value & 0xffff),
		priv->adap_mdio + EMAC_MDIO_USERACCESS0);

	return wait_for_user_access(priv, NULL);
}

static int davinci_emac_get_ethaddr(struct eth_device *edev, unsigned char *adr)
{
	return -1;
}

/*
 * This function must be called before emac_open() if you want to override
 * the default mac address.
 */
static int davinci_emac_set_ethaddr(struct eth_device *edev, const unsigned char *addr)
{
	struct davinci_emac_priv *priv = edev->priv;
	int i;

	for (i = 0; i < sizeof(priv->mac_addr); i++)
		priv->mac_addr[i] = addr[i];
	return 0;
}

static int davinci_emac_init(struct eth_device *edev)
{
	struct davinci_emac_priv *priv = edev->priv;
	uint32_t cnt;

	/* Set DMA head and completion pointers to 0 */
	for(cnt = 0; cnt < 8; cnt++) {
		writel(0, (void *)priv->adap_emac + EMAC_TX0HDP + 4 * cnt);
		writel(0, (void *)priv->adap_emac + EMAC_RX0HDP + 4 * cnt);
		writel(0, (void *)priv->adap_emac + EMAC_TX0CP + 4 * cnt);
		writel(0, (void *)priv->adap_emac + EMAC_RX0CP + 4 * cnt);
	}

	dev_dbg(&edev->dev, "* emac_init\n");
	return 0;
}

static int davinci_emac_open(struct eth_device *edev)
{
	struct davinci_emac_priv *priv = edev->priv;
	uint32_t cnt;
	void __iomem *rx_desc;
	unsigned long mac_hi, mac_lo;
	int ret;

	dev_dbg(priv->dev, "+ emac_open\n");

	dev_dbg(priv->dev, "emac->TXIDVER: 0x%08x\n",
		readl(priv->adap_emac + EMAC_TXIDVER));
	dev_dbg(priv->dev, "emac->RXIDVER: 0x%08x\n",
		readl(priv->adap_emac + EMAC_RXIDVER));

	/* Reset EMAC module and disable interrupts in wrapper */
	writel(1, priv->adap_emac + EMAC_SOFTRESET);
	while (readl(priv->adap_emac + EMAC_SOFTRESET) != 0);
	writel(1, priv->adap_ewrap + EMAC_EWRAP_SOFTRESET);
	while (readl(priv->adap_ewrap + EMAC_EWRAP_SOFTRESET) != 0);

	writel(0, priv->adap_ewrap + EMAC_EWRAP_C0RXEN);
	writel(0, priv->adap_ewrap + EMAC_EWRAP_C1RXEN);
	writel(0, priv->adap_ewrap + EMAC_EWRAP_C2RXEN);
	writel(0, priv->adap_ewrap + EMAC_EWRAP_C0TXEN);
	writel(0, priv->adap_ewrap + EMAC_EWRAP_C1TXEN);
	writel(0, priv->adap_ewrap + EMAC_EWRAP_C2TXEN);
	writel(0, priv->adap_ewrap + EMAC_EWRAP_C0MISCEN);
	writel(0, priv->adap_ewrap + EMAC_EWRAP_C1MISCEN);
	writel(0, priv->adap_ewrap + EMAC_EWRAP_C2MISCEN);

	rx_desc = priv->emac_rx_desc;

	/*
	 * Set MAC Addresses & Init multicast Hash to 0 (disable any multicast
	 * receive)
	 * Use channel 0 only - other channels are disabled
	 */
	writel(0, priv->adap_emac + EMAC_MACINDEX);
	mac_hi = (priv->mac_addr[3] << 24) |
		 (priv->mac_addr[2] << 16) |
		 (priv->mac_addr[1] << 8)  |
		 (priv->mac_addr[0]);
	mac_lo = (priv->mac_addr[5] << 8) |
		 (priv->mac_addr[4]);

	writel(mac_hi, priv->adap_emac + EMAC_MACADDRHI);
	writel(mac_lo | EMAC_MAC_ADDR_IS_VALID | EMAC_MAC_ADDR_MATCH,
	       priv->adap_emac + EMAC_MACADDRLO);

	/* Set source MAC address - REQUIRED */
	writel(mac_hi, priv->adap_emac + EMAC_MACSRCADDRHI);
	writel(mac_lo, priv->adap_emac + EMAC_MACSRCADDRLO);

	/* Set DMA head and completion pointers to 0 */
	for(cnt = 0; cnt < 8; cnt++) {
		writel(0, (void *)priv->adap_emac + EMAC_TX0HDP + 4 * cnt);
		writel(0, (void *)priv->adap_emac + EMAC_RX0HDP + 4 * cnt);
		writel(0, (void *)priv->adap_emac + EMAC_TX0CP + 4 * cnt);
		writel(0, (void *)priv->adap_emac + EMAC_RX0CP + 4 * cnt);
	}

	/* Clear Statistics (do this before setting MacControl register) */
	for(cnt = 0; cnt < EMAC_NUM_STATS; cnt++)
		writel(0, (void *)priv->adap_emac + EMAC_RXGOODFRAMES + 4 * cnt);

	/* No multicast addressing */
	writel(0, priv->adap_emac + EMAC_MACHASH1);
	writel(0, priv->adap_emac + EMAC_MACHASH2);

	writel(0x01, priv->adap_emac + EMAC_TXCONTROL);
	writel(0x01, priv->adap_emac + EMAC_RXCONTROL);

	/* Create RX queue and set receive process in place */
	priv->emac_rx_active_head = priv->emac_rx_desc;
	for (cnt = 0; cnt < EMAC_MAX_RX_BUFFERS; cnt++) {
		writel(BD_TO_HW(rx_desc + EMAC_DESC_SIZE), rx_desc + EMAC_DESC_NEXT);
		writel(&priv->emac_rx_buffers[cnt * (EMAC_MAX_ETHERNET_PKT_SIZE + EMAC_PKT_ALIGN)], rx_desc + EMAC_DESC_BUFFER);
		writel(EMAC_MAX_ETHERNET_PKT_SIZE, rx_desc + EMAC_DESC_BUFF_OFF_LEN);
		writel(EMAC_CPPI_OWNERSHIP_BIT, rx_desc + EMAC_DESC_PKT_FLAG_LEN);
		rx_desc += EMAC_DESC_SIZE;
	}

	/* Set the last descriptor's "next" parameter to 0 to end the RX desc list */
	rx_desc -= EMAC_DESC_SIZE;
	writel(0, rx_desc + EMAC_DESC_NEXT);
	priv->emac_rx_active_tail = rx_desc;
	priv->emac_rx_queue_active = 1;

	/* Enable TX/RX */
	writel(EMAC_MAX_ETHERNET_PKT_SIZE, priv->adap_emac + EMAC_RXMAXLEN);
	writel(0, priv->adap_emac + EMAC_RXBUFFEROFFSET);

	/* No fancy configs - Use this for promiscous for debug - EMAC_RXMBPENABLE_RXCAFEN_ENABLE */
	writel(EMAC_RXMBPENABLE_RXBROADEN, priv->adap_emac + EMAC_RXMBPENABLE);

	/* Enable ch 0 only */
	writel(0x01, priv->adap_emac + EMAC_RXUNICASTSET);

	/* Enable MII interface and full duplex mode (using RMMI) */
	writel((EMAC_MACCONTROL_MIIEN_ENABLE |
		EMAC_MACCONTROL_FULLDUPLEX_ENABLE |
		EMAC_MACCONTROL_RMIISPEED_100),
	       priv->adap_emac + EMAC_MACCONTROL);

	/* Start receive process */
	writel(BD_TO_HW(priv->emac_rx_desc), priv->adap_emac + EMAC_RX0HDP);

	ret = phy_device_connect(edev, NULL, -1, NULL, 0, priv->interface);
	if (ret)
		return ret;

	dev_dbg(priv->dev, "- emac_open\n");

	return 0;
}

/* EMAC Channel Teardown */
static void davinci_eth_ch_teardown(struct davinci_emac_priv *priv, int ch)
{
	uint32_t dly = 0xff;
	uint32_t cnt;

	dev_dbg(priv->dev, "+ emac_ch_teardown\n");

	if (ch == EMAC_CH_TX) {
		/* Init TX channel teardown */
		writel(0, priv->adap_emac + EMAC_TXTEARDOWN);
		for(cnt = 0; cnt != 0xfffffffc; cnt = readl(priv->adap_emac + EMAC_TX0CP)) {
			/* Wait here for Tx teardown completion interrupt to occur
			 * Note: A task delay can be called here to pend rather than
			 * occupying CPU cycles - anyway it has been found that teardown
			 * takes very few cpu cycles and does not affect functionality */
			 dly--;
			 udelay(1);
			 if (dly == 0)
				break;
		}
		writel(cnt, priv->adap_emac + EMAC_TX0CP);
		writel(0, priv->adap_emac + EMAC_TX0HDP);
	} else {
		/* Init RX channel teardown */
		writel(0, priv->adap_emac + EMAC_RXTEARDOWN);
		for(cnt = 0; cnt != 0xfffffffc; cnt = readl(priv->adap_emac + EMAC_RX0CP)) {
			/* Wait here for Rx teardown completion interrupt to occur
			 * Note: A task delay can be called here to pend rather than
			 * occupying CPU cycles - anyway it has been found that teardown
			 * takes very few cpu cycles and does not affect functionality */
			 dly--;
			 udelay(1);
			 if (dly == 0)
				break;
		}
		writel(cnt, priv->adap_emac + EMAC_RX0CP);
		writel(0, priv->adap_emac + EMAC_RX0HDP);
	}

	dev_dbg(priv->dev, "- emac_ch_teardown\n");
}

static void davinci_emac_halt(struct eth_device *edev)
{
	struct davinci_emac_priv *priv = edev->priv;

	dev_dbg(priv->dev, "+ emac_halt\n");

	davinci_eth_ch_teardown(priv, EMAC_CH_TX);	/* TX Channel teardown */
	davinci_eth_ch_teardown(priv, EMAC_CH_RX);	/* RX Channel teardown */

	/* Reset EMAC module and disable interrupts in wrapper */
	writel(1, priv->adap_emac + EMAC_SOFTRESET);
	writel(1, priv->adap_ewrap + EMAC_EWRAP_SOFTRESET);

	writel(0, priv->adap_ewrap + EMAC_EWRAP_C0RXEN);
	writel(0, priv->adap_ewrap + EMAC_EWRAP_C1RXEN);
	writel(0, priv->adap_ewrap + EMAC_EWRAP_C2RXEN);
	writel(0, priv->adap_ewrap + EMAC_EWRAP_C0TXEN);
	writel(0, priv->adap_ewrap + EMAC_EWRAP_C1TXEN);
	writel(0, priv->adap_ewrap + EMAC_EWRAP_C2TXEN);
	writel(0, priv->adap_ewrap + EMAC_EWRAP_C0MISCEN);
	writel(0, priv->adap_ewrap + EMAC_EWRAP_C1MISCEN);
	writel(0, priv->adap_ewrap + EMAC_EWRAP_C2MISCEN);

	dev_dbg(priv->dev, "- emac_halt\n");
}

/*
 * This function sends a single packet on the network
 * and returns 0 on successful transmit or negative for error
 */
static int davinci_emac_send(struct eth_device *edev, void *packet, int length)
{
	struct davinci_emac_priv *priv = edev->priv;
	uint64_t start;
	int ret_status;

	dev_dbg(priv->dev, "+ emac_send (length %d)\n", length);

	/* Check packet size and if < EMAC_MIN_ETHERNET_PKT_SIZE, pad it up */
	if (length < EMAC_MIN_ETHERNET_PKT_SIZE) {
		length = EMAC_MIN_ETHERNET_PKT_SIZE;
	}

	/* Populate the TX descriptor */
	writel(0, priv->emac_tx_desc + EMAC_DESC_NEXT);
	writel((uint8_t *) packet, priv->emac_tx_desc + EMAC_DESC_BUFFER);
	writel((length & 0xffff), priv->emac_tx_desc + EMAC_DESC_BUFF_OFF_LEN);
	writel(((length & 0xffff) | EMAC_CPPI_SOP_BIT |
				    EMAC_CPPI_OWNERSHIP_BIT |
				    EMAC_CPPI_EOP_BIT),
		priv->emac_tx_desc + EMAC_DESC_PKT_FLAG_LEN);
	dma_sync_single_for_device((unsigned long)packet, length, DMA_TO_DEVICE);
	/* Send the packet */
	writel(BD_TO_HW(priv->emac_tx_desc), priv->adap_emac + EMAC_TX0HDP);

	/* Wait for packet to complete or link down */
	start = get_time_ns();
	while (1) {
		if (readl(priv->adap_emac + EMAC_TXINTSTATRAW) & 0x01) {
			/* Acknowledge the TX descriptor */
			writel(BD_TO_HW(priv->emac_tx_desc), priv->adap_emac + EMAC_TX0CP);
			ret_status = 0;
			break;
		}
		if (is_timeout(start, 100 * MSECOND)) {
			ret_status = -ETIMEDOUT;
			break;
		}
	}
	dma_sync_single_for_cpu((unsigned long)packet, length, DMA_TO_DEVICE);

	dev_dbg(priv->dev, "- emac_send (ret_status %i)\n", ret_status);
	return ret_status;
}

/*
 * This function handles receipt of a packet from the network
 */
static int davinci_emac_recv(struct eth_device *edev)
{
	struct davinci_emac_priv *priv = edev->priv;
	void __iomem *rx_curr_desc, *curr_desc, *tail_desc;
	unsigned char *pkt;
	int status, len, ret = -1;

	dev_dbg(priv->dev, "+ emac_recv\n");

	rx_curr_desc = priv->emac_rx_active_head;
	status = readl(rx_curr_desc + EMAC_DESC_PKT_FLAG_LEN);
	if (status & EMAC_CPPI_OWNERSHIP_BIT) {
		ret = 0;
		goto out;
	}

	if (status & EMAC_CPPI_RX_ERROR_FRAME) {
		/* Error in packet - discard it and requeue desc */
		dev_warn(priv->dev, "WARN: emac_rcv_pkt: Error in packet\n");
	} else {
		pkt = (unsigned char *)readl(rx_curr_desc + EMAC_DESC_BUFFER);
		len = readl(rx_curr_desc + EMAC_DESC_BUFF_OFF_LEN) & 0xffff;
		dev_dbg(priv->dev, "| emac_recv got packet (length %i)\n", len);
		dma_sync_single_for_cpu((unsigned long)pkt, len, DMA_FROM_DEVICE);
		net_receive(edev, pkt, len);
		dma_sync_single_for_device((unsigned long)pkt, len, DMA_FROM_DEVICE);
		ret = len;
	}

	/* Ack received packet descriptor */
	writel(BD_TO_HW(rx_curr_desc), priv->adap_emac + EMAC_RX0CP);
	curr_desc = rx_curr_desc;
	priv->emac_rx_active_head = HW_TO_BD(readl(rx_curr_desc + EMAC_DESC_NEXT));

	if (status & EMAC_CPPI_EOQ_BIT) {
		if (priv->emac_rx_active_head) {
			writel(BD_TO_HW(priv->emac_rx_active_head),
				priv->adap_emac + EMAC_RX0HDP);
		} else {
			priv->emac_rx_queue_active = 0;
			dev_info(priv->dev, "INFO:emac_rcv_packet: RX Queue not active\n");
		}
	}

	/* Recycle RX descriptor */
	writel(EMAC_MAX_ETHERNET_PKT_SIZE, rx_curr_desc + EMAC_DESC_BUFF_OFF_LEN);
	writel(EMAC_CPPI_OWNERSHIP_BIT, rx_curr_desc + EMAC_DESC_PKT_FLAG_LEN);
	writel(0, rx_curr_desc + EMAC_DESC_NEXT);

	if (priv->emac_rx_active_head == 0) {
		dev_info(priv->dev, "INFO: emac_rcv_pkt: active queue head = 0\n");
		priv->emac_rx_active_head = curr_desc;
		priv->emac_rx_active_tail = curr_desc;
		if (priv->emac_rx_queue_active != 0) {
			writel(BD_TO_HW(priv->emac_rx_active_head), priv->adap_emac + EMAC_RX0HDP);
			dev_info(priv->dev, "INFO: emac_rcv_pkt: active queue head = 0, HDP fired\n");
			priv->emac_rx_queue_active = 1;
		}
	} else {
		tail_desc = priv->emac_rx_active_tail;
		priv->emac_rx_active_tail = curr_desc;
		writel(BD_TO_HW(curr_desc), tail_desc + EMAC_DESC_NEXT);
		status = readl(tail_desc + EMAC_DESC_PKT_FLAG_LEN);
		if (status & EMAC_CPPI_EOQ_BIT) {
			writel(BD_TO_HW(curr_desc), priv->adap_emac + EMAC_RX0HDP);
			status &= ~EMAC_CPPI_EOQ_BIT;
			writel(status, tail_desc + EMAC_DESC_PKT_FLAG_LEN);
		}
	}

out:
	dev_dbg(priv->dev, "- emac_recv\n");

	return ret;
}

static int davinci_emac_probe(struct device_d *dev)
{
	struct resource *iores;
	struct davinci_emac_priv *priv;
	uint32_t ctrl_reg_offset;
	uint32_t ctrl_ram_offset;
	struct device_node *np = dev->device_node;

	dev_dbg(dev, "+ emac_probe\n");

	priv = xzalloc(sizeof(*priv));
	dev->priv = priv;

	priv->dev = dev;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);

	priv->adap_ewrap = IOMEM(iores->start);

	of_property_read_u32(np, "ti,davinci-ctrl-reg-offset", &ctrl_reg_offset);
	priv->adap_emac = IOMEM(iores->start) + ctrl_reg_offset;

	of_property_read_u32(np, "ti,davinci-ctrl-ram-offset", &ctrl_ram_offset);
	priv->emac_desc_base = IOMEM(iores->start) + ctrl_ram_offset;

	/* EMAC descriptors */
	priv->emac_rx_desc = priv->emac_desc_base + EMAC_RX_DESC_BASE;
	priv->emac_tx_desc = priv->emac_desc_base + EMAC_TX_DESC_BASE;
	priv->emac_rx_active_head = NULL;
	priv->emac_rx_active_tail = NULL;
	priv->emac_rx_queue_active = 0;

	/* Receive packet buffers */
	priv->emac_rx_buffers = xmemalign(4096, EMAC_MAX_RX_BUFFERS * (EMAC_MAX_ETHERNET_PKT_SIZE + EMAC_PKT_ALIGN));

	priv->edev.priv = priv;
	priv->edev.init = davinci_emac_init;
	priv->edev.open = davinci_emac_open;
	priv->edev.halt = davinci_emac_halt;
	priv->edev.send = davinci_emac_send;
	priv->edev.recv = davinci_emac_recv;
	priv->edev.get_ethaddr = davinci_emac_get_ethaddr;
	priv->edev.set_ethaddr = davinci_emac_set_ethaddr;
	priv->edev.parent = dev;

	priv->interface = of_get_phy_mode(np);

	eth_register(&priv->edev);

	dev_dbg(dev, "- emac_probe\n");
	return 0;
}

static void davinci_emac_remove(struct device_d *dev)
{
	struct davinci_emac_priv *priv = dev->priv;

	davinci_emac_halt(&priv->edev);
}

static __maybe_unused struct of_device_id davinci_emac_dt_ids[] = {
	{
		.compatible = "ti,am3517-emac",
	}, {
		/* sentinel */
	}
};

static struct driver_d davinci_emac_driver = {
	.name   = "davinci_emac",
	.probe  = davinci_emac_probe,
	.remove = davinci_emac_remove,
	.of_compatible = DRV_OF_COMPAT(davinci_emac_dt_ids),
};
device_platform_driver(davinci_emac_driver);

static int davinci_mdio_probe(struct device_d *dev)
{
	struct resource *iores;
	struct davinci_mdio_priv *priv;
	int ret;
	uint32_t clkdiv;

	priv = xzalloc(sizeof(*priv));

	priv->dev = dev;
	priv->miibus.read = davinci_miibus_read;
	priv->miibus.write = davinci_miibus_write;
	priv->miibus.priv = priv;
	priv->miibus.parent = dev;

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);

	priv->adap_mdio = IOMEM(iores->start);

	davinci_eth_mdio_enable(priv);

	/* Init MDIO & get link state */
	clkdiv = (EMAC_MDIO_BUS_FREQ / EMAC_MDIO_CLOCK_FREQ) - 1;
	writel((clkdiv & 0xff) | MDIO_CONTROL_ENABLE | MDIO_CONTROL_FAULT,
		priv->adap_mdio + EMAC_MDIO_CONTROL);

	ret = mdiobus_register(&priv->miibus);
	if (ret)
		goto err;

	return 0;

err:
	free(priv);

	return ret;
}

static __maybe_unused struct of_device_id davinci_mdio_dt_ids[] = {
	{
		.compatible = "ti,davinci_mdio",
	}, {
		/* sentinel */
	}
};

static struct driver_d davinci_mdio_driver = {
	.name   = "davinci_mdio",
	.probe  = davinci_mdio_probe,
	.of_compatible = DRV_OF_COMPAT(davinci_mdio_dt_ids),
};
device_platform_driver(davinci_mdio_driver);
