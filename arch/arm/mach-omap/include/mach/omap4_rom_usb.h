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

#ifndef _OMAP4_ROM_USB_H_
#define _OMAP4_ROM_USB_H_

/* public api */
#define PUBLIC_API_BASE_4430		(0x28400)
#define PUBLIC_API_BASE_4460		(0x30400)

#define PUBLIC_GET_DRIVER_MEM_OFFSET (0x04)
#define PUBLIC_GET_DRIVER_PER_OFFSET (0x08)
#define PUBLIC_GET_DEVICE_MEM_OFFSET (0x80)
#define PUBLIC_GET_DEVICE_PER_OFFSET (0x84)

#define DEVICE_NULL	0x40
#define DEVICE_UART1	0x41
#define DEVICE_UART2	0x42
#define DEVICE_UART3	0x43
#define DEVICE_UART4	0x44
#define DEVICE_USB	0x45
#define DEVICE_USBEXT	0x46

#define XFER_MODE_CPU 0
#define XFER_MODE_DMA 1

#define STATUS_OKAY		0
#define STATUS_FAILED		1
#define STATUS_TIMEOUT		2
#define STATUS_BAD_PARAM	3
#define STATUS_WAITING		4
#define STATUS_NO_MEMORY	5
#define STATUS_INVALID_PTR	6

/* Memory ROM interface */
struct read_desc {
	u32 sector_start;
	u32 sector_count;
	void *destination;
};

struct mem_device {
	u32 initialized;
	u8 device_type;
	u8 trials_count;
	u32 xip_device;
	u16 search_size;
	u32 base_address;
	u16 hs_toc_mask;
	u16 gp_toc_mask;
	void *device_data;
	u16 *boot_options;
};

struct mem_driver {
	int (*init)(struct mem_device *md);
	int (*read)(struct mem_device *md, struct read_desc *rd);
	int (*configure)(struct mem_device *md, void *config);
};


/* Peripheral ROM interface */
struct per_handle {
	void *set_to_null;
	void (*callback)(struct per_handle *rh);
	void *data;
	u32 length;
	u16 *options;
	u32 xfer_mode;
	u32 device_type;
	u32 status;
	u16 hs_toc_mask;
	u16 gp_toc_mask;
	u32 config_timeout;
};

struct per_driver {
	int (*init)(struct per_handle *rh);
	int (*read)(struct per_handle *rh);
	int (*write)(struct per_handle *rh);
	int (*close)(struct per_handle *rh);
	int (*config)(struct per_handle *rh, void *x);
};

#define USB_SETCONFIGDESC_ATTRIBUTES      (0)
#define USB_SETCONFIGDESC_MAXPOWER        (1)
#define USB_SETSUSPEND_CALLBACK           (2)
struct per_usb_config {
	u32 configid;
	u32 value;
};

#define API(n) ((void *) (*((u32 *) (n))))
/* ROM API End */

struct omap4_usbboot {
	struct per_handle dread;
	struct per_handle dwrite;
	struct per_driver *io;
	int ready;
};

int omap4_usbboot_open(void);
int omap4_usbboot_ready(void);
void omap4_usbboot_close(void);

void omap4_usbboot_queue_read(void *data, unsigned len);
int omap4_usbboot_wait_read(void);
int omap4_usbboot_is_read_waiting(void);
int omap4_usbboot_is_read_ok(void);

void omap4_usbboot_queue_write(void *data, unsigned len);
int omap4_usbboot_wait_write(void);

int omap4_usbboot_read(void *data, unsigned len);
int omap4_usbboot_write(void *data, unsigned len);
void omap4_usbboot_puts(const char *s);

#endif
