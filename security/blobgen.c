/*
 * Copyright (C) 2016 Pengutronix, Steffen Trumtrar <kernel@pengutronix.de>
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
#include <blobgen.h>
#include <base64.h>
#include <malloc.h>
#include <crypto.h>
#include <dma.h>
#include <environment.h>

static LIST_HEAD(blobs);
static struct blobgen *bg_default;

/**
 * blob_gen_register - register a blob device
 * @dev: The parent device
 * @bg: The blobgen device
 *
 * This registers a blob device. Returns 0 for success or a negative error
 * code otherwise.
 */
int blob_gen_register(struct device *dev, struct blobgen *bg)
{
	int ret;

	dev_set_name(&bg->dev, "blob");
	bg->dev.parent = dev;

	ret = register_device(&bg->dev);
	if (ret)
		return ret;

	list_add_tail(&bg->list, &blobs);

	if (!bg_default)
		bg_default = bg;

	return 0;
}

/**
 * blobgen_get - get a blob generator of given name
 * @name: The name of the blob generator to look for or NULL
 *
 * Finds a blob generator by name and returns it.
 * If name is NULL, returns the first blob generator encountered.
 * Returns NULL if none is found.
 */
struct blobgen *blobgen_get(const char *name)
{
	struct device *dev = NULL;
	struct blobgen *bg;

	if (!name)
		return bg_default;

	if (name) {
		dev = get_device_by_name(name);
		if (!dev)
			return NULL;
	}

	list_for_each_entry(bg, &blobs, list) {
		if (!dev || dev == &bg->dev)
			return bg;
	}

	return NULL;
}

/**
 * blob_encrypt - encrypt a data blob
 * @bg: The blob generator to use
 * @modifier: Modifier string
 * @plain: The plaintext input
 * @plainsize: Length of the plain data in bytes
 * @retblob: The encrypted blob is returned here
 * @blobsize: The returned length of the encrypted blob
 *
 * This encrypts a blob passed in @plain to an allocated buffer returned in
 * @retblob. Returns 0 for success or a negative error code otherwise. @retblob
 * is valid when the function returns successfully. The caller must free the
 * buffer after use.
 */
int blob_encrypt(struct blobgen *bg, const char *modifier, const void *plain,
		 int plainsize, void **retblob, int *blobsize)
{
	void *blob;
	int ret;

	if (plainsize > bg->max_payload_size)
		return -EINVAL;

	pr_debug("%s plain:\n", __func__);
	pr_memory_display(MSG_DEBUG, plain, 0, plainsize, 1, 0);

	blob = dma_alloc(MAX_BLOB_LEN);
	if (!blob)
		return -ENOMEM;

	ret = bg->encrypt(bg, modifier, plain, plainsize, blob, blobsize);

	if (ret) {
		free(blob);
	} else {
		pr_debug("%s encrypted:\n", __func__);
		pr_memory_display(MSG_DEBUG, blob, 0, *blobsize, 1, 0);
		*retblob = blob;
	}

	return ret;
}

/**
 * blob_encrypt_to_env -  encrypt blob to environment variable
 * @bg: The blob generator to use
 * @modifier: Modifier string
 * @plain: The plaintext input
 * @plainsize: Length of the plain data in bytes
 * @varname: Name of the variable to set with the output blob
 *
 * This uses blob_encrypt to encrypt a blob. The result is base64 encoded and
 * written to the environment variable @varname. Returns 0 for success or a
 * negative error code otherwise.
 */
int blob_encrypt_to_env(struct blobgen *bg, const char *modifier,
			const void *plain, int plainsize, const char *varname)
{
	int ret;
	int blobsize;
	void *blob;
	char *value;

	ret = blob_encrypt(bg, modifier, plain, plainsize, &blob, &blobsize);
	if (ret)
		return ret;

	value = malloc(BASE64_LENGTH(blobsize) + 1);
	if (!value)
		return -ENOMEM;

	uuencode(value, blob, blobsize);

	pr_debug("%s encrypted base64: \"%s\"\n", __func__, value);

	ret = setenv(varname, value);

	free(value);
	free(blob);

	return ret;
}

/**
 * blob_decrypt - decrypt a blob
 * @bg: The blob generator to use
 * @modifier: Modifier string
 * @blob: The encrypted blob
 * @blobsize: Size of the encrypted blob
 * @plain: Plaintext is returned here
 * @plainsize: Size of the data returned in bytes
 *
 * This function decrypts a blob generated with blob_encrypt. @modifier must match
 * the modifier used to encrypt the data. Returns 0 when the data could be
 * decrypted successfully or a negative error code otherwise.
 */
int blob_decrypt(struct blobgen *bg, const char *modifier, const void *blob,
		 int blobsize, void **plain, int *plainsize)
{
	int ret;

	pr_debug("%s encrypted:\n", __func__);
	pr_memory_display(MSG_DEBUG, blob, 0, blobsize, 1, 0);

	ret = bg->decrypt(bg, modifier, blob, blobsize, plain, plainsize);

	if (!ret) {
		pr_debug("%s decrypted:\n", __func__);
		pr_memory_display(MSG_DEBUG, *plain, 0, *plainsize, 1, 0);
	}

	return ret;
}

/**
 * blob_decrypt_from_base64 - decrypt a base64 encoded blob
 * @bg: The blob generator to use
 * @modifier: Modifier string
 * @encrypted: base64 encoded encrypted data
 * @plain: Plaintext is returned here
 * @plainsize: Size of the data returned in bytes
 *
 * like blob_decrypt, but takes the encrypted data as a base64 encoded string.
 * Returns 0 when the data could be decrypted successfully or a negative error
 * code otherwise.
 */
int blob_decrypt_from_base64(struct blobgen *bg, const char *modifier,
			     const char *encrypted, void **plain,
			     int *plainsize)
{
	char *data;
	int ret, len;

	data = dma_alloc(MAX_BLOB_LEN);
	if (!data)
		return -ENOMEM;

	pr_debug("encrypted base64: \"%s\"\n", encrypted);

	len = decode_base64(data, MAX_BLOB_LEN, encrypted);

	ret = blob_decrypt(bg, modifier, data, len, plain, plainsize);

	free(data);

	return ret;
}
