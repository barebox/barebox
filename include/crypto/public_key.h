#ifndef __CRYPTO_PUBLIC_KEY_H
#define __CRYPTO_PUBLIC_KEY_H

struct rsa_public_key;
struct ecdsa_public_key;

enum pulic_key_type {
	PUBLIC_KEY_TYPE_RSA,
};

struct public_key {
	enum pulic_key_type type;

	union {
		struct rsa_public_key *rsa;
		struct ecdsa_public_key *ecdsa;
	};
};

#endif /* __CRYPTO_PUBLIC_KEY_H */
