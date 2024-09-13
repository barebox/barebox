// SPDX-License-Identifier: GPL-2.0-only
#define pr_fmt(fmt) "public-keys: " fmt

#include <common.h>
#include <crypto/public_key.h>
#include <rsa.h>

extern const struct public_key * const __public_keys_start;
extern const struct public_key * const __public_keys_end;

static int init_public_keys(void)
{
	const struct public_key * const *iter;
	int ret;

	for (iter = &__public_keys_start; iter != &__public_keys_end; iter++) {
		struct rsa_public_key *rsa_key;

		switch ((*iter)->type) {
		case PUBLIC_KEY_TYPE_RSA:
			rsa_key = rsa_key_dup((*iter)->rsa);
			if (!rsa_key)
				continue;

			ret = rsa_key_add(rsa_key);
			if (ret)
				pr_err("Cannot add rsa key: %pe\n", ERR_PTR(ret));
			break;
		default:
			pr_err("Ignoring unknown key type %u\n", (*iter)->type);
		}

	}

	return 0;
}

device_initcall(init_public_keys);

#ifdef CONFIG_CRYPTO_BUILTIN_KEYS
#include "public-keys.h"
#endif
