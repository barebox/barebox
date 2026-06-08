// SPDX-License-Identifier: GPL-2.0-only
#define pr_fmt(fmt) "public-keys: " fmt

#include <common.h>
#include <crypto/public_key.h>
#include <crypto/rsa.h>
#include <crypto/ecdsa.h>
#include <linux/list.h>
#include <malloc.h>
#include <xfuncs.h>

LIST_HEAD(keyring_registry);

struct keyring *keyring_find(const char *name)
{
	struct keyring *kr;

	if (!name)
		return NULL;

	list_for_each_entry(kr, &keyring_registry, node) {
		if (!strcmp(kr->name, name))
			return kr;
	}
	return NULL;
}

struct keyring *keyring_create(const char *name)
{
	struct keyring *kr;

	if (!name || !*name)
		return ERR_PTR(-EINVAL);

	if (keyring_find(name))
		return ERR_PTR(-EEXIST);

	kr = xzalloc(sizeof(*kr));
	kr->name = xstrdup(name);
	INIT_LIST_HEAD(&kr->links);
	INIT_LIST_HEAD(&kr->node);
	list_add_tail(&kr->node, &keyring_registry);

	return kr;
}

int keyring_link_key(struct keyring *kr, const struct public_key *key)
{
	struct keyring_link *link;

	if (!kr || !key)
		return -EINVAL;

	link = xzalloc(sizeof(*link));
	link->type = KEYRING_LINK_KEY;
	link->key = key;
	list_add_tail(&link->node, &kr->links);

	return 0;
}

int keyring_unlink_key(struct keyring *kr, const struct public_key *key)
{
	struct keyring_link *link, *n;

	if (!kr || !key)
		return -EINVAL;

	list_for_each_entry_safe(link, n, &kr->links, node) {
		if (link->type == KEYRING_LINK_KEY && link->key == key) {
			list_del(&link->node);
			free(link);
			return 0;
		}
	}
	return -ENOENT;
}

static bool keyring_contains(const struct keyring *kr, const struct keyring *target)
{
	const struct keyring_link *link;

	if (kr == target)
		return true;

	list_for_each_entry(link, &kr->links, node) {
		if (link->type != KEYRING_LINK_KEYRING)
			continue;
		if (keyring_contains(link->keyring, target))
			return true;
	}
	return false;
}

int keyring_link_keyring(struct keyring *kr, const struct keyring *sub)
{
	struct keyring_link *link;

	if (!kr || !sub)
		return -EINVAL;

	/*
	 * Refuse to create a cycle: if sub already (transitively) contains kr
	 * — or sub is kr itself — adding sub as a child of kr would loop.
	 * Assuming the graph was cycle-free before, this check is enough to
	 * keep it that way.
	 */
	if (keyring_contains(sub, kr))
		return -ELOOP;

	link = xzalloc(sizeof(*link));
	link->type = KEYRING_LINK_KEYRING;
	link->keyring = sub;
	list_add_tail(&link->node, &kr->links);

	return 0;
}

int keyring_unlink_keyring(struct keyring *kr, const struct keyring *sub)
{
	struct keyring_link *link, *n;

	if (!kr || !sub)
		return -EINVAL;

	list_for_each_entry_safe(link, n, &kr->links, node) {
		if (link->type == KEYRING_LINK_KEYRING && link->keyring == sub) {
			list_del(&link->node);
			free(link);
			return 0;
		}
	}
	return -ENOENT;
}

const struct public_key *keyring_iter_next(struct keyring_iter *it,
					   const struct keyring *root)
{
	if (it->depth < 0) {
		if (!root)
			return NULL;
		it->depth = 0;
		it->stack[0] = root;
		it->cursor[0] = (struct list_head *)&root->links;
	}

	while (it->depth >= 0) {
		struct list_head *head = (struct list_head *)&it->stack[it->depth]->links;
		struct list_head *next = it->cursor[it->depth]->next;
		struct keyring_link *link;

		if (next == head) {
			it->depth--;
			continue;
		}

		it->cursor[it->depth] = next;
		link = list_entry(next, struct keyring_link, node);

		if (link->type == KEYRING_LINK_KEY)
			return link->key;

		if (it->depth + 1 >= KEYRING_MAX_DEPTH) {
			pr_warn("keyring nesting too deep, skipping %s\n",
				link->keyring->name);
			continue;
		}

		it->depth++;
		it->stack[it->depth] = link->keyring;
		it->cursor[it->depth] = (struct list_head *)&link->keyring->links;
	}

	return NULL;
}

const struct public_key *keyring_find_key(const struct keyring *kr,
					  const char *key_name_hint)
{
	const struct public_key *key;

	if (!kr || !key_name_hint)
		return ERR_PTR(-EINVAL);

	for_each_key_in_keyring(key, kr) {
		if (!key->key_name_hint)
			continue;
		if (!strcmp(key->key_name_hint, key_name_hint))
			return key;
	}

	return ERR_PTR(-ENOENT);
}

int public_key_add(const char *keyring, const struct public_key *key)
{
	struct keyring *kr;
	const struct public_key *conflict;

	if (!keyring || !*keyring)
		return -EINVAL;

	kr = keyring_find(keyring);
	if (!kr) {
		kr = keyring_create(keyring);
		if (IS_ERR(kr))
			return PTR_ERR(kr);
	}

	conflict = keyring_find_key(kr, key->key_name_hint);
	if (!IS_ERR(conflict)) {
		pr_warn("Cannot add key: key_name_hint %s already exists in keyring %s\n",
			key->key_name_hint, keyring);
		return -EEXIST;
	}

	return keyring_link_key(kr, key);
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

extern const struct public_key_record __public_keys_start[];
extern const struct public_key_record __public_keys_end[];

static int init_public_keys(void)
{
	const struct public_key_record *rec;
	int ret;

	for (rec = __public_keys_start; rec != __public_keys_end; rec++) {
		ret = public_key_add(rec->keyring, rec->key);
		if (ret)
			pr_warn("error while adding key %s to %s: %pe\n",
				rec->key->key_name_hint ?: "(noname)",
				rec->keyring, ERR_PTR(ret));
	}

	return 0;
}

device_initcall(init_public_keys);

#ifdef CONFIG_CRYPTO_BUILTIN_KEYS
#include "public-keys.h"
#endif
