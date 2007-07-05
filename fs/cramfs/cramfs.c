/*
 * cramfs.c
 *
 * Copyright (C) 1999 Linus Torvalds
 *
 * Copyright (C) 2000-2002 Transmeta Corporation
 *
 * Copyright (C) 2003 Kai-Uwe Bloem,
 * Auerswald GmbH & Co KG, <linux-development@auerswald.de>
 * - adapted from the www.tuxbox.org u-boot tree, added "ls" command
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License (Version 2) as
 * published by the Free Software Foundation.
 *
 * Compressed ROM filesystem for Linux.
 *
 * TODO:
 * add support for resolving symbolic links
 */

/*
 * These are the VFS interfaces to the compressed ROM filesystem.
 * The actual compression is based on zlib, see the other files.
 */

#include <common.h>
#include <malloc.h>
#include <driver.h>
#include <init.h>
#include <asm-generic/errno.h>
#include <fs.h>

#include <asm/byteorder.h>
#include <linux/stat.h>
#include <jffs2/jffs2.h>
#include <jffs2/load_kernel.h>
#include <cramfs/cramfs_fs.h>

/* These two macros may change in future, to provide better st_ino
   semantics. */
#define CRAMINO(x)	(CRAMFS_GET_OFFSET(x) ? CRAMFS_GET_OFFSET(x)<<2 : 1)
#define OFFSET(x)	((x)->i_ino)

struct cramfs_super super;

static int cramfs_read_super (struct device_d *dev)
{
	unsigned long root_offset;

	if (read(dev, &super, sizeof (super), 0, 0) < sizeof (super)) {
		printf("read superblock failed\n");
		return -EINVAL;
	}

	/* Do sanity checks on the superblock */
	if (super.magic != CRAMFS_32 (CRAMFS_MAGIC)) {
		/* check at 512 byte offset */
		if (read(dev, &super, sizeof (super), 512, 0) < sizeof (super)) {
			printf("read superblock failed\n");
			return -EINVAL;
		}
		if (super.magic != CRAMFS_32 (CRAMFS_MAGIC)) {
			printf ("cramfs: wrong magic\n");
			return -1;
		}
	}

	/* flags is reused several times, so swab it once */
	super.flags = CRAMFS_32 (super.flags);
	super.size = CRAMFS_32 (super.size);

	/* get feature flags first */
	if (super.flags & ~CRAMFS_SUPPORTED_FLAGS) {
		printf ("cramfs: unsupported filesystem features\n");
		return -1;
	}

	/* Check that the root inode is in a sane state */
	if (!S_ISDIR (CRAMFS_16 (super.root.mode))) {
		printf ("cramfs: root is not a directory\n");
		return -1;
	}
	root_offset = CRAMFS_GET_OFFSET (&(super.root)) << 2;
	if (root_offset == 0) {
		printf ("cramfs: empty filesystem");
	} else if (!(super.flags & CRAMFS_FLAG_SHIFTED_ROOT_OFFSET) &&
		   ((root_offset != sizeof (struct cramfs_super)) &&
		    (root_offset != 512 + sizeof (struct cramfs_super)))) {
		printf ("cramfs: bad root offset %lu\n", root_offset);
		return -1;
	}

	return 0;
}

static unsigned long cramfs_resolve (unsigned long begin, unsigned long offset,
				     unsigned long size, int raw,
				     char *filename)
{
	unsigned long inodeoffset = 0, nextoffset;

	while (inodeoffset < size) {
		struct cramfs_inode *inode;
		char *name;
		int namelen;

		inode = (struct cramfs_inode *) (begin + offset +
						 inodeoffset);

		/*
		 * Namelengths on disk are shifted by two
		 * and the name padded out to 4-byte boundaries
		 * with zeroes.
		 */
		namelen = CRAMFS_GET_NAMELEN (inode) << 2;
		name = (char *) inode + sizeof (struct cramfs_inode);

		nextoffset =
			inodeoffset + sizeof (struct cramfs_inode) + namelen;

		for (;;) {
			if (!namelen)
				return -1;
			if (name[namelen - 1])
				break;
			namelen--;
		}

		if (!strncmp (filename, name, namelen)) {
			char *p = strtok (NULL, "/");

			if (raw && (p == NULL || *p == '\0'))
				return offset + inodeoffset;

			if (S_ISDIR (CRAMFS_16 (inode->mode))) {
				return cramfs_resolve (begin,
						       CRAMFS_GET_OFFSET
						       (inode) << 2,
						       CRAMFS_24 (inode->
								  size), raw,
						       p);
			} else if (S_ISREG (CRAMFS_16 (inode->mode))) {
				return offset + inodeoffset;
			} else {
				printf ("%*.*s: unsupported file type (%x)\n",
					namelen, namelen, name,
					CRAMFS_16 (inode->mode));
				return 0;
			}
		}

		inodeoffset = nextoffset;
	}

	printf ("can't find corresponding entry\n");
	return 0;
}

static int cramfs_uncompress (unsigned long begin, unsigned long offset,
			      unsigned long loadoffset)
{
	struct cramfs_inode *inode = (struct cramfs_inode *) (begin + offset);
	unsigned long *block_ptrs = (unsigned long *)
		(begin + (CRAMFS_GET_OFFSET (inode) << 2));
	unsigned long curr_block = (CRAMFS_GET_OFFSET (inode) +
				    (((CRAMFS_24 (inode->size)) +
				      4095) >> 12)) << 2;
	int size, total_size = 0;
	int i;

	cramfs_uncompress_init ();

	for (i = 0; i < ((CRAMFS_24 (inode->size) + 4095) >> 12); i++) {
		size = cramfs_uncompress_block ((void *) loadoffset,
						(void *) (begin + curr_block),
						(CRAMFS_32 (block_ptrs[i]) -
						 curr_block));
		if (size < 0)
			return size;
		loadoffset += size;
		total_size += size;
		curr_block = CRAMFS_32 (block_ptrs[i]);
	}

	cramfs_uncompress_exit ();
	return total_size;
}

int cramfs_load (char *loadoffset, struct device_d *dev, const char *filename)
{
	unsigned long offset;
	char *f;
	if (cramfs_read_super (dev))
		return -1;

	f = strdup(filename);
	offset = cramfs_resolve (dev->map_base,
				 CRAMFS_GET_OFFSET (&(super.root)) << 2,
				 CRAMFS_24 (super.root.size), 0,
				 strtok (f, "/"));

	free(f);

	if (offset <= 0)
		return offset;

	return cramfs_uncompress (dev->map_base, offset,
				  (unsigned long) loadoffset);
}

static int cramfs_list_inode (struct device_d *dev, unsigned long offset)
{
	struct cramfs_inode *inode = (struct cramfs_inode *)
		(dev->map_base + offset);
	char *name, str[20];
	int namelen, nextoff;

	/*
	 * Namelengths on disk are shifted by two
	 * and the name padded out to 4-byte boundaries
	 * with zeroes.
	 */
	namelen = CRAMFS_GET_NAMELEN (inode) << 2;
	name = (char *) inode + sizeof (struct cramfs_inode);
	nextoff = namelen;

	for (;;) {
		if (!namelen)
			return namelen;
		if (name[namelen - 1])
			break;
		namelen--;
	}

	printf (" %s %8d %*.*s", mkmodestr (CRAMFS_16 (inode->mode), str),
		CRAMFS_24 (inode->size), namelen, namelen, name);

	if ((CRAMFS_16 (inode->mode) & S_IFMT) == S_IFLNK) {
		/* symbolic link.
		 * Unpack the link target, trusting in the inode's size field.
		 */
		unsigned long size = CRAMFS_24 (inode->size);
		char *link = malloc (size);

		if (link != NULL && cramfs_uncompress (dev->map_base, offset,
						       (unsigned long) link)
		    == size)
			printf (" -> %*.*s\n", (int) size, (int) size, link);
		else
			printf (" [Error reading link]\n");
		if (link)
			free (link);
	} else
		printf ("\n");

	return nextoff;
}

int cramfs_ls (struct device_d *dev, const char *filename)
{
	struct cramfs_inode *inode;
	unsigned long inodeoffset = 0, nextoffset;
	unsigned long offset, size;
	char *f;

	printf("cramfs_ls: %s\n", filename);

	if (cramfs_read_super (dev))
		return -1;

	if (strlen (filename) == 0 || !strcmp (filename, "/")) {
		/* Root directory. Use root inode in super block */
		offset = CRAMFS_GET_OFFSET (&(super.root)) << 2;
		size = CRAMFS_24 (super.root.size);
	} else {
		f = strdup(filename);
		/* Resolve the path */
		offset = cramfs_resolve (dev->map_base,
					 CRAMFS_GET_OFFSET (&(super.root)) <<
					 2, CRAMFS_24 (super.root.size), 1,
					 strtok (f, "/"));
		free(f);
		if (offset <= 0)
			return offset;

		/* Resolving was successful. Examine the inode */
		inode = (struct cramfs_inode *) (dev->map_base + offset);
		if (!S_ISDIR (CRAMFS_16 (inode->mode))) {
			/* It's not a directory - list it, and that's that */
			return (cramfs_list_inode (dev, offset) > 0);
		}

		/* It's a directory. List files within */
		offset = CRAMFS_GET_OFFSET (inode) << 2;
		size = CRAMFS_24 (inode->size);
	}

	/* List the given directory */
	while (inodeoffset < size) {
		inode = (struct cramfs_inode *) (dev->map_base + offset +
						 inodeoffset);

		nextoffset = cramfs_list_inode (dev, offset + inodeoffset);
		if (nextoffset == 0)
			break;
		inodeoffset += sizeof (struct cramfs_inode) + nextoffset;
	}

	return 0;
}

int cramfs_info (struct device_d *dev)
{
	if (cramfs_read_super (dev))
		return 0;

	printf ("size: 0x%x (%u)\n", super.size, super.size);

	if (super.flags != 0) {
		printf ("flags:\n");
		if (super.flags & CRAMFS_FLAG_FSID_VERSION_2)
			printf ("\tFSID version 2\n");
		if (super.flags & CRAMFS_FLAG_SORTED_DIRS)
			printf ("\tsorted dirs\n");
		if (super.flags & CRAMFS_FLAG_HOLES)
			printf ("\tholes\n");
		if (super.flags & CRAMFS_FLAG_SHIFTED_ROOT_OFFSET)
			printf ("\tshifted root offset\n");
	}

	printf ("fsid:\n\tcrc: 0x%x\n\tedition: 0x%x\n",
		super.fsid.crc, super.fsid.edition);
	printf ("name: %16s\n", super.name);

	return 1;
}

int cramfs_probe(struct device_d *dev)
{
	if (cramfs_read_super (dev)) {
		printf("no valid cramfs found on %s\n",dev->id);
		return -EINVAL;
	}

	return 0;
}

static struct fs_driver_d cramfs_driver = {
	.type  = FS_TYPE_CRAMFS,
	.probe = cramfs_probe,
	.ls    = cramfs_ls,
	.drv = {
		.type = DEVICE_TYPE_FS,
		.name = "cramfs",
		.driver_data = &cramfs_driver,
	}
};

int cramfs_init(void)
{
	return register_fs_driver(&cramfs_driver);
}

device_initcall(cramfs_init);
