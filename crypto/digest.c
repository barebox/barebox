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
#include <crypto/internal.h>

static LIST_HEAD(digests);

static struct digest_algo *digest_algo_get_by_name(const char *name);

static int dummy_init(struct digest *d)
{
	return 0;
}

static void dummy_free(struct digest *d) {}

int digest_generic_verify(struct digest *d, const unsigned char *md)
{
	int ret;
	int len = digest_length(d);
	unsigned char *tmp;

	tmp = xmalloc(len);

	ret = digest_final(d, tmp);
	if (ret)
		goto end;

	ret = memcmp(md, tmp, len);
	ret = ret ? -EINVAL : 0;
end:
	free(tmp);
	return ret;
}

int digest_generic_digest(struct digest *d, const void *data,
			  unsigned int len, u8 *md)

{
	int ret;

	if (!data || len == 0 || !md)
		return -EINVAL;

	ret = digest_init(d);
	if (ret)
		return ret;
	ret = digest_update(d, data, len);
	if (ret)
		return ret;
	return digest_final(d, md);
}

int digest_algo_register(struct digest_algo *d)
{
	if (!d || !d->base.name || !d->update || !d->final || !d->verify)
		return -EINVAL;

	if (!d->init)
		d->init = dummy_init;

	if (!d->alloc)
		d->alloc = dummy_init;

	if (!d->free)
		d->free = dummy_free;

	list_add_tail(&d->list, &digests);

	return 0;
}
EXPORT_SYMBOL(digest_algo_register);

void digest_algo_unregister(struct digest_algo *d)
{
	if (!d)
		return;

	list_del(&d->list);
}
EXPORT_SYMBOL(digest_algo_unregister);

static struct digest_algo *digest_algo_get_by_name(const char *name)
{
	struct digest_algo *d = NULL;
	struct digest_algo *tmp;
	int priority = -1;

	if (!name)
		return NULL;

	list_for_each_entry(tmp, &digests, list) {
		if (strcmp(tmp->base.name, name) != 0)
			continue;

		if (tmp->base.priority <= priority)
			continue;

		d = tmp;
		priority = tmp->base.priority;
	}

	return d;
}

static struct digest_algo *digest_algo_get_by_algo(enum hash_algo algo)
{
	struct digest_algo *d = NULL;
	struct digest_algo *tmp;
	int priority = -1;

	list_for_each_entry(tmp, &digests, list) {
		if (tmp->base.algo != algo)
			continue;

		if (tmp->base.priority <= priority)
			continue;

		d = tmp;
		priority = tmp->base.priority;
	}

	return d;
}

void digest_algo_prints(const char *prefix)
{
	struct digest_algo* d;

	printf("%s%-15s\t%-20s\t%-15s\n", prefix, "name", "driver", "priority");
	printf("%s--------------------------------------------------\n", prefix);
	list_for_each_entry(d, &digests, list) {
		printf("%s%-15s\t%-20s\t%d\n", prefix, d->base.name,
			d->base.driver_name, d->base.priority);
	}
}

struct digest *digest_alloc(const char *name)
{
	struct digest *d;
	struct digest_algo *algo;

	algo = digest_algo_get_by_name(name);
	if (!algo)
		return NULL;

	d = xzalloc(sizeof(*d));
	d->algo = algo;
	d->ctx = xzalloc(algo->ctx_length);
	if (d->algo->alloc(d)) {
		digest_free(d);
		return NULL;
	}

	return d;
}
EXPORT_SYMBOL_GPL(digest_alloc);

struct digest *digest_alloc_by_algo(enum hash_algo hash_algo)
{
	struct digest *d;
	struct digest_algo *algo;

	algo = digest_algo_get_by_algo(hash_algo);
	if (!algo)
		return NULL;

	d = xzalloc(sizeof(*d));
	d->algo = algo;
	d->ctx = xzalloc(algo->ctx_length);
	if (d->algo->alloc(d)) {
		digest_free(d);
		return NULL;
	}

	return d;
}
EXPORT_SYMBOL_GPL(digest_alloc_by_algo);

void digest_free(struct digest *d)
{
	if (!d)
		return;
	d->algo->free(d);
	free(d->ctx);
	free(d);
}
EXPORT_SYMBOL_GPL(digest_free);

int digest_file_window(struct digest *d, const char *filename,
		       unsigned char *hash,
		       const unsigned char *sig,
		       ulong start, ulong size)
{
	ulong len = 0;
	int fd, now, ret = 0;
	unsigned char *buf;
	int flags = 0;

	ret = digest_init(d);
	if (ret)
		return ret;

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

		ret = digest_update(d, buf, now);
		if (ret)
			goto out_free;
		size -= now;
		len += now;
	}

	if (sig)
		ret = digest_verify(d, sig);
	else
		ret = digest_final(d, hash);

out_free:
	if (flags)
		free(buf);
out:
	close(fd);

	return ret;
}
EXPORT_SYMBOL_GPL(digest_file_window);

int digest_file(struct digest *d, const char *filename,
		unsigned char *hash,
		const unsigned char *sig)
{
	struct stat st;
	int ret;

	ret = stat(filename, &st);

	if (ret < 0)
		return ret;

	return digest_file_window(d, filename, hash, sig, 0, st.st_size);
}
EXPORT_SYMBOL_GPL(digest_file);

int digest_file_by_name(const char *algo, const char *filename,
			unsigned char *hash,
			const unsigned char *sig)
{
	struct digest *d;
	int ret;

	d = digest_alloc(algo);
	if (!d)
		return -EIO;

	ret = digest_file(d, filename, hash, sig);
	digest_free(d);
	return ret;
}
EXPORT_SYMBOL_GPL(digest_file_by_name);
