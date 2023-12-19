// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2012-2014 Freescale Semiconductor, Inc.
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdint.h>
#include <getopt.h>
#include <endian.h>
#include <byteswap.h>
#include <stdbool.h>
#include <linux/kernel.h>

#include "common.h"
#include "common.c"
#include "../crypto/crc32.c"

#define PBL_ACS_CONT_CMD	0x81000000
#define PBL_ADDR_24BIT_MASK	0x00ffffff

#define RCW_BYTES	64
#define RCW_PREAMBLE	0xaa55aa55
#define RCW_HEADER	0x010e0100

/*
 * The maximum PBL size we support. The PBL portion in the input image shouldn't
 * get bigger than this
 */
#define MAX_PBL_SIZE	(64 * 1024)

/*
 * The offset of the 2nd stage image in the output file. This must match with the
 * offset barebox expects the 2nd stage image.
 */
#define BAREBOX_START	(128 * 1024)

/*
 * need to store all bytes in memory for calculating crc32, then write the
 * bytes to image file for PBL boot.
 */
static unsigned char mem_buf[1000000];
static unsigned char *pmem_buf = mem_buf;
static int pbl_size;
static int pbl_end;
static int image_size;
static int out_fd;
static int in_fd;
static int spiimage;

static uint32_t pbi_crc_cmd1;
static uint32_t pbi_crc_cmd2;

enum soc_type {
	SOC_TYPE_INVALID = -1,
	SOC_TYPE_LS1046A,
	SOC_TYPE_LS1028A,
};

struct soc_type_entry {
	const char *name;
	bool big_endian;
};

static struct soc_type_entry socs[] = {
	[SOC_TYPE_LS1046A] = {
		.name = "ls1046a",
		.big_endian = true,
	},
	[SOC_TYPE_LS1028A] = {
		.name = "ls1028a",
		.big_endian = false,
	},
};

static enum soc_type soc_type = SOC_TYPE_INVALID;

static char *rcwfile;
static char *pbifile;
static char *outfile;
static unsigned long loadaddr = 0x10000000;
static char *infile;

static uint32_t pbl_crc32(uint32_t in_crc, const char *buf, uint32_t len)
{
	return crc32_be(~in_crc, buf, len);
}

/*
 * The PBL can load up to 64 bytes at a time, so we split the image into 64 byte
 * chunks. PBL needs a command for each piece, of the form "81xxxxxx", where
 * "xxxxxx" is the offset. Calculate the start offset by subtracting the size of
 * the image from the top of the allowable 24-bit range.
 */
static void generate_pbl_cmd(uint32_t val)
{
	int i;

	for (i = 3; i >= 0; i--) {
		*pmem_buf++ = (val >> (i * 8)) & 0xff;
		pbl_size++;
	}
}

static void pbl_fget(size_t size, int fd)
{
	int r;

	r = read(fd, pmem_buf, size);
	if (r < 0) {
		perror("read");
		exit(EXIT_FAILURE);
	}

	pmem_buf += r;

	if (r < size) {
		memset(pmem_buf, 0xff, size - r);
		pmem_buf += size - r;
	}

	pbl_size += size;
}

static void check_get_hexval(const char *filename, int lineno, char *token)
{
	uint32_t hexval;
	int i;

	if (!sscanf(token, "%x", &hexval)) {
		fprintf(stderr, "Error:%s[%d] - Invalid hex data(%s)\n",
			filename, lineno, token);
		exit(EXIT_FAILURE);
	}

	if (socs[soc_type].big_endian) {
		for (i = 3; i >= 0; i--) {
			*pmem_buf++ = (hexval >> (i * 8)) & 0xff;
			pbl_size++;
		}
	} else {
		for (i = 0; i < 4; i++) {
			*pmem_buf++ = (hexval >> (i * 8)) & 0xff;
			pbl_size++;
		}
	}
}

static void pbl_parser(char *name)
{
	FILE *f = NULL;
	char *line = NULL;
	char *token, *saveptr1, *saveptr2;
	size_t len = 0;
	int lineno = 0;

	f = fopen(name, "r");
	if (!f) {
		fprintf(stderr, "Error: Cannot open %s: %s\n", name,
			strerror(errno));
		exit(EXIT_FAILURE);
	}

	while ((getline(&line, &len, f)) > 0) {
		lineno++;
		token = strtok_r(line, "\r\n", &saveptr1);
		/* drop all lines with zero tokens (= empty lines) */
		if (!token)
			continue;
		for (line = token;; line = NULL) {
			token = strtok_r(line, " \t", &saveptr2);
			if (!token)
				break;
			/* Drop all text starting with '#' as comments */
			if (token[0] == '#')
				break;
			check_get_hexval(name, lineno, token);
		}
	}
	if (line)
		free(line);
	fclose(f);
}

/* write end command and crc command to memory. */
static void add_end_cmd(void)
{
	uint32_t crc32_pbl;

	/* Add PBI CRC command. */
	*pmem_buf++ = 0x08;
	*pmem_buf++ = pbi_crc_cmd1;
	*pmem_buf++ = pbi_crc_cmd2;
	*pmem_buf++ = 0x40;
	pbl_size += 4;

	/* calculated CRC32 and write it to memory. */
	crc32_pbl = pbl_crc32(0, (const char *)mem_buf, pbl_size);
	*pmem_buf++ = (crc32_pbl >> 24) & 0xff;
	*pmem_buf++ = (crc32_pbl >> 16) & 0xff;
	*pmem_buf++ = (crc32_pbl >> 8) & 0xff;
	*pmem_buf++ = (crc32_pbl) & 0xff;
	pbl_size += 4;
}

static void pbl_load_image(void)
{
	int size;
	unsigned int n;
	uint64_t *buf64 = (void *)mem_buf;
	uint32_t *buf32 = (void *)mem_buf;

	/* parse the rcw.cfg file. */
	pbl_parser(rcwfile);

	if (soc_type == SOC_TYPE_LS1028A) {
		uint32_t chksum = 0;
		int i;

		for (i = 0; i < 34; i++)
			chksum += buf32[i];

		buf32[0x22] = chksum;
		pbl_size += 4;
		pmem_buf += 4;
	}

	/* parse the pbi.cfg file. */
	if (pbifile)
		pbl_parser(pbifile);

	if (soc_type == SOC_TYPE_LS1046A) {
		for (n = 0; n < image_size; n += 0x40) {
			uint32_t pbl_cmd;

			pbl_cmd = (loadaddr & PBL_ADDR_24BIT_MASK) | PBL_ACS_CONT_CMD;
			pbl_cmd += n;
			generate_pbl_cmd(pbl_cmd);
			pbl_fget(64, in_fd);
		}

		add_end_cmd();
	} else if (soc_type == SOC_TYPE_LS1028A) {
		buf32 = (void *)pmem_buf;

		buf32[0] = 0x80000008;
		buf32[1] = 0x2000;
		buf32[2] = 0x18010000;
		buf32[3] = image_size;
		buf32[4] = 0x80ff0000;
		pbl_size += 20;
		pmem_buf += 20;

		read(in_fd, mem_buf + 0x1000, image_size);
		pbl_size = 0x1000 + image_size;
		printf("%s imagesize: %d\n", rcwfile, image_size);
	} else {
		exit(EXIT_FAILURE);
	}

	if (spiimage) {
		int i;

		pbl_size = roundup(pbl_size, 8);

		for (i = 0; i < pbl_size / 8; i++)
			buf64[i] = bswap_64(buf64[i]);
	}

	size = pbl_size;

	if (write(out_fd, (const void *)&mem_buf, size) != size) {
		fprintf(stderr, "Write error on %s: %s\n",
			outfile, strerror(errno));
		exit(EXIT_FAILURE);
	}
}

static int pblimage_check_params(void)
{
	struct stat st;

	in_fd = open(infile, O_RDONLY);
	if (in_fd < 0) {
		fprintf(stderr, "Error: Cannot open %s: %s\n", infile,
			strerror(errno));
		exit(EXIT_FAILURE);
	}

	if (fstat(in_fd, &st) == -1) {
		fprintf(stderr, "Error: Could not determine u-boot image size. %s\n",
		       strerror(errno));
		exit(EXIT_FAILURE);
	}

	/* For the variable size, pad it to 64 byte boundary */
	if (soc_type == SOC_TYPE_LS1046A)
		image_size = roundup(pbl_end, 64);
	else if (soc_type == SOC_TYPE_LS1028A)
		image_size = roundup(pbl_end, 512);
	else
		exit(EXIT_FAILURE);

	if (image_size > MAX_PBL_SIZE) {
		fprintf(stderr, "Error: pbl size %d in %s exceeds maximum size %d\n",
		       pbl_end, infile, MAX_PBL_SIZE);
		exit(EXIT_FAILURE);
	}

	pbi_crc_cmd1 = 0x61;
	pbi_crc_cmd2 = 0;

	return 0;
};

static int copy_fd(int in, int out)
{
	int bs = 4096;
	void *buf = malloc(bs);

	if (!buf)
		exit(EXIT_FAILURE);

	while (1) {
		int now, wr;

		now = read(in, buf, bs);
		if (now < 0) {
			fprintf(stderr, "read failed with %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}

		if (!now)
			break;

		wr = write(out, buf, now);
		if (wr < 0) {
			fprintf(stderr, "write failed with %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}

		if (wr != now) {
			fprintf(stderr, "short write\n");
			exit(EXIT_FAILURE);
		}
	}

	free(buf);

	return 0;
}

int main(int argc, char *argv[])
{
	int opt, ret, i;
	off_t pos;
	char *cputypestr = NULL;

	while ((opt = getopt(argc, argv, "i:r:p:o:m:sc:")) != -1) {
		switch (opt) {
		case 'i':
			infile = optarg;
			break;
		case 'r':
			rcwfile = optarg;
			break;
		case 'p':
			pbifile = optarg;
			break;
		case 'o':
			outfile = optarg;
			break;
		case 'm':
			pbl_end = atoi(optarg);
			break;
		case 's':
			spiimage = 1;
			break;
		case 'c':
			cputypestr = optarg;
			break;
		default:
			exit(EXIT_FAILURE);
		}
	}

	if (!infile) {
		fprintf(stderr, "No infile given\n");
		exit(EXIT_FAILURE);
	}

	if (!outfile) {
		fprintf(stderr, "No outfile given\n");
		exit(EXIT_FAILURE);
	}

	if (!rcwfile) {
		fprintf(stderr, "No rcwfile given\n");
		exit(EXIT_FAILURE);
	}

	if (!cputypestr) {
		fprintf(stderr, "No CPU type given\n");
		exit(EXIT_FAILURE);
	}

	for (i = 0; i < ARRAY_SIZE(socs); i++) {
		if (!strcmp(socs[i].name, cputypestr)) {
			soc_type = i;
			break;
		}
	}

	if (soc_type == SOC_TYPE_INVALID) {
		fprintf(stderr, "Invalid CPU type %s. Valid types are:\n", cputypestr);
		for (i = 0; i < ARRAY_SIZE(socs); i++)
			printf("  %s\n", socs[i].name);

		exit(EXIT_FAILURE);
	}

	pblimage_check_params();

	out_fd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (out_fd < 0) {
		fprintf(stderr, "Cannot open %s for writing: %s\n",
			outfile, strerror(errno));
		exit(EXIT_FAILURE);
	}

	pbl_load_image();

	ret = ftruncate(out_fd, BAREBOX_START);
	if (ret) {
		fprintf(stderr, "Cannot truncate\n");
		exit(EXIT_FAILURE);
	}

	pos = lseek(out_fd, BAREBOX_START, SEEK_SET);
	if (pos == (off_t)-1) {
		fprintf(stderr, "Cannot lseek 1\n");
		exit(EXIT_FAILURE);
	}

	pos = lseek(in_fd, 0, SEEK_SET);
	if (pos == (off_t)-1) {
		fprintf(stderr, "Cannot lseek 2\n");
		exit(EXIT_FAILURE);
	}

	copy_fd(in_fd, out_fd);

	close(in_fd);
	close(out_fd);

	exit(EXIT_SUCCESS);
}
