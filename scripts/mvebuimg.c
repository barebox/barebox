// SPDX-License-Identifier: GPL-2.0
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define ALIGN_SUP(x, a) (((x) + ((a) - 1)) & ~((a) - 1))
#define min(a, b) ((a) < (b) ? (a) : (b))
#define is_power_of_two(a) (((a) & ((a) - 1)) == 0)
#define container_of(ptr, type, member) \
    (type *)((char *)(ptr) - (char *) &((type *)0)->member)


#define pr_err(fmt, arg...) fprintf(stderr, "ERROR: " fmt, ##arg)
#define pr_info(fmt, arg...) fprintf(stderr, "INFO: " fmt, ##arg)

static const struct {
	unsigned char id;
	char *name;
} boot_ids[] = {
	{ 0x4d, "i2c" },
	{ 0x5a, "spi" },
	{ 0x69, "uart" },
	{ 0x78, "sata" },
	{ 0x8b, "nand" },
	{ 0x9c, "pex" },
	{ /* sentinel */ }
};

struct mvebuimg_checksum_method {
	void (*update)(struct mvebuimg_checksum_method *self,
		       const void *buf, size_t len);
};

struct mvebuimg_checksum_method8 {
	struct mvebuimg_checksum_method method;
	uint8_t csum;
};

static void mvebuimg_checksum8(struct mvebuimg_checksum_method *self,
			       const void *buf, size_t len)
{
	struct mvebuimg_checksum_method8 *realself =
		container_of(self, struct mvebuimg_checksum_method8, method);

	while (len--) {
		realself->csum += *(uint8_t *)buf;
		buf++;
	}
}

struct mvebuimg_checksum_method32 {
	struct mvebuimg_checksum_method method;
	uint32_t csum;
	int mod;
};

static void mvebuimg_checksum32(struct mvebuimg_checksum_method *self,
				const void *buf, size_t len)
{
	struct mvebuimg_checksum_method32 *realself =
		container_of(self, struct mvebuimg_checksum_method32, method);

	while (len--) {
		realself->csum += *(uint8_t *)buf << (8 * realself->mod);
		realself->mod = (realself->mod + 1) % 4;
		buf++;
	}
}

static ssize_t mvebuimg_sendfile(int outfd, int infd,
				 struct mvebuimg_checksum_method *csumm,
				 size_t count)
{
	char buf[1024];
	size_t head = 0, tail = 0;
	int ret;
	size_t count_read = 0, count_written = 0;

	while (count_written < count) {
		if (head == tail) {
			head = 0;
			tail = 0;
		}

		if (tail < sizeof(buf) && count_read < count) {
			ret = read(infd, buf + tail,
				   min(count, sizeof(buf) - tail));
			if (ret < 0) {
				pr_err("read error\n");
				return ret;
			}

			if (ret == 0) {
				pr_err("hit EOF\n");
				return -EINVAL;
			}

			if (csumm)
				csumm->update(csumm, buf + tail, ret);

			tail += ret;
			count_read += ret;
		}

		if (head < tail) {
			ret = write(outfd, buf + head, tail - head);
			if (ret < 0)
				return ret;

			head += ret;
			count_written += ret;
		}
	}

	return 0;
}

static int mvebuimg_v1_write_binhdr(int fd, size_t *offset,
				    const char *filename,
				    struct mvebuimg_checksum_method8 *summ,
				    int is_last_header)
{
	int binhdr_fd;
	char binhdr[16] = { 0, };
	struct stat s;
	uint32_t hdrsize;
	int ret;
	off_t ret_lseek;
	char trailer[4] = { 0, };

	binhdr[0] = 0x2;

	binhdr_fd = open(filename, O_RDONLY);
	if (binhdr_fd < 0) {
		pr_err("Failed to open binhdr source \"%s\" (%s)\n",
		       filename, strerror(errno));
		return -errno;
	}

	ret = fstat(binhdr_fd, &s);
	if (ret < 0) {
		pr_err("Failed to stat binhdr source \"%s\" (%s)\n",
		       filename, strerror(errno));
		return -errno;
	}

	if (ALIGN_SUP(s.st_size, 4) > 0xffffff - 0x14) {
		pr_err("binhdr \"%s\" too big\n", filename);
		return -EINVAL;
	}

	hdrsize = /* len of hdr's header */ 0x10 +
		/* actual image */ ALIGN_SUP(s.st_size, 4) +
		/* trailer word */ 4;

	binhdr[1] = hdrsize >> 16;
	binhdr[2] = hdrsize;
	binhdr[3] = hdrsize >> 8;

	/* parameters that are used by all images I know of */
	binhdr[4] = 2;
	binhdr[8] = 0x5b;
	binhdr[12] = 0x68;

	summ->method.update(&summ->method, binhdr, sizeof(binhdr));
	ret = pwrite(fd, binhdr, sizeof(binhdr), *offset);
	if (ret < 0) {
		pr_err("failed to write header of binhdr \"%s\"\n",
			filename);
		return -errno;
	}

	*offset += sizeof(binhdr);

	ret_lseek = lseek(fd, *offset, SEEK_SET);
	if (ret_lseek == (off_t)-1) {
		pr_err("failed to seek to binhdr offset\n");
		return -errno;
	}

	ret = mvebuimg_sendfile(fd, binhdr_fd, &summ->method, s.st_size);
	if (ret < 0)
		return ret;

	*offset += ALIGN_SUP(s.st_size, 4);
	trailer[0] = is_last_header ? 0 : 1;

	summ->method.update(&summ->method, trailer, sizeof(trailer));
	ret = pwrite(fd, trailer, sizeof(trailer), *offset);
	if (ret < 0) {
		pr_err("failed to write trailer of binhdr \"%s\"\n",
		       filename);
		return -errno;
	}

	*offset += 4;

	return 0;
}

static int mvebuimg_v1_write_payload(int fd, size_t *offset,
				     const char *filename)
{
	int payload_fd;
	struct stat s;
	int ret;
	char pad[4] = { 0, };
	struct mvebuimg_checksum_method32 csumm32 = {
		.method = {
			.update = mvebuimg_checksum32,
		},
		.csum = 0,
	};

	payload_fd = open(filename, O_RDONLY);
	if (payload_fd < 0) {
		pr_err("Failed to open payload \"%s\" (%s)\n",
			filename, strerror(errno));
		return -errno;
	}

	ret = fstat(payload_fd, &s);
	if (ret < 0) {
		pr_err("Failed to stat payload \"%s\" (%s)\n",
			filename, strerror(errno));
		return -errno;
	}

	lseek(fd, *offset, SEEK_SET);

	ret = mvebuimg_sendfile(fd, payload_fd, &csumm32.method, s.st_size);
	if (ret < 0)
		return ret;

	/* pad to a multiple of 4 bytes */
	ret = pwrite(fd, pad, ALIGN_SUP(s.st_size, 4) - s.st_size,
		     *offset + s.st_size);
	if (ret < 0) {
		pr_err("failed to write padding of payload\n");
		return -errno;
	}
	*offset += ALIGN_SUP(s.st_size, 4);

	/* For version 1 images the checksum doesn't seem to be necessary. We're
	 * still adding it for now to create bytewise identical images as
	 * kwbimage.
	 */
	pad[0] = csumm32.csum;
	pad[1] = csumm32.csum >> 8;
	pad[2] = csumm32.csum >> 16;
	pad[3] = csumm32.csum >> 24;

	ret = pwrite(fd, pad, 4, *offset);
	*offset += 4;

	return 0;
}

/*
 * Usage: create [-b binhdr]* [-o outputfilename] bootsrc payload
 */
static int mvebuimg_v1_create(int argc, char *const argv[])
{
	int opt;
	char *output = NULL;
	unsigned int extensionheaders = 0;
	int fd, ret;
	long l;
	char *endptr;
	char mainhdr[0x20] = { 0, };
	struct mvebuimg_checksum_method8 csumm = {
		.method = {
			.update = mvebuimg_checksum8,
		},
		.csum = 0,
	};
	size_t offset, offset_payload;
	size_t size_payload;
	unsigned long payload_align = 4096;
	int debug = 0;
	long destaddr, execaddr;
	int destaddr_provided = 0, execaddr_provided = 0;
	uint8_t nand_blksz = 0;
	uint8_t nand_bbloc = 0;

	optind = 0;
	while ((opt = getopt(argc, argv, "+a:b:B:Dd:e:o:")) != -1) {
		switch (opt) {
		case 'a':
			errno = 0;
			payload_align = strtol(optarg, &endptr, 0);
			if (errno || *endptr != '\0' ||
			    !is_power_of_two(payload_align)) {
				pr_err("Invalid payload alignment, must be a power of two\n");
				return EXIT_FAILURE;
			}
			break;

		case 'b':
			++extensionheaders;
			break;

		case 'B':
			errno = 0;
			l = strtol(optarg, &endptr, 0);
			if (errno || (*endptr != '\0' && *endptr != ':') ||
			    (l != 0 && (l < 0x10000 || l > 0xff0000 ||
					!is_power_of_two(l)))) {
				pr_err("Invalid NAND block size\n");
				return EXIT_FAILURE;
			}
			nand_blksz = l >> 16;

			if (*endptr != ':')
				break;

			if ((*(endptr + 1) == '0' || *(endptr + 1) == '1') &&
			    *(endptr + 2) == '\0') {
				nand_bbloc = *(endptr + 1) - '0';
			} else {
				pr_err("Invalid NAND bad block location\n");
				return EXIT_FAILURE;
			}

			break;

		case 'D':
			debug = 1;
			break;

		case 'd':
			errno = 0;
			destaddr = strtol(optarg, &endptr, 0);
			if (errno || *endptr != '\0' ||
			    destaddr < 0 || destaddr > 0xffffffff) {
				pr_err("invalid dest address\n");
				return EXIT_FAILURE;
			}
			destaddr_provided = 1;
			break;

		case 'e':
			errno = 0;
			execaddr = strtol(optarg, &endptr, 0);
			if (errno || *endptr != '\0' ||
			    execaddr < 0 || execaddr > 0xffffffff) {
				pr_err("invalid exec address\n");
				return EXIT_FAILURE;
			}
			execaddr_provided = 1;
			break;

		case 'o':
			if (output) {
				pr_err("more than one output file specified\n");
				return EXIT_FAILURE;
			}
			output = optarg;
			break;

		default:
			pr_err("Unsupported option\n");
			return EXIT_FAILURE;
		}
	}

	if (argc - optind < 2) {
		pr_err("too few parameters\n");
		return EXIT_FAILURE;
	}

	errno = 0;
	l = strtol(argv[optind], &endptr, 0);
	if (errno || *endptr != '\0' || l < 0 || l > 0xff) {
		int i;
		int idfound = 0;

		for (i = 0; boot_ids[i].name; ++i) {
			if (strcmp(boot_ids[i].name, argv[optind]) == 0) {
				mainhdr[0] = boot_ids[i].id;
				idfound = 1;
				break;
			}
		}

		if (!idfound) {
			fprintf(stderr, "failed to parse bootsrc\n");
			return EXIT_FAILURE;
		}

	} else {
		mainhdr[0] = l;
	}

	if (!destaddr_provided)
		destaddr = 0x1000000;

	if (!execaddr_provided)
		execaddr = destaddr;

	if (!output) {
		pr_err("no output option given\n");
		return EXIT_FAILURE;
	}

	fd = open(output, O_CREAT | O_WRONLY | O_TRUNC, 0644);
	if (fd < 0) {
		pr_err("failed to open output\n");
		return EXIT_FAILURE;
	}

	if (extensionheaders)
		mainhdr[0x1e] = 1;

	offset = 0x20;
	optind = 0;
	while ((opt = getopt(argc, argv, "+a:b:B:Dd:e:o:")) != -1) {
		switch (opt) {
		case 'b':
			--extensionheaders;

			ret = mvebuimg_v1_write_binhdr(fd, &offset, optarg,
						       &csumm,
						       extensionheaders == 0);
			if (ret < 0)
				return EXIT_FAILURE;

			break;

		default:
			break;
		}
	}

	/* Align the payload to a reasonable boundary */
	offset = offset_payload = ALIGN_SUP(offset, 4096);

	/* BootROM debugging */
	mainhdr[0x01] = debug ? 0x1 : 0x0;

	/* Boot Image Format Version */
	mainhdr[0x08] = 0x1;

	/* Header size */
	mainhdr[0x09] = offset_payload >> 16;
	mainhdr[0x0a] = offset_payload;
	mainhdr[0x0b] = offset_payload >> 8;

	/* Offset of payload in the image */
	mainhdr[0x0c] = (char)offset_payload;
	mainhdr[0x0d] = (char)(offset_payload >> 8);
	mainhdr[0x0e] = (char)(offset_payload >> 16);
	mainhdr[0x0f] = (char)(offset_payload >> 24);

	/* Address to load the image to */
	mainhdr[0x10] = (char)destaddr;
	mainhdr[0x11] = (char)(destaddr >> 8);
	mainhdr[0x12] = (char)(destaddr >> 16);
	mainhdr[0x13] = (char)(destaddr >> 24);

	/* Entry point */
	mainhdr[0x14] = (char)execaddr;
	mainhdr[0x15] = (char)(execaddr >> 8);
	mainhdr[0x16] = (char)(execaddr >> 16);
	mainhdr[0x17] = (char)(execaddr >> 24);

	/* For NAND images set block size and bad block location */
	mainhdr[0x19] = nand_blksz;
	mainhdr[0x1a] = nand_bbloc;

	ret = mvebuimg_v1_write_payload(fd, &offset, argv[optind + 1]);
	if (ret < 0)
		return EXIT_FAILURE;

	size_payload = offset - offset_payload;
	mainhdr[0x04] = (char)size_payload;
	mainhdr[0x05] = (char)(size_payload >> 8);
	mainhdr[0x06] = (char)(size_payload >> 16);
	mainhdr[0x07] = (char)(size_payload >> 24);

	csumm.method.update(&csumm.method, mainhdr, sizeof(mainhdr));
	mainhdr[0x1f] = csumm.csum;
	ret = pwrite(fd, mainhdr, sizeof(mainhdr), 0);
	if (ret < 0) {
		pr_err("failure write main header (%s)\n",
			strerror(errno));
		return EXIT_FAILURE;
	}

	ret = close(fd);
	if (ret) {
		pr_err("failure during output closing (%s)\n",
			strerror(errno));
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

static const struct {
	const char *name;
	int (*func[2])(int argc, char *const argv[]);
} commands[] = {
	{
		.name = "create",
		.func = { NULL, mvebuimg_v1_create }
	}, {
		/* sentinel */
	}
};

int main(int argc, char *const argv[])
{
	int opt;
	long version = -1;
	int i;

	char *endptr;

	optind = 0;
	while ((opt = getopt(argc, argv, "+v:")) != -1) {
		switch (opt) {
		case 'v':
			errno = 0;
			version = strtol(optarg, &endptr, 0);
			if (errno || *endptr != '\0' ||
			    version < 0 || version > 1) {
				pr_err("Unsupported version\n");
				return EXIT_FAILURE;
			}
			break;
		default:
			pr_err("Unsupported option\n");
			return EXIT_FAILURE;
		}
	}

	if (version < 0) {
		pr_info("Defaulting to version 1\n");
		version = 1;
	}

	argc -= optind;
	argv += optind;

	if (argc < 1) {
		pr_err("No command given\n");
		return EXIT_FAILURE;
	}

	for (i = 0; commands[i].name != NULL; ++i) {
		if (strcmp(commands[i].name, argv[0]) == 0) {
			if (!commands[i].func[version]) {
				pr_err("ERROR: command not implemented\n");
				return EXIT_FAILURE;
			}
			return commands[i].func[version](argc, argv);
		}
	}

	pr_err("command \"%s\" not found\n", argv[0]);
	return EXIT_FAILURE;
}
