// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Cirrus Logic EP93xx ethernet MAC / MII driver.
 *
 * Copyright (C) 2009 Matthias Kaehlcke <matthias@kaehlcke.net>
 *
 * Copyright (C) 2004, 2005
 * Cory T. Tusar, Videon Central, Inc., <ctusar@videon-central.com>
 *
 * Based on the original eth.[ch] Cirrus Logic EP93xx Rev D. Ethernet Driver,
 * which is
 *
 * (C) Copyright 2002 2003
 * Adam Bezanson, Network Audio Technologies, Inc.
 * <bezanson@netaudiotech.com>
 */

#include <common.h>
#include <command.h>
#include <init.h>
#include <malloc.h>
#include <io.h>
#include <linux/types.h>
#include <mach/ep93xx-regs.h>
#include <linux/phy.h>
#include <platform_data/eth-ep93xx.h>
#include "ep93xx.h"

#define EP93XX_MAX_PKT_SIZE    1536

static int ep93xx_phy_read(struct mii_bus *bus, int phy_addr, int phy_reg);
static int ep93xx_phy_write(struct mii_bus *bus, int phy_addr, int phy_reg,
			    u16 value);

static inline struct ep93xx_eth_priv *ep93xx_get_priv(struct eth_device *edev)
{
	return (struct ep93xx_eth_priv *)edev->priv;
}

static inline struct mac_regs *ep93xx_get_regs(struct eth_device *edev)
{
	struct ep93xx_eth_priv *priv = ep93xx_get_priv(edev);

	return priv->regs;
}

#if defined(EP93XX_MAC_DEBUG)
/**
 * Dump ep93xx_mac values to the terminal.
 */
static void dump_dev(struct eth_device *edev)
{
	struct ep93xx_eth_priv *priv = ep93xx_get_priv(edev);
	int i;

	printf("\ndump_dev()\n");
	printf("  rx_dq.base	     %p\n", priv->rx_dq.base);
	printf("  rx_dq.current	     %p\n", priv->rx_dq.current);
	printf("  rx_dq.end	     %p\n", priv->rx_dq.end);
	printf("  rx_sq.base	     %p\n", priv->rx_sq.base);
	printf("  rx_sq.current	     %p\n", priv->rx_sq.current);
	printf("  rx_sq.end	     %p\n", priv->rx_sq.end);

	for (i = 0; i < NUMRXDESC; i++)
		printf("  rx_buffer[%2.d]      %p\n", i, NetRxPackets[i]);

	printf("  tx_dq.base	     %p\n", priv->tx_dq.base);
	printf("  tx_dq.current	     %p\n", priv->tx_dq.current);
	printf("  tx_dq.end	     %p\n", priv->tx_dq.end);
	printf("  tx_sq.base	     %p\n", priv->tx_sq.base);
	printf("  tx_sq.current	     %p\n", priv->tx_sq.current);
	printf("  tx_sq.end	     %p\n", priv->tx_sq.end);
}

/**
 * Dump all RX descriptor queue entries to the terminal.
 */
static void dump_rx_descriptor_queue(struct eth_device *edev)
{
	struct ep93xx_eth_priv *priv = ep93xx_get_priv(edev);
	int i;

	printf("\ndump_rx_descriptor_queue()\n");
	printf("  descriptor address	 word1		 word2\n");
	for (i = 0; i < NUMRXDESC; i++) {
		printf("  [ %p ]	     %08X	 %08X\n",
			priv->rx_dq.base + i,
			(priv->rx_dq.base + i)->word1,
			(priv->rx_dq.base + i)->word2);
	}
}

/**
 * Dump all RX status queue entries to the terminal.
 */
static void dump_rx_status_queue(struct eth_device *edev)
{
	struct ep93xx_eth_priv *priv = ep93xx_get_priv(edev);
	int i;

	printf("\ndump_rx_status_queue()\n");
	printf("  descriptor address	 word1		 word2\n");
	for (i = 0; i < NUMRXDESC; i++) {
		printf("  [ %p ]	     %08X	 %08X\n",
			priv->rx_sq.base + i,
			(priv->rx_sq.base + i)->word1,
			(priv->rx_sq.base + i)->word2);
	}
}

/**
 * Dump all TX descriptor queue entries to the terminal.
 */
static void dump_tx_descriptor_queue(struct eth_device *edev)
{
	struct ep93xx_eth_priv *priv = ep93xx_get_priv(edev);
	int i;

	printf("\ndump_tx_descriptor_queue()\n");
	printf("  descriptor address	 word1		 word2\n");
	for (i = 0; i < NUMTXDESC; i++) {
		printf("  [ %p ]	     %08X	 %08X\n",
			priv->tx_dq.base + i,
			(priv->tx_dq.base + i)->word1,
			(priv->tx_dq.base + i)->word2);
	}
}

/**
 * Dump all TX status queue entries to the terminal.
 */
static void dump_tx_status_queue(struct eth_device *edev)
{
	struct ep93xx_eth_priv *priv = ep93xx_get_priv(edev);
	int i;

	printf("\ndump_tx_status_queue()\n");
	printf("  descriptor address	 word1\n");
	for (i = 0; i < NUMTXDESC; i++) {
		printf("  [ %p ]	     %08X\n",
			priv->rx_sq.base + i,
			(priv->rx_sq.base + i)->word1);
	}
}
#else
#define dump_dev(x)
#define dump_rx_descriptor_queue(x)
#define dump_rx_status_queue(x)
#define dump_tx_descriptor_queue(x)
#define dump_tx_status_queue(x)
#endif	/* defined(EP93XX_MAC_DEBUG) */

/**
 * Reset the EP93xx MAC by twiddling the soft reset bit and spinning until
 * it's cleared.
 */
static void ep93xx_eth_reset(struct eth_device *edev)
{
	struct mac_regs *regs = ep93xx_get_regs(edev);
	uint32_t value;

	pr_debug("+ep93xx_eth_reset\n");

	value = readl(&regs->selfctl);
	value |= SELFCTL_RESET;
	writel(value, &regs->selfctl);

	while (readl(&regs->selfctl) & SELFCTL_RESET)
		; /* noop */

	pr_debug("-ep93xx_eth_reset\n");
}

static int ep93xx_eth_init_dev(struct eth_device *edev)
{
	pr_debug("+ep93xx_eth_init_dev\n");

	pr_debug("-ep93xx_eth_init_dev\n");

	return 0;
}

static int ep93xx_eth_open(struct eth_device *edev)
{
	struct ep93xx_eth_priv *priv = ep93xx_get_priv(edev);
	struct mac_regs *regs = ep93xx_get_regs(edev);
	int i;
	int ret;

	pr_debug("+ep93xx_eth_open\n");

	ret = phy_device_connect(edev, &priv->miibus, priv->phy_addr, NULL,
				 0, priv->interface);
	if (ret)
		return ret;

	ep93xx_eth_reset(edev);

	/* Reset the descriptor queues' current and end address values */
	priv->tx_dq.current = priv->tx_dq.base;
	priv->tx_dq.end = (priv->tx_dq.base + NUMTXDESC);

	priv->tx_sq.current = priv->tx_sq.base;
	priv->tx_sq.end = (priv->tx_sq.base + NUMTXDESC);

	priv->rx_dq.current = priv->rx_dq.base;
	priv->rx_dq.end = (priv->rx_dq.base + NUMRXDESC);

	priv->rx_sq.current = priv->rx_sq.base;
	priv->rx_sq.end = (priv->rx_sq.base + NUMRXDESC);

	/*
	 * Set the transmit descriptor and status queues' base address,
	 * current address, and length registers.  Set the maximum frame
	 * length and threshold. Enable the transmit descriptor processor.
	 */
	writel((uint32_t)priv->tx_dq.base, &regs->txdq.badd);
	writel((uint32_t)priv->tx_dq.base, &regs->txdq.curadd);
	writel(sizeof(struct tx_descriptor) * NUMTXDESC, &regs->txdq.blen);

	writel((uint32_t)priv->tx_sq.base, &regs->txstsq.badd);
	writel((uint32_t)priv->tx_sq.base, &regs->txstsq.curadd);
	writel(sizeof(struct tx_status) * NUMTXDESC, &regs->txstsq.blen);

	writel(0x00040000, &regs->txdthrshld);
	writel(0x00040000, &regs->txststhrshld);

	writel((TXSTARTMAX << 0) | (EP93XX_MAX_PKT_SIZE << 16), &regs->maxfrmlen);
	writel(BMCTL_TXEN, &regs->bmctl);

	/*
	 * Set the receive descriptor and status queues' base address,
	 * current address, and length registers.  Enable the receive
	 * descriptor processor.
	 */
	writel((uint32_t)priv->rx_dq.base, &regs->rxdq.badd);
	writel((uint32_t)priv->rx_dq.base, &regs->rxdq.curadd);
	writel(sizeof(struct rx_descriptor) * NUMRXDESC, &regs->rxdq.blen);

	writel((uint32_t)priv->rx_sq.base, &regs->rxstsq.badd);
	writel((uint32_t)priv->rx_sq.base, &regs->rxstsq.curadd);
	writel(sizeof(struct rx_status) * NUMRXDESC, &regs->rxstsq.blen);

	writel(0x00040000, &regs->rxdthrshld);

	writel(BMCTL_RXEN, &regs->bmctl);

	writel(0x00040000, &regs->rxststhrshld);

	/* Wait until the receive descriptor processor is active */
	while (!(readl(&regs->bmsts) & BMSTS_RXACT))
		; /* noop */

	/*
	 * Initialize the RX descriptor queue. Clear the TX descriptor queue.
	 * Clear the RX and TX status queues. Enqueue the RX descriptor and
	 * status entries to the MAC.
	 */
	for (i = 0; i < NUMRXDESC; i++) {
		/* set buffer address */
		(priv->rx_dq.base + i)->word1 = (uint32_t)NetRxPackets[i];

		/* set buffer length, clear buffer index and NSOF */
		(priv->rx_dq.base + i)->word2 = EP93XX_MAX_PKT_SIZE;
	}

	memset(priv->tx_dq.base, 0,
		(sizeof(struct tx_descriptor) * NUMTXDESC));
	memset(priv->rx_sq.base, 0,
		(sizeof(struct rx_status) * NUMRXDESC));
	memset(priv->tx_sq.base, 0,
		(sizeof(struct tx_status) * NUMTXDESC));

	writel(NUMRXDESC, &regs->rxdqenq);
	writel(NUMRXDESC, &regs->rxstsqenq);

	/* Turn on RX and TX */
	writel(RXCTL_IA0 | RXCTL_BA | RXCTL_SRXON |
		RXCTL_RCRCA | RXCTL_MA, &regs->rxctl);
	writel(TXCTL_STXON, &regs->txctl);

	/* Dump data structures if we're debugging */
	dump_dev(edev);
	dump_rx_descriptor_queue(edev);
	dump_rx_status_queue(edev);
	dump_tx_descriptor_queue(edev);
	dump_tx_status_queue(edev);

	pr_debug("-ep93xx_eth_open\n");

	return 0;
}

/**
 * Halt EP93xx MAC transmit and receive by clearing the TxCTL and RxCTL
 * registers.
 */
static void ep93xx_eth_halt(struct eth_device *edev)
{
	struct mac_regs *regs = ep93xx_get_regs(edev);

	pr_debug("+ep93xx_eth_halt\n");

	writel(0x00000000, &regs->rxctl);
	writel(0x00000000, &regs->txctl);

	pr_debug("-ep93xx_eth_halt\n");
}

/**
 * Copy a frame of data from the MAC into the protocol layer for further
 * processing.
 */
static int ep93xx_eth_rcv_packet(struct eth_device *edev)
{
	struct ep93xx_eth_priv *priv = ep93xx_get_priv(edev);
	struct mac_regs *regs = ep93xx_get_regs(edev);
	int ret = -1;

	pr_debug("+ep93xx_eth_rcv_packet\n");

	if (RX_STATUS_RFP(priv->rx_sq.current)) {
		if (RX_STATUS_RWE(priv->rx_sq.current)) {
			/*
			 * We have a good frame. Extract the frame's length
			 * from the current rx_status_queue entry, and copy
			 * the frame's data into NetRxPackets[] of the
			 * protocol stack. We track the total number of
			 * bytes in the frame (nbytes_frame) which will be
			 * used when we pass the data off to the protocol
			 * layer via net_receive().
			 */
			net_receive(edev, (uchar *)priv->rx_dq.current->word1,
				RX_STATUS_FRAME_LEN(priv->rx_sq.current));
			pr_debug("reporting %d bytes...\n",
				RX_STATUS_FRAME_LEN(priv->rx_sq.current));

			ret = 0;

		} else {
			/* Do we have an erroneous packet? */
			pr_err("packet rx error, status %08X %08X\n",
				priv->rx_sq.current->word1,
				priv->rx_sq.current->word2);
			dump_rx_descriptor_queue(edev);
			dump_rx_status_queue(edev);
		}

		/*
		 * Clear the associated status queue entry, and
		 * increment our current pointers to the next RX
		 * descriptor and status queue entries (making sure
		 * we wrap properly).
		 */
		memset((void *)priv->rx_sq.current, 0,
			sizeof(struct rx_status));

		priv->rx_sq.current++;
		if (priv->rx_sq.current >= priv->rx_sq.end)
			priv->rx_sq.current = priv->rx_sq.base;

		priv->rx_dq.current++;
		if (priv->rx_dq.current >= priv->rx_dq.end)
			priv->rx_dq.current = priv->rx_dq.base;

		/*
		 * Finally, return the RX descriptor and status entries
		 * back to the MAC engine, and loop again, checking for
		 * more descriptors to process.
		 */
		writel(1, &regs->rxdqenq);
		writel(1, &regs->rxstsqenq);
	} else {
		ret = 0;
	}

	pr_debug("-ep93xx_eth_rcv_packet %d\n", ret);

	return ret;
}

/**
 * Send a block of data via ethernet.
 */
static int ep93xx_eth_send_packet(struct eth_device *edev,
				void *packet, int length)
{
	struct ep93xx_eth_priv *priv = ep93xx_get_priv(edev);
	struct mac_regs *regs = ep93xx_get_regs(edev);
	int ret = -1;

	pr_debug("+ep93xx_eth_send_packet\n");

	/*
	 * Initialize the TX descriptor queue with the new packet's info.
	 * Clear the associated status queue entry. Enqueue the packet
	 * to the MAC for transmission.
	 */

	/* set buffer address */
	priv->tx_dq.current->word1 = (uint32_t)packet;

	/* set buffer length and EOF bit */
	priv->tx_dq.current->word2 = length | TX_DESC_EOF;

	/* clear tx status */
	priv->tx_sq.current->word1 = 0;

	/* enqueue the TX descriptor */
	writel(1, &regs->txdqenq);

	/* wait for the frame to become processed */
	while (!TX_STATUS_TXFP(priv->tx_sq.current))
		; /* noop */

	if (!TX_STATUS_TXWE(priv->tx_sq.current)) {
		pr_err("packet tx error, status %08X\n",
			priv->tx_sq.current->word1);
		dump_tx_descriptor_queue(edev);
		dump_tx_status_queue(edev);

		/* TODO: Add better error handling? */
		goto eth_send_failed_0;
	}

	ret = 0;
	/* Fall through */

eth_send_failed_0:
	pr_debug("-ep93xx_eth_send_packet %d\n", ret);

	return ret;
}

static int ep93xx_eth_get_ethaddr(struct eth_device *edev,
				unsigned char *mac_addr)
{
	struct mac_regs *regs = ep93xx_get_regs(edev);
	uint32_t value;

	value = readl(&regs->indad);
	mac_addr[0] = value & 0xFF;
	mac_addr[1] = (value >> 8) & 0xFF;
	mac_addr[2] = (value >> 16) & 0xFF;
	mac_addr[3] = (value >> 24) & 0xFF;

	value = readl(&regs->indad_upper);
	mac_addr[4] = value & 0xFF;
	mac_addr[5] = (value >> 8) & 0xFF;

	return 0;
}

static int ep93xx_eth_set_ethaddr(struct eth_device *edev,
				const unsigned char *mac_addr)
{
	struct mac_regs *regs = ep93xx_get_regs(edev);

	writel(AFP_IAPRIMARY, &regs->afp);

	writel(mac_addr[0] | (mac_addr[1] << 8) |
		(mac_addr[2] << 16) | (mac_addr[3] << 24),
		&regs->indad);
	writel(mac_addr[4] | (mac_addr[5] << 8), &regs->indad_upper);

	return 0;
}

static int ep93xx_eth_probe(struct device_d *dev)
{
	struct ep93xx_eth_platform_data *pdata = (struct ep93xx_eth_platform_data *)dev->platform_data;
	struct eth_device *edev;
	struct ep93xx_eth_priv *priv;
	int ret = -1;

	pr_debug("ep93xx_eth_probe()\n");

	edev = xzalloc(sizeof(struct eth_device) +
		sizeof(struct ep93xx_eth_priv));
	edev->priv = (struct ep93xx_eth_priv *)(edev + 1);

	priv = edev->priv;
	priv->regs = (struct mac_regs *)MAC_BASE;

	edev->init = ep93xx_eth_init_dev;
	edev->open = ep93xx_eth_open;
	edev->send = ep93xx_eth_send_packet;
	edev->recv = ep93xx_eth_rcv_packet;
	edev->halt = ep93xx_eth_halt;
	edev->get_ethaddr = ep93xx_eth_get_ethaddr;
	edev->set_ethaddr = ep93xx_eth_set_ethaddr;
	edev->parent = dev;

	if (pdata) {
		priv->interface = pdata->xcv_type;
		priv->phy_addr = pdata->phy_addr;
	} else {
		priv->interface = PHY_INTERFACE_MODE_NA;
		priv->phy_addr = 0;
	}

	priv->miibus.read = ep93xx_phy_read;
	priv->miibus.write = ep93xx_phy_write;
	priv->miibus.parent = dev;
	priv->miibus.priv = edev;

	priv->tx_dq.base = calloc(NUMTXDESC,
				sizeof(struct tx_descriptor));
	if (priv->tx_dq.base == NULL) {
		pr_err("calloc() failed: tx_dq.base");
		goto eth_probe_failed_0;
	}

	priv->tx_sq.base = calloc(NUMTXDESC,
				sizeof(struct tx_status));
	if (priv->tx_sq.base == NULL) {
		pr_err("calloc() failed: tx_sq.base");
		goto eth_probe_failed_1;
	}

	priv->rx_dq.base = calloc(NUMRXDESC,
				sizeof(struct rx_descriptor));
	if (priv->rx_dq.base == NULL) {
		pr_err("calloc() failed: rx_dq.base");
		goto eth_probe_failed_2;
	}

	priv->rx_sq.base = calloc(NUMRXDESC,
				sizeof(struct rx_status));
	if (priv->rx_sq.base == NULL) {
		pr_err("calloc() failed: rx_sq.base");
		goto eth_probe_failed_3;
	}

	mdiobus_register(&priv->miibus);
	eth_register(edev);

	ret = 0;

	goto eth_probe_done;

eth_probe_failed_3:
	free(priv->rx_dq.base);
	/* Fall through */

eth_probe_failed_2:
	free(priv->tx_sq.base);
	/* Fall through */

eth_probe_failed_1:
	free(priv->tx_dq.base);
	/* Fall through */

eth_probe_failed_0:
	/* Fall through */

eth_probe_done:
	return ret;
}

/* -----------------------------------------------------------------------------
 * EP93xx ethernet MII functionality.
 */

/**
 * Maximum MII address we support
 */
#define MII_ADDRESS_MAX			31

/**
 * Maximum MII register address we support
 */
#define MII_REGISTER_MAX		31

/**
 * Read a 16-bit value from an MII register.
 */
static int ep93xx_phy_read(struct mii_bus *bus, int phy_addr, int phy_reg)
{
	struct mac_regs *regs = ep93xx_get_regs(bus->priv);
	int value = -1;
	uint32_t self_ctl;

	pr_debug("+ep93xx_phy_read\n");

	/*
	 * Save the current SelfCTL register value.  Set MAC to send
	 * preamble bits.  Wait for any previous MII command to complete
	 * before issuing the new command.
	 */
	self_ctl = readl(&regs->selfctl);
	writel(self_ctl & ~(1 << 8), &regs->selfctl);

	while (readl(&regs->miists) & MIISTS_BUSY)
		; /* noop */

	/*
	 * Issue the MII 'read' command.  Wait for the command to complete.
	 * Read the MII data value.
	 */
	writel(MIICMD_OPCODE_READ | ((uint32_t)phy_addr << 5) |
		(uint32_t)phy_reg, &regs->miicmd);
	while (readl(&regs->miists) & MIISTS_BUSY)
		; /* noop */

	value = (unsigned short)readl(&regs->miidata);

	/* Restore the saved SelfCTL value and return. */
	writel(self_ctl, &regs->selfctl);

	pr_debug("-ep93xx_phy_read\n");

	return value;
}

/**
 * Write a 16-bit value to an MII register.
 */
static int ep93xx_phy_write(struct mii_bus *bus, int phy_addr,
			int phy_reg, u16 value)
{
	struct mac_regs *regs = ep93xx_get_regs(bus->priv);
	uint32_t self_ctl;

	pr_debug("+ep93xx_phy_write\n");

	/*
	 * Save the current SelfCTL register value.  Set MAC to send
	 * preamble bits.  Wait for any previous MII command to complete
	 * before issuing the new command.
	 */
	self_ctl = readl(&regs->selfctl);
	writel(self_ctl & ~(1 << 8), &regs->selfctl);

	while (readl(&regs->miists) & MIISTS_BUSY)
		; /* noop */

	/* Issue the MII 'write' command.  Wait for the command to complete. */
	writel((uint32_t)value, &regs->miidata);
	writel(MIICMD_OPCODE_WRITE | ((uint32_t)phy_addr << 5) | phy_reg,
		&regs->miicmd);
	while (readl(&regs->miists) & MIISTS_BUSY)
		; /* noop */

	/* Restore the saved SelfCTL value and return. */
	writel(self_ctl, &regs->selfctl);

	pr_debug("-ep93xx_phy_write\n");

	return 0;
}

static struct driver_d ep93xx_eth_driver = {
	.name  = "ep93xx_eth",
	.probe = ep93xx_eth_probe,
};
device_platform_driver(ep93xx_eth_driver);
