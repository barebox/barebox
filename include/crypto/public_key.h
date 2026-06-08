#ifndef __CRYPTO_PUBLIC_KEY_H
#define __CRYPTO_PUBLIC_KEY_H

#include <digest.h>
#include <linux/list.h>
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
	const unsigned char *hash;
	unsigned int hashlen;

	union {
		const struct rsa_public_key *rsa;
		const struct ecdsa_public_key *ecdsa;
	};
};

/*
 * Linker-list entry assigning a compiled-in public_key to a named keyring.
 * Emitted by scripts/keytoc into .public_keys.rodata.* and consumed at init.
 */
struct public_key_record {
	const char *keyring;
	const struct public_key *key;
};

/*
 * A keyring is a named container that holds keyring_link entries. Each link
 * references either a public_key or another keyring (sub-keyring), so keyrings
 * can be nested. A given key may be linked into any number of keyrings.
 */
struct keyring {
	const char *name;
	struct list_head links;	/* list of struct keyring_link */
	struct list_head node;	/* link in keyring_registry */
};

enum keyring_link_type {
	KEYRING_LINK_KEY,
	KEYRING_LINK_KEYRING,
};

struct keyring_link {
	enum keyring_link_type type;
	struct list_head node;
	union {
		const struct public_key *key;
		const struct keyring *keyring;
	};
};

struct keyring *keyring_create(const char *name);
struct keyring *keyring_find(const char *name);

int keyring_link_key(struct keyring *kr, const struct public_key *key);
int keyring_unlink_key(struct keyring *kr, const struct public_key *key);

int keyring_link_keyring(struct keyring *kr, const struct keyring *sub);
int keyring_unlink_keyring(struct keyring *kr, const struct keyring *sub);

const struct public_key *keyring_find_key(const struct keyring *kr,
					  const char *key_name_hint);

#define KEYRING_MAX_DEPTH 4

struct keyring_iter {
	const struct keyring *stack[KEYRING_MAX_DEPTH];
	struct list_head *cursor[KEYRING_MAX_DEPTH];
	int depth;
};

const struct public_key *keyring_iter_next(struct keyring_iter *it,
					   const struct keyring *root);

/*
 * Iterate every key reachable from @kr, recursing into sub-keyrings.
 * The same key may be yielded more than once if it is linked into multiple
 * (sub-)keyrings; callers that care must dedup themselves.
 */
#define for_each_key_in_keyring(key, kr)					\
	for (struct keyring_iter __it = { .depth = -1 };			\
	     ((key) = keyring_iter_next(&__it, (kr))) != NULL;			\
	    )

/* Iterate only the direct links of @kr (does not recurse into sub-keyrings) */
#define for_each_link_in_keyring(link, kr) \
	list_for_each_entry(link, &(kr)->links, node)

extern struct list_head keyring_registry;

#define for_each_keyring(kr) \
	list_for_each_entry(kr, &keyring_registry, node)

int public_key_add(const char *keyring, const struct public_key *key);
int public_key_verify(const struct public_key *key, const uint8_t *sig,
		      const uint32_t sig_len, const uint8_t *hash,
		      enum hash_algo algo);

#endif /* __CRYPTO_PUBLIC_KEY_H */
