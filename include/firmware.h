/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2013 Juergen Beisert <kernel@pengutronix.de>, Pengutronix
 */

#ifndef FIRMWARE_H
#define FIRMWARE_H

#include <pbl.h>
#include <printf.h>
#include <types.h>
#include <driver.h>
#include <debug_ll.h>
#include <linux/kernel.h>
#include <asm/sections.h>
#include <crypto/sha.h>
#include <crypto.h>

struct firmware {
	size_t size;
	const u8 *data;
};

struct firmware_handler {
	char *id; /* unique identifier for this firmware device */
	char *model; /* description for this device */
	struct device *dev;
	void *priv;
	struct device_node *device_node;
	/* called once to prepare the firmware's programming cycle */
	int (*open)(struct firmware_handler*);
	/* called multiple times to program the firmware with the given data */
	int (*write)(struct firmware_handler*, const void*, size_t);
	/* called once to finish programming cycle */
	int (*close)(struct firmware_handler*);
};

struct firmware_mgr;

int firmwaremgr_register(struct firmware_handler *);

struct firmware_mgr *firmwaremgr_find(const char *);
#ifdef CONFIG_FIRMWARE
struct firmware_mgr *firmwaremgr_find_by_node(struct device_node *np);
int firmwaremgr_load_file(struct firmware_mgr *, const char *path);
char *firmware_get_searchpath(void);
void firmware_set_searchpath(const char *path);
int request_firmware(const struct firmware **fw, const char *fw_name, struct device *dev);
void release_firmware(const struct firmware *fw);
#else
static inline struct firmware_mgr *firmwaremgr_find_by_node(struct device_node *np)
{
	return NULL;
}

static inline int firmwaremgr_load_file(struct firmware_mgr *mgr, const char *path)
{
	return -ENOSYS;
}

static inline char *firmware_get_searchpath(void)
{
	return NULL;
}

static inline void firmware_set_searchpath(const char *path)
{
}

static inline int request_firmware(const struct firmware **fw, const char *fw_name,
				   struct device *dev)
{
	return -EINVAL;
}

static inline void release_firmware(const struct firmware *fw)
{
}
#endif

void firmwaremgr_list_handlers(void);

static inline void firmware_ext_verify(const void *data_start, size_t data_size,
				       const void *hash_start, size_t hash_size)
{
	if (pbl_barebox_verify(data_start, data_size,
			       hash_start, hash_size) != 0) {
		putc_ll('!');
		panic("hash mismatch, refusing to decompress");
	}
}

#define __get_builtin_firmware(name, offset, start, size)		\
	do {								\
		extern char _fw_##name##_start[];			\
		extern char _fw_##name##_end[];				\
		extern char _fw_##name##_sha_start[];			\
		extern char _fw_##name##_sha_end[];			\
		*start = (typeof(*start)) _fw_##name##_start;		\
		*size = _fw_##name##_end - _fw_##name##_start;		\
		if (!(offset))						\
			break;						\
		*start += (offset);					\
		firmware_ext_verify(					\
			*start, *size,					\
			_fw_##name##_sha_start,				\
			_fw_##name##_sha_end - _fw_##name##_sha_start	\
		);							\
	} while (0)


#define get_builtin_firmware(name, start, size) \
	__get_builtin_firmware(name, 0, start, size)

#define get_builtin_firmware_ext(name, base, start, size)		\
	__get_builtin_firmware(name, (long)base - (long)_text, start, size)

static inline int firmware_next_image_check_sha256(const void *hash, bool verbose)
{
	extern char _fw_next_image_bin_sha_start[];
	int hsize = SHA256_DIGEST_SIZE;
	int ret;

	if (!IS_ENABLED(CONFIG_FIRMWARE_NEXT_IMAGE))
		return -EINVAL;

	ret = crypto_memneq(hash, _fw_next_image_bin_sha_start, hsize);

	if (verbose) {
		if (ret) {
			pr_err("next image hash mismatch!\n");
			pr_err("expected: sha256=%*phN\n", hsize, _fw_next_image_bin_sha_start);
			pr_err("found:    sha256=%*phN\n", hsize, hash);
		} else {
			pr_info("hash sha256=%*phN OK\n", hsize, _fw_next_image_bin_sha_start);
		}
	}

	return ret;
}

#endif /* FIRMWARE_H */
