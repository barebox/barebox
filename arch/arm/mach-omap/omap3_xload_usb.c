/*
 * Copyright (c) 2015 Sascha Hauer <s.hauer@pengutronix.de>, Pengutronix
 *
 * Based on a patch by:
 *
 * Copyright (C) 2011 Rick Bronson
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <io.h>
#include <malloc.h>
#include <mach/omap3-silicon.h>
#include <mach/omap3-generic.h>

static void __iomem *omap3_usb_base = (void __iomem *)OMAP3_MUSB0_BASE;

#define OMAP34XX_USB_EP(n)         (omap3_usb_base + 0x100 + 0x10 * (n))

#define MUSB_RXCSR		0x06
#define MUSB_RXCOUNT		0x08
#define MUSB_TXCSR		0x02
#define MUSB_FIFOSIZE		0x0F
#define OMAP34XX_USB_RXCSR(n)      (OMAP34XX_USB_EP(n) + MUSB_RXCSR)
#define OMAP34XX_USB_RXCOUNT(n)    (OMAP34XX_USB_EP(n) + MUSB_RXCOUNT)
#define OMAP34XX_USB_TXCSR(n)      (OMAP34XX_USB_EP(n) + MUSB_TXCSR)
#define OMAP34XX_USB_FIFOSIZE(n)   (OMAP34XX_USB_EP(n) + MUSB_FIFOSIZE)
#define OMAP34XX_USB_FIFO(n)       (omap3_usb_base + 0x20 + ((n) * 4))

/* memory mapped registers */
#define BULK_ENDPOINT 1
#define MUSB_RXCSR_RXPKTRDY		0x0001
#define MUSB_TXCSR_TXPKTRDY		0x0001

#define PACK4(a,b,c,d)     (((d) << 24) | ((c) << 16) | ((b) << 8) | (a))
#define USBLOAD_CMD_FILE PACK4('U', 'S', 'B', 's')  /* send file size */
#define USBLOAD_CMD_JUMP PACK4('U', 'S', 'B', 'j')  /* go where I tell you */
#define USBLOAD_CMD_FILE_REQ PACK4('U', 'S', 'B', 'f')  /* file request */
#define USBLOAD_CMD_ECHO_SZ PACK4('U', 'S', 'B', 'n')  /* echo file size */
#define USBLOAD_CMD_REPORT_SZ PACK4('U', 'S', 'B', 'o')  /* report file size */
#define USBLOAD_CMD_MESSAGE PACK4('U', 'S', 'B', 'm')  /* message for debug */

static int usb_send(unsigned char *buffer, unsigned int buffer_size)
{
	unsigned int cntr;
	u16 txcsr;
	void __iomem *reg = (void *)OMAP34XX_USB_TXCSR(BULK_ENDPOINT);
	void __iomem *bulk_fifo = (void *)OMAP34XX_USB_FIFO(BULK_ENDPOINT);

	txcsr = readw(reg);

	if (txcsr & MUSB_TXCSR_TXPKTRDY)
		return 0;

	for (cntr = 0; cntr < buffer_size; cntr++)
		writeb(buffer[cntr], bulk_fifo);

	txcsr = readw(reg);
	txcsr |= MUSB_TXCSR_TXPKTRDY;
	writew(txcsr, reg);

	return buffer_size;
}

static int usb_recv(u8 *buffer)
{
	int cntr;
	u16 count = 0;
	u16 rxcsr;
	void __iomem *reg = (void *)OMAP34XX_USB_RXCSR(BULK_ENDPOINT);
	void __iomem *bulk_fifo = (void *)OMAP34XX_USB_FIFO(BULK_ENDPOINT);

	rxcsr = readw(reg);

	if (!(rxcsr & MUSB_RXCSR_RXPKTRDY))
		return 0;

	count = readw((void *)OMAP34XX_USB_RXCOUNT(BULK_ENDPOINT));
	for (cntr = 0; cntr < count; cntr++)
		*buffer++ = readb(bulk_fifo);

	/* Clear the RXPKTRDY bit */
	rxcsr = readw(reg);
	rxcsr &= ~MUSB_RXCSR_RXPKTRDY;
	writew(rxcsr, reg);

	return count;
}

static unsigned char usb_outbuffer[64];

static void usb_msg(unsigned int cmd, const char *msg)
{
	unsigned char *p_char = usb_outbuffer;

	*(int *)p_char = cmd;

	p_char += sizeof(cmd);

	if (msg) {
		while (*msg)
			*p_char++= *msg++;
		*p_char++= 0;
	}

	usb_send(usb_outbuffer, p_char - usb_outbuffer);
}

static void usb_code(unsigned int cmd, u32 code)
{
	unsigned int *p_int = (unsigned int *)usb_outbuffer;

	*p_int++ = cmd;
	*p_int++ = code;
	usb_send (usb_outbuffer, ((unsigned char *) p_int) - usb_outbuffer);
}

void *omap3_xload_boot_usb(void)
{
	int res;
	void *buf;
	u32 *buf32;
	u32 total;
	void *addr;
	u32 bytes;
	int size;
	int cntr;
	void *fn;
	u8 __buf[512];

	buf32 = buf = __buf;

	usb_msg (USBLOAD_CMD_FILE_REQ, "file req");
	for (cntr = 0; cntr < 10000000; cntr++)  {
		size = usb_recv(buf);
		if (!size)
			continue;

		switch (buf32[0]) {
		case USBLOAD_CMD_FILE:
			pr_debug ("USBLOAD_CMD_FILE total = %d size = 0x%x addr = 0x%x\n",
					res, buf32[1], buf32[2]);
			total = buf32[1];  /* get size and address */
			addr = (void *)buf32[2];
			usb_code(USBLOAD_CMD_ECHO_SZ, total);

			bytes = 0;

			while (bytes < total) {
				size = usb_recv(buf);
				memcpy(addr, buf, size);
				addr += size;
				bytes += size;
			}

			usb_code(USBLOAD_CMD_REPORT_SZ, total);  /* tell him we got this many bytes */
			usb_msg (USBLOAD_CMD_FILE_REQ, "file req");  /* see if they have another file for us */
			break;
		case USBLOAD_CMD_JUMP:
			pr_debug("USBLOAD_CMD_JUMP total = %d addr = 0x%x val = 0x%x\n",
					res, buf32[0], buf32[1]);
			fn = (void *)buf32[1];
			goto out;
		default:
			break;
		}
	}

	fn = NULL;
out:

	return fn;
}
