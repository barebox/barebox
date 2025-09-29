/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __DM_H
#define __DM_H

struct dm_device;

struct dm_device *dm_find_by_name(const char *name);
int dm_foreach(int (*cb)(struct dm_device *dm, void *ctx), void *ctx);

char *dm_asprint(struct dm_device *dm);

void dm_destroy(struct dm_device *dm);
struct dm_device *dm_create(const char *name, const char *ctable);

#if defined(CONFIG_DM_BLK_VERITY)
char *dm_verity_config_from_sb(const char *data_dev, const char *hash_dev,
			       const char *root_hash);
#endif

#endif /* __DM_H */
