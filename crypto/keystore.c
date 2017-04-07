/*
 * Copyright (C) 2015 Pengutronix, Marc Kleine-Budde <kernel@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License version 2 as published by the
 * Free Software Foundation.
 */

#include <common.h>
#include <linux/kernel.h>
#include <linux/list.h>

static LIST_HEAD(keystore_list);

#define for_each_key(key) list_for_each_entry(key, &keystore_list, list)

struct keystore_key {
	struct list_head list;
	char *name;
	u8 *secret;
	int secret_len;
};

static int keystore_compare(struct list_head *a, struct list_head *b)
{
	const char *na = list_entry(a, struct keystore_key, list)->name;
	const char *nb = list_entry(b, struct keystore_key, list)->name;

	return strcmp(na, nb);
}

static struct keystore_key *get_key(const char *name)
{
	struct keystore_key *key;

	for_each_key(key)
		if (!strcmp(name, key->name))
			return key;

	return NULL;
};

/**
 * @param[in] name Name of the secret to get
 * @param[out] secret Double pointer to memory representing the secret, do _not_ free() after use
 * @param[out] secret_len Pointer to length of the secret
 */
int keystore_get_secret(const char *name, const u8 **secret, int *secret_len)
{
	struct keystore_key *key;

	if (!secret || !secret_len)
		return 0;

	key = get_key(name);
	if (!key)
		return -ENOENT;

	*secret = key->secret;
	*secret_len = key->secret_len;

	return 0;
}

/**
 * @param[in] name Name of the secret to set
 * @param[in] secret Pointer to memory holding the secret
 * @param[in] secret_len Length of the secret
 */
int keystore_set_secret(const char *name, const u8 *secret, int secret_len)
{
	struct keystore_key *key;

	/* check if key is already in store */
	key = get_key(name);
	if (key)
		return -EBUSY;

	key = xzalloc(sizeof(*key));
	INIT_LIST_HEAD(&key->list);
	key->name = xstrdup(name);
	key->secret = xmemdup(secret, secret_len);
	key->secret_len = secret_len;

	list_add_sort(&key->list, &keystore_list, keystore_compare);

	return 0;
}

void keystore_forget_secret(const char *name)
{
	struct keystore_key *key;

	key = get_key(name);
	if (!key)
		return;

	list_del(&key->list);

	free(key->name);
	free(key->secret);
	free(key);
}
