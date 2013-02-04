/*
 * Copyright (C) 2011 Sascha Hauer, Pengutronix
 * Copyright (C) 2012 Jean-Christophe PLAGNIOL-VILLARD <plagnio@jcrosoft.com>
 *
 * Under GPLv2
 */

#include <common.h>
#include <partition.h>
#include <nand.h>
#include <driver.h>
#include <linux/mtd/mtd.h>
#include <fcntl.h>
#include <filetype.h>
#include <sizes.h>
#include <errno.h>
#include <malloc.h>
#include <bootstrap.h>

#if defined(CONFIG_ARM) || defined(CONFIG_MIPS)
#if defined(CONFIG_ARM)
#define BAREBOX_HEAD_SIZE		ARM_HEAD_SIZE
#define BAREBOX_HEAD_SIZE_OFFSET	ARM_HEAD_SIZE_OFFSET
#elif defined(CONFIG_MIPS)
#define BAREBOX_HEAD_SIZE		MIPS_HEAD_SIZE
#define BAREBOX_HEAD_SIZE_OFFSET	MIPS_HEAD_SIZE_OFFSET
#endif

static void *read_image_head(const char *name)
{
	void *header = xmalloc(BAREBOX_HEAD_SIZE);
	struct cdev *cdev;
	int ret;

	cdev = cdev_open(name, O_RDONLY);
	if (!cdev) {
		bootstrap_err("failed to open partition\n");
		return NULL;
	}

	ret = cdev_read(cdev, header, BAREBOX_HEAD_SIZE, 0, 0);
	cdev_close(cdev);

	if (ret != BAREBOX_HEAD_SIZE) {
		bootstrap_err("failed to read from partition\n");
		return NULL;
	}

	return header;
}

static unsigned int get_image_size(void *head)
{
	unsigned int ret = 0;
	unsigned int *psize = head + BAREBOX_HEAD_SIZE_OFFSET;

	if (is_barebox_head(head))
		ret = *psize;
	debug("Detected barebox image size %u\n", ret);

	return ret;
}
#else
static void *read_image_head(const char *name)
{
	return NULL;
}

static unsigned int get_image_size(void *head)
{
	return 0;
}
#endif

void* bootstrap_read_devfs(char *devname, bool use_bb, int offset,
			   int default_size, int max_size)
{
	int ret;
	int size = 0;
	void *to, *header;
	struct cdev *cdev;
	char *partname = "x";

	devfs_add_partition(devname, offset, max_size, DEVFS_PARTITION_FIXED, partname);
	if (use_bb) {
		dev_add_bb_dev(partname, "bbx");
		partname = "bbx";
	}

	header = read_image_head(partname);
	if (header) {
		size = get_image_size(header);
		if (!size)
			bootstrap_err("%s: failed to get image size\n", devname);
	}

	if (!size) {
		size = default_size;
		bootstrap_err("%s: failed to detect barebox and it's image size so use %d\n",
			 devname, size);
	}

	to = xmalloc(size);

	cdev = cdev_open(partname, O_RDONLY);
	if (!cdev) {
		bootstrap_err("%s: failed to open %s\n", devname, partname);
		return NULL;
	}

	ret = cdev_read(cdev, to, size, 0, 0);
	if (ret != size) {
		bootstrap_err("%s: failed to read from %s\n", devname, partname);
		return NULL;
	}

	return to;
}
