// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2014 Lucas Stach <l.stach@pengutronix.de>
 */

#include <common.h>
#include <dma.h>
#include <init.h>
#include <net.h>
#include <malloc.h>
#include <linux/pci.h>

#define NUM_TX_DESC	1
#define NUM_RX_DESC	4
#define PKT_BUF_SIZE	1536
#define ETH_ZLEN	60

struct rtl8169_chip_info {
	const char *name;
	u8 version;
	u32 RxConfigMask;
};

#define BD_STAT_OWN	0x80000000
#define BD_STAT_EOR	0x40000000
#define BD_STAT_FS	0x20000000
#define BD_STAT_LS	0x10000000
#define BD_STAT_RX_RES	0x00200000
struct bufdesc {
	u32 status;
	u32 vlan_tag;
	u32 buf_addr;
	u32 buf_Haddr;
};

struct rtl8169_priv {
	struct eth_device	edev;
	void __iomem		*base;
	struct pci_dev		*pci_dev;
	int			chipset;

	volatile struct bufdesc	*tx_desc;
	dma_addr_t		tx_desc_phys;
	void			*tx_buf;
	unsigned int		cur_tx;

	volatile struct bufdesc	*rx_desc;
	dma_addr_t		rx_desc_phys;
	void			*rx_buf;
	unsigned int		cur_rx;

	struct mii_bus miibus;
};

#define MAC0 			0x00
#define MAR0			0x08
#define TxDescStartAddrLow	0x20
#define TxDescStartAddrHigh	0x24
#define TxHDescStartAddrLow	0x28
#define TxHDescStartAddrHigh	0x2c
#define FLASH			0x30
#define ERSR			0x36
#define ChipCmd			0x37
#define  CmdReset		0x10
#define  CmdRxEnb		0x08
#define  CmdTxEnb		0x04
#define  RxBufEmpty		0x01
#define TxPoll			0x38
#define IntrMask		0x3c
#define IntrStatus		0x3e
#define  SYSErr			0x8000
#define  PCSTimeout		0x4000
#define  SWInt			0x0100
#define  TxDescUnavail		0x80
#define  RxFIFOOver		0x40
#define  RxUnderrun		0x20
#define  RxOverflow		0x10
#define  TxErr			0x08
#define  TxOK			0x04
#define  RxErr			0x02
#define  RxOK			0x01
#define TxConfig		0x40
#define  TxInterFrameGapShift	24
#define  TxDMAShift		8
#define RxConfig		0x44
#define  AcceptErr		0x20
#define  AcceptRunt		0x10
#define  AcceptBroadcast	0x08
#define  AcceptMulticast	0x04
#define  AcceptMyPhys		0x02
#define  AcceptAllPhys		0x01
#define  RxCfgFIFOShift		13
#define  RxCfgDMAShift		8
#define RxMissed		0x4c
#define Cfg9346			0x50
#define  Cfg9346_Lock		0x00
#define  Cfg9346_Unlock		0xc0
#define Config0			0x51
#define Config1			0x52
#define Config2			0x53
#define Config3			0x54
#define Config4			0x55
#define Config5			0x56
#define MultiIntr		0x5c
#define PHYAR			0x60
#define TBICSR			0x64
#define TBI_ANAR		0x68
#define TBI_LPAR		0x6a
#define PHYstatus		0x6c
#define RxMaxSize		0xda
#define CPlusCmd		0xe0
#define RxDescStartAddrLow	0xe4
#define RxDescStartAddrHigh	0xe8
#define EarlyTxThres		0xec
#define FuncEvent		0xf0
#define FuncEventMask		0xf4
#define FuncPresetState		0xf8
#define FuncForceEvent		0xfc

/* write MMIO register */
#define RTL_W8(priv, reg, val)	writeb(val, ((char *)(priv->base) + reg))
#define RTL_W16(priv, reg, val)	writew(val, ((char *)(priv->base) + reg))
#define RTL_W32(priv, reg, val)	writel(val, ((char *)(priv->base) + reg))

/* read MMIO register */
#define RTL_R8(priv, reg)	readb(((char *)(priv->base) + reg))
#define RTL_R16(priv, reg)	readw(((char *)(priv->base) + reg))
#define RTL_R32(priv, reg)	readl(((char *)(priv->base) + reg))

static const u32 rtl8169_rx_config =
		 (7 << RxCfgFIFOShift) | (6 << RxCfgDMAShift);

static void rtl8169_chip_reset(struct rtl8169_priv *priv)
{
	int i;

	/* Soft reset the chip. */
	RTL_W8(priv, ChipCmd, CmdReset);

	/* Check that the chip has finished the reset. */
	for (i = 1000; i > 0; i--) {
		if ((RTL_R8(priv, ChipCmd) & CmdReset) == 0)
			break;
		udelay(10);
	}
}

static struct rtl8169_chip_info chip_info[] = {
	{"RTL-8169",		0x00,	0xff7e1880},
	{"RTL-8169",		0x04,	0xff7e1880},
	{"RTL-8169",		0x00,	0xff7e1880},
	{"RTL-8169s/8110s",	0x02,	0xff7e1880},
	{"RTL-8169s/8110s",	0x04,	0xff7e1880},
	{"RTL-8169sb/8110sb",	0x10,	0xff7e1880},
	{"RTL-8169sc/8110sc",	0x18,	0xff7e1880},
	{"RTL-8168b/8111sb",	0x30,	0xff7e1880},
	{"RTL-8168b/8111sb",	0x38,	0xff7e1880},
	{"RTL-8168d/8111d",	0x28,	0xff7e1880},
	{"RTL-8168evl/8111evl",	0x2e,	0xff7e1880},
	{"RTL-8168/8111g",	0x4c,	0xff7e1880,},
	{"RTL-8101e",		0x34,	0xff7e1880},
	{"RTL-8100e",		0x32,	0xff7e1880},
};

static void rtl8169_chip_identify(struct rtl8169_priv *priv)
{
	u32 val;
	int i;

	val = RTL_R32(priv, TxConfig);
	val = ((val & 0x7c000000) + ((val & 0x00800000) << 2)) >> 24;

	for (i = ARRAY_SIZE(chip_info) - 1; i >= 0; i--){
		if (val == chip_info[i].version) {
			priv->chipset = i;
			dev_dbg(&priv->pci_dev->dev, "found %s chipset\n",
				chip_info[i].name);
			return;
		}
	}

	dev_dbg(&priv->pci_dev->dev,
		"no matching chip version found, assuming RTL-8169\n");
	priv->chipset = 0;
}

static int rtl8169_init_dev(struct eth_device *edev)
{
	struct rtl8169_priv *priv = edev->priv;

	rtl8169_chip_reset(priv);
	rtl8169_chip_identify(priv);
	pci_set_master(priv->pci_dev);

	return 0;
}

static void __set_rx_mode(struct rtl8169_priv *priv)
{
	u32 mc_filter[2], val;

	/* IFF_ALLMULTI */
	/* Too many to filter perfectly -- accept all multicasts. */
	mc_filter[1] = mc_filter[0] = 0xffffffff;

	val = AcceptBroadcast | AcceptMulticast | AcceptMyPhys |
	      rtl8169_rx_config | (RTL_R32(priv, RxConfig) &
				  chip_info[priv->chipset].RxConfigMask);

	RTL_W32(priv, RxConfig, val);
	RTL_W32(priv, MAR0 + 0, mc_filter[0]);
	RTL_W32(priv, MAR0 + 4, mc_filter[1]);
}

static void rtl8169_init_ring(struct rtl8169_priv *priv)
{
	int i;

	priv->cur_rx = priv->cur_tx = 0;

	priv->tx_desc = dma_alloc_coherent(NUM_TX_DESC *
				sizeof(struct bufdesc), &priv->tx_desc_phys);
	priv->tx_buf = malloc(NUM_TX_DESC * PKT_BUF_SIZE);
	priv->rx_desc = dma_alloc_coherent(NUM_RX_DESC *
				sizeof(struct bufdesc), &priv->rx_desc_phys);
	priv->rx_buf = malloc(NUM_RX_DESC * PKT_BUF_SIZE);
	dma_sync_single_for_device((unsigned long)priv->rx_buf,
				   NUM_RX_DESC * PKT_BUF_SIZE, DMA_FROM_DEVICE);

	memset((void *)priv->tx_desc, 0, NUM_TX_DESC * sizeof(struct bufdesc));
	memset((void *)priv->rx_desc, 0, NUM_RX_DESC * sizeof(struct bufdesc));

	for (i = 0; i < NUM_RX_DESC; i++) {
		if (i == (NUM_RX_DESC - 1))
			priv->rx_desc[i].status =
				BD_STAT_OWN | BD_STAT_EOR | PKT_BUF_SIZE;
		else
			priv->rx_desc[i].status =
				BD_STAT_OWN | PKT_BUF_SIZE;

		priv->rx_desc[i].buf_addr =
				virt_to_phys(priv->rx_buf + i * PKT_BUF_SIZE);
	}
}

static void rtl8169_hw_start(struct rtl8169_priv *priv)
{
	u32 val;

	RTL_W8(priv, Cfg9346, Cfg9346_Unlock);

	/* RTL-8169sb/8110sb or previous version */
	if (priv->chipset <= 5)
		RTL_W8(priv, ChipCmd, CmdTxEnb | CmdRxEnb);

	RTL_W8(priv, EarlyTxThres, 0x3f);

	/* For gigabit rtl8169 */
	RTL_W16(priv, RxMaxSize, 0x800);

	/* Set Rx Config register */
	val = rtl8169_rx_config | (RTL_R32(priv, RxConfig) &
	      chip_info[priv->chipset].RxConfigMask);
	RTL_W32(priv, RxConfig, val);

	/* Set DMA burst size and Interframe Gap Time */
	RTL_W32(priv, TxConfig, (6 << TxDMAShift) | (3 << TxInterFrameGapShift));

	RTL_W32(priv, TxDescStartAddrLow, priv->tx_desc_phys);
	RTL_W32(priv, TxDescStartAddrHigh, 0);
	RTL_W32(priv, RxDescStartAddrLow, priv->rx_desc_phys);
	RTL_W32(priv, RxDescStartAddrHigh, 0);

	/* RTL-8169sc/8110sc or later version */
	if (priv->chipset > 5)
		RTL_W8(priv, ChipCmd, CmdTxEnb | CmdRxEnb);

	RTL_W8(priv, Cfg9346, Cfg9346_Lock);
	udelay(10);

	RTL_W32(priv, RxMissed, 0);

	__set_rx_mode(priv);

	/* no early-rx interrupts */
	RTL_W16(priv, MultiIntr, RTL_R16(priv, MultiIntr) & 0xf000);
}

static int rtl8169_eth_open(struct eth_device *edev)
{
	struct rtl8169_priv *priv = edev->priv;
	int ret;

	rtl8169_init_ring(priv);
	rtl8169_hw_start(priv);

	ret = phy_device_connect(edev, &priv->miibus, 0, NULL, 0,
				 PHY_INTERFACE_MODE_NA);

	return ret;
}

static int rtl8169_phy_write(struct mii_bus *bus, int phy_addr,
	int reg, u16 val)
{
	struct rtl8169_priv *priv = bus->priv;
	int i;

	if (phy_addr != 0)
		return -1;

	RTL_W32(priv, PHYAR, 0x80000000 | (reg & 0xff) << 16 | val);
	mdelay(1);

	for (i = 2000; i > 0; i--) {
		if (!(RTL_R32(priv, PHYAR) & 0x80000000)) {
			return 0;
		} else {
			udelay(100);
		}
	}

	return -1;
}

static int rtl8169_phy_read(struct mii_bus *bus, int phy_addr, int reg)
{
	struct rtl8169_priv *priv = bus->priv;
	int i, val = 0xffff;

	RTL_W32(priv, PHYAR, 0x0 | (reg & 0xff) << 16);
	mdelay(10);

	if (phy_addr != 0)
		return val;

	for (i = 2000; i > 0; i--) {
		if (RTL_R32(priv, PHYAR) & 0x80000000) {
			val = (int) (RTL_R32(priv, PHYAR) & 0xffff);
			break;
		} else {
			udelay(100);
		}
	}
	return val;
}

static int rtl8169_eth_send(struct eth_device *edev, void *packet,
				int packet_length)
{
	struct rtl8169_priv *priv = edev->priv;
	unsigned int entry;

	entry = priv->cur_tx % NUM_TX_DESC;

	if (packet_length < ETH_ZLEN)
		memset(priv->tx_buf + entry * PKT_BUF_SIZE, 0, ETH_ZLEN);
	memcpy(priv->tx_buf + entry * PKT_BUF_SIZE, packet, packet_length);
	dma_sync_single_for_device((unsigned long)priv->tx_buf + entry *
				   PKT_BUF_SIZE, PKT_BUF_SIZE, DMA_TO_DEVICE);

	priv->tx_desc[entry].buf_Haddr = 0;
	priv->tx_desc[entry].buf_addr =
		virt_to_phys(priv->tx_buf + entry * PKT_BUF_SIZE);

	if (entry != (NUM_TX_DESC - 1)) {
		priv->tx_desc[entry].status =
			BD_STAT_OWN | BD_STAT_FS | BD_STAT_LS |
			((packet_length > ETH_ZLEN) ? packet_length : ETH_ZLEN);
	} else {
		priv->tx_desc[entry].status =
			BD_STAT_OWN | BD_STAT_EOR | BD_STAT_FS | BD_STAT_LS |
			((packet_length > ETH_ZLEN) ? packet_length : ETH_ZLEN);
	}

	RTL_W8(priv, TxPoll, 0x40);

	while (priv->tx_desc[entry].status & BD_STAT_OWN)
		;

	dma_sync_single_for_cpu((unsigned long)priv->tx_buf + entry *
				PKT_BUF_SIZE, PKT_BUF_SIZE, DMA_TO_DEVICE);

	priv->cur_tx++;

	return 0;
}

static int rtl8169_eth_rx(struct eth_device *edev)
{
	struct rtl8169_priv *priv = edev->priv;
	unsigned int entry, pkt_size = 0;
	u8 status;

	entry = priv->cur_rx % NUM_RX_DESC;

	if ((priv->rx_desc[entry].status & BD_STAT_OWN) == 0) {
		if (!(priv->rx_desc[entry].status & BD_STAT_RX_RES)) {
			pkt_size = (priv->rx_desc[entry].status & 0x1fff) - 4;

			dma_sync_single_for_cpu((unsigned long)priv->rx_buf
						+ entry * PKT_BUF_SIZE,
						pkt_size, DMA_FROM_DEVICE);

			net_receive(edev, priv->rx_buf + entry * PKT_BUF_SIZE,
			            pkt_size);

			dma_sync_single_for_device((unsigned long)priv->rx_buf
						   + entry * PKT_BUF_SIZE,
						   pkt_size, DMA_FROM_DEVICE);

			if (entry == NUM_RX_DESC - 1)
				priv->rx_desc[entry].status = BD_STAT_OWN |
					BD_STAT_EOR | PKT_BUF_SIZE;
			else
				priv->rx_desc[entry].status =
					BD_STAT_OWN | PKT_BUF_SIZE;
			priv->rx_desc[entry].buf_addr =
				virt_to_phys(priv->rx_buf +
				             entry * PKT_BUF_SIZE);
		} else {
			dev_err(&edev->dev, "rx error\n");
		}

		priv->cur_rx++;

		return pkt_size;

	} else {
		status = RTL_R8(priv, IntrStatus);
		RTL_W8(priv, IntrStatus, status & ~(TxErr | RxErr | SYSErr));
		udelay(100);	/* wait */
	}

	return 0;
}

static int rtl8169_get_ethaddr(struct eth_device *edev, unsigned char *m)
{
	struct rtl8169_priv *priv = edev->priv;
	int i;

	for (i = 0; i < 6; i++) {
		m[i] = RTL_R8(priv, MAC0 + i);
	}

	return 0;
}

static int rtl8169_set_ethaddr(struct eth_device *edev, const unsigned char *mac_addr)
{
	struct rtl8169_priv *priv = edev->priv;
	int i;

	RTL_W8(priv, Cfg9346, Cfg9346_Unlock);

	for (i = 0; i < 6; i++) {
		RTL_W8(priv, (MAC0 + i), mac_addr[i]);
		RTL_R8(priv, mac_addr[i]);
	}

	RTL_W8(priv, Cfg9346, Cfg9346_Lock);

	return 0;
}

static void rtl8169_eth_halt(struct eth_device *edev)
{
	struct rtl8169_priv *priv = edev->priv;

	/* Stop the chip's Tx and Rx DMA processes. */
	RTL_W8(priv, ChipCmd, 0x00);

	/* Disable interrupts by clearing the interrupt mask. */
	RTL_W16(priv, IntrMask, 0x0000);
	RTL_W32(priv, RxMissed, 0);

	pci_clear_master(priv->pci_dev);
}

static int rtl8169_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	struct device_d *dev = &pdev->dev;
	struct eth_device *edev;
	struct rtl8169_priv *priv;
	int ret;

	/* enable pci device */
	pci_enable_device(pdev);

	priv = xzalloc(sizeof(struct rtl8169_priv));

	edev = &priv->edev;
	dev->type_data = edev;
	edev->priv = priv;

	priv->pci_dev = pdev;

	priv->miibus.read = rtl8169_phy_read;
	priv->miibus.write = rtl8169_phy_write;
	priv->miibus.priv = priv;
	priv->miibus.parent = &edev->dev;

	priv->base = pci_iomap(pdev, pdev->device == 0x8168 ? 2 : 1);

	dev_dbg(dev, "rtl%04x (rev %02x) (base=%p)\n",
		 pdev->device, pdev->revision, priv->base);

	edev->init = rtl8169_init_dev;
	edev->open = rtl8169_eth_open;
	edev->send = rtl8169_eth_send;
	edev->recv = rtl8169_eth_rx;
	edev->get_ethaddr = rtl8169_get_ethaddr;
	edev->set_ethaddr = rtl8169_set_ethaddr;
	edev->halt = rtl8169_eth_halt;
	edev->parent = dev;
	ret = eth_register(edev);
	if (ret)
		goto eth_err;

	ret = mdiobus_register(&priv->miibus);
	if (ret)
		goto mdio_err;

	return 0;

mdio_err:
	eth_unregister(edev);

eth_err:
	free(priv);

	return ret;
}
static DEFINE_PCI_DEVICE_TABLE(rtl8169_pci_tbl) = {
	{ PCI_DEVICE(PCI_VENDOR_ID_REALTEK, 0x8167), },
	{ PCI_DEVICE(PCI_VENDOR_ID_REALTEK, 0x8168), },
	{ PCI_DEVICE(PCI_VENDOR_ID_REALTEK, 0x8169), },
	{ /* sentinel */ }
};

static struct pci_driver rtl8169_eth_driver = {
	.name = "rtl8169_eth",
	.id_table = rtl8169_pci_tbl,
	.probe = rtl8169_probe,
};

static int rtl8169_init(void)
{
	return pci_register_driver(&rtl8169_eth_driver);
}
device_initcall(rtl8169_init);
