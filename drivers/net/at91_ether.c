// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2009-2012 Jean-Christophe PLAGNIOL-VILLARD <plagnio@jcrosoft.com>
 *
 * (C) Copyright 2003
 * Author : Hamid Ikdoumi (Atmel)
 */

#include <common.h>
#include <dma.h>
#include <net.h>
#include <clock.h>
#include <malloc.h>
#include <driver.h>
#include <xfuncs.h>
#include <init.h>
#include <asm/io.h>
#include <mach/hardware.h>
#include <mach/at91rm9200_emac.h>
#include <mach/board.h>
#include <generated/mach-types.h>
#include <linux/clk.h>
#include <linux/mii.h>
#include <errno.h>
#include <linux/phy.h>

#include "at91_ether.h"

struct ether_device {
	struct eth_device netdev;
	struct mii_bus miibus;
	struct rbf_t *rbfp;
	struct rbf_t *rbfdt;
	unsigned char *rbf_framebuf;
	int phy_addr;
	phy_interface_t interface;
};
#define to_ether(_nd) container_of(_nd, struct ether_device, netdev)

/*
 * Enable the MDIO bit in MAC control register
 * When not called from an interrupt-handler, access to the PHY must be
 *  protected by a spinlock.
 */
static void enable_mdi(void)
{
	unsigned long ctl;

	ctl = at91_emac_read(AT91_EMAC_CTL);
	at91_emac_write(AT91_EMAC_CTL, ctl | AT91_EMAC_MPE);	/* enable management port */
}

/*
 * Disable the MDIO bit in the MAC control register
 */
static void disable_mdi(void)
{
	unsigned long ctl;

	ctl = at91_emac_read(AT91_EMAC_CTL);
	at91_emac_write(AT91_EMAC_CTL, ctl & ~AT91_EMAC_MPE);	/* disable management port */
}

/*
 * Wait until the PHY operation is complete.
 */
static inline int at91_phy_wait(void)
{
	uint64_t start;

	start = get_time_ns();

	do {
		if (is_timeout(start, 2 * MSECOND)) {
			puts("at91_ether: MIO timeout\n");
			return -1;
		}
	} while (!(at91_emac_read(AT91_EMAC_SR) & AT91_EMAC_SR_IDLE));

	return 0;
}

static int at91_ether_mii_read(struct mii_bus *dev, int addr, int reg)
{
	int value;

	enable_mdi();

	at91_emac_write(AT91_EMAC_MAN, AT91_EMAC_MAN_802_3 | AT91_EMAC_RW_R
		| ((addr & 0x1f) << 23) | (reg << 18));

	/* Wait until IDLE bit in Network Status register is cleared */
	value = at91_phy_wait();
	if (value < 0)
		goto out;

	value = at91_emac_read(AT91_EMAC_MAN) & AT91_EMAC_DATA;

out:
	disable_mdi();

	return value;
}

static int at91_ether_mii_write(struct mii_bus *dev, int addr, int reg, u16 val)
{
	int ret;

	enable_mdi();
	at91_emac_write(AT91_EMAC_MAN, AT91_EMAC_MAN_802_3 | AT91_EMAC_RW_W
		| ((addr & 0x1f) << 23) | (reg << 18) | (val & AT91_EMAC_DATA));

	/* Wait until IDLE bit in Network Status register is cleared */
	ret = at91_phy_wait();

	disable_mdi();

	return ret;
}

static void update_linkspeed(struct eth_device *edev)
{
	unsigned int mac_cfg;

	/* Update the MAC */
	mac_cfg = at91_emac_read(AT91_EMAC_CFG) & ~(AT91_EMAC_SPD | AT91_EMAC_FD);
	if (edev->phydev->speed == SPEED_100) {
		if (edev->phydev->duplex)
			mac_cfg |= AT91_EMAC_SPD | AT91_EMAC_FD;
		else					/* 100 Half Duplex */
			mac_cfg |= AT91_EMAC_SPD;
	} else {
		if (edev->phydev->duplex)
			mac_cfg |= AT91_EMAC_FD;
		else {}					/* 10 Half Duplex */
	}
	at91_emac_write(AT91_EMAC_CFG, mac_cfg);
}

static int at91_ether_open(struct eth_device *edev)
{
	int i;
	unsigned long ctl;
	struct ether_device *etdev = to_ether(edev);
	unsigned char *rbf_framebuf = etdev->rbf_framebuf;
	int ret;

	ret = phy_device_connect(edev, &etdev->miibus, etdev->phy_addr,
				 update_linkspeed, 0, etdev->interface);
	if (ret)
		return ret;

	/* Clear internal statistics */
	ctl = at91_emac_read(AT91_EMAC_CTL);
	at91_emac_write(AT91_EMAC_CTL, ctl | AT91_EMAC_CSR);

	/* Init Ethernet buffers */
	etdev->rbfp = etdev->rbfdt;
	for (i = 0; i < MAX_RX_DESCR; i++) {
		etdev->rbfp[i].addr = (unsigned long)rbf_framebuf;
		etdev->rbfp[i].size = 0;
		rbf_framebuf += MAX_RBUFF_SZ;
	}
	etdev->rbfp[i - 1].addr |= RBF_WRAP;

	/* Program address of descriptor list in Rx Buffer Queue register */
	at91_emac_write(AT91_EMAC_RBQP, (unsigned long) etdev->rbfdt);

	ctl = at91_emac_read(AT91_EMAC_RSR);
	ctl &= ~(AT91_EMAC_RSR_OVR | AT91_EMAC_RSR_REC | AT91_EMAC_RSR_BNA);
	at91_emac_write(AT91_EMAC_RSR, ctl);

	ctl = at91_emac_read(AT91_EMAC_CFG);
	ctl |= AT91_EMAC_CAF | AT91_EMAC_NBC;
	at91_emac_write(AT91_EMAC_CFG, ctl);

	/* Enable Receive and Transmit */
	ctl = at91_emac_read(AT91_EMAC_CTL);
	ctl |= AT91_EMAC_RE | AT91_EMAC_TE;
	at91_emac_write(AT91_EMAC_CTL, ctl);

	return 0;
}

static int at91_ether_send(struct eth_device *edev, void *packet, int length)
{
	while (!(at91_emac_read(AT91_EMAC_TSR) & AT91_EMAC_TSR_BNQ));

	dma_sync_single_for_device((unsigned long)packet, length, DMA_TO_DEVICE);

	/* Set address of the data in the Transmit Address register */
	at91_emac_write(AT91_EMAC_TAR, (unsigned long) packet);
	/* Set length of the packet in the Transmit Control register */
	at91_emac_write(AT91_EMAC_TCR, length);

	while (at91_emac_read(AT91_EMAC_TCR) & 0x7ff);

	at91_emac_write(AT91_EMAC_TSR,
		at91_emac_read(AT91_EMAC_TSR) | AT91_EMAC_TSR_COMP);

	dma_sync_single_for_cpu((unsigned long)packet, length, DMA_TO_DEVICE);

	return 0;
}

static int at91_ether_rx(struct eth_device *edev)
{
	struct ether_device *etdev = to_ether(edev);
	int size;
	struct rbf_t *rbfp = etdev->rbfp;

	if (!(rbfp->addr & RBF_OWNER))
		return 0;

	size = rbfp->size & RBF_SIZE;

	dma_sync_single_for_cpu((unsigned long)rbfp->addr, size,
				DMA_FROM_DEVICE);
	net_receive(edev, (unsigned char *)(rbfp->addr & RBF_ADDR), size);
	dma_sync_single_for_device((unsigned long)rbfp->addr, size,
				   DMA_FROM_DEVICE);

	rbfp->addr &= ~RBF_OWNER;
	if (rbfp->addr & RBF_WRAP)
		etdev->rbfp = etdev->rbfdt;
	else
		etdev->rbfp++;

	at91_emac_write(AT91_EMAC_RSR,
		at91_emac_read(AT91_EMAC_RSR) | AT91_EMAC_RSR_REC);

	return size;
}

static void at91_ether_halt (struct eth_device *edev)
{
	unsigned long ctl;

	/* Disable Receiver and Transmitter */
	ctl = at91_emac_read(AT91_EMAC_CTL);
	ctl &= ~(AT91_EMAC_TE | AT91_EMAC_RE);
	at91_emac_write(AT91_EMAC_CTL, ctl);
}

static int at91_ether_get_ethaddr(struct eth_device *eth, unsigned char *adr)
{
	/* We have no eeprom */
	return -1;
}

static int at91_ether_set_ethaddr(struct eth_device *eth, const unsigned char *adr)
{
	int i;

	/* The CSB337 originally used a version of the MicroMonitor bootloader
	 * which saved Ethernet addresses in the "wrong" order.  Operating
	 * systems (like Linux) know this, and apply a workaround.  Replicate
	 * that MicroMonitor behavior so we avoid needing to make such OS code
	 * care about which bootloader was used.
	 */
	if (machine_is_csb337()) {
		at91_emac_write(AT91_EMAC_SA2H,
				   (adr[0] <<  8) | (adr[1]));
		at91_emac_write(AT91_EMAC_SA2L,
				   (adr[2] << 24) | (adr[3] << 16)
				 | (adr[4] <<  8) | (adr[5]));
	} else {
		at91_emac_write(AT91_EMAC_SA2L,
				   (adr[3] << 24) | (adr[2] << 16)
				 | (adr[1] <<  8) | (adr[0]));
		at91_emac_write(AT91_EMAC_SA2H,
				   (adr[5] <<  8) | (adr[4]));
	}

	for (i = 0; i < 5; i++)
		debug ("%02x:", adr[i]);
	debug ("%02x\n", adr[5]);

	return 0;
}

static int at91_ether_init(struct eth_device *edev)
{
	return 0;
}

static int at91_ether_probe(struct device_d *dev)
{
	unsigned int mac_cfg;
	struct ether_device *ether_dev;
	struct eth_device *edev;
	struct mii_bus *miibus;
	unsigned long ether_hz;
	struct clk *pclk;
	struct macb_platform_data *pdata;

	if (!dev->platform_data) {
		printf("at91_ether: no platform_data\n");
		return -ENODEV;
	}

	pdata = dev->platform_data;

	ether_dev = xzalloc(sizeof(struct ether_device));

	edev = &ether_dev->netdev;
	miibus = &ether_dev->miibus;
	edev->priv = ether_dev;

	edev->init = at91_ether_init;
	edev->open = at91_ether_open;
	edev->send = at91_ether_send;
	edev->recv = at91_ether_rx;
	edev->halt = at91_ether_halt;
	edev->get_ethaddr = at91_ether_get_ethaddr;
	edev->set_ethaddr = at91_ether_set_ethaddr;
	ether_dev->rbf_framebuf = dma_alloc_coherent(MAX_RX_DESCR * MAX_RBUFF_SZ,
						     DMA_ADDRESS_BROKEN);
	ether_dev->rbfdt = dma_alloc_coherent(sizeof(struct rbf_t) * MAX_RX_DESCR,
					      DMA_ADDRESS_BROKEN);

	ether_dev->phy_addr = pdata->phy_addr;
	miibus->read = at91_ether_mii_read;
	miibus->write = at91_ether_mii_write;

	/* Sanitize the clocks */
	mac_cfg = at91_emac_read(AT91_EMAC_CFG);

	pclk = clk_get(dev, "ether_clk");
	clk_enable(pclk);
	ether_hz = clk_get_rate(pclk);
	if (ether_hz > 40000000) {
		/* MDIO clock must not exceed 2.5 MHz, so enable MCK divider */
		mac_cfg |= AT91_EMAC_CLK_DIV64;
	} else {
		mac_cfg &= ~AT91_EMAC_CLK;
	}

	mac_cfg |= AT91_EMAC_CLK_DIV32 | AT91_EMAC_BIG;

	if (pdata->phy_interface == PHY_INTERFACE_MODE_RMII) {
		ether_dev->interface = PHY_INTERFACE_MODE_RGMII;
		mac_cfg |= AT91_EMAC_RMII;
	} else {
		ether_dev->interface = PHY_INTERFACE_MODE_MII;
	}

	at91_emac_write(AT91_EMAC_CFG, mac_cfg);

	mdiobus_register(miibus);
	eth_register(edev);

	return 0;
}

static struct driver_d at91_ether_driver = {
	.name = "at91_ether",
	.probe = at91_ether_probe,
};
device_platform_driver(at91_ether_driver);
