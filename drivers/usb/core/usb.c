/*
 *
 * Most of this source has been derived from the Linux USB
 * project:
 * (C) Copyright Linus Torvalds 1999
 * (C) Copyright Johannes Erdfelt 1999-2001
 * (C) Copyright Andreas Gal 1999
 * (C) Copyright Gregory P. Smith 1999
 * (C) Copyright Deti Fliegl 1999 (new USB architecture)
 * (C) Copyright Randy Dunlap 2000
 * (C) Copyright David Brownell 2000 (kernel hotplug, usb_device_id)
 * (C) Copyright Yggdrasil Computing, Inc. 2000
 *     (usb_device_id matching changes by Adam J. Richter)
 *
 * Adapted for barebox:
 * (C) Copyright 2001 Denis Peter, MPL AG Switzerland
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *
 */

/*
 * How it works:
 *
 * Since this is a bootloader, the devices will not be automatic
 * (re)configured on hotplug, but after a restart of the USB the
 * device should work.
 *
 * For each transfer (except "Interrupt") we wait for completion.
 */

#define pr_fmt(fmt) "usb: " fmt

#include <common.h>
#include <command.h>
#include <malloc.h>
#include <driver.h>
#include <linux/ctype.h>
#include <asm/byteorder.h>
#include <xfuncs.h>
#include <init.h>
#include <dma.h>

#include <usb/usb.h>
#include <usb/ch9.h>

#include "usb.h"
#include "hub.h"

#define USB_BUFSIZ	512

static int dev_count;
static int dev_index;
static int asynch_allowed;

static LIST_HEAD(host_list);
LIST_HEAD(usb_device_list);

static void print_usb_device(struct usb_device *dev)
{
	pr_info("Bus %03d Device %03d: ID %04x:%04x %s\n",
		dev->host->busnum, dev->devnum,
		dev->descriptor->idVendor,
		dev->descriptor->idProduct,
		dev->prod);
}

static int host_busnum = 1;

static inline int usb_host_acquire(struct usb_host *host)
{
	if (host->sem)
		return -EAGAIN;
	host->sem++;
	return 0;
}

static inline void usb_host_release(struct usb_host *host)
{
	if (host->sem > 0)
		host->sem--;
}

int usb_register_host(struct usb_host *host)
{
	list_add_tail(&host->list, &host_list);
	host->busnum = host_busnum++;
	host->sem = 0;
	asynch_allowed = 1;
	return 0;
}

/**
 * set configuration number to configuration
 */
static int usb_set_configuration(struct usb_device *dev, int configuration)
{
	int res;

	pr_debug("set configuration %d\n", configuration);

	/* set setup command */
	res = usb_control_msg(dev, usb_sndctrlpipe(dev, 0),
				USB_REQ_SET_CONFIGURATION, 0,
				configuration, 0,
				NULL, 0, USB_CNTL_TIMEOUT);
	if (res == 0) {
		dev->toggle[0] = 0;
		dev->toggle[1] = 0;
		return 0;
	} else
		return -1;
}

/* The routine usb_set_maxpacket_ep() is extracted from the loop of routine
 * usb_set_maxpacket(), because the optimizer of GCC 4.x chokes on this routine
 * when it is inlined in 1 single routine. What happens is that the register r3
 * is used as loop-count 'i', but gets overwritten later on.
 * This is clearly a compiler bug, but it is easier to workaround it here than
 * to update the compiler (Occurs with at least several GCC 4.{1,2},x
 * CodeSourcery compilers like e.g. 2007q3, 2008q1, 2008q3 lite editions on ARM)
 */
static void  noinline
usb_set_maxpacket_ep(struct usb_device *dev, struct usb_endpoint_descriptor *ep)
{
	int b;

	b = ep->bEndpointAddress & USB_ENDPOINT_NUMBER_MASK;

	if ((ep->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) ==
						USB_ENDPOINT_XFER_CONTROL) {
		/* Control => bidirectional */
		dev->epmaxpacketout[b] = ep->wMaxPacketSize;
		dev->epmaxpacketin[b] = ep->wMaxPacketSize;
		pr_debug("##Control EP epmaxpacketout/in[%d] = %d\n",
			   b, dev->epmaxpacketin[b]);
	} else {
		if ((ep->bEndpointAddress & 0x80) == 0) {
			/* OUT Endpoint */
			if (ep->wMaxPacketSize > dev->epmaxpacketout[b]) {
				dev->epmaxpacketout[b] = ep->wMaxPacketSize;
				pr_debug("##EP epmaxpacketout[%d] = %d\n",
					   b, dev->epmaxpacketout[b]);
			}
		} else {
			/* IN Endpoint */
			if (ep->wMaxPacketSize > dev->epmaxpacketin[b]) {
				dev->epmaxpacketin[b] = ep->wMaxPacketSize;
				pr_debug("##EP epmaxpacketin[%d] = %d\n",
					   b, dev->epmaxpacketin[b]);
			}
		} /* if out */
	} /* if control */
}

/*
 * set the max packed value of all endpoints in the given configuration
 */
static int usb_set_maxpacket(struct usb_device *dev)
{
	int i, ii;

	for (i = 0; i < dev->config.desc.bNumInterfaces; i++)
		for (ii = 0; ii < dev->config.interface[i].desc.bNumEndpoints; ii++)
			usb_set_maxpacket_ep(dev,
					  &dev->config.interface[i].ep_desc[ii]);

	return 0;
}

/**
 * Parse the config, located in buffer, and fills the dev->config structure.
 * Note that all little/big endian swapping are done automatically.
 */
static int usb_parse_config(struct usb_device *dev, unsigned char *buffer, int cfgno)
{
	struct usb_descriptor_header *head;
	int index, ifno, epno, curr_if_num;
	int i;
	unsigned char *ch;

	ifno = -1;
	epno = -1;
	curr_if_num = -1;

	dev->configno = cfgno;
	head = (struct usb_descriptor_header *) &buffer[0];
	if (head->bDescriptorType != USB_DT_CONFIG) {
		printf(" ERROR: NOT USB_CONFIG_DESC %x\n",
			head->bDescriptorType);
		return -1;
	}
	memcpy(&dev->config, buffer, buffer[0]);
	le16_to_cpus(&(dev->config.desc.wTotalLength));
	dev->config.no_of_if = 0;

	index = dev->config.desc.bLength;
	/* Ok the first entry must be a configuration entry,
	 * now process the others */
	head = (struct usb_descriptor_header *) &buffer[index];
	while (index + 1 < dev->config.desc.wTotalLength) {
		switch (head->bDescriptorType) {
		case USB_DT_INTERFACE:
			if (((struct usb_interface_descriptor *) \
			     &buffer[index])->bInterfaceNumber != curr_if_num) {
				/* this is a new interface, copy new desc */
				ifno = dev->config.no_of_if;
				/* if ifno > USB_MAXINTERFACES, then
				 * next memcpy() will corrupt dev->config
				 */
				if (ifno > USB_MAXINTERFACES) {
					printf("ifno = %d > "
						"USB_MAXINTERFACES = %d !\n",
						ifno,
						USB_MAXINTERFACES);
					break;
				}
				dev->config.no_of_if++;
				memcpy(&dev->config.interface[ifno].desc,
					&buffer[index], buffer[index]);
				dev->config.interface[ifno].no_of_ep = 0;
				dev->config.interface[ifno].num_altsetting = 1;
				curr_if_num =
				     dev->config.interface[ifno].desc.bInterfaceNumber;
			} else {
				/* found alternate setting for the interface */
				dev->config.interface[ifno].num_altsetting++;
			}
			break;
		case USB_DT_ENDPOINT:
			epno = dev->config.interface[ifno].no_of_ep;
			/* found an endpoint */
			dev->config.interface[ifno].no_of_ep++;
			memcpy(&dev->config.interface[ifno].ep_desc[epno],
				&buffer[index], buffer[index]);
			le16_to_cpus(&(dev->config.interface[ifno].ep_desc[epno].\
							       wMaxPacketSize));
			pr_debug("if %d, ep %d\n", ifno, epno);
			break;
		default:
			if (head->bLength == 0)
				return 1;

			pr_debug("unknown Description Type : %x\n",
				   head->bDescriptorType);

			{
				ch = (unsigned char *)head;
				for (i = 0; i < head->bLength; i++)
					pr_debug("%02X ", *ch++);
				pr_debug("\n\n\n");
			}
			break;
		}
		index += head->bLength;
		head = (struct usb_descriptor_header *)&buffer[index];
	}
	return 1;
}

/**
 * set address of a device to the value in dev->devnum.
 * This can only be done by addressing the device via the default address (0)
 */
static int usb_set_address(struct usb_device *dev)
{
	int res;

	pr_debug("set address %d\n", dev->devnum);

	res = usb_control_msg(dev, usb_snddefctrl(dev),
				USB_REQ_SET_ADDRESS, 0,
				(dev->devnum), 0,
				NULL, 0, USB_CNTL_TIMEOUT);
	return res;
}

static int usb_get_descriptor(struct usb_device *dev, unsigned char type,
			unsigned char index, void *buf, int size)
{
	int res;
	res = usb_control_msg(dev, usb_rcvctrlpipe(dev, 0),
			USB_REQ_GET_DESCRIPTOR, USB_DIR_IN,
			(type << 8) + index, 0,
			buf, size, USB_CNTL_TIMEOUT);
	return res;
}

/*
 * By the time we get here, the device has gotten a new device ID
 * and is in the default state. We need to identify the thing and
 * get the ball rolling..
 *
 * Returns 0 for success, != 0 for error.
 */
int usb_new_device(struct usb_device *dev)
{
	int addr, err;
	int tmp;
	void *buf;
	struct usb_device_descriptor *desc;
	struct usb_device *parent = dev->parent;
	unsigned short portstatus;
	char str[16];

	buf = dma_alloc(USB_BUFSIZ);

	/* We still haven't set the Address yet */
	addr = dev->devnum;
	dev->devnum = 0;

	/* This is a Windows scheme of initialization sequence, with double
	 * reset of the device (Linux uses the same sequence)
	 * Some equipment is said to work only with such init sequence; this
	 * patch is based on the work by Alan Stern:
	 * http://sourceforge.net/mailarchive/forum.php?
	 * thread_id=5729457&forum_id=5398
	 */

	/* send 64-byte GET-DEVICE-DESCRIPTOR request.  Since the descriptor is
	 * only 18 bytes long, this will terminate with a short packet.  But if
	 * the maxpacket size is 8 or 16 the device may be waiting to transmit
	 * some more, or keeps on retransmitting the 8 byte header. */

	desc = buf;
	dev->descriptor->bMaxPacketSize0 = 64;	    /* Start off at 64 bytes  */
	/* Default to 64 byte max packet size */
	dev->maxpacketsize = PACKET_SIZE_64;
	dev->epmaxpacketin[0] = 64;
	dev->epmaxpacketout[0] = 64;

	err = usb_get_descriptor(dev, USB_DT_DEVICE, 0, desc, 64);
	if (err < 0) {
		pr_debug("%s: usb_get_descriptor() failed with %d\n", __func__, err);
		goto err_out;
	}

	dev->descriptor->bMaxPacketSize0 = desc->bMaxPacketSize0;

	/* find the port number we're at */
	if (parent) {
		/* reset the port for the second time */
		err = hub_port_reset(dev->parent, dev->portnr - 1, &portstatus);
		if (err < 0) {
			printf("\n     Couldn't reset port %i\n", dev->portnr);
			goto err_out;
		}
	}

	dev->epmaxpacketin[0] = dev->descriptor->bMaxPacketSize0;
	dev->epmaxpacketout[0] = dev->descriptor->bMaxPacketSize0;
	switch (dev->descriptor->bMaxPacketSize0) {
	case 8:
		dev->maxpacketsize  = PACKET_SIZE_8;
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
	dev->devnum = addr;

	err = usb_set_address(dev); /* set address */

	if (err < 0) {
		printf("\n      USB device not accepting new address " \
			"(error=%lX)\n", dev->status);
		goto err_out;
	}

	mdelay(10);	/* Let the SET_ADDRESS settle */

	tmp = sizeof(*dev->descriptor);

	err = usb_get_descriptor(dev, USB_DT_DEVICE, 0,
				 dev->descriptor, sizeof(*dev->descriptor));
	if (err < tmp) {
		if (err < 0)
			printf("unable to get device descriptor (error=%d)\n",
			       err);
		else
			printf("USB device descriptor short read " \
				"(expected %i, got %i)\n", tmp, err);
		goto err_out;
	}
	/* correct le values */
	le16_to_cpus(&dev->descriptor->bcdUSB);
	le16_to_cpus(&dev->descriptor->idVendor);
	le16_to_cpus(&dev->descriptor->idProduct);
	le16_to_cpus(&dev->descriptor->bcdDevice);
	/* only support for one config for now */
	usb_get_configuration_no(dev, buf, 0);
	usb_parse_config(dev, buf, 0);
	usb_set_maxpacket(dev);
	/* we set the default configuration here */
	if (usb_set_configuration(dev, dev->config.desc.bConfigurationValue)) {
		printf("failed to set default configuration " \
			"len %d, status %lX\n", dev->act_len, dev->status);
		goto err_out;
	}
	pr_debug("new device: Mfr=%d, Product=%d, SerialNumber=%d\n",
		   dev->descriptor->iManufacturer, dev->descriptor->iProduct,
		   dev->descriptor->iSerialNumber);
	memset(dev->mf, 0, sizeof(dev->mf));
	memset(dev->prod, 0, sizeof(dev->prod));
	memset(dev->serial, 0, sizeof(dev->serial));
	if (dev->descriptor->iManufacturer)
		usb_string(dev, dev->descriptor->iManufacturer,
			   dev->mf, sizeof(dev->mf));
	if (dev->descriptor->iProduct)
		usb_string(dev, dev->descriptor->iProduct,
			   dev->prod, sizeof(dev->prod));
	if (dev->descriptor->iSerialNumber)
		usb_string(dev, dev->descriptor->iSerialNumber,
			   dev->serial, sizeof(dev->serial));

	if (parent) {
		sprintf(dev->dev.name, "%s-%d", parent->dev.name, dev->portnr - 1);
	} else {
		sprintf(dev->dev.name, "usb%d", dev->host->busnum);
	}

	dev->dev.id = DEVICE_ID_SINGLE;

	print_usb_device(dev);

	err = register_device(&dev->dev);
	if (err) {
		printf("Failed to register device: %s\n", strerror(-err));
		goto err_out;
	}

	dev_add_param_int_ro(&dev->dev, "iManufacturer",
			dev->descriptor->iManufacturer, "%d");
	dev_add_param_int_ro(&dev->dev, "iProduct",
			dev->descriptor->iProduct, "%d");
	dev_add_param_int_ro(&dev->dev, "iSerialNumber",
			dev->descriptor->iSerialNumber, "%d");
	dev_add_param_fixed(&dev->dev, "iSerialNumber", str);
	dev_add_param_fixed(&dev->dev, "Manufacturer", dev->mf);
	dev_add_param_fixed(&dev->dev, "Product", dev->prod);
	dev_add_param_fixed(&dev->dev, "SerialNumber", dev->serial);
	dev_add_param_int_ro(&dev->dev, "idVendor",
			dev->descriptor->idVendor, "%04x");
	dev_add_param_int_ro(&dev->dev, "idProduct",
			dev->descriptor->idProduct, "%04x");
	list_add_tail(&dev->list, &usb_device_list);
	dev_count++;

	err = 0;

err_out:
	dma_free(buf);
	return err;
}

void usb_free_device(struct usb_device *usbdev)
{
	dma_free(usbdev->descriptor);
	dma_free(usbdev->setup_packet);
	free(usbdev);
}

void usb_remove_device(struct usb_device *usbdev)
{
	int i;

	if (!usbdev)
		return;

	for (i = 0; i < usbdev->maxchild; i++)
		usb_remove_device(usbdev->children[i]);
	if (usbdev->parent && usbdev->portnr)
		usbdev->parent->children[usbdev->portnr - 1] = NULL;
	list_del(&usbdev->list);
	dev_count--;

	if (unregister_device(&usbdev->dev))
		dev_err(&usbdev->dev, "failed to unregister\n");
	else
		dev_info(&usbdev->dev, "removed\n");

	usb_free_device(usbdev);
}

struct usb_device *usb_alloc_new_device(void)
{
	struct usb_device *usbdev = xzalloc(sizeof (*usbdev));

	usbdev->devnum = ++dev_index;
	usbdev->maxchild = 0;
	usbdev->dev.bus = &usb_bus_type;
	usbdev->setup_packet = dma_alloc(sizeof(*usbdev->setup_packet));
	usbdev->descriptor = dma_alloc(sizeof(*usbdev->descriptor));

	return usbdev;
}

int usb_host_detect(struct usb_host *host)
{
	int ret;

	if (!host->root_dev) {
		ret = host->init(host);
		if (ret)
			return ret;

		host->root_dev = usb_alloc_new_device();
		host->root_dev->dev.parent = host->hw_dev;
		host->root_dev->host = host;

		ret = usb_new_device(host->root_dev);
		if (ret) {
			usb_free_device(host->root_dev);
			return ret;
		}
	}

	device_detect(&host->root_dev->dev);

	return 0;
}

void usb_rescan(void)
{
	struct usb_host *host;
	int ret;

	pr_info("USB: scanning bus for devices...\n");

	list_for_each_entry(host, &host_list, list) {
		ret = usb_host_detect(host);
		if (ret)
			continue;
	}

	pr_info("%d USB Device(s) found\n", dev_count);
}

/*
 * disables the asynch behaviour of the control message. This is used for data
 * transfers that uses the exclusiv access to the control and bulk messages.
 */
void usb_disable_asynch(int disable)
{
	asynch_allowed = !disable;
}


/*-------------------------------------------------------------------
 * Message wrappers.
 *
 */

/*
 * submits an Interrupt Message
 */
int usb_submit_int_msg(struct usb_device *dev, unsigned long pipe,
			void *buffer, int transfer_len, int interval)
{
	struct usb_host *host = dev->host;
	int ret;

	ret = usb_host_acquire(host);
	if (ret)
		return ret;

	ret = host->submit_int_msg(dev, pipe, buffer, transfer_len, interval);

	usb_host_release(host);

	return ret;
}

/*
 * submits a control message and waits for completion (at least timeout * 1ms)
 * If timeout is 0, we don't wait for completion (used as example to set and
 * clear keyboards LEDs). For data transfers, (storage transfers) we don't
 * allow control messages with 0 timeout, by previously resetting the flag
 * asynch_allowed (usb_disable_asynch(1)).
 * returns the transfered length if OK or -1 if error. The transfered length
 * and the current status are stored in the dev->act_len and dev->status.
 */
int usb_control_msg(struct usb_device *dev, unsigned int pipe,
			unsigned char request, unsigned char requesttype,
			unsigned short value, unsigned short index,
			void *data, unsigned short size, int timeout)
{
	struct usb_host *host = dev->host;
	int ret;
	struct devrequest *setup_packet = dev->setup_packet;

	if ((timeout == 0) && (!asynch_allowed)) {
		/* request for a asynch control pipe is not allowed */
		return -1;
	}

	ret = usb_host_acquire(host);
	if (ret)
		return ret;

	/* set setup command */
	setup_packet->requesttype = requesttype;
	setup_packet->request = request;
	setup_packet->value = cpu_to_le16(value);
	setup_packet->index = cpu_to_le16(index);
	setup_packet->length = cpu_to_le16(size);
	pr_debug("usb_control_msg: request: 0x%X, requesttype: 0x%X, " \
		   "value 0x%X index 0x%X length 0x%X\n",
		   request, requesttype, value, index, size);
	dev->status = USB_ST_NOT_PROC; /*not yet processed */

	ret = host->submit_control_msg(dev, pipe, data, size, setup_packet,
			timeout);

	usb_host_release(host);

	if (ret)
		return ret;

	return dev->act_len;
}

/*-------------------------------------------------------------------
 * submits bulk message, and waits for completion. returns 0 if Ok or
 * -1 if Error.
 * synchronous behavior
 */
int usb_bulk_msg(struct usb_device *dev, unsigned int pipe,
			void *data, int len, int *actual_length, int timeout)
{
	struct usb_host *host = dev->host;
	int ret;

	if (len < 0)
		return -1;

	ret = usb_host_acquire(host);
	if (ret)
		return ret;

	dev->status = USB_ST_NOT_PROC; /* not yet processed */
	ret = host->submit_bulk_msg(dev, pipe, data, len, timeout);

	usb_host_release(host);

	if (ret)
		return ret;

	*actual_length = dev->act_len;

	return (dev->status == 0) ? 0 : -1;
}


/*-------------------------------------------------------------------
 * Max Packet stuff
 */

/*
 * returns the max packet size, depending on the pipe direction and
 * the configurations values
 */
int usb_maxpacket(struct usb_device *dev, unsigned long pipe)
{
	/* direction is out -> use emaxpacket out */
	if ((pipe & USB_DIR_IN) == 0)
		return dev->epmaxpacketout[((pipe>>15) & 0xf)];
	else
		return dev->epmaxpacketin[((pipe>>15) & 0xf)];
}

/***********************************************************************
 * Clears an endpoint
 * endp: endpoint number in bits 0-3;
 * direction flag in bit 7 (1 = IN, 0 = OUT)
 */
int usb_clear_halt(struct usb_device *dev, int pipe)
{
	int result;
	int endp = usb_pipeendpoint(pipe)|(usb_pipein(pipe)<<7);

	result = usb_control_msg(dev, usb_sndctrlpipe(dev, 0),
				 USB_REQ_CLEAR_FEATURE, USB_RECIP_ENDPOINT, 0,
				 endp, NULL, 0, USB_CNTL_TIMEOUT);

	/* don't clear if failed */
	if (result < 0)
		return result;

	/*
	 * NOTE: we do not get status and verify reset was successful
	 * as some devices are reported to lock up upon this check..
	 */

	usb_endpoint_running(dev, usb_pipeendpoint(pipe), usb_pipeout(pipe));

	/* toggle is reset on clear */
	usb_settoggle(dev, usb_pipeendpoint(pipe), usb_pipeout(pipe), 0);
	return 0;
}

/**********************************************************************
 * gets configuration cfgno and store it in the buffer
 */
int usb_get_configuration_no(struct usb_device *dev,
			     unsigned char *buffer, int cfgno)
{
	int result;
	unsigned int tmp;
	struct usb_config_descriptor *config;


	config = (struct usb_config_descriptor *)&buffer[0];
	result = usb_get_descriptor(dev, USB_DT_CONFIG, cfgno, buffer, 9);
	if (result < 9) {
		if (result < 0)
			printf("unable to get descriptor, error %lX\n",
				dev->status);
		else
			printf("config descriptor too short " \
				"(expected %i, got %i)\n", 9, result);
		return -1;
	}
	tmp = le16_to_cpu(config->wTotalLength);

	if (tmp > USB_BUFSIZ) {
		pr_debug("usb_get_configuration_no: failed to get " \
			   "descriptor - too long: %u\n", tmp);
		return -1;
	}

	result = usb_get_descriptor(dev, USB_DT_CONFIG, cfgno, buffer, tmp);
	pr_debug("get_conf_no %d Result %d, wLength %u\n",
		   cfgno, result, tmp);
	return result;
}

/********************************************************************
 * set interface number to interface
 */
int usb_set_interface(struct usb_device *dev, int interface, int alternate)
{
	struct usb_interface *if_face = NULL;
	int ret, i;

	for (i = 0; i < dev->config.desc.bNumInterfaces; i++) {
		if (dev->config.interface[i].desc.bInterfaceNumber == interface) {
			if_face = &dev->config.interface[i];
			break;
		}
	}
	if (!if_face) {
		printf("selecting invalid interface %d", interface);
		return -1;
	}
	/*
	 * We should return now for devices with only one alternate setting.
	 * According to 9.4.10 of the Universal Serial Bus Specification
	 * Revision 2.0 such devices can return with a STALL. This results in
	 * some USB sticks timeouting during initialization and then being
	 * unusable in barebox.
	 */
	if (if_face->num_altsetting == 1)
		return 0;

	ret = usb_control_msg(dev, usb_sndctrlpipe(dev, 0),
				USB_REQ_SET_INTERFACE, USB_RECIP_INTERFACE,
				alternate, interface, NULL, 0,
				USB_CNTL_TIMEOUT);
	if (ret < 0)
		return ret;

	return 0;
}

/********************************************************************
 * set protocol to protocol
 */
int usb_set_protocol(struct usb_device *dev, int ifnum, int protocol)
{
	return usb_control_msg(dev, usb_sndctrlpipe(dev, 0),
		USB_REQ_SET_PROTOCOL, USB_TYPE_CLASS | USB_RECIP_INTERFACE,
		protocol, ifnum, NULL, 0, USB_CNTL_TIMEOUT);
}

/********************************************************************
 * set idle
 */
int usb_set_idle(struct usb_device *dev, int ifnum, int duration, int report_id)
{
	return usb_control_msg(dev, usb_sndctrlpipe(dev, 0),
		USB_REQ_SET_IDLE, USB_TYPE_CLASS | USB_RECIP_INTERFACE,
		(duration << 8) | report_id, ifnum, NULL, 0, USB_CNTL_TIMEOUT);
}

/********************************************************************
 * get report
 */
int usb_get_report(struct usb_device *dev, int ifnum, unsigned char type,
		   unsigned char id, void *buf, int size)
{
	return usb_control_msg(dev, usb_rcvctrlpipe(dev, 0),
			USB_REQ_GET_REPORT,
			USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE,
			(type << 8) + id, ifnum, buf, size, USB_CNTL_TIMEOUT);
}

/********************************************************************
 * get class descriptor
 */
int usb_get_class_descriptor(struct usb_device *dev, int ifnum,
		unsigned char type, unsigned char id, void *buf, int size)
{
	return usb_control_msg(dev, usb_rcvctrlpipe(dev, 0),
		USB_REQ_GET_DESCRIPTOR, USB_RECIP_INTERFACE | USB_DIR_IN,
		(type << 8) + id, ifnum, buf, size, USB_CNTL_TIMEOUT);
}

/********************************************************************
 * get string index in buffer
 */
static int usb_get_string(struct usb_device *dev, unsigned short langid,
		   unsigned char index, void *buf, int size)
{
	int i;
	int result;

	for (i = 0; i < 3; ++i) {
		/* some devices are flaky */
		result = usb_control_msg(dev, usb_rcvctrlpipe(dev, 0),
			USB_REQ_GET_DESCRIPTOR, USB_DIR_IN,
			(USB_DT_STRING << 8) + index, langid, buf, size,
			USB_CNTL_TIMEOUT);

		if (result > 0)
			break;
	}

	return result;
}


static void usb_try_string_workarounds(unsigned char *buf, int *length)
{
	int newlength, oldlength = *length;

	for (newlength = 2; newlength + 1 < oldlength; newlength += 2)
		if (!isprint(buf[newlength]) || buf[newlength + 1])
			break;

	if (newlength > 2) {
		buf[0] = newlength;
		*length = newlength;
	}
}


static int usb_string_sub(struct usb_device *dev, unsigned int langid,
		unsigned int index, unsigned char *buf)
{
	int rc;

	/* Try to read the string descriptor by asking for the maximum
	 * possible number of bytes */
	rc = usb_get_string(dev, langid, index, buf, 255);

	/* If that failed try to read the descriptor length, then
	 * ask for just that many bytes */
	if (rc < 2) {
		rc = usb_get_string(dev, langid, index, buf, 2);
		if (rc == 2)
			rc = usb_get_string(dev, langid, index, buf, buf[0]);
	}

	if (rc >= 2) {
		if (!buf[0] && !buf[1])
			usb_try_string_workarounds(buf, &rc);

		/* There might be extra junk at the end of the descriptor */
		if (buf[0] < rc)
			rc = buf[0];

		rc = rc - (rc & 1); /* force a multiple of two */
	}

	if (rc < 2)
		rc = -1;

	return rc;
}


/********************************************************************
 * usb_string:
 * Get string index and translate it to ascii.
 * returns string length (> 0) or error (< 0)
 */
int usb_string(struct usb_device *dev, int index, char *buf, size_t size)
{
	unsigned char mybuf[USB_BUFSIZ];
	unsigned char *tbuf;
	int err;
	unsigned int u, idx;

	if (size <= 0 || !buf || !index)
		return -1;
	buf[0] = 0;
	tbuf = &mybuf[0];

	/* get langid for strings if it's not yet known */
	if (!dev->have_langid) {
		err = usb_string_sub(dev, 0, 0, tbuf);
		if (err < 0) {
			pr_debug("error getting string descriptor 0 " \
				   "(error=%lx)\n", dev->status);
			return -1;
		} else if (tbuf[0] < 4) {
			pr_debug("string descriptor 0 too short\n");
			return -1;
		} else {
			dev->have_langid = -1;
			dev->string_langid = tbuf[2] | (tbuf[3] << 8);
				/* always use the first langid listed */
			pr_debug("USB device number %d default " \
				   "language ID 0x%x\n",
				   dev->devnum, dev->string_langid);
		}
	}

	err = usb_string_sub(dev, dev->string_langid, index, tbuf);
	if (err < 0)
		return err;

	size--;		/* leave room for trailing NULL char in output buffer */
	for (idx = 0, u = 2; u < err; u += 2) {
		if (idx >= size)
			break;
		if (tbuf[u+1])			/* high byte */
			buf[idx++] = '?';  /* non-ASCII character */
		else
			buf[idx++] = tbuf[u];
	}
	buf[idx] = 0;
	err = idx;
	return err;
}

int usb_driver_register(struct usb_driver *drv)
{
	drv->driver.name = drv->name;
	drv->driver.bus = &usb_bus_type;
	return register_driver(&drv->driver);
}

/* returns 0 if no match, 1 if match */
static int usb_match_device(struct usb_device *dev, const struct usb_device_id *id)
{
	if ((id->match_flags & USB_DEVICE_ID_MATCH_VENDOR) &&
	    id->idVendor != dev->descriptor->idVendor)
		return 0;

	if ((id->match_flags & USB_DEVICE_ID_MATCH_PRODUCT) &&
	    id->idProduct != dev->descriptor->idProduct)
		return 0;

	return 1;
}


/* returns 0 if no match, 1 if match */
static int usb_match_one_id(struct usb_device *usbdev,
		     const struct usb_device_id *id)
{
	int ifno;

	/* proc_connectinfo in devio.c may call us with id == NULL. */
	if (id == NULL)
		return 0;

	if (!usb_match_device(usbdev, id))
		return 0;

	/* The interface class, subclass, and protocol should never be
	 * checked for a match if the device class is Vendor Specific,
	 * unless the match record specifies the Vendor ID. */
	if (usbdev->descriptor->bDeviceClass == USB_CLASS_VENDOR_SPEC &&
			!(id->match_flags & USB_DEVICE_ID_MATCH_VENDOR) &&
			(id->match_flags & USB_DEVICE_ID_MATCH_INT_INFO))
		return 0;

	if ( (id->match_flags & USB_DEVICE_ID_MATCH_INT_INFO) ) {
		/* match any interface */
		for (ifno=0; ifno<usbdev->config.no_of_if; ifno++) {
			struct usb_interface_descriptor *intf;
			intf = &usbdev->config.interface[ifno].desc;

			if ((id->match_flags & USB_DEVICE_ID_MATCH_INT_CLASS) &&
			    (id->bInterfaceClass != intf->bInterfaceClass))
				continue;

			if ((id->match_flags & USB_DEVICE_ID_MATCH_INT_SUBCLASS) &&
			    (id->bInterfaceSubClass != intf->bInterfaceSubClass))
				continue;

			if ((id->match_flags & USB_DEVICE_ID_MATCH_INT_PROTOCOL) &&
			    (id->bInterfaceProtocol != intf->bInterfaceProtocol))
				continue;
			break;
		}
		if (ifno >= usbdev->config.no_of_if)
			return 0;
	}

	return 1;
}
EXPORT_SYMBOL(usb_match_one_id);

static const struct usb_device_id *usb_match_id(struct usb_device *usbdev,
					 const struct usb_device_id *id)
{
	/* proc_connectinfo in devio.c may call us with id == NULL. */
	if (id == NULL)
		return NULL;

	/* It is important to check that id->driver_info is nonzero,
	   since an entry that is all zeroes except for a nonzero
	   id->driver_info is the way to create an entry that
	   indicates that the driver want to examine every
	   device and interface. */
	for (; id->idVendor || id->idProduct || id->bDeviceClass ||
	       id->bInterfaceClass || id->driver_info; id++) {
		if (usb_match_one_id(usbdev, id))
			return id;
	}

	return NULL;
}
EXPORT_SYMBOL(usb_match_id);

static int usb_match(struct device_d *dev, struct driver_d *drv)
{
	struct usb_device *usbdev = container_of(dev, struct usb_device, dev);
	struct usb_driver *usbdrv = container_of(dev->driver, struct usb_driver, driver);
	const struct usb_device_id *id;

	pr_debug("matching: 0x%04x 0x%04x\n", usbdev->descriptor->idVendor,
			usbdev->descriptor->idProduct);

	id = usb_match_id(usbdev, usbdrv->id_table);
	if (id) {
		pr_debug("match: 0x%04x 0x%04x\n", id->idVendor, id->idProduct);
		return 0;
	}
	return 1;
}

static int usb_probe(struct device_d *dev)
{
	struct usb_device *usbdev = container_of(dev, struct usb_device, dev);
	struct usb_driver *usbdrv = container_of(dev->driver, struct usb_driver, driver);
	const struct usb_device_id *id;

	id = usb_match_id(usbdev, usbdrv->id_table);

	return usbdrv->probe(usbdev, id);
}

static void usb_remove(struct device_d *dev)
{
	struct usb_device *usbdev = container_of(dev, struct usb_device, dev);
	struct usb_driver *usbdrv = container_of(dev->driver, struct usb_driver, driver);

	usbdrv->disconnect(usbdev);
}

struct bus_type usb_bus_type = {
	.name	= "usb",
	.match	= usb_match,
	.probe	= usb_probe,
	.remove	= usb_remove,
};

static int usb_bus_init(void)
{
	return bus_register(&usb_bus_type);
}
pure_initcall(usb_bus_init);
