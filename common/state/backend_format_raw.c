/*
 * Copyright (C) 2012-2014 Pengutronix, Jan Luebbe <j.luebbe@pengutronix.de>
 * Copyright (C) 2013-2014 Pengutronix, Sascha Hauer <s.hauer@pengutronix.de>
 * Copyright (C) 2015 Pengutronix, Marc Kleine-Budde <mkl@pengutronix.de>
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
#include <common.h>
#include <crypto/keystore.h>
#include <digest.h>
#include <linux/kernel.h>
#include <malloc.h>
#include <crc.h>
#include <of.h>
#include <crc.h>

#include "state.h"

struct state_backend_format_raw {
	struct state_backend_format format;

	struct digest *digest;
	unsigned int digest_length;

	/* For outputs */
	struct device_d *dev;

	char *secret_name;
	int needs_secret;
	char *algo;
};

struct __attribute__((__packed__)) backend_raw_header {
	uint32_t magic;
	uint16_t reserved;
	uint16_t data_len;
	uint32_t data_crc;
	uint32_t header_crc;
};

const int format_raw_min_length = sizeof(struct backend_raw_header);

static inline struct state_backend_format_raw *get_format_raw(
		struct state_backend_format *format)
{
	return container_of(format, struct state_backend_format_raw, format);
}

static int backend_raw_digest_init(struct state_backend_format_raw *raw)
{
	const unsigned char *key;
	int key_len;
	int ret;

	if (!raw->digest) {
		raw->digest = digest_alloc(raw->algo);
		if (!raw->digest) {
			dev_err(raw->dev, "algo %s not found\n",
				raw->algo);
			return -ENODEV;
		}
		raw->digest_length = digest_length(raw->digest);
	}

	ret = keystore_get_secret(raw->secret_name, &key, &key_len);
	if (ret) {
		dev_err(raw->dev, "Could not get secret '%s'\n",
			raw->secret_name);
		return ret;
	}

	ret = digest_set_key(raw->digest, key, key_len);
	if (ret)
		return ret;

	ret = digest_init(raw->digest);
	if (ret) {
		dev_err(raw->dev, "Failed to initialize digest: %s\n",
			strerror(-ret));
		return ret;
	}

	return 0;
}

static int backend_format_raw_verify(struct state_backend_format *format,
				     uint32_t magic, const void * buf,
				     ssize_t *lenp, enum state_flags flags)
{
	uint32_t crc;
	struct backend_raw_header *header;
	int d_len = 0;
	int ret;
	const void *data;
	ssize_t len = *lenp;
	struct state_backend_format_raw *backend_raw = get_format_raw(format);
	ssize_t complete_len;

	if (len < format_raw_min_length) {
		dev_err(backend_raw->dev, "Error, buffer length (%zd) is shorter than the minimum required header length\n",
			len);
		return -EINVAL;
	}

	header = (struct backend_raw_header *)buf;
	crc = crc32(0, header, sizeof(*header) - sizeof(uint32_t));
	if (crc != header->header_crc) {
		dev_err(backend_raw->dev, "Error, invalid header crc in raw format, calculated 0x%08x, found 0x%08x\n",
			crc, header->header_crc);
		return -EINVAL;
	}

	if (magic && magic != header->magic) {
		dev_err(backend_raw->dev, "Error, invalid magic in raw format 0x%08x, should be 0x%08x\n",
			header->magic, magic);
		return -EINVAL;
	}

	if (backend_raw->algo && !(flags & STATE_FLAG_NO_AUTHENTIFICATION)) {
		ret = backend_raw_digest_init(backend_raw);
		if (ret)
			return ret;

		d_len = digest_length(backend_raw->digest);
	}

	complete_len = header->data_len + d_len + format_raw_min_length;
	if (complete_len > len) {
		dev_err(backend_raw->dev, "Error, invalid data_len %u in header, have data of len %zu\n",
			header->data_len, len);
		return -EINVAL;
	}

	data = buf + sizeof(*header);

	crc = crc32(0, data, header->data_len);
	if (crc != header->data_crc) {
		dev_err(backend_raw->dev, "invalid data crc, calculated 0x%08x, found 0x%08x\n",
			crc, header->data_crc);
		return -EINVAL;
	}

	*lenp = header->data_len + sizeof(*header);

	if (backend_raw->algo && !(flags & STATE_FLAG_NO_AUTHENTIFICATION)) {
		const void *hmac = data + header->data_len;

		/* hmac over header and data */
		ret = digest_update(backend_raw->digest, buf, sizeof(*header) + header->data_len);
		if (ret) {
			dev_err(backend_raw->dev, "Failed to update digest, %d\n",
				ret);
			return ret;
		}

		ret = digest_verify(backend_raw->digest, hmac);
		if (ret < 0) {
			dev_err(backend_raw->dev, "Failed to verify data, hmac, %d\n",
				ret);
			return ret;
		}
	}

	return 0;
}

static int backend_format_raw_unpack(struct state_backend_format *format,
				     struct state *state, const void * buf,
				     ssize_t len)
{
	struct state_variable *sv;
	const struct backend_raw_header *header;
	const void *data;
	struct state_backend_format_raw *backend_raw = get_format_raw(format);

	header = (const struct backend_raw_header *)buf;
	data = buf + sizeof(*header);

	list_for_each_entry(sv, &state->variables, list) {
		if (sv->start + sv->size > header->data_len) {
			dev_err(backend_raw->dev, "State variable ends behind valid data, %s\n",
				sv->name);
			continue;
		}
		memcpy(sv->raw, data + sv->start, sv->size);
	}

	return 0;
}

static int backend_format_raw_pack(struct state_backend_format *format,
				   struct state *state, void ** buf_out,
				   ssize_t * len_out)
{
	struct state_backend_format_raw *backend_raw = get_format_raw(format);
	void *buf, *data, *hmac;
	struct backend_raw_header *header;
	struct state_variable *sv;
	unsigned int size_full;
	unsigned int size_data;
	int ret;

	if (backend_raw->algo) {
		ret = backend_raw_digest_init(backend_raw);
		if (ret)
			return ret;
	}

	sv = list_last_entry(&state->variables, struct state_variable, list);
	size_data = sv->start + sv->size;
	size_full = size_data + sizeof(*header) + backend_raw->digest_length;

	buf = xzalloc(size_full);

	header = buf;
	data = buf + sizeof(*header);
	hmac = data + size_data;

	list_for_each_entry(sv, &state->variables, list)
		memcpy(data + sv->start, sv->raw, sv->size);

	header->magic = state->magic;
	header->data_len = size_data;
	header->data_crc = crc32(0, data, size_data);
	header->header_crc = crc32(0, header,
				   sizeof(*header) - sizeof(uint32_t));

	if (backend_raw->algo) {
		/* hmac over header and data */
		ret = digest_update(backend_raw->digest, buf, sizeof(*header) + size_data);
		if (ret) {
			dev_err(backend_raw->dev, "Failed to update digest for packing, %d\n",
				ret);
			goto out_free;
		}

		ret = digest_final(backend_raw->digest, hmac);
		if (ret < 0) {
			dev_err(backend_raw->dev, "Failed to finish digest for packing, %d\n",
				ret);
			goto out_free;
		}
	}

	*buf_out = buf;
	*len_out = size_full;

	return 0;

out_free:
	free(buf);

	return ret;
}

static void backend_format_raw_free(struct state_backend_format *format)
{
	struct state_backend_format_raw *backend_raw = get_format_raw(format);

	free(backend_raw);
}

static int backend_format_raw_init_digest(struct state_backend_format_raw *raw,
					  struct device_node *root,
					  const char *secret_name)
{
	struct property *p;
	const char *algo;
	int ret;

	p = of_find_property(root, "algo", NULL);
	if (!p)			/* does not exist */
		return 0;

	ret = of_property_read_string(root, "algo", &algo);
	if (ret)
		return ret;

	if (!IS_ENABLED(CONFIG_STATE_CRYPTO) && IS_ENABLED(__BAREBOX__)) {
		dev_err(raw->dev, "algo %s specified, but crypto support for state framework (CONFIG_STATE_CRYPTO) not enabled.\n",
			algo);
		return -EINVAL;
	}

	raw->algo = xstrdup(algo);

	return 0;
}

int backend_format_raw_create(struct state_backend_format **format,
			      struct device_node *node, const char *secret_name,
			      struct device_d *dev)
{
	struct state_backend_format_raw *raw;
	int ret;

	raw = xzalloc(sizeof(*raw));

	raw->dev = dev;
	ret = backend_format_raw_init_digest(raw, node, secret_name);
	if (ret) {
		dev_err(raw->dev, "Failed initializing digest for raw format, %d\n",
			ret);
		free(raw);
		return ret;
	}

	raw->secret_name = xstrdup(secret_name);
	raw->format.pack = backend_format_raw_pack;
	raw->format.unpack = backend_format_raw_unpack;
	raw->format.verify = backend_format_raw_verify;
	raw->format.free = backend_format_raw_free;
	raw->format.name = "raw";
	*format = &raw->format;

	return 0;
}

struct digest *state_backend_format_raw_get_digest(struct state_backend_format
						   *format)
{
	struct state_backend_format_raw *backend_raw = get_format_raw(format);

	return backend_raw->digest;
}
