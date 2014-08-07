/*
 * u_serial.c - utilities for USB gadget "serial port"/TTY support
 *
 * Copyright (C) 2003 Al Borchers (alborchers@steinerpoint.com)
 * Copyright (C) 2008 David Brownell
 * Copyright (C) 2008 by Nokia Corporation
 *
 * This code also borrows from usbserial.c, which is
 * Copyright (C) 1999 - 2002 Greg Kroah-Hartman (greg@kroah.com)
 * Copyright (C) 2000 Peter Berger (pberger@brimson.com)
 * Copyright (C) 2000 Al Borchers (alborchers@steinerpoint.com)
 *
 * This software is distributed under the terms of the GNU General
 * Public License ("GPL") as published by the Free Software Foundation,
 * either version 2 of that License or (at your option) any later version.
 */

/* #define VERBOSE_DEBUG */

#include <common.h>
#include <complete.h>
#include <usb/cdc.h>
#include <kfifo.h>
#include <clock.h>
#include <linux/err.h>
#include <dma.h>

#include "u_serial.h"


/*
 * This component encapsulates the TTY layer glue needed to provide basic
 * "serial port" functionality through the USB gadget stack.  Each such
 * port is exposed through a /dev/ttyGS* node.
 *
 * After this module has been loaded, the individual TTY port can be requested
 * (gserial_alloc_line()) and it will stay available until they are removed
 * (gserial_free_line()). Each one may be connected to a USB function
 * (gserial_connect), or disconnected (with gserial_disconnect) when the USB
 * host issues a config change event. Data can only flow when the port is
 * connected to the host.
 *
 * A given TTY port can be made available in multiple configurations.
 * For example, each one might expose a ttyGS0 node which provides a
 * login application.  In one case that might use CDC ACM interface 0,
 * while another configuration might use interface 3 for that.  The
 * work to handle that (including descriptor management) is not part
 * of this component.
 *
 * Configurations may expose more than one TTY port.  For example, if
 * ttyGS0 provides login service, then ttyGS1 might provide dialer access
 * for a telephone or fax link.  And ttyGS2 might be something that just
 * needs a simple byte stream interface for some messaging protocol that
 * is managed in userspace ... OBEX, PTP, and MTP have been mentioned.
 */

#define PREFIX	"ttyGS"

/*
 * gserial is the lifecycle interface, used by USB functions
 * gs_port is the I/O nexus, used by the tty driver
 * tty_struct links to the tty/filesystem framework
 *
 * gserial <---> gs_port ... links will be null when the USB link is
 * inactive; managed by gserial_{connect,disconnect}().  each gserial
 * instance can wrap its own USB control protocol.
 *	gserial->ioport == usb_ep->driver_data ... gs_port
 *	gs_port->port_usb ... gserial
 *
 * gs_port <---> tty_struct ... links will be null when the TTY file
 * isn't opened; managed by gs_open()/gs_close()
 *	gserial->port_tty ... tty_struct
 *	tty_struct->driver_data ... gserial
 */

/* RX and TX queues can buffer QUEUE_SIZE packets before they hit the
 * next layer of buffering.  For TX that's a circular buffer; for RX
 * consider it a NOP.  A third layer is provided by the TTY code.
 */
#define QUEUE_SIZE		16
#define WRITE_BUF_SIZE		8192		/* TX only */
#define RECV_FIFO_SIZE		(1024 * 8)

/* circular buffer */
struct gs_buf {
	unsigned		buf_size;
	char			*buf_buf;
	char			*buf_get;
	char			*buf_put;
};

/*
 * The port structure holds info for each port, one for each minor number
 * (and thus for each /dev/ node).
 */
struct gs_port {
	struct gserial		*port_usb;
	struct console_device	cdev;
	struct kfifo		*recv_fifo;

	u8			port_num;

	struct list_head	read_pool;
	unsigned		read_nb_queued;

	struct list_head	write_pool;

	/* REVISIT this state ... */
	struct usb_cdc_line_coding port_line_coding;	/* 8-N-1 etc */
};

static struct portmaster {
	struct gs_port	*port;
} ports[MAX_U_SERIAL_PORTS];

#define GS_CLOSE_TIMEOUT		15		/* seconds */

static unsigned gs_start_rx(struct gs_port *port)
{
	struct list_head	*pool = &port->read_pool;
	struct usb_ep		*out = port->port_usb->out;
	unsigned		started = 0;

	while (!list_empty(pool) &&
	       ((out->maxpacket * (port->read_nb_queued + 1) +
		kfifo_len(port->recv_fifo)) < RECV_FIFO_SIZE)) {
		struct usb_request	*req;
		int			status;

		req = list_entry(pool->next, struct usb_request, list);
		list_del(&req->list);
		req->length = out->maxpacket;

		/* drop lock while we call out; the controller driver
		 * may need to call us back (e.g. for disconnect)
		 */
		port->read_nb_queued++;
		status = usb_ep_queue(out, req);

		if (status) {
			pr_debug("%s: %s %s err %d\n",
					__func__, "queue", out->name, status);
			list_add(&req->list, pool);
			break;
		}
		started++;

		/* abort immediately after disconnect */
		if (!port->port_usb)
			break;
	}
	return started;
}

/*-------------------------------------------------------------------------*/

static void gs_read_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct gs_port	*port = ep->driver_data;

	if (req->status == -ESHUTDOWN)
		return;

	kfifo_put(port->recv_fifo, req->buf, req->actual);
	list_add_tail(&req->list, &port->read_pool);
	port->read_nb_queued--;

	gs_start_rx(port);
}

/*-------------------------------------------------------------------------*/

/* I/O glue between TTY (upper) and USB function (lower) driver layers */

static void gs_write_complete(struct usb_ep *ep, struct usb_request *req)
{
	struct gs_port	*port = ep->driver_data;

	list_add(&req->list, &port->write_pool);

	switch (req->status) {
	default:
		/* presumably a transient fault */
		pr_warning("%s: unexpected %s status %d\n",
				__func__, ep->name, req->status);
		/* FALL THROUGH */
	case 0:
		/* normal completion */
		break;

	case -ESHUTDOWN:
		/* disconnect */
		pr_vdebug("%s: %s shutdown\n", __func__, ep->name);
		break;
	}
}

/*
 * gs_alloc_req
 *
 * Allocate a usb_request and its buffer.  Returns a pointer to the
 * usb_request or NULL if there is an error.
 */
struct usb_request *
gs_alloc_req(struct usb_ep *ep, unsigned len)
{
	struct usb_request *req;

	req = usb_ep_alloc_request(ep);

	if (req != NULL) {
		req->length = len;
		req->buf = dma_alloc(len);
	}

	return req;
}
EXPORT_SYMBOL_GPL(gs_alloc_req);

/*
 * gs_free_req
 *
 * Free a usb_request and its buffer.
 */
void gs_free_req(struct usb_ep *ep, struct usb_request *req)
{
	kfree(req->buf);
	usb_ep_free_request(ep, req);
}
EXPORT_SYMBOL_GPL(gs_free_req);

static void gs_free_requests(struct usb_ep *ep, struct list_head *head)
{
	struct usb_request	*req;

	while (!list_empty(head)) {
		req = list_entry(head->next, struct usb_request, list);
		list_del(&req->list);
		gs_free_req(ep, req);
	}
}

static int gs_alloc_requests(struct usb_ep *ep, struct list_head *head,
		void (*fn)(struct usb_ep *, struct usb_request *))
{
	int			i;
	struct usb_request	*req;

	/* Pre-allocate up to QUEUE_SIZE transfers, but if we can't
	 * do quite that many this time, don't fail ... we just won't
	 * be as speedy as we might otherwise be.
	 */
	for (i = 0; i < QUEUE_SIZE; i++) {
		req = gs_alloc_req(ep, ep->maxpacket);
		if (!req)
			return list_empty(head) ? -ENOMEM : 0;
		req->complete = fn;
		list_add_tail(&req->list, head);
	}
	return 0;
}

/**
 * gs_start_io - start USB I/O streams
 * @dev: encapsulates endpoints to use
 * Context: holding port_lock; port_tty and port_usb are non-null
 *
 * We only start I/O when something is connected to both sides of
 * this port.  If nothing is listening on the host side, we may
 * be pointlessly filling up our TX buffers and FIFO.
 */
static int gs_start_io(struct gs_port *port)
{
	struct list_head	*head = &port->read_pool;
	struct usb_ep		*ep = port->port_usb->out;
	int			status;
	unsigned		started;

	/* Allocate RX and TX I/O buffers.  We can't easily do this much
	 * earlier (with GFP_KERNEL) because the requests are coupled to
	 * endpoints, as are the packet sizes we'll be using.  Different
	 * configurations may use different endpoints with a given port;
	 * and high speed vs full speed changes packet sizes too.
	 */
	status = gs_alloc_requests(ep, head, gs_read_complete);
	if (status)
		return status;

	status = gs_alloc_requests(port->port_usb->in, &port->write_pool,
			gs_write_complete);
	if (status) {
		gs_free_requests(ep, head);
		return status;
	}

	/* queue read requests */
	port->read_nb_queued = 0;
	started = gs_start_rx(port);

	/* unblock any pending writes into our circular buffer */
	if (!started) {
		gs_free_requests(ep, head);
		gs_free_requests(port->port_usb->in, &port->write_pool);
		status = -EIO;
	}

	return status;
}

/*-------------------------------------------------------------------------*/

static int
gs_port_alloc(unsigned port_num, struct usb_cdc_line_coding *coding)
{
	struct gs_port	*port;
	int		ret = 0;

	if (ports[port_num].port) {
		ret = -EBUSY;
		goto out;
	}

	port = kzalloc(sizeof(struct gs_port), GFP_KERNEL);
	if (port == NULL) {
		ret = -ENOMEM;
		goto out;
	}

	INIT_LIST_HEAD(&port->read_pool);
	INIT_LIST_HEAD(&port->write_pool);

	port->port_num = port_num;
	port->port_line_coding = *coding;

	ports[port_num].port = port;
out:
	return ret;
}

static void gserial_free_port(struct gs_port *port)
{
	kfree(port);
}

void gserial_free_line(unsigned char port_num)
{
	struct gs_port	*port;

	if (WARN_ON(!ports[port_num].port))
		return;

	port = ports[port_num].port;
	ports[port_num].port = NULL;

	gserial_free_port(port);
}
EXPORT_SYMBOL_GPL(gserial_free_line);

int gserial_alloc_line(unsigned char *line_num)
{
	struct usb_cdc_line_coding	coding;
	int				ret;
	int				port_num;

	coding.dwDTERate = cpu_to_le32(9600);
	coding.bCharFormat = 8;
	coding.bParityType = USB_CDC_NO_PARITY;
	coding.bDataBits = USB_CDC_1_STOP_BITS;

	for (port_num = 0; port_num < MAX_U_SERIAL_PORTS; port_num++) {
		ret = gs_port_alloc(port_num, &coding);
		if (ret == -EBUSY)
			continue;
		if (ret)
			return ret;
		break;
	}
	if (ret)
		return ret;

	/* ... and sysfs class devices, so mdev/udev make /dev/ttyGS* */

	*line_num = port_num;

	return ret;
}
EXPORT_SYMBOL_GPL(gserial_alloc_line);

static void serial_putc(struct console_device *cdev, char c)
{
	struct gs_port	*port = container_of(cdev,
					struct gs_port, cdev);
	struct list_head	*pool = &port->write_pool;
	struct usb_ep		*in;
	struct usb_request	*req;
	int status;
	uint64_t to;

	if (list_empty(pool))
		return;
	in = port->port_usb->in;
	req = list_entry(pool->next, struct usb_request, list);

	req->length = 1;
	list_del(&req->list);

	*(unsigned char *)req->buf = c;
	status = usb_ep_queue(in, req);

	to = get_time_ns();
	while (status >= 0 && list_empty(pool)) {
		status = usb_gadget_poll();
		if (is_timeout(to, 300 * MSECOND))
			break;
	}
}

static int serial_tstc(struct console_device *cdev)
{
	struct gs_port	*port = container_of(cdev,
					struct gs_port, cdev);

	gs_start_rx(port);
	return (kfifo_len(port->recv_fifo) == 0) ? 0 : 1;
}

static int serial_getc(struct console_device *cdev)
{
	struct gs_port	*port = container_of(cdev,
					struct gs_port, cdev);
	unsigned char ch;
	uint64_t to;

	if (!port->port_usb)
		return -EIO;
	to = get_time_ns();
	while (kfifo_getc(port->recv_fifo, &ch)) {
		usb_gadget_poll();
		if (is_timeout(to, 300 * MSECOND))
			goto timeout;
	}

	gs_start_rx(port);
	return ch;
timeout:
	gs_start_rx(port);
	return -ETIMEDOUT;
}

static void serial_flush(struct console_device *cdev)
{
}

static int serial_setbaudrate(struct console_device *cdev, int baudrate)
{
	return 0;
}

/**
 * gserial_connect - notify TTY I/O glue that USB link is active
 * @gser: the function, set up with endpoints and descriptors
 * @port_num: which port is active
 * Context: any (usually from irq)
 *
 * This is called activate endpoints and let the TTY layer know that
 * the connection is active ... not unlike "carrier detect".  It won't
 * necessarily start I/O queues; unless the TTY is held open by any
 * task, there would be no point.  However, the endpoints will be
 * activated so the USB host can perform I/O, subject to basic USB
 * hardware flow control.
 *
 * Caller needs to have set up the endpoints and USB function in @dev
 * before calling this, as well as the appropriate (speed-specific)
 * endpoint descriptors, and also have allocate @port_num by calling
 * @gserial_alloc_line().
 *
 * Returns negative errno or zero.
 * On success, ep->driver_data will be overwritten.
 */
int gserial_connect(struct gserial *gser, u8 port_num)
{
	struct gs_port	*port;
	int		status;
	struct console_device *cdev;

	if (port_num >= MAX_U_SERIAL_PORTS)
		return -ENXIO;

	port = ports[port_num].port;
	if (!port) {
		pr_err("serial line %d not allocated.\n", port_num);
		return -EINVAL;
	}
	if (port->port_usb) {
		pr_err("serial line %d is in use.\n", port_num);
		return -EBUSY;
	}

	/* activate the endpoints */
	status = usb_ep_enable(gser->in);
	if (status < 0)
		return status;
	gser->in->driver_data = port;

	status = usb_ep_enable(gser->out);
	if (status < 0)
		goto fail_out;
	gser->out->driver_data = port;

	/* then tell the tty glue that I/O can work */
	gser->ioport = port;
	port->port_usb = gser;

	/* REVISIT unclear how best to handle this state...
	 * we don't really couple it with the Linux TTY.
	 */
	gser->port_line_coding = port->port_line_coding;

	port->recv_fifo = kfifo_alloc(RECV_FIFO_SIZE);

	/*printf("gserial_connect: start ttyGS%d\n", port->port_num);*/
	gs_start_io(port);
	if (gser->connect)
		gser->connect(gser);

	cdev = &port->cdev;
	cdev->tstc = serial_tstc;
	cdev->putc = serial_putc;
	cdev->getc = serial_getc;
	cdev->flush = serial_flush;
	cdev->setbrg = serial_setbaudrate;
	cdev->devname = "usbserial";

	status = console_register(cdev);
	if (status)
		goto fail_out;

	dev_set_param(&cdev->class_dev, "active", "ioe");

	/* REVISIT if waiting on "carrier detect", signal. */

	/* if it's already open, start I/O ... and notify the serial
	 * protocol about open/close status (connect/disconnect).
	 */
	if (1) {
		pr_debug("gserial_connect: start ttyGS%d\n", port->port_num);
		if (gser->connect)
			gser->connect(gser);
	} else {
		if (gser->disconnect)
			gser->disconnect(gser);
	}

	return status;

fail_out:
	usb_ep_disable(gser->in);
	gser->in->driver_data = NULL;
	return status;
}
EXPORT_SYMBOL_GPL(gserial_connect);

/**
 * gserial_disconnect - notify TTY I/O glue that USB link is inactive
 * @gser: the function, on which gserial_connect() was called
 * Context: any (usually from irq)
 *
 * This is called to deactivate endpoints and let the TTY layer know
 * that the connection went inactive ... not unlike "hangup".
 *
 * On return, the state is as if gserial_connect() had never been called;
 * there is no active USB I/O on these endpoints.
 */
void gserial_disconnect(struct gserial *gser)
{
	struct gs_port	*port = gser->ioport;
	struct console_device *cdev;

	if (!port)
		return;

	cdev = &port->cdev;

	/* tell the TTY glue not to do I/O here any more */
	console_unregister(cdev);

	/* REVISIT as above: how best to track this? */
	port->port_line_coding = gser->port_line_coding;

	port->port_usb = NULL;
	gser->ioport = NULL;

	/* disable endpoints, aborting down any active I/O */
	usb_ep_disable(gser->out);
	gser->out->driver_data = NULL;

	usb_ep_disable(gser->in);
	gser->in->driver_data = NULL;

	/* finally, free any unused/unusable I/O buffers */
	gs_free_requests(gser->out, &port->read_pool);
	gs_free_requests(gser->in, &port->write_pool);

	kfifo_free(port->recv_fifo);
}
