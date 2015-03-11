/*
 * (C) Copyright 2015 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * GPL v2 only
 */

#include <common.h>
#include <digest.h>
#include <malloc.h>

#include "internal.h"

struct digest_hmac {
	char *name;
	unsigned int pad_length;
	struct digest_algo algo;
};

struct digest_hmac_ctx {
	struct digest *d;

	unsigned char *ipad;
	unsigned char *opad;

	unsigned char *key;
	unsigned int keylen;
};

static inline struct digest_hmac * to_digest_hmac(struct digest_algo *algo)
{
	return container_of(algo, struct digest_hmac, algo);
}

static int digest_hmac_alloc(struct digest *d)
{
	struct digest_hmac_ctx *dh = d->ctx;
	struct digest_hmac *hmac = to_digest_hmac(d->algo);

	dh->d = digest_alloc(hmac->name);
	if (!dh->d)
		return -EINVAL;

	dh->ipad = xmalloc(hmac->pad_length);
	dh->opad = xmalloc(hmac->pad_length);

	return 0;
}

static void digest_hmac_free(struct digest *d)
{
	struct digest_hmac_ctx *dh = d->ctx;

	free(dh->ipad);
	free(dh->opad);
	free(dh->key);

	digest_free(dh->d);
}

static int digest_hmac_set_key(struct digest *d, const unsigned char *key,
				unsigned int len)
{
	struct digest_hmac_ctx *dh = d->ctx;
	struct digest_hmac *hmac = to_digest_hmac(d->algo);

	free(dh->key);
	if (len > hmac->pad_length) {
		unsigned char *sum;

		sum = xmalloc(digest_length(dh->d));
		digest_init(dh->d);
		digest_update(dh->d, dh->key, dh->keylen);
		digest_final(dh->d, sum);
		dh->keylen = digest_length(dh->d);
		dh->key = sum;
	} else {
		dh->key = xmalloc(len);
		memcpy(dh->key, key, len);
		dh->keylen = len;
	}

	return 0;
}

static int digest_hmac_init(struct digest *d)
{
	struct digest_hmac_ctx *dh = d->ctx;
	struct digest_hmac *hmac = to_digest_hmac(d->algo);
	int i;
	unsigned char *key = dh->key;
	unsigned int keylen = dh->keylen;

	memset(dh->ipad, 0x36, hmac->pad_length);
	memset(dh->opad, 0x5C, hmac->pad_length);

	for (i = 0; i < keylen; i++) {
		dh->ipad[i] = (unsigned char)(dh->ipad[i] ^ key[i]);
		dh->opad[i] = (unsigned char)(dh->opad[i] ^ key[i]);
	}

	digest_init(dh->d);
	digest_update(dh->d, dh->ipad, hmac->pad_length);

	return 0;
}

static int digest_hmac_update(struct digest *d, const void *data,
			      unsigned long len)
{
	struct digest_hmac_ctx *dh = d->ctx;

	return digest_update(dh->d, data, len);
}

static int digest_hmac_final(struct digest *d, unsigned char *md)
{
	struct digest_hmac_ctx *dh = d->ctx;
	struct digest_hmac *hmac = to_digest_hmac(d->algo);
	unsigned char *tmp = NULL;

	tmp = xmalloc(digest_length(d));

	digest_final(dh->d, tmp);
	digest_init(dh->d);
	digest_update(dh->d, dh->opad, hmac->pad_length);
	digest_update(dh->d, tmp, digest_length(d));
	digest_final(dh->d, md);

	free(tmp);

	return 0;
}

struct digest_algo hmac_algo = {
	.alloc = digest_hmac_alloc,
	.init = digest_hmac_init,
	.update = digest_hmac_update,
	.final = digest_hmac_final,
	.set_key = digest_hmac_set_key,
	.free = digest_hmac_free,
	.ctx_length = sizeof(struct digest_hmac),
};

int digest_hmac_register(struct digest_algo *algo, unsigned int pad_length)
{
	struct digest_hmac *dh;

	if (!algo || !pad_length)
		return -EINVAL;

	dh = xzalloc(sizeof(*dh));
	dh->name = xstrdup(algo->name);
	dh->pad_length = pad_length;
	dh->algo = hmac_algo;
	dh->algo.length = algo->length;
	dh->algo.name = asprintf("hmac(%s)", algo->name);

	return digest_algo_register(&dh->algo);
}
