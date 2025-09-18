// SPDX-License-Identifier: GPL-2.0-only
// SPDX-FileCopyrightText: © 2025 Tobias Waldekranz <tobias@waldekranz.com>, Wires
/*
 * Based on dm-verity from Linux:
 *
 * Copyright (C) 2012 Red Hat, Inc.
 *
 * Author: Mikulas Patocka <mpatocka@redhat.com>
 *
 * Based on Chromium dm-verity driver (C) 2011 The Chromium OS Authors
 */

#include <block.h>
#include <device-mapper.h>
#include <digest.h>
#include <disks.h>
#include <fcntl.h>
#include <xfuncs.h>
#include <unistd.h>

#include <linux/bitmap.h>
#include <linux/bitops.h>
#include <linux/hex.h>
#include <linux/kstrtox.h>

#include "dm-target.h"

#define DM_VERITY_MAX_LEVELS 63

struct dm_verity {
	struct dm_cdev ddev;
	struct dm_cdev hdev;

	struct digest *digest_algo;
	size_t digest_len;

	u8 *root_digest;
	u8 *salt;
	size_t salt_size;

	u8 levels;
	u8 hash_per_block_bits;
	sector_t hash_level_block[DM_VERITY_MAX_LEVELS];

	struct {
		unsigned long *trusted;
		u8 *digest;

		struct {
			u8 *data;
			sector_t block;
		} hblock;
	} verify;
};

static sector_t dm_verity_position_at_level(struct dm_verity *v, sector_t dblock,
					    int level)
{
	return dblock >> (level * v->hash_per_block_bits);
}

static void dm_verity_hash_at_level(struct dm_verity *v, sector_t dblock, int level,
				    sector_t *hblock, unsigned int *offset)
{
	sector_t position = dm_verity_position_at_level(v, dblock, level);
	unsigned int idx;

	*hblock = v->hash_level_block[level] + (position >> v->hash_per_block_bits);

	if (!offset)
		return;

	idx = position & ((1 << v->hash_per_block_bits) - 1);
	*offset = idx << (v->hdev.blk.bits - v->hash_per_block_bits);
}

static int dm_verity_set_digest(struct dm_verity *v, const void *buf, size_t buflen)
{
	int err;

	err = digest_init(v->digest_algo);
	err = err ? : digest_update(v->digest_algo, v->salt, v->salt_size);
	err = err ? : digest_update(v->digest_algo, buf, buflen);
	err = err ? : digest_final(v->digest_algo, v->verify.digest);
	return err;
}

static int dm_verity_set_hblock(struct dm_verity *v, sector_t hblock)
{
	int err;

	if (v->verify.hblock.block == hblock)
		/* Requested block is already loaded. This is the
		 * common scenario for sequential block checking once
		 * the upper levels of hash blocks have been marked as
		 * trusted.
		 */
		return 0;

	err = dm_cdev_read(&v->hdev, v->verify.hblock.data, hblock, 1);
	if (err)
		return err;

	v->verify.hblock.block = hblock;
	return 0;
}

static int dm_verity_verify(struct dm_target *ti, const void *buf, sector_t dblock)
{
	struct dm_verity *v = ti->private;
	const u8 *expected;
	unsigned int hoffs;
	sector_t hblock;
	int err, level;

	err = dm_verity_set_digest(v, buf, 1 << v->ddev.blk.bits);
	if (err)
		return err;

	for (level = 0; level < v->levels; level++) {
		dm_verity_hash_at_level(v, dblock, level, &hblock, &hoffs);

		err = dm_verity_set_hblock(v, hblock);
		if (err)
			return err;

		expected = v->verify.hblock.data + hoffs;

		if (memcmp(v->verify.digest, expected, v->digest_len)) {
			dm_target_err_once(
				ti, "Verity error for data block %llu at level %d\n",
				dblock, level);
			return -EINVAL;
		}

		if (test_bit(hblock, v->verify.trusted)) {
			/* This hash block has already been validated
			 * all the way up to the root digest by an
			 * earlier operation, we do not need to ascend
			 * any further.
			 *
			 * Make sure to mark any subset of branches
			 * that this operation might have verified.
			 */
			goto mark_as_trusted;
		}

		/* Current level OK. Now calculate the digest for the
		 * entire hblock, which then becomes the input when
		 * checking the next level up.
		 */
		err = dm_verity_set_digest(v, v->verify.hblock.data,
					   1 << v->hdev.blk.bits);
		if (err)
			return err;
	}

	/* Data is consistent with hash tree. Now make sure that the top
	 * level matches the externally provided root digest.
	 */
	if (memcmp(v->verify.digest, v->root_digest, v->digest_len)) {
		dm_target_err_once(ti, "Verity error for data block %llu at root\n",
				   dblock);
		return -EINVAL;

	}

mark_as_trusted:
	/* All hash blocks in the chain from dblock to the root digest
	 * are valid. Cache this knowledge for subsequent operations
	 * that map to the same subtree.
	 */
	for (level--; level >= 0; level--) {
		dm_verity_hash_at_level(v, dblock, level, &hblock, NULL);
		set_bit(hblock, v->verify.trusted);
	}

	return 0;
}

static int dm_verity_verify_range(struct dm_target *ti, const void *buf,
				  sector_t block, blkcnt_t num_blocks)
{
	struct dm_verity *v = ti->private;
	int err;

	for (; num_blocks; block++, num_blocks--, buf += 1 << v->ddev.blk.bits) {
		err = dm_verity_verify(ti, buf, block);
		if (err)
			return err;
	}

	return 0;
}

static int dm_verity_read(struct dm_target *ti, void *in_buf,
			  sector_t block, blkcnt_t num_blocks)
{
	struct dm_verity *v = ti->private;
	blkcnt_t pre_blocks, post_blocks, dblocks;
	void *buf = in_buf;
	sector_t dblock;
	int err;

	/* The dm-verity data block size is guaranteed to be at least
	 * 512B, but typically larger. Make sure we align the request
	 * to even data block size boundaries, since those are the
	 * chunks we need to hash.
	 */
	pre_blocks = block & v->ddev.blk.mask;
	post_blocks = (pre_blocks + num_blocks) & v->ddev.blk.mask;
	if (post_blocks)
		post_blocks = v->ddev.blk.mask + 1 - post_blocks;

	if (pre_blocks || post_blocks) {
		/* Need to read more data than in_buf will hold. */
		buf = malloc((pre_blocks + num_blocks + post_blocks) << SECTOR_SHIFT);
		if (!buf)
			return -ENOMEM;
	}

	dblock = block - pre_blocks;
	dblock >>= (v->ddev.blk.bits - SECTOR_SHIFT);

	dblocks = pre_blocks + num_blocks + post_blocks;
	dblocks >>= (v->ddev.blk.bits - SECTOR_SHIFT);

	err = dm_cdev_read(&v->ddev, buf, dblock, dblocks);
	if (err)
		goto err;

	err = dm_verity_verify_range(ti, buf, dblock, dblocks);
	if (err)
		goto err;

	if (buf != in_buf) {
		memcpy(in_buf, buf + (pre_blocks << SECTOR_SHIFT),
		       num_blocks << SECTOR_SHIFT);
		free(buf);
	}

	return 0;

err:
	if (buf != in_buf)
		free(buf);

	return err;
}

static int dm_verity_measure(struct dm_target *ti)
{
	struct dm_verity *v = ti->private;
	sector_t lstart, lsize;
	unsigned int lshift;
	size_t minsize;
	int i;

	v->hash_per_block_bits =
		fls((1 << v->hdev.blk.bits) / v->digest_len) - 1;

	v->levels = 0;
	if (v->ddev.blk.num)
		while (v->hash_per_block_bits * v->levels < 64 &&
		       (unsigned long long)(v->ddev.blk.num - 1) >>
		       (v->hash_per_block_bits * v->levels))
			v->levels++;

	if (v->levels > DM_VERITY_MAX_LEVELS) {
		dm_target_err(ti, "Too many tree levels\n");
		return -E2BIG;
	}

	for (lstart = 0, i = v->levels - 1; i >= 0; i--) {
		v->hash_level_block[i] = lstart;

		lshift = (i + 1) * v->hash_per_block_bits;
		lsize = (v->ddev.blk.num + ((sector_t)1 << lshift) - 1) >> lshift;
		if (lstart + lsize < lstart) {
			dm_target_err(ti, "Hash device offset overflow\n");
			return -E2BIG;
		}
		lstart += lsize;
	}
	v->hdev.blk.num = lstart;

	minsize = (v->hdev.blk.start + v->hdev.blk.num) << v->hdev.blk.bits;
	if (minsize > v->hdev.cdev->size) {
		dm_target_err(ti, "Hash device is too small to fit tree\n");
		return -E2BIG;
	}

	return 0;
}

static int dm_verity_cdev_init(struct dm_target *ti, struct dm_cdev *dmcdev,
			       const char *devstr, const char *blkszstr,
			       const char *num_blkstr, const char *start_blkstr)
{
	struct dm_verity *v = ti->private;
	const char *kind = (dmcdev == &v->ddev) ? "data" : "hash";
	unsigned long blocksize;
	blkcnt_t num_blocks = 0;
	sector_t start = 0;
	char *errmsg;
	int err;

	if (kstrtoul(blkszstr, 0, &blocksize)) {
		dm_target_err(ti, "Invalid %s block size: \"%s\"\n", kind, blkszstr);
		return -EINVAL;
	}

	if (num_blkstr && kstrtoull(num_blkstr, 0, &num_blocks)) {
		dm_target_err(ti, "Invalid # of %s blocks: \"%s\"\n", kind, num_blkstr);
		return -EINVAL;
	}

	if (start_blkstr && kstrtoull(start_blkstr, 0, &start)) {
		dm_target_err(ti, "Invalid start %s block: \"%s\"\n", kind, start_blkstr);
		return -EINVAL;
	}

	err = dm_cdev_open(dmcdev, devstr, O_RDONLY,
			   start, num_blocks, blocksize, &errmsg);
	if (err) {
		dm_target_err(ti, "Error opening %d device: %s\n", kind, errmsg);
		free(errmsg);
	}

	return err;
}

static int dm_verity_create(struct dm_target *ti, unsigned int argc, char **argv)
{
	struct dm_verity *v;
	unsigned int ver;
	int err;

	if (argc != 10) {
		dm_target_err(ti, "Expected 10 arguments, got %u\n", argc);
		return -EINVAL;
	}

	if (kstrtouint(argv[0], 0, &ver) || ver != 1) {
		dm_target_err(ti, "Only version 1 is supported, not \"%s\"\n", argv[0]);
		return -EINVAL;
	}

	v = xzalloc(sizeof(*v));
	ti->private = v;

	err = dm_verity_cdev_init(ti, &v->ddev, argv[1], argv[3], argv[5], NULL);
	if (err)
		goto err;

	err = dm_verity_cdev_init(ti, &v->hdev, argv[2], argv[4], NULL, argv[6]);
	if (err)
		goto err;

	v->digest_algo = digest_alloc(argv[7]);
	if (!v->digest_algo) {
		dm_target_err(ti, "Unknown digest \"%s\"\n", argv[7]);
		err = -EINVAL;
		goto err;
	}

	v->digest_len = digest_length(v->digest_algo);
	if ((1 << v->hdev.blk.bits) < v->digest_len * 2) {
		dm_target_err(ti, "Digest size too big\n");
		err = -EINVAL;
		goto err;
	}

	v->root_digest = xmalloc(v->digest_len);
	if (strlen(argv[8]) != v->digest_len * 2 ||
	    hex2bin(v->root_digest, argv[8], v->digest_len)) {
		dm_target_err(ti, "Invalid root digest \"%s\"\n", argv[8]);
		err = -EINVAL;
		goto err;
	}

	if (strcmp(argv[9], "-")) {
		v->salt_size = strlen(argv[9]) / 2;
		v->salt = xmalloc(v->salt_size);

		if (strlen(argv[9]) != v->salt_size * 2 ||
		    hex2bin(v->salt, argv[9], v->salt_size)) {
			dm_target_err(ti, "Invalid salt \"%s\"\n", argv[9]);
			err = -EINVAL;
			goto err;
		}
	}

	err = dm_verity_measure(ti);
	if (err)
		goto err;

	/* Initialize this to a value larger than the largest possible
	 * hash block lba to make sure that the first hash block read
	 * in dm_verity_verify() always misses the cache.
	 */
	v->verify.hblock.block = v->hdev.blk.num;
	v->verify.hblock.data = xmalloc(1 << v->hdev.blk.bits);

	v->verify.digest = xmalloc(v->digest_len);
	v->verify.trusted = bitmap_xzalloc(v->hdev.blk.num);
	return 0;

err:
	if (v->salt)
		free(v->salt);
	if (v->root_digest)
		free(v->root_digest);
	if (v->digest_algo)
		digest_free(v->digest_algo);
	if (v->hdev.cdev)
		cdev_close(v->hdev.cdev);
	if (v->ddev.cdev)
		cdev_close(v->ddev.cdev);

	free(v);
	return err;
}

static int dm_verity_destroy(struct dm_target *ti)
{
	struct dm_verity *v = ti->private;

	free(v->verify.digest);
	free(v->verify.hblock.data);
	free(v->verify.trusted);
	free(v->salt);
	free(v->root_digest);
	digest_free(v->digest_algo);
	dm_cdev_close(&v->hdev);
	dm_cdev_close(&v->ddev);

	free(v);
	return 0;
}

static char *dm_verity_asprint(struct dm_target *ti)
{
	struct dm_verity *v = ti->private;

	return xasprintf("data:%s hash:%s@%llu root:%*phN",
			 cdev_name(v->ddev.cdev),
			 cdev_name(v->hdev.cdev), v->hdev.blk.start,
			 (int)v->digest_len, v->root_digest);
}

static struct dm_target_ops dm_verity_ops = {
	.name = "verity",
	.asprint = dm_verity_asprint,
	.create = dm_verity_create,
	.destroy = dm_verity_destroy,
	.read = dm_verity_read,
};

static int __init dm_verity_init(void)
{
	return dm_target_register(&dm_verity_ops);
}
device_initcall(dm_verity_init);

struct dm_verity_sb {
	    u8 signature[8];    /* "verity\0\0" */
	__le32 version;         /* superblock version, 1 */
	__le32 hash_type;       /* 0 - Chrome OS, 1 - normal */
	    u8 uuid[16];        /* UUID of hash device */
	    u8 algorithm[32];   /* hash algorithm name */
	__le32 data_block_size; /* data block in bytes */
	__le32 hash_block_size; /* hash block in bytes */
	__le64 data_blocks;     /* number of data blocks */
	__le16 salt_size;       /* salt size */
	    u8 _pad1[6];
	    u8 salt[256];       /* salt */
	    u8 _pad2[168];
} __packed;

char *dm_verity_config_from_sb(const char *data_dev, const char *hash_dev,
			       const char *root_hash)
{
	struct dm_verity_sb sb;
	blkcnt_t sects;
	ssize_t n;
	int fd;

	fd = open(hash_dev, O_RDONLY);
	if (fd < 0)
		return ERR_PTR(-ENOENT);

	n = read(fd, &sb, sizeof(sb));
	close(fd);
	if (n != sizeof(sb))
		return ERR_PTR((n < 0) ? n : -EIO);

	if (memcmp(sb.signature, "verity\0\0", sizeof(sb.signature)))
		return ERR_PTR(-EINVAL);

	if (le32_to_cpu(sb.version) != 1)
		return ERR_PTR(-ENOTSUPP);

	sects = le32_to_cpu(sb.data_block_size) >> SECTOR_SHIFT;
	if (!sects)
		return ERR_PTR(-ERANGE);

	sects *= le64_to_cpu(sb.data_blocks);

	return xasprintf("0 %llu verity %u %s %s %u %u %llu 1 %s %s %*phN",
			 sects, le32_to_cpu(sb.hash_type), data_dev, hash_dev,
			 le32_to_cpu(sb.data_block_size),
			 le32_to_cpu(sb.hash_block_size),
			 le64_to_cpu(sb.data_blocks), sb.algorithm, root_hash,
			 le16_to_cpu(sb.salt_size), sb.salt);
}
EXPORT_SYMBOL(dm_verity_config_from_sb);
