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

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#define offsetof(TYPE, MEMBER) __builtin_offsetof(TYPE, MEMBER)

#define MAX_DCD 1024

static uint32_t image_load_addr;
static uint32_t image_dcd_offset;
static uint32_t dcdtable[MAX_DCD];
static int curdcd;
static int header_version;
static int add_barebox_header;

/*
 * ============================================================================
 * i.MX flash header v1 handling. Found on i.MX35 and i.MX51
 * ============================================================================
 */
struct imx_flash_header {
	uint32_t app_code_jump_vector;
	uint32_t app_code_barker;
	uint32_t app_code_csf;
	uint32_t dcd_ptr_ptr;
	uint32_t super_root_key;
	uint32_t dcd;
	uint32_t app_dest;
	uint32_t dcd_barker;
	uint32_t dcd_block_len;
} __attribute__((packed));

#define FLASH_HEADER_OFFSET 0x400
#define DCD_BARKER       0xb17219e9

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

static int add_header_v1(void *buf, int offset, uint32_t loadaddr, uint32_t imagesize)
{
	struct imx_flash_header *hdr;
	int dcdsize = curdcd * sizeof(uint32_t);

	if (add_barebox_header)
		memcpy(buf, bb_header, sizeof(bb_header));

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

struct imx_boot_data {
	uint32_t start;
	uint32_t size;
	uint32_t plugin;
} __attribute__((packed));

#define TAG_IVT_HEADER	0xd1
#define IVT_VERSION	0x40
#define TAG_DCD_HEADER	0xd2
#define DCD_VERSION	0x40
#define TAG_WRITE	0xcc
#define TAG_CHECK	0xcf

struct imx_ivt_header {
	uint8_t tag;
	uint16_t length;
	uint8_t version;
} __attribute__((packed));

struct imx_flash_header_v2 {
	struct imx_ivt_header header;

	uint32_t entry;
	uint32_t reserved1;
	uint32_t dcd_ptr;
	uint32_t boot_data_ptr;
	uint32_t self;
	uint32_t csf;
	uint32_t reserved2;

	struct imx_boot_data boot_data;
	struct imx_ivt_header dcd_header;
} __attribute__((packed));

static int add_header_v2(void *buf, int offset, uint32_t loadaddr, uint32_t imagesize)
{
	struct imx_flash_header_v2 *hdr;
	int dcdsize = curdcd * sizeof(uint32_t);

	if (add_barebox_header)
		memcpy(buf, bb_header, sizeof(bb_header));

	buf += offset;
	hdr = buf;

	hdr->header.tag		= TAG_IVT_HEADER;
	hdr->header.length	= htobe16(32);
	hdr->header.version	= IVT_VERSION;

	hdr->entry		= loadaddr + 0x1000;
	hdr->dcd_ptr		= loadaddr + 0x400 + offsetof(struct imx_flash_header_v2, dcd_header);
	hdr->boot_data_ptr	= loadaddr + 0x400 + offsetof(struct imx_flash_header_v2, boot_data);
	hdr->self		= loadaddr + 0x400;

	hdr->boot_data.start	= loadaddr;
	hdr->boot_data.size	= imagesize;

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
		"-h           this help\n", prgname);
	exit(1);
}

#define MAXARGS 5

static int parse_line(char *line, char *argv[])
{
	int nargs = 0;

	while (nargs < MAXARGS) {

		/* skip any white space */
		while ((*line == ' ') || (*line == '\t'))
			++line;

		if (*line == '\0')	/* end of line, no more args	*/
			argv[nargs] = NULL;

		if (*line == '\0') {	/* end of line, no more args	*/
			argv[nargs] = NULL;
			return nargs;
		}

		argv[nargs++] = line;	/* begin of argument string	*/

		/* find end of string */
		while (*line && (*line != ' ') && (*line != '\t'))
			++line;

		if (*line == '\0') {	/* end of line, no more args	*/
			argv[nargs] = NULL;
			return nargs;
		}

		*line++ = '\0';		/* terminate current arg	 */
	}

	printf("** Too many args (max. %d) **\n", MAXARGS);

	return nargs;
}

struct command {
	const char *name;
	int (*parse)(int argc, char *argv[]);
};

static uint32_t last_cmd;
static int last_cmd_len;
static uint32_t *last_dcd;

static void check_last_dcd(uint32_t cmd)
{
	if (last_dcd) {
		if (last_cmd == cmd) {
			return;
		} else {
			uint32_t l = be32toh(*last_dcd);

			l |= last_cmd_len << 8;

			*last_dcd = htobe32(l);

			last_dcd = NULL;
		}
	}

	if (!cmd)
		return;

	if (!last_dcd) {
		last_dcd = &dcdtable[curdcd++];
		*last_dcd = htobe32(cmd);
		last_cmd_len = sizeof(uint32_t);
		last_cmd = cmd;
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

	last_cmd_len += sizeof(uint32_t) * 2;
	dcdtable[curdcd++] = htobe32(addr);
	dcdtable[curdcd++] = htobe32(val);

	return 0;
}

static const char *check_cmds[] = {
	"while_all_bits_clear",		/* while ((*address & mask) == 0); */
	"while_all_bits_set"	,	/* while ((*address & mask) == mask); */
	"while_any_bit_clear",		/* while ((*address & mask) != mask); */
	"while_any_bit_set",		/* while ((*address & mask) != 0); */
};

static void do_cmd_check_usage(void)
{
	fprintf(stderr,
			"usage: check <width> <cmd> <addr> <mask>\n"
			"<width> access width in bytes [1|2|4]\n"
			"with <cmd> one of:\n"
			"while_all_bits_clear: while ((*addr & mask) == 0)\n"
			"while_all_bits_set:   while ((*addr & mask) == mask)\n"
			"while_any_bit_clear:  while ((*addr & mask) != mask)\n"
			"while_any_bit_set:    while ((*addr & mask) != 0)\n");
}

static int do_cmd_check(int argc, char *argv[])
{
	uint32_t addr, mask, cmd;
	int i, width;
	const char *scmd;

	if (argc < 5) {
		do_cmd_check_usage();
		return -EINVAL;
	}

	width = strtoul(argv[1], NULL, 0) >> 3;
	scmd = argv[2];
	addr = strtoul(argv[3], NULL, 0);
	mask = strtoul(argv[4], NULL, 0);

	switch (width) {
	case 1:
	case 2:
	case 4:
		break;
	default:
		fprintf(stderr, "illegal width %d\n", width);
		return -EINVAL;
	};

	if (curdcd > MAX_DCD - 3) {
		fprintf(stderr, "At maximum %d dcd entried are allowed\n", MAX_DCD);
		return -ENOMEM;
	}

	for (i = 0; i < ARRAY_SIZE(check_cmds); i++) {
		if (!strcmp(scmd, check_cmds[i]))
			break;
	}

	if (i == ARRAY_SIZE(check_cmds)) {
		do_cmd_check_usage();
		return -EINVAL;
	}

	cmd = (TAG_CHECK << 24) | (i << 3) | width;

	check_last_dcd(cmd);

	last_cmd_len += sizeof(uint32_t) * 2;
	dcdtable[curdcd++] = htobe32(addr);
	dcdtable[curdcd++] = htobe32(mask);

	return 0;
}

static int do_cmd_write_mem(int argc, char *argv[])
{
	uint32_t addr, val, width;

	if (argc != 4) {
		fprintf(stderr, "usage: wm [8|16|32] <addr> <val>\n");
		return -EINVAL;
	}

	width = strtoul(argv[1], NULL, 0);
	addr = strtoul(argv[2], NULL, 0);
	val = strtoul(argv[3], NULL, 0);

	width >>= 3;

	switch (width) {
	case 1:
	case 2:
	case 4:
		break;
	default:
		fprintf(stderr, "illegal width %d\n", width);
		return -EINVAL;
	};

	switch (header_version) {
	case 1:
		return write_mem_v1(addr, val, width);
	case 2:
		return write_mem_v2(addr, val, width);
	default:
		return -EINVAL;
	}
}

static int do_loadaddr(int argc, char *argv[])
{
	if (argc < 2)
		return -EINVAL;

	image_load_addr = strtoul(argv[1], NULL, 0);

	return 0;
}

static int do_dcd_offset(int argc, char *argv[])
{
	if (argc < 2)
		return -EINVAL;

	image_dcd_offset = strtoul(argv[1], NULL, 0);

	return 0;
}

struct soc_type {
	char *name;
	int header_version;
};

static struct soc_type socs[] = {
	{ .name = "imx35", .header_version = 1, },
	{ .name = "imx51", .header_version = 1, },
	{ .name = "imx53", .header_version = 2, },
	{ .name = "imx6", .header_version = 2, },
};

static int do_soc(int argc, char *argv[])
{
	char *soc;
	int i;

	if (argc < 2)
		return -EINVAL;

	soc = argv[1];

	for (i = 0; i < ARRAY_SIZE(socs); i++) {
		if (!strcmp(socs[i].name, soc)) {
			header_version = socs[i].header_version;
			return 0;
		}
	}

	fprintf(stderr, "unkown SoC type \"%s\". Known SoCs are:\n", soc);
	for (i = 0; i < ARRAY_SIZE(socs); i++)
		fprintf(stderr, "%s ", socs[i].name);
	fprintf(stderr, "\n");

	return -EINVAL;
}

struct command cmds[] = {
	{
		.name = "wm",
		.parse = do_cmd_write_mem,
	}, {
		.name = "check",
		.parse = do_cmd_check,
	}, {
		.name = "loadaddr",
		.parse = do_loadaddr,
	}, {
		.name = "dcdofs",
		.parse = do_dcd_offset,
	}, {
		.name = "soc",
		.parse = do_soc,
	},
};

static char *readcmd(FILE *f)
{
	static char *buf;
	char *str;
	ssize_t ret;

	if (!buf) {
		buf = malloc(4096);
		if (!buf)
			return NULL;
	}

	str = buf;
	*str = 0;

	while (1) {
		ret = fread(str, 1, 1, f);
		if (!ret)
			return strlen(buf) ? buf : NULL;

		if (*str == '\n' || *str == ';') {
			*str = 0;
			return buf;
		}

		str++;
	}
}

static int parse_config(const char *filename)
{
	FILE *f;
	int lineno = 0;
	char *line = NULL, *tmp;
	char *argv[MAXARGS];
	int nargs, i, ret;

	f = fopen(filename, "r");
	if (!f) {
		fprintf(stderr, "Error: %s - Can't open DCD file\n", filename);
		exit(1);
	}

	while (1) {
		line = readcmd(f);
		if (!line)
			break;

		lineno++;

		tmp = strchr(line, '#');
		if (tmp)
			*tmp = 0;

		nargs = parse_line(line, argv);
		if (!nargs)
			continue;

		ret = -ENOENT;

		for (i = 0; i < ARRAY_SIZE(cmds); i++) {
			if (!strcmp(cmds[i].name, argv[0])) {
				ret = cmds[i].parse(nargs, argv);
				if (ret) {
					fprintf(stderr, "error in line %d: %s\n",
							lineno, strerror(-ret));
					return ret;
				}
				break;
			}
		}

		if (ret == -ENOENT) {
			fprintf(stderr, "no such command: %s\n", argv[0]);
			return ret;
		}
	}

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

int main(int argc, char *argv[])
{
	int opt, ret;
	char *configfile = NULL;
	char *imagename = NULL;
	char *outfile = NULL;
	void *buf;
	size_t image_size = 0, load_size;
	struct stat s;
	int infd, outfd;
	int dcd_only = 0;

	while ((opt = getopt(argc, argv, "c:hf:o:bd")) != -1) {
		switch (opt) {
		case 'c':
			configfile = optarg;
			break;
		case 'f':
			imagename = optarg;
			break;
		case 'o':
			outfile = optarg;
			break;
		case 'b':
			add_barebox_header = 1;
			break;
		case 'd':
			dcd_only = 1;
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

	if (!outfile) {
		fprintf(stderr, "output file not given\n");
		exit(1);
	}

	if (!dcd_only) {
		ret = stat(imagename, &s);
		if (ret) {
			perror("stat");
			exit(1);
		}

		image_size = s.st_size;
	}

	ret = parse_config(configfile);
	if (ret)
		exit(1);

	buf = calloc(4096, 1);
	if (!buf)
		exit(1);

	if (!image_dcd_offset) {
		fprintf(stderr, "no dcd offset given ('dcdofs'). Defaulting to 0x400\n");
		image_dcd_offset = 0x400;
	}

	if (!header_version) {
		fprintf(stderr, "no SoC given. (missing 'soc' in config)\n");
		exit(1);
	}

	if (header_version == 2)
		check_last_dcd(0);

	if (dcd_only) {
		ret = write_dcd(outfile);
		if (ret)
			exit(1);
		exit (0);
	}

	/*
	 * Add 0x1000 to the image size for the DCD.
	 * Align up to a 4k boundary, because:
	 * - at least i.MX5 NAND boot only reads full NAND pages and misses the
	 *   last partial NAND page.
	 * - i.MX6 SPI NOR boot corrupts the last few bytes of an image loaded
	 *   in ver funy ways when the image size is not 4 byte aligned
	 */
	load_size = ((image_size + 0x1000) + 0xfff) & ~0xfff;

	switch (header_version) {
	case 1:
		add_header_v1(buf, image_dcd_offset, image_load_addr, load_size);
		break;
	case 2:
		add_header_v2(buf, image_dcd_offset, image_load_addr, load_size);
		break;
	default:
		fprintf(stderr, "Congratulations! You're welcome to implement header version %d\n",
				header_version);
		exit(1);
	}

	outfd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	if (outfd < 0) {
		perror("open");
		exit(1);
	}

	ret = xwrite(outfd, buf, 4096);
	if (ret < 0) {
		perror("write");
		exit(1);
	}

	infd = open(imagename, O_RDONLY);
	if (infd < 0) {
		perror("open");
		exit(1);
	}

	while (image_size) {
		int now = image_size < 4096 ? image_size : 4096;

		ret = xread(infd, buf, now);
		if (ret) {
			perror("read");
			exit(1);
		}

		ret = xwrite(outfd, buf, now);
		if (ret) {
			perror("write");
			exit(1);
		}

		image_size -= now;
	}

	ret = close(outfd);
	if (ret) {
		perror("close");
		exit(1);
	}

	exit(0);
}
