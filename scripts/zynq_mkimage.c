/*
 * Copyright (C) 2013 Steffen Trumtrar <s.trumtrar@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <endian.h>
#include <errno.h>
#include <getopt.h>
#include <linux/kernel.h>
#include <mach/zynq-flash-header.h>
#include <malloc.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

static char *prgname;

static void usage(char *name)
{
	fprintf(stderr, "usage: %s [OPTIONS]\n\n"
			"-c <config>  configuration input file"
			"-f <input>   input image file\n"
			"-o <output>  output file\n"
			"-h           this help\n", prgname);
		exit(1);
}

#define MAXARGS 16

static int parse_line(char *line, char *argv[])
{
	int nargs = 0;

	while (nargs < MAXARGS) {

		/* skip any white space */
		while ((*line == ' ') || (*line == '\t'))
			++line;

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
	int (*parse)(char *buf, int argc, char *argv[]);
};

static int do_cmd_write_mem(char *buf, int argc, char *argv[])
{
	unsigned int *wordbuf = (unsigned int *)(buf + REGINIT_OFFSET);
	static int reginit_offset;
	uint32_t addr, val, width;
	char *end;

	if (argc != 4) {
		fprintf(stderr, "usage: wm [8|16|32] <addr> <val>\n");
		return -EINVAL;
	}

	width = strtoul(argv[1], &end, 0);
	if (*end != '\0' || width != 32) {
		fprintf(stderr, "illegal width token \"%s\"\n", argv[1]);
		return -EINVAL;
	}

	addr = strtoul(argv[2], &end, 0);
	if (*end != '\0') {
		fprintf(stderr, "illegal address token \"%s\"\n", argv[2]);
		return -EINVAL;
	}

	val = strtoul(argv[3], &end, 0);
	if (*end != '\0') {
		fprintf(stderr, "illegal value token \"%s\"\n", argv[3]);
		return -EINVAL;
	}

	wordbuf[reginit_offset] = htole32(addr);
	wordbuf[reginit_offset + 1] = htole32(val);
	wordbuf[reginit_offset + 1] = htole32(val);

	reginit_offset += 2;

	return 0;
}

struct command cmds[] = {
	{
		.name = "wm",
		.parse = do_cmd_write_mem,
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

int parse_config(char *buf, const char *filename)
{
	FILE *f;
	int lineno = 0;
	char *line = NULL, *tmp;
	char *argv[MAXARGS];
	int nargs, i, ret = 0;

	f = fopen(filename, "r");
	if (!f) {
		fprintf(stderr, "Cannot open config file\n");
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
				ret = cmds[i].parse(buf, nargs, argv);
				if (ret) {
					fprintf(stderr, "error in line %d: %s\n",
							lineno, strerror(-ret));
					goto cleanup;
				}
				break;
			}
		}

		if (ret == -ENOENT) {
			fprintf(stderr, "no such command: %s\n", argv[0]);
			goto cleanup;
		}
	}

cleanup:
	fclose(f);
	return ret;
}

static void add_header(char *buf, unsigned int image_size)
{
	unsigned int *wordbuf = (unsigned int *)buf;
	struct zynq_flash_header flash_header = {
		.width_det			= WIDTH_DETECTION_MAGIC,
		.image_id			= IMAGE_IDENTIFICATION,
		.enc_stat			= 0x0,
		.user				= 0x0,
		.flash_offset		= IMAGE_OFFSET,
		.length				= image_size,
		.res0				= 0x0,
		.start_of_exec		= 0x0,
		.total_len			= image_size,
		.res1				= 0x1,
		.checksum			= 0x0,
		.res2				= 0x0,
	};
	int i, sum = 0;

	memcpy(wordbuf + 0x8, &flash_header, sizeof(flash_header));

	for (i = 0x8; i < 0x12; i++)
		sum += htole32(wordbuf[i]);

	sum = ~sum;
	wordbuf[i] = sum;
}

int main(int argc, char *argv[])
{
	FILE *ifile, *ofile;
	char *buf;
	const char *infile = NULL, *outfile = NULL, *cfgfile = NULL;
	struct stat st;
	int opt;

	prgname = argv[0];

	while ((opt = getopt(argc, argv, "c:f:ho:")) != -1) {
		switch (opt) {
		case 'c':
			cfgfile = optarg;
			break;
		case 'f':
			infile = optarg;
			break;
		case 'o':
			outfile = optarg;
			break;
		case 'h':
			usage(argv[0]);
		default:
			exit(1);
		}
	}

	if (!infile) {
		fprintf(stderr, "input file not given\n");
		exit(1);
	}

	if (!outfile) {
		fprintf(stderr, "output file not given\n");
		exit(1);
	}

	if (!cfgfile) {
		fprintf(stderr, "config file not given\n");
		exit(1);
	}

	if (stat(infile, &st) == -1) {
		perror("stat");
		exit(EXIT_FAILURE);
	}

	if (st.st_size > 192 * 1024) {
		fprintf(stderr, "Image too big, will not fit in OCRAM!\n");
		exit(EXIT_FAILURE);
	}

	buf = calloc(st.st_size + IMAGE_OFFSET, sizeof(char));
	if (!buf) {
		fprintf(stderr, "Unable to allocate buffer\n");
		return -1;
	}
	ifile = fopen(infile, "rb");
	if (!ifile) {
		fprintf(stderr, "Cannot open %s for reading\n",
			infile);
		free(buf);
		exit(EXIT_FAILURE);
	}
	ofile = fopen(outfile, "wb");
	if (!ofile) {
		fprintf(stderr, "Cannot open %s for writing\n",
			outfile);
		fclose(ifile);
		free(buf);
		exit(EXIT_FAILURE);
	}

	fread(buf + IMAGE_OFFSET, sizeof(char), st.st_size, ifile);

	add_header(buf, st.st_size);

	if (cfgfile)
		parse_config(buf, cfgfile);

	fwrite(buf, sizeof(char), st.st_size + IMAGE_OFFSET, ofile);

	fclose(ofile);
	fclose(ifile);
	free(buf);

	return 0;
}
