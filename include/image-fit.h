/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) Jan Lübbe, 2014
 */

#ifndef __IMAGE_FIT_H__
#define __IMAGE_FIT_H__

#include <linux/types.h>
#include <linux/refcount.h>
#include <bootm.h>

struct fit_handle {
	const void *fit;
	void *fit_alloc;
	size_t size;
	char *filename;

	struct list_head entry;
	refcount_t users;

	bool verbose;
	enum bootm_verify verify;

	struct device_node *root;
	struct device_node *images;
	struct device_node *configurations;
};

static inline struct fit_handle *fit_open_handle(struct fit_handle *handle)
{
	if (handle)
		refcount_inc(&handle->users);
	return handle;
}

struct fit_handle *fit_open(const char *filename, bool verbose,
			    enum bootm_verify verify);
struct fit_handle *fit_open_buf(const void *buf, size_t len, bool verbose,
				enum bootm_verify verify);
void *fit_open_configuration(struct fit_handle *handle, const char *name,
			     bool (*match_valid)(struct fit_handle *handle,
						 struct device_node *config));
/**
 * fit_count_images() - count images of a given type in a FIT configuration
 * @handle: FIT handle as returned by fit_open() or fit_open_buf()
 * @configuration: configuration node as returned by fit_open_configuration()
 * @name: image type property name (e.g., "kernel", "fdt", "ramdisk")
 *
 * Return: number of images on success, negative error code on failure
 */
int fit_count_images(struct fit_handle *handle, void *configuration,
		     const char *name);

/**
 * fit_has_image() - check if a FIT configuration contains an image type
 * @handle: FIT handle as returned by fit_open() or fit_open_buf()
 * @configuration: configuration node as returned by fit_open_configuration()
 * @name: image type property name (e.g., "kernel", "fdt", "ramdisk")
 *
 * Return: true if at least one image of the given type exists, false otherwise
 */
static inline bool fit_has_image(struct fit_handle *handle, void *configuration,
				 const char *name)
{
	return fit_count_images(handle, configuration, name) > 0;
}

int fit_open_image(struct fit_handle *handle, void *configuration,
		   const char *name, int idx,
		   const void **outdata, unsigned long *outsize);
int fit_get_image_address(struct fit_handle *handle, void *configuration,
			  const char *name, const char *property,
			  unsigned long *address);
int fit_config_verify_signature(struct fit_handle *handle, struct device_node *conf_node);
const char *fit_config_get_name(struct fit_handle *handle, void *config);

void fit_close(struct fit_handle *handle);

#endif	/* __IMAGE_FIT_H__ */
