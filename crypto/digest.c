/*
 * (C) Copyright 2008-2010 Jean-Christophe PLAGNIOL-VILLARD <plagnioj@jcrosoft.com>
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
#include <crypto.h>
#include <crypto/internal.h>

static LIST_HEAD(digests);

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

	if (crypto_memneq(md, tmp, len))
		ret = -EINVAL;
	else
		ret = 0;
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
	struct digest_algo *d_by_name = NULL, *d_by_driver = NULL;
	struct digest_algo *tmp;
	int priority = -1;

	if (!name)
		return NULL;

	list_for_each_entry(tmp, &digests, list) {
		if (strcmp(tmp->base.driver_name, name) == 0)
			d_by_driver = tmp;

		if (strcmp(tmp->base.name, name) != 0)
			continue;

		if (tmp->base.priority <= priority)
			continue;

		d_by_name = tmp;
		priority = tmp->base.priority;
	}

	return d_by_name ?: d_by_driver;
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

static int digest_update_interruptible(struct digest *d, const void *data,
				       unsigned long len)
{
	if (ctrlc())
		return -EINTR;

	return digest_update(d, data, len);
}

static int digest_update_from_fd(struct digest *d, int fd,
				 loff_t start, loff_t size)
{
	unsigned char *buf = xmalloc(PAGE_SIZE);
	int ret = 0;

	if (lseek(fd, start, SEEK_SET) != start) {
		perror("lseek");
		ret = -errno;
		goto out_free;
	}

	while (size) {
		unsigned long now = min_t(typeof(size), PAGE_SIZE, size);

		ret = read(fd, buf, now);
		if (ret < 0) {
			perror("read");
			goto out_free;
		}

		if (!ret)
			break;

		ret = digest_update_interruptible(d, buf, ret);
		if (ret)
			goto out_free;

		size -= now;
	}

out_free:
	free(buf);
	return ret;
}

int digest_file_window(struct digest *d, const char *filename,
		       unsigned char *hash,
		       const unsigned char *sig,
		       loff_t start, loff_t size)
{
	int fd, ret;

	ret = digest_init(d);
	if (ret)
		return ret;

	fd = open(filename, O_RDONLY);
	if (fd < 0) {
		perror(filename);
		return -errno;
	}

	ret = digest_update_from_fd(d, fd, start, size);
	if (ret)
		goto out;

	if (sig)
		ret = digest_verify(d, sig);
	else
		ret = digest_final(d, hash);
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

	if (stat(filename, &st))
		return -errno;

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
