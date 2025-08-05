// SPDX-License-Identifier: GPL-2.0
#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <openssl/evp.h>
#include <errno.h>
#include <stdbool.h>

#include "common.h"
#include "common.c"
#include "rockchip.h"

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))
#define ALIGN(x, a)        (((x) + (a) - 1) & ~((a) - 1))

static void sha256(const void *buf, int len, void *out)
{
	EVP_MD_CTX *md_ctx;

	md_ctx = EVP_MD_CTX_new();
	EVP_DigestInit(md_ctx, EVP_sha256());
	EVP_DigestUpdate(md_ctx, buf, len);
	EVP_DigestFinal(md_ctx, out, NULL);
	EVP_MD_CTX_free(md_ctx);
}

static void sha512(const void *buf, int len, void *out)
{
	EVP_MD_CTX *md_ctx;

	md_ctx = EVP_MD_CTX_new();
	EVP_DigestInit(md_ctx, EVP_sha512());
	EVP_DigestUpdate(md_ctx, buf, len);
	EVP_DigestFinal(md_ctx, out, NULL);
	EVP_MD_CTX_free(md_ctx);
}

typedef enum {
	HASH_TYPE_SHA256 = 1,
	HASH_TYPE_SHA512 = 2,
} hash_type_t;

struct rkcode {
	const char *path;
	size_t size;
	void *buf;
};

static int n_code;
struct rkcode *code;

static int create_newidb(struct newidb *idb)
{
	hash_type_t hash_type = HASH_TYPE_SHA256;
	bool keep_cert = false;
	int image_offset;
	int i;
	unsigned char *idbu8 = (void *)idb;

	idb->magic = NEWIDB_MAGIC;
	idb->n_files = (n_code << 16) | (1 << 7) | (1 << 8);
	idb->flags = 0;

	if (hash_type == HASH_TYPE_SHA256)
		idb->flags |= NEWIDB_FLAGS_SHA256;
	else if (hash_type == HASH_TYPE_SHA512)
		idb->flags |= NEWIDB_FLAGS_SHA512;

	if (!keep_cert)
		image_offset = 4;
	else
		image_offset = 8;

	for (i = 0; i < n_code; i++) {
		struct newidb_entry *entry = &idb->entries[i];
		struct rkcode *c = &code[i];
		unsigned int image_sector;

		image_sector = c->size / RK_SECTOR_SIZE;

		entry->sector  = (image_sector << 16) + image_offset;
		entry->unknown_ffffffff = 0xffffffff;
		entry->image_number = i + 1;

		if (hash_type == HASH_TYPE_SHA256)
			sha256(c->buf, c->size, entry->hash); /* File size padded to sector size */
		else if (hash_type == HASH_TYPE_SHA512)
			sha512(c->buf, c->size, entry->hash);

		image_offset += image_sector;

	}

	if (hash_type == HASH_TYPE_SHA256)
		sha256(idbu8, 1536, idbu8 + 1536);
	else if (hash_type == HASH_TYPE_SHA512)
		sha512(idbu8, 1536, idbu8 + 1536);

	return 0;
}

struct option cbootcmd[] = {
	{"help", 0, NULL, 'h'},
	{"o", 1, NULL, 'o'},
	{0, 0, 0, 0},
};

static void usage(const char *prgname)
{
	printf(
"Usage: %s [OPTIONS] <FILES>\n"
"\n"
"Generate a Rockchip boot image from <FILES>\n"
"This tool is suitable for SoCs beginning with rk3568. Older SoCs\n"
"have a different image format.\n"
"\n"
"Options:\n"
"  -o <file>   Output image to <file>\n"
"  -h          This help\n",
	prgname);
}

int main(int argc, char *argv[])
{
	int opt, i, fd;
	const char *outfile = NULL;
	struct newidb idb = {};

	while ((opt = getopt_long(argc, argv, "ho:", cbootcmd, NULL)) > 0) {
		switch (opt) {
		case 'o':
			outfile = optarg;
			break;
		case 'h':
			usage(argv[0]);
			exit(0);
		}
	}

	n_code = argc - optind;
	if (!n_code) {
		usage(argv[0]);
		exit(1);
	}

	code = calloc(n_code, sizeof(*code));
	if (!code)
		exit(1);

	for (i = 0; i < n_code; i++) {
		struct stat s;
		int fd, ret;
		struct rkcode *c = &code[i];
		const char *path = argv[i + optind];

		fd = open(path, O_RDONLY);
		if (fd < 0) {
			fprintf(stderr, "Cannot open %s: %s\n", path, strerror(errno));
			exit(1);
		}

		ret = fstat(fd, &s);
		if (ret)
			exit(1);

		c->path = path;
		c->size = ALIGN(s.st_size, RK_PAGE_SIZE);
		c->buf = calloc(c->size, 1);
		if (!c->buf)
			exit(1);

		read_full(fd, c->buf, s.st_size);
		close(fd);
	}

	create_newidb(&idb);

	fd = creat(outfile, 0644);
	if (fd < 0) {
		fprintf(stderr, "Cannot open %s: %s\n", outfile, strerror(errno));
		exit(1);
	}

	write_full(fd, &idb, sizeof(idb));

	for (i = 0; i < n_code; i++) {
		struct rkcode *c = &code[i];

		write_full(fd, c->buf, c->size);
	}

	close(fd);

	return 0;
}
