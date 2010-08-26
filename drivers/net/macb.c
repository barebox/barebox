/*
 * Copyright (C) 2005-2006 Atmel Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <common.h>

/*
 * The barebox networking stack is a little weird.  It seems like the
 * networking core allocates receive buffers up front without any
 * regard to the hardware that's supposed to actually receive those
 * packets.
 *
 * The MACB receives packets into 128-byte receive buffers, so the
 * buffers allocated by the core isn't very practical to use.  We'll
 * allocate our own, but we need one such buffer in case a packet
 * wraps around the DMA ring so that we have to copy it.
 *
 * Therefore, define CFG_RX_ETH_BUFFER to 1 in the board-specific
 * configuration header.  This way, the core allocates one RX buffer
 * and one TX buffer, each of which can hold a ethernet packet of
 * maximum size.
 *
 * For some reason, the networking core unconditionally specifies a
 * 32-byte packet "alignment" (which really should be called
 * "padding").  MACB shouldn't need that, but we'll refrain from any
 * core modifications here...
 */

#include <net.h>
#include <clock.h>
#include <malloc.h>
#include <xfuncs.h>
#include <init.h>
#include <miidev.h>
#include <errno.h>
#include <asm/io.h>
#include <mach/board.h>
#include <linux/clk.h>

#include "macb.h"

#define CFG_MACB_RX_BUFFER_SIZE		4096
#define CFG_MACB_RX_RING_SIZE		(CFG_MACB_RX_BUFFER_SIZE / 128)
#define CFG_MACB_TX_TIMEOUT		1000
#define CFG_MACB_AUTONEG_TIMEOUT	5000000

struct macb_dma_desc {
	u32	addr;
	u32	ctrl;
};

#define RXADDR_USED		0x00000001
#define RXADDR_WRAP		0x00000002

#define RXBUF_FRMLEN_MASK	0x00000fff
#define RXBUF_FRAME_START	0x00004000
#define RXBUF_FRAME_END		0x00008000
#define RXBUF_TYPEID_MATCH	0x00400000
#define RXBUF_ADDR4_MATCH	0x00800000
#define RXBUF_ADDR3_MATCH	0x01000000
#define RXBUF_ADDR2_MATCH	0x02000000
#define RXBUF_ADDR1_MATCH	0x04000000
#define RXBUF_BROADCAST		0x80000000

#define TXBUF_FRMLEN_MASK	0x000007ff
#define TXBUF_FRAME_END		0x00008000
#define TXBUF_NOCRC		0x00010000
#define TXBUF_EXHAUSTED		0x08000000
#define TXBUF_UNDERRUN		0x10000000
#define TXBUF_MAXRETRY		0x20000000
#define TXBUF_WRAP		0x40000000
#define TXBUF_USED		0x80000000

struct macb_device {
	void			*regs;

	unsigned int		rx_tail;
	unsigned int		tx_tail;

	void			*rx_buffer;
	void			*tx_buffer;
	struct macb_dma_desc	*rx_ring;
	struct macb_dma_desc	*tx_ring;

	const struct device	*dev;
	struct eth_device	netdev;

	struct mii_device	miidev;

	unsigned int		flags;
};

static int macb_send(struct eth_device *edev, void *packet,
		     int length)
{
	struct macb_device *macb = edev->priv;
	unsigned long ctrl;

	debug("%s\n", __func__);

	ctrl = length & TXBUF_FRMLEN_MASK;
	ctrl |= TXBUF_FRAME_END | TXBUF_WRAP;

	macb->tx_ring[0].ctrl = ctrl;
	macb->tx_ring[0].addr = (ulong)packet;
	barrier();
	writel(MACB_BIT(TE) | MACB_BIT(RE) | MACB_BIT(TSTART), macb->regs + MACB_NCR);

	if (ctrl & TXBUF_UNDERRUN)
		printf("TX underrun\n");
	if (ctrl & TXBUF_EXHAUSTED)
		printf("TX buffers exhausted in mid frame\n");

	/* No one cares anyway */
	return 0;
}

static void reclaim_rx_buffers(struct macb_device *macb,
			       unsigned int new_tail)
{
	unsigned int i;

	debug("%s\n", __func__);

	i = macb->rx_tail;
	while (i > new_tail) {
		macb->rx_ring[i].addr &= ~RXADDR_USED;
		i++;
		if (i > CFG_MACB_RX_RING_SIZE)
			i = 0;
	}

	while (i < new_tail) {
		macb->rx_ring[i].addr &= ~RXADDR_USED;
		i++;
	}

	barrier();
	macb->rx_tail = new_tail;
}

static int macb_recv(struct eth_device *edev)
{
	struct macb_device *macb = edev->priv;
	unsigned int rx_tail = macb->rx_tail;
	void *buffer;
	int length;
	int wrapped = 0;
	u32 status;

//	printf("%s\n", __func__);

	for (;;) {
		if (!(macb->rx_ring[rx_tail].addr & RXADDR_USED))
			return -1;

		status = macb->rx_ring[rx_tail].ctrl;
		if (status & RXBUF_FRAME_START) {
			if (rx_tail != macb->rx_tail)
				reclaim_rx_buffers(macb, rx_tail);
			wrapped = 0;
		}

		if (status & RXBUF_FRAME_END) {
			buffer = macb->rx_buffer + 128 * macb->rx_tail;
			length = status & RXBUF_FRMLEN_MASK;
			if (wrapped) {
				unsigned int headlen, taillen;

				headlen = 128 * (CFG_MACB_RX_RING_SIZE
						 - macb->rx_tail);
				taillen = length - headlen;
				memcpy((void *)NetRxPackets[0],
				       buffer, headlen);
				memcpy((void *)NetRxPackets[0] + headlen,
				       macb->rx_buffer, taillen);
				buffer = (void *)NetRxPackets[0];
			}

			net_receive(buffer, length);
			if (++rx_tail >= CFG_MACB_RX_RING_SIZE)
				rx_tail = 0;
			reclaim_rx_buffers(macb, rx_tail);
		} else {
			if (++rx_tail >= CFG_MACB_RX_RING_SIZE) {
				wrapped = 1;
				rx_tail = 0;
			}
		}
		barrier();
	}

	return 0;
}

static int macb_open(struct eth_device *edev)
{
	struct macb_device *macb = edev->priv;
	int duplex = 1, speed = 1;
	u32 ncfgr;

	debug("%s\n", __func__);

	miidev_wait_aneg(&macb->miidev);
	miidev_print_status(&macb->miidev);

	ncfgr = readl(macb->regs + MACB_NCFGR);
	ncfgr &= ~(MACB_BIT(SPD) | MACB_BIT(FD));
	if (speed)
		ncfgr |= MACB_BIT(SPD);
	if (duplex)
		ncfgr |= MACB_BIT(FD);
	writel(ncfgr, macb->regs + MACB_NCFGR);

	return 0;
}

static int macb_init(struct eth_device *edev)
{
	struct macb_device *macb = edev->priv;
	unsigned long paddr, val = 0;
	int i;

	debug("%s\n", __func__);

	/*
	 * macb_halt should have been called at some point before now,
	 * so we'll assume the controller is idle.
	 */

	/* initialize DMA descriptors */
	paddr = (ulong)macb->rx_buffer;
	for (i = 0; i < CFG_MACB_RX_RING_SIZE; i++) {
		if (i == (CFG_MACB_RX_RING_SIZE - 1))
			paddr |= RXADDR_WRAP;
		macb->rx_ring[i].addr = paddr;
		macb->rx_ring[i].ctrl = 0;
		paddr += 128;
	}

	macb->tx_ring[0].addr = 0;
	macb->tx_ring[0].ctrl = TXBUF_USED | TXBUF_WRAP;

	macb->rx_tail = macb->tx_tail = 0;

	writel((ulong)macb->rx_ring, macb->regs + MACB_RBQP);
	writel((ulong)macb->tx_ring, macb->regs + MACB_TBQP);

	if (macb->flags & AT91SAM_ETHER_RMII)
		val |= MACB_BIT(RMII);
	else
		val &= ~MACB_BIT(RMII);

#if defined(CONFIG_ARCH_AT91)
	val |= MACB_BIT(CLKEN);
#endif
	writel(val, macb->regs + MACB_USRIO);

	/* Enable TX and RX */
	writel(MACB_BIT(TE) | MACB_BIT(RE), macb->regs + MACB_NCR);

	return 0;
}

static void macb_halt(struct eth_device *edev)
{
	struct macb_device *macb = edev->priv;
	u32 ncr, tsr;

	/* Halt the controller and wait for any ongoing transmission to end. */
	ncr = readl(macb->regs + MACB_NCR);
	ncr |= MACB_BIT(THALT);
	writel(ncr, macb->regs + MACB_NCR);

	do {
		tsr = readl(macb->regs + MACB_TSR);
	} while (tsr & MACB_BIT(TGO));

	/* Disable TX and RX, and clear statistics */
	writel(MACB_BIT(CLRSTAT), macb->regs + MACB_NCR);
}

static int macb_phy_read(struct mii_device *mdev, int addr, int reg)
{
	struct eth_device *edev = mdev->edev;
	struct macb_device *macb = edev->priv;

	unsigned long netctl;
	unsigned long netstat;
	unsigned long frame;
	int iflag;
	int value;
	uint64_t start;

	debug("%s\n", __func__);

	iflag = disable_interrupts();
	netctl = readl(macb->regs + MACB_NCR);
	netctl |= MACB_BIT(MPE);
	writel(netctl, macb->regs + MACB_NCR);
	if (iflag)
		enable_interrupts();

	frame = (MACB_BF(SOF, 1)
		 | MACB_BF(RW, 2)
		 | MACB_BF(PHYA, addr)
		 | MACB_BF(REGA, reg)
		 | MACB_BF(CODE, 2));
	writel(frame, macb->regs + MACB_MAN);

	start = get_time_ns();
	do {
		netstat = readl(macb->regs + MACB_NSR);
		if (is_timeout(start, SECOND)) {
			printf("phy read timed out\n");
			return -1;
		}
	} while (!(netstat & MACB_BIT(IDLE)));

	frame = readl(macb->regs + MACB_MAN);
	value = MACB_BFEXT(DATA, frame);

	iflag = disable_interrupts();
	netctl = readl(macb->regs + MACB_NCR);
	netctl &= ~MACB_BIT(MPE);
	writel(netctl, macb->regs + MACB_NCR);
	if (iflag)
		enable_interrupts();

	return value;
}

static int macb_phy_write(struct mii_device *mdev, int addr, int reg, int value)
{
	struct eth_device *edev = mdev->edev;
	struct macb_device *macb = edev->priv;
	unsigned long netctl;
	unsigned long netstat;
	unsigned long frame;
	int iflag;

	debug("%s\n", __func__);

	iflag = disable_interrupts();
	netctl = readl(macb->regs + MACB_NCR);
	netctl |= MACB_BIT(MPE);
	writel(netctl, macb->regs + MACB_NCR);
	if (iflag)
		enable_interrupts();

	frame = (MACB_BF(SOF, 1)
		 | MACB_BF(RW, 1)
		 | MACB_BF(PHYA, addr)
		 | MACB_BF(REGA, reg)
		 | MACB_BF(CODE, 2)
		 | MACB_BF(DATA, value));
	writel(frame, macb->regs + MACB_MAN);

	do {
		netstat = readl(macb->regs + MACB_NSR);
	} while (!(netstat & MACB_BIT(IDLE)));

	iflag = disable_interrupts();
	netctl = readl(macb->regs + MACB_NCR);
	netctl &= ~MACB_BIT(MPE);
	writel(netctl, macb->regs + MACB_NCR);
	if (iflag)
		enable_interrupts();

	return 0;
}

static int macb_get_ethaddr(struct eth_device *edev, unsigned char *adr)
{
	debug("%s\n", __func__);

	return -1;
}

static int macb_set_ethaddr(struct eth_device *edev, unsigned char *adr)
{
	struct macb_device *macb = edev->priv; 

	debug("%s\n", __func__);

	/* set hardware address */

	writel(adr[0] | adr[1] << 8 | adr[2] << 16 | adr[3] << 24, macb->regs + MACB_SA1B);
	writel(adr[4] | adr[5] << 8, macb->regs + MACB_SA1T);

	return 0;
}

static int macb_probe(struct device_d *dev)
{
	struct eth_device *edev;
	struct macb_device *macb;
	unsigned long macb_hz;
	u32 ncfgr;
	struct at91_ether_platform_data *pdata;
#if defined(CONFIG_ARCH_AT91)
	struct clk *pclk;
#endif

	if (!dev->platform_data) {
		printf("macb: no platform_data\n");
		return -ENODEV;
	}
	pdata = dev->platform_data;

	edev = xzalloc(sizeof(struct eth_device) + sizeof(struct macb_device));
	dev->type_data = edev;
	edev->priv = (struct macb_device *)(edev + 1);
	macb = edev->priv;

	edev->init = macb_init;
	edev->open = macb_open;
	edev->send = macb_send;
	edev->recv = macb_recv;
	edev->halt = macb_halt;
	edev->get_ethaddr = macb_get_ethaddr;
	edev->set_ethaddr = macb_set_ethaddr;

	macb->miidev.read = macb_phy_read;
	macb->miidev.write = macb_phy_write;
	macb->miidev.address = pdata->phy_addr;
	macb->miidev.flags = pdata->flags & AT91SAM_ETHER_FORCE_LINK ?
		MIIDEV_FORCE_LINK : 0;
	macb->miidev.edev = edev;
	macb->flags = pdata->flags;

	macb->rx_buffer = xmalloc(CFG_MACB_RX_BUFFER_SIZE); 
	macb->rx_ring = xmalloc(CFG_MACB_RX_RING_SIZE * sizeof(struct macb_dma_desc));
	macb->tx_ring = xmalloc(sizeof(struct macb_dma_desc));

	macb->regs = (void *)dev->map_base;

	/*
	 * Do some basic initialization so that we at least can talk
	 * to the PHY
	 */
#if defined(CONFIG_ARCH_AT91)
	pclk = clk_get(dev, "macb_clk");
	clk_enable(pclk);
	macb_hz = clk_get_rate(pclk);
#else
	macb_hz = get_macb_pclk_rate(0);
#endif
	if (macb_hz < 20000000)
		ncfgr = MACB_BF(CLK, MACB_CLK_DIV8);
	else if (macb_hz < 40000000)
		ncfgr = MACB_BF(CLK, MACB_CLK_DIV16);
	else if (macb_hz < 80000000)
		ncfgr = MACB_BF(CLK, MACB_CLK_DIV32);
	else
		ncfgr = MACB_BF(CLK, MACB_CLK_DIV64);

	writel(ncfgr, macb->regs + MACB_NCFGR);

	mii_register(&macb->miidev);
	eth_register(edev);

	return 0;
}

static struct driver_d macb_driver = {
        .name  = "macb",
        .probe = macb_probe,
};

static int macb_driver_init(void)
{
	debug("%s\n", __func__);
        register_driver(&macb_driver);
        return 0;
}

device_initcall(macb_driver_init);

