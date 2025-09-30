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
#include <errno.h>
#include <stdbool.h>

#include <openssl/bn.h>
/*
 * TODO Switch from the OpenSSL ENGINE API to the PKCS#11 provider and the
 * PROVIDER API: https://github.com/latchset/pkcs11-provider
 */
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <openssl/engine.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>

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

static void idb_hash(struct newidb *idb)
{
	unsigned char *idbu8 = (void *)idb;
	size_t size = 1536;

	if (idb->flags & NEWIDB_FLAGS_SHA256)
		sha256(idbu8, size, idbu8 + size);
	else if (idb->flags & NEWIDB_FLAGS_SHA512)
		sha512(idbu8, size, idbu8 + size);
}

static __attribute__((unused)) EVP_PKEY *load_key_pkcs11(const char *path)
{
	const char *engine_id = "pkcs11";
	ENGINE *e;
	EVP_PKEY *pkey = NULL;

	ENGINE_load_builtin_engines();

	e = ENGINE_by_id(engine_id);
	if (!e) {
		fprintf(stderr, "Engine %s isn't available\n", engine_id);
		goto err_engine_by_id;
	}
	if (!ENGINE_init(e)) {
		fprintf(stderr, "Couldn't initialize engine\n");
		goto err_engine_init;
	}

	pkey = ENGINE_load_private_key(e, path, NULL, NULL);

	ENGINE_finish(e);
err_engine_init:
	ENGINE_free(e);
err_engine_by_id:
#if OPENSSL_VERSION_NUMBER < 0x10100000L || \
	(defined(LIBRESSL_VERSION_NUMBER) && LIBRESSL_VERSION_NUMBER < 0x02070000fL)
	ENGINE_cleanup();
#endif
	return pkey;
}

static __attribute__((unused)) EVP_PKEY *load_key_file(const char *path)
{
	BIO *key;
	EVP_PKEY *pkey = NULL;

	key = BIO_new_file(path, "r");
	if (!key)
		return NULL;

	pkey = PEM_read_bio_PrivateKey(key, NULL, NULL, NULL);

	BIO_free_all(key);

	return pkey;
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

static bool has_magic(void *buf)
{
	uint32_t magic;

	magic = *(uint32_t *)buf;

	return magic == NEWIDB_MAGIC_RKSS || magic == NEWIDB_MAGIC_RKNS;
}

static int create_newidb(struct newidb *idb)
{
	hash_type_t hash_type = HASH_TYPE_SHA256;
	bool keep_cert = false;
	int image_offset;
	int i;

	idb->magic = NEWIDB_MAGIC_RKNS;
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

	idb_hash(idb);

	return 0;
}

#if OPENSSL_VERSION_NUMBER >= 0x30000000L
#include <openssl/core_names.h>

static int rsa_get_params(EVP_PKEY *key, BIGNUM *e, BIGNUM *n, BIGNUM *np)
{
	BN_CTX *ctx = BN_CTX_new();
	int ret;

	ret = EVP_PKEY_get_bn_param(key, OSSL_PKEY_PARAM_RSA_E, &e);
	if (ret != 1)
		return ret;

	ret = EVP_PKEY_get_bn_param(key, OSSL_PKEY_PARAM_RSA_N, &n);
	if (ret != 1)
		return ret;

	/* Downstream U-Boot documents NP as an "acceleration factor" */
	BN_set_bit(np, EVP_PKEY_get_bits(key) + 132);
	BN_div(np, NULL, np, n, ctx);

	BN_CTX_free(ctx);

	return 0;
}

static int bin2lebin(const unsigned char *src, unsigned char *dst, size_t size)
{
	size_t i;

	for (i = 0; i < size; i++)
		dst[i] = src[size - 1 - i];

	return i;
}

static int idb_sign(struct newidb *idb, EVP_PKEY *key)
{
	EVP_PKEY_CTX *ctx = NULL;
	unsigned char *sig = NULL;
	const EVP_MD *md;
	size_t mdlen;
	size_t siglen;
	int ret = -1;

	if (!!(idb->flags & NEWIDB_FLAGS_SHA256)) {
		md = EVP_sha256();
	} else if (!!(idb->flags & NEWIDB_FLAGS_SHA512)) {
		md = EVP_sha512();
	} else {
		fprintf(stderr, "Unsupported hash function\n");
		return -EINVAL;
	}

	ctx = EVP_PKEY_CTX_new_from_pkey(NULL, key, NULL);
	if (!ctx)
		return -1;

	if (EVP_PKEY_sign_init(ctx) <= 0)
		goto out;
	if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_PSS_PADDING) <= 0)
		goto out;
	if (EVP_PKEY_CTX_set_signature_md(ctx, md) <= 0)
		goto out;
	/* rk_sign_tool verification fails if saltlen != 32 with sha256 */
	if (EVP_PKEY_CTX_set_rsa_pss_saltlen(ctx, RSA_PSS_SALTLEN_DIGEST) <= 0)
		goto out;

	mdlen = EVP_MD_get_size(md);
	if (EVP_PKEY_sign(ctx, NULL, &siglen, idb->hash, mdlen) <= 0)
		goto out;
	if (siglen > sizeof(idb->hash))
		goto out;

	sig = OPENSSL_malloc(siglen);
	if (!sig) {
		ret = -ENOMEM;
		goto out;
	}

	/* Finally update the header before adding the actual signature */
	idb->magic = NEWIDB_MAGIC_RKSS;
	idb->flags |= NEWIDB_FLAGS_SIGNED;
	if (EVP_PKEY_get_bits(key) == 4096)
		idb->flags |= NEWIDB_FLAGS_RSA4096;
	else
		idb->flags |= NEWIDB_FLAGS_RSA2048;
	idb_hash(idb);

	/* Non-deterministic signature due to random MGF initialization */
	ret = EVP_PKEY_sign(ctx, sig, &siglen, idb->hash, mdlen);
	if (ret <= 0) {
		fprintf(stderr, "Failed to sign image: %s\n",
			ERR_error_string(ERR_get_error(), 0));
		goto out;
	}

	bin2lebin(sig, idb->hash, siglen);

	ret = 0;
out:
	OPENSSL_free(sig);
	EVP_PKEY_CTX_free(ctx);

	return ret;
}

static int idb_add_pub_key(struct newidb *idb, EVP_PKEY *key)
{
	BIGNUM *e;
	BIGNUM *n;
	BIGNUM *np;
	int ret;

	e = BN_new();
	n = BN_new();
	np = BN_new();
	if (!e || !n || !np) {
		ret = -ENOMEM;
		goto out;
	}

	ret = rsa_get_params(key, e, n, np);
	if (ret) {
		ret = -EINVAL;
		goto out;
	}

	BN_bn2lebinpad(e, idb->key.exponent, sizeof(idb->key.exponent));
	BN_bn2lebinpad(n, idb->key.modulus, sizeof(idb->key.modulus));
	BN_bn2lebinpad(np, idb->key.np, sizeof(idb->key.np));

	idb_hash(idb);

out:
	BN_free(np);
	BN_free(n);
	BN_free(e);

	return ret;
}

static int sign_newidb(struct newidb *idb, const char *path)
{
	EVP_PKEY *pkey = NULL;
	int ret = -1;

	if (!path)
		return -EINVAL;

	if (strncmp(path, "pkcs11:", 7) == 0)
		pkey = load_key_pkcs11(path);
	else
		pkey = load_key_file(path);
	if (!pkey) {
		fprintf(stderr, "Failed to load key %s\n", path);
		return -EINVAL;
	}
	if (!EVP_PKEY_is_a(pkey, "RSA") && !EVP_PKEY_is_a(pkey, "RSA-PSS")) {
		fprintf(stderr, "%s is not an RSA key\n", path);
		ret = -EINVAL;
		goto out;
	}

	ret = idb_add_pub_key(idb, pkey);
	if (ret) {
		fprintf(stderr, "Failed to add public key to header\n");
		goto out;
	}
	ret = idb_sign(idb, pkey);
	if (ret) {
		fprintf(stderr, "Failed to sign image\n");
		goto out;
	}

out:
	EVP_PKEY_free(pkey);

	return ret;
}
#else
static int sign_newidb(struct newidb *idb, const char *path)
{
       fprintf(stderr, "Signing support requires at least OpenSSL 3.0\n");
       return -ENOSYS;
}
#endif

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
"  -k <key>    Sign the image with <key> as PEM file or PKCS#11 uri\n"
"  -h          This help\n",
	prgname);
}

int main(int argc, char *argv[])
{
	int opt, i, fd;
	const char *outfile = NULL;
	const char *key = NULL;
	struct newidb idb = {};

	while ((opt = getopt_long(argc, argv, "ho:k:", cbootcmd, NULL)) > 0) {
		switch (opt) {
		case 'o':
			outfile = optarg;
			break;
		case 'k':
			key = optarg;
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

	if (!(n_code == 1 && has_magic(code[0].buf)))
		create_newidb(&idb);

	if (key) {
		int ret;

		ret = sign_newidb(&idb, key);
		if (ret) {
			fprintf(stderr, "%s failed: Cannot sign image\n", outfile);
			exit(1);
		}
	}

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
