#include <common.h>
#include <init.h>
#include <net.h>
#include <linux/phy.h>
#include <usb/usb.h>
#include <usb/usbnet.h>
#include <errno.h>
#include <malloc.h>
#include <asm/byteorder.h>
#include <asm/unaligned.h>

/* ASIX AX8817X based USB 2.0 Ethernet Devices */

#define AX_CMD_SET_SW_MII		0x06
#define AX_CMD_READ_MII_REG		0x07
#define AX_CMD_WRITE_MII_REG		0x08
#define AX_CMD_SET_HW_MII		0x0a
#define AX_CMD_READ_EEPROM		0x0b
#define AX_CMD_WRITE_EEPROM		0x0c
#define AX_CMD_WRITE_ENABLE		0x0d
#define AX_CMD_WRITE_DISABLE		0x0e
#define AX_CMD_READ_RX_CTL		0x0f
#define AX_CMD_WRITE_RX_CTL		0x10
#define AX_CMD_READ_IPG012		0x11
#define AX_CMD_WRITE_IPG0		0x12
#define AX_CMD_WRITE_IPG1		0x13
#define AX_CMD_READ_NODE_ID		0x13
#define AX_CMD_WRITE_IPG2		0x14
#define AX_CMD_WRITE_MULTI_FILTER	0x16
#define AX88172_CMD_READ_NODE_ID	0x17
#define AX_CMD_READ_PHY_ID		0x19
#define AX_CMD_READ_MEDIUM_STATUS	0x1a
#define AX_CMD_WRITE_MEDIUM_MODE	0x1b
#define AX_CMD_READ_MONITOR_MODE	0x1c
#define AX_CMD_WRITE_MONITOR_MODE	0x1d
#define AX_CMD_READ_GPIOS		0x1e
#define AX_CMD_WRITE_GPIOS		0x1f
#define AX_CMD_SW_RESET			0x20
#define AX_CMD_SW_PHY_STATUS		0x21
#define AX_CMD_SW_PHY_SELECT		0x22

#define AX_MONITOR_MODE			0x01
#define AX_MONITOR_LINK			0x02
#define AX_MONITOR_MAGIC		0x04
#define AX_MONITOR_HSFS			0x10

/* AX88172 Medium Status Register values */
#define AX88172_MEDIUM_FD		0x02
#define AX88172_MEDIUM_TX		0x04
#define AX88172_MEDIUM_FC		0x10
#define AX88172_MEDIUM_DEFAULT \
		( AX88172_MEDIUM_FD | AX88172_MEDIUM_TX | AX88172_MEDIUM_FC )

#define AX_MCAST_FILTER_SIZE		8
#define AX_MAX_MCAST			64

#define AX_SWRESET_CLEAR		0x00
#define AX_SWRESET_RR			0x01
#define AX_SWRESET_RT			0x02
#define AX_SWRESET_PRTE			0x04
#define AX_SWRESET_PRL			0x08
#define AX_SWRESET_BZ			0x10
#define AX_SWRESET_IPRL			0x20
#define AX_SWRESET_IPPD			0x40

#define AX88772_IPG0_DEFAULT		0x15
#define AX88772_IPG1_DEFAULT		0x0c
#define AX88772_IPG2_DEFAULT		0x12

/* AX88772 & AX88178 Medium Mode Register */
#define AX_MEDIUM_PF		0x0080
#define AX_MEDIUM_JFE		0x0040
#define AX_MEDIUM_TFC		0x0020
#define AX_MEDIUM_RFC		0x0010
#define AX_MEDIUM_ENCK		0x0008
#define AX_MEDIUM_AC		0x0004
#define AX_MEDIUM_FD		0x0002
#define AX_MEDIUM_GM		0x0001
#define AX_MEDIUM_SM		0x1000
#define AX_MEDIUM_SBP		0x0800
#define AX_MEDIUM_PS		0x0200
#define AX_MEDIUM_RE		0x0100

#define AX88178_MEDIUM_DEFAULT	\
	(AX_MEDIUM_PS | AX_MEDIUM_FD | AX_MEDIUM_AC | \
	 AX_MEDIUM_RFC | AX_MEDIUM_TFC | AX_MEDIUM_JFE | \
	 AX_MEDIUM_RE )

#define AX88772_MEDIUM_DEFAULT	\
	(AX_MEDIUM_FD | AX_MEDIUM_RFC | \
	 AX_MEDIUM_TFC | AX_MEDIUM_PS | \
	 AX_MEDIUM_AC | AX_MEDIUM_RE )

/* AX88772 & AX88178 RX_CTL values */
#define AX_RX_CTL_SO			0x0080
#define AX_RX_CTL_AP			0x0020
#define AX_RX_CTL_AM			0x0010
#define AX_RX_CTL_AB			0x0008
#define AX_RX_CTL_SEP			0x0004
#define AX_RX_CTL_AMALL			0x0002
#define AX_RX_CTL_PRO			0x0001
#define AX_RX_CTL_MFB_2048		0x0000
#define AX_RX_CTL_MFB_4096		0x0100
#define AX_RX_CTL_MFB_8192		0x0200
#define AX_RX_CTL_MFB_16384		0x0300

#define AX_DEFAULT_RX_CTL	\
	(AX_RX_CTL_SO | AX_RX_CTL_AB )

/* GPIO 0 .. 2 toggles */
#define AX_GPIO_GPO0EN		0x01	/* GPIO0 Output enable */
#define AX_GPIO_GPO_0		0x02	/* GPIO0 Output value */
#define AX_GPIO_GPO1EN		0x04	/* GPIO1 Output enable */
#define AX_GPIO_GPO_1		0x08	/* GPIO1 Output value */
#define AX_GPIO_GPO2EN		0x10	/* GPIO2 Output enable */
#define AX_GPIO_GPO_2		0x20	/* GPIO2 Output value */
#define AX_GPIO_RESERVED	0x40	/* Reserved */
#define AX_GPIO_RSE		0x80	/* Reload serial EEPROM */

#define AX_EEPROM_MAGIC		0xdeadbeef
#define AX88172_EEPROM_LEN	0x40
#define AX88772_EEPROM_LEN	0xff

#define PHY_MODE_MARVELL	0x0000
#define MII_MARVELL_LED_CTRL	0x0018
#define MII_MARVELL_STATUS	0x001b
#define MII_MARVELL_CTRL	0x0014

#define MARVELL_LED_MANUAL	0x0019

#define MARVELL_STATUS_HWCFG	0x0004

#define MARVELL_CTRL_TXDELAY	0x0002
#define MARVELL_CTRL_RXDELAY	0x0080

#define FLAG_EEPROM_MAC		(1UL << 0) /* init device MAC from eeprom */

#define RX_FIXUP_SIZE	1514

/* This structure cannot exceed sizeof(unsigned long [5]) AKA 20 bytes */
struct asix_data {
	u8 multi_filter[AX_MCAST_FILTER_SIZE];
	u8 phymode;
	u8 ledmode;
	u8 eeprom_len;
};

struct ax88172_int_data {
	__le16 res1;
	u8 link;
	__le16 res2;
	u8 status;
	__le16 res3;
} __attribute__ ((packed));

struct asix_rx_fixup_info {
	u32 header;
	u16 size;
	u16 offset;
	bool split_head;
	unsigned char ax_skb[RX_FIXUP_SIZE] __aligned(2);
};

struct asix_common_private {
	struct asix_rx_fixup_info rx_fixup_info;
};

static int asix_read_cmd(struct usbnet *dev, u8 cmd, u16 value, u16 index,
			    u16 size, void *data)
{
	void *buf;
	int err = -ENOMEM;

	dev_dbg(&dev->edev.dev, "asix_read_cmd() cmd=0x%02x value=0x%04x index=0x%04x size=%d\n",
		cmd, value, index, size);

	buf = malloc(size);
	if (!buf)
		goto out;

	err = usb_control_msg(
		dev->udev,
		usb_rcvctrlpipe(dev->udev, 0),
		cmd,
		USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
		value,
		index,
		buf,
		size,
		USB_CTRL_GET_TIMEOUT);
	if (err == size)
		memcpy(data, buf, size);
	else if (err >= 0)
		err = -EINVAL;
	free(buf);

out:
	return err;
}

static int asix_write_cmd(struct usbnet *dev, u8 cmd, u16 value, u16 index,
			     u16 size, void *data)
{
	void *buf = NULL;
	int err = -ENOMEM;

	dev_dbg(&dev->edev.dev, "asix_write_cmd() cmd=0x%02x value=0x%04x index=0x%04x size=%d\n",
		cmd, value, index, size);

	if (data) {
		buf = malloc(size);
		if (!buf)
			goto out;
		memcpy(buf, data, size);
	}

	err = usb_control_msg(
		dev->udev,
		usb_sndctrlpipe(dev->udev, 0),
		cmd,
		USB_DIR_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
		value,
		index,
		buf,
		size,
		USB_CTRL_SET_TIMEOUT);
	free(buf);

out:
	return err;
}

static inline int asix_set_sw_mii(struct usbnet *dev)
{
	int ret;
	ret = asix_write_cmd(dev, AX_CMD_SET_SW_MII, 0x0000, 0, 0, NULL);
	if (ret < 0)
		dev_err(&dev->edev.dev, "Failed to enable software MII access\n");
	return ret;
}

static inline int asix_set_hw_mii(struct usbnet *dev)
{
	int ret;
	ret = asix_write_cmd(dev, AX_CMD_SET_HW_MII, 0x0000, 0, 0, NULL);
	if (ret < 0)
		dev_err(&dev->edev.dev, "Failed to enable hardware MII access\n");
	return ret;
}

static int asix_mdio_read(struct mii_bus *bus, int phy_id, int loc)
{
	struct usbnet *dev = bus->priv;
	__le16 res;
	int ret;

	ret = asix_set_sw_mii(dev);
	if (ret < 0)
		return ret;

	ret = asix_read_cmd(dev, AX_CMD_READ_MII_REG, phy_id, (__u16)loc, 2, &res);
	if (ret < 0)
		return ret;

	ret = asix_set_hw_mii(dev);
	if (ret < 0)
		return ret;

	dev_dbg(&dev->edev.dev, "asix_mdio_read() phy_id=0x%02x, loc=0x%02x, returns=0x%04x\n",
			phy_id, loc, le16_to_cpu(res));

	return le16_to_cpu(res);
}

static int asix_mdio_write(struct mii_bus *bus, int phy_id, int loc, u16 val)
{
	struct usbnet *dev = bus->priv;
	__le16 res = cpu_to_le16(val);
	int ret;

	dev_dbg(&dev->edev.dev, "asix_mdio_write() phy_id=0x%02x, loc=0x%02x, val=0x%04x\n",
			phy_id, loc, val);

	ret = asix_set_sw_mii(dev);
	if (ret < 0)
		return ret;

	ret = asix_write_cmd(dev, AX_CMD_WRITE_MII_REG, phy_id, (__u16)loc, 2, &res);
	if (ret < 0)
		return ret;

	ret = asix_set_hw_mii(dev);
	if (ret < 0)
		return ret;

	return 0;
}

static inline int asix_get_phy_addr(struct usbnet *dev)
{
	u8 buf[2];
	int ret = asix_read_cmd(dev, AX_CMD_READ_PHY_ID, 0, 0, 2, buf);

	dev_dbg(&dev->edev.dev, "asix_get_phy_addr()");

	if (ret < 0) {
		dev_err(&dev->edev.dev, "Error reading PHYID register: %02x\n", ret);
		goto out;
	}
	dev_dbg(&dev->edev.dev, "asix_get_phy_addr() returning 0x%04x\n", *((__le16 *)buf));
	ret = buf[1];

out:
	return ret;
}

static int asix_sw_reset(struct usbnet *dev, u8 flags)
{
	int ret;

	ret = asix_write_cmd(dev, AX_CMD_SW_RESET, flags, 0, 0, NULL);
	if (ret < 0)
		dev_err(&dev->edev.dev, "Failed to send software reset: %02x\n", ret);

	return ret;
}

static u16 asix_read_rx_ctl(struct usbnet *dev)
{
	__le16 v;
	int ret = asix_read_cmd(dev, AX_CMD_READ_RX_CTL, 0, 0, 2, &v);

	if (ret < 0) {
		dev_err(&dev->edev.dev, "Error reading RX_CTL register: %02x\n", ret);
		goto out;
	}
	ret = le16_to_cpu(v);
out:
	return ret;
}

static int asix_write_rx_ctl(struct usbnet *dev, u16 mode)
{
	int ret;

	dev_dbg(&dev->edev.dev, "asix_write_rx_ctl() - mode = 0x%04x\n", mode);
	ret = asix_write_cmd(dev, AX_CMD_WRITE_RX_CTL, mode, 0, 0, NULL);
	if (ret < 0)
		dev_err(&dev->edev.dev, "Failed to write RX_CTL mode to 0x%04x: %02x\n",
		       mode, ret);

	return ret;
}

static u16 asix_read_medium_status(struct usbnet *dev)
{
	__le16 v;
	int ret = asix_read_cmd(dev, AX_CMD_READ_MEDIUM_STATUS, 0, 0, 2, &v);

	if (ret < 0) {
		dev_err(&dev->edev.dev, "Error reading Medium Status register: %02x\n", ret);
		goto out;
	}
	ret = le16_to_cpu(v);
out:
	return ret;
}

static int asix_write_medium_mode(struct usbnet *dev, u16 mode)
{
	int ret;

	dev_dbg(&dev->edev.dev, "asix_write_medium_mode() - mode = 0x%04x\n", mode);
	ret = asix_write_cmd(dev, AX_CMD_WRITE_MEDIUM_MODE, mode, 0, 0, NULL);
	if (ret < 0)
		dev_err(&dev->edev.dev, "Failed to write Medium Mode mode to 0x%04x: %02x\n",
			mode, ret);

	return ret;
}

static int asix_write_gpio(struct usbnet *dev, u16 value, int sleep)
{
	int ret;

	dev_dbg(&dev->edev.dev,"asix_write_gpio() - value = 0x%04x\n", value);
	ret = asix_write_cmd(dev, AX_CMD_WRITE_GPIOS, value, 0, 0, NULL);
	if (ret < 0)
		dev_err(&dev->edev.dev, "Failed to write GPIO value 0x%04x: %02x\n",
			value, ret);

	if (sleep)
		mdelay(sleep);

	return ret;
}

static int asix_get_ethaddr(struct eth_device *edev, unsigned char *adr)
{
	struct usbnet *udev = container_of(edev, struct usbnet, edev);
	int i, ret;

	/* Get the MAC address */
	if (udev->driver_info->data & FLAG_EEPROM_MAC) {
		for (i = 0; i < (6 >> 1); i++) {
			ret = asix_read_cmd(udev, AX_CMD_READ_EEPROM, 0x04 + i,
					0, 2, adr + i * 2);
			if (ret < 0)
				break;
		}
	} else {
		ret = asix_read_cmd(udev, AX_CMD_READ_NODE_ID, 0, 0, 6, adr);
	}

	if (ret < 0) {
		debug("Failed to read MAC address: %d\n", ret);
		return ret;
	}

	return 0;
}

static int asix_set_ethaddr(struct eth_device *edev, const unsigned char *adr)
{
	/* not possible? */
	return 0;
}

static int ax88172_get_ethaddr(struct eth_device *edev, unsigned char *adr)
{
	struct usbnet *udev = container_of(edev, struct usbnet, edev);
	int ret;

	/* Get the MAC address */
	if ((ret = asix_read_cmd(udev, AX88172_CMD_READ_NODE_ID,
				0, 0, 6, adr)) < 0) {
		debug("read AX_CMD_READ_NODE_ID failed: %d\n", ret);
		return ret;
	}

	return 0;
}

static int asix_rx_fixup_internal(struct usbnet *dev, void *buf, int len,
			   struct asix_rx_fixup_info *rx)
{
	int offset = 0;

	while (offset + sizeof(u16) < len) {
		u16 remaining = 0;

		if (!rx->size) {
			if ((len - offset == sizeof(u16)) ||
			    rx->split_head) {
				if (!rx->split_head) {
					rx->header = get_unaligned_le16(
							buf + offset);
					rx->split_head = true;
					offset += sizeof(u16);
					break;
				} else {
					rx->header |= (get_unaligned_le16(
							buf + offset)
							<< 16);
					rx->split_head = false;
					offset += sizeof(u16);
				}
			} else {
				rx->header = get_unaligned_le32(buf +
								offset);
				offset += sizeof(u32);
			}
			rx->offset = 0U;

			/* get the packet length */
			rx->size = (u16) (rx->header & 0x7ff);
			if (rx->size != ((~rx->header >> 16) & 0x7ff)) {
				dev_err(&dev->edev.dev, "asix_rx_fixup() Bad Header Length 0x%x, offset %d\n",
					   rx->header, offset);
				rx->size = 0;
				return -1;
			}
		}
		if (rx->size > RX_FIXUP_SIZE) {
			dev_err(&dev->edev.dev, "asix_rx_fixup() Bad RX Length %d\n",
				   rx->size);
			rx->offset = 0U;
			rx->size = 0U;

			return -1;
		}

		if (rx->size > (len - offset)) {
			remaining = rx->size - (len - offset);
			rx->size = len - offset;
		}

		memcpy(rx->ax_skb + rx->offset, buf + offset, rx->size);
		rx->offset += rx->size;
		if (!remaining)
			net_receive(&dev->edev, rx->ax_skb,
						(u16) (rx->header & 0x7ff));

		offset += ((rx->size + 1) & 0xfffe);
		rx->size = remaining;
	}

	if (len != offset) {
		dev_err(&dev->edev.dev, "asix_rx_fixup() Bad SKB Length %d, %d\n",
			   len, offset);
		return -1;
	}

	return 0;
}

static int asix_rx_fixup_common(struct usbnet *dev, void *buf, int len)
{
	struct asix_common_private *dp = dev->driver_priv;
	struct asix_rx_fixup_info *rx = &dp->rx_fixup_info;

	return asix_rx_fixup_internal(dev, buf, len, rx);
}

static int asix_tx_fixup(struct usbnet *dev,
				void *buf, int len,
				void *nbuf, int *nlen)
{
	unsigned int packet_len;

	memmove(nbuf + 4, buf, len);

	packet_len = ((len ^ 0x0000ffff) << 16) + len;
	cpu_to_le32s(&packet_len);
	memcpy(nbuf, &packet_len, 4);
	len += 4;

	if((len % 512) == 0) {
		unsigned int padbytes = 0xffff0000;
		cpu_to_le32s(&padbytes);
		memcpy(nbuf + len, &padbytes, 4);
		len += 4;
	}

	*nlen = len;

	return 0;
}

static int asix_init_mii(struct usbnet *dev)
{
	dev->miibus.read = asix_mdio_read;
	dev->miibus.write = asix_mdio_write;
	dev->phy_addr = asix_get_phy_addr(dev);
	dev->miibus.priv = dev;
	dev->miibus.parent = &dev->udev->dev;

	return mdiobus_register(&dev->miibus);
}

static int ax88172_link_reset(struct usbnet *dev)
{
	u8 mode;

	mode = AX88172_MEDIUM_DEFAULT;

//	if (ecmd.duplex != DUPLEX_FULL)
//		mode |= ~AX88172_MEDIUM_FD;

	asix_write_medium_mode(dev, mode);

	return 0;
}

static int ax88172_bind(struct usbnet *dev)
{
	int ret = 0;
	int i;
	unsigned long gpio_bits = dev->driver_info->data;
	struct asix_data *data = (struct asix_data *)&dev->data;

	dev_dbg(&dev->edev.dev, "%s\n", __func__);

	data->eeprom_len = AX88172_EEPROM_LEN;

	ret = usbnet_get_endpoints(dev);
	if (ret) {
		dev_err(&dev->edev.dev, "can not get EPs\n");
		return ret;
	}

	/* Toggle the GPIOs in a manufacturer/model specific way */
	for (i = 2; i >= 0; i--) {
		if ((ret = asix_write_cmd(dev, AX_CMD_WRITE_GPIOS,
					(gpio_bits >> (i * 8)) & 0xff, 0, 0,
					NULL)) < 0)
			goto out;
		mdelay(5);
	}

	if ((ret = asix_write_rx_ctl(dev, 0x80)) < 0)
		goto out;

	dev->edev.get_ethaddr = ax88172_get_ethaddr;
	dev->edev.set_ethaddr = asix_set_ethaddr;
	asix_init_mii(dev);

	return 0;

out:
	return ret;
}

static int ax88772_bind(struct usbnet *dev)
{
	int ret, embd_phy;
	u16 rx_ctl;
	struct asix_data *data = (struct asix_data *)&dev->data;

	debug("%s\n", __func__);

	data->eeprom_len = AX88772_EEPROM_LEN;

	usbnet_get_endpoints(dev);

	if ((ret = asix_write_gpio(dev,
			AX_GPIO_RSE | AX_GPIO_GPO_2 | AX_GPIO_GPO2EN, 5)) < 0)
		goto out;

	/* 0x10 is the phy id of the embedded 10/100 ethernet phy */
	embd_phy = ((asix_get_phy_addr(dev) & 0x1f) == 0x10 ? 1 : 0);
	if ((ret = asix_write_cmd(dev, AX_CMD_SW_PHY_SELECT,
				embd_phy, 0, 0, NULL)) < 0) {
		debug("Select PHY #1 failed: %d\n", ret);
		goto out;
	}

	if ((ret = asix_sw_reset(dev, AX_SWRESET_IPPD | AX_SWRESET_PRL)) < 0)
		goto out;

	mdelay(150);
	if ((ret = asix_sw_reset(dev, AX_SWRESET_CLEAR)) < 0)
		goto out;

	mdelay(150);
	if (embd_phy) {
		if ((ret = asix_sw_reset(dev, AX_SWRESET_IPRL)) < 0)
			goto out;
	}
	else {
		if ((ret = asix_sw_reset(dev, AX_SWRESET_PRTE)) < 0)
			goto out;
	}

	mdelay(150);
	rx_ctl = asix_read_rx_ctl(dev);
	debug("RX_CTL is 0x%04x after software reset\n", rx_ctl);
	if ((ret = asix_write_rx_ctl(dev, 0x0000)) < 0)
		goto out;

	rx_ctl = asix_read_rx_ctl(dev);
	debug("RX_CTL is 0x%04x setting to 0x0000\n", rx_ctl);

	dev->edev.get_ethaddr = asix_get_ethaddr;
	dev->edev.set_ethaddr = asix_set_ethaddr;
	asix_init_mii(dev);

	if ((ret = asix_sw_reset(dev, AX_SWRESET_PRL)) < 0)
		goto out;

	mdelay(150);

	if ((ret = asix_sw_reset(dev, AX_SWRESET_IPRL | AX_SWRESET_PRL)) < 0)
		goto out;

	mdelay(150);

	if ((ret = asix_write_medium_mode(dev, AX88772_MEDIUM_DEFAULT)) < 0)
		goto out;

	if ((ret = asix_write_cmd(dev, AX_CMD_WRITE_IPG0,
				AX88772_IPG0_DEFAULT | AX88772_IPG1_DEFAULT,
				AX88772_IPG2_DEFAULT, 0, NULL)) < 0) {
		debug("Write IPG,IPG1,IPG2 failed: %d\n", ret);
		goto out;
	}

	/* Set RX_CTL to default values with 2k buffer, and enable cactus */
	if ((ret = asix_write_rx_ctl(dev, AX_DEFAULT_RX_CTL)) < 0)
		goto out;

	rx_ctl = asix_read_rx_ctl(dev);
	debug("RX_CTL is 0x%04x after all initializations\n", rx_ctl);

	rx_ctl = asix_read_medium_status(dev);
	debug("Medium Status is 0x%04x after all initializations\n", rx_ctl);

	/* Asix framing packs multiple eth frames into a 2K usb bulk transfer */
	if (dev->driver_info->flags & FLAG_FRAMING_AX) {
		/* hard_mtu  is still the default - the device does not support
		   jumbo eth frames */
		dev->rx_urb_size = 2048;
	}

	dev->driver_priv = kzalloc(sizeof(struct asix_common_private),
				GFP_KERNEL);
	if (!dev->driver_priv)
		return -ENOMEM;

	return 0;

out:
	return ret;
}

static void asix_unbind(struct usbnet *dev)
{
	mdiobus_unregister(&dev->miibus);
	kfree(dev->driver_priv);
}

static struct driver_info ax8817x_info = {
	.description = "ASIX AX8817x USB 2.0 Ethernet",
	.bind = ax88172_bind,
	.unbind = asix_unbind,
	.link_reset = ax88172_link_reset,
	.reset = ax88172_link_reset,
	.flags =  FLAG_ETHER,
	.data = 0x00130103,
};

static struct driver_info dlink_dub_e100_info = {
	.description = "DLink DUB-E100 USB Ethernet",
	.bind = ax88172_bind,
	.unbind = asix_unbind,
	.link_reset = ax88172_link_reset,
	.reset = ax88172_link_reset,
	.flags =  FLAG_ETHER,
	.data = 0x009f9d9f,
};

static struct driver_info netgear_fa120_info = {
	.description = "Netgear FA-120 USB Ethernet",
	.bind = ax88172_bind,
	.unbind = asix_unbind,
	.link_reset = ax88172_link_reset,
	.reset = ax88172_link_reset,
	.flags =  FLAG_ETHER,
	.data = 0x00130103,
};

static struct driver_info hawking_uf200_info = {
	.description = "Hawking UF200 USB Ethernet",
	.bind = ax88172_bind,
	.unbind = asix_unbind,
	.link_reset = ax88172_link_reset,
	.reset = ax88172_link_reset,
	.flags =  FLAG_ETHER,
	.data = 0x001f1d1f,
};

static struct driver_info ax88772_info = {
	.description = "ASIX AX88772 USB 2.0 Ethernet",
	.bind = ax88772_bind,
	.unbind = asix_unbind,
	.flags = FLAG_ETHER | FLAG_FRAMING_AX,
	.rx_fixup = asix_rx_fixup_common,
	.tx_fixup = asix_tx_fixup,
};

static struct driver_info ax88772b_info = {
	.description = "ASIX AX88772B USB 2.0 Ethernet",
	.bind = ax88772_bind,
	.unbind = asix_unbind,
	.flags = FLAG_ETHER | FLAG_FRAMING_AX,
	.rx_fixup = asix_rx_fixup_common,
	.tx_fixup = asix_tx_fixup,
	.data = FLAG_EEPROM_MAC,
};

static const struct usb_device_id products [] = {
{
	// Linksys USB200M
	USB_DEVICE (0x077b, 0x2226),
	.driver_info =	&ax8817x_info,
}, {
	// Netgear FA120
	USB_DEVICE (0x0846, 0x1040),
	.driver_info =  &netgear_fa120_info,
}, {
	// DLink DUB-E100
	USB_DEVICE (0x2001, 0x1a00),
	.driver_info =  &dlink_dub_e100_info,
}, {
	// Intellinet, ST Lab USB Ethernet
	USB_DEVICE (0x0b95, 0x1720),
	.driver_info =  &ax8817x_info,
}, {
	// Hawking UF200, TrendNet TU2-ET100
	USB_DEVICE (0x07b8, 0x420a),
	.driver_info =  &hawking_uf200_info,
}, {
	// Billionton Systems, USB2AR
	USB_DEVICE (0x08dd, 0x90ff),
	.driver_info =  &ax8817x_info,
}, {
	// ATEN UC210T
	USB_DEVICE (0x0557, 0x2009),
	.driver_info =  &ax8817x_info,
}, {
	// Buffalo LUA-U2-KTX
	USB_DEVICE (0x0411, 0x003d),
	.driver_info =  &ax8817x_info,
}, {
	// Sitecom LN-029 "USB 2.0 10/100 Ethernet adapter"
	USB_DEVICE (0x6189, 0x182d),
	.driver_info =  &ax8817x_info,
}, {
	// corega FEther USB2-TX
	USB_DEVICE (0x07aa, 0x0017),
	.driver_info =  &ax8817x_info,
}, {
	// Surecom EP-1427X-2
	USB_DEVICE (0x1189, 0x0893),
	.driver_info = &ax8817x_info,
}, {
	// goodway corp usb gwusb2e
	USB_DEVICE (0x1631, 0x6200),
	.driver_info = &ax8817x_info,
}, {
	// JVC MP-PRX1 Port Replicator
	USB_DEVICE (0x04f1, 0x3008),
	.driver_info = &ax8817x_info,
}, {
	// ASIX AX88772 10/100
	USB_DEVICE (0x0b95, 0x7720),
	.driver_info = &ax88772_info,
}, {
	// Linksys USB200M Rev 2
	USB_DEVICE (0x13b1, 0x0018),
	.driver_info = &ax88772_info,
}, {
	// 0Q0 cable ethernet
	USB_DEVICE (0x1557, 0x7720),
	.driver_info = &ax88772_info,
}, {
	// DLink DUB-E100 H/W Ver B1
	USB_DEVICE (0x07d1, 0x3c05),
	.driver_info = &ax88772_info,
}, {
	// DLink DUB-E100 H/W Ver B1 Alternate
	USB_DEVICE (0x2001, 0x3c05),
	.driver_info = &ax88772_info,
}, {
	// Apple USB Ethernet Adapter
	USB_DEVICE(0x05ac, 0x1402),
	.driver_info = &ax88772_info,
}, {
	// Cables-to-Go USB Ethernet Adapter
	USB_DEVICE(0x0b95, 0x772a),
	.driver_info = &ax88772_info,
}, {
	// LevelOne USB Fast Ethernet Adapter
	USB_DEVICE(0x0b95, 0x772b),
	.driver_info = &ax88772b_info,
}, {
	// DLink DUB-E100 H/W Ver C1
	USB_DEVICE(0x2001, 0x1a02),
	.driver_info = &ax88772_info,
},
	{ },		// END
};

static struct usb_driver asix_driver = {
	.name =		"asix",
	.id_table =	products,
	.probe =	usbnet_probe,
	.disconnect =	usbnet_disconnect,
};

static int __init asix_init(void)
{
	return usb_driver_register(&asix_driver);
}
device_initcall(asix_init);
