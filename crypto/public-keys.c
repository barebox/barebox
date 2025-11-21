// SPDX-License-Identifier: GPL-2.0-only
#define pr_fmt(fmt) "public-keys: " fmt

#include <common.h>
#include <crypto/public_key.h>
#include <crypto/rsa.h>
#include <crypto/ecdsa.h>

DEFINE_IDR(public_keys);

const struct public_key *public_key_get(const char *name, const char *keyring)
{
	const struct public_key *key;
	int id;

	for_each_public_key_keyring(key, id, keyring) {
		if (!strcmp(key->key_name_hint, name))
			return key;
	}

	return NULL;
}

int public_key_add(struct public_key *key)
{
	if (!key->keyring || *key->keyring == '\0') {
		pr_warn("Aborting addition of public key: No keyring specified\n");
		return -EINVAL;
	}

	if (public_key_get(key->key_name_hint, key->keyring)) {
		pr_warn("Aborting addition of public key: Duplicate fit name hint\n");
		return -EEXIST;
	}

	return idr_alloc(&public_keys, key, 0, INT_MAX, GFP_NOWAIT);
}

int public_key_verify(const struct public_key *key, const uint8_t *sig,
		      const uint32_t sig_len, const uint8_t *hash,
		      enum hash_algo algo)
{
	switch (key->type) {
	case PUBLIC_KEY_TYPE_RSA:
		return rsa_verify(key->rsa, sig, sig_len, hash, algo);
	case PUBLIC_KEY_TYPE_ECDSA:
		return ecdsa_verify(key->ecdsa, sig, sig_len, hash);
	}

	return -ENOKEY;
}

extern struct public_key * __public_keys_start[];
extern struct public_key * __public_keys_end[];

static int init_public_keys(void)
{
	struct public_key * const *iter;
	int ret;

	for (iter = __public_keys_start; iter != __public_keys_end; iter++) {
		ret = idr_alloc(&public_keys, *iter, 0, INT_MAX, GFP_NOWAIT);
		if (ret < 0)
			pr_warn("error while adding key\n");
	}

	return 0;
}

device_initcall(init_public_keys);

#ifdef CONFIG_CRYPTO_BUILTIN_KEYS
#include "public-keys.h"
#endif
