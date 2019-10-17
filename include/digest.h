/*
 * (C) Copyright 2008-2010 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of
 * the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#ifndef __DIGEST_H__
#define __DIGEST_H__

#include <linux/list.h>
#include <errno.h>

struct digest;

enum hash_algo {
	HASH_ALGO_MD4,
	HASH_ALGO_MD5,
	HASH_ALGO_SHA1,
	HASH_ALGO_RIPE_MD_160,
	HASH_ALGO_SHA256,
	HASH_ALGO_SHA384,
	HASH_ALGO_SHA512,
	HASH_ALGO_SHA224,
	HASH_ALGO_RIPE_MD_128,
	HASH_ALGO_RIPE_MD_256,
	HASH_ALGO_RIPE_MD_320,
	HASH_ALGO_WP_256,
	HASH_ALGO_WP_384,
	HASH_ALGO_WP_512,
	HASH_ALGO_TGR_128,
	HASH_ALGO_TGR_160,
	HASH_ALGO_TGR_192,
	HASH_ALGO_SM3_256,
	/*
	 * The above are exported from the Kernel as
	 * /usr/include/linux/hash_info.h and thus have a fixed number, do not
	 * change it. Below are barebox specific, subject to renumbering.
	 */
	HASH_ALGO_CRC32,
	HASH_ALGO__LAST
};

struct crypto_alg {
	char *name;
	char *driver_name;
	int priority;
#define DIGEST_ALGO_NEED_KEY	(1 << 0)
	unsigned int flags;
	enum hash_algo algo;
};

struct digest_algo {
	struct crypto_alg base;

	int (*alloc)(struct digest *d);
	void (*free)(struct digest *d);
	int (*init)(struct digest *d);
	int (*update)(struct digest *d, const void *data, unsigned long len);
	int (*final)(struct digest *d, unsigned char *md);
	int (*digest)(struct digest *d, const void *data,
		      unsigned int len, u8 *out);
	int (*set_key)(struct digest *d, const unsigned char *key, unsigned int len);
	int (*verify)(struct digest *d, const unsigned char *md);

	unsigned int length;
	unsigned int ctx_length;

	struct list_head list;
};

struct digest {
	struct digest_algo *algo;
	void *ctx;
	unsigned int length;
};

/*
 * digest functions
 */
#ifdef CONFIG_DIGEST
int digest_algo_register(struct digest_algo *d);
void digest_algo_unregister(struct digest_algo *d);
void digest_algo_prints(const char *prefix);

struct digest *digest_alloc(const char *name);
struct digest *digest_alloc_by_algo(enum hash_algo);
void digest_free(struct digest *d);

int digest_file_window(struct digest *d, const char *filename,
		       unsigned char *hash,
		       const unsigned char *sig,
		       loff_t start, loff_t size);
int digest_file(struct digest *d, const char *filename,
		unsigned char *hash,
		const unsigned char *sig);
int digest_file_by_name(const char *algo, const char *filename,
			unsigned char *hash,
			const unsigned char *sig);
#else
static inline struct digest *digest_alloc(const char *name)
{
	return NULL;
}

static inline struct digest *digest_alloc_by_algo(enum hash_algo algo)
{
	return NULL;
}

static inline void digest_free(struct digest *d)
{
}
#endif

static inline int digest_init(struct digest *d)
{
	return d->algo->init(d);
}

static inline int digest_update(struct digest *d, const void *data,
				      unsigned long len)
{
	return d->algo->update(d, data, len);
}

static inline int digest_final(struct digest *d, unsigned char *md)
{
	return d->algo->final(d, md);
}

static inline int digest_digest(struct digest *d, const void *data,
		      unsigned int len, u8 *md)
{
	return d->algo->digest(d, data, len, md);
}

static inline int digest_verify(struct digest *d, const unsigned char *md)
{
	return d->algo->verify(d, md);
}

static inline int digest_length(struct digest *d)
{
	return d->length ? d->length : d->algo->length;
}

static inline int digest_set_key(struct digest *d, const unsigned char *key,
		unsigned int len)
{
	if (!d->algo->set_key)
		return -ENOTSUPP;
	return d->algo->set_key(d, key, len);
}

static inline int digest_is_flags(struct digest *d, unsigned int flags)
{
	return d->algo->base.flags & flags;
}

static inline const char *digest_name(struct digest *d)
{
	return d->algo->base.name;
}

static inline void* digest_ctx(struct digest *d)
{
	return d->ctx;
}

#endif /* __SH_ST_DEVICES_H__ */
