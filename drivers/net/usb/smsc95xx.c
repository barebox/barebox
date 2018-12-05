// SPDX-License-Identifier: GPL-2.0-or-later
/* Copyright (C) 2007-2008 SMSC */

#include <common.h>
#include <command.h>
#include <init.h>
#include <net.h>
#include <usb/usb.h>
#include <usb/usbnet.h>
#include <malloc.h>
#include <asm/byteorder.h>
#include <errno.h>
#include <linux/phy.h>
#include "smsc95xx.h"

#define SMSC_CHIPNAME			"smsc95xx"
#define SMSC_DRIVER_VERSION		"1.0.4"
#define HS_USB_PKT_SIZE			(512)
#define FS_USB_PKT_SIZE			(64)
#define DEFAULT_HS_BURST_CAP_SIZE	(16 * 1024 + 5 * HS_USB_PKT_SIZE)
#define DEFAULT_FS_BURST_CAP_SIZE	(6 * 1024 + 33 * FS_USB_PKT_SIZE)
#define DEFAULT_BULK_IN_DELAY		(0x00002000)
#define MAX_SINGLE_PACKET_SIZE		(2048)
#define LAN95XX_EEPROM_MAGIC		(0x9500)
#define EEPROM_MAC_OFFSET		(0x01)
#define DEFAULT_TX_CSUM_ENABLE		(1)
#define DEFAULT_RX_CSUM_ENABLE		(1)
#define SMSC95XX_INTERNAL_PHY_ID	(1)
#define SMSC95XX_TX_OVERHEAD		(8)
#define SMSC95XX_TX_OVERHEAD_CSUM	(12)

#define ETH_ALEN 6
#define NET_IP_ALIGN	2
#define ETH_FRAME_LEN	1514	/* Max. octets in frame sans FCS */
#define ETH_P_8021Q	0x8100	/* 802.1Q VLAN Extended Header  */

#define netdev_warn(x, fmt, arg...)	printf(fmt, ##arg)
#ifdef DEBUG
#define netif_dbg(x, y, z, fmt, arg...)	printf(fmt, ##arg)
#else
#define netif_dbg(x, y, z, fmt, arg...)	do {} while(0)
#endif

#define FLOW_CTRL_RX	0x02

struct smsc95xx_priv {
	u32 mac_cr;
	int use_tx_csum;
	int use_rx_csum;
};

static int turbo_mode = 0;

static int smsc95xx_read_reg(struct usbnet *dev, u32 index, u32 *data)
{
	int ret;

	ret = usb_control_msg(dev->udev, usb_rcvctrlpipe(dev->udev, 0),
		USB_VENDOR_REQUEST_READ_REGISTER,
		USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
		00, index, data, 4, USB_CTRL_GET_TIMEOUT);

	if (ret < 0)
		netdev_warn(dev->net, "Failed to read register index 0x%08x\n", index);

	le32_to_cpus(data);

	debug("%s: 0x%08x 0x%08x\n", __func__, index, *data);

	return ret;
}

static int smsc95xx_write_reg(struct usbnet *dev, u32 index, u32 data)
{
	int ret;

	cpu_to_le32s(&data);

	ret = usb_control_msg(dev->udev, usb_sndctrlpipe(dev->udev, 0),
		USB_VENDOR_REQUEST_WRITE_REGISTER,
		USB_DIR_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
		00, index, &data, 4, USB_CTRL_SET_TIMEOUT);

	if (ret < 0)
		netdev_warn(dev->net, "Failed to write register index 0x%08x\n", index);

	debug("%s: 0x%08x 0x%08x\n", __func__, index, data);

	return ret;
}

/* Loop until the read is completed with timeout
 * called with phy_mutex held */
static int smsc95xx_phy_wait_not_busy(struct usbnet *dev)
{
	u32 val;
	int timeout = 1000;

	do {
		smsc95xx_read_reg(dev, MII_ADDR, &val);
		if (!(val & MII_BUSY_))
			return 0;
		udelay(100);
	} while (--timeout);

	return -EIO;
}

static int smsc95xx_mdio_read(struct mii_bus *bus, int phy_id, int idx)
{
	struct usbnet *dev = bus->priv;
	u32 val, addr;

	/* confirm MII not busy */
	if (smsc95xx_phy_wait_not_busy(dev)) {
		netdev_warn(dev->net, "MII is busy in smsc95xx_mdio_read\n");
		return -EIO;
	}

	/* set the address, index & direction (read from PHY) */
	addr = (phy_id << 11) | (idx << 6) | MII_READ_;
	smsc95xx_write_reg(dev, MII_ADDR, addr);

	if (smsc95xx_phy_wait_not_busy(dev)) {
		netdev_warn(dev->net, "Timed out reading MII reg %02X\n", idx);
		return -EIO;
	}

	smsc95xx_read_reg(dev, MII_DATA, &val);

	return val & 0xffff;
}

static int smsc95xx_mdio_write(struct mii_bus *bus, int phy_id, int idx,
		u16 regval)
{
	struct usbnet *dev = bus->priv;
	u32 val, addr;

	/* confirm MII not busy */
	if (smsc95xx_phy_wait_not_busy(dev)) {
		netdev_warn(dev->net, "MII is busy in smsc95xx_mdio_write\n");
		return -EBUSY;
	}

	val = regval;
	smsc95xx_write_reg(dev, MII_DATA, val);

	/* set the address, index & direction (write to PHY) */
	addr = (phy_id << 11) | (idx << 6) | MII_WRITE_;
	smsc95xx_write_reg(dev, MII_ADDR, addr);

	if (smsc95xx_phy_wait_not_busy(dev))
		netdev_warn(dev->net, "Timed out writing MII reg %02X\n", idx);

	return 0;
}

static int smsc95xx_wait_eeprom(struct usbnet *dev)
{
	int timeout = 1000;
	u32 val;

	do {
		smsc95xx_read_reg(dev, E2P_CMD, &val);
		if (!(val & E2P_CMD_BUSY_) || (val & E2P_CMD_TIMEOUT_))
			break;
		udelay(100);
	} while (--timeout);

	if (val & (E2P_CMD_TIMEOUT_ | E2P_CMD_BUSY_)) {
		netdev_warn(dev->net, "EEPROM read operation timeout\n");
		return -EIO;
	}

	return 0;
}

static int smsc95xx_eeprom_confirm_not_busy(struct usbnet *dev)
{
	int timeout = 1000;
	u32 val;

	do {
		smsc95xx_read_reg(dev, E2P_CMD, &val);

		if (!(val & E2P_CMD_BUSY_))
			return 0;
		udelay(100);
	} while (--timeout);

	netdev_warn(dev->net, "EEPROM is busy\n");
	return -EIO;
}

static int smsc95xx_read_eeprom(struct usbnet *dev, u32 offset, u32 length,
				u8 *data)
{
	u32 val;
	int i, ret;

	ret = smsc95xx_eeprom_confirm_not_busy(dev);
	if (ret)
		return ret;

	for (i = 0; i < length; i++) {
		val = E2P_CMD_BUSY_ | E2P_CMD_READ_ | (offset & E2P_CMD_ADDR_);
		smsc95xx_write_reg(dev, E2P_CMD, val);

		ret = smsc95xx_wait_eeprom(dev);
		if (ret < 0)
			return ret;

		smsc95xx_read_reg(dev, E2P_DATA, &val);

		data[i] = val & 0xFF;
		offset++;
	}

	return 0;
}

#define GPIO_CFG_GPEN_		(0xff000000)
#define GPIO_CFG_GPO0_EN_	(0x01000000)
#define GPIO_CFG_GPTYPE		(0x00ff0000)
#define GPIO_CFG_GPO0_TYPE	(0x00010000)
#define GPIO_CFG_GPDIR_		(0x0000ff00)
#define GPIO_CFG_GPO0_DIR_	(0x00000100)
#define GPIO_CFG_GPDATA_	(0x000000ff)
#define GPIO_CFG_GPO0_DATA_	(0x00000001)
#define LED_GPIO_CFG_FDX_LED	(0x00010000)
#define LED_GPIO_CFG_GPBUF_08_	(0x00000100)
#define LED_GPIO_CFG_GPDIR_08_	(0x00000010)
#define LED_GPIO_CFG_GPDATA_08_	(0x00000001)
#define LED_GPIO_CFG_GPCTL_LED_	(0x00000001)

#if 0
static int smsc95xx_enable_gpio(struct usbnet *dev, int gpio, int type)
{
	int ret = -1;
	u32 val, reg;
	int dir_shift, enable_shift, type_shift;

	if (gpio < 8) {
		reg = GPIO_CFG;
		enable_shift = 24 + gpio;
		type_shift = 16 + gpio;
		dir_shift = 8 + gpio;
	} else {
		gpio -= 8;
		reg = LED_GPIO_CFG;
		enable_shift = 16 + gpio * 4;
		type_shift = 8 + gpio;
		dir_shift = 4 + gpio;
	}

	ret = smsc95xx_read_reg(dev, reg, &val);
	if (ret < 0)
		return ret;

	val &= ~(1 << enable_shift);

	if (type)
		val &= ~(1 << type_shift);
	else
		val |= (1 << type_shift);

	val |= (1 << dir_shift);

	ret = smsc95xx_write_reg(dev, reg, val);

	return ret < 0 ? ret : 0;
}

static int smsc95xx_gpio_set_value(struct usbnet *dev, int gpio, int value)
{
	int ret = -1;
	u32 tmp, reg;

	if (gpio > 10)
		return -EINVAL;

	smsc95xx_enable_gpio(dev, gpio, 0);

	if (gpio < 8) {
		reg = GPIO_CFG;
	} else {
		reg = LED_GPIO_CFG;
		gpio -= 8;
	}

	ret = smsc95xx_read_reg(dev, reg, &tmp);
	if (ret < 0)
		return ret;

	if (value)
		tmp |= 1 << gpio;
	else
		tmp &= ~(1 << gpio);

	ret = smsc95xx_write_reg(dev, reg, tmp);

	return ret < 0 ? ret : 0;
}
#endif

static void smsc95xx_set_multicast(struct usbnet *dev)
{
	struct smsc95xx_priv *pdata = (struct smsc95xx_priv *)(dev->data[0]);
	u32 hash_hi = 0;
	u32 hash_lo = 0;

	netif_dbg(dev, drv, dev->net, "receive own packets only\n");
	pdata->mac_cr &= ~(MAC_CR_PRMS_ | MAC_CR_MCPAS_ | MAC_CR_HPFILT_);

	/* Initiate async writes, as we can't wait for completion here */
	smsc95xx_write_reg(dev, HASHH, hash_hi);
	smsc95xx_write_reg(dev, HASHL, hash_lo);
	smsc95xx_write_reg(dev, MAC_CR, pdata->mac_cr);
}

/* Enable or disable Tx & Rx checksum offload engines */
static int smsc95xx_set_csums(struct usbnet *dev)
{
	struct smsc95xx_priv *pdata = (struct smsc95xx_priv *)(dev->data[0]);
	u32 read_buf;
	int ret = smsc95xx_read_reg(dev, COE_CR, &read_buf);
	if (ret < 0) {
		netdev_warn(dev->net, "Failed to read COE_CR: %d\n", ret);
		return ret;
	}

	if (pdata->use_tx_csum)
		read_buf |= Tx_COE_EN_;
	else
		read_buf &= ~Tx_COE_EN_;

	if (pdata->use_rx_csum)
		read_buf |= Rx_COE_EN_;
	else
		read_buf &= ~Rx_COE_EN_;

	ret = smsc95xx_write_reg(dev, COE_CR, read_buf);
	if (ret < 0) {
		netdev_warn(dev->net, "Failed to write COE_CR: %d\n", ret);
		return ret;
	}

	netif_dbg(dev, hw, dev->net, "COE_CR = 0x%08x\n", read_buf);
	return 0;
}

static int smsc95xx_set_ethaddr(struct eth_device *edev, const unsigned char *adr)
{
	struct usbnet *udev = container_of(edev, struct usbnet, edev);

	u32 addr_lo = adr[0] | adr[1] << 8 |
		adr[2] << 16 | adr[3] << 24;
	u32 addr_hi = adr[4] | adr[5] << 8;
	int ret;

	ret = smsc95xx_write_reg(udev, ADDRL, addr_lo);
	if (ret < 0) {
		netdev_warn(dev->net, "Failed to write ADDRL: %d\n", ret);
		return ret;
	}

	ret = smsc95xx_write_reg(udev, ADDRH, addr_hi);
	if (ret < 0) {
		netdev_warn(dev->net, "Failed to write ADDRH: %d\n", ret);
		return ret;
	}

	return 0;
}

static int smsc95xx_get_ethaddr(struct eth_device *edev, unsigned char *adr)
{
	struct usbnet *udev = container_of(edev, struct usbnet, edev);

	/* try reading mac address from EEPROM */
	if (smsc95xx_read_eeprom(udev, EEPROM_MAC_OFFSET, ETH_ALEN,
			adr) == 0) {
		return 0;
	}

	return -EINVAL;
}

/* starts the TX path */
static void smsc95xx_start_tx_path(struct usbnet *dev)
{
	struct smsc95xx_priv *pdata = (struct smsc95xx_priv *)(dev->data[0]);
	u32 reg_val;

	/* Enable Tx at MAC */
	pdata->mac_cr |= MAC_CR_TXEN_;

	smsc95xx_write_reg(dev, MAC_CR, pdata->mac_cr);

	/* Enable Tx at SCSRs */
	reg_val = TX_CFG_ON_;
	smsc95xx_write_reg(dev, TX_CFG, reg_val);
}

/* Starts the Receive path */
static void smsc95xx_start_rx_path(struct usbnet *dev)
{
	struct smsc95xx_priv *pdata = (struct smsc95xx_priv *)(dev->data[0]);

	pdata->mac_cr |= MAC_CR_RXEN_;

	smsc95xx_write_reg(dev, MAC_CR, pdata->mac_cr);
}

static int smsc95xx_phy_initialize(struct usbnet *dev)
{
	int timeout = 0;
	int phy_id = 1; /* FIXME */
	uint16_t val, bmcr;

	/* Initialize MII structure */
	dev->miibus.read = smsc95xx_mdio_read;
	dev->miibus.write = smsc95xx_mdio_write;
	dev->phy_addr = 1; /* FIXME: asix_get_phy_addr(dev); */
	dev->miibus.priv = dev;
	dev->miibus.parent = &dev->udev->dev;
//	dev->miibus.name = dev->edev.name;

	/* reset phy and wait for reset to complete */
	smsc95xx_mdio_write(&dev->miibus, phy_id, MII_BMCR, BMCR_RESET);

	do {
		udelay(10 * 1000);
		bmcr = smsc95xx_mdio_read(&dev->miibus, phy_id, MII_BMCR);
		timeout++;
	} while ((bmcr & BMCR_RESET) && (timeout < 100));

	if (timeout >= 100) {
		netdev_warn(dev->net, "timeout on PHY Reset");
		return -EIO;
	}

	smsc95xx_mdio_write(&dev->miibus, phy_id, MII_ADVERTISE,
		ADVERTISE_ALL | ADVERTISE_CSMA | ADVERTISE_PAUSE_CAP |
		ADVERTISE_PAUSE_ASYM);

	/* read to clear */
	val = smsc95xx_mdio_read(&dev->miibus, phy_id, PHY_INT_SRC);

	smsc95xx_mdio_write(&dev->miibus, phy_id, PHY_INT_MASK,
		PHY_INT_MASK_DEFAULT_);

	netif_dbg(dev, ifup, dev->net, "phy initialised successfully\n");
	return 0;
}

static int smsc95xx_reset(struct usbnet *dev)
{
	struct smsc95xx_priv *pdata = (struct smsc95xx_priv *)(dev->data[0]);
	u32 read_buf, write_buf, burst_cap = 0;
	int ret = 0, timeout;

	netif_dbg(dev, ifup, dev->net, "entering %s\n", __func__);

	write_buf = HW_CFG_LRST_;
	ret = smsc95xx_write_reg(dev, HW_CFG, write_buf);
	if (ret < 0) {
		netdev_warn(dev->net, "Failed to write HW_CFG_LRST_ bit in HW_CFG register, ret = %d\n",
			    ret);
		return ret;
	}

	timeout = 0;
	do {
		ret = smsc95xx_read_reg(dev, HW_CFG, &read_buf);
		if (ret < 0) {
			netdev_warn(dev->net, "Failed to read HW_CFG: %d\n", ret);
			return ret;
		}
		udelay(1000 * 10);
		timeout++;
	} while ((read_buf & HW_CFG_LRST_) && (timeout < 100));

	if (timeout >= 100) {
		netdev_warn(dev->net, "timeout waiting for completion of Lite Reset\n");
		return ret;
	}

	write_buf = PM_CTL_PHY_RST_;
	ret = smsc95xx_write_reg(dev, PM_CTRL, write_buf);
	if (ret < 0) {
		netdev_warn(dev->net, "Failed to write PM_CTRL: %d\n", ret);
		return ret;
	}

	timeout = 0;
	do {
		ret = smsc95xx_read_reg(dev, PM_CTRL, &read_buf);
		if (ret < 0) {
			netdev_warn(dev->net, "Failed to read PM_CTRL: %d\n", ret);
			return ret;
		}
		udelay(1000 * 10);
		timeout++;
	} while ((read_buf & PM_CTL_PHY_RST_) && (timeout < 100));

	if (timeout >= 100) {
		netdev_warn(dev->net, "timeout waiting for PHY Reset\n");
		return ret;
	}

	ret = smsc95xx_read_reg(dev, HW_CFG, &read_buf);
	if (ret < 0) {
		netdev_warn(dev->net, "Failed to read HW_CFG: %d\n", ret);
		return ret;
	}

	netif_dbg(dev, ifup, dev->net,
		  "Read Value from HW_CFG : 0x%08x\n", read_buf);

	read_buf |= HW_CFG_BIR_;

	ret = smsc95xx_write_reg(dev, HW_CFG, read_buf);
	if (ret < 0) {
		netdev_warn(dev->net, "Failed to write HW_CFG_BIR_ bit in HW_CFG register, ret = %d\n",
			    ret);
		return ret;
	}

	ret = smsc95xx_read_reg(dev, HW_CFG, &read_buf);
	if (ret < 0) {
		netdev_warn(dev->net, "Failed to read HW_CFG: %d\n", ret);
		return ret;
	}
	netif_dbg(dev, ifup, dev->net,
		  "Read Value from HW_CFG after writing HW_CFG_BIR_: 0x%08x\n",
		  read_buf);

	if (!turbo_mode) {
		burst_cap = 0;
		dev->rx_urb_size = MAX_SINGLE_PACKET_SIZE;
	} else if (0) { /* highspeed */
		burst_cap = DEFAULT_HS_BURST_CAP_SIZE / HS_USB_PKT_SIZE;
		dev->rx_urb_size = DEFAULT_HS_BURST_CAP_SIZE;
	} else {
		burst_cap = DEFAULT_FS_BURST_CAP_SIZE / FS_USB_PKT_SIZE;
		dev->rx_urb_size = DEFAULT_FS_BURST_CAP_SIZE;
	}

	netif_dbg(dev, ifup, dev->net,
		  "rx_urb_size=%ld\n", (ulong)dev->rx_urb_size);

	ret = smsc95xx_write_reg(dev, BURST_CAP, burst_cap);
	if (ret < 0) {
		netdev_warn(dev->net, "Failed to write BURST_CAP: %d\n", ret);
		return ret;
	}

	ret = smsc95xx_read_reg(dev, BURST_CAP, &read_buf);
	if (ret < 0) {
		netdev_warn(dev->net, "Failed to read BURST_CAP: %d\n", ret);
		return ret;
	}
	netif_dbg(dev, ifup, dev->net,
		  "Read Value from BURST_CAP after writing: 0x%08x\n",
		  read_buf);

	read_buf = DEFAULT_BULK_IN_DELAY;
	ret = smsc95xx_write_reg(dev, BULK_IN_DLY, read_buf);
	if (ret < 0) {
		netdev_warn(dev->net, "ret = %d\n", ret);
		return ret;
	}

	ret = smsc95xx_read_reg(dev, BULK_IN_DLY, &read_buf);
	if (ret < 0) {
		netdev_warn(dev->net, "Failed to read BULK_IN_DLY: %d\n", ret);
		return ret;
	}
	netif_dbg(dev, ifup, dev->net,
		  "Read Value from BULK_IN_DLY after writing: 0x%08x\n",
		  read_buf);

	ret = smsc95xx_read_reg(dev, HW_CFG, &read_buf);
	if (ret < 0) {
		netdev_warn(dev->net, "Failed to read HW_CFG: %d\n", ret);
		return ret;
	}
	netif_dbg(dev, ifup, dev->net,
		  "Read Value from HW_CFG: 0x%08x\n", read_buf);

	if (turbo_mode)
		read_buf |= (HW_CFG_MEF_ | HW_CFG_BCE_);

	read_buf &= ~HW_CFG_RXDOFF_;

	/* set Rx data offset=2, Make IP header aligns on word boundary. */
	read_buf |= NET_IP_ALIGN << 9;

	ret = smsc95xx_write_reg(dev, HW_CFG, read_buf);
	if (ret < 0) {
		netdev_warn(dev->net, "Failed to write HW_CFG register, ret=%d\n",
			    ret);
		return ret;
	}

	ret = smsc95xx_read_reg(dev, HW_CFG, &read_buf);
	if (ret < 0) {
		netdev_warn(dev->net, "Failed to read HW_CFG: %d\n", ret);
		return ret;
	}
	netif_dbg(dev, ifup, dev->net,
		  "Read Value from HW_CFG after writing: 0x%08x\n", read_buf);

	write_buf = 0xFFFFFFFF;
	ret = smsc95xx_write_reg(dev, INT_STS, write_buf);
	if (ret < 0) {
		netdev_warn(dev->net, "Failed to write INT_STS register, ret=%d\n",
			    ret);
		return ret;
	}

	ret = smsc95xx_read_reg(dev, ID_REV, &read_buf);
	if (ret < 0) {
		netdev_warn(dev->net, "Failed to read ID_REV: %d\n", ret);
		return ret;
	}
	netif_dbg(dev, ifup, dev->net, "ID_REV = 0x%08x\n", read_buf);

	/* Configure GPIO pins as LED outputs */
	write_buf = LED_GPIO_CFG_SPD_LED | LED_GPIO_CFG_LNK_LED |
		LED_GPIO_CFG_FDX_LED;
	ret = smsc95xx_write_reg(dev, LED_GPIO_CFG, write_buf);
	if (ret < 0) {
		netdev_warn(dev->net, "Failed to write LED_GPIO_CFG register, ret=%d\n",
			    ret);
		return ret;
	}

	/* Init Tx */
	write_buf = 0;
	ret = smsc95xx_write_reg(dev, FLOW, write_buf);
	if (ret < 0) {
		netdev_warn(dev->net, "Failed to write FLOW: %d\n", ret);
		return ret;
	}

	read_buf = AFC_CFG_DEFAULT;
	ret = smsc95xx_write_reg(dev, AFC_CFG, read_buf);
	if (ret < 0) {
		netdev_warn(dev->net, "Failed to write AFC_CFG: %d\n", ret);
		return ret;
	}

	/* Don't need mac_cr_lock during initialisation */
	ret = smsc95xx_read_reg(dev, MAC_CR, &pdata->mac_cr);
	if (ret < 0) {
		netdev_warn(dev->net, "Failed to read MAC_CR: %d\n", ret);
		return ret;
	}

	/* Init Rx */
	/* Set Vlan */
	write_buf = (u32)ETH_P_8021Q;
	ret = smsc95xx_write_reg(dev, VLAN1, write_buf);
	if (ret < 0) {
		netdev_warn(dev->net, "Failed to write VAN1: %d\n", ret);
		return ret;
	}

	ret = smsc95xx_set_csums(dev);
	if (ret < 0) {
		netdev_warn(dev->net, "Failed to set csum offload: %d\n", ret);
		return ret;
	}

	smsc95xx_set_multicast(dev);

	if (smsc95xx_phy_initialize(dev) < 0)
		return -EIO;

	ret = smsc95xx_read_reg(dev, INT_EP_CTL, &read_buf);
	if (ret < 0) {
		netdev_warn(dev->net, "Failed to read INT_EP_CTL: %d\n", ret);
		return ret;
	}

	/* enable PHY interrupts */
	read_buf |= INT_EP_CTL_PHY_INT_;

	ret = smsc95xx_write_reg(dev, INT_EP_CTL, read_buf);
	if (ret < 0) {
		netdev_warn(dev->net, "Failed to write INT_EP_CTL: %d\n", ret);
		return ret;
	}

	smsc95xx_start_tx_path(dev);
	smsc95xx_start_rx_path(dev);

	netif_dbg(dev, ifup, dev->net, "%s: return 0\n", __func__);
	return 0;
}

static int smsc95xx_bind(struct usbnet *dev)
{
	struct smsc95xx_priv *pdata = NULL;
	int ret;

	printf(SMSC_CHIPNAME " v" SMSC_DRIVER_VERSION "\n");

	ret = usbnet_get_endpoints(dev);
	if (ret < 0) {
		netdev_warn(dev->net, "usbnet_get_endpoints failed: %d\n", ret);
		return ret;
	}

	dev->data[0] = (unsigned long)malloc(sizeof(struct smsc95xx_priv));

	pdata = (struct smsc95xx_priv *)(dev->data[0]);
	if (!pdata) {
		netdev_warn(dev->net, "Unable to allocate struct smsc95xx_priv\n");
		return -ENOMEM;
	}

	pdata->use_tx_csum = DEFAULT_TX_CSUM_ENABLE;
	pdata->use_rx_csum = DEFAULT_RX_CSUM_ENABLE;

	/* Init all registers */
	ret = smsc95xx_reset(dev);

	dev->edev.get_ethaddr = smsc95xx_get_ethaddr;
	dev->edev.set_ethaddr = smsc95xx_set_ethaddr;
	mdiobus_register(&dev->miibus);

	return 0;
}

static void smsc95xx_unbind(struct usbnet *dev)
{
	struct smsc95xx_priv *pdata = (struct smsc95xx_priv *)(dev->data[0]);

	mdiobus_unregister(&dev->miibus);

	if (pdata) {
		netif_dbg(dev, ifdown, dev->net, "free pdata\n");
		free(pdata);
		pdata = NULL;
		dev->data[0] = 0;
	}
}

static int smsc95xx_rx_fixup(struct usbnet *dev, void *buf, int len)
{
	while (len > 0) {
		u32 header, align_count;
		unsigned char *packet;
		u16 size;

		memcpy(&header, buf, sizeof(header));
		le32_to_cpus(&header);
		buf += 4 + NET_IP_ALIGN;
		len -= 4 + NET_IP_ALIGN;
		packet = buf;

		/* get the packet length */
		size = (u16)((header & RX_STS_FL_) >> 16);
		align_count = (4 - ((size + NET_IP_ALIGN) % 4)) % 4;

		if (header & RX_STS_ES_) {
			netif_dbg(dev, rx_err, dev->net,
				  "Error header=0x%08x\n", header);
		} else {
			/* ETH_FRAME_LEN + 4(CRC) + 2(COE) + 4(Vlan) */
			if (size > (ETH_FRAME_LEN + 12)) {
				netif_dbg(dev, rx_err, dev->net,
					  "size err header=0x%08x\n", header);
				return 0;
			}

			/* last frame in this batch */
			if (len == size) {
				net_receive(&dev->edev, buf, len - 4);
				return 1;
			}

			net_receive(&dev->edev, packet, len - 4);
		}

		len -= size;

		/* padding bytes before the next frame starts */
		if (len)
			len -= align_count;
	}

	if (len < 0) {
		netdev_warn(dev->net, "invalid rx length<0 %d\n", len);
		return 0;
	}

	return 1;
}
#if 0
static u32 smsc95xx_calc_csum_preamble(struct sk_buff *skb)
{
	int len = skb->data - skb->head;
	u16 high_16 = (u16)(skb->csum_offset + skb->csum_start - len);
	u16 low_16 = (u16)(skb->csum_start - len);
	return (high_16 << 16) | low_16;
}
#endif
static int smsc95xx_tx_fixup(struct usbnet *dev,
                                void *buf, int len,
                                void *nbuf, int *nlen)
{
	u32 tx_cmd_a, tx_cmd_b;

	tx_cmd_a = (u32)(len) | TX_CMD_A_FIRST_SEG_ | TX_CMD_A_LAST_SEG_;
	cpu_to_le32s(&tx_cmd_a);
	memcpy(nbuf, &tx_cmd_a, 4);

	tx_cmd_b = (u32)(len);
	cpu_to_le32s(&tx_cmd_b);
	memcpy(nbuf + 4, &tx_cmd_b, 4);

	memcpy(nbuf + 8, buf, len);

	*nlen = len + 8;

	return 0;
}

static struct driver_info smsc95xx_info = {
	.description	= "smsc95xx USB 2.0 Ethernet",
	.bind		= smsc95xx_bind,
	.unbind		= smsc95xx_unbind,
	.rx_fixup	= smsc95xx_rx_fixup,
	.tx_fixup	= smsc95xx_tx_fixup,
};

static const struct usb_device_id products[] = {
	{
		/* SMSC9500 USB Ethernet Device */
		USB_DEVICE(0x0424, 0x9500),
		.driver_info = &smsc95xx_info,
	}, {
		/* SMSC9505 USB Ethernet Device */
		USB_DEVICE(0x0424, 0x9505),
		.driver_info = &smsc95xx_info,
	}, {
		/* SMSC9500A USB Ethernet Device */
		USB_DEVICE(0x0424, 0x9E00),
		.driver_info = &smsc95xx_info,
	}, {
		/* SMSC9505A USB Ethernet Device */
		USB_DEVICE(0x0424, 0x9E01),
		.driver_info = &smsc95xx_info,
	}, {
		/* SMSC9512/9514 USB Hub & Ethernet Device */
		USB_DEVICE(0x0424, 0xec00),
		.driver_info = &smsc95xx_info,
	}, {
		/* SMSC9500 USB Ethernet Device (SAL10) */
		USB_DEVICE(0x0424, 0x9900),
		.driver_info = &smsc95xx_info,
	}, {
		/* SMSC9505 USB Ethernet Device (SAL10) */
		USB_DEVICE(0x0424, 0x9901),
		.driver_info = &smsc95xx_info,
	}, {
		/* SMSC9500A USB Ethernet Device (SAL10) */
		USB_DEVICE(0x0424, 0x9902),
		.driver_info = &smsc95xx_info,
	}, {
		/* SMSC9505A USB Ethernet Device (SAL10) */
		USB_DEVICE(0x0424, 0x9903),
		.driver_info = &smsc95xx_info,
	}, {
		/* SMSC9512/9514 USB Hub & Ethernet Device (SAL10) */
		USB_DEVICE(0x0424, 0x9904),
		.driver_info = &smsc95xx_info,
	}, {
		/* SMSC9500A USB Ethernet Device (HAL) */
		USB_DEVICE(0x0424, 0x9905),
		.driver_info = &smsc95xx_info,
	}, {
		/* SMSC9505A USB Ethernet Device (HAL) */
		USB_DEVICE(0x0424, 0x9906),
		.driver_info = &smsc95xx_info,
	}, {
		/* SMSC9500 USB Ethernet Device (Alternate ID) */
		USB_DEVICE(0x0424, 0x9907),
		.driver_info = &smsc95xx_info,
	}, {
		/* SMSC9500A USB Ethernet Device (Alternate ID) */
		USB_DEVICE(0x0424, 0x9908),
		.driver_info = &smsc95xx_info,
	}, {
		/* SMSC9512/9514 USB Hub & Ethernet Device (Alternate ID) */
		USB_DEVICE(0x0424, 0x9909),
		.driver_info = &smsc95xx_info,
	},
	{ },		/* END */
};

static struct usb_driver smsc95xx_driver = {
	.name =		"smsc95xx",
	.id_table =	products,
	.probe =	usbnet_probe,
	.disconnect =	usbnet_disconnect,
};

static int __init smsc95xx_init(void)
{
	return usb_driver_register(&smsc95xx_driver);
}
device_initcall(smsc95xx_init);
