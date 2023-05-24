/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) Jan Lübbe, 2014
 */

#ifndef __IMAGE_FIT_H__
#define __IMAGE_FIT_H__

#include <linux/types.h>
#include <bootm.h>

struct fit_handle {
	const void *fit;
	void *fit_alloc;
	size_t size;

	bool verbose;
	enum bootm_verify verify;

	struct device_node *root;
	struct device_node *images;
	struct device_node *configurations;
};

struct fit_handle *fit_open(const char *filename, bool verbose,
			    enum bootm_verify verify, loff_t max_size);
struct fit_handle *fit_open_buf(const void *buf, size_t len, bool verbose,
				enum bootm_verify verify);
void *fit_open_configuration(struct fit_handle *handle, const char *name);
int fit_has_image(struct fit_handle *handle, void *configuration,
		  const char *name);
int fit_open_image(struct fit_handle *handle, void *configuration,
		   const char *name, const void **outdata,
		   unsigned long *outsize);
int fit_get_image_address(struct fit_handle *handle, void *configuration,
			  const char *name, const char *property,
			  unsigned long *address);

void fit_close(struct fit_handle *handle);

#endif	/* __IMAGE_FIT_H__ */
