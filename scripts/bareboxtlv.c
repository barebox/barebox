// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: 2021 Ahmad Fatoum, Pengutronix

/* bareboxtlv.c - generate a barebox TLV header */

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>

#include "compiler.h"
#include <linux/stringify.h>

#define debug(...)
#define printk_once(...)

#include "../include/tlv/format.h"
#include "../crypto/crc32.c"

#include "common.h"
#include "common.c"

static void usage(char *prgname)
{
	fprintf(stderr,
		"USAGE: %s [FILE]\n"
		"Manipulate barebox TLV blobs\n"
		"\n"
		"options:\n"
		"  -m        TLV magic to use (default: " __stringify(TLV_MAGIC_BAREBOX_V1) " or input magic for -s)\n"
		"  -s        strip input TLV header from output\n"
		"            (default is adding header to headerless input)\n"
		"  -@        offset to start reading at\n"
		"  -v        verify CRC\n",
		prgname);
}

static inline ssize_t write_full_verbose(int fd, const void *buf, size_t len)
{
	ssize_t ret;

	ret = write_full(fd, buf, len);
	if (ret < 0)
		perror("write");
	return ret;
}

int main(int argc, char *argv[])
{
	u32 magic = 0;
	struct tlv_header *inhdr = NULL;
	struct tlv_header hdr;
	size_t offset = 0, size = 0;
	ssize_t ret;
	bool strip_header = false, verify = false;
	int opt, ecode = EXIT_FAILURE;
	int infd = STDIN_FILENO;
	const u8 *p;
	char *endptr;
	void *buf;
	u32 crc = ~0;

	while((opt = getopt(argc, argv, "svm:@:h")) != -1) {
		switch (opt) {
		case 'v':
			verify = true;
			/* fallthrough; */
		case 's':
			strip_header = true;
			break;
		case 'm':
			magic = strtoul(optarg, NULL, 16);
			if (!magic) {
				fprintf(stderr, "invalid magic: %s\n", optarg);
				return EXIT_FAILURE;
			}
			break;
		case '@':
			offset = strtoul(optarg, &endptr, 0);
			if (endptr == optarg) {
				fprintf(stderr, "invalid offset: %s\n", optarg);
				return EXIT_FAILURE;
			}
			break;
		case 'h':
			usage(argv[0]);
			return 0;
		}
	}

	if (argc - optind > 1) {
		usage(argv[0]);
		return EXIT_FAILURE;
	}

	if (argv[optind]) {
		infd = open(argv[optind], O_RDONLY);
		if (infd < 0) {
			perror("open");
			return EXIT_FAILURE;
		}
		lseek(infd, offset, SEEK_SET);
	} else if (offset) {
		fprintf(stderr, "can't combine -@ with stdin\n");
		return EXIT_FAILURE;
	}

	p = buf = read_fd(infd, &size);
	if (!buf) {
		perror("read_fd");
		goto out;
	}

	/* validate, but skip, if header given */
	if (strip_header) {
		inhdr = buf;

		if (size < tlv_total_len(inhdr)) {
			fprintf(stderr, "short input: got %zu bytes, but header claims %zu\n",
			       size, tlv_total_len(inhdr));
			goto out;
		}

		if (!magic)
			magic = get_unaligned_be32(p);

		p += sizeof(struct tlv_header);
		size = cpu_to_be32(inhdr->length_tlv);

		if (cpu_to_be32(inhdr->length_sig) != 0) {
			fprintf(stderr, "Signatures not yet supported\n");
			goto out;
		}

	}

	if (!magic)
		magic = TLV_MAGIC_BAREBOX_V1;

	hdr.magic = cpu_to_be32(magic);
	hdr.length_tlv = cpu_to_be32(size);
	hdr.length_sig = cpu_to_be32(0);

	if (!verify) {
		if (!strip_header) {
			ret = write_full_verbose(1, &hdr, sizeof(hdr));
			if (ret < 0)
				goto out;
		}
		ret = write_full_verbose(1, p, size);
		if (ret < 0)
			goto out;
	}

	crc = crc32_be(crc, &hdr, sizeof(hdr));
	crc = crc32_be(crc, p, size);

	if (verify) {
		u32 oldcrc;

		oldcrc = get_unaligned_be32(p + size);
		if (oldcrc != crc) {
			fprintf(stderr, "CRC mismatch: expected %08x, but calculated %08x\n",
			       oldcrc, crc);
			goto out;
		}
	}

	if (!strip_header) {
		crc = cpu_to_be32(crc);

		ret = write_full_verbose(1, &crc, sizeof(crc));
		if (ret < 0)
			goto out;
	}

	ecode = 0;
out:
	free(buf);
	if (argv[optind])
		close(infd);
	return ecode;
}
