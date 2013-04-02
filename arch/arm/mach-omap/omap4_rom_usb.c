/*
 * This code is based on:
 * git://github.com/swetland/omap4boot.git
 */
/*
 * Copyright (C) 2010 The Android Open Source Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <common.h>
#include <mach/omap4-silicon.h>
#include <mach/omap4_rom_usb.h>
#include <mach/generic.h>
#include <init.h>

static struct omap4_usbboot omap4_usbboot_data;

int omap4_usbboot_open(void)
{
	int (*rom_get_per_driver)(struct per_driver **io, u32 device_type);
	int (*rom_get_per_device)(struct per_handle **rh);
	struct per_handle *boot;
	int n;
	u32 base;

	if (omap4_revision() >= OMAP4460_ES1_0)
		base = PUBLIC_API_BASE_4460;
	else
		base = PUBLIC_API_BASE_4430;

	rom_get_per_driver = API(base + PUBLIC_GET_DRIVER_PER_OFFSET);
	rom_get_per_device = API(base + PUBLIC_GET_DEVICE_PER_OFFSET);

	n = rom_get_per_device(&boot);
	if (n)
		return n;

	if ((boot->device_type != DEVICE_USB) &&
	    (boot->device_type != DEVICE_USBEXT))
		return 0;

	memset(&omap4_usbboot_data, 0, sizeof(omap4_usbboot_data));
	n = rom_get_per_driver(&omap4_usbboot_data.io, boot->device_type);
	if (n)
		return n;

	omap4_usbboot_data.dread.status = -1;
	omap4_usbboot_data.dread.xfer_mode = boot->xfer_mode;
	omap4_usbboot_data.dread.options = boot->options;
	omap4_usbboot_data.dread.device_type = boot->device_type;

	omap4_usbboot_data.dwrite.status = -1;
	omap4_usbboot_data.dwrite.xfer_mode = boot->xfer_mode;
	omap4_usbboot_data.dwrite.options = boot->options;
	omap4_usbboot_data.dwrite.device_type = boot->device_type;
	__asm__ __volatile__ ("cpsie i\n");
	omap4_usbboot_data.ready = 1;

	omap4_usbboot_puts("USB communications initialized\n");
	return 0;
}
core_initcall(omap4_usbboot_open);

int omap4_usbboot_ready(void){
	return omap4_usbboot_data.ready;
}

static void rom_read_callback(struct per_handle *rh)
{
	omap4_usbboot_data.dread.status = rh->status;
	return;
}

void omap4_usbboot_queue_read(void *data, unsigned len)
{
	int n;
	omap4_usbboot_data.dread.data = data;
	omap4_usbboot_data.dread.length = len;
	omap4_usbboot_data.dread.status = STATUS_WAITING;
	omap4_usbboot_data.dread.xfer_mode = 1;
	omap4_usbboot_data.dread.callback = rom_read_callback;
	n = omap4_usbboot_data.io->read(&omap4_usbboot_data.dread);
	if (n)
		omap4_usbboot_data.dread.status = n;
}

int omap4_usbboot_wait_read(void)
{
	int ret;
	while (omap4_usbboot_data.dread.status == STATUS_WAITING)
		/* cpu_relax(); */
		barrier();
	ret = omap4_usbboot_data.dread.status;
	omap4_usbboot_data.dread.status = -1;
	return ret;
}

int omap4_usbboot_is_read_waiting(void)
{
	barrier();
	return omap4_usbboot_data.dread.status == STATUS_WAITING;
}

int omap4_usbboot_is_read_ok(void)
{
	barrier();
	return omap4_usbboot_data.dread.status == STATUS_OKAY;
}

static void rom_write_callback(struct per_handle *rh)
{
	omap4_usbboot_data.dwrite.status = rh->status;
	return;
}

void omap4_usbboot_queue_write(void *data, unsigned len)
{
	int n;
	omap4_usbboot_data.dwrite.data = data;
	omap4_usbboot_data.dwrite.length = len;
	omap4_usbboot_data.dwrite.status = STATUS_WAITING;
	omap4_usbboot_data.dwrite.xfer_mode = 1;
	omap4_usbboot_data.dwrite.callback = rom_write_callback;
	n = omap4_usbboot_data.io->write(&omap4_usbboot_data.dwrite);
	if (n)
		omap4_usbboot_data.dwrite.status = n;
}

int omap4_usbboot_wait_write(void)
{
	int ret;
	while (omap4_usbboot_data.dwrite.status == STATUS_WAITING)
		/* cpu_relax(); */
		barrier();
	ret = omap4_usbboot_data.dwrite.status;
	omap4_usbboot_data.dwrite.status = -1;
	return ret;
}

#define USB_MAX_IO 65536
int omap4_usbboot_read(void *data, unsigned len)
{
	unsigned xfer;
	unsigned char *x = data;
	int n;
	while (len > 0) {
		xfer = (len > USB_MAX_IO) ? USB_MAX_IO : len;
		omap4_usbboot_queue_read(x, xfer);
		n = omap4_usbboot_wait_read();
		if (n)
			return n;
		x += xfer;
		len -= xfer;
	}
	return 0;
}

int omap4_usbboot_write(void *data, unsigned len)
{
	omap4_usbboot_queue_write(data, len);
	return omap4_usbboot_wait_write();
}

void omap4_usbboot_close(void)
{
	omap4_usbboot_data.io->close(&omap4_usbboot_data.dread);
}

void omap4_usbboot_puts(const char *s)
{
	u32 c;
	while ((c = *s++))
		omap4_usbboot_write(&c, 4);
}
