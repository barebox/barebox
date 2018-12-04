/*
 * Copyright (C) 2011 Sascha Hauer, Pengutronix
 * Copyright (C) 2012 Jean-Christophe PLAGNIOL-VILLARD <plagnio@jcrosoft.com>
 *
 * Under GPLv2
 */

#include <common.h>
#include <fs.h>
#include <fcntl.h>
#include <linux/sizes.h>
#include <errno.h>
#include <malloc.h>
#include <libfile.h>
#include <bootstrap.h>

void* bootstrap_read_disk(const char *dev, const char *fstype)
{
	int ret;
	void *buf;
	size_t len;
	const char *path = "/";

	ret = mount(dev, fstype, path, NULL);
	if (ret) {
		bootstrap_err("mounting %s failed with %d\n", dev, ret);
		return NULL;
	}

	buf = read_file("/barebox.bin", &len);
	if (!buf) {
		bootstrap_err("could not read barebox.bin from %s\n", dev);
		umount(path);
		return NULL;
	}

	return buf;
}
