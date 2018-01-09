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
 * SPDX-License-Identifier:	GPL-2.0+
 */

#define pr_fmt(fmt) "fastboot: " fmt

#include <common.h>
#include <command.h>
#include <errno.h>
#include <malloc.h>
#include <fcntl.h>
#include <clock.h>
#include <ioctl.h>
#include <libbb.h>
#include <bbu.h>
#include <bootm.h>
#include <dma.h>
#include <fs.h>
#include <libfile.h>
#include <ubiformat.h>
#include <stdlib.h>
#include <file-list.h>
#include <magicvar.h>
#include <linux/sizes.h>
#include <progress.h>
#include <environment.h>
#include <globalvar.h>
#include <restart.h>
#include <console_countdown.h>
#include <image-sparse.h>
#include <usb/ch9.h>
#include <usb/gadget.h>
#include <usb/fastboot.h>
#include <usb/composite.h>
#include <linux/err.h>
#include <linux/compiler.h>
#include <linux/stat.h>
#include <linux/mtd/mtd-abi.h>
#include <linux/mtd/mtd.h>

#define FASTBOOT_VERSION		"0.4"

#define FASTBOOT_INTERFACE_CLASS	0xff
#define FASTBOOT_INTERFACE_SUB_CLASS	0x42
#define FASTBOOT_INTERFACE_PROTOCOL	0x03

#define FASTBOOT_TMPFILE		"/.fastboot.img"

#define EP_BUFFER_SIZE			4096

static unsigned int fastboot_max_download_size = SZ_8M;

struct fb_variable {
	char *name;
	char *value;
	struct list_head list;
};

struct f_fastboot {
	struct usb_function func;

	/* IN/OUT EP's and correspoinding requests */
	struct usb_ep *in_ep, *out_ep;
	struct usb_request *in_req, *out_req;
	struct file_list *files;
	int download_fd;
	size_t download_bytes;
	size_t download_size;
	struct list_head variables;
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

static int in_req_complete;

static void fastboot_complete(struct usb_ep *ep, struct usb_request *req)
{
	int status = req->status;

	in_req_complete = 1;

	pr_debug("status: %d ep '%s' trans: %d\n", status, ep->name, req->actual);
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

static void fb_setvar(struct fb_variable *var, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	var->value = bvasprintf(fmt, ap);
	va_end(ap);
}

static struct fb_variable *fb_addvar(struct f_fastboot *f_fb, const char *fmt, ...)
{
	struct fb_variable *var = xzalloc(sizeof(*var));
	va_list ap;

	va_start(ap, fmt);
	var->name = bvasprintf(fmt, ap);
	va_end(ap);

	list_add_tail(&var->list, &f_fb->variables);

	return var;
}

static int fastboot_add_partition_variables(struct f_fastboot *f_fb,
		struct file_list_entry *fentry)
{
	struct stat s;
	size_t size = 0;
	int fd, ret;
	struct mtd_info_user mtdinfo;
	char *type = NULL;
	struct fb_variable *var;

	ret = stat(fentry->filename, &s);
	if (ret) {
		if (fentry->flags & FILE_LIST_FLAG_CREATE) {
			ret = 0;
			type = "file";
			goto out;
		}

		goto out;
	}

	fd = open(fentry->filename, O_RDWR);
	if (fd < 0) {
		ret = -EINVAL;
		goto out;
	}

	size = s.st_size;

	ret = ioctl(fd, MEMGETINFO, &mtdinfo);

	close(fd);

	if (!ret) {
		switch (mtdinfo.type) {
		case MTD_NANDFLASH:
			type = "NAND-flash";
			break;
		case MTD_NORFLASH:
			type = "NOR-flash";
			break;
		case MTD_UBIVOLUME:
			type = "UBI";
			break;
		default:
			type = "flash";
			break;
		}

		goto out;
	}

	type = "basic";
	ret = 0;

out:
	if (ret)
		return ret;

	var = fb_addvar(f_fb, "partition-size:%s", fentry->name);
	fb_setvar(var, "%08zx", size);
	var = fb_addvar(f_fb, "partition-type:%s", fentry->name);
	fb_setvar(var, "%s", type);

	return ret;
}

static int fastboot_add_bbu_variables(struct bbu_handler *handler, void *ctx)
{
	struct f_fastboot *f_fb = ctx;
	char *name;
	int ret;

	name = basprintf("bbu-%s", handler->name);

	ret = file_list_add_entry(f_fb->files, name, handler->devicefile, 0);

	free(name);

	return ret;
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
	struct file_list_entry *fentry;
	struct fb_variable *var;

	f_fb->files = opts->files;

	var = fb_addvar(f_fb, "version");
	fb_setvar(var, "0.4");
	var = fb_addvar(f_fb, "bootloader-version");
	fb_setvar(var, release_string);
	var = fb_addvar(f_fb, "max-download-size");
	fb_setvar(var, "%u", fastboot_max_download_size);

	if (IS_ENABLED(CONFIG_BAREBOX_UPDATE) && opts->export_bbu)
		bbu_handlers_iterate(fastboot_add_bbu_variables, f_fb);

	file_list_for_each_entry(f_fb->files, fentry) {
		ret = fastboot_add_partition_variables(f_fb, fentry);
		if (ret)
			return ret;
	}

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
	f_fb->out_req->context = f_fb;

	ret = usb_assign_descriptors(f, fb_fs_descs, fb_hs_descs, NULL);
	if (ret)
		return ret;

	return 0;
}

static void fastboot_unbind(struct usb_configuration *c, struct usb_function *f)
{
	struct f_fastboot *f_fb = func_to_fastboot(f);
	struct fb_variable *var, *tmp;

	usb_ep_dequeue(f_fb->in_ep, f_fb->in_req);
	free(f_fb->in_req->buf);
	usb_ep_free_request(f_fb->in_ep, f_fb->in_req);
	f_fb->in_req = NULL;

	usb_ep_dequeue(f_fb->out_ep, f_fb->out_req);
	free(f_fb->out_req->buf);
	usb_ep_free_request(f_fb->out_ep, f_fb->out_req);
	f_fb->out_req = NULL;

	list_for_each_entry_safe(var, tmp, &f_fb->variables, list) {
		free(var->name);
		free(var->value);
		list_del(&var->list);
		free(var);
	}
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

	free(f_fb);
}

static struct usb_function *fastboot_alloc_func(struct usb_function_instance *fi)
{
	struct f_fastboot *f_fb;

	f_fb = xzalloc(sizeof(*f_fb));

	INIT_LIST_HEAD(&f_fb->variables);

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

static int fastboot_tx_write(struct f_fastboot *f_fb, const char *buffer, unsigned int buffer_size)
{
	struct usb_request *in_req = f_fb->in_req;
	uint64_t start;
	int ret;

	memcpy(in_req->buf, buffer, buffer_size);
	in_req->length = buffer_size;
	in_req_complete = 0;
	ret = usb_ep_queue(f_fb->in_ep, in_req);
	if (ret)
		pr_err("Error %d on queue\n", ret);

	start = get_time_ns();

	while (!in_req_complete) {
		if (is_timeout(start, 2 * SECOND))
			return -ETIMEDOUT;
		usb_gadget_poll();
	}

	return 0;
}

int fastboot_tx_print(struct f_fastboot *f_fb, const char *fmt, ...)
{
	char buf[64];
	va_list ap;
	int n;

	va_start(ap, fmt);
	n = vsnprintf(buf, 64, fmt, ap);
	va_end(ap);

	if (n > 64)
		n = 64;

	return fastboot_tx_write(f_fb, buf, n);
}

static void compl_do_reset(struct usb_ep *ep, struct usb_request *req)
{
	restart_machine();
}

static void cb_reboot(struct usb_ep *ep, struct usb_request *req, const char *cmd)
{
	struct f_fastboot *f_fb = req->context;

	f_fb->in_req->complete = compl_do_reset;
	fastboot_tx_print(f_fb, "OKAY");
}

static int strcmp_l1(const char *s1, const char *s2)
{
	if (!s1 || !s2)
		return -1;
	return strncmp(s1, s2, strlen(s1));
}

static void cb_getvar(struct usb_ep *ep, struct usb_request *req, const char *cmd)
{
	struct f_fastboot *f_fb = req->context;
	struct fb_variable *var;

	pr_debug("getvar: \"%s\"\n", cmd);

	if (!strcmp_l1(cmd, "all")) {
		list_for_each_entry(var, &f_fb->variables, list) {
			fastboot_tx_print(f_fb, "INFO%s: %s", var->name, var->value);
		}
		fastboot_tx_print(f_fb, "OKAY");
		return;
	}

	list_for_each_entry(var, &f_fb->variables, list) {
		if (!strcmp(cmd, var->name)) {
			fastboot_tx_print(f_fb, "OKAY%s", var->value);
			return;
		}
	}

	fastboot_tx_print(f_fb, "OKAY");
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

	ret = write(f_fb->download_fd, buffer, req->actual);
	if (ret < 0) {
		fastboot_tx_print(f_fb, "FAIL%s", strerror(-ret));
		return;
	}

	f_fb->download_bytes += req->actual;

	req->length = f_fb->download_size - f_fb->download_bytes;
	if (req->length > EP_BUFFER_SIZE)
		req->length = EP_BUFFER_SIZE;

	show_progress(f_fb->download_bytes);

	/* Check if transfer is done */
	if (f_fb->download_bytes >= f_fb->download_size) {
		req->complete = rx_handler_command;
		req->length = EP_BUFFER_SIZE;
		close(f_fb->download_fd);

		fastboot_tx_print(f_fb, "INFODownloading %d bytes finished",
				f_fb->download_bytes);

		fastboot_tx_print(f_fb, "OKAY");

		printf("\n");
	}

	req->actual = 0;
	usb_ep_queue(ep, req);
}

static void cb_download(struct usb_ep *ep, struct usb_request *req, const char *cmd)
{
	struct f_fastboot *f_fb = req->context;

	f_fb->download_size = simple_strtoul(cmd, NULL, 16);
	f_fb->download_bytes = 0;

	fastboot_tx_print(f_fb, "INFODownloading %d bytes...", f_fb->download_size);

	init_progression_bar(f_fb->download_size);

	f_fb->download_fd = open(FASTBOOT_TMPFILE, O_WRONLY | O_CREAT | O_TRUNC);
	if (f_fb->download_fd < 0) {
		fastboot_tx_print(f_fb, "FAILInternal Error");
		return;
	}

	if (!f_fb->download_size) {
		fastboot_tx_print(f_fb, "FAILdata invalid size");
	} else {
		fastboot_tx_print(f_fb, "DATA%08x", f_fb->download_size);
		req->complete = rx_handler_dl_image;
		req->length = EP_BUFFER_SIZE;
		if (req->length < ep->maxpacket)
			req->length = ep->maxpacket;
	}
}

static void do_bootm_on_complete(struct usb_ep *ep, struct usb_request *req)
{
	int ret;
	struct bootm_data data = {
		.initrd_address = UIMAGE_INVALID_ADDRESS,
		.os_address = UIMAGE_SOME_ADDRESS,
	};

	pr_info("Booting kernel..\n");

	globalvar_set_match("linux.bootargs.dyn.", "");
	globalvar_set_match("bootm.", "");

	data.os_file = xstrdup(FASTBOOT_TMPFILE);

	ret = bootm_boot(&data);
	if (ret)
		pr_err("Booting failed\n");
}

static void __maybe_unused cb_boot(struct usb_ep *ep, struct usb_request *req,
				   const char *opt)
{
	struct f_fastboot *f_fb = req->context;

	f_fb->in_req->complete = do_bootm_on_complete;
	fastboot_tx_print(f_fb, "OKAY");
}

static struct mtd_info *get_mtd(struct f_fastboot *f_fb, const char *filename)
{
	int fd, ret;
	struct mtd_info_user meminfo;

	fd = open(filename, O_RDONLY);
	if (fd < 0)
		return ERR_PTR(-errno);

	ret = ioctl(fd, MEMGETINFO, &meminfo);

	close(fd);

	if (ret)
		return ERR_PTR(ret);

	return meminfo.mtd;
}

static int do_ubiformat(struct f_fastboot *f_fb, struct mtd_info *mtd,
			const char *file)
{
	struct ubiformat_args args = {
		.yes = 1,
		.image = file,
	};

	if (!file)
		args.novtbl = 1;

	if (!IS_ENABLED(CONFIG_UBIFORMAT)) {
		fastboot_tx_print(f_fb, "FAILubiformat is not available");
		return -ENODEV;
	}

	return ubiformat(mtd, &args);
}


static int check_ubi(struct f_fastboot *f_fb, struct file_list_entry *fentry,
		     enum filetype filetype)
{
	struct mtd_info *mtd;

	mtd = get_mtd(f_fb, fentry->filename);

	/*
	 * Issue a warning when we are about to write a UBI image to a MTD device
	 * and the FILE_LIST_FLAG_UBI is not given as this means we loose all
	 * erase counters.
	 */
	if (!IS_ERR(mtd) && filetype == filetype_ubi &&
	    !(fentry->flags & FILE_LIST_FLAG_UBI)) {
		    fastboot_tx_print(f_fb, "INFOwriting UBI image to MTD device, "
					    "add the 'u' ");
		    fastboot_tx_print(f_fb, "INFOflag to the partition description");
		    return 0;
	}

	if (!(fentry->flags & FILE_LIST_FLAG_UBI))
		return 0;

	if (!IS_ENABLED(CONFIG_UBIFORMAT)) {
		fastboot_tx_print(f_fb, "FAILformat not available");
		return -ENOSYS;
	}

	if (IS_ERR(mtd)) {
		fastboot_tx_print(f_fb, "FAILUBI flag given on non-MTD device");
		return -EINVAL;
	}

	if (filetype == filetype_ubi) {
		fastboot_tx_print(f_fb, "INFOThis is an UBI image...");
		return 1;
	} else {
		fastboot_tx_print(f_fb, "FAILThis is no UBI image but %s",
			file_type_to_string(filetype));
		return -EINVAL;
	}
}

static int fastboot_handle_sparse(struct f_fastboot *f_fb,
				  struct file_list_entry *fentry)
{
	struct sparse_image_ctx *sparse;
	void *buf = NULL;
	int ret, fd;
	unsigned int flags = O_RDWR;
	int bufsiz = SZ_128K;
	struct stat s;
	struct mtd_info *mtd = NULL;

	ret = stat(fentry->filename, &s);
	if (ret) {
		if (fentry->flags & FILE_LIST_FLAG_CREATE)
			flags |= O_CREAT;
		else
			return ret;
	}

	fd = open(fentry->filename, flags);
	if (fd < 0)
		return -errno;

	ret = fstat(fd, &s);
	if (ret)
		goto out_close_fd;

	sparse = sparse_image_open(FASTBOOT_TMPFILE);
	if (IS_ERR(sparse)) {
		pr_err("Cannot open sparse image\n");
		ret = PTR_ERR(sparse);
		goto out_close_fd;
	}

	if (S_ISREG(s.st_mode)) {
		ret = ftruncate(fd, sparse_image_size(sparse));
		if (ret)
			goto out;
	}

	buf = malloc(bufsiz);
	if (!buf) {
		ret = -ENOMEM;
		goto out;
	}

	if (fentry->flags & FILE_LIST_FLAG_UBI) {
		mtd = get_mtd(f_fb, fentry->filename);
		if (IS_ERR(mtd)) {
			ret = PTR_ERR(mtd);
			goto out;
		}
	}

	while (1) {
		int retlen;
		loff_t pos;

		ret = sparse_image_read(sparse, buf, &pos, bufsiz, &retlen);
		if (ret)
			goto out;
		if (!retlen)
			break;

		if (pos == 0) {
			ret = check_ubi(f_fb, fentry, file_detect_type(buf, retlen));
			if (ret < 0)
				goto out;
		}

		if (fentry->flags & FILE_LIST_FLAG_UBI) {
			if (!IS_ENABLED(CONFIG_UBIFORMAT)) {
				ret = -ENOSYS;
				goto out;
			}

			if (pos == 0) {
				ret = do_ubiformat(f_fb, mtd, NULL);
				if (ret)
					goto out;
			}

			ret = ubiformat_write(mtd, buf, retlen, pos);
			if (ret)
				goto out;
		} else {
			pos = lseek(fd, pos, SEEK_SET);
			if (pos == -1) {
				ret = -errno;
				goto out;
			}

			ret = write_full(fd, buf, retlen);
			if (ret < 0)
				goto out;
		}
	}

	ret = 0;

out:
	free(buf);
	sparse_image_close(sparse);
out_close_fd:
	close(fd);

	return ret;
}

static void cb_flash(struct usb_ep *ep, struct usb_request *req, const char *cmd)
{
	struct f_fastboot *f_fb = req->context;
	struct file_list_entry *fentry;
	int ret;
	const char *filename = NULL;
	enum filetype filetype = file_name_detect_type(FASTBOOT_TMPFILE);

	fastboot_tx_print(f_fb, "INFOCopying file to %s...", cmd);

	fentry = file_list_entry_by_name(f_fb->files, cmd);

	if (!fentry) {
		fastboot_tx_print(f_fb, "FAILNo such partition: %s", cmd);
		return;
	}

	filename = fentry->filename;

	if (filetype == filetype_android_sparse) {
		ret = fastboot_handle_sparse(f_fb, fentry);
		if (ret) {
			fastboot_tx_print(f_fb, "FAILwriting sparse image: %s",
					  strerror(-ret));
			return;
		}

		goto out;
	}

	ret = check_ubi(f_fb, fentry, filetype);
	if (ret < 0)
		return;

	if (ret > 0) {
		struct mtd_info *mtd;

		mtd = get_mtd(f_fb, fentry->filename);

		ret = do_ubiformat(f_fb, mtd, FASTBOOT_TMPFILE);
		if (ret) {
			fastboot_tx_print(f_fb, "FAILwrite partition: %s", strerror(-ret));
			return;
		}

		goto out;
	}

	if (IS_ENABLED(CONFIG_BAREBOX_UPDATE) && filetype_is_barebox_image(filetype)) {
		struct bbu_data data = {
			.devicefile = filename,
			.imagefile = FASTBOOT_TMPFILE,
			.flags = BBU_FLAG_YES,
		};

		if (!barebox_update_handler_exists(&data))
			goto copy;

		fastboot_tx_print(f_fb, "INFOThis is a barebox image...");

		data.image = read_file(data.imagefile, &data.len);
		if (!data.image) {
			fastboot_tx_print(f_fb, "FAILreading barebox");
			return;
		}

		ret = barebox_update(&data);

		free(data.image);

		if (ret) {
			fastboot_tx_print(f_fb, "FAILupdate barebox: %s", strerror(-ret));
			return;
		}

		goto out;
	}

copy:
	ret = copy_file(FASTBOOT_TMPFILE, filename, 1);

	unlink(FASTBOOT_TMPFILE);

	if (ret) {
		fastboot_tx_print(f_fb, "FAILwrite partition: %s", strerror(-ret));
		return;
	}

out:
	fastboot_tx_print(f_fb, "OKAY");
}

static void cb_erase(struct usb_ep *ep, struct usb_request *req, const char *cmd)
{
	struct f_fastboot *f_fb = req->context;
	struct file_list_entry *fentry;
	int ret;
	const char *filename = NULL;
	int fd;

	fastboot_tx_print(f_fb, "INFOErasing %s...", cmd);

	file_list_for_each_entry(f_fb->files, fentry) {
		if (!strcmp(cmd, fentry->name)) {
			filename = fentry->filename;
			break;
		}
	}

	if (!filename) {
		fastboot_tx_print(f_fb, "FAILNo such partition: %s", cmd);
		return;
	}

	fd = open(filename, O_RDWR);
	if (fd < 0)
		fastboot_tx_print(f_fb, "FAIL%s", strerror(-fd));

	ret = erase(fd, ERASE_SIZE_ALL, 0);

	close(fd);

	if (ret)
		fastboot_tx_print(f_fb, "FAILcannot erase partition %s: %s",
				filename, strerror(-ret));
	else
		fastboot_tx_print(f_fb, "OKAY");
}

struct cmd_dispatch_info {
	char *cmd;
	void (*cb)(struct usb_ep *ep, struct usb_request *req, const char *opt);
};

static void fb_run_command(struct usb_ep *ep, struct usb_request *req, const char *cmd,
		const struct cmd_dispatch_info *cmds, int num_commands)
{
	void (*func_cb)(struct usb_ep *ep, struct usb_request *req, const char *cmd) = NULL;
	struct f_fastboot *f_fb = req->context;
	int i;

	console_countdown_abort();

	for (i = 0; i < num_commands; i++) {
		if (!strcmp_l1(cmds[i].cmd, cmd)) {
			func_cb = cmds[i].cb;
			cmd += strlen(cmds[i].cmd);
			func_cb(ep, req, cmd);
			return;
		}
	}

	fastboot_tx_print(f_fb, "FAILunknown command %s", cmd);
}

static void cb_oem_getenv(struct usb_ep *ep, struct usb_request *req, const char *cmd)
{
	struct f_fastboot *f_fb = req->context;
	const char *value;

	pr_debug("%s: \"%s\"\n", __func__, cmd);

	value = getenv(cmd);

	fastboot_tx_print(f_fb, "INFO%s", value ? value : "");
	fastboot_tx_print(f_fb, "OKAY");
}

static void cb_oem_setenv(struct usb_ep *ep, struct usb_request *req, const char *cmd)
{
	struct f_fastboot *f_fb = req->context;
	char *var = xstrdup(cmd);
	char *value;
	int ret;

	pr_debug("%s: \"%s\"\n", __func__, cmd);

	value = strchr(var, '=');
	if (!value) {
		ret = -EINVAL;
		goto out;
	}

	*value++ = 0;

	ret = setenv(var, value);
	if (ret)
		goto out;

	fastboot_tx_print(f_fb, "OKAY");
out:
	free(var);

	if (ret)
		fastboot_tx_print(f_fb, "FAIL%s", strerror(-ret));
}

static void cb_oem_exec(struct usb_ep *ep, struct usb_request *req, const char *cmd)
{
	struct f_fastboot *f_fb = req->context;
	int ret;

	if (!IS_ENABLED(CONFIG_COMMAND_SUPPORT)) {
		fastboot_tx_print(f_fb, "FAILno command support available");
		return;
	}

	ret = run_command(cmd);
	if (ret < 0)
		fastboot_tx_print(f_fb, "FAIL%s", strerror(-ret));
	else if (ret > 0)
		fastboot_tx_print(f_fb, "FAIL");
	else
		fastboot_tx_print(f_fb, "OKAY");
}

static const struct cmd_dispatch_info cmd_oem_dispatch_info[] = {
	{
		.cmd = "getenv ",
		.cb = cb_oem_getenv,
	}, {
		.cmd = "setenv ",
		.cb = cb_oem_setenv,
	}, {
		.cmd = "exec ",
		.cb = cb_oem_exec,
	},
};

static void cb_oem(struct usb_ep *ep, struct usb_request *req, const char *cmd)
{
	pr_debug("%s: \"%s\"\n", __func__, cmd);

	fb_run_command(ep, req, cmd, cmd_oem_dispatch_info, ARRAY_SIZE(cmd_oem_dispatch_info));
}

static const struct cmd_dispatch_info cmd_dispatch_info[] = {
	{
		.cmd = "reboot",
		.cb = cb_reboot,
	}, {
		.cmd = "getvar:",
		.cb = cb_getvar,
	}, {
		.cmd = "download:",
		.cb = cb_download,
#if defined(CONFIG_BOOTM)
	}, {
		.cmd = "boot",
		.cb = cb_boot,
#endif
	}, {
		.cmd = "flash:",
		.cb = cb_flash,
	}, {
		.cmd = "erase:",
		.cb = cb_erase,
	}, {
		.cmd = "oem ",
		.cb = cb_oem,
	},
};

static void rx_handler_command(struct usb_ep *ep, struct usb_request *req)
{
	char *cmdbuf = req->buf;

	if (req->status != 0)
		return;

	*(cmdbuf + req->actual) = 0;

	fb_run_command(ep, req, cmdbuf, cmd_dispatch_info,
				ARRAY_SIZE(cmd_dispatch_info));

	*cmdbuf = '\0';
	req->actual = 0;
	memset(req->buf, 0, EP_BUFFER_SIZE);
	usb_ep_queue(ep, req);
}

static int fastboot_globalvars_init(void)
{
	globalvar_add_simple_int("usbgadget.fastboot_max_download_size",
				 &fastboot_max_download_size, "%u");

	return 0;
}

device_initcall(fastboot_globalvars_init);

BAREBOX_MAGICVAR_NAMED(global_usbgadget_fastboot_max_download_size,
		       global.usbgadget.fastboot_max_download_size,
		       "Fastboot maximum download size");
