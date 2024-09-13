// SPDX-License-Identifier: GPL-2.0-only
#ifndef _ECDSA_H
#define _ECDSA_H

#include <linux/types.h>
#include <linux/list.h>
#include <errno.h>

struct ecdsa_public_key {
	const char *curve_name;	/* Name of curve, e.g. "prime256v1" */
	const uint64_t *x;	/* x coordinate of public key */
	const uint64_t *y;	/* y coordinate of public key */
	unsigned int size_bits;	/* key size in bits, derived from curve name */
	struct list_head list;
};

const struct ecdsa_public_key *ecdsa_key_next(const struct ecdsa_public_key *prev);

#define for_each_ecdsa_key(key) \
	for (key = ecdsa_key_next(NULL); key; key = ecdsa_key_next(key))

#ifdef CONFIG_CRYPTO_ECDSA
int ecdsa_verify(const struct ecdsa_public_key *key, const uint8_t *sig,
		 const uint32_t sig_len, const uint8_t *hash);
int ecdsa_key_add(struct ecdsa_public_key *key);
struct ecdsa_public_key *ecdsa_key_dup(const struct ecdsa_public_key *key);
#else
static inline int ecdsa_verify(const struct ecdsa_public_key *key, const uint8_t *sig,
		 const uint32_t sig_len, const uint8_t *hash)
{
	return -ENOSYS;
}

static inline int ecdsa_key_add(struct ecdsa_public_key *key)
{
	return -ENOSYS;
}

static inline struct ecdsa_public_key *ecdsa_key_dup(const struct ecdsa_public_key *key)
{
	return NULL;
}

#endif

#endif /* _ECDSA_H */
