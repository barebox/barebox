/* SPDX-License-Identifier: GPL-2.0-only */
/* SPDX-FileCopyrightText: © 2025 Tobias Waldekranz <tobias@waldekranz.com>, Wires */

#ifndef __DM_TARGET_H
#define __DM_TARGET_H

struct dm_cdev {
	struct cdev *cdev;
	bool loop;

	struct {
		sector_t start;
		blkcnt_t num;
		u32 mask;
		u8 bits;
	} blk;
};

int dm_cdev_read(struct dm_cdev *dmc, void *buf, sector_t block,
		 blkcnt_t num_blocks);
int dm_cdev_write(struct dm_cdev *dmc, const void *buf, sector_t block,
		  blkcnt_t num_blocks);
int dm_cdev_open(struct dm_cdev *dmcdev, const char *path, ulong flags,
		 sector_t start, blkcnt_t num_blocks, size_t blocksize, char **errmsg);
void dm_cdev_close(struct dm_cdev *dmcdev);

struct dm_device;
struct dm_target_ops;

struct dm_target {
	struct dm_device *dm;
	struct list_head list;

	sector_t base;
	blkcnt_t size;

	const struct dm_target_ops *ops;
	void *private;
};

void dm_target_err(struct dm_target *ti, const char *fmt, ...);

struct dm_target_ops {
	struct list_head list;
	const char *name;

	char *(*asprint)(struct dm_target *ti);
	int (*create)(struct dm_target *ti, unsigned int argc, char **argv);
	int (*destroy)(struct dm_target *ti);
	int (*read)(struct dm_target *ti, void *buf,
		    sector_t block, blkcnt_t num_blocks);
	int (*write)(struct dm_target *ti, const void *buf,
		     sector_t block, blkcnt_t num_blocks);
};

int dm_target_register(struct dm_target_ops *ops);
void dm_target_unregister(struct dm_target_ops *ops);

#endif	/* __DM_TARGET_H */
