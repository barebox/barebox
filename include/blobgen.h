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

#ifndef __BLOBGEN_H__
#define __BLOBGEN_H__

#include <common.h>

enum access_rights {
	KERNEL,
	KERNEL_EVM,
	USERSPACE,
};

#define KEYMOD_LENGTH		16
#define MAX_BLOB_LEN		4096
#define BLOCKSIZE_BYTES		8

struct blobgen {
	struct device dev;
	int (*encrypt)(struct blobgen *bg, const char *modifier,
		       const void *plain, int plainsize, void *blob,
		       int *blobsize);
	int (*decrypt)(struct blobgen *bg, const char *modifier,
		       const void *blob, int blobsize, void **plain,
		       int *plainsize);

	enum access_rights access;
	unsigned int max_payload_size;

	struct list_head list;
};

int blob_gen_register(struct device *dev, struct blobgen *bg);

#ifdef CONFIG_BLOBGEN
struct blobgen *blobgen_get(const char *name);
#else
static inline struct blobgen *blobgen_get(const char *name)
{
	return NULL;
}
#endif

int blob_encrypt(struct blobgen *blg, const char *modifier, const void *plain,
		 int plainsize, void **blob, int *blobsize);
int blob_encrypt_to_env(struct blobgen *blg, const char *modifier,
			const void *plain, int plainsize, const char *varname);
int blob_decrypt(struct blobgen *bg, const char *modifier, const void *blob,
		 int blobsize, void **plain, int *plainsize);
int blob_decrypt_from_base64(struct blobgen *blg, const char *modifier,
			     const char *encrypted, void **plain, int *plainsize);

#endif
