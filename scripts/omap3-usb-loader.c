/*
 * OMAP Loader, a USB uploader application targeted at OMAP3 processors
 * Copyright (C) 2008 Martin Mueller <martinmm@pfump.org>
 * Copyright (C) 2014 Grant Hernandez <grant.h.hernandez@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>

/*
 * Reasons for the name change: this is a complete rewrite of
 * the unversioned omap3_usbload so to lower ambiguity the name was changed.
 * The GPLv2 license specifies rewrites as derived work.
 */
#define PROG_NAME "OMAP Loader"
#define VERSION "1.0.0"

#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#define OMAP_IS_BIG_ENDIAN
#endif

#ifdef OMAP_IS_BIG_ENDIAN
#include <arpa/inet.h>
#endif

#include <unistd.h>		/* for usleep and friends */
#include <getopt.h>
#include <errno.h>
#include <libgen.h>		/* for basename */

#include <libusb-1.0/libusb.h>		/* the main event */

/* Device specific defines (OMAP)
 * Primary source: http://www.ti.com/lit/pdf/sprugn4
 * Section 26.4.5 "Peripheral Booting"
 */
#define OMAP_BASE_ADDRESS 0x40200000
#define OMAP_PERIPH_BOOT 0xF0030002
#define OMAP_VENDOR_ID 0x0451
#define OMAP_PRODUCT_ID 0xD00E
/* TODO: dynamically discover these endpoints */
#define OMAP_USB_BULK_IN 0x81
#define OMAP_USB_BULK_OUT 0x01
#define OMAP_ASIC_ID_LEN 69

#ifdef OMAP_IS_BIG_ENDIAN
#define cpu_to_le32(v) (((v & 0xff) << 24) | ((v & 0xff00) << 8) | \
    ((v & 0xff0000) >> 8) | ((v & 0xff000000) >> 24))
#define le32_to_cpu(v) cpu_to_le32(v)
#else
#define cpu_to_le32(v) (v)
#define le32_to_cpu(v) (v)
#endif

/*
 * taken from x-loader/drivers/usb/usb.c
 * All credit to Martin Mueller
 */
#define PACK4(a,b,c,d) (((d)<<24) | ((c)<<16) | ((b)<<8) | (a))
#define USBLOAD_CMD_FILE PACK4('U','S','B','s')		/* file size request */
#define USBLOAD_CMD_FILE_REQ PACK4('U','S','B','f')	/* file size resp */
#define USBLOAD_CMD_JUMP PACK4('U','S','B','j')		/* execute code here */
#define USBLOAD_CMD_ECHO_SZ PACK4('U','S','B','n')	/* file size confirm to */
#define USBLOAD_CMD_REPORT_SZ PACK4('U','S','B','o')	/* confirm full file */
#define USBLOAD_CMD_MESSAGE PACK4('U','S','B','m')	/* debug message */

/* USB transfer characteristics */
#define USB_MAX_WAIT 5000
#define USB_TIMEOUT 1000
#define USB_MAX_TIMEOUTS (USB_MAX_WAIT/USB_TIMEOUT)

/* stores the data and attributes of a file to upload in to memory */
struct file_upload {
	size_t size;
	void *data;
	uint32_t addr;
	char *path;
	char *basename;
};

/* stores all of the arguments read in by getopt in main() */
struct arg_state {
	struct file_upload **files;
	int numfiles;
	uint32_t jumptarget;
	uint16_t vendor, product;
};

static int g_verbose = 0;

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

static bool omap_usb_read(libusb_device_handle *handle, unsigned char *data,
	      int length, int *actuallength)
{
	int ret = 0;
	int iter = 0;
	int sizeleft = length;

	if (!actuallength)
		return false;

	while (sizeleft > 0) {
		int actualread = 0;
		int readamt = sizeleft;

		ret = libusb_bulk_transfer(handle, OMAP_USB_BULK_IN, data + iter,
					 readamt, &actualread, USB_TIMEOUT);

		if (ret == LIBUSB_ERROR_TIMEOUT) {
			sizeleft -= actualread;
			iter += actualread;

			/* we got some data, lets cut our losses and stop here */
			if (iter > 0)
				break;

			log_error("device timed out while transfering in %d bytes (got %d)\n",
					length, iter);

			return false;
		} else if (ret == LIBUSB_SUCCESS) {
			/* we cant trust actualRead on anything but a timeout or success */
			sizeleft -= actualread;
			iter += actualread;

			/* stop at the first sign of data */
			if (iter > 0)
				break;
		} else {
			log_error("fatal transfer error (BULK_IN) for %d bytes (got %d): %s\n",
					length, iter, libusb_error_name(ret));
			return false;
		}
	}

	*actuallength = iter;

	return true;
}

static bool omap_usb_write(libusb_device_handle *handle, void *data, int length)
{
	int ret = 0;
	int numtimeouts = 0;
	int iter = 0;
	int sizeleft = length;

	while (sizeleft > 0) {
		int actualwrite = 0;
		int writeamt = sizeleft > 512 ? 512 : sizeleft;

		ret = libusb_bulk_transfer(handle, OMAP_USB_BULK_OUT, data + iter,
					 writeamt, &actualwrite, USB_TIMEOUT);

		if (ret == LIBUSB_ERROR_TIMEOUT) {
			numtimeouts++;
			sizeleft -= actualwrite;
			iter += actualwrite;

			/* build in some reliablity */
			if (numtimeouts > USB_MAX_TIMEOUTS) {
				log_error("device timed out while transfering out %d bytes (%d made it)\n",
						length, iter);
				return false;
			}
		} else if (ret == LIBUSB_SUCCESS) {
			/* we cant trust actualWrite on anything but a timeout or success */
			sizeleft -= actualwrite;
			iter += actualwrite;
		} else {
			log_error("fatal transfer error (BULK_OUT) for %d bytes (%d made it): %s\n",
					length, iter, libusb_error_name(ret));
			return false;
		}
	}

	return true;
}

static unsigned char *omap_usb_get_string(libusb_device_handle *handle, uint8_t idx)
{
	unsigned char *data = NULL;
	int len = 0;
	int ret = 0;

	if (!handle)
		return NULL;

	while (true) {
		if (!len || ret < 0) {
			len += 256;
			data = realloc(data, len);

			if (!data)
				return NULL;
		}

		ret = libusb_get_string_descriptor_ascii(handle, idx, data, len);

		/* we can still recover... */
		if (ret < 0) {
			if (ret == LIBUSB_ERROR_INVALID_PARAM)
				continue;	/* try again with an increased size */

			log_error("failed to lookup string index %hhu: %s\n",
				  idx, libusb_error_name(ret));

			/* unrecoverable */
			free(data);
			return NULL;
		} else {
			/* we got something! */
			break;
		}
	}

	return data;
}

uint16_t omap_products[] = {
	0xd009,
	0xd00f,
};

static libusb_device_handle *omap_usb_open(libusb_context *ctx, uint16_t vendor, uint16_t product)
{
	libusb_device **devlist;
	libusb_device_handle *handle;
	struct libusb_device_descriptor desc;
	ssize_t count, i;
	int ret;

	log_info("scanning for USB device matching %04hx:%04hx...\n",
		 vendor, product);

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

			if (desc.idVendor != vendor)
				continue;

			if (product) {
				if (desc.idProduct != product)
					continue;
				goto found;
			}

			for (i = 0; i < sizeof(omap_products) / sizeof(uint16_t); i++)
				if (desc.idProduct == omap_products[i]) {
					product = desc.idProduct;
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
				vendor, product, libusb_error_name(ret));
		libusb_free_device_list(devlist, 1);
		return NULL;
	}

	ret = libusb_claim_interface(handle, 0);
	if (ret) {
		printf("Claim failed\n");
		return NULL;
	}

	/* grab the manufacturer and product strings for printing */
	unsigned char *mfgstr = omap_usb_get_string(handle, desc.iManufacturer);
	unsigned char *prodstr = omap_usb_get_string(handle, desc.iProduct);

	log_info("successfully opened %04hx:%04hx (", vendor, product);

	if (mfgstr) {
		fprintf(stdout, prodstr ? "%s " : "%s", mfgstr);
		free(mfgstr);
	}

	if (prodstr) {
		fprintf(stdout, "%s", prodstr);
		free(prodstr);
	}

	fprintf(stdout, ")\n");

	return handle;
}

static unsigned char *read_file(char *path, size_t *readamt)
{
	FILE *fp = fopen(path, "rb");

	if (!fp) {
		log_error("failed to open file \'%s\': %s\n", path,
			  strerror(errno));
		return NULL;
	}

	unsigned char *data = NULL;
	size_t allocsize = 0;
	size_t iter = 0;

	while (1) {
		allocsize += 1024;
		data = realloc(data, allocsize);
		if (!data)
			return NULL;

		size_t readsize = allocsize - iter;
		size_t ret = fread(data + iter, sizeof (unsigned char), readsize, fp);

		iter += ret;

		if (ret != readsize) {
			if (feof(fp)) {
				break;
			} else if (ferror(fp)) {
				log_error("error file reading file \'%s\': %s\n",
						path, strerror(errno));
				free(data);
				return NULL;
			}
		}
	}

	/* trim the allocation down to size */
	data = realloc(data, iter);
	*readamt = iter;

	return data;
}

static int transfer_first_stage(libusb_device_handle * handle, struct arg_state *args)
{
	unsigned char *buffer = NULL;
	uint32_t cmd = 0;
	uint32_t filelen = 0;
	int bufsize = 0x200;
	int transLen = 0;
	int i;
	void *data;
	uint32_t *dbuf;

	struct file_upload *file = args->files[0];

	/* TODO determine buffer size based on endpoint */
	buffer = calloc(bufsize, sizeof (unsigned char));
	filelen = cpu_to_le32(file->size);

	data = file->data;
	dbuf = data;

	if (le32toh(dbuf[5]) == 0x45534843) {
		int chsettingssize = 512 + 2 * sizeof(uint32_t);

		log_info("CHSETTINGS image detected. Skipping header\n");

		data += chsettingssize;
		filelen -= chsettingssize;
	}

	/* read the ASIC ID */
	if (!omap_usb_read(handle, buffer, bufsize, &transLen)) {
		log_error("failed to read ASIC ID from USB connection. "
			  "Check your USB device!\n");
		goto fail;
	}

	if (transLen != OMAP_ASIC_ID_LEN) {
		log_error("got some ASIC ID, but it's not the right length, %d "
			  "(expected %d)\n", transLen, OMAP_ASIC_ID_LEN);
		goto fail;
	}

	/* optionally, print some ASIC ID info */
	if (g_verbose) {
		char *fields[] =
		    { "Num Subblocks", "Device ID Info", "Reserved",
			"Ident Data", "Reserved", "CRC (4 bytes)"
		};
		int fieldLen[] = { 1, 7, 4, 23, 23, 11 };
		int field = 0;
		int nextSep = 0;

		log_info("got ASIC ID - ");

		for (i = 0; i < transLen; i++) {
			if (i == nextSep) {
				fprintf(stdout, "%s%s ",
					(field > 0) ? ", " : "", fields[field]);
				nextSep += fieldLen[field];
				field++;

				fprintf(stdout, "[");
			}

			fprintf(stdout, "%02x", buffer[i]);

			if (i + 1 == nextSep)
				fprintf(stdout, "]");
		}

		fprintf(stdout, "\n");
	}

	/* send the peripheral boot command */
	cmd = cpu_to_le32(OMAP_PERIPH_BOOT);

	if (!omap_usb_write(handle, (unsigned char *) &cmd, sizeof (cmd))) {
		log_error("failed to send the peripheral boot command 0x%08x\n",
			  OMAP_PERIPH_BOOT);
		goto fail;
	}

	/* send the length of the first file (little endian) */
	if (!omap_usb_write
	    (handle, (unsigned char *) &filelen, sizeof (filelen))) {
		log_error("failed to length specifier of %u to OMAP BootROM\n",
			  filelen);
		goto fail;
	}

	/* send the file! */
	if (!omap_usb_write(handle, data, filelen)) {
		log_error("failed to send file \'%s\' (size %u)\n",
			  file->basename, filelen);
		goto fail;
	}

	free(buffer);
	return 1;

      fail:
	free(buffer);
	return 0;
}

static int transfer_other_files(libusb_device_handle *handle, struct arg_state *args)
{
	uint32_t *buffer = NULL;
	int bufsize = 128 * sizeof (*buffer);
	int numfailures = 0;	/* build in some reliablity */
	int maxfailures = 3;
	int transLen = 0;
	int curfile = 1;	/* skip the first file */
	size_t len;

	buffer = calloc(bufsize, sizeof(unsigned char));

	/* handle the state machine for the X-Loader */
	while (curfile < args->numfiles) {
		uint32_t opcode = 0;
		uint8_t *extra = NULL;
		struct file_upload *f = args->files[curfile];

		/* read the opcode from xloader ID */
		if (!omap_usb_read
		    (handle, (unsigned char *) buffer, bufsize, &transLen)) {
			numfailures++;

			if (numfailures >= maxfailures) {
				log_error("failed to read command from X-Loader\n");
				goto fail;
			}

			/* sleep a bit */
			usleep(2000 * 1000);	/* 2s */
			continue;	/* try the opcode read again */
		}

		if (transLen < 8) {
			log_error("failed to recieve enough data for the opcode\n");
			goto fail;
		}

		/* extract the opcode and extra data pointer */
		opcode = le32_to_cpu(buffer[0]);
		extra = (uint8_t *)buffer;

		switch (opcode) {
		case USBLOAD_CMD_FILE_REQ:
			/* X-loader is requesting a file to be sent */
			/* send the opcode, size, and addr */
			buffer[0] = cpu_to_le32(USBLOAD_CMD_FILE);
			buffer[1] = cpu_to_le32(f->size);
			buffer[2] = cpu_to_le32(f->addr);

			if (!omap_usb_write(handle, (unsigned char *)buffer, sizeof(*buffer) * 3)) {
				log_error("failed to send load file command to the X-loader\n");
				goto fail;
			}

			if (g_verbose) {
				log_info("uploading \'%s\' (size %zu) to 0x%08x\n",
						f->basename, f->size, f->addr);
			}

			break;
		case USBLOAD_CMD_ECHO_SZ:
			/* X-loader confirms the size to recieve */
			if (buffer[1] != f->size) {
				log_error
				    ("X-loader failed to recieve the right file size for "
				     "file \'%s\' (got %u, expected %zu)\n",
				     f->basename, buffer[1], f->size);
				goto fail;
			}

			/* upload the raw file data */
			if (!omap_usb_write(handle, f->data, f->size)) {
				log_error
				    ("failed to send file \'%s\' to the X-loader\n",
				     f->basename);
				goto fail;
			}

			break;
		case USBLOAD_CMD_REPORT_SZ:
			/* X-loader confirms the amount of data it recieved */
			if (buffer[1] != f->size) {
				log_error
				    ("X-loader failed to recieve the right amount of data for "
				     "file \'%s\' (got %u, expected %zu)\n",
				     f->basename, buffer[1], f->size);
				goto fail;
			}

			curfile++;	/* move on to the next file */
			break;
		case USBLOAD_CMD_MESSAGE:
			/* X-loader debug message */
			len = strlen((char *)extra);

			if (len > (bufsize - sizeof (opcode) - 1))
				log_error("X-loader debug message not NUL terminated (size %zu)\n",
				     len);
			else
				fprintf(stdout, "X-loader Debug: %s\n", extra);
			break;
		default:
			log_error("unknown X-Loader opcode 0x%08X (%c%c%c%c)\n",
				  opcode, extra[0], extra[1], extra[2],
				  extra[3]);
			goto fail;
		}
	}

	/* we're done uploading files to X-loader send the jump command */
	buffer[0] = cpu_to_le32(USBLOAD_CMD_JUMP);
	buffer[1] = cpu_to_le32(args->jumptarget);

	if (!omap_usb_write
	    (handle, (unsigned char *) buffer, sizeof (*buffer) * 2)) {
		log_error
		    ("failed to send the final jump command to the X-loader. "
		     "Target was 0x%08x\n", args->jumptarget);
		goto fail;
	}

	if (g_verbose)
		log_info("jumping to address 0x%08x\n", args->jumptarget);

	free(buffer);
	return 1;

      fail:
	free(buffer);
	return 0;
}

static int process_args(struct arg_state *args)
{
	int i;

	/* For each file, load it in to memory
	 * TODO: defer this until transfer time (save memory and pipeline IO)
	 */

	for (i = 0; i < args->numfiles; i++) {
		struct file_upload *f = args->files[i];

		f->data = read_file(f->path, &f->size);

		if (!f->data) {
			return 1;
		}
	}

	if (g_verbose > 0) {
		for (i = 0; i < args->numfiles; i++) {
			struct file_upload *f = args->files[i];

			printf("File \'%s\' at 0x%08x, size %zu\n",
			       f->basename, f->addr, f->size);
		}
	}

	libusb_context *ctx;
	libusb_device_handle *dev;
	int ret;

	if ((ret = libusb_init(&ctx)) < 0) {
		log_error("failed to initialize libusb context: %s\n",
			  libusb_error_name(ret));
		return ret;
	}

	dev = omap_usb_open(ctx, args->vendor, args->product);

	if (!dev) {
		libusb_exit(ctx);
		return 1;
	}

	/* Communicate with the TI BootROM directly
	 * - retrieve ASIC ID
	 * - start peripheral boot
	 * - upload first file
	 * - execute first file
	 */
	if (!transfer_first_stage(dev, args)) {
		log_error("failed to transfer the first stage file \'%s\'\n",
			  args->files[0]->basename);
		goto fail;
	}

	/* Note: this is a race between the target's processor getting X-loader
	 * running and our processor. If we fail to communicate with the X-loader,
	 * it's possible that it hasn't been fully initialized. I'm not going to put
	 * some stupid, arbitrary sleep value here. The transfer_other_files function
	 * should be robust enough to handle some errors.
	 */

	/* If we are passed one file, assume that the user just wants to
	 * upload some initial code with no X-loader chaining
	 */
	if (args->numfiles > 1) {
		if (!transfer_other_files(dev, args)) {
			log_error
			    ("failed to transfer the additional files in to memory\n");
			goto fail;
		}
	}

	log_info("successfully transfered %d %s\n", args->numfiles,
		 (args->numfiles > 1) ? "files" : "file");

	/* safely close our USB handle and context */
	libusb_close(dev);
	libusb_exit(ctx);
	return 0;

fail:
	libusb_close(dev);
	libusb_exit(ctx);

	return 1;
}

/* getopt configuration */
static int do_version = 0;
static const char *const short_opt = "f:a:j:i:p:vh";
static const struct option long_opt[] = {
	{"file", 1, NULL, 'f'},
	{"addr", 1, NULL, 'a'},
	{"jump", 1, NULL, 'j'},
	{"vendor", 1, NULL, 'i'},
	{"product", 1, NULL, 'p'},
	{"verbose", 0, NULL, 'v'},
	{"help", 0, NULL, 'h'},
	{"version", 0, &do_version, 1},
	{NULL, 0, NULL, 0}
};

static void usage(char *exe)
{
	printf(
"Usage: %s [options] -f first-stage [-f file -a addr]...\n"
"Options:\n"
"  -f, --file      Provide the filename of a binary file to be\n"
"                  uploaded. The first file specified is uploaded to\n"
"                  the fixed address 0x%08x as defined by the manual.\n"
"                  Additional files must be followed by an address\n"
"                  argument (-a).\n"
"  -a, --addr      The address to load the prior file at.\n"
"  -j, --jump      Specify the address to jump to after loading all\n"
"                  of the files in to memory.\n"
"  -i, --vendor    Override the default vendor ID to poll for\n"
"                  (default 0x%04x).\n"
"  -p, --product   Poll for specific product id. Default: All known OMAP chips\n"
"  -h, --help      Display this message.\n"
"  -v, --verbose   Enable verbose output.\n"
"\n"
"Description:\n"
"  %s's basic usage is to upload an arbitrary file in to the memory\n"
"  of a TI OMAP3 compatible processor. This program directly\n"
"  communicates with the TI BootROM in order to upload a first stage\n"
"  payload, in most cases, TI's X-Loader. Using a compatible X-Loader\n"
"  will enable the upload of any file to any part in device memory.\n"
"\n"
"Examples:\n"
"  Uploading a compatible X-Loader, U-Boot, Kernel, and RAMDisk, then jumping\n"
"  to the U-Boot image for further bootloading:\n"
"    %s -f x-load.bin -f u-boot.bin -a 0x80200000 -f uImage -a 0x80800000 \\\n"
"       -f uRamdisk -a 0x81000000 -j 0x80200000\n"
"  Uploading arbitrary code to be executed (doesn't have to be X-loader):\n"
"    %s -f exec_me.bin\n"
"  Trying to debug an upload issue using verbose output:\n"
"    %s -v -f exec_me.bin -f file1.bin -a 0xdeadbeef -j 0xabad1dea\n"
"\n"
"Authors:\n"
"  Grant Hernandez <grant.h.hernandez@gmail.com> - rewrite of omap3_usbload to\n"
"    use the newer libusb 1.0\n"
"  Martin Mueller <martinmm@pfump.org> - initial code (omap3_usbload)\n"
"    and X-Loader patch\n",
	exe, OMAP_BASE_ADDRESS, OMAP_VENDOR_ID, PROG_NAME, exe, exe, exe
	);
}

static void license(void)
{
	printf(
"Copyright (C) 2008 Martin Mueller <martinmm@pfump.org>\n"
"Copyright (C) 2014 Grant Hernandez <grant.h.hernandez@gmail.com>\n"
"License GPLv2: GNU GPL version 2 or later <http://gnu.org/licenses/gpl.html>.\n"
"This is free software: you are free to change and redistribute it.\n"
"There is NO WARRANTY, to the extent permitted by law.\n"
	);
}

int main(int argc, char *argv[])
{
	int opt;
	bool gotfile = false;
	bool gotjump = false;
	int filecount = 0;
	char *exe = NULL;

	/* temporary local file object */
	struct file_upload file;
	/* total arg state */
	struct arg_state *args = calloc(1, sizeof (*args));

	if (argc < 1) {
		log_error("invalid arguments (no argv[0])\n");
		return 1;
	}

	exe = argv[0];

	fprintf(stdout, "%s %s\n", PROG_NAME, VERSION);

	/* set the default vendor and product */
	args->vendor = OMAP_VENDOR_ID;
	args->product = 0;

	while ((opt = getopt_long(argc, argv, short_opt, long_opt, NULL)) != -1) {
		switch (opt) {
		case 0:
			if (do_version) {
				license();
				return 0;
			}
			break;
		case 'f':
			if (gotfile) {
				log_error("missing address argument (-a) for file \'%s\'\n",
						file.path);
				usage(exe);
				return 1;
			}

			file.path = strdup(optarg);

			/* necessary to be sure that we own all the memory
			   and that the path input can be modified */
			char *tmpPath = strdup(file.path);
			file.basename = strdup(basename(tmpPath));
			free(tmpPath);

			filecount++;

			/* the first file gets uploaded to a fixed address
			   as specified by the technical reference manual */
			if (filecount == 1) {
				file.addr = OMAP_BASE_ADDRESS;

				/* commit the file object with the processor specified base address */
				args->files = realloc(args->files, filecount);
				args->numfiles = filecount;
				args->files[filecount - 1] = malloc(sizeof (file));
				memcpy(args->files[filecount - 1], &file, sizeof (file));
			} else {
				/* commit only after an address is specified */
				gotfile = true;
			}
			break;
		case 'a':
			if (!gotfile) {
				log_error
				    ("missing file argument (-f) before address \'%s\'\n",
				     optarg);
				usage(exe);
				return 1;
			}

			/* passing 0 to strtoul enables detection of the 0x prefix with
			   base-10 fallback if missing */
			file.addr = strtoul(optarg, NULL, 0);

			/* commit the file object */
			args->files = realloc(args->files, filecount);
			args->numfiles = filecount;
			args->files[filecount - 1] = malloc(sizeof(file));
			memcpy(args->files[filecount - 1], &file, sizeof(file));

			gotfile = false;
			break;
		case 'j':
			args->jumptarget = strtoul(optarg, NULL, 0);
			gotjump = true;
			break;
		case 'i':
			args->vendor = (uint16_t)strtoul(optarg, NULL, 0);
			break;
		case 'p':
			args->product = (uint16_t)strtoul(optarg, NULL, 0);
			break;
		case 'v':
			g_verbose++;
			break;
		case 'h':
			usage(exe);
			return 0;
		default:
			usage(exe);
			return 1;
		}
	}

	if (gotfile) {
		log_error("got file \'%s\', but no address!\n", file.path);
		usage(exe);
		return 1;
	}

	if (args->numfiles <= 0) {
		log_error("at least one file needs to be specified\n");
		usage(exe);
		return 1;
	}

	if (args->numfiles == 1 && gotjump) {
		log_info
		    ("WARNING: jump target 0x%08x specified, but will never be taken "
		     "(more than one file required)\n", args->jumptarget);
	} else if (args->numfiles > 1 && !gotjump) {
		log_info
		    ("WARNING: no jump target specified. Defaulting to the first "
		     "file's (\'%s\') address 0x%08x\n",
		     args->files[1]->basename, args->files[1]->addr);
		args->jumptarget = args->files[1]->addr;
	}

	return process_args(args);
}
