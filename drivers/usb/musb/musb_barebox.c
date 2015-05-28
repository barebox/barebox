#include <common.h>
#include <init.h>
#include <clock.h>
#include <usb/musb.h>
#include <usb/usb.h>
#include <linux/types.h>
#include <linux/err.h>
#include <linux/barebox-wrapper.h>

#include "musb_core.h"
#include "musb_gadget.h"

static struct usb_host_endpoint hep;
static struct urb urb;

static void musb_host_complete_urb(struct urb *urb)
{
	urb->dev->status &= ~USB_ST_NOT_PROC;
	urb->dev->act_len = urb->actual_length;
}

static struct urb *construct_urb(struct usb_device *dev, int endpoint_type,
				unsigned long pipe, void *buffer, int len,
				struct devrequest *setup, int interval)
{
	int epnum = usb_pipeendpoint(pipe);
	int is_in = usb_pipein(pipe);

	memset(&urb, 0, sizeof(struct urb));
	memset(&hep, 0, sizeof(struct usb_host_endpoint));
	INIT_LIST_HEAD(&hep.urb_list);
	INIT_LIST_HEAD(&urb.urb_list);
	urb.ep = &hep;
	urb.complete = musb_host_complete_urb;
	urb.status = -EINPROGRESS;
	urb.dev = dev;
	urb.pipe = pipe;
	urb.transfer_buffer = buffer;
	urb.transfer_dma = (unsigned long)buffer;
	urb.transfer_buffer_length = len;
	urb.setup_packet = (unsigned char *)setup;

	urb.ep->desc.wMaxPacketSize =
		__cpu_to_le16(is_in ? dev->epmaxpacketin[epnum] :
				dev->epmaxpacketout[epnum]);
	urb.ep->desc.bmAttributes = endpoint_type;
	urb.ep->desc.bEndpointAddress =
		(is_in ? USB_DIR_IN : USB_DIR_OUT) | epnum;
	urb.ep->desc.bInterval = interval;

	return &urb;
}

#define MUSB_HOST_TIMEOUT 0x5fffff

static int submit_urb(struct usb_device *dev, struct urb *urb, int timeout_ms)
{
	struct usb_host *host = dev->host;
	struct musb *musb = to_musb(host);
	int ret;
	uint64_t start;
	uint64_t timeout = timeout_ms;

	ret = musb_urb_enqueue(musb->hcd, urb, 0);
	if (ret < 0) {
		printf("Failed to enqueue URB to controller\n");
		return ret;
	}

	start = get_time_ns();

	do {
		musb->isr(musb);

		if (!urb->status)
			return 0;

	} while (!is_timeout(start, timeout * MSECOND));

	if (urb->dev->status & USB_ST_NOT_PROC)
		ret = -ETIMEDOUT;
	else
		ret = urb->status;

	musb_urb_dequeue(musb->hcd, urb, -ECONNRESET);

	return ret;
}

static int
submit_bulk_msg(struct usb_device *dev, unsigned long pipe, void *buffer,
		int length, int timeout)
{
	struct urb *urb = construct_urb(dev, USB_ENDPOINT_XFER_BULK, pipe,
					buffer, length, NULL, 0);
	return submit_urb(dev, urb, timeout);
}

static int
submit_control_msg(struct usb_device *dev, unsigned long pipe, void *buffer,
		   int length, struct devrequest *setup, int timeout)
{
	struct usb_host *host = dev->host;
	struct musb *musb = to_musb(host);
	struct urb *urb = construct_urb(dev, USB_ENDPOINT_XFER_CONTROL, pipe,
					buffer, length, setup, 0);

	/* Fix speed for non hub-attached devices */
	if (!dev->parent)
		dev->speed = musb->host_speed;

	return submit_urb(dev, urb, timeout);
}

static int
submit_int_msg(struct usb_device *dev, unsigned long pipe, void *buffer,
	       int length, int interval)
{
	struct urb *urb = construct_urb(dev, USB_ENDPOINT_XFER_INT, pipe,
					buffer, length, NULL, interval);
	return submit_urb(dev, urb, 100);
}

static int musb_detect(struct device_d *dev)
{
	struct musb *musb = dev->priv;

	return usb_host_detect(&musb->host);
}

int musb_register(struct musb *musb)
{
	struct usb_host *host;

	host = &musb->host;
	host->hw_dev = musb->controller;
	host->init = musb_init;
	host->submit_int_msg = submit_int_msg;
	host->submit_control_msg = submit_control_msg;
	host->submit_bulk_msg = submit_bulk_msg;

	musb->controller->priv = musb;
	musb->controller->detect = musb_detect;
	usb_register_host(host);

	return 0;
}
