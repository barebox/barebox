/*
 * (C) Copyright 2008 - 2009
 * Windriver, <www.windriver.com>
 * Tom Rix <Tom.Rix@windriver.com>
 *
 * Copyright 2011 Sebastian Andrzej Siewior <bigeasy@linutronix.de>
 *
 * Copyright 2014 Linaro, Ltd.
 * Rob Herring <robh@kernel.org>
 *
 * Copyright 2014 Sascha Hauer <s.hauer@pengutronix.de>
 * Ported to barebox
 *
 * Copyright 2020 Edmund Henniges <eh@emlix.com>
 * Copyright 2020 Daniel Glöckner <dg@emlix.com>
 * Split off of generic parts
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#define pr_fmt(fmt) "fastboot: " fmt

#include <dma.h>
#include <work.h>
#include <unistd.h>
#include <progress.h>
#include <fastboot.h>
#include <usb/fastboot.h>

#define FASTBOOT_INTERFACE_CLASS	0xff
#define FASTBOOT_INTERFACE_SUB_CLASS	0x42
#define FASTBOOT_INTERFACE_PROTOCOL	0x03

#define EP_BUFFER_SIZE			4096

struct f_fastboot {
	struct fastboot fastboot;
	struct usb_function func;

	/* IN/OUT EP's and corresponding requests */
	struct usb_ep *in_ep, *out_ep;
	struct usb_request *in_req, *out_req;
	struct work_queue wq;
};

static inline struct f_fastboot *func_to_fastboot(struct usb_function *f)
{
	return container_of(f, struct f_fastboot, func);
}

static struct usb_endpoint_descriptor fs_ep_in = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize		= cpu_to_le16(64),
	.bInterval		= 0x00,
};

static struct usb_endpoint_descriptor fs_ep_out = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_OUT,
	.bmAttributes		= USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize		= cpu_to_le16(64),
	.bInterval		= 0x00,
};

static struct usb_endpoint_descriptor hs_ep_in = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_IN,
	.bmAttributes		= USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize		= cpu_to_le16(512),
	.bInterval		= 0x00,
};

static struct usb_endpoint_descriptor hs_ep_out = {
	.bLength		= USB_DT_ENDPOINT_SIZE,
	.bDescriptorType	= USB_DT_ENDPOINT,
	.bEndpointAddress	= USB_DIR_OUT,
	.bmAttributes		= USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize		= cpu_to_le16(512),
	.bInterval		= 0x00,
};

static struct usb_interface_descriptor interface_desc = {
	.bLength		= USB_DT_INTERFACE_SIZE,
	.bDescriptorType	= USB_DT_INTERFACE,
	.bInterfaceNumber	= 0x00,
	.bAlternateSetting	= 0x00,
	.bNumEndpoints		= 0x02,
	.bInterfaceClass	= FASTBOOT_INTERFACE_CLASS,
	.bInterfaceSubClass	= FASTBOOT_INTERFACE_SUB_CLASS,
	.bInterfaceProtocol	= FASTBOOT_INTERFACE_PROTOCOL,
};

static struct usb_descriptor_header *fb_fs_descs[] = {
	(struct usb_descriptor_header *)&interface_desc,
	(struct usb_descriptor_header *)&fs_ep_in,
	(struct usb_descriptor_header *)&fs_ep_out,
	NULL,
};

static struct usb_descriptor_header *fb_hs_descs[] = {
	(struct usb_descriptor_header *)&interface_desc,
	(struct usb_descriptor_header *)&hs_ep_in,
	(struct usb_descriptor_header *)&hs_ep_out,
	NULL,
};

/*
 * static strings, in UTF-8
 */
static const char fastboot_name[] = "Android Fastboot";

static struct usb_string fastboot_string_defs[] = {
	[0].s = fastboot_name,
	{  }			/* end of list */
};

static struct usb_gadget_strings stringtab_fastboot = {
	.language	= 0x0409,	/* en-us */
	.strings	= fastboot_string_defs,
};

static struct usb_gadget_strings *fastboot_strings[] = {
	&stringtab_fastboot,
	NULL,
};

static void rx_handler_command(struct usb_ep *ep, struct usb_request *req);
static int fastboot_write_usb(struct fastboot *fb, const char *buffer,
			      unsigned int buffer_size);
static void fastboot_start_download_usb(struct fastboot *fb);

static void fastboot_complete(struct usb_ep *ep, struct usb_request *req)
{
}

struct fastboot_work {
	struct work_struct work;
	struct f_fastboot *f_fb;
	char command[FASTBOOT_MAX_CMD_LEN + 1];
};

static void fastboot_do_work(struct work_struct *w)
{
	struct fastboot_work *fw = container_of(w, struct fastboot_work, work);
	struct f_fastboot *f_fb = fw->f_fb;

	fastboot_exec_cmd(&f_fb->fastboot, fw->command);

	memset(f_fb->out_req->buf, 0, EP_BUFFER_SIZE);
	usb_ep_queue(f_fb->out_ep, f_fb->out_req);

	free(fw);
}

static void fastboot_work_cancel(struct work_struct *w)
{
	struct fastboot_work *fw = container_of(w, struct fastboot_work, work);

	free(fw);
}

static struct usb_request *fastboot_alloc_request(struct usb_ep *ep)
{
	struct usb_request *req;

	req = usb_ep_alloc_request(ep);
	if (!req)
		return NULL;

	req->length = EP_BUFFER_SIZE;
	req->buf = dma_alloc(EP_BUFFER_SIZE);
	if (!req->buf) {
		usb_ep_free_request(ep, req);
		return NULL;
	}
	memset(req->buf, 0, EP_BUFFER_SIZE);

	return req;
}

static int fastboot_bind(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_composite_dev *cdev = c->cdev;
	int id, ret;
	struct usb_gadget *gadget = c->cdev->gadget;
	struct f_fastboot *f_fb = func_to_fastboot(f);
	struct usb_string *us;
	const struct usb_function_instance *fi = f->fi;
	struct f_fastboot_opts *opts = container_of(fi, struct f_fastboot_opts, func_inst);

	f_fb->fastboot.write = fastboot_write_usb;
	f_fb->fastboot.start_download = fastboot_start_download_usb;

	f_fb->fastboot.files = opts->common.files;
	f_fb->fastboot.cmd_exec = opts->common.cmd_exec;
	f_fb->fastboot.cmd_flash = opts->common.cmd_flash;

	f_fb->wq.fn = fastboot_do_work;
	f_fb->wq.cancel = fastboot_work_cancel;

	wq_register(&f_fb->wq);

	ret = fastboot_generic_init(&f_fb->fastboot, opts->common.export_bbu);
	if (ret)
		return ret;

	/* DYNAMIC interface numbers assignments */
	id = usb_interface_id(c, f);
	if (id < 0)
		return id;
	interface_desc.bInterfaceNumber = id;

	id = usb_string_id(c->cdev);
	if (id < 0)
		return id;
	fastboot_string_defs[0].id = id;
	interface_desc.iInterface = id;

	us = usb_gstrings_attach(cdev, fastboot_strings, 1);
	if (IS_ERR(us)) {
		ret = PTR_ERR(us);
		return ret;
	}

	f_fb->in_ep = usb_ep_autoconfig(gadget, &fs_ep_in);
	if (!f_fb->in_ep)
		return -ENODEV;
	f_fb->in_ep->driver_data = c->cdev;

	f_fb->out_ep = usb_ep_autoconfig(gadget, &fs_ep_out);
	if (!f_fb->out_ep)
		return -ENODEV;
	f_fb->out_ep->driver_data = c->cdev;

	hs_ep_out.bEndpointAddress = fs_ep_out.bEndpointAddress;
	hs_ep_in.bEndpointAddress = fs_ep_in.bEndpointAddress;

	f_fb->out_req = fastboot_alloc_request(f_fb->out_ep);
	if (!f_fb->out_req) {
		puts("failed to alloc out req\n");
		ret = -EINVAL;
		return ret;
	}

	f_fb->out_req->complete = rx_handler_command;
	f_fb->out_req->context = f_fb;

	f_fb->in_req = fastboot_alloc_request(f_fb->in_ep);
	if (!f_fb->in_req) {
		puts("failed alloc req in\n");
		ret = -EINVAL;
		return ret;
	}
	f_fb->in_req->complete = fastboot_complete;

	ret = usb_assign_descriptors(f, fb_fs_descs, fb_hs_descs, NULL);
	if (ret)
		return ret;

	return 0;
}

static void fastboot_unbind(struct usb_configuration *c, struct usb_function *f)
{
	struct f_fastboot *f_fb = func_to_fastboot(f);

	usb_ep_dequeue(f_fb->in_ep, f_fb->in_req);
	free(f_fb->in_req->buf);
	usb_ep_free_request(f_fb->in_ep, f_fb->in_req);
	f_fb->in_req = NULL;

	usb_ep_dequeue(f_fb->out_ep, f_fb->out_req);
	free(f_fb->out_req->buf);
	usb_ep_free_request(f_fb->out_ep, f_fb->out_req);
	f_fb->out_req = NULL;

	wq_unregister(&f_fb->wq);

	fastboot_generic_free(&f_fb->fastboot);
}

static void fastboot_disable(struct usb_function *f)
{
	struct f_fastboot *f_fb = func_to_fastboot(f);

	usb_ep_disable(f_fb->out_ep);
	usb_ep_disable(f_fb->in_ep);
}

static int fastboot_set_alt(struct usb_function *f,
			    unsigned interface, unsigned alt)
{
	int ret;
	struct f_fastboot *f_fb = func_to_fastboot(f);

	pr_debug("%s: func: %s intf: %d alt: %d\n",
	      __func__, f->name, interface, alt);

	ret = config_ep_by_speed(f->config->cdev->gadget, f,
			f_fb->out_ep);
	if (ret)
		return ret;

	ret = usb_ep_enable(f_fb->out_ep);
	if (ret) {
		pr_err("failed to enable out ep: %s\n", strerror(-ret));
		return ret;
	}

	ret = config_ep_by_speed(f->config->cdev->gadget, f,
			f_fb->in_ep);
	if (ret)
		return ret;

	ret = usb_ep_enable(f_fb->in_ep);
	if (ret) {
		pr_err("failed to enable in ep: %s\n", strerror(-ret));
		return ret;
	}

	memset(f_fb->out_req->buf, 0, EP_BUFFER_SIZE);
	ret = usb_ep_queue(f_fb->out_ep, f_fb->out_req);
	if (ret)
		goto err;

	return 0;
err:
	fastboot_disable(f);
	return ret;
}

static void fastboot_free_func(struct usb_function *f)
{
	struct f_fastboot *f_fb = container_of(f, struct f_fastboot, func);

	fastboot_generic_close(&f_fb->fastboot);
	free(f_fb);
}

static struct usb_function *fastboot_alloc_func(struct usb_function_instance *fi)
{
	struct f_fastboot *f_fb;

	f_fb = xzalloc(sizeof(*f_fb));

	f_fb->func.name = "fastboot";
	f_fb->func.strings = fastboot_strings;
	f_fb->func.bind = fastboot_bind;
	f_fb->func.set_alt = fastboot_set_alt;
	f_fb->func.disable = fastboot_disable;
	f_fb->func.unbind = fastboot_unbind;
	f_fb->func.free_func = fastboot_free_func;

	return &f_fb->func;
}

static void fastboot_free_instance(struct usb_function_instance *fi)
{
	struct f_fastboot_opts *opts;

	opts = container_of(fi, struct f_fastboot_opts, func_inst);
	kfree(opts);
}

static struct usb_function_instance *fastboot_alloc_instance(void)
{
	struct f_fastboot_opts *opts;

	opts = xzalloc(sizeof(*opts));
	opts->func_inst.free_func_inst = fastboot_free_instance;

	return &opts->func_inst;
}

DECLARE_USB_FUNCTION_INIT(fastboot, fastboot_alloc_instance, fastboot_alloc_func);

static int fastboot_write_usb(struct fastboot *fb, const char *buffer, unsigned int buffer_size)
{
	struct f_fastboot *f_fb = container_of(fb, struct f_fastboot, fastboot);
	struct usb_request *in_req = f_fb->in_req;
	uint64_t start;
	int ret;

	memcpy(in_req->buf, buffer, buffer_size);
	in_req->length = buffer_size;

	ret = usb_ep_queue(f_fb->in_ep, in_req);
	if (ret)
		pr_err("Error %d on queue\n", ret);

	start = get_time_ns();

	while (in_req->status == -EINPROGRESS) {
		if (is_timeout(start, 2 * SECOND))
			return -ETIMEDOUT;
		usb_gadget_poll();
	}

	if (in_req->status)
		pr_err("Failed to send answer: %d\n", in_req->status);

	return 0;
}

static int rx_bytes_expected(struct f_fastboot *f_fb)
{
	int remaining = f_fb->fastboot.download_size
		      - f_fb->fastboot.download_bytes;

	if (remaining >= EP_BUFFER_SIZE)
		return EP_BUFFER_SIZE;

	return ALIGN(remaining, f_fb->out_ep->maxpacket);
}

static void rx_handler_dl_image(struct usb_ep *ep, struct usb_request *req)
{
	struct f_fastboot *f_fb = req->context;
	const unsigned char *buffer = req->buf;
	int ret;

	if (req->status != 0) {
		pr_err("Bad status: %d\n", req->status);
		return;
	}

	ret = fastboot_handle_download_data(&f_fb->fastboot, buffer,
					    req->actual);
	if (ret < 0) {
		fastboot_tx_print(&f_fb->fastboot, FASTBOOT_MSG_FAIL,
				  strerror(-ret));
		return;
	}

	req->length = rx_bytes_expected(f_fb);

	/* Check if transfer is done */
	if (f_fb->fastboot.download_bytes >= f_fb->fastboot.download_size) {
		req->complete = rx_handler_command;
		req->length = EP_BUFFER_SIZE;

		fastboot_download_finished(&f_fb->fastboot);
	}

	req->actual = 0;
	usb_ep_queue(ep, req);
}

static void fastboot_start_download_usb(struct fastboot *fb)
{
	struct f_fastboot *f_fb = container_of(fb, struct f_fastboot, fastboot);
	struct usb_request *req = f_fb->out_req;

	req->complete = rx_handler_dl_image;
	req->length = rx_bytes_expected(f_fb);
	fastboot_start_download_generic(fb);
}

static void rx_handler_command(struct usb_ep *ep, struct usb_request *req)
{
	struct f_fastboot *f_fb = req->context;
	struct fastboot_work *w;
	int len;

	if (req->status != 0)
		return;

	w = xzalloc(sizeof(*w));
	w->f_fb = f_fb;

	len = min_t(unsigned int, req->actual, FASTBOOT_MAX_CMD_LEN);

	memcpy(w->command, req->buf, len);

	wq_queue_work(&f_fb->wq, &w->work);
}
