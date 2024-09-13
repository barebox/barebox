// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2021 IBM Corporation
 */

#include <common.h>

#include <crypto/internal/ecc.h>
#include <crypto/public_key.h>
#include <crypto/ecdh.h>
#include <crypto/ecdsa.h>

struct ecc_ctx {
	unsigned int curve_id;
	const struct ecc_curve *curve;

	bool pub_key_set;
	u64 x[ECC_MAX_DIGITS]; /* pub key x and y coordinates */
	u64 y[ECC_MAX_DIGITS];
	struct ecc_point pub_key;
};

static int _ecdsa_verify(struct ecc_ctx *ctx, const u64 *hash, const u64 *r, const u64 *s)
{
	const struct ecc_curve *curve = ctx->curve;
	unsigned int ndigits = curve->g.ndigits;
	u64 s1[ECC_MAX_DIGITS];
	u64 u1[ECC_MAX_DIGITS];
	u64 u2[ECC_MAX_DIGITS];
	u64 x1[ECC_MAX_DIGITS];
	u64 y1[ECC_MAX_DIGITS];
	struct ecc_point res = ECC_POINT_INIT(x1, y1, ndigits);

	/* 0 < r < n  and 0 < s < n */
	if (vli_is_zero(r, ndigits) || vli_cmp(r, curve->n, ndigits) >= 0 ||
	    vli_is_zero(s, ndigits) || vli_cmp(s, curve->n, ndigits) >= 0)
		return -EBADMSG;

	/* hash is given */
	pr_debug("hash : %016llx %016llx ... %016llx\n",
		 hash[ndigits - 1], hash[ndigits - 2], hash[0]);

	/* s1 = (s^-1) mod n */
	vli_mod_inv(s1, s, curve->n, ndigits);
	/* u1 = (hash * s1) mod n */
	vli_mod_mult_slow(u1, hash, s1, curve->n, ndigits);
	/* u2 = (r * s1) mod n */
	vli_mod_mult_slow(u2, r, s1, curve->n, ndigits);
	/* res = u1*G + u2 * pub_key */
	ecc_point_mult_shamir(&res, u1, &curve->g, u2, &ctx->pub_key, curve);

	/* res.x = res.x mod n (if res.x > order) */
	if (unlikely(vli_cmp(res.x, curve->n, ndigits) == 1))
		/* faster alternative for NIST p521, p384, p256 & p192 */
		vli_sub(res.x, res.x, curve->n, ndigits);

	if (!vli_cmp(res.x, r, ndigits))
		return 0;

	return -EKEYREJECTED;
}

static int ecdsa_key_size(const char *curve_name)
{
	if (!strcmp(curve_name, "prime256v1"))
		return 256;
	else
		return 0;
}

int ecdsa_verify(const struct ecdsa_public_key *key, const uint8_t *sig,
		 const uint32_t sig_len, const uint8_t *hash)
{
	struct ecc_ctx _ctx = {};
	struct ecc_ctx *ctx = &_ctx;
	unsigned int curve_id = ECC_CURVE_NIST_P256;
	int ret;
	const void *r, *s;
	u64 rh[4], sh[4];
	u64 mhash[ECC_MAX_DIGITS];
	int key_size_bytes = key->size_bits / 8;

	ctx->curve_id = curve_id;
	ctx->curve = ecc_get_curve(curve_id);
	if (!ctx->curve)
		return -EINVAL;

	ctx->pub_key = ECC_POINT_INIT(ctx->x, ctx->y, ctx->curve->g.ndigits);
	memcpy(ctx->pub_key.x, key->x, key_size_bytes);
	memcpy(ctx->pub_key.y, key->y, key_size_bytes);

	ret = ecc_is_pubkey_valid_full(ctx->curve, &ctx->pub_key);
	if (ret)
		return ret;

	r = sig;
	s = sig + key_size_bytes;

	ecc_swap_digits((u64 *)r, rh, ctx->curve->g.ndigits);
	ecc_swap_digits((u64 *)s, sh, ctx->curve->g.ndigits);

	ecc_swap_digits((u64 *)hash, mhash, ctx->curve->g.ndigits);

	return _ecdsa_verify(ctx, (void *)mhash, rh, sh);
}

static LIST_HEAD(ecdsa_keys);

int ecdsa_key_add(struct ecdsa_public_key *key)
{
	list_add_tail(&key->list, &ecdsa_keys);

	return 0;
}

struct ecdsa_public_key *ecdsa_key_dup(const struct ecdsa_public_key *key)
{
	struct ecdsa_public_key *new;
	int key_size_bits;

	key_size_bits = ecdsa_key_size(key->curve_name);
	if (!key_size_bits)
		return NULL;

	new = xmemdup(key, sizeof(*key));
	new->x = xmemdup(key->x, key_size_bits / 8);
	new->y = xmemdup(key->y, key_size_bits / 8);
	new->size_bits = key_size_bits;

	return new;
}

const struct ecdsa_public_key *ecdsa_key_next(const struct ecdsa_public_key *prev)
{
	prev = list_prepare_entry(prev, &ecdsa_keys, list);
	list_for_each_entry_continue(prev, &ecdsa_keys, list)
		return prev;

	return NULL;
}
