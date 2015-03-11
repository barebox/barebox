/*
 * (C) Copyright 2008-2010 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of
 * the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include <digest.h>
#include <malloc.h>
#include <fs.h>
#include <fcntl.h>
#include <linux/stat.h>
#include <errno.h>
#include <module.h>
#include <linux/err.h>

static LIST_HEAD(digests);

static int dummy_init(struct digest *d)
{
	return 0;
}

int digest_register(struct digest *d)
{
	if (!d || !d->name || !d->update || !d->final || d->length < 1)
		return -EINVAL;

	if (!d->init)
		d->init = dummy_init;

	if (digest_get_by_name(d->name))
		return -EEXIST;

	list_add_tail(&d->list, &digests);

	return 0;
}
EXPORT_SYMBOL(digest_register);

void digest_unregister(struct digest *d)
{
	if (!d)
		return;

	list_del(&d->list);
}
EXPORT_SYMBOL(digest_unregister);

struct digest* digest_get_by_name(char* name)
{
	struct digest* d;

	if (!name)
		return NULL;

	list_for_each_entry(d, &digests, list) {
		if(strcmp(d->name, name) == 0)
			return d;
	}

	return NULL;
}
EXPORT_SYMBOL_GPL(digest_get_by_name);

int digest_file_window(struct digest *d, char *filename,
		       unsigned char *hash,
		       ulong start, ulong size)
{
	ulong len = 0;
	int fd, now, ret = 0;
	unsigned char *buf;
	int flags = 0;

	d->init(d);

	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		perror(filename);
		return fd;
	}

	buf = memmap(fd, PROT_READ);
	if (buf == (void *)-1) {
		buf = xmalloc(4096);
		flags = 1;
	}

	if (start > 0) {
		if (flags) {
			ret = lseek(fd, start, SEEK_SET);
			if (ret == -1) {
				perror("lseek");
				goto out;
			}
		} else {
			buf += start;
		}
	}

	while (size) {
		now = min((ulong)4096, size);
		if (flags) {
			now = read(fd, buf, now);
			if (now < 0) {
				ret = now;
				perror("read");
				goto out_free;
			}
			if (!now)
				break;
		}

		if (ctrlc()) {
			ret = -EINTR;
			goto out_free;
		}

		d->update(d, buf, now);
		size -= now;
		len += now;
	}

	d->final(d, hash);

out_free:
	if (flags)
		free(buf);
out:
	close(fd);

	return ret;
}
EXPORT_SYMBOL_GPL(digest_file_window);

int digest_file(struct digest *d, char *filename,
		       unsigned char *hash)
{
	struct stat st;
	int ret;

	ret = stat(filename, &st);

	if (ret < 0)
		return ret;

	return digest_file_window(d, filename, hash, 0, st.st_size);
}
EXPORT_SYMBOL_GPL(digest_file);

int digest_file_by_name(char *algo, char *filename,
		       unsigned char *hash)
{
	struct digest *d;

	d = digest_get_by_name(algo);
	if (!d)
		return -EIO;

	return digest_file(d, filename, hash);
}
EXPORT_SYMBOL_GPL(digest_file_by_name);
