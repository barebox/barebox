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

struct digest;

struct digest_algo {
	char *name;

	int (*alloc)(struct digest *d);
	void (*free)(struct digest *d);
	int (*init)(struct digest *d);
	int (*update)(struct digest *d, const void *data, unsigned long len);
	int (*final)(struct digest *d, unsigned char *md);
	int (*set_key)(struct digest *d, const unsigned char *key, unsigned int len);
	int (*verify)(struct digest *d, const unsigned char *md);

	unsigned int length;
	unsigned int ctx_length;

	struct list_head list;
};

struct digest {
	struct digest_algo *algo;
	void *ctx;
};

/*
 * digest functions
 */
int digest_algo_register(struct digest_algo *d);
void digest_algo_unregister(struct digest_algo *d);

struct digest *digest_alloc(const char *name);
void digest_free(struct digest *d);

int digest_file_window(struct digest *d, const char *filename,
		       unsigned char *hash,
		       ulong start, ulong size);
int digest_file(struct digest *d, const char *filename,
		       unsigned char *hash);
int digest_file_by_name(const char *algo, const char *filename,
		       unsigned char *hash);

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

static inline int digest_verify(struct digest *d, const unsigned char *md)
{
	return d->algo->verify(d, md);
}

static inline int digest_length(struct digest *d)
{
	return d->algo->length;
}

static inline int digest_set_key(struct digest *d, const unsigned char *key,
		unsigned int len)
{
	if (!d->algo->set_key)
		return -ENOTSUPP;
	return d->algo->set_key(d, key, len);
}

#endif /* __SH_ST_DEVICES_H__ */
