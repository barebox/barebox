// SPDX-License-Identifier: GPL-2.0-only
/*
 * Fuzz a file system driver: mount the input and read what it describes
 */

#include <common.h>
#include <block.h>
#include <fcntl.h>
#include <fs.h>
#include <fuzz.h>
#include <libfile.h>
#include <xfuncs.h>
#include <linux/stat.h>

#define FUZZ_FS_MNT	"/mnt/fuzz"

/* Bound the work per input: a crafted image can describe any tree */
static int fuzz_fs_budget;

static void fuzz_fs_read(const char *path, loff_t size)
{
	char buf[4096];
	int fd, i;

	fd = open(path, O_RDONLY);
	if (fd < 0)
		return;

	/*
	 * Read the start of the file, then its end, which for a large file
	 * goes through the deeper levels of the block lookup, like indirect
	 * blocks, extent tree nodes or block list indexes.
	 */
	for (i = 0; i < 64; i++)
		if (read(fd, buf, sizeof(buf)) < (ssize_t)sizeof(buf))
			break;

	if (size > (loff_t)sizeof(buf) &&
	    lseek(fd, size - sizeof(buf), SEEK_SET) >= 0)
		read(fd, buf, sizeof(buf));

	close(fd);
}

static void fuzz_fs_walk(const char *path, int depth)
{
	char buf[PATH_MAX];
	struct dirent *d;
	struct stat s;
	DIR *dir;

	dir = opendir(path);
	if (!dir)
		return;

	while (fuzz_fs_budget > 0 && (d = readdir(dir))) {
		char *p;

		/* Entry names come from the image: don't leave the mount */
		if (!strcmp(d->d_name, ".") || !strcmp(d->d_name, "..") ||
		    strchr(d->d_name, '/'))
			continue;

		fuzz_fs_budget--;
		p = basprintf("%s/%s", path, d->d_name);

		if (!lstat(p, &s)) {
			if (S_ISDIR(s.st_mode)) {
				if (depth < 8)
					fuzz_fs_walk(p, depth + 1);
			} else if (S_ISLNK(s.st_mode)) {
				readlink(p, buf, sizeof(buf));
			} else {
				fuzz_fs_read(p, s.st_size);
			}
		}

		free(p);
	}

	closedir(dir);
}

int fuzz_fs(struct block_device *blk, const char *fstype)
{
	char *dev = basprintf("/dev/%s", cdev_name(&blk->cdev));

	if (!mount(dev, fstype, FUZZ_FS_MNT, NULL)) {
		fuzz_fs_budget = 256;
		fuzz_fs_walk(FUZZ_FS_MNT, 0);
		BUG_ON(umount(FUZZ_FS_MNT));
	}

	free(dev);

	return 0;
}

void fuzz_fs_init(void)
{
	make_directory(FUZZ_FS_MNT);
}
