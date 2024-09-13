// SPDX-License-Identifier: GPL-2.0-only
#define pr_fmt(fmt) "public-keys: " fmt

#include <common.h>
#include <crypto/public_key.h>
#include <rsa.h>

static LIST_HEAD(public_keys);

const struct public_key *public_key_next(const struct public_key *prev)
{
	prev = list_prepare_entry(prev, &public_keys, list);
	list_for_each_entry_continue(prev, &public_keys, list)
		return prev;

	return NULL;
}

const struct public_key *public_key_get(const char *name)
{
	const struct public_key *key;

	list_for_each_entry(key, &public_keys, list) {
		if (!strcmp(key->key_name_hint, name))
			return key;
	}

	return NULL;
}

int public_key_add(struct public_key *key)
{
	if (public_key_get(key->key_name_hint))
		return -EEXIST;

	list_add_tail(&key->list, &public_keys);

	return 0;
}

static struct public_key *public_key_dup(const struct public_key *key)
{
	struct public_key *k = xzalloc(sizeof(*k));

	k->type = key->type;
	if (key->key_name_hint)
		k->key_name_hint = xstrdup(key->key_name_hint);

	switch (key->type) {
	case PUBLIC_KEY_TYPE_RSA:
		k->rsa = rsa_key_dup(key->rsa);
		if (!k->rsa)
			goto err;
		break;
	default:
		goto err;
	}

	return k;
err:
	free(k->key_name_hint);
	free(k);

	return NULL;
}

int public_key_verify(const struct public_key *key, const uint8_t *sig,
		      const uint32_t sig_len, const uint8_t *hash,
		      enum hash_algo algo)
{
	switch (key->type) {
	case PUBLIC_KEY_TYPE_RSA:
		return rsa_verify(key->rsa, sig, sig_len, hash, algo);
	}

	return -ENOKEY;
}

extern const struct public_key * const __public_keys_start;
extern const struct public_key * const __public_keys_end;

static int init_public_keys(void)
{
	const struct public_key * const *iter;

	for (iter = &__public_keys_start; iter != &__public_keys_end; iter++) {
		struct public_key *key = public_key_dup(*iter);

		if (!key)
			continue;

		public_key_add(key);
	}

	return 0;
}

device_initcall(init_public_keys);

#ifdef CONFIG_CRYPTO_BUILTIN_KEYS
#include "public-keys.h"
#endif
