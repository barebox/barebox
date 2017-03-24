/*
 * (C) Copyright 2015 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * GPL v2 only
 */

#include <common.h>
#include <digest.h>
#include <malloc.h>
#include <init.h>
#include <crypto/internal.h>

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

static inline struct digest_hmac *to_digest_hmac(struct digest_algo *algo)
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

	d->length = dh->d->algo->length;

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
	unsigned char *sum = NULL;
	int ret;

	free(dh->key);
	if (len > hmac->pad_length) {
		sum = xmalloc(digest_length(dh->d));
		ret = digest_digest(dh->d, dh->key, dh->keylen, sum);
		if (ret)
			goto err;
		dh->keylen = digest_length(dh->d);
		dh->key = sum;
	} else {
		dh->key = xmalloc(len);
		memcpy(dh->key, key, len);
		dh->keylen = len;
	}

	ret = 0;
err:
	free(sum);
	return ret;
}

static int digest_hmac_init(struct digest *d)
{
	struct digest_hmac_ctx *dh = d->ctx;
	struct digest_hmac *hmac = to_digest_hmac(d->algo);
	int i, ret;
	unsigned char *key = dh->key;
	unsigned int keylen = dh->keylen;

	memset(dh->ipad, 0x36, hmac->pad_length);
	memset(dh->opad, 0x5C, hmac->pad_length);

	for (i = 0; i < keylen; i++) {
		dh->ipad[i] = (unsigned char)(dh->ipad[i] ^ key[i]);
		dh->opad[i] = (unsigned char)(dh->opad[i] ^ key[i]);
	}

	ret = digest_init(dh->d);
	if (ret)
		return ret;
	return digest_update(dh->d, dh->ipad, hmac->pad_length);
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
	int ret;

	tmp = xmalloc(digest_length(d));

	ret = digest_final(dh->d, tmp);
	if (ret)
		goto err;
	ret = digest_init(dh->d);
	if (ret)
		goto err;
	ret = digest_update(dh->d, dh->opad, hmac->pad_length);
	if (ret)
		goto err;
	ret = digest_update(dh->d, tmp, digest_length(d));
	if (ret)
		goto err;
	ret = digest_final(dh->d, md);

err:
	free(tmp);

	return ret;
}

struct digest_algo hmac_algo = {
	.base = {
		.priority	= 0,
		.flags		= DIGEST_ALGO_NEED_KEY,
	},
	.alloc		= digest_hmac_alloc,
	.init		= digest_hmac_init,
	.update		= digest_hmac_update,
	.final		= digest_hmac_final,
	.digest		= digest_generic_digest,
	.verify		= digest_generic_verify,
	.set_key	= digest_hmac_set_key,
	.free		= digest_hmac_free,
	.ctx_length	= sizeof(struct digest_hmac),
};

static int digest_hmac_register(char *name, unsigned int pad_length)
{
	struct digest_hmac *dh;

	if (!name || !pad_length)
		return -EINVAL;

	dh = xzalloc(sizeof(*dh));
	dh->name = xstrdup(name);
	dh->pad_length = pad_length;
	dh->algo = hmac_algo;
	dh->algo.base.name = basprintf("hmac(%s)", name);
	dh->algo.base.driver_name = basprintf("hmac(%s)-generic", name);

	return digest_algo_register(&dh->algo);
}

static int digest_hmac_initcall(void)
{
	if (IS_ENABLED(CONFIG_MD5))
		digest_hmac_register("md5", 64);
	if (IS_ENABLED(CONFIG_SHA1))
		digest_hmac_register("sha1", 64);
	if (IS_ENABLED(CONFIG_SHA224))
		digest_hmac_register("sha224", 64);
	if (IS_ENABLED(CONFIG_SHA256))
		digest_hmac_register("sha256", 64);
	if (IS_ENABLED(CONFIG_SHA384))
		digest_hmac_register("sha384", 128);
	if (IS_ENABLED(CONFIG_SHA512))
		digest_hmac_register("sha512", 128);

	return 0;
}
coredevice_initcall(digest_hmac_initcall);
