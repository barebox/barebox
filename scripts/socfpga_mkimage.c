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

#include "common.h"
#include "common.c"
#include "../crypto/crc32.c"

#define VALIDATION_WORD 0x31305341

#define BRANCH_INST 0xea /* ARM opcode for "b" (unconditional branch) */

#define MAX_V0IMAGE_SIZE (60 * 1024 - 4)
/* Max size without authentication is 224 KB, due to memory used by
 * the ROM boot code as a workspace out of the 256 KB of OCRAM */
#define MAX_V1IMAGE_SIZE (224 * 1024 - 4)

static int add_barebox_header;

struct socfpga_header {
	uint8_t validation_word[4];
	uint8_t version;
	uint8_t flags;
	union {
		struct {
			uint8_t program_length[2];
			uint8_t spare[2];
			uint8_t checksum[2];
			uint8_t start_vector[4];
		} v0;
		struct {
			uint8_t header_length[2];
			uint8_t program_length[4];
			uint8_t entry_offset[4];
			uint8_t spare[2];
			uint8_t checksum[2];
		} v1;
	};
};

static uint32_t bb_header[] = {
	0xea00007e,	/* b 0x200  */
	0xeafffffe,	/* 1: b 1b  */
	0xeafffffe,	/* 1: b 1b  */
	0xeafffffe,	/* 1: b 1b  */
	0xeafffffe,	/* 1: b 1b  */
	0xeafffffe,	/* 1: b 1b  */
	0xeafffffe,	/* 1: b 1b  */
	0xeafffffe,	/* 1: b 1b  */
	0x65726162,	/* 'bare'   */
	0x00786f62,	/* 'box\0'  */
	0x00000000,	/* padding  */
	0x00000000,	/* padding  */
	0x00000000,	/* padding  */
	0x00000000,	/* padding  */
	0x00000000,	/* padding  */
	0x00000000,	/* padding  */
	0x00000000,	/* socfpga header */
	0x00000000,	/* socfpga header */
	0x00000000,	/* socfpga header */
	0xea00006b,	/* entry. b 0x200 (offset may be adjusted) */
};

/* Create an ARM relative branch instuction
 * branch is where the instruction will be placed and dest points to where
 * it should branch too. */
static void branch(uint8_t *branch, uint8_t *dest)
{
	int offset = dest - branch - 8; /* PC is offset +8 bytes on ARM */

	branch[0] = (offset >> 2) & 0xff; /* instruction uses offset/4 */
	branch[1] = (offset >> 10) & 0xff;
	branch[2] = (offset >> 18) & 0xff;
	branch[3] = BRANCH_INST;
}

/* start_addr is where the socfpga header's start instruction should branch to.
 * It should be relative to the start of buf */
static int add_socfpga_header(void *buf, size_t size, unsigned start_addr, unsigned version)
{
	struct socfpga_header *header = buf + 0x40;
	void *entry;
	uint8_t *bufp, *sumendp;
	uint32_t *crc;
	unsigned checksum;

	if (size & 0x3) {
		fprintf(stderr, "%s: size must be multiple of 4\n", __func__);
		return -EINVAL;
	}

	/* Absolute address of entry point in buf */
	entry = buf + start_addr;
	if (version == 0) {
		sumendp = &header->v0.checksum[0];
	} else {
		sumendp = &header->v1.checksum[0];

		/* The ROM loader can't handle a negative offset */
		if (entry < (void*)header) {
			/* add a trampoline branch inst after end of the header */
			uint8_t *trampoline = (void*)(header + 1);
			branch(trampoline, entry);

			/* and then make the trampoline the entry point */
			entry = trampoline;
		}
		/* Calculate start address as offset relative to start of header */
		start_addr = entry - (void*)header;
	}

	header->validation_word[0] = VALIDATION_WORD & 0xff;
	header->validation_word[1] = (VALIDATION_WORD >> 8) & 0xff;
	header->validation_word[2] = (VALIDATION_WORD >> 16) & 0xff;
	header->validation_word[3] = (VALIDATION_WORD >> 24) & 0xff;
	header->version = version;
	header->flags = 0;

	if (version == 0) {
		header->v0.program_length[0] = (size >>  2) & 0xff; /* length in words */
		header->v0.program_length[1] = (size >> 10) & 0xff;
		header->v0.spare[0] = 0;
		header->v0.spare[1] = 0;
		branch(header->v0.start_vector, entry);
	} else {
		header->v1.header_length[0] = (sizeof(*header) >> 0) & 0xff;
		header->v1.header_length[1] = (sizeof(*header) >> 8) & 0xff;
		header->v1.program_length[0] = (size >>  0) & 0xff;
		header->v1.program_length[1] = (size >>  8) & 0xff;
		header->v1.program_length[2] = (size >> 16) & 0xff;
		header->v1.program_length[3] = (size >> 24) & 0xff;
		header->v1.entry_offset[0] = (start_addr >>  0) & 0xff;
		header->v1.entry_offset[1] = (start_addr >>  8) & 0xff;
		header->v1.entry_offset[2] = (start_addr >> 16) & 0xff;
		header->v1.entry_offset[3] = (start_addr >> 24) & 0xff;
		header->v1.spare[0] = 0;
		header->v1.spare[1] = 0;
	}

	/* Sum from beginning of header to start of checksum field */
	checksum = 0;
	for (bufp = (uint8_t*)header; bufp < sumendp; bufp++)
		checksum += *bufp;

	if (version == 0) {
		header->v0.checksum[0] = checksum & 0xff;;
		header->v0.checksum[1] = (checksum >> 8) & 0xff;;
	} else {
		header->v1.checksum[0] = checksum & 0xff;;
		header->v1.checksum[1] = (checksum >> 8) & 0xff;;
	}

	crc = buf + size - sizeof(uint32_t);

	*crc = crc32_be(0xffffffff, buf, size - sizeof(uint32_t));
	*crc ^= 0xffffffff;

	return 0;
}

static void usage(const char *prgname)
{
	fprintf(stderr, "usage: %s [-hbs] [-v version] <infile> -o <outfile>\n", prgname);
}

int main(int argc, char *argv[])
{
	int opt, ret;
	const char *outfile = NULL, *infile;
	struct stat s;
	void *buf;
	int fd;
	int max_image_size, min_image_size = 80;
	int addsize = 0, pad;
	int fixup_size = 0;
	unsigned int version = 0;
	int fixed_size = 0;

	while ((opt = getopt(argc, argv, "o:hbsv:")) != -1) {
		switch (opt) {
		case 'v':
			version = atoi(optarg);
			if (version > 1) {
				printf("Versions supported: 0 or 1\n");
				usage(argv[0]);
				exit(1);
			}
			break;
		case 'b':
			add_barebox_header = 1;
			min_image_size = 0;
			addsize = 512;
			break;
		case 's':
			fixup_size = 1;
			break;
		case 'h':
			usage(argv[0]);
			exit(0);
		case 'o':
			outfile = optarg;
			break;
		default:
			usage(argv[0]);
			exit(1);
		}
	}
	if (version == 0) {
		max_image_size = MAX_V0IMAGE_SIZE;
	} else {
		max_image_size = MAX_V1IMAGE_SIZE;
	}
	max_image_size -= addsize;

	if (optind == argc || !outfile) {
		usage(argv[0]);
		exit(1);
	}
	infile = argv[optind];

	ret = stat(infile, &s);
	if (ret) {
		perror("stat");
		exit(1);
	}

	if (s.st_size < min_image_size) {
		fprintf(stderr, "input image too small. Minimum is %d bytes\n",
			min_image_size);
		exit(1);
	}

	if (s.st_size > max_image_size) {
		fprintf(stderr, "input image too big. Maximum is %d bytes, got %ld bytes\n",
				max_image_size, s.st_size);
		exit(1);
	}

	fd = open(infile, O_RDONLY);
	if (fd == -1) {
		perror("open infile");
		exit(1);
	}

	pad = s.st_size & 0x3;
	if (pad)
		pad = 4 - pad;

	buf = calloc(s.st_size + 4 + addsize + pad, 1);
	if (!buf) {
		perror("malloc");
		exit(1);
	}

	ret = read_full(fd, buf + addsize, s.st_size);
	if (ret < 0) {
		perror("read infile");
		exit(1);
	}

	fixed_size = s.st_size;

	close(fd);

	if (add_barebox_header) {
		int barebox_size = 0;
		int *image_size = buf + 0x2c;

		memcpy(buf, bb_header, sizeof(bb_header));

		if (fixup_size) {
			fixed_size = htole32(fixed_size);

			barebox_size = *((uint32_t *)buf + (fixed_size + addsize + pad) / 4 - 1);

			/* size of barebox+pbl, header, size */
			fixed_size += (barebox_size + addsize + 4);

			*image_size = fixed_size;
		}
	}

	ret = add_socfpga_header(buf, s.st_size + 4 + addsize + pad, addsize,
	                         version);
	if (ret)
		exit(1);

	ret = write_file(outfile, buf, s.st_size + 4 + addsize + pad);
	if (ret)
		exit(1);

	exit(0);
}
