// SPDX-License-Identifier: GPL-2.0+ OR BSD-3-Clause
/*
 * Copyright (C) 2018, STMicroelectronics - All Rights Reserved
 * Copyright (C) 2019, Pengutronix
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdint.h>
#include <getopt.h>
#include "compiler.h"

#ifndef MAP_POPULATE
#define MAP_POPULATE 0
#endif

/* magic ='S' 'T' 'M' 0x32 */
#define HEADER_MAGIC htobe32(0x53544D32)
#define VER_MAJOR_IDX	2
#define VER_MINOR_IDX	1
#define VER_VARIANT_IDX	0
/* default option : bit0 => no signature */
#define HEADER_DEFAULT_OPTION	htole32(0x00000001)
/* default binary type for barebox */
#define HEADER_TYPE_BAREBOX	htole32(0x00000000)
#define HEADER_LENGTH	0x100

#define FSBL_LOADADDR		0x2ffc2400
#define FSBL_ENTRYPOINT		(FSBL_LOADADDR + HEADER_LENGTH)
#define MAX_FSBL_PAYLOAD_SIZE	(247 * 1024)

struct __attribute((packed)) stm32_header {
	uint32_t magic_number;
	uint32_t image_signature[64 / 4];
	uint32_t image_checksum;
	uint8_t  header_version[4];
	uint32_t image_length;
	uint32_t image_entry_point;
	uint32_t reserved1;
	uint32_t load_address;
	uint32_t reserved2;
	uint32_t version_number;
	uint32_t option_flags;
	uint32_t ecdsa_algorithm;
	uint32_t ecdsa_public_key[64 / 4];
	uint32_t padding[83 / 4];
	uint32_t binary_type;
};

static struct stm32_header stm32image_header;

static const char *infile;
static const char *outfile;
static int in_fd;
static int out_fd;
static uint32_t loadaddr;
static uint32_t entrypoint;
static uint32_t pbl_size;
static uint32_t version = 0x01;

static void stm32image_print_header(void)
{
	printf("Image Type   : STMicroelectronics STM32 V%d.%d\n",
	       stm32image_header.header_version[VER_MAJOR_IDX],
	       stm32image_header.header_version[VER_MINOR_IDX]);
	printf("Image Size   : %u bytes\n",
	       le32toh(stm32image_header.image_length));
	printf("Image Load   : 0x%08x\n",
	       le32toh(stm32image_header.load_address));
	printf("Entry Point  : 0x%08x\n",
	       le32toh(stm32image_header.image_entry_point));
	printf("Checksum     : 0x%08x\n",
	       le32toh(stm32image_header.image_checksum));
	printf("Option     : 0x%08x\n",
	       le32toh(stm32image_header.option_flags));
	printf("BinaryType : 0x%08x\n",
	       le32toh(stm32image_header.binary_type));
}

static uint32_t stm32image_checksum(void)
{
	uint32_t csum = 0;
	uint32_t len = pbl_size;
	uint8_t *p;

	p = mmap(NULL, len, PROT_READ, MAP_PRIVATE | MAP_POPULATE, in_fd, 0);
	if (p == MAP_FAILED) {
		perror("mmap");
		exit(EXIT_FAILURE);
	}

	for (; len > 0; len--)
		csum += *p++;

	munmap(p, len);

	return csum;
}

static void stm32image_set_header(void)
{

	memset(&stm32image_header, 0, sizeof(struct stm32_header));

	/* set default values */
	stm32image_header.magic_number = HEADER_MAGIC;
	stm32image_header.header_version[VER_MAJOR_IDX] = version;
	stm32image_header.option_flags = HEADER_DEFAULT_OPTION;
	stm32image_header.ecdsa_algorithm = 1;
	/* used to specify the 2nd-stage barebox address within dram */
	stm32image_header.load_address = loadaddr;
	stm32image_header.binary_type = HEADER_TYPE_BAREBOX;

	stm32image_header.image_entry_point = htole32(entrypoint);
	stm32image_header.image_length = htole32(pbl_size);
	stm32image_header.image_checksum = stm32image_checksum();
}

static void stm32image_check_params(void)
{
	off_t ret;

	in_fd = open(infile, O_RDONLY);
	if (in_fd < 0) {
		fprintf(stderr, "Error: Cannot open %s for reading: %s\n", infile,
			strerror(errno));
		exit(EXIT_FAILURE);
	}

	if (!pbl_size) {
		pbl_size = lseek(in_fd, 0, SEEK_END);
		if (pbl_size == (uint32_t)-1) {
			fprintf(stderr, "Cannot seek to end\n");
			exit(EXIT_FAILURE);
		}

		ret = lseek(in_fd, 0, SEEK_SET);
		if (ret == (off_t)-1) {
			fprintf(stderr, "Cannot seek to start\n");
			exit(EXIT_FAILURE);
		}
	}

	out_fd = creat(outfile, 0644);
	if (out_fd < 0) {
		fprintf(stderr, "Cannot open %s for writing: %s\n",
			outfile, strerror(errno));
		exit(EXIT_FAILURE);
	}

	if (loadaddr < 0xc0000000) {
		fprintf(stderr, "Error: loadaddr must be within the DDR memory space\n");
		exit(EXIT_FAILURE);
	}
}

static void copy_fd(int in, int out)
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
}

int main(int argc, char *argv[])
{
	const char *verbose;
	int opt, ret;
	off_t pos;
	entrypoint = FSBL_ENTRYPOINT;

	while ((opt = getopt(argc, argv, "i:o:a:e:s:v:h")) != -1) {
		switch (opt) {
		case 'i':
			infile = optarg;
			break;
		case 'o':
			outfile = optarg;
			break;
		case 'a':
			loadaddr = strtol(optarg, NULL, 16);
			break;
		case 'e':
			entrypoint = strtol(optarg, NULL, 16);
			break;
		case 's':
			pbl_size = strtol(optarg, NULL, 16);
			break;
		case 'v':
			version = strtol(optarg, NULL, 16);
			break;
		case 'h':
			printf("%s [-i inputfile] [-o outputfile] [-a loadaddr] [-s pblimage size in byte]\n", argv[0]);
			exit(EXIT_SUCCESS);
		default:
			fprintf(stderr, "Unknown option: -%c\n", opt);
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

	stm32image_check_params();
	stm32image_set_header();

	ret = write(out_fd, (const void *)&stm32image_header, sizeof(struct stm32_header));
	if (ret != 0x100) {
		fprintf(stderr, "Error: write on %s: %s\n", outfile,
			strerror(errno));
		exit(EXIT_FAILURE);
	}

	verbose = getenv("V");
	if (verbose && !strcmp(verbose, "1"))
		stm32image_print_header();

	ret = ftruncate(out_fd, HEADER_LENGTH);
	if (ret) {
		fprintf(stderr, "Cannot truncate\n");
		exit(EXIT_FAILURE);
	}

	pos = lseek(out_fd, HEADER_LENGTH, SEEK_SET);
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
