#ifndef __CRYPTO_PUBLIC_KEY_H
#define __CRYPTO_PUBLIC_KEY_H

#include <digest.h>
#include <linux/idr.h>
#include <string.h>

struct rsa_public_key;
struct ecdsa_public_key;

enum public_key_type {
	PUBLIC_KEY_TYPE_RSA,
	PUBLIC_KEY_TYPE_ECDSA,
};

static inline const char *public_key_type_string(enum public_key_type type)
{
	switch (type) {
	case PUBLIC_KEY_TYPE_RSA:
		return "RSA";
	case PUBLIC_KEY_TYPE_ECDSA:
		return "ECDSA";
	default:
		return "unknown";
	}
}

struct public_key {
	enum public_key_type type;
	const char *key_name_hint;
	char *keyring;
	const unsigned char *hash;
	unsigned int hashlen;

	union {
		const struct rsa_public_key *rsa;
		const struct ecdsa_public_key *ecdsa;
	};
};

int public_key_add(struct public_key *key);
const struct public_key *public_key_get(const char *name, const char *keyring);
const struct public_key *public_key_next(const struct public_key *prev);

extern struct idr public_keys;

#define for_each_public_key(key, id) \
		idr_for_each_entry(&public_keys, key, id)

#define for_each_public_key_keyring(key, id, _keyring)                    \
	for_each_public_key(key, id)                                      \
		if (!key->keyring || strcmp(key->keyring, _keyring) != 0) \
			continue;                                         \
		else

int public_key_verify(const struct public_key *key, const uint8_t *sig,
		      const uint32_t sig_len, const uint8_t *hash,
		      enum hash_algo algo);

#endif /* __CRYPTO_PUBLIC_KEY_H */
