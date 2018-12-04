// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Altera TSE Network driver
 *
 * Copyright (C) 2008 Altera Corporation.
 * Copyright (C) 2010 Thomas Chou <thomas@wytron.com.tw>
 * Copyright (C) 2011 Franck JULLIEN, <elec4fun@gmail.com>
 */

#include <common.h>
#include <dma.h>
#include <net.h>
#include <init.h>
#include <clock.h>
#include <linux/mii.h>
#include <linux/phy.h>
#include <linux/err.h>

#include <io.h>
#include <asm/dma-mapping.h>

#include "altera_tse.h"

/* This is a generic routine that the SGDMA mode-specific routines
 * call to populate a descriptor.
 * arg1     :pointer to first SGDMA descriptor.
 * arg2     :pointer to next  SGDMA descriptor.
 * arg3     :Address to where data to be written.
 * arg4     :Address from where data to be read.
 * arg5     :no of byte to transaction.
 * arg6     :variable indicating to generate start of packet or not
 * arg7     :read fixed
 * arg8     :write fixed
 * arg9     :read burst
 * arg10    :write burst
 * arg11    :atlantic_channel number
 */
static void alt_sgdma_construct_descriptor_burst(
	struct alt_sgdma_descriptor *desc,
	struct alt_sgdma_descriptor *next,
	uint32_t *read_addr,
	uint32_t *write_addr,
	uint16_t length_or_eop,
	uint8_t generate_eop,
	uint8_t read_fixed,
	uint8_t write_fixed_or_sop,
	uint8_t read_burst,
	uint8_t write_burst,
	uint8_t atlantic_channel)
{
	uint32_t temp;

	/*
	 * Mark the "next" descriptor as "not" owned by hardware. This prevents
	 * The SGDMA controller from continuing to process the chain. This is
	 * done as a single IO write to bypass cache, without flushing
	 * the entire descriptor, since only the 8-bit descriptor status must
	 * be flushed.
	 */
	if (!next)
		printf("Next descriptor not defined!!\n");

	temp = readb(&next->descriptor_control);
	writeb(temp & ~ALT_SGDMA_DESCRIPTOR_CONTROL_OWNED_BY_HW_MSK,
		&next->descriptor_control);

	writel((uint32_t)read_addr, &desc->source);
	writel((uint32_t)write_addr, &desc->destination);
	writel((uint32_t)next, &desc->next);

	writel(0, &desc->source_pad);
	writel(0, &desc->destination_pad);
	writel(0, &desc->next_pad);
	writew(length_or_eop, &desc->bytes_to_transfer);
	writew(0, &desc->actual_bytes_transferred);
	writeb(0, &desc->descriptor_status);

	/* SGDMA burst not currently supported */
	writeb(0, &desc->read_burst);
	writeb(0, &desc->write_burst);

	/*
	 * Set the descriptor control block as follows:
	 * - Set "owned by hardware" bit
	 * - Optionally set "generate EOP" bit
	 * - Optionally set the "read from fixed address" bit
	 * - Optionally set the "write to fixed address bit (which serves
	 *   serves as a "generate SOP" control bit in memory-to-stream mode).
	 * - Set the 4-bit atlantic channel, if specified
	 *
	 * Note this step is performed after all other descriptor information
	 * has been filled out so that, if the controller already happens to be
	 * pointing at this descriptor, it will not run (via the "owned by
	 * hardware" bit) until all other descriptor has been set up.
	 */

	writeb((ALT_SGDMA_DESCRIPTOR_CONTROL_OWNED_BY_HW_MSK) |
		(generate_eop ? ALT_SGDMA_DESCRIPTOR_CONTROL_GENERATE_EOP_MSK : 0) |
		(read_fixed ? ALT_SGDMA_DESCRIPTOR_CONTROL_READ_FIXED_ADDRESS_MSK : 0) |
		(write_fixed_or_sop ? ALT_SGDMA_DESCRIPTOR_CONTROL_WRITE_FIXED_ADDRESS_MSK : 0) |
		(atlantic_channel ? ((atlantic_channel & 0x0F) << 3) : 0),
		&desc->descriptor_control);
}

static int alt_sgdma_do_sync_transfer(struct alt_sgdma_registers *dev,
					struct alt_sgdma_descriptor *desc)
{
	uint32_t temp;
	uint64_t start;
	uint64_t tout;

	/* Wait for any pending transfers to complete */
	tout = ALT_TSE_SGDMA_BUSY_WATCHDOG_TOUT * MSECOND;

	start = get_time_ns();

	while (readl(&dev->status) & ALT_SGDMA_STATUS_BUSY_MSK) {
		if (is_timeout(start, tout)) {
			debug("Timeout waiting sgdma in do sync!\n");
			break;
		}
	}

	/*
	 * Clear any (previous) status register information
	 * that might occlude our error checking later.
	 */
	writel(0xFF, &dev->status);

	/* Point the controller at the descriptor */
	writel((uint32_t)desc, &dev->next_descriptor_pointer);
	debug("next desc in sgdma 0x%x\n", (uint32_t)dev->next_descriptor_pointer);

	/*
	 * Set up SGDMA controller to:
	 * - Disable interrupt generation
	 * - Run once a valid descriptor is written to controller
	 * - Stop on an error with any particular descriptor
	 */
	writel(ALT_SGDMA_CONTROL_RUN_MSK | ALT_SGDMA_CONTROL_STOP_DMA_ER_MSK,
		&dev->control);

	/* Wait for the descriptor (chain) to complete */
	debug("wait for sgdma....");
	start = get_time_ns();

	while (readl(&dev->status) & ALT_SGDMA_STATUS_BUSY_MSK) {
		if (is_timeout(start, tout)) {
			debug("Timeout waiting sgdma in do sync!\n");
			break;
		}
	}

	debug("done\n");

	/* Clear Run */
	temp = readl(&dev->control);
	writel(temp & ~ALT_SGDMA_CONTROL_RUN_MSK, &dev->control);

	/* Get & clear status register contents */
	debug("tx sgdma status = 0x%x", readl(&dev->status));
	writel(0xFF, &dev->status);

	return 0;
}

static int alt_sgdma_do_async_transfer(struct alt_sgdma_registers *dev,
					struct alt_sgdma_descriptor *desc)
{
	uint64_t start;
	uint64_t tout;

	/* Wait for any pending transfers to complete */
	tout = ALT_TSE_SGDMA_BUSY_WATCHDOG_TOUT * MSECOND;

	start = get_time_ns();

	while (readl(&dev->status) & ALT_SGDMA_STATUS_BUSY_MSK) {
		if (is_timeout(start, tout)) {
			debug("Timeout waiting sgdma in do async!\n");
			break;
		}
	}

	/*
	 * Clear any (previous) status register information
	 * that might occlude our error checking later.
	 */
	writel(0xFF, &dev->status);

	/* Point the controller at the descriptor */
	writel((uint32_t)desc, &dev->next_descriptor_pointer);

	/*
	 * Set up SGDMA controller to:
	 * - Disable interrupt generation
	 * - Run once a valid descriptor is written to controller
	 * - Stop on an error with any particular descriptor
	 */
	writel(ALT_SGDMA_CONTROL_RUN_MSK | ALT_SGDMA_CONTROL_STOP_DMA_ER_MSK,
		&dev->control);

	return 0;
}

static int tse_get_ethaddr(struct eth_device *edev, unsigned char *m)
{
	struct altera_tse_priv *priv = edev->priv;
	struct alt_tse_mac *mac_dev = priv->tse_regs;

	m[5] = (readl(&mac_dev->mac_addr_1) >> 8)  && 0xFF;
	m[4] = (readl(&mac_dev->mac_addr_1))       && 0xFF;
	m[3] = (readl(&mac_dev->mac_addr_0) >> 24) && 0xFF;
	m[2] = (readl(&mac_dev->mac_addr_0) >> 16) && 0xFF;
	m[1] = (readl(&mac_dev->mac_addr_0) >>  8) && 0xFF;
	m[0] = (readl(&mac_dev->mac_addr_0))       && 0xFF;

	return 0;
}

static int tse_set_ethaddr(struct eth_device *edev, const unsigned char *m)
{
	struct altera_tse_priv *priv = edev->priv;
	struct alt_tse_mac *mac_dev = priv->tse_regs;

	debug("Setting MAC address to %02x:%02x:%02x:%02x:%02x:%02x\n",
		m[0], m[1], m[2], m[3], m[4], m[5]);

	writel(m[3] << 24 | m[2] << 16 | m[1] << 8 | m[0], &mac_dev->mac_addr_0);
	writel((m[5] << 8 | m[4]) & 0xFFFF, &mac_dev->mac_addr_1);

	return 0;
}

static int tse_phy_read(struct mii_bus *bus, int phy_addr, int reg)
{
	struct altera_tse_priv *priv = bus->priv;
	struct alt_tse_mac *mac_dev = priv->tse_regs;
	uint32_t *mdio_regs;

	writel(phy_addr, &mac_dev->mdio_phy1_addr);

	mdio_regs = (uint32_t *)&mac_dev->mdio_phy1;

	return readl(&mdio_regs[reg]) & 0xFFFF;
}

static int tse_phy_write(struct mii_bus *bus, int phy_addr, int reg, u16 val)
{
	struct altera_tse_priv *priv = bus->priv;
	struct alt_tse_mac *mac_dev = priv->tse_regs;
	uint32_t *mdio_regs;

	writel(phy_addr, &mac_dev->mdio_phy1_addr);

	mdio_regs = (uint32_t *)&mac_dev->mdio_phy1;

	writel((uint32_t)val, &mdio_regs[reg]);

	return 0;
}

static void tse_reset(struct eth_device *edev)
{
	/* stop sgdmas, disable tse receive */
	struct altera_tse_priv *priv = edev->priv;
	struct alt_tse_mac *mac_dev = priv->tse_regs;
	struct alt_sgdma_registers *rx_sgdma = priv->sgdma_rx_regs;
	struct alt_sgdma_registers *tx_sgdma = priv->sgdma_tx_regs;
	struct alt_sgdma_descriptor *rx_desc = priv->rx_desc;
	struct alt_sgdma_descriptor *tx_desc = priv->tx_desc;
	uint64_t start;
	uint64_t tout;

	tout = ALT_TSE_SGDMA_BUSY_WATCHDOG_TOUT * MSECOND;

	/* clear rx desc & wait for sgdma to complete */
	writeb(0, &rx_desc->descriptor_control);
	writel(0, &rx_sgdma->control);

	writel(ALT_SGDMA_CONTROL_SOFTWARERESET_MSK, &rx_sgdma->control);
	writel(ALT_SGDMA_CONTROL_SOFTWARERESET_MSK, &rx_sgdma->control);
	mdelay(100);

	start = get_time_ns();

	while (readl(&rx_sgdma->status) & ALT_SGDMA_STATUS_BUSY_MSK) {
		if (is_timeout(start, tout)) {
			printf("Timeout waiting for rx sgdma!\n");
			writel(ALT_SGDMA_CONTROL_SOFTWARERESET_MSK, &rx_sgdma->control);
			writel(ALT_SGDMA_CONTROL_SOFTWARERESET_MSK, &rx_sgdma->control);
			break;
		}
	}

	/* clear tx desc & wait for sgdma to complete */
	writeb(0, &tx_desc->descriptor_control);
	writel(0, &tx_sgdma->control);

	writel(ALT_SGDMA_CONTROL_SOFTWARERESET_MSK, &tx_sgdma->control);
	writel(ALT_SGDMA_CONTROL_SOFTWARERESET_MSK, &tx_sgdma->control);
	mdelay(100);

	start = get_time_ns();

	while (readl(&tx_sgdma->status) & ALT_SGDMA_STATUS_BUSY_MSK) {
		if (is_timeout(start, tout)) {
			printf("Timeout waiting for tx sgdma!\n");
			writel(ALT_SGDMA_CONTROL_SOFTWARERESET_MSK, &tx_sgdma->control);
			writel(ALT_SGDMA_CONTROL_SOFTWARERESET_MSK, &tx_sgdma->control);
			break;
		}
	}

	/* reset the mac */
	writel(ALTERA_TSE_CMD_TX_ENA_MSK | ALTERA_TSE_CMD_RX_ENA_MSK |
		ALTERA_TSE_CMD_SW_RESET_MSK, &mac_dev->command_config);

	start = get_time_ns();
	tout = ALT_TSE_SW_RESET_WATCHDOG_TOUT * MSECOND;

	while (readl(&mac_dev->command_config) & ALTERA_TSE_CMD_SW_RESET_MSK) {
		if (is_timeout(start, tout)) {
			printf("TSEMAC SW reset bit never cleared!\n");
			break;
		}
	}
}

static int tse_eth_open(struct eth_device *edev)
{
	struct altera_tse_priv *priv = edev->priv;
	int ret;

	ret = phy_device_connect(edev, priv->miibus, priv->phy_addr, NULL, 0,
				 PHY_INTERFACE_MODE_NA);
	if (ret)
		return ret;

	return 0;
}

static int tse_eth_send(struct eth_device *edev, void *packet, int length)
{

	struct altera_tse_priv *priv = edev->priv;
	struct alt_sgdma_registers *tx_sgdma = priv->sgdma_tx_regs;
	struct alt_sgdma_descriptor *tx_desc = priv->tx_desc;
	struct alt_sgdma_descriptor *tx_desc_cur = tx_desc;

	flush_dcache_range((uint32_t)packet, (uint32_t)packet + length);
	alt_sgdma_construct_descriptor_burst(
		(struct alt_sgdma_descriptor *)&tx_desc[0],
		(struct alt_sgdma_descriptor *)&tx_desc[1],
		(uint32_t *)packet,	/* read addr */
		(uint32_t *)0,		/*           */
		length,			/* length or EOP ,will change for each tx */
		0x1,			/* gen eop */
		0x0,			/* read fixed */
		0x1,			/* write fixed or sop */
		0x0,			/* read burst */
		0x0,			/* write burst */
		0x0			/* channel */
		);

	alt_sgdma_do_sync_transfer(tx_sgdma, tx_desc_cur);

	return 0;
}

static void tse_eth_halt(struct eth_device *edev)
{
	struct altera_tse_priv *priv = edev->priv;
	struct alt_sgdma_registers *rx_sgdma = priv->sgdma_rx_regs;
	struct alt_sgdma_registers *tx_sgdma = priv->sgdma_tx_regs;

	writel(0, &rx_sgdma->control); /* Stop the controller and reset settings */
	writel(0, &tx_sgdma->control); /* Stop the controller and reset settings */
}

static int tse_eth_rx(struct eth_device *edev)
{
	uint16_t packet_length = 0;

	struct altera_tse_priv *priv = edev->priv;
	struct alt_sgdma_descriptor *rx_desc = priv->rx_desc;
	struct alt_sgdma_descriptor *rx_desc_cur = rx_desc;
	struct alt_sgdma_registers *rx_sgdma = priv->sgdma_rx_regs;

	if (rx_desc_cur->descriptor_status &
		ALT_SGDMA_DESCRIPTOR_STATUS_TERMINATED_BY_EOP_MSK) {

		packet_length = rx_desc->actual_bytes_transferred;
		net_receive(edev, NetRxPackets[0], packet_length);

		/* Clear Run */
		rx_sgdma->control = (rx_sgdma->control & (~ALT_SGDMA_CONTROL_RUN_MSK));

		/* start descriptor again */
		flush_dcache_range((uint32_t)(NetRxPackets[0]), (uint32_t)(NetRxPackets[0]) + PKTSIZE);
		alt_sgdma_construct_descriptor_burst(
			(struct alt_sgdma_descriptor *)&rx_desc[0],
			(struct alt_sgdma_descriptor *)&rx_desc[1],
			(uint32_t)0x0,			/* read addr */
			(uint32_t *)NetRxPackets[0],	/*           */
			0x0,				/* length or EOP */
			0x0,				/* gen eop */
			0x0,				/* read fixed */
			0x0,				/* write fixed or sop */
			0x0,				/* read burst */
			0x0,				/* write burst */
			0x0				/* channel */
		);

		/* setup the sgdma */
		alt_sgdma_do_async_transfer(priv->sgdma_rx_regs, rx_desc);
	}

	return 0;
}

static int tse_init_dev(struct eth_device *edev)
{
	struct altera_tse_priv *priv = edev->priv;
	struct alt_tse_mac *mac_dev = priv->tse_regs;
	struct alt_sgdma_descriptor *tx_desc = priv->tx_desc;
	struct alt_sgdma_descriptor *rx_desc = priv->rx_desc;
	struct alt_sgdma_descriptor *rx_desc_cur;

	rx_desc_cur = rx_desc;

	tse_reset(edev);

	/* need to create sgdma */
	alt_sgdma_construct_descriptor_burst(
		(struct alt_sgdma_descriptor *)&tx_desc[0],
		(struct alt_sgdma_descriptor *)&tx_desc[1],
		(uint32_t *)NULL,	/* read addr */
		(uint32_t *)0,		/*           */
		0,			/* length or EOP ,will change for each tx */
		0x1,			/* gen eop */
		0x0,			/* read fixed */
		0x1,			/* write fixed or sop */
		0x0,			/* read burst */
		0x0,			/* write burst */
		0x0			/* channel */
		);

	flush_dcache_range((uint32_t)(NetRxPackets[0]), (uint32_t)(NetRxPackets[0]) + PKTSIZE);
	alt_sgdma_construct_descriptor_burst(
		(struct alt_sgdma_descriptor *)&rx_desc[0],
		(struct alt_sgdma_descriptor *)&rx_desc[1],
		(uint32_t)0x0,			/* read addr */
		(uint32_t *)NetRxPackets[0],	/*           */
		0x0,				/* length or EOP */
		0x0,				/* gen eop */
		0x0,				/* read fixed */
		0x0,				/* write fixed or sop */
		0x0,				/* read burst */
		0x0,				/* write burst */
		0x0				/* channel */
		);

	/* start rx async transfer */
	alt_sgdma_do_async_transfer(priv->sgdma_rx_regs, rx_desc_cur);

	/* Initialize MAC registers */
	writel(PKTSIZE, &mac_dev->max_frame_length);

	/* NO Shift */
	writel(0, &mac_dev->rx_cmd_stat);
	writel(0, &mac_dev->tx_cmd_stat);

	/* enable MAC */
	writel(ALTERA_TSE_CMD_TX_ENA_MSK | ALTERA_TSE_CMD_RX_ENA_MSK, &mac_dev->command_config);

	return 0;
}

static int tse_probe(struct device_d *dev)
{
	struct resource *iores;
	struct altera_tse_priv *priv;
	struct mii_bus *miibus;
	struct eth_device *edev;
	struct alt_sgdma_descriptor *rx_desc;
	struct alt_sgdma_descriptor *tx_desc;
#ifndef CONFIG_TSE_USE_DEDICATED_DESC_MEM
	uint32_t dma_handle;
#endif
	edev = xzalloc(sizeof(struct eth_device));
	priv = xzalloc(sizeof(struct altera_tse_priv));
	miibus = xzalloc(sizeof(struct mii_bus));

	edev->priv = priv;

	edev->init = tse_init_dev;
	edev->open = tse_eth_open;
	edev->send = tse_eth_send;
	edev->recv = tse_eth_rx;
	edev->halt = tse_eth_halt;
	edev->get_ethaddr = tse_get_ethaddr;
	edev->set_ethaddr = tse_set_ethaddr;
	edev->parent = dev;

#ifdef CONFIG_TSE_USE_DEDICATED_DESC_MEM
	iores = dev_request_mem_resource(dev, 3);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	tx_desc = IOMEM(iores->start);
	rx_desc = tx_desc + 2;
#else
	tx_desc = dma_alloc_coherent(sizeof(*tx_desc) * (3 + PKTBUFSRX), (dma_addr_t *)&dma_handle);
	rx_desc = tx_desc + 2;

	if (!tx_desc) {
		free(edev);
		free(miibus);
		return 0;
	}
#endif

	memset(rx_desc, 0, (sizeof *rx_desc) * (PKTBUFSRX + 1));
	memset(tx_desc, 0, (sizeof *tx_desc) * 2);

	iores = dev_request_mem_resource(dev, 0);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	priv->tse_regs = IOMEM(iores->start);
	iores = dev_request_mem_resource(dev, 1);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	priv->sgdma_rx_regs = IOMEM(iores->start);

	iores = dev_request_mem_resource(dev, 2);
	if (IS_ERR(iores))
		return PTR_ERR(iores);
	priv->sgdma_tx_regs = IOMEM(iores->start);
	priv->rx_desc = rx_desc;
	priv->tx_desc = tx_desc;

	priv->miibus = miibus;

	miibus->read = tse_phy_read;
	miibus->write = tse_phy_write;
	miibus->priv = priv;
	miibus->parent = dev;

	if (dev->platform_data != NULL)
		priv->phy_addr = *((int8_t *)(dev->platform_data));
	else
		priv->phy_addr = -1;

	mdiobus_register(miibus);

	return eth_register(edev);
}

static struct driver_d altera_tse_driver = {
	.name = "altera_tse",
	.probe = tse_probe,
};
device_platform_driver(altera_tse_driver);
