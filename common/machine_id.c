/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2019 Pengutronix, Bastian Krause <kernel@pengutronix.de>
 */

#include <common.h>
#include <init.h>
#include <digest.h>
#include <globalvar.h>
#include <magicvar.h>
#include <crypto/sha.h>
#include <machine_id.h>

#define MACHINE_ID_LENGTH 32

static bool __machine_id_initialized;
static void *__machine_id_hashable;
static size_t __machine_id_hashable_length;

const void *machine_id_get_hashable(size_t *len)
{
	*len = __machine_id_hashable_length;
	return __machine_id_hashable;
}

/**
 * machine_id_set_hashable - Provide per-board unique data
 * @hashable: Buffer
 * @len: size of buffer
 *
 * The data supplied to the last call of this function prior to
 * late_initcall will be hashed and stored into global.machine_id,
 * which can be later used for fixup into the kernel command line
 * or for deriving application specific unique IDs via
 * machine_id_get_app_specific().
 */
void machine_id_set_hashable(const void *hashable, size_t len)
{
	__machine_id_hashable = xmemdup(hashable, len);
	__machine_id_hashable_length = len;
}

/**
 * machine_id_get_app_specific - Generates an application-specific UUID
 * @result: UUID output of the function
 * @...: pairs of (const void *, size_t) arguments of data to factor
 * into the UUID followed by a NULL sentinel value.
 *
 * Combines the machine ID with the application specific varargs data
 * to arrive at an application-specific and board-specific UUID that is
 * stable and unique.
 *
 * The function returns 0 if a UUID was successfully written into @result
 * and a negative error code otherwise.
 */
int machine_id_get_app_specific(uuid_t *result, ...)
{
	static u8 hmac[SHA256_DIGEST_SIZE];
	const void *data;
	size_t size;
	va_list args;
	struct digest *d;
	int ret;

	if (!__machine_id_initialized)
		return -ENODATA;

	d = digest_alloc("hmac(sha256)");
	if (!d)
		return -ENOSYS;

	ret = digest_set_key(d, __machine_id_hashable, __machine_id_hashable_length);
	if (ret)
		goto out;

	ret = digest_init(d);
	if (ret)
		goto out;

	ret = -ENODATA;

	va_start(args, result);

	while ((data = va_arg(args, const void *))) {
		size = va_arg(args, size_t);

		ret = digest_update(d, data, size);
		if (ret)
			break;
	}

	va_end(args);

	if (ret)
		goto out;

	ret = digest_final(d, hmac);
	if (ret)
		goto out;

	/* Take only the first half. */
	memcpy(result, hmac, min(sizeof(hmac), sizeof(*result)));

	uuid_make_v4(result);

out:
	digest_free(d);

	return ret;
}

static int machine_id_set_globalvar(void)
{
	struct digest *digest = NULL;
	unsigned char machine_id[SHA1_DIGEST_SIZE];
	char hex_machine_id[MACHINE_ID_LENGTH];
	char *env_machine_id;
	int ret;

	/* nothing to do if no hashable information provided */
	if (!__machine_id_hashable)
		return 0;

	digest = digest_alloc_by_algo(HASH_ALGO_SHA1);
	if (!digest) {
		ret = -EOPNOTSUPP;
		goto out;
	}

	ret = digest_init(digest);
	if (ret)
		goto out;

	ret = digest_update(digest, __machine_id_hashable,
			    __machine_id_hashable_length);
	if (ret)
		goto out;

	ret = digest_final(digest, machine_id);
	if (ret)
		goto out;

	/* use the first 16 bytes of the sha1 hash as the machine id */
	bin2hex(hex_machine_id, machine_id, MACHINE_ID_LENGTH/2);

	env_machine_id = basprintf("%.*s", MACHINE_ID_LENGTH, hex_machine_id);
	globalvar_add_simple("machine_id", env_machine_id);
	free(env_machine_id);
	__machine_id_initialized = true;

out:
	digest_free(digest);
	return ret;

}
late_initcall(machine_id_set_globalvar);

BAREBOX_MAGICVAR(global.machine_id, "Persistent device-specific, hexadecimal, 32-character id");
