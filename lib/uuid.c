// SPDX-License-Identifier: GPL-2.0-only
/*
 * Unified UUID/GUID definition
 *
 * Copyright (C) 2009, 2016 Intel Corp.
 *	Huang Ying <ying.huang@intel.com>
 */

#include <linux/uuid.h>
#include <module.h>
#include <stdlib.h>
#include <linux/export.h>

const guid_t guid_null;
EXPORT_SYMBOL(guid_null);
const uuid_t uuid_null;
EXPORT_SYMBOL(uuid_null);

/**
 * generate_random_uuid - generate a random UUID
 * @uuid: where to put the generated UUID
 *
 * Random UUID interface
 *
 * Used to create a Boot ID or a filesystem UUID/GUID, but can be
 * useful for other kernel drivers.
 */
void generate_random_uuid(unsigned char uuid[16])
{
	get_random_bytes(uuid, 16);
	/* Set UUID version to 4 --- truly random generation */
	uuid[6] = (uuid[6] & 0x0F) | 0x40;
	/* Set the UUID variant to DCE */
	uuid[8] = (uuid[8] & 0x3F) | 0x80;
}
EXPORT_SYMBOL(generate_random_uuid);

void generate_random_guid(unsigned char guid[16])
{
	get_random_bytes(guid, 16);
	/* Set GUID version to 4 --- truly random generation */
	guid[7] = (guid[7] & 0x0F) | 0x40;
	/* Set the GUID variant to DCE */
	guid[8] = (guid[8] & 0x3F) | 0x80;
}
EXPORT_SYMBOL(generate_random_guid);
