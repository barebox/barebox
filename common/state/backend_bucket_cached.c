/*
 * Copyright (C) 2016 Pengutronix, Markus Pargmann <mpa@pengutronix.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <common.h>
#include "state.h"

struct state_backend_storage_bucket_cache {
	struct state_backend_storage_bucket bucket;

	struct state_backend_storage_bucket *raw;

	u8 *data;
	ssize_t data_len;
	bool force_write;

	/* For outputs */
	struct device_d *dev;
};

static inline struct state_backend_storage_bucket_cache
    *get_bucket_cache(struct state_backend_storage_bucket *bucket)
{
	return container_of(bucket,
			    struct state_backend_storage_bucket_cache,
			    bucket);
}

static inline void state_backend_bucket_cache_drop(
		struct state_backend_storage_bucket_cache *cache)
{
	if (cache->data) {
		free(cache->data);
		cache->data = NULL;
		cache->data_len = 0;
	}
}

static int state_backend_bucket_cache_fill(
		struct state_backend_storage_bucket_cache *cache)
{
	int ret;

	ret = cache->raw->read(cache->raw, &cache->data, &cache->data_len);
	if (ret == -EUCLEAN) {
		cache->force_write = true;
		ret = 0;
	}

	return ret;
}

static int state_backend_bucket_cache_read(struct state_backend_storage_bucket *bucket,
					   uint8_t ** buf_out,
					   ssize_t * len_hint)
{
	struct state_backend_storage_bucket_cache *cache =
			get_bucket_cache(bucket);
	int ret;

	if (!cache->data) {
		ret = state_backend_bucket_cache_fill(cache);
		if (ret)
			return ret;
	}

	if (cache->data) {
		*buf_out = xmemdup(cache->data, cache->data_len);
		if (!*buf_out)
			return -ENOMEM;
		*len_hint = cache->data_len;
	}

	return 0;
}

static int state_backend_bucket_cache_write(struct state_backend_storage_bucket *bucket,
					    const uint8_t * buf, ssize_t len)
{
	struct state_backend_storage_bucket_cache *cache =
			get_bucket_cache(bucket);
	int ret;

	if (!cache->force_write) {
		if (!cache->data)
			ret = state_backend_bucket_cache_fill(cache);

		if (cache->data_len == len && !memcmp(cache->data, buf, len))
			return 0;
	}

	state_backend_bucket_cache_drop(cache);

	ret = cache->raw->write(cache->raw, buf, len);
	if (ret)
		return ret;

	cache->data = xmemdup(buf, len);
	cache->data_len = len;
	return 0;
}

static void state_backend_bucket_cache_free(
		struct state_backend_storage_bucket *bucket)
{
	struct state_backend_storage_bucket_cache *cache =
			get_bucket_cache(bucket);

	state_backend_bucket_cache_drop(cache);
	cache->raw->free(cache->raw);
	free(cache);
}

int state_backend_bucket_cached_create(struct device_d *dev,
				       struct state_backend_storage_bucket *raw,
				       struct state_backend_storage_bucket **out)
{
	struct state_backend_storage_bucket_cache *cache;

	cache = xzalloc(sizeof(*cache));
	cache->raw = raw;
	cache->dev = dev;

	cache->bucket.free = state_backend_bucket_cache_free;
	cache->bucket.read = state_backend_bucket_cache_read;
	cache->bucket.write = state_backend_bucket_cache_write;

	*out = &cache->bucket;

	return 0;
}
