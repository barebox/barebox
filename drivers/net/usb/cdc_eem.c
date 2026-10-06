// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-Comment: Origin-URL: https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/drivers/net/usb/cdc_eem.c?id=7fa2d1707d4102a5b3dcd3f49b235015f4d66955
/*
 * USB CDC EEM network interface driver
 * Copyright (C) 2009 Oberthur Technologies
 * by Omar Laazimani, Olivier Condemine
 */

#include <common.h>
#include <init.h>
#include <crc.h>
#include <net.h>
#include <linux/usb/usb.h>
#include <linux/usb/usbnet.h>
#include <linux/usb/cdc.h>
#include <asm/unaligned.h>

/*
 * This driver is an implementation of the CDC "Ethernet Emulation
 * Model" (EEM) specification, which encapsulates Ethernet frames
 * for transport over USB using a simpler USB device model than the
 * previous CDC "Ethernet Control Model" (ECM, or "CDC Ethernet").
 *
 * For details, see https://usb.org/sites/default/files/CDC_EEM10.pdf
 */

#define EEM_HEAD	2		/* 2 byte header */

/*-------------------------------------------------------------------------*/

static int eem_open(struct eth_device *edev)
{
	/* EEM has no PHY — link is always up when USB is connected */
	return 0;
}

static int eem_get_ethaddr(struct eth_device *edev, unsigned char *adr)
{
	/* EEM devices have no MAC address of their own */
	if (generate_ether_addr(adr, edev->dev.id))
		random_ether_addr(adr);

	return 0;
}

static int eem_set_ethaddr(struct eth_device *edev, const unsigned char *adr)
{
	return 0;
}

static void eem_linkcmd(struct usbnet *dev, const void *buf, u16 len)
{
	int alen;
	int n = EEM_HEAD + len;

	put_unaligned_le16(BIT(15) | BIT(11) | len, dev->tx_buf);
	memcpy(dev->tx_buf + EEM_HEAD, buf, len);

	/* a zero length EEM packet instead of a ZLP, as in eem_tx_fixup() */
	if (!(n % dev->maxpacket)) {
		put_unaligned_le16(0, dev->tx_buf + n);
		n += EEM_HEAD;
	}

	if (usb_bulk_msg(dev->udev, dev->out, dev->tx_buf, n, &alen, 1000))
		dev_warn(&dev->edev.dev, "link cmd failure\n");
}

static int eem_bind(struct usbnet *dev)
{
	int status;

	status = usbnet_get_endpoints(dev);
	if (status < 0)
		return status;

	/* no jumbogram (16K) support for now */

	/*
	 * Enough for an echo command with the maximum of 2047 bytes and
	 * for devices bundling several frames into one transfer. As a
	 * multiple of maxpacket, a longer transfer is split instead of
	 * failing with a babble error.
	 */
	dev->rx_urb_size = 4096;

	/* EEM has no PHY — override usbnet's default open handler */
	dev->edev.open = eem_open;
	dev->edev.get_ethaddr = eem_get_ethaddr;
	dev->edev.set_ethaddr = eem_set_ethaddr;

	return 0;
}

/*
 * EEM permits packing multiple Ethernet frames into USB transfers
 * (a "bundle"), but for TX we don't try to do that.
 */
static int eem_tx_fixup(struct usbnet *dev, void *buf, int len,
			void *nbuf, int *nlen)
{
	u32 crc;
	int padlen = 0;
	int pos = 0;
	u16 header;

	/*
	 * When ((len + EEM_HEAD + ETH_FCS_LEN) % dev->maxpacket) is
	 * zero, stick two bytes of zero length EEM packet on the end.
	 * Else the framework would add invalid single byte padding,
	 * since it can't know whether ZLPs will be handled right by
	 * all the relevant hardware and software.
	 */
	if (!((len + EEM_HEAD + ETH_FCS_LEN) % dev->maxpacket))
		padlen += 2;

	/*
	 * EEM packet header format:
	 * b0..13:	length of ethernet frame (including CRC)
	 * b14:		bmCRC (1 == valid Ethernet CRC)
	 * b15:		bmType (0 == data)
	 */
	header = BIT(14) | (len + ETH_FCS_LEN);
	put_unaligned_le16(header, nbuf + pos);
	pos += EEM_HEAD;

	/* Copy Ethernet frame */
	memcpy(nbuf + pos, buf, len);
	pos += len;

	/* We don't use the "no Ethernet CRC" option */
	crc = crc32(0, buf, len);
	put_unaligned_le32(crc, nbuf + pos);
	pos += ETH_FCS_LEN;

	/* Bundle a zero length EEM packet if needed */
	if (padlen) {
		put_unaligned_le16(0, nbuf + pos);
		pos += 2;
	}

	*nlen = pos;

	return 0;
}

static int eem_rx_fixup(struct usbnet *dev, void *buf, int len)
{
	int offset = 0;

	/*
	 * Our task here is to strip off EEM framing and deliver Ethernet
	 * frames to the network stack.  We may have received multiple EEM
	 * payloads, or command payloads.
	 */
	while (offset + EEM_HEAD <= len) {
		u16 header;
		u16 frame_len;

		/*
		 * EEM packet header format:
		 * b0..14:	EEM type dependent (Data or Command)
		 * b15:		bmType
		 */
		header = get_unaligned_le16(buf + offset);
		offset += EEM_HEAD;

		/*
		 * The bmType bit helps to denote when EEM
		 * packet is data or command:
		 *	bmType = 0	: EEM data payload
		 *	bmType = 1	: EEM (link) command
		 */
		if (header & BIT(15)) {
			/*
			 * EEM (link) command packet:
			 * b0..10:	bmEEMCmdParam
			 * b11..13:	bmEEMCmd
			 * b14:		bmReserved (must be 0)
			 * b15:		1 (EEM command)
			 */
			u16 bmEEMCmd;

			if (header & BIT(14)) {
				dev_dbg(&dev->edev.dev,
					"reserved command %04x\n", header);
				continue;
			}

			bmEEMCmd = (header >> 11) & 0x7;
			switch (bmEEMCmd) {
			/* Responding to echo requests is mandatory. */
			case 0:		/* Echo command */
				frame_len = header & 0x7FF;
				if (len - offset < frame_len)
					return 0;
				eem_linkcmd(dev, buf + offset, frame_len);
				offset += frame_len;
				break;

			case 2:		/* Suspend hint */
			case 3:		/* Response hint */
			case 4:		/* Response complete hint */
				/* Host may choose to ignore hints */
				continue;

			case 1:		/* Echo response */
			case 5:		/* Tickle */
			default:	/* reserved */
				dev_warn(&dev->edev.dev,
					 "unexpected link command %d\n",
					 bmEEMCmd);
				if (bmEEMCmd != 1)
					continue;

				/* skip the echoed data, it's not EEM packets */
				frame_len = header & 0x7FF;
				if (len - offset < frame_len)
					return 0;
				offset += frame_len;
				break;
			}
		} else {
			u32 crc, crc2;

			/* zero length EEM packet? */
			if (header == 0)
				continue;

			/*
			 * EEM data packet header:
			 * b0..13:	length of ethernet frame (with CRC)
			 * b14:		bmCRC
			 * b15:		0 (EEM data)
			 */
			frame_len = header & 0x3FFF;

			/* bogus EEM payload? */
			if (len - offset < frame_len)
				return 0;

			/* bogus ethernet frame? */
			if (frame_len < (ETH_HLEN + ETH_FCS_LEN) ||
			    frame_len - ETH_FCS_LEN > PKTSIZE) {
				offset += frame_len;
				continue;
			}

			/*
			 * The bmCRC helps to denote when the CRC field in
			 * the Ethernet frame contains a calculated CRC:
			 *	bmCRC = 1	: CRC is calculated
			 *	bmCRC = 0	: CRC = 0xDEADBEEF
			 */
			if (header & BIT(14)) {
				crc = get_unaligned_le32(buf + offset +
							 frame_len -
							 ETH_FCS_LEN);
				crc2 = crc32(0, buf + offset,
					     frame_len - ETH_FCS_LEN);
			} else {
				crc = get_unaligned_be32(buf + offset +
							 frame_len -
							 ETH_FCS_LEN);
				crc2 = 0xdeadbeef;
			}

			if (crc == crc2) {
				net_receive(&dev->edev, buf + offset,
					    frame_len - ETH_FCS_LEN);
			} else {
				dev_dbg(&dev->edev.dev, "rx crc error\n");
			}

			offset += frame_len;
		}
	}

	return 0;
}

static struct driver_info eem_info = {
	.description =	"CDC EEM Device",
	.flags =	FLAG_ETHER,
	.bind =		eem_bind,
	.rx_fixup =	eem_rx_fixup,
	.tx_fixup =	eem_tx_fixup,
};

/*-------------------------------------------------------------------------*/

static const struct usb_device_id products[] = {
{
	USB_INTERFACE_INFO(USB_CLASS_COMM, USB_CDC_SUBCLASS_EEM,
			USB_CDC_PROTO_EEM),
	.driver_info = &eem_info,
},
{
	/* EMPTY == end of list */
},
};

static struct usb_driver eem_driver = {
	.name =		"cdc_eem",
	.id_table =	products,
	.probe =	usbnet_probe,
	.disconnect =	usbnet_disconnect,
};

static int __init eem_init(void)
{
	return usb_driver_register(&eem_driver);
}
device_initcall(eem_init);

MODULE_AUTHOR("Omar Laazimani <omar.oberthur@gmail.com>");
MODULE_DESCRIPTION("USB CDC EEM");
MODULE_LICENSE("GPL");
