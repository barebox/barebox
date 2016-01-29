/*
 * (C) Copyright 2013 Sascha Hauer, Pengutronix
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <endian.h>
#include <linux/kernel.h>

#include "imx.h"

#include <include/filetype.h>

#define MAX_DCD 1024
#define HEADER_LEN 0x1000	/* length of the blank area + IVT + DCD */
#define CSF_LEN 0x2000		/* length of the CSF (needed for HAB) */

static uint32_t dcdtable[MAX_DCD];
static int curdcd;
static int add_barebox_header;
static int prepare_sign;

/*
 * ============================================================================
 * i.MX flash header v1 handling. Found on i.MX35 and i.MX51
 * ============================================================================
 */

#define FLASH_HEADER_OFFSET 0x400

static uint32_t bb_header[] = {
	0xea0003fe,	/* b 0x1000 */
	0xeafffffe,	/* 1: b 1b  */
	0xeafffffe,	/* 1: b 1b  */
	0xeafffffe,	/* 1: b 1b  */
	0xeafffffe,	/* 1: b 1b  */
	0xeafffffe,	/* 1: b 1b  */
	0xeafffffe,	/* 1: b 1b  */
	0xeafffffe,	/* 1: b 1b  */
	0x65726162,	/* 'bare'   */
	0x00786f62,	/* 'box\0'  */
	0x00000000,
	0x00000000,
	0x55555555,
	0x55555555,
	0x55555555,
	0x55555555,
	0x55555555,
	0x55555555,
	0x55555555,
	0x55555555,
};

static int add_header_v1(struct config_data *data, void *buf)
{
	struct imx_flash_header *hdr;
	int dcdsize = curdcd * sizeof(uint32_t);
	uint32_t *psize = buf + ARM_HEAD_SIZE_OFFSET;
	int offset = data->image_dcd_offset;
	uint32_t loadaddr = data->image_load_addr;
	uint32_t imagesize = data->load_size;

	if (add_barebox_header) {
		memcpy(buf, bb_header, sizeof(bb_header));
		*psize = imagesize;
	}

	buf += offset;
	hdr = buf;

	hdr->app_code_jump_vector = loadaddr + 0x1000;
	hdr->app_code_barker = 0x000000b1;
	hdr->app_code_csf = 0x0;
	hdr->dcd_ptr_ptr = loadaddr + offset + offsetof(struct imx_flash_header, dcd);
	hdr->super_root_key = 0x0;
	hdr->dcd = loadaddr + offset + offsetof(struct imx_flash_header, dcd_barker);
	hdr->app_dest = loadaddr;
	hdr->dcd_barker = DCD_BARKER;
	hdr->dcd_block_len = dcdsize;

	buf += sizeof(struct imx_flash_header);

	memcpy(buf, dcdtable, dcdsize);

	buf += dcdsize;

	*(uint32_t *)buf = imagesize;

	return 0;
}

static int write_mem_v1(uint32_t addr, uint32_t val, int width)
{
	if (curdcd > MAX_DCD - 3) {
		fprintf(stderr, "At maximum %d dcd entried are allowed\n", MAX_DCD);
		return -ENOMEM;
	}

	dcdtable[curdcd++] = width;
	dcdtable[curdcd++] = addr;
	dcdtable[curdcd++] = val;

	return 0;
}

/*
 * ============================================================================
 * i.MX flash header v2 handling. Found on i.MX53 and i.MX6
 * ============================================================================
 */

static int add_header_v2(struct config_data *data, void *buf)
{
	struct imx_flash_header_v2 *hdr;
	int dcdsize = curdcd * sizeof(uint32_t);
	uint32_t *psize = buf + ARM_HEAD_SIZE_OFFSET;
	int offset = data->image_dcd_offset;
	uint32_t loadaddr = data->image_load_addr;
	uint32_t imagesize = data->load_size;

	if (add_barebox_header)
		memcpy(buf, bb_header, sizeof(bb_header));

	buf += offset;
	hdr = buf;

	hdr->header.tag		= TAG_IVT_HEADER;
	hdr->header.length	= htobe16(32);
	hdr->header.version	= IVT_VERSION;

	hdr->entry		= loadaddr + HEADER_LEN;
	hdr->dcd_ptr		= loadaddr + offset + offsetof(struct imx_flash_header_v2, dcd_header);
	hdr->boot_data_ptr	= loadaddr + offset + offsetof(struct imx_flash_header_v2, boot_data);
	hdr->self		= loadaddr + offset;

	hdr->boot_data.start	= loadaddr;
	hdr->boot_data.size	= imagesize;

	if (prepare_sign) {
		hdr->csf = loadaddr + imagesize;
		hdr->boot_data.size += CSF_LEN;
	}

	if (add_barebox_header)
		*psize = hdr->boot_data.size;

	hdr->dcd_header.tag	= TAG_DCD_HEADER;
	hdr->dcd_header.length	= htobe16(sizeof(uint32_t) + dcdsize);
	hdr->dcd_header.version	= DCD_VERSION;

	buf += sizeof(*hdr);

	memcpy(buf, dcdtable, dcdsize);

	return 0;
}

static void usage(const char *prgname)
{
	fprintf(stderr, "usage: %s [OPTIONS]\n\n"
		"-c <config>  specify configuration file\n"
		"-f <input>   input image file\n"
		"-o <output>  output file\n"
		"-b           add barebox header to image. If used, barebox recognizes\n"
		"             the image as regular barebox image which can be used as\n"
		"             second stage image\n"
		"-p           prepare image for signing\n"
		"-h           this help\n", prgname);
	exit(1);
}

static uint32_t last_write_cmd;
static int last_cmd_len;
static uint32_t *last_dcd;

static void check_last_dcd(uint32_t cmd)
{
	cmd &= 0xff0000ff;

	if (cmd == last_write_cmd) {
		last_cmd_len += sizeof(uint32_t) * 2;
		return;
	}

	/* write length ... */
	if (last_write_cmd)
		*last_dcd = htobe32(last_write_cmd | (last_cmd_len << 8));

	if ((cmd >> 24) == TAG_WRITE) {
		last_write_cmd = cmd;
		last_dcd = &dcdtable[curdcd++];
		last_cmd_len = sizeof(uint32_t) * 3;
	} else {
		last_write_cmd = 0;
	}
}

static int write_mem_v2(uint32_t addr, uint32_t val, int width)
{
	uint32_t cmd;

	cmd = (TAG_WRITE << 24) | width;

	if (curdcd > MAX_DCD - 3) {
		fprintf(stderr, "At maximum %d dcd entried are allowed\n", MAX_DCD);
		return -ENOMEM;
	}

	check_last_dcd(cmd);

	dcdtable[curdcd++] = htobe32(addr);
	dcdtable[curdcd++] = htobe32(val);

	return 0;
}

static int xread(int fd, void *buf, int len)
{
	int ret;

	while (len) {
		ret = read(fd, buf, len);
		if (ret < 0)
			return ret;
		if (!ret)
			return EOF;
		buf += ret;
		len -= ret;
	}

	return 0;
}

static int xwrite(int fd, void *buf, int len)
{
	int ret;

	while (len) {
		ret = write(fd, buf, len);
		if (ret < 0)
			return ret;
		buf += ret;
		len -= ret;
	}

	return 0;
}

static int write_dcd(const char *outfile)
{
	int outfd, ret;
	int dcdsize = curdcd * sizeof(uint32_t);

	outfd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	if (outfd < 0) {
		perror("open");
		exit(1);
	}

	ret = xwrite(outfd, dcdtable, dcdsize);
	if (ret < 0) {
		perror("write");
		exit(1);
	}

	return 0;
}

static int check(struct config_data *data, uint32_t cmd, uint32_t addr, uint32_t mask)
{
	if (curdcd > MAX_DCD - 3) {
		fprintf(stderr, "At maximum %d dcd entried are allowed\n", MAX_DCD);
		return -ENOMEM;
	}

	check_last_dcd(cmd);

	dcdtable[curdcd++] = htobe32(cmd);
	dcdtable[curdcd++] = htobe32(addr);
	dcdtable[curdcd++] = htobe32(mask);

	return 0;
}

static int write_mem(struct config_data *data, uint32_t addr, uint32_t val, int width)
{
	switch (data->header_version) {
	case 1:
		return write_mem_v1(addr, val, width);
	case 2:
		return write_mem_v2(addr, val, width);
	default:
		return -EINVAL;
	}
}

int main(int argc, char *argv[])
{
	int opt, ret;
	char *configfile = NULL;
	char *imagename = NULL;
	void *buf;
	size_t insize;
	void *infile;
	struct stat s;
	int infd, outfd;
	int dcd_only = 0;
	int now = 0;
	struct config_data data = {
		.image_dcd_offset = 0xffffffff,
		.write_mem = write_mem,
		.check = check,
	};

	while ((opt = getopt(argc, argv, "c:hf:o:bdp")) != -1) {
		switch (opt) {
		case 'c':
			configfile = optarg;
			break;
		case 'f':
			imagename = optarg;
			break;
		case 'o':
			data.outfile = optarg;
			break;
		case 'b':
			add_barebox_header = 1;
			break;
		case 'd':
			dcd_only = 1;
			break;
		case 'p':
			prepare_sign = 1;
			break;
		case 'h':
			usage(argv[0]);
		default:
			exit(1);
		}
	}

	if (!imagename && !dcd_only) {
		fprintf(stderr, "image name not given\n");
		exit(1);
	}

	if (!configfile) {
		fprintf(stderr, "config file not given\n");
		exit(1);
	}

	if (!data.outfile) {
		fprintf(stderr, "output file not given\n");
		exit(1);
	}

	if (!dcd_only) {
		ret = stat(imagename, &s);
		if (ret) {
			perror("stat");
			exit(1);
		}

		data.image_size = s.st_size;
	}

	ret = parse_config(&data, configfile);
	if (ret)
		exit(1);

	buf = calloc(1, HEADER_LEN);
	if (!buf)
		exit(1);

	if (data.image_dcd_offset == 0xffffffff) {
		fprintf(stderr, "no dcd offset given ('dcdofs'). Defaulting to 0x%08x\n",
			FLASH_HEADER_OFFSET);
		data.image_dcd_offset = FLASH_HEADER_OFFSET;
	}

	if (!data.header_version) {
		fprintf(stderr, "no SoC given. (missing 'soc' in config)\n");
		exit(1);
	}

	if (data.header_version == 2)
		check_last_dcd(0);

	if (dcd_only) {
		ret = write_dcd(data.outfile);
		if (ret)
			exit(1);
		exit (0);
	}

	/*
	 * Add HEADER_LEN to the image size for the blank aera + IVT + DCD.
	 * Align up to a 4k boundary, because:
	 * - at least i.MX5 NAND boot only reads full NAND pages and misses the
	 *   last partial NAND page.
	 * - i.MX6 SPI NOR boot corrupts the last few bytes of an image loaded
	 *   in ver funy ways when the image size is not 4 byte aligned
	 */
	data.load_size = roundup(data.image_size + HEADER_LEN, 0x1000);

	if (data.cpu_type == 35)
		data.load_size += HEADER_LEN;

	switch (data.header_version) {
	case 1:
		add_header_v1(&data, buf);
		break;
	case 2:
		add_header_v2(&data, buf);
		break;
	default:
		fprintf(stderr, "Congratulations! You're welcome to implement header version %d\n",
				data.header_version);
		exit(1);
	}

	infd = open(imagename, O_RDONLY);
	if (infd < 0) {
		perror("open");
		exit(1);
	}

	ret = fstat(infd, &s);
	if (ret)
		return ret;

	insize = s.st_size;
	infile = malloc(insize);
	if (!infile)
		exit(1);

	xread(infd, infile, insize);
	close(infd);

	outfd = open(data.outfile, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	if (outfd < 0) {
		perror("open");
		exit(1);
	}

	ret = xwrite(outfd, buf, HEADER_LEN);
	if (ret < 0) {
		perror("write");
		exit(1);
	}

	if (data.cpu_type == 35) {
		ret = xwrite(outfd, buf, HEADER_LEN);
		if (ret < 0) {
			perror("write");
			exit(1);
		}
	}

	ret = xwrite(outfd, infile, insize);
	if (ret) {
		perror("write");
		exit(1);
	}

	/* pad until next 4k boundary */
	now = 4096 - (insize % 4096);
	if (prepare_sign && now) {
		memset(buf, 0x5a, now);

		ret = xwrite(outfd, buf, now);
		if (ret) {
			perror("write");
			exit(1);
		}
	}

	ret = close(outfd);
	if (ret) {
		perror("close");
		exit(1);
	}

	exit(0);
}
