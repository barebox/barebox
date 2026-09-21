// SPDX-License-Identifier: GPL-2.0-only
#include <efi/payload.h>
#include <efi/payload/init.h>
#include <efi/payload/driver.h>
#include <efi/protocol/usb.h>
#include <efi/error.h>
#include <linux/usb/usb.h>

#include "../core/usb.h"

struct efi_usb_io_priv {
	struct efi_usb_io_protocol *protocol;
	struct device *dev;
	struct usb_host host;
};

#define usb_dev_to_efi_priv(ptr) \
	container_of(ptr->host, struct efi_usb_io_priv, host)

static int efi_usb_error_check(efi_status_t efiret, int status)
{
	if (efiret == EFI_DEVICE_ERROR) {
		if (status & EFI_USB_ERR_TIMEOUT)
			return -ETIMEDOUT;
		if (status & EFI_USB_ERR_STALL)
			return -EPIPE;
		if (status & EFI_USB_ERR_NAK)
			return -EPROTO;
		if (status & EFI_USB_ERR_BUFFER)
			return -EINVAL;
		if (status & EFI_USB_ERR_NOTEXECUTE)
			return -EIO;
		if (status & EFI_USB_ERR_BABBLE)
			return -EIO;
		if (status & EFI_USB_ERR_CRC)
			return -EIO;
		if (status & EFI_USB_ERR_BITSTUFF)
			return -EIO;
		if (status & EFI_USB_ERR_SYSTEM)
			return -EIO;
	}

	if (EFI_ERROR(efiret))
		return -efi_errno(efiret);

	return 0;
}

static int efi_usb_control_msg(struct usb_device *dev, unsigned long pipe,
			       void *buffer, int length,
			       struct devrequest *setup, int timeout)
{
	struct efi_usb_io_priv *priv = usb_dev_to_efi_priv(dev);
	enum efi_usb_data_direction direction;
	efi_status_t efiret;
	efi_uintn_t efi_timeout = timeout;
	u32 status;

	if (usb_pipein(pipe))
		direction = EFI_USB_DATA_IN;
	else
		direction = EFI_USB_DATA_OUT;
	if (length == 0)
		direction = EFI_USB_NO_DATA;

	efiret = priv->protocol->control_transfer(priv->protocol, setup,
						  direction, efi_timeout,
						  buffer, length, &status);

	dev->status = status; /* dev-status is a long, status is u32 */
	dev->act_len = length;

	return efi_usb_error_check(efiret, status);
}

static int efi_usb_bulk_msg(struct usb_device *dev, unsigned long pipe,
			    void *buffer, int length, int timeout)
{
	struct efi_usb_io_priv *priv = usb_dev_to_efi_priv(dev);
	efi_uintn_t efi_length = length;
	efi_status_t efiret;
	u32 status;

	u8 epnum = usb_pipeendpoint(pipe) | (usb_pipein(pipe) << 7);

	efiret = priv->protocol->bulk_transfer(priv->protocol, epnum, buffer,
					       &efi_length, timeout, &status);

	dev->status = status;
	dev->act_len = efi_length;

	return efi_usb_error_check(efiret, status);
}

static int efi_usb_int_msg(struct usb_device *dev, unsigned long pipe,
			   void *buffer, int length,
			   int __always_unused interval)
{
	struct efi_usb_io_priv *priv = usb_dev_to_efi_priv(dev);
	efi_status_t efiret;
	u32 status;
	efi_uintn_t efi_length = length;

	u8 epnum = usb_pipeendpoint(pipe) | (usb_pipein(pipe) << 7);

	efiret = priv->protocol->sync_interrupt_transfer(
		priv->protocol, epnum, buffer, &efi_length, 100, &status);

	dev->status = status;
	dev->act_len = efi_length;

	return efi_usb_error_check(efiret, status);
}

static int efi_get_usb_string(struct efi_usb_io_protocol *protocol, u16 lang_id,
			      int index, char *buf, size_t size)
{
	wchar_t *efi_name;
	efi_status_t efiret;
	unsigned int u, idx;

	memset(buf, 0, size);

	if (!index)
		return 0;

	efiret = protocol->get_string_descriptor(protocol, lang_id, index,
						 &efi_name);
	if (EFI_ERROR(efiret))
		return -efi_errno(efiret);

	size--; /* leave room for trailing NULL char in output buffer */
	for (idx = 0, u = 0;; u++) {
		if (idx >= size)
			break;
		if (efi_name[u] & 0xff00) /* high byte */
			buf[idx++] = '?'; /* non-ASCII character */
		else if (efi_name[u])
			buf[idx++] = efi_name[u] & 0xff;
		else
			break;
	}
	buf[idx] = 0;
	BS->free_pool(efi_name);

	return 0;
}

static void print_usb_device_info(struct device *dev)
{
	struct usb_device *usb_dev = container_of(dev, struct usb_device, dev);

	dev_info(dev, "Bus %03d Device %03d: ID %04x:%04x %s\n",
		 usb_dev->host->busnum, usb_dev->devnum,
		 usb_dev->descriptor->idVendor, usb_dev->descriptor->idProduct,
		 usb_dev->prod);
}

static int create_usb_device(struct efi_usb_io_priv *priv)
{
	struct usb_host *host = &priv->host;
	struct usb_device *dev;
	efi_status_t efiret;
	struct usb_interface *interface;
	int err;
	u16 *lang_ids;
	u16 table_size;

	dev = usb_alloc_new_device();
	dev->host = host;

	dev_set_name(&dev->dev, "usb%d", dev->host->busnum);
	dev->dev.id = DEVICE_ID_SINGLE;

	efiret = priv->protocol->get_device_descriptor(priv->protocol,
						       dev->descriptor);
	if (EFI_ERROR(efiret)) {
		err = -efi_errno(efiret);
		goto out_err;
	}

	switch (dev->descriptor->bMaxPacketSize0) {
	case 8:
		dev->maxpacketsize = PACKET_SIZE_8;
		break;
	case 16:
		dev->maxpacketsize = PACKET_SIZE_16;
		break;
	case 32:
		dev->maxpacketsize = PACKET_SIZE_32;
		break;
	case 64:
		dev->maxpacketsize = PACKET_SIZE_64;
		break;
	}

	/* There is only the possibility to access the current active configuration
	 * and not set the configuration.
	 * https://uefi.org/specs/UEFI/2.11/17_Protocols_USB_Support.html#efi-usb-io-protocol-usbgetconfigdescriptor */
	efiret = priv->protocol->get_config_descriptor(priv->protocol,
						       &dev->config.desc);
	if (EFI_ERROR(efiret)) {
		err = -efi_errno(efiret);
		goto out_err;
	}

	/* UEFI has by definition only one interface per config
	 * https://uefi.org/specs/UEFI/2.11/17_Protocols_USB_Support.html#efi-usb-io-protocol-usbgetinterfacedescriptor */
	dev->config.no_of_if = 1;
	interface = &dev->config.interface[0];

	efiret = priv->protocol->get_interface_descriptor(priv->protocol,
							  &interface->desc);
	if (EFI_ERROR(efiret)) {
		err = -efi_errno(efiret);
		goto out_err;
	}

	interface->no_of_ep = interface->desc.bNumEndpoints;

	if (interface->no_of_ep > USB_MAXENDPOINTS) {
		err = -EPROTO;
		goto out_err;
	}

	for (int i = 0; i < interface->no_of_ep; i++) {
		efiret = priv->protocol->get_endpoint_descriptor(
			priv->protocol, i, &interface->ep_desc[i]);
		if (EFI_ERROR(efiret)) {
			err = -efi_errno(efiret);
			goto out_err;
		}
		usb_set_maxpacket_ep(dev, &interface->ep_desc[i]);
	}

	efiret = priv->protocol->get_supported_languages(
		priv->protocol, &lang_ids, &table_size);
	if (EFI_ERROR(efiret)) {
		err = -efi_errno(efiret);
		goto out_err;
	}

	/* table size is given in bytes, not entries */
	dev->have_langid = table_size >= sizeof(*lang_ids);

	dev_info(&dev->dev, "new device: Mfr=%d, Product=%d, SerialNumber=%d\n",
		 dev->descriptor->iManufacturer, dev->descriptor->iProduct,
		 dev->descriptor->iSerialNumber);

	if (dev->have_langid) {
		dev->string_langid = lang_ids[0];
		efi_get_usb_string(priv->protocol, dev->string_langid,
				   dev->descriptor->iManufacturer, dev->mf,
				   sizeof(dev->mf));
		efi_get_usb_string(priv->protocol, dev->string_langid,
				   dev->descriptor->iProduct, dev->prod,
				   sizeof(dev->prod));
		efi_get_usb_string(priv->protocol, dev->string_langid,
				   dev->descriptor->iSerialNumber, dev->serial,
				   sizeof(dev->serial));
	}

	print_usb_device_info(&dev->dev);
	devinfo_add(&dev->dev, print_usb_device_info);

	err = register_device(&dev->dev);
	if (err) {
		dev_err(&dev->dev, "Failed to register device: %pe\n",
			ERR_PTR(err));
		goto out_err;
	}

	/* register as root device for host */
	host->root_dev = dev;

	usb_add_device(dev);

	return 0;

out_err:
	dev_err(&dev->dev, "Failed to create UEFI-USB-IO device: %pe\n",
		ERR_PTR(err));
	usb_free_device(dev);
	return err;
}

static int efi_usb_io_probe(struct efi_device *efidev)
{
	struct device *dev = &efidev->dev;
	struct efi_usb_io_priv *priv;
	struct usb_host *host;

	priv = xzalloc(sizeof(*priv));

	BS->handle_protocol(efidev->handle, &efi_usb_io_protocol_guid,
			    (void **)&priv->protocol);
	if (!priv->protocol)
		return -ENODEV;

	dev->priv = priv;
	priv->dev = dev;

	host->hw_dev = dev;

	/* EFI has one device per probe, which now needs its own host controller,
	 * since there is no shared host controller resource. */

	host = &priv->host;
	host->submit_int_msg = efi_usb_int_msg;
	host->submit_control_msg = efi_usb_control_msg;
	host->submit_bulk_msg = efi_usb_bulk_msg;
	usb_register_host(host);

	return create_usb_device(priv);
}

static struct efi_driver efi_usb_io_driver = {
	.driver = {
		.name  = "efi-usb-io-protocol",
	},
	.probe = efi_usb_io_probe,
	.guid = EFI_USB_IO_PROTOCOL_GUID,
};
device_efi_driver(efi_usb_io_driver);
