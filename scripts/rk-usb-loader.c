// SPDX-License-Identifier: GPL-2.0-or-later

/*
 * rk-usb-loader: A tool to USB Bootstrap Rockchip SoCs
 *
 * This tool bootstraps Rockchip SoCs via USB. It is known to work
 * on these SoCs:
 *
 * - RK3568
 * - RK3566
 *
 * rk-usb-loader takes the barebox images the barebox build process
 * generates as input. The upload protocol has been taken from the
 * rkdevelop tool, but it's not a full replacement of that tool.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <libusb.h>

#include "common.h"
#include "common.c"
#include "rockchip.h"

static void log_error(char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	fprintf(stdout, "[-] ");
	vfprintf(stdout, fmt, va);
	va_end(va);
}

static void log_info(char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	fprintf(stdout, "[+] ");
	vfprintf(stdout, fmt, va);
	va_end(va);
}

static int debug;

static void log_debug(char *fmt, ...)
{
	va_list va;

	if (!debug)
		return;

	va_start(va, fmt);
	fprintf(stdout, "[D] ");
	vfprintf(stdout, fmt, va);
	va_end(va);
}

static libusb_device_handle *rk_usb_open(libusb_context *ctx,
					 uint16_t vendor, uint16_t vmask,
					 uint16_t product, uint16_t pmask)
{
	libusb_device **devlist;
	libusb_device_handle *handle;
	struct libusb_device_descriptor desc;
	ssize_t count, i;
	int ret;

	log_info("scanning for USB device matching %04hx/%04hx:%04hx/%04hx...\n",
		 vendor, vmask, product, pmask);

	while (1) {
		if ((count = libusb_get_device_list(ctx, &devlist)) < 0) {
			log_error("failed to gather USB device list: %s\n",
				  libusb_error_name(count));
			return NULL;
		}

		for (i = 0; i < count; i++) {
			ret = libusb_get_device_descriptor(devlist[i], &desc);
			if (ret < 0) {
				log_error("failed to get USB device descriptor: %s\n",
						libusb_error_name(ret));
				libusb_free_device_list(devlist, 1);
				return NULL;
			}

			if ((desc.idVendor & vmask) != (vendor & vmask))
				continue;

			if (product) {
				if ((desc.idProduct & pmask) != (product & pmask))
					continue;
				goto found;
			}
		}

		libusb_free_device_list(devlist, 1);

		/* nothing found yet. have a 10ms nap */
		usleep(10000);
	}
found:

	ret = libusb_open(devlist[i], &handle);
	if (ret < 0) {
		log_error("failed to open USB device %04hx:%04hx: %s\n",
				desc.idVendor, desc.idProduct,
				libusb_error_name(ret));
		libusb_free_device_list(devlist, 1);
		return NULL;
	}

	ret = libusb_claim_interface(handle, 0);
	if (ret) {
		printf("Claim failed\n");
		return NULL;
	}

	log_info("successfully opened %04hx:%04hx\n",
		 desc.idVendor, desc.idProduct);

	return handle;
}

#define poly16_CCITT    0x1021          /* crc-ccitt mask */

static uint16_t crc_calculate(uint16_t crc, unsigned char ch)
{
	unsigned int i;

	for (i = 0x80; i != 0; i >>= 1) {
		if (crc & 0x8000) {
			crc <<= 1;
			crc ^= poly16_CCITT;
		} else {
			crc <<= 1;
		}

		if (ch & i)
			crc ^= poly16_CCITT;
	}
	return crc;
}

static uint16_t crc_ccitt(unsigned char *p, int n)
{
	uint16_t crc = 0xffff;

	while (n--) {
		crc = crc_calculate(crc, *p);
		p++;
	}

	return crc;
}

static int upload(libusb_device_handle *dev, unsigned int dwRequest, void *buf, int n_bytes)
{
	uint16_t crc;
	uint8_t *data;
	int sent = 0, ret;

	data = calloc(n_bytes + 5, 1);
	memcpy(data, buf, n_bytes);

	crc = crc_ccitt(data, n_bytes);
	data[n_bytes] = (crc & 0xff00) >> 8;
	data[n_bytes + 1] = crc & 0x00ff;
	n_bytes += 2;

	while (sent < n_bytes) {
		int now;

		if (n_bytes - sent > 4096)
			now = 4096;
		else
			now = n_bytes - sent;

		ret = libusb_control_transfer(dev, 0x40, 0xC, 0, dwRequest,
					      data + sent, now, 0);
		if (ret != now) {
			log_error("DeviceRequest 0x%x failed, err=%d",
				  dwRequest, ret);

			ret = -EIO;
			goto err;
		}
		sent += now;
	}

	ret = 0;
err:
	free(data);

	return ret;
}

static int upload_image(const char *filename)
{
	libusb_context *ctx;
	libusb_device_handle *dev;
	int ret;
	void *buf;
	struct newidb *hdr;
	int i, n_files;
	size_t size;

	buf = read_file(filename, &size);
	if (!buf)
		exit(1);

	hdr = buf;

	if (hdr->magic != NEWIDB_MAGIC) {
		log_error("%s has invalid magic 0x%08x ( != 0x%08x )\n", filename,
			  hdr->magic, NEWIDB_MAGIC);
		exit(1);
	}

	ret = libusb_init(&ctx);
	if (ret < 0) {
		log_error("failed to initialize libusb context: %s\n",
			  libusb_error_name(ret));
		return ret;
	}

	dev = rk_usb_open(ctx, 0x2207, 0xffff, 0x350a, 0xfffe);
	if (!dev) {
		libusb_exit(ctx);
		return 1;
	}

	n_files = hdr->n_files >> 16;

	if (n_files > 2) {
		/*
		 * This tool is designed for barebox images generated with rkimage.
		 * These have one blob containing the SDRAM setup sent with the
		 * CODE471_OPTION and one blob containing the barebox image sent with
		 * the CODE472_OPTION.
		 */
		log_error("Invalid image with %d blobs\n", n_files);
		ret = -EINVAL;
		goto err;
	}

	for (i = 0; i < n_files; i++) {
		struct newidb_entry *entry = &hdr->entries[i];
		int foffset, fsize, wIndex;

		if (i)
			wIndex = 0x472;
		else
			wIndex = 0x471;

		log_info("Uploading %d/%d\n", i + 1, n_files);

		foffset = (entry->sector & 0xffff) * RK_SECTOR_SIZE;
		fsize = (entry->sector >> 16) * RK_SECTOR_SIZE;

		log_debug("image starting at offset 0x%08x, size 0x%08x\n", foffset, fsize);

		ret = upload(dev, wIndex, buf + foffset, fsize);
		if (ret)
			goto err;
	}

	ret = 0;
err:
	libusb_close(dev);
	libusb_exit(ctx);

	return ret;
}

static void usage(const char *prgname)
{
	printf(
"Usage: %s [OPTIONS] <IMAGE>\n"
"\n"
"Options:\n"
"  -d          Enable debugging output\n"
"  -h          This help\n",
	prgname);
}

static struct option cbootcmd[] = {
	{"debug", 0, NULL, 'd'},
	{"help", 0, NULL, 'h'},
	{0, 0, 0, 0},
};

int main(int argc, char **argv)
{
	int opt, ret;
	const char *filename;

	while ((opt = getopt_long(argc, argv, "hd", cbootcmd, NULL)) > 0) {
		switch (opt) {
		case 'h':
			usage(argv[0]);
			exit(0);
		case 'd':
			debug = 1;
			break;
		}
	}

	if (argc == optind) {
		usage(argv[0]);
		exit(1);
	}

	filename = argv[optind];

	ret = upload_image(filename);
	if (ret)
		exit(1);

	exit(0);
}
