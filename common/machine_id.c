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

static void *__machine_id_hashable;
static size_t __machine_id_hashable_length;


void machine_id_set_hashable(const void *hashable, size_t len)
{

	__machine_id_hashable = xmemdup(hashable, len);
	__machine_id_hashable_length = len;
}

static int machine_id_set_bootarg(void)
{
	struct digest *digest = NULL;
	unsigned char machine_id[SHA1_DIGEST_SIZE];
	char hex_machine_id[MACHINE_ID_LENGTH];
	char *env_machine_id;
	int ret = 0;

	/* nothing to do if no hashable information provided */
	if (!__machine_id_hashable)
		goto out;

	digest = digest_alloc_by_algo(HASH_ALGO_SHA1);
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

out:
	globalvar_add_simple("machine_id", NULL);

	digest_free(digest);
	return ret;

}
late_initcall(machine_id_set_bootarg);

BAREBOX_MAGICVAR_NAMED(global_machine_id, global.machine_id, "Persistent device-specific, hexadecimal, 32-character id");
