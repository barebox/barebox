// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * ASIX AX88179/178A USB 3.0/2.0 to Gigabit Ethernet Devices
 *
 * Copyright (C) 2011-2013 ASIX
 */
#include <common.h>
#include <init.h>
#include <net.h>
#include <linux/phy.h>
#include <usb/usb.h>
#include <usb/usbnet.h>
#include <errno.h>
#include <malloc.h>
#include <poller.h>
#include <dma.h>
#include <asm/byteorder.h>
#include <asm/unaligned.h>

#define AX88179_PHY_ID				0x03
#define AX_EEPROM_LEN				0x100
#define AX88179_EEPROM_MAGIC			0x17900b95
#define AX_MCAST_FLTSIZE			8
#define AX_MAX_MCAST				64
#define AX_INT_PPLS_LINK			((u32)BIT(16))
#define AX_RXHDR_L4_TYPE_MASK			0x1c
#define AX_RXHDR_L4_TYPE_UDP			4
#define AX_RXHDR_L4_TYPE_TCP			16
#define AX_RXHDR_L3CSUM_ERR			2
#define AX_RXHDR_L4CSUM_ERR			1
#define AX_RXHDR_CRC_ERR			((u32)BIT(29))
#define AX_RXHDR_DROP_ERR			((u32)BIT(31))
#define AX_ACCESS_MAC				0x01
#define AX_ACCESS_PHY				0x02
#define AX_ACCESS_EEPROM			0x04
#define AX_ACCESS_EFUS				0x05
#define AX_PAUSE_WATERLVL_HIGH			0x54
#define AX_PAUSE_WATERLVL_LOW			0x55

#define PHYSICAL_LINK_STATUS			0x02
	#define	AX_USB_SS		0x04
	#define	AX_USB_HS		0x02

#define GENERAL_STATUS				0x03
/* Check AX88179 version. UA1:Bit2 = 0,  UA2:Bit2 = 1 */
	#define	AX_SECLD		0x04

#define AX_SROM_ADDR				0x07
#define AX_SROM_CMD				0x0a
	#define EEP_RD			0x04
	#define EEP_BUSY		0x10

#define AX_SROM_DATA_LOW			0x08
#define AX_SROM_DATA_HIGH			0x09

#define AX_RX_CTL				0x0b
	#define AX_RX_CTL_DROPCRCERR	0x0100
	#define AX_RX_CTL_IPE		0x0200
	#define AX_RX_CTL_START		0x0080
	#define AX_RX_CTL_AP		0x0020
	#define AX_RX_CTL_AM		0x0010
	#define AX_RX_CTL_AB		0x0008
	#define AX_RX_CTL_AMALL		0x0002
	#define AX_RX_CTL_PRO		0x0001
	#define AX_RX_CTL_STOP		0x0000

#define AX_NODE_ID				0x10
#define AX_MULFLTARY				0x16

#define AX_MEDIUM_STATUS_MODE			0x22
	#define AX_MEDIUM_GIGAMODE	0x01
	#define AX_MEDIUM_FULL_DUPLEX	0x02
	#define AX_MEDIUM_EN_125MHZ	0x08
	#define AX_MEDIUM_RXFLOW_CTRLEN	0x10
	#define AX_MEDIUM_TXFLOW_CTRLEN	0x20
	#define AX_MEDIUM_RECEIVE_EN	0x100
	#define AX_MEDIUM_PS		0x200
	#define AX_MEDIUM_JUMBO_EN	0x8040

#define AX_MONITOR_MOD				0x24
	#define AX_MONITOR_MODE_RWLC	0x02
	#define AX_MONITOR_MODE_RWMP	0x04
	#define AX_MONITOR_MODE_PMEPOL	0x20
	#define AX_MONITOR_MODE_PMETYPE	0x40

#define AX_GPIO_CTRL				0x25
	#define AX_GPIO_CTRL_GPIO3EN	0x80
	#define AX_GPIO_CTRL_GPIO2EN	0x40
	#define AX_GPIO_CTRL_GPIO1EN	0x20

#define AX_PHYPWR_RSTCTL			0x26
	#define AX_PHYPWR_RSTCTL_BZ	0x0010
	#define AX_PHYPWR_RSTCTL_IPRL	0x0020
	#define AX_PHYPWR_RSTCTL_AT	0x1000

#define AX_RX_BULKIN_QCTRL			0x2e
#define AX_CLK_SELECT				0x33
	#define AX_CLK_SELECT_BCS	0x01
	#define AX_CLK_SELECT_ACS	0x02
	#define AX_CLK_SELECT_ULR	0x08

#define AX_RXCOE_CTL				0x34
	#define AX_RXCOE_IP		0x01
	#define AX_RXCOE_TCP		0x02
	#define AX_RXCOE_UDP		0x04
	#define AX_RXCOE_TCPV6		0x20
	#define AX_RXCOE_UDPV6		0x40

#define AX_TXCOE_CTL				0x35
	#define AX_TXCOE_IP		0x01
	#define AX_TXCOE_TCP		0x02
	#define AX_TXCOE_UDP		0x04
	#define AX_TXCOE_TCPV6		0x20
	#define AX_TXCOE_UDPV6		0x40

#define AX_LEDCTRL				0x73

#define GMII_PHY_PHYSR				0x11
	#define GMII_PHY_PHYSR_SMASK	0xc000
	#define GMII_PHY_PHYSR_GIGA	0x8000
	#define GMII_PHY_PHYSR_100	0x4000
	#define GMII_PHY_PHYSR_FULL	0x2000
	#define GMII_PHY_PHYSR_LINK	0x400

#define GMII_LED_ACT				0x1a
	#define	GMII_LED_ACTIVE_MASK	0xff8f
	#define	GMII_LED0_ACTIVE	BIT(4)
	#define	GMII_LED1_ACTIVE	BIT(5)
	#define	GMII_LED2_ACTIVE	BIT(6)

#define GMII_LED_LINK				0x1c
	#define	GMII_LED_LINK_MASK	0xf888
	#define	GMII_LED0_LINK_10	BIT(0)
	#define	GMII_LED0_LINK_100	BIT(1)
	#define	GMII_LED0_LINK_1000	BIT(2)
	#define	GMII_LED1_LINK_10	BIT(4)
	#define	GMII_LED1_LINK_100	BIT(5)
	#define	GMII_LED1_LINK_1000	BIT(6)
	#define	GMII_LED2_LINK_10	BIT(8)
	#define	GMII_LED2_LINK_100	BIT(9)
	#define	GMII_LED2_LINK_1000	BIT(10)
	#define	LED0_ACTIVE		BIT(0)
	#define	LED0_LINK_10		BIT(1)
	#define	LED0_LINK_100		BIT(2)
	#define	LED0_LINK_1000		BIT(3)
	#define	LED0_FD			BIT(4)
	#define	LED0_USB3_MASK		0x001f
	#define	LED1_ACTIVE		BIT(5)
	#define	LED1_LINK_10		BIT(6)
	#define	LED1_LINK_100		BIT(7)
	#define	LED1_LINK_1000		BIT(8)
	#define	LED1_FD			BIT(9)
	#define	LED1_USB3_MASK		0x03e0
	#define	LED2_ACTIVE		BIT(10)
	#define	LED2_LINK_1000		BIT(13)
	#define	LED2_LINK_100		BIT(12)
	#define	LED2_LINK_10		BIT(11)
	#define	LED2_FD			BIT(14)
	#define	LED_VALID		BIT(15)
	#define	LED2_USB3_MASK		0x7c00

#define GMII_PHYPAGE				0x1e
#define GMII_PHY_PAGE_SELECT			0x1f
	#define GMII_PHY_PGSEL_EXT	0x0007
	#define GMII_PHY_PGSEL_PAGE0	0x0000
	#define GMII_PHY_PGSEL_PAGE3	0x0003
	#define GMII_PHY_PGSEL_PAGE5	0x0005

static const struct {
	unsigned char ctrl, timer_l, timer_h, size, ifg;
} AX88179_BULKIN_SIZE[] =	{
	{7, 0x4f, 0,	2, 0xff},
	{7, 0x20, 3,	3, 0xff},
	{7, 0xae, 7,	4, 0xff},
	{7, 0xcc, 0x4c, 4, 8},
};

static
int usbnet_read_cmd(struct usbnet *dev, u8 cmd, u8 reqtype,
		    u16 value, u16 index, void *data, u16 size)
{
	void *buf = NULL;
	int err = -ENOMEM;

	if (size) {
		buf = dma_alloc(size);
		if (!buf)
			goto out;
	}

	err = usb_control_msg(dev->udev, usb_rcvctrlpipe(dev->udev, 0),
			      cmd, reqtype, value, index, buf, size,
			      USB_CTRL_GET_TIMEOUT);
	if (err > 0 && err <= size) {
		if (data)
			memcpy(data, buf, err);
	}
	free(buf);
out:
	return err;
}

static
int usbnet_write_cmd(struct usbnet *dev, u8 cmd, u8 reqtype,
		     u16 value, u16 index, const void *data, u16 size)
{
	void *buf = NULL;
	int err = -ENOMEM;

	if (data) {
		buf = dma_alloc(size);
		if (!buf)
			goto out;
		memcpy(buf, data, size);
	} else {
		if (size) {
			err = -EINVAL;
			goto out;
		}
	}

	err = usb_control_msg(dev->udev, usb_sndctrlpipe(dev->udev, 0),
			      cmd, reqtype, value, index, buf, size,
			      USB_CTRL_SET_TIMEOUT);
	free(buf);

out:
	return err;
}

static int __ax88179_read_cmd(struct usbnet *dev, u8 cmd, u16 value, u16 index,
			      u16 size, void *data)
{
	int ret;

	BUG_ON(!dev);

	ret = usbnet_read_cmd(dev, cmd, USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
		 value, index, data, size);

	if (ret < 0)
		dev_warn(&dev->edev.dev, "Failed to read reg index 0x%04x: %d\n",
			    index, ret);

	return ret;
}

static int __ax88179_write_cmd(struct usbnet *dev, u8 cmd, u16 value, u16 index,
			       u16 size, const void *data)
{
	int ret;

	BUG_ON(!dev);

	ret = usbnet_write_cmd(dev, cmd, USB_DIR_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
		 value, index, data, size);

	if (ret < 0)
		dev_warn(&dev->edev.dev, "Failed to write reg index 0x%04x: %d\n",
			    index, ret);

	return ret;
}

static int ax88179_read_cmd(struct usbnet *dev, u8 cmd, u16 value, u16 index,
			    u16 size, void *data)
{
	int ret;

	if (size == 2) {
		u16 buf;
		ret = __ax88179_read_cmd(dev, cmd, value, index, size, &buf);
		*((u16 *)data) = le16_to_cpu(buf);
	} else if (size == 4) {
		u32 buf;
		ret = __ax88179_read_cmd(dev, cmd, value, index, size, &buf);
		*((u32 *)data) = le32_to_cpu(buf);
	} else {
		ret = __ax88179_read_cmd(dev, cmd, value, index, size, data);
	}

	return ret;
}

static int ax88179_write_cmd(struct usbnet *dev, u8 cmd, u16 value, u16 index,
			     u16 size, const void *data)
{
	int ret;

	if (size == 2) {
		u16 buf = cpu_to_le16(*((u16 *)data));
		ret = __ax88179_write_cmd(dev, cmd, value, index, size, &buf);
	} else {
		ret = __ax88179_write_cmd(dev, cmd, value, index, size, data);
	}

	return ret;
}

static int ax88179_mdio_read(struct mii_bus *bus, int phy_id, int loc)
{
	struct usbnet *dev = bus->priv;
	u16 res;
	int ret;
	u16 tmp16;

	tmp16 = AX_PHYPWR_RSTCTL_IPRL;
	ax88179_write_cmd(dev, AX_ACCESS_MAC, AX_PHYPWR_RSTCTL, 2, 2, &tmp16);

	ret = ax88179_read_cmd(dev, AX_ACCESS_PHY, phy_id, (__u16)loc, 2, &res);
	if (ret < 0)
		return ret;

	dev_dbg(&dev->udev->dev, "%s: phy: %d loc: %d ret: %d, res: 0x%04x\n",
		__func__, phy_id, loc, ret, res);

	return res;
}

static int ax88179_mdio_write(struct mii_bus *bus, int phy_id, int loc, u16 val)
{
	struct usbnet *dev = bus->priv;
	u16 res = (u16) val;
	int ret;

	ret = ax88179_write_cmd(dev, AX_ACCESS_PHY, phy_id, (__u16)loc, 2, &res);
	if (ret < 0)
		return ret;

	return 0;
}

static int ax88179_set_mac_addr(struct eth_device *edev, const unsigned char *adr)
{
	struct usbnet *udev = container_of(edev, struct usbnet, edev);
	int ret;

	ret = ax88179_write_cmd(udev, AX_ACCESS_MAC, AX_NODE_ID, ETH_ALEN,
				 ETH_ALEN, adr);
	if (ret < 0)
		return ret;

	return 0;
}

static int ax88179_get_mac_addr(struct eth_device *edev, unsigned char *adr)
{
	struct usbnet *udev = container_of(edev, struct usbnet, edev);

	ax88179_read_cmd(udev, AX_ACCESS_MAC, AX_NODE_ID, ETH_ALEN,
			 ETH_ALEN, adr);
	return 0;
}

struct ax88179_priv {
	struct poller_struct poller;
	uint64_t last;
	struct usbnet *dev;
};

/*
 * FIXME: What is happening here? We have to read from the mdio bus every few
 *        seconds. Otherwise the mdio registers all return zero and the link goes
 *        down. It seems the phy goes into some power saving mode, but I can't
 *        find any reason for this or any traces in the U-Boot or kernel driver
 *        what we could do different.
 */
static void ax88179_poller(struct poller_struct *poller)
{
	struct ax88179_priv *priv = container_of(poller, struct ax88179_priv, poller);
	struct usbnet *dev = priv->dev;

	if (!is_timeout_non_interruptible(priv->last, 2 * SECOND))
		return;

	priv->last = get_time_ns();

	ax88179_mdio_read(&dev->miibus, 3, 0);
}

static int ax88179_bind(struct usbnet *dev)
{
	int ret;
	struct ax88179_priv *priv;

	dev_dbg(&dev->udev->dev, "%s\n", __func__);

	usbnet_get_endpoints(dev);

	/* Initialize MII structure */
	dev->miibus.parent = &dev->udev->dev;
	dev->miibus.read = ax88179_mdio_read;
	dev->miibus.write = ax88179_mdio_write;
	dev->miibus.priv = dev;
	dev->phy_addr = AX88179_PHY_ID;

	dev->rx_urb_size = 1024 * (AX88179_BULKIN_SIZE[3].size + 2);

	ret = mdiobus_register(&dev->miibus);
	if (ret)
		return ret;

	dev->edev.get_ethaddr = ax88179_get_mac_addr;
	dev->edev.set_ethaddr = ax88179_set_mac_addr;

	priv = xzalloc(sizeof(*priv));
	priv->dev = dev;
	dev->driver_priv = priv;

	priv->last = get_time_ns();
	priv->poller.func = ax88179_poller;
	poller_register(&priv->poller);

	return 0;
}

static void ax88179_unbind(struct usbnet *dev)
{
	u16 tmp16;

	/* Configure RX control register => stop operation */
	tmp16 = AX_RX_CTL_STOP;
	ax88179_write_cmd(dev, AX_ACCESS_MAC, AX_RX_CTL, 2, 2, &tmp16);

	tmp16 = 0;
	ax88179_write_cmd(dev, AX_ACCESS_MAC, AX_CLK_SELECT, 1, 1, &tmp16);

	/* Power down ethernet PHY */
	tmp16 = 0;
	ax88179_write_cmd(dev, AX_ACCESS_MAC, AX_PHYPWR_RSTCTL, 2, 2, &tmp16);
}

static int ax88179_rx_fixup(struct usbnet *dev, void *buf, int len)
{
	int pkt_cnt, frame_pos;
	u32 rx_hdr;
	u16 hdr_off;
	u32 *pkt_hdr;

	if (len == dev->rx_urb_size) {
		dev_err(&dev->udev->dev, "broken package\n");
		return 0;
	}

	rx_hdr = get_unaligned_le32(buf + len - 4);

	pkt_cnt = (u16)rx_hdr;
	hdr_off = (u16)(rx_hdr >> 16);
	pkt_hdr = (u32 *)(buf + hdr_off);

	frame_pos = 0;

	while (pkt_cnt--) {
		u16 pkt_len;
		u32 hdr = le32_to_cpup(pkt_hdr);

		pkt_len = (hdr >> 16) & 0x1fff;

		/* Check CRC or runt packet */
		if ((hdr & AX_RXHDR_CRC_ERR) ||
		    (hdr & AX_RXHDR_DROP_ERR)) {
			pkt_hdr++;
			continue;
		}

		frame_pos += 2;

		dev_dbg(&dev->udev->dev, "%s: loop: frame_pos: %d len: %d\n",
			__func__, frame_pos, pkt_len);

		net_receive(&dev->edev, buf + frame_pos, pkt_len);
		
		pkt_hdr++;
		frame_pos += ((pkt_len + 7) & 0xfff8) - 2;
	}

	return 0;
}

static int ax88179_tx_fixup(struct usbnet *dev, void *buf, int len,
			    void *nbuf, int *nlen)
{
	u32 tx_hdr1, tx_hdr2;
	int frame_size = dev->maxpacket;

	tx_hdr1 = len;
	tx_hdr2 = 0;
	if (((len + 8) % frame_size) == 0)
		tx_hdr2 |= 0x80008000;	/* Enable padding */

	put_unaligned_le32(tx_hdr1, nbuf);
	put_unaligned_le32(tx_hdr2, nbuf + 4);

	memcpy(nbuf + 8, buf, len);

	*nlen = len + 8;

	return 0;
}

static int ax88179_link_reset(struct usbnet *dev)
{
	u8 link_sts;
	u16 mode, physr;
	int idx;

	dev_dbg(&dev->udev->dev, "%s\n", __func__);

	mode = AX_MEDIUM_RECEIVE_EN | AX_MEDIUM_TXFLOW_CTRLEN |
	       AX_MEDIUM_RXFLOW_CTRLEN;

	ax88179_read_cmd(dev, AX_ACCESS_MAC, PHYSICAL_LINK_STATUS,
			 1, 1, &link_sts);

	ax88179_read_cmd(dev, AX_ACCESS_PHY, AX88179_PHY_ID,
			 GMII_PHY_PHYSR, 2, &physr);

	if (!(physr & GMII_PHY_PHYSR_LINK))
		return 0;

	dev_dbg(&dev->udev->dev, "%s: link_sts: 0x%08x GMII_PHY_PHYSR: 0x%08x\n",
		__func__, link_sts, physr);

	if ((physr & GMII_PHY_PHYSR_SMASK) == GMII_PHY_PHYSR_GIGA) {
		mode |= AX_MEDIUM_GIGAMODE | AX_MEDIUM_EN_125MHZ;

		if (link_sts & AX_USB_SS)
			idx = 0;
		else if (link_sts & AX_USB_HS)
			idx = 1;
		else
			idx = 3;
	} else if ((physr & GMII_PHY_PHYSR_SMASK) == GMII_PHY_PHYSR_100) {
		mode |= AX_MEDIUM_PS;

		if (link_sts & (AX_USB_SS | AX_USB_HS))
			idx = 2;
		else
			idx = 3;
	} else {
		idx = 3;
	}

	/* RX bulk configuration */
	ax88179_write_cmd(dev, AX_ACCESS_MAC, AX_RX_BULKIN_QCTRL, 5, 5,
			  &AX88179_BULKIN_SIZE[idx]);

	dev->rx_urb_size = 1024 * (AX88179_BULKIN_SIZE[idx].size + 2);

	if (physr & GMII_PHY_PHYSR_FULL)
		mode |= AX_MEDIUM_FULL_DUPLEX;

	ax88179_write_cmd(dev, AX_ACCESS_MAC, AX_MEDIUM_STATUS_MODE,
			  2, 2, &mode);

	return 0;
}

static int ax88179_reset(struct usbnet *dev)
{
	u16 tmp16;
	u8 tmp;

	dev_dbg(&dev->udev->dev, "%s\n", __func__);

	/* Power up ethernet PHY */
	tmp16 = 0;
	ax88179_write_cmd(dev, AX_ACCESS_MAC, AX_PHYPWR_RSTCTL, 2, 2, &tmp16);

	tmp16 = AX_PHYPWR_RSTCTL_IPRL;
	ax88179_write_cmd(dev, AX_ACCESS_MAC, AX_PHYPWR_RSTCTL, 2, 2, &tmp16);
	mdelay(200);

	tmp = AX_CLK_SELECT_ACS | AX_CLK_SELECT_BCS;
	ax88179_write_cmd(dev, AX_ACCESS_MAC, AX_CLK_SELECT, 1, 1, &tmp);
	mdelay(100);

	/* RX bulk configuration */
	ax88179_write_cmd(dev, AX_ACCESS_MAC, AX_RX_BULKIN_QCTRL, 5, 5,
			  &AX88179_BULKIN_SIZE[0]);

	tmp = 0x34;
	ax88179_write_cmd(dev, AX_ACCESS_MAC, AX_PAUSE_WATERLVL_LOW, 1, 1, &tmp);

	tmp = 0x52;
	ax88179_write_cmd(dev, AX_ACCESS_MAC, AX_PAUSE_WATERLVL_HIGH,
			  1, 1, &tmp);

	/* Enable checksum offload */
	tmp = AX_RXCOE_IP | AX_RXCOE_TCP | AX_RXCOE_UDP |
	      AX_RXCOE_TCPV6 | AX_RXCOE_UDPV6;
	ax88179_write_cmd(dev, AX_ACCESS_MAC, AX_RXCOE_CTL, 1, 1, &tmp);

	tmp = AX_TXCOE_IP | AX_TXCOE_TCP | AX_TXCOE_UDP |
	      AX_TXCOE_TCPV6 | AX_TXCOE_UDPV6;
	ax88179_write_cmd(dev, AX_ACCESS_MAC, AX_TXCOE_CTL, 1, 1, &tmp);

	/* Configure RX control register => start operation */
	tmp16 = AX_RX_CTL_DROPCRCERR | AX_RX_CTL_IPE | AX_RX_CTL_START |
		AX_RX_CTL_AP | AX_RX_CTL_AMALL | AX_RX_CTL_AB;
	ax88179_write_cmd(dev, AX_ACCESS_MAC, AX_RX_CTL, 2, 2, &tmp16);

	tmp = AX_MONITOR_MODE_PMETYPE | AX_MONITOR_MODE_PMEPOL |
	      AX_MONITOR_MODE_RWMP;
	ax88179_write_cmd(dev, AX_ACCESS_MAC, AX_MONITOR_MOD, 1, 1, &tmp);

	/* Configure default medium type => giga */
	tmp16 = AX_MEDIUM_RECEIVE_EN | AX_MEDIUM_TXFLOW_CTRLEN |
		AX_MEDIUM_RXFLOW_CTRLEN | AX_MEDIUM_FULL_DUPLEX |
		AX_MEDIUM_GIGAMODE;
	ax88179_write_cmd(dev, AX_ACCESS_MAC, AX_MEDIUM_STATUS_MODE,
			  2, 2, &tmp16);

	return 0;
}

static const struct driver_info ax88179_info = {
	.description = "ASIX AX88179 USB 3.0 Gigabit Ethernet",
	.bind = ax88179_bind,
	.unbind = ax88179_unbind,
	.link_reset = ax88179_link_reset,
	.reset = ax88179_reset,
	.flags = FLAG_ETHER | FLAG_FRAMING_AX,
	.rx_fixup = ax88179_rx_fixup,
	.tx_fixup = ax88179_tx_fixup,
};

static const struct driver_info ax88178a_info = {
	.description = "ASIX AX88178A USB 2.0 Gigabit Ethernet",
	.bind = ax88179_bind,
	.unbind = ax88179_unbind,
	.link_reset = ax88179_link_reset,
	.reset = ax88179_reset,
	.flags = FLAG_ETHER | FLAG_FRAMING_AX,
	.rx_fixup = ax88179_rx_fixup,
	.tx_fixup = ax88179_tx_fixup,
};

static const struct driver_info cypress_GX3_info = {
	.description = "Cypress GX3 SuperSpeed to Gigabit Ethernet Controller",
	.bind = ax88179_bind,
	.unbind = ax88179_unbind,
	.link_reset = ax88179_link_reset,
	.reset = ax88179_reset,
	.flags = FLAG_ETHER | FLAG_FRAMING_AX,
	.rx_fixup = ax88179_rx_fixup,
	.tx_fixup = ax88179_tx_fixup,
};

static const struct driver_info dlink_dub1312_info = {
	.description = "D-Link DUB-1312 USB 3.0 to Gigabit Ethernet Adapter",
	.bind = ax88179_bind,
	.unbind = ax88179_unbind,
	.link_reset = ax88179_link_reset,
	.reset = ax88179_reset,
	.flags = FLAG_ETHER | FLAG_FRAMING_AX,
	.rx_fixup = ax88179_rx_fixup,
	.tx_fixup = ax88179_tx_fixup,
};

static const struct driver_info sitecom_info = {
	.description = "Sitecom USB 3.0 to Gigabit Adapter",
	.bind = ax88179_bind,
	.unbind = ax88179_unbind,
	.link_reset = ax88179_link_reset,
	.reset = ax88179_reset,
	.flags = FLAG_ETHER | FLAG_FRAMING_AX,
	.rx_fixup = ax88179_rx_fixup,
	.tx_fixup = ax88179_tx_fixup,
};

static const struct driver_info samsung_info = {
	.description = "Samsung USB Ethernet Adapter",
	.bind = ax88179_bind,
	.unbind = ax88179_unbind,
	.link_reset = ax88179_link_reset,
	.reset = ax88179_reset,
	.flags = FLAG_ETHER | FLAG_FRAMING_AX,
	.rx_fixup = ax88179_rx_fixup,
	.tx_fixup = ax88179_tx_fixup,
};

static const struct driver_info lenovo_info = {
	.description = "Lenovo OneLinkDock Gigabit LAN",
	.bind = ax88179_bind,
	.unbind = ax88179_unbind,
	.link_reset = ax88179_link_reset,
	.reset = ax88179_reset,
	.flags = FLAG_ETHER | FLAG_FRAMING_AX,
	.rx_fixup = ax88179_rx_fixup,
	.tx_fixup = ax88179_tx_fixup,
};

static const struct driver_info belkin_info = {
	.description = "Belkin USB Ethernet Adapter",
	.bind	= ax88179_bind,
	.unbind = ax88179_unbind,
	.link_reset = ax88179_link_reset,
	.reset	= ax88179_reset,
	.flags	= FLAG_ETHER | FLAG_FRAMING_AX,
	.rx_fixup = ax88179_rx_fixup,
	.tx_fixup = ax88179_tx_fixup,
};

static const struct usb_device_id products[] = {
{
	/* ASIX AX88179 10/100/1000 */
	USB_DEVICE(0x0b95, 0x1790),
	.driver_info = &ax88179_info,
}, {
	/* ASIX AX88178A 10/100/1000 */
	USB_DEVICE(0x0b95, 0x178a),
	.driver_info = &ax88178a_info,
}, {
	/* Cypress GX3 SuperSpeed to Gigabit Ethernet Bridge Controller */
	USB_DEVICE(0x04b4, 0x3610),
	.driver_info = &cypress_GX3_info,
}, {
	/* D-Link DUB-1312 USB 3.0 to Gigabit Ethernet Adapter */
	USB_DEVICE(0x2001, 0x4a00),
	.driver_info = &dlink_dub1312_info,
}, {
	/* Sitecom USB 3.0 to Gigabit Adapter */
	USB_DEVICE(0x0df6, 0x0072),
	.driver_info = &sitecom_info,
}, {
	/* Samsung USB Ethernet Adapter */
	USB_DEVICE(0x04e8, 0xa100),
	.driver_info = &samsung_info,
}, {
	/* Lenovo OneLinkDock Gigabit LAN */
	USB_DEVICE(0x17ef, 0x304b),
	.driver_info = &lenovo_info,
}, {
	/* Belkin B2B128 USB 3.0 Hub + Gigabit Ethernet Adapter */
	USB_DEVICE(0x050d, 0x0128),
	.driver_info = &belkin_info,
},
	{ },
};

static struct usb_driver ax88179_178a_driver = {
	.name =		"ax88179_178a",
	.id_table =	products,
	.probe =	usbnet_probe,
	.disconnect =	usbnet_disconnect,
};

static int __init ax88179_178a_init(void)
{
	return usb_driver_register(&ax88179_178a_driver);
}
device_initcall(ax88179_178a_init);
