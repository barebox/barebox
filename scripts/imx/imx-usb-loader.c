/*
 * imx_usb:
 *
 * Program to download and execute an image over the USB boot protocol
 * on i.MX series processors.
 *
 * Code originally written by Eric Nelson.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */
#include <stdio.h>
#include <sys/types.h>
#include <time.h>

#include <unistd.h>
#include <ctype.h>
#include <sys/io.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <libusb.h>
#include <getopt.h>
#include <arpa/inet.h>
#include <linux/kernel.h>

#include "imx.h"

#define get_min(a, b) (((a) < (b)) ? (a) : (b))

#define FT_APP	0xaa
#define FT_CSF	0xcc
#define FT_DCD	0xee
#define FT_LOAD_ONLY	0x00

int verbose;
static int skip_image_dcd;
static struct libusb_device_handle *usb_dev_handle;
static struct usb_id *usb_id;

struct mach_id {
	struct mach_id * next;
	unsigned short vid;
	unsigned short pid;
	char file_name[256];
	char *name;
#define MODE_HID	0
#define MODE_BULK	1
	unsigned char mode;
#define HDR_NONE	0
#define HDR_MX51	1
#define HDR_MX53	2
	unsigned char header_type;
	unsigned short max_transfer;
};

struct usb_work {
	char filename[256];
	unsigned char dcd;
	unsigned char plug;
};

struct usb_id {
	struct mach_id *mach_id;
	struct usb_work *work;
};

struct mach_id imx_ids[] = {
	{
		.vid = 0x066f,
		.pid = 0x3780,
		.name = "i.MX23",
		.mode = MODE_BULK,
	}, {
		.vid = 0x15a2,
		.pid = 0x004f,
		.name = "i.MX28",
	}, {
		.vid = 0x15a2,
		.pid = 0x0052,
		.name = "i.MX50",
	}, {
		.vid = 0x15a2,
		.pid = 0x0054,
		.name = "i.MX6q",
		.header_type = HDR_MX53,
		.mode = MODE_HID,
		.max_transfer = 1024,
	}, {
		.vid = 0x15a2,
		.pid = 0x0061,
		.name = "i.MX6dl/s",
		.header_type = HDR_MX53,
		.mode = MODE_HID,
		.max_transfer = 1024,
	}, {
		.vid = 0x15a2,
		.pid = 0x0071,
		.name = "i.MX6 SoloX",
		.header_type = HDR_MX53,
		.mode = MODE_HID,
		.max_transfer = 1024,
	}, {
		.vid = 0x15a2,
		.pid = 0x007d,
		.name = "i.MX6ul",
		.header_type = HDR_MX53,
		.mode = MODE_HID,
		.max_transfer = 1024,
	}, {
		.vid = 0x15a2,
		.pid = 0x0041,
		.name = "i.MX51",
		.header_type = HDR_MX51,
		.mode = MODE_BULK,
		.max_transfer = 64,
	}, {
		.vid = 0x15a2,
		.pid = 0x004e,
		.name = "i.MX53",
		.header_type = HDR_MX53,
		.mode = MODE_BULK,
		.max_transfer = 512,
	}, {
		.vid = 0x15a2,
		.pid = 0x0030,
		.name = "i.MX35",
		.header_type = HDR_MX51,
		.mode = MODE_BULK,
		.max_transfer = 64,
	}, {
		.vid = 0x15a2,
		.pid = 0x003a,
		.name = "i.MX25",
		.header_type = HDR_MX51,
		.mode = MODE_BULK,
		.max_transfer = 64,
	},
};

#define SDP_READ_REG		0x0101
#define SDP_WRITE_REG		0x0202
#define SDP_WRITE_FILE		0x0404
#define SDP_ERROR_STATUS	0x0505
#define SDP_JUMP_ADDRESS	0x0b0b

struct sdp_command  {
	uint16_t cmd;
	uint32_t addr;
	uint8_t format;
	uint32_t cnt;
	uint32_t data;
	uint8_t rsvd;
} __attribute__((packed));

static struct mach_id *imx_device(unsigned short vid, unsigned short pid)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(imx_ids); i++) {
		struct mach_id *id = &imx_ids[i];
		if (id->vid == vid && id->pid == pid) {
			fprintf(stderr, "found %s USB device [%04x:%04x]\n",
					id->name, vid, pid);
			return id;
		}
	}

	return NULL;
}

static libusb_device *find_imx_dev(libusb_device **devs, struct mach_id **pp_id)
{
	int i = 0;
	struct mach_id *p;

	for (;;) {
		struct libusb_device_descriptor desc;
		int r;

		libusb_device *dev = devs[i++];
		if (!dev)
			break;

		r = libusb_get_device_descriptor(dev, &desc);
		if (r < 0) {
			fprintf(stderr, "failed to get device descriptor");
			return NULL;
		}

		p = imx_device(desc.idVendor, desc.idProduct);
		if (p) {
			*pp_id = p;
			return dev;
		}
	}
	*pp_id = NULL;

	return NULL;
}

static void dump_long(const void *src, unsigned cnt, unsigned addr)
{
	const unsigned *p = (unsigned *)src;

	while (cnt >= 32) {
		printf("%08x: %08x %08x %08x %08x  %08x %08x %08x %08x\n",
				addr, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]);
		p += 8;
		cnt -= 32;
		addr += 32;
	}
	if (cnt) {
		printf("%08x:", addr);
		while (cnt >= 4) {
			printf(" %08x", p[0]);
			p++;
			cnt -= 4;
		}
		printf("\n");
	}
}

static void dump_bytes(const void *src, unsigned cnt, unsigned addr)
{
	const unsigned char *p = src;
	int i;

	while (cnt >= 16) {
		printf("%08x: %02x %02x %02x %02x  %02x %02x %02x %02x  %02x %02x %02x %02x  %02x %02x %02x %02x\n",
				addr,
				p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9], p[10],
				p[11], p[12], p[13], p[14], p[15]);
		p += 16;
		cnt -= 16;
		addr += 16;
	}

	if (cnt) {
		printf("%08x:", addr);
		i = 0;
		while (cnt) {
			printf(" %02x", p[0]);
			p++;
			cnt--;
			i++;
			if (cnt) if (i == 4) {
				i = 0;
				printf(" ");
			}
		}
		printf("\n");
	}
}

static long get_file_size(FILE *xfile)
{
	long size;
	fseek(xfile, 0, SEEK_END);
	size = ftell(xfile);
	rewind(xfile);

	return size;
}

static int read_file(const char *name, unsigned char **buffer, unsigned *size)
{
	FILE *xfile;
	unsigned fsize;
	int cnt;
	unsigned char *buf;
	xfile = fopen(name, "rb");
	if (!xfile) {
		printf("error, can not open input file: %s\n", name);
		return -5;
	}

	fsize = get_file_size(xfile);
	if (fsize < 0x20) {
		printf("error, file: %s is too small\n", name);
		fclose(xfile);
		return -2;
	}

	buf = malloc(fsize);
	if (!buf) {
		printf("error, out of memory\n");
		fclose(xfile);
		return -2;
	}

	cnt = fread(buf, 1 , fsize, xfile);
	if (cnt < fsize) {
		printf("error, cannot read %s\n", name);
		fclose(xfile);
		free(buf);
		return -1;
	}

	if (size)
		*size = fsize;

	if (buffer)
		*buffer = buf;
	else
		free(buf);

	return 0;
}

/*
 * HID Class-Specific Requests values. See section 7.2 of the HID specifications
 */
#define HID_GET_REPORT			0x01
#define HID_GET_IDLE			0x02
#define HID_GET_PROTOCOL		0x03
#define HID_SET_REPORT			0x09
#define HID_SET_IDLE			0x0A
#define HID_SET_PROTOCOL		0x0B
#define HID_REPORT_TYPE_INPUT		0x01
#define HID_REPORT_TYPE_OUTPUT		0x02
#define HID_REPORT_TYPE_FEATURE		0x03
#define CTRL_IN			LIBUSB_ENDPOINT_IN |LIBUSB_REQUEST_TYPE_CLASS|LIBUSB_RECIPIENT_INTERFACE
#define CTRL_OUT		LIBUSB_ENDPOINT_OUT|LIBUSB_REQUEST_TYPE_CLASS|LIBUSB_RECIPIENT_INTERFACE

#define EP_IN	0x80

/*
 * For HID class drivers, 4 reports are used to implement
 * Serial Download protocol(SDP)
 * Report 1 (control out endpoint) 16 byte SDP comand
 *  (total of 17 bytes with 1st byte report id of 0x01
 * Report 2 (control out endpoint) data associated with report 1 commands
 *  (max size of 1025 with 1st byte of 0x02)
 * Report 3 (interrupt in endpoint) HAB security state
 *  (max size of 5 bytes with 1st byte of 0x03)
 *  (0x12343412 production)
 *  (0x56787856 engineering)
 * Report 4 (interrupt in endpoint) date associated with report 1 commands
 *  (max size of 65 bytes with 1st byte of 0x04)
 *
 */
/*
 * For Bulk class drivers, the device is configured as
 * EP0IN, EP0OUT control transfer
 * EP1OUT - bulk out
 * (max packet size of 512 bytes)
 * EP2IN - bulk in
 * (max packet size of 512 bytes)
 */
static int transfer(int report, unsigned char *p, unsigned cnt, int *last_trans)
{
	int err;
	if (cnt > usb_id->mach_id->max_transfer)
		cnt = usb_id->mach_id->max_transfer;

	if (verbose > 4) {
		printf("report=%i\n", report);
		if (report < 3)
			dump_bytes(p, cnt, 0);
	}

	if (usb_id->mach_id->mode == MODE_BULK) {
		*last_trans = 0;
		err = libusb_bulk_transfer(usb_dev_handle,
					   (report < 3) ? 1 : 2 + EP_IN, p, cnt, last_trans, 1000);
	} else {
		unsigned char tmp[1028];

		tmp[0] = (unsigned char)report;

		if (report < 3) {
			memcpy(&tmp[1], p, cnt);
			err = libusb_control_transfer(usb_dev_handle,
					CTRL_OUT,
					HID_SET_REPORT,
					(HID_REPORT_TYPE_OUTPUT << 8) | report,
					0,
					tmp, cnt + 1, 1000);
			*last_trans = (err > 0) ? err - 1 : 0;
			if (err > 0)
				err = 0;
		} else {
			*last_trans = 0;
			memset(&tmp[1], 0, cnt);
			err = libusb_interrupt_transfer(usb_dev_handle,
							1 + EP_IN, tmp, cnt + 1, last_trans, 1000);
			if (err >= 0) {
				if (tmp[0] == (unsigned char)report) {
					if (*last_trans > 1) {
						*last_trans -= 1;
						memcpy(p, &tmp[1], *last_trans);
					}
				} else {
					printf("Unexpected report %i err=%i, cnt=%u, last_trans=%i, %02x %02x %02x %02x\n",
						tmp[0], err, cnt, *last_trans, tmp[0], tmp[1], tmp[2], tmp[3]);
					err = 0;
				}
			}
		}
	}

	if (verbose > 4 && report >= 3)
		dump_bytes(p, cnt, 0);

	return err;
}

int do_status(void)
{
	int last_trans;
	unsigned char tmp[64];
	int retry = 0;
	int err;
	static const struct sdp_command status_command = {
		.cmd = SDP_ERROR_STATUS,
		.addr = 0,
		.format = 0,
		.cnt = 0,
		.data = 0,
		.rsvd = 0,
	};

	for (;;) {
		err = transfer(1, (unsigned char *) &status_command, 16,
			       &last_trans);

		if (verbose > 2)
			printf("report 1, wrote %i bytes, err=%i\n", last_trans, err);

		memset(tmp, 0, sizeof(tmp));

		err = transfer(3, tmp, 64, &last_trans);

		if (verbose > 2) {
			printf("report 3, read %i bytes, err=%i\n", last_trans, err);
			printf("read=%02x %02x %02x %02x\n", tmp[0], tmp[1], tmp[2], tmp[3]);
		}

		if (!err)
			break;

		retry++;

		if (retry > 5)
			break;
	}

	if (usb_id->mach_id->mode == MODE_HID) {
		err = transfer(4, tmp, sizeof(tmp), &last_trans);
		if (err)
			printf("4 in err=%i, last_trans=%i  %02x %02x %02x %02x\n",
					err, last_trans, tmp[0], tmp[1], tmp[2], tmp[3]);
	}

	return err;
}

static int read_memory(unsigned addr, void *dest, unsigned cnt)
{
	static struct sdp_command read_reg_command = {
		.cmd = SDP_READ_REG,
		.addr = 0,
		.format = 0x20,
		.cnt = 0,
		.data = 0,
		.rsvd = 0,
	};

	int retry = 0;
	int last_trans;
	int err;
	int rem;
	unsigned char tmp[64];
	read_reg_command.addr = htonl(addr);
	read_reg_command.cnt = htonl(cnt);

	for (;;) {
		err = transfer(1, (unsigned char *) &read_reg_command, 16,
			       &last_trans);
		if (!err)
			break;
		printf("read_reg_command err=%i, last_trans=%i\n", err, last_trans);
		if (retry > 5) {
			return -4;
		}
		retry++;
	}

	err = transfer(3, tmp, 4, &last_trans);
	if (err) {
		printf("r3 in err=%i, last_trans=%i  %02x %02x %02x %02x\n",
				err, last_trans, tmp[0], tmp[1], tmp[2], tmp[3]);
		return err;
	}

	rem = cnt;

	while (rem) {
		tmp[0] = tmp[1] = tmp[2] = tmp[3] = 0;
		err = transfer(4, tmp, 64, &last_trans);
		if (err) {
			printf("r4 in err=%i, last_trans=%i  %02x %02x %02x %02x cnt=%u rem=%d\n",
					err, last_trans, tmp[0], tmp[1], tmp[2], tmp[3], cnt, rem);
			break;
		}
		if ((last_trans > rem) || (last_trans > 64)) {
			if ((last_trans == 64) && (cnt == rem)) {
				/* Last transfer is expected to be too large for HID */
			} else {
				printf("err: %02x %02x %02x %02x cnt=%u rem=%d last_trans=%i\n",
						tmp[0], tmp[1], tmp[2], tmp[3], cnt, rem, last_trans);
			}
			last_trans = rem;
			if (last_trans > 64)
				last_trans = 64;
		}
		memcpy(dest, tmp, last_trans);
		dest += last_trans;
		rem -= last_trans;
	}
	return err;
}

static int write_memory(unsigned addr, unsigned val, int width)
{
	int retry = 0;
	int last_trans;
	int err = 0;
	unsigned char tmp[64];
	static struct sdp_command write_reg_command = {
		.cmd = SDP_WRITE_REG,
		.addr = 0,
		.format = 0,
		.cnt = 0,
		.data = 0,
		.rsvd = 0,
	};

	write_reg_command.addr = htonl(addr);
	write_reg_command.cnt = htonl(4);

	if (verbose > 1)
		printf("write memory reg: 0x%08x val: 0x%08x width: %d\n", addr, val, width);

	switch (width) {
		case 1:
			write_reg_command.format = 0x8;
			break;
		case 2:
			write_reg_command.format = 0x10;
			break;
		case 4:
			write_reg_command.format = 0x20;
			break;
		default:
			return -1;
	}

	write_reg_command.data = htonl(val);

	for (;;) {
		err = transfer(1, (unsigned char *) &write_reg_command, 16,
			       &last_trans);
		if (!err)
			break;
		printf("write_reg_command err=%i, last_trans=%i\n", err, last_trans);
		if (retry > 5) {
			return -4;
		}
		retry++;
	}

	memset(tmp, 0, sizeof(tmp));

	err = transfer(3, tmp, sizeof(tmp), &last_trans);
	if (err) {
		printf("w3 in err=%i, last_trans=%i  %02x %02x %02x %02x\n",
				err, last_trans, tmp[0], tmp[1], tmp[2], tmp[3]);
		printf("addr=0x%08x, val=0x%08x\n", addr, val);
	}

	memset(tmp, 0, sizeof(tmp));

	err = transfer(4, tmp, sizeof(tmp), &last_trans);
	if (err)
		printf("w4 in err=%i, last_trans=%i  %02x %02x %02x %02x\n",
				err, last_trans, tmp[0], tmp[1], tmp[2], tmp[3]);
	return err;
}

static int modify_memory(unsigned addr, unsigned val, int width, int set_bits, int clear_bits)
{
	int err;

	if (set_bits || clear_bits) {
		uint32_t r;

		err = read_memory(addr, &r, 4);
		if (err < 0)
			return err;

		if (verbose > 1)
			printf("reg 0x%08x val: 0x%08x %s0x%08x\n", addr, r,
			       set_bits ? "|= " : "&= ~", val);

		if (set_bits)
			r |= val;
		if (clear_bits)
			r &= ~val;
		val = r;
	}

	return write_memory(addr, val, 4);
}

static int load_file(void *buf, unsigned len, unsigned dladdr, unsigned char type)
{
	static struct sdp_command dl_command = {
		.cmd = SDP_WRITE_FILE,
		.addr = 0,
		.format = 0,
		.cnt = 0,
		.data = 0,
		.rsvd = 0,
	};

	int last_trans, err;
	int retry = 0;
	unsigned transfer_size = 0;
	unsigned char tmp[64];
	void *p;
	int cnt;

	dl_command.addr = htonl(dladdr);
	dl_command.cnt = htonl(len);
	dl_command.rsvd = type;

	for (;;) {
		err = transfer(1, (unsigned char *) &dl_command, 16,
			       &last_trans);
		if (!err)
			break;

		printf("dl_command err=%i, last_trans=%i\n", err, last_trans);

		if (retry > 5)
			return -4;
		retry++;
	}

	retry = 0;

	if (usb_id->mach_id->mode == MODE_BULK) {
		err = transfer(3, tmp, sizeof(tmp), &last_trans);
		if (err)
			printf("in err=%i, last_trans=%i  %02x %02x %02x %02x\n",
					err, last_trans, tmp[0], tmp[1], tmp[2], tmp[3]);
	}

	p = buf;
	cnt = len;

	while (1) {
		int now = get_min(cnt, usb_id->mach_id->max_transfer);

		if (!now)
			break;

		err = transfer(2, p, now, &now);
		if (err) {
			printf("dl_command err=%i, last_trans=%i\n", err, last_trans);
			return err;
		}

		p += now;
		cnt -= now;
	}

	if (usb_id->mach_id->mode == MODE_HID) {
		err = transfer(3, tmp, sizeof(tmp), &last_trans);
		if (err)
			printf("3 in err=%i, last_trans=%i  %02x %02x %02x %02x\n",
					err, last_trans, tmp[0], tmp[1], tmp[2], tmp[3]);
		err = transfer(4, tmp, sizeof(tmp), &last_trans);
		if (err)
			printf("4 in err=%i, last_trans=%i  %02x %02x %02x %02x\n",
					err, last_trans, tmp[0], tmp[1], tmp[2], tmp[3]);
	} else {
		do_status();
	}

	return transfer_size;
}

static int sdp_jump_address(unsigned addr)
{
	unsigned char tmp[64];
	static struct sdp_command jump_command = {
		.cmd = SDP_JUMP_ADDRESS,
		.addr = 0,
		.format = 0,
		.cnt = 0,
		.data = 0,
		.rsvd = 0,
	};
	int last_trans, err;
	int retry = 0;

	jump_command.addr = htonl(addr);

	for (;;) {
		err = transfer(1, (unsigned char *) &jump_command, 16,
			       &last_trans);
		if (!err)
			break;

		printf("jump_command err=%i, last_trans=%i\n", err, last_trans);

		if (retry > 5)
			return -4;

		retry++;
	}

	memset(tmp, 0, sizeof(tmp));
	err = transfer(3, tmp, sizeof(tmp), &last_trans);

	if (err)
		printf("j3 in err=%i, last_trans=%i  %02x %02x %02x %02x\n",
			err, last_trans, tmp[0], tmp[1], tmp[2], tmp[3]);
	return 0;
}

static int write_dcd_table_ivt(const struct imx_flash_header_v2 *hdr,
			       const unsigned char *file_start, unsigned cnt)
{
	unsigned char *dcd_end;
	unsigned m_length;
#define cvt_dest_to_src		(((unsigned char *)hdr) - hdr->self)
	unsigned char* dcd;
	const unsigned char *file_end = file_start + cnt;
	int err = 0;

	if (!hdr->dcd_ptr) {
		printf("No dcd table in this ivt\n");
		return 0; /* nothing to do */
	}

	dcd = hdr->dcd_ptr + cvt_dest_to_src;

	if ((dcd < file_start) || ((dcd + 4) > file_end)) {
		printf("bad dcd_ptr %08x\n", hdr->dcd_ptr);
		return -1;
	}

	m_length = (dcd[1] << 8) + dcd[2];

	printf("main dcd length %x\n", m_length);

	if ((dcd[0] != 0xd2) || (dcd[3] != 0x40)) {
		printf("Unknown tag\n");
		return -1;
	}

	dcd_end = dcd + m_length;

	if (dcd_end > file_end) {
		printf("bad dcd length %08x\n", m_length);
		return -1;
	}
	dcd += 4;

	while (dcd < dcd_end) {
		unsigned s_length = (dcd[1] << 8) + dcd[2];
		unsigned char *s_end = dcd + s_length;
		int set_bits = 0, clear_bits = 0;

		printf("command: 0x%02x sub dcd length: 0x%04x, flags: 0x%02x\n", dcd[0], s_length, dcd[3]);

		if ((dcd[0] != 0xcc)) {
			printf("Skipping unknown sub tag 0x%02x with len %04x\n", dcd[0], s_length);
			usleep(50000);
			dcd += s_length;
			continue;
		}

		if (dcd[3] & PARAMETER_FLAG_MASK) {
			if (dcd[3] & PARAMETER_FLAG_SET)
				set_bits = 1;
			else
				clear_bits = 1;
		}

		dcd += 4;

		if (s_end > dcd_end) {
			printf("error s_end(%p) > dcd_end(%p)\n", s_end, dcd_end);
			return -1;
		}

		while (dcd < s_end) {
			unsigned addr = (dcd[0] << 24) | (dcd[1] << 16) | (dcd[2] << 8) | dcd[3];
			unsigned val = (dcd[4] << 24) | (dcd[5] << 16) | (dcd[6] << 8) | dcd[7];

			dcd += 8;

			modify_memory(addr, val, 4, set_bits, clear_bits);
		}
	}
	return err;
}

static int get_dcd_range_old(const struct imx_flash_header *hdr,
		const unsigned char *file_start, unsigned cnt,
		unsigned char **pstart, unsigned char **pend)
{
	unsigned char *dcd_end;
	unsigned m_length;
#define cvt_dest_to_src_old		(((unsigned char *)&hdr->dcd) - hdr->dcd_ptr_ptr)
	unsigned char* dcd;
	unsigned val;
	const unsigned char *file_end = file_start + cnt;

	if (!hdr->dcd) {
		printf("No dcd table, barker=%x\n", hdr->app_code_barker);
		*pstart = *pend = ((unsigned char *)hdr) + sizeof(struct imx_flash_header);
		return 0; /* nothing to do */
	}

	dcd = hdr->dcd + cvt_dest_to_src_old;

	if ((dcd < file_start) || ((dcd + 8) > file_end)) {
		printf("bad dcd_ptr %08x\n", hdr->dcd);
		return -1;
	}

	val = (dcd[0] << 0) | (dcd[1] << 8) | (dcd[2] << 16) | (dcd[3] << 24);

	if (val != DCD_BARKER) {
		printf("Unknown tag\n");
		return -1;
	}

	dcd += 4;
	m_length = (dcd[0] << 0) | (dcd[1] << 8) | (dcd[2] << 16) | (dcd[3] << 24);
	dcd += 4;
	dcd_end = dcd + m_length;

	if (dcd_end > file_end) {
		printf("bad dcd length %08x\n", m_length);
		return -1;
	}

	*pstart = dcd;
	*pend = dcd_end;

	return 0;
}

static int write_dcd_table_old(const struct imx_flash_header *hdr,
			       const unsigned char *file_start, unsigned cnt)
{
	unsigned val;
	unsigned char *dcd_end;
	unsigned char* dcd;
	int err = get_dcd_range_old(hdr, file_start, cnt, &dcd, &dcd_end);
	if (err < 0)
		return err;

	printf("writing DCD table...\n");

	while (dcd < dcd_end) {
		unsigned type = (dcd[0] << 0) | (dcd[1] << 8) | (dcd[2] << 16) | (dcd[3] << 24);
		unsigned addr = (dcd[4] << 0) | (dcd[5] << 8) | (dcd[6] << 16) | (dcd[7] << 24);
		val = (dcd[8] << 0) | (dcd[9] << 8) | (dcd[10] << 16) | (dcd[11] << 24);
		dcd += 12;

		switch (type) {
		case 1:
			if (verbose > 1)
				printf("type=%08x *0x%08x = 0x%08x\n", type, addr, val);
			err = write_memory(addr, val, 1);
			if (err < 0)
				return err;
			break;
		case 4:
			if (verbose > 1)
				printf("type=%08x *0x%08x = 0x%08x\n", type, addr, val);
			err = write_memory(addr, val, 4);
			if (err < 0)
				return err;
			break;
		default:
			printf("!!!unknown type=%08x *0x%08x = 0x%08x\n", type, addr, val);
		}
	}

	if (err)
		fprintf(stderr, "writing DCD table failed with %d\n", err);
	else
		printf("DCD table successfully written\n");

	return err;
}

static int verify_memory(const void *buf, unsigned len, unsigned addr)
{
	int ret, mismatch = 0;
	void *readbuf;
	unsigned offset = 0, now;

	readbuf = malloc(len);
	if (!readbuf)
		return -ENOMEM;

	ret = read_memory(addr, readbuf, len);
	if (ret < 0)
		goto err;

	while (len) {
		now = get_min(len, 32);

		if (memcmp(buf + offset, readbuf + offset, now)) {
			printf("mismatch at offset 0x%08x. expected:\n", offset);
			dump_long(buf + offset, now, addr + offset);
			printf("read:\n");
			dump_long(readbuf + offset, now, addr + offset);
			ret = -EINVAL;
			mismatch++;
			if (mismatch > 4)
				goto err;
		}

		len -= now;
		offset += now;
	}

err:
	free(readbuf);

	return ret;
}

static int is_header(const unsigned char *p)
{
	const struct imx_flash_header *ohdr =
		(const struct imx_flash_header *)p;
	const struct imx_flash_header_v2 *hdr =
		(const struct imx_flash_header_v2 *)p;

	switch (usb_id->mach_id->header_type) {
	case HDR_MX51:
		if (ohdr->app_code_barker == 0xb1)
			return 1;
		break;
	case HDR_MX53:
		if (hdr->header.tag == TAG_IVT_HEADER && hdr->header.version == IVT_VERSION)
			return 1;
	}

	return 0;
}

static int perform_dcd(unsigned char *p, const unsigned char *file_start,
		       unsigned cnt)
{
	struct imx_flash_header *ohdr = (struct imx_flash_header *)p;
	struct imx_flash_header_v2 *hdr = (struct imx_flash_header_v2 *)p;
	int ret = 0;

	if (skip_image_dcd)
		return 0;

	switch (usb_id->mach_id->header_type) {
	case HDR_MX51:
		ret = write_dcd_table_old(ohdr, file_start, cnt);
		ohdr->dcd_block_len = 0;

		break;
	case HDR_MX53:
		ret = write_dcd_table_ivt(hdr, file_start, cnt);
		hdr->dcd_ptr = 0;

		break;
	}

	return ret;
}

static int get_dl_start(const unsigned char *p, const unsigned char *file_start,
		unsigned cnt, unsigned *dladdr, unsigned *max_length, unsigned *plugin,
		unsigned *header_addr)
{
	const unsigned char *file_end = file_start + cnt;
	switch (usb_id->mach_id->header_type) {
	case HDR_MX51:
	{
		struct imx_flash_header *ohdr = (struct imx_flash_header *)p;
		unsigned char *dcd_end;
		unsigned char* dcd;
		int err = get_dcd_range_old(ohdr, file_start, cnt, &dcd, &dcd_end);

		*dladdr = ohdr->app_dest;
		*header_addr = ohdr->dcd_ptr_ptr - offsetof(struct imx_flash_header, dcd);
		*plugin = 0;
		if (err >= 0)
			*max_length = dcd_end[0] | (dcd_end[1] << 8) | (dcd_end[2] << 16) | (dcd_end[3] << 24);

		break;
	}
	case HDR_MX53:
	{
		unsigned char *bd;
		struct imx_flash_header_v2 *hdr = (struct imx_flash_header_v2 *)p;

		*dladdr = hdr->self;
		*header_addr = hdr->self;
		bd = hdr->boot_data_ptr + cvt_dest_to_src;
		if ((bd < file_start) || ((bd + 4) > file_end)) {
			printf("bad boot_data_ptr %08x\n", hdr->boot_data_ptr);
			return -1;
		}

		*dladdr = ((struct imx_boot_data *)bd)->start;
		*max_length = ((struct imx_boot_data *)bd)->size;
		*plugin = ((struct imx_boot_data *)bd)->plugin;
		((struct imx_boot_data *)bd)->plugin = 0;

		break;
	}
	}
	return 0;
}

static int process_header(struct usb_work *curr, unsigned char *buf, int cnt,
		unsigned *p_dladdr, unsigned *p_max_length, unsigned *p_plugin,
		unsigned *p_header_addr)
{
	int ret;
	unsigned header_max = 0x800;
	unsigned header_inc = 0x400;
	unsigned header_offset = 0;
	int header_cnt = 0;
	unsigned char *p = buf;

	for (header_offset = 0; header_offset < header_max; header_offset += header_inc, p += header_inc) {

		if (!is_header(p))
			continue;

		ret = get_dl_start(p, buf, cnt, p_dladdr, p_max_length, p_plugin, p_header_addr);
		if (ret < 0) {
			printf("!!get_dl_start returned %i\n", ret);
			return ret;
		}

		if (curr->dcd) {
			ret = perform_dcd(p, buf, cnt);
			if (ret < 0) {
				printf("!!perform_dcd returned %i\n", ret);
				return ret;
			}
			curr->dcd = 0;
		}

		if (*p_plugin && (!curr->plug) && (!header_cnt)) {
			header_cnt++;
			header_max = header_offset + *p_max_length + 0x400;
			if (header_max > cnt - 32)
				header_max = cnt - 32;
			printf("header_max=%x\n", header_max);
			header_inc = 4;
		} else {
			if (!*p_plugin)
				curr->plug = 0;
			return header_offset;
		}
	}

	fprintf(stderr, "no DCD header found in image, run imx-image first\n");

	return -ENODEV;
}

static int do_irom_download(struct usb_work *curr, int verify)
{
	int ret;
	unsigned char type;
	unsigned fsize = 0;
	unsigned header_offset;
	unsigned file_base;
	unsigned char *buf = NULL;
	unsigned char *image;
	unsigned char *verify_buffer = NULL;
	unsigned dladdr = 0;
	unsigned max_length;
	unsigned plugin = 0;
	unsigned header_addr = 0;
	unsigned skip = 0;

	ret = read_file(curr->filename, &buf, &fsize);
	if (ret < 0)
		return ret;

	max_length = fsize;

	ret = process_header(curr, buf, fsize,
			&dladdr, &max_length, &plugin, &header_addr);
	if (ret < 0)
		goto cleanup;

	header_offset = ret;

	if (plugin && (!curr->plug)) {
		printf("Only plugin header found\n");
		ret = -1;
		goto cleanup;
	}

	if (!dladdr) {
		printf("unknown load address\n");
		ret = -3;
		goto cleanup;
	}

	file_base = header_addr - header_offset;

	if (usb_id->mach_id->mode == MODE_BULK) {
		/* No jump command, dladdr should point to header */
		dladdr = header_addr;
	}

	if (file_base > dladdr) {
		max_length -= (file_base - dladdr);
		dladdr = file_base;
	}

	skip = dladdr - file_base;

	image = buf + skip;
	fsize -= skip;

	if (fsize > max_length)
		fsize = max_length;

	type = FT_APP;

	if (verify) {
		verify_buffer = malloc(64);

		if (!verify_buffer) {
			printf("error, out of memory\n");
			ret = -2;
			goto cleanup;
		}

		memcpy(verify_buffer, image, 64);

		if ((type == FT_APP) && (usb_id->mach_id->mode != MODE_HID)) {
			type = FT_LOAD_ONLY;
			verify = 2;
		}
	}

	printf("loading binary file(%s) to %08x, skip=0x%x, fsize=%u type=%d...\n",
			curr->filename, dladdr, skip, fsize, type);

	ret = load_file(image, fsize, dladdr, type);
	if (ret < 0)
		goto cleanup;

	printf("binary file successfully loaded\n");

	if (verify) {
		printf("verifying file...\n");

		ret = verify_memory(image, fsize, dladdr);
		if (ret < 0) {
			printf("verifying failed\n");
			goto cleanup;
		}

		printf("file successfully verified\n");

		if (verify == 2) {
			/*
			 * In bulk mode we do not have an explicit jump command,
			 * so we load part of the image again with type FT_APP
			 * this time.
			 */
			ret = load_file(verify_buffer, 64, dladdr, FT_APP);
			if (ret < 0)
				goto cleanup;

		}
	}

	if (usb_id->mach_id->mode == MODE_HID && type == FT_APP) {
		printf("jumping to 0x%08x\n", header_addr);

		ret = sdp_jump_address(header_addr);
		if (ret < 0)
			return ret;
	}

	ret = 0;
cleanup:
	free(verify_buffer);
	free(buf);

	return ret;
}

static int write_mem(const struct config_data *data, uint32_t addr,
		     uint32_t val, int width, int set_bits, int clear_bits)
{
	return modify_memory(addr, val, width, set_bits, clear_bits);
}

static int parse_initfile(const char *filename)
{
	struct config_data data = {
		.write_mem = write_mem,
	};

	return parse_config(&data, filename);
}

static void usage(const char *prgname)
{
	fprintf(stderr, "usage: %s [OPTIONS] [FILENAME]\n\n"
		"-c           check correctness of flashed image\n"
		"-i <cfgfile> Specify custom SoC initialization file\n"
		"-s           skip DCD included in image\n"
		"-v           verbose (give multiple times to increase)\n"
		"-h           this help\n", prgname);
	exit(1);
}

int main(int argc, char *argv[])
{
	struct mach_id *mach;
	libusb_device **devs;
	libusb_device *dev;
	int r;
	int err;
	int ret = 1;
	ssize_t cnt;
	int config = 0;
	int verify = 0;
	struct usb_work w = {};
	int opt;
	char *initfile = NULL;

	while ((opt = getopt(argc, argv, "cvhi:s")) != -1) {
		switch (opt) {
		case 'c':
			verify = 1;
			break;
		case 'v':
			verbose++;
			break;
		case 'h':
			usage(argv[0]);
		case 'i':
			initfile = optarg;
			break;
		case 's':
			skip_image_dcd = 1;
			break;
		default:
			exit(1);
		}
	}

	if (optind == argc) {
		fprintf(stderr, "no filename given\n");
		usage(argv[0]);
		exit(1);
	}

	w.plug = 1;
	w.dcd = 1;
	strncpy(w.filename, argv[optind], sizeof(w.filename) - 1);

	r = libusb_init(NULL);
	if (r < 0)
		goto out;

	cnt = libusb_get_device_list(NULL, &devs);
	if (cnt < 0) {
		fprintf(stderr, "no supported device found\n");
		goto out;
	}

	dev = find_imx_dev(devs, &mach);
	if (!dev) {
		fprintf(stderr, "no supported device found\n");
		goto out;
	}

	err = libusb_open(dev, &usb_dev_handle);
	if (err) {
		fprintf(stderr, "Could not open device vid=0x%x pid=0x%x err=%d\n",
				mach->vid, mach->pid, err);
		goto out;
	}

	libusb_free_device_list(devs, 1);

	libusb_get_configuration(usb_dev_handle, &config);

	if (libusb_kernel_driver_active(usb_dev_handle, 0))
		 libusb_detach_kernel_driver(usb_dev_handle, 0);

	err = libusb_claim_interface(usb_dev_handle, 0);
	if (err) {
		printf("Claim failed\n");
		goto out;
	}

	usb_id = malloc(sizeof(*usb_id));
	if (!usb_id) {
		perror("malloc");
		exit(1);
	}

	usb_id->mach_id = mach;

	err = do_status();
	if (err) {
		printf("status failed\n");
		goto out;
	}

	if (initfile) {
		err = parse_initfile(initfile);
		if (err)
			goto out;
	}

	err = do_irom_download(&w, verify);
	if (err) {
		err = do_status();
		goto out;
	}

	ret = 0;
out:
	if (usb_id)
		free(usb_id);

	if (usb_dev_handle)
		libusb_close(usb_dev_handle);

	libusb_exit(NULL);

	return ret;
}
